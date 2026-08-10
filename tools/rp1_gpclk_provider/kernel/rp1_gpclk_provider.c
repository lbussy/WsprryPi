// SPDX-License-Identifier: GPL-2.0
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rp1-gpclk-lease.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include "rp1-gpclk-contract.h"

#define TICKS_DMA0_CTRL 0x0
#define TICKS_DMA0_CYCLES 0x4
#define DMA_TICK0_EN 0x0
#define DMA_TICK0_CTRL 0x4
#define DMA_TICK_REQ BIT(0)
#define DMA_TICK_SINGLE BIT(1)
#define DMA_TICK_FINISH_CLEAR BIT(0)
#define DMA_TICK_DWELL (19U << 4)

struct rp1_gpclk_provider {
	struct device *dev;
	struct miscdevice misc;
	struct clk *clk;
	struct dma_chan *dma;
	struct rp1_gpclk_dma_lease lease;
	void __iomem *ticks;
	void __iomem *dma_tick;
	u32 *words;
	dma_addr_t words_dma;
	struct mutex lock;
	struct file *owner;
	u64 generation;
	u32 state;
	u32 expected_final;
	bool lease_held;
	bool submitted;
	bool release_pending;
	struct delayed_work verify_work;
};

static void stop_tick(struct rp1_gpclk_provider *provider)
{
	writel(0, provider->dma_tick + DMA_TICK0_EN);
	writel(0, provider->ticks + TICKS_DMA0_CTRL);
}

static void verify_completion(struct work_struct *work)
{
	struct rp1_gpclk_provider *provider = container_of(to_delayed_work(work),
		struct rp1_gpclk_provider, verify_work);
	u32 div_int, div_frac;
	int ret;

	stop_tick(provider);
	ret = rp1_gpclk_dma_lease_read(&provider->lease, &div_int, &div_frac);
	mutex_lock(&provider->lock);
	provider->submitted = false;
	provider->state = ret || div_int != 3 ||
		div_frac != provider->expected_final ?
		RP1_GPCLK_STATE_FAILED : RP1_GPCLK_STATE_COMPLETE;
	if (provider->release_pending) {
		rp1_gpclk_dma_lease_put(provider->clk, &provider->lease);
		provider->lease_held = false;
		provider->release_pending = false;
		provider->owner = NULL;
	}
	mutex_unlock(&provider->lock);
}

static void transfer_done(void *argument)
{
	struct rp1_gpclk_provider *provider = argument;

	stop_tick(provider);
	schedule_delayed_work(&provider->verify_work, msecs_to_jiffies(50));
}

static int submit_program(struct rp1_gpclk_provider *provider,
	const struct rp1_gpclk_program *request)
{
	struct dma_slave_config config = {};
	struct dma_async_tx_descriptor *descriptor;
	u64 accumulator = 0;
	dma_cookie_t cookie;
	u32 lower, upper;
	unsigned int i;
	int ret;

	if (!rp1_gpclk_valid_header(request->version, request->size,
			sizeof(*request)))
		return -EPROTO;
	if (!rp1_gpclk_valid_program(request, provider->generation))
		return -EINVAL;
	if (provider->state == RP1_GPCLK_STATE_RUNNING ||
		provider->state == RP1_GPCLK_STATE_DRAINING)
		return -EBUSY;
	lower = rp1_gpclk_pack_fraction(request->lower_divider_word);
	upper = rp1_gpclk_pack_fraction(request->upper_divider_word);
	for (i = 0; i < RP1_GPCLK_WRITES_PER_SYMBOL; ++i) {
		accumulator += request->lower_count;
		if (accumulator >= RP1_GPCLK_WRITES_PER_SYMBOL) {
			provider->words[i] = lower;
			accumulator -= RP1_GPCLK_WRITES_PER_SYMBOL;
		} else {
			provider->words[i] = upper;
		}
	}
	ret = rp1_gpclk_dma_lease_configure(&provider->lease, 3);
	if (ret)
		return ret;
	config.direction = DMA_MEM_TO_DEV;
	config.dst_addr = provider->lease.divider_dma_addr;
	config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	config.dst_maxburst = 1;
	ret = dmaengine_slave_config(provider->dma, &config);
	if (ret)
		return ret;
	descriptor = dmaengine_prep_slave_single(provider->dma,
		provider->words_dma, RP1_GPCLK_WRITES_PER_SYMBOL * sizeof(u32),
		DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!descriptor)
		return -EIO;
	descriptor->callback = transfer_done;
	descriptor->callback_param = provider;
	cookie = dmaengine_submit(descriptor);
	ret = dma_submit_error(cookie);
	if (ret)
		return ret;
	provider->generation = request->generation;
	provider->expected_final = provider->words[RP1_GPCLK_WRITES_PER_SYMBOL - 1];
	provider->state = RP1_GPCLK_STATE_RUNNING;
	provider->submitted = true;
	writel(RP1_GPCLK_TICK_DIVIDER, provider->ticks + TICKS_DMA0_CYCLES);
	writel(DMA_TICK_FINISH_CLEAR | DMA_TICK_DWELL,
		provider->dma_tick + DMA_TICK0_CTRL);
	dma_async_issue_pending(provider->dma);
	writel(DMA_TICK_REQ | DMA_TICK_SINGLE,
		provider->dma_tick + DMA_TICK0_EN);
	writel(1, provider->ticks + TICKS_DMA0_CTRL);
	return 0;
}

static long provider_ioctl(struct file *file, unsigned int command,
	unsigned long argument)
{
	struct rp1_gpclk_provider *provider = file->private_data;
	void __user *user = (void __user *)argument;
	struct rp1_gpclk_generation generation;
	struct rp1_gpclk_acquire acquire;
	struct rp1_gpclk_program program;
	long ret = 0;

	mutex_lock(&provider->lock);
	switch (command) {
	case RP1_GPCLK_IOC_ACQUIRE:
		if (copy_from_user(&acquire, user, sizeof(acquire))) { ret = -EFAULT; break; }
		if (!rp1_gpclk_valid_header(acquire.version, acquire.size, sizeof(acquire))) { ret = -EPROTO; break; }
		if (acquire.flags || acquire.reserved || !rp1_gpclk_valid_drive(acquire.drive_ma)) { ret = -EINVAL; break; }
		if (provider->owner) { ret = -EBUSY; break; }
		ret = rp1_gpclk_dma_lease_get(provider->clk, provider->dma->device->dev,
			&provider->lease);
		if (!ret) { provider->owner = file; provider->lease_held = true; provider->state = RP1_GPCLK_STATE_IDLE; }
		break;
	case RP1_GPCLK_IOC_SUBMIT:
		if (provider->owner != file) { ret = -EPERM; break; }
		if (copy_from_user(&program, user, sizeof(program))) { ret = -EFAULT; break; }
		ret = submit_program(provider, &program);
		break;
	case RP1_GPCLK_IOC_STOP:
	case RP1_GPCLK_IOC_STATE:
		if (provider->owner != file) { ret = -EPERM; break; }
		if (copy_from_user(&generation, user, sizeof(generation))) { ret = -EFAULT; break; }
		if (!rp1_gpclk_valid_header(generation.version, generation.size, sizeof(generation))) { ret = -EPROTO; break; }
		if (generation.generation != provider->generation) { ret = -ESTALE; break; }
		if (command == RP1_GPCLK_IOC_STOP && provider->state == RP1_GPCLK_STATE_RUNNING)
			provider->state = RP1_GPCLK_STATE_DRAINING;
		generation.state = provider->state;
		if (command == RP1_GPCLK_IOC_STATE && copy_to_user(user, &generation, sizeof(generation))) ret = -EFAULT;
		break;
	case RP1_GPCLK_IOC_RELEASE:
		if (provider->owner != file) { ret = -EPERM; break; }
		if (provider->state == RP1_GPCLK_STATE_RUNNING || provider->state == RP1_GPCLK_STATE_DRAINING) { ret = -EBUSY; break; }
		if (provider->lease_held) rp1_gpclk_dma_lease_put(provider->clk, &provider->lease);
		provider->lease_held = false; provider->owner = NULL; provider->state = RP1_GPCLK_STATE_IDLE;
		break;
	default:
		ret = -ENOTTY;
	}
	mutex_unlock(&provider->lock);
	return ret;
}

static int provider_open(struct inode *inode, struct file *file)
{
	struct miscdevice *misc = file->private_data;
	file->private_data = container_of(misc, struct rp1_gpclk_provider, misc);
	return nonseekable_open(inode, file);
}

static int provider_release_file(struct inode *inode, struct file *file)
{
	struct rp1_gpclk_provider *provider = file->private_data;
	mutex_lock(&provider->lock);
	if (provider->owner == file && provider->state != RP1_GPCLK_STATE_RUNNING &&
		provider->state != RP1_GPCLK_STATE_DRAINING) {
		if (provider->lease_held) rp1_gpclk_dma_lease_put(provider->clk, &provider->lease);
		provider->lease_held = false; provider->owner = NULL;
	} else if (provider->owner == file) {
		provider->release_pending = true;
		provider->state = RP1_GPCLK_STATE_DRAINING;
	}
	mutex_unlock(&provider->lock);
	return 0;
}

static const struct file_operations provider_fops = {
	.owner = THIS_MODULE, .open = provider_open, .release = provider_release_file,
	.unlocked_ioctl = provider_ioctl, .compat_ioctl = compat_ptr_ioctl,
	.llseek = noop_llseek,
};

static int rp1_gpclk_provider_probe(struct platform_device *pdev)
{
	struct rp1_gpclk_provider *provider;
	int ret;

	provider = devm_kzalloc(&pdev->dev, sizeof(*provider), GFP_KERNEL);
	if (!provider) return -ENOMEM;
	provider->dev = &pdev->dev; mutex_init(&provider->lock);
	INIT_DELAYED_WORK(&provider->verify_work, verify_completion);
	provider->clk = devm_clk_get(&pdev->dev, "gpclk");
	if (IS_ERR(provider->clk)) return PTR_ERR(provider->clk);
	provider->ticks = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(provider->ticks)) return PTR_ERR(provider->ticks);
	provider->dma_tick = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(provider->dma_tick)) return PTR_ERR(provider->dma_tick);
	provider->dma = dma_request_chan(&pdev->dev, "tick0");
	if (IS_ERR(provider->dma)) return PTR_ERR(provider->dma);
	provider->words = dma_alloc_coherent(provider->dma->device->dev,
		RP1_GPCLK_WRITES_PER_SYMBOL * sizeof(u32), &provider->words_dma, GFP_KERNEL);
	if (!provider->words) { ret = -ENOMEM; goto release_dma; }
	provider->misc.minor = MISC_DYNAMIC_MINOR; provider->misc.name = "rp1-gpclk0";
	provider->misc.fops = &provider_fops; provider->misc.parent = &pdev->dev;
	ret = misc_register(&provider->misc);
	if (ret) goto free_words;
	platform_set_drvdata(pdev, provider);
	return 0;
free_words:
	dma_free_coherent(provider->dma->device->dev,
		RP1_GPCLK_WRITES_PER_SYMBOL * sizeof(u32), provider->words, provider->words_dma);
release_dma:
	dma_release_channel(provider->dma);
	return ret;
}

static void rp1_gpclk_provider_remove(struct platform_device *pdev)
{
	struct rp1_gpclk_provider *provider = platform_get_drvdata(pdev);
	misc_deregister(&provider->misc); cancel_delayed_work_sync(&provider->verify_work);
	stop_tick(provider);
	if (provider->submitted) dmaengine_terminate_sync(provider->dma);
	if (provider->lease_held) rp1_gpclk_dma_lease_put(provider->clk, &provider->lease);
	dma_free_coherent(provider->dma->device->dev,
		RP1_GPCLK_WRITES_PER_SYMBOL * sizeof(u32), provider->words, provider->words_dma);
	dma_release_channel(provider->dma);
}

static const struct of_device_id provider_of_match[] = {
	{ .compatible = "raspberrypi,rp1-gpclk-provider" }, {}
};
MODULE_DEVICE_TABLE(of, provider_of_match);
static struct platform_driver provider_driver = {
	.probe = rp1_gpclk_provider_probe, .remove = rp1_gpclk_provider_remove,
	.driver = { .name = "rp1-gpclk-provider", .of_match_table = provider_of_match },
};
module_platform_driver(provider_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RP1 GPCLK provider-owned tick DMA UAPI");
