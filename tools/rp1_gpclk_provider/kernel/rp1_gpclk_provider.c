// SPDX-License-Identifier: GPL-2.0
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/rp1-gpclk-lease.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include "rp1-gpclk-contract.h"

#define TICKS_DMA0_CTRL 0x0
#define TICKS_DMA0_CYCLES 0x4
#define DMA_TICK0_EN 0x0
#define DMA_TICK0_CTRL 0x4
#define DMA_TICK_REQ BIT(0)
#define DMA_TICK_SINGLE BIT(1)
#define DMA_TICK_DWELL (19U << 4)
#define RP1_GPCLK_BUFFER_WRITES \
	((size_t)RP1_GPCLK_WRITES_PER_SYMBOL * RP1_GPCLK_WSPR_SYMBOL_COUNT)
#define RP1_GPCLK_BUFFER_BYTES (RP1_GPCLK_BUFFER_WRITES * sizeof(u32))
#define RP1_GPCLK_SYMBOL_BYTES \
		((size_t)RP1_GPCLK_WRITES_PER_SYMBOL * sizeof(u32))
#define RP1_GPCLK_EVENT_DEADLINE_SLACK_NS 5000000ULL
#define RP1_GPCLK_EVENT_START_LEAD_NS 1000000ULL

static bool live_output;
module_param(live_output, bool, 0444);
MODULE_PARM_DESC(live_output, "Permit provider-owned GPCLK0 output activation");

struct rp1_gpclk_provider {
	struct device *dev;
	struct miscdevice misc;
	struct clk *clk;
	struct dma_chan *dma;
	struct rp1_gpclk_dma_lease lease;
	struct pinctrl *pinctrl;
	struct pinctrl_state *safe_state;
	struct pinctrl_state *drive_states[4];
	void __iomem *ticks;
	void __iomem *dma_tick;
	u32 *words;
	dma_addr_t words_dma;
	struct mutex lock;
	struct file *owner;
	u64 generation;
	u32 state;
	u32 expected_final;
	u32 drive_index;
	bool lease_held;
	bool submitted;
	bool output_active;
	bool release_pending;
	u64 started_ns;
	bool timing_failed;
	struct delayed_work verify_work;
	struct hrtimer event_timer;
	spinlock_t event_lock;
	struct rp1_gpclk_event_program *event_program;
	u32 current_event;
	u32 terminal_reason;
	ktime_t event_deadline;
	bool event_submitted;
	bool event_armed;
	bool event_live;
	size_t event_write_count;
	struct work_struct event_cleanup_work;
};

static int deactivate_output(struct rp1_gpclk_provider *provider);
static void stop_tick(struct rp1_gpclk_provider *provider);

static void fail_event_locked(struct rp1_gpclk_provider *provider, u32 reason)
{
	rp1_gpclk_mark_event_failed(&provider->state,
		&provider->terminal_reason, &provider->event_submitted,
		&provider->event_armed, reason);
}

static int gate_event_output(struct rp1_gpclk_provider *provider, bool enabled)
{
	if (!provider->event_live)
		return 0;
	return rp1_gpclk_dma_lease_set_output(&provider->lease, enabled);
}

static enum hrtimer_restart fail_event_boundary(
	struct rp1_gpclk_provider *provider, unsigned long flags, u32 reason)
{
	fail_event_locked(provider, reason);
	stop_tick(provider);
	/* Best effort only: preserve the triggering failure if shutdown fails. */
	gate_event_output(provider, false);
	spin_unlock_irqrestore(&provider->event_lock, flags);
	schedule_work(&provider->event_cleanup_work);
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart event_deadline(struct hrtimer *timer)
{
	struct rp1_gpclk_provider *provider = container_of(timer,
		struct rp1_gpclk_provider, event_timer);
	unsigned long flags;
	ktime_t now = ktime_get();

	spin_lock_irqsave(&provider->event_lock, flags);
	if (!provider->event_submitted ||
		(!provider->event_armed &&
		 provider->state != RP1_GPCLK_STATE_RUNNING)) {
		spin_unlock_irqrestore(&provider->event_lock, flags);
		return HRTIMER_NORESTART;
	}
	if (ktime_after(now, ktime_add_ns(provider->event_deadline,
			RP1_GPCLK_EVENT_DEADLINE_SLACK_NS))) {
		return fail_event_boundary(provider, flags,
			RP1_GPCLK_TERMINAL_DEADLINE_MISSED);
	}
	if (provider->event_armed) {
		int ret = 0;

		provider->event_armed = false;
		if (provider->event_live) {
			ret = gate_event_output(provider,
				provider->event_program->events[0].flags &
					RP1_GPCLK_EVENT_RF_ON);
			if (!ret) {
				/* Publish the GPCLK gate before DMA can consume word zero. */
				wmb();
				writel(DMA_TICK_REQ | DMA_TICK_SINGLE,
					provider->dma_tick + DMA_TICK0_EN);
				writel(1, provider->ticks + TICKS_DMA0_CTRL);
			}
		}
		if (ret)
			return fail_event_boundary(provider, flags,
				RP1_GPCLK_TERMINAL_ADAPTER_FAILED);
		provider->state = RP1_GPCLK_STATE_RUNNING;
		provider->event_deadline = ktime_add_ns(provider->event_deadline,
			provider->event_program->events[0].duration_ns);
		hrtimer_set_expires(timer, provider->event_deadline);
		spin_unlock_irqrestore(&provider->event_lock, flags);
		return HRTIMER_RESTART;
	}
	if (++provider->current_event == provider->event_program->event_count) {
		stop_tick(provider);
		if (gate_event_output(provider, false))
			return fail_event_boundary(provider, flags,
				RP1_GPCLK_TERMINAL_ADAPTER_FAILED);
		provider->state = RP1_GPCLK_STATE_COMPLETE;
		provider->terminal_reason = RP1_GPCLK_TERMINAL_COMPLETE;
		provider->event_submitted = false;
		spin_unlock_irqrestore(&provider->event_lock, flags);
		schedule_work(&provider->event_cleanup_work);
		return HRTIMER_NORESTART;
	}
	if (gate_event_output(provider,
		provider->event_program->events[provider->current_event].flags &
			RP1_GPCLK_EVENT_RF_ON))
		return fail_event_boundary(provider, flags,
			RP1_GPCLK_TERMINAL_ADAPTER_FAILED);
	provider->event_deadline = ktime_add_ns(provider->event_deadline,
		provider->event_program->events[provider->current_event].duration_ns);
	hrtimer_set_expires(timer, provider->event_deadline);
	spin_unlock_irqrestore(&provider->event_lock, flags);
	return HRTIMER_RESTART;
}

static int prepare_event_words(struct rp1_gpclk_provider *provider,
	const struct rp1_gpclk_event_program *request)
{
	u64 accumulators[RP1_GPCLK_EVENT_MAX_TONES] = {};
	u64 elapsed_ns = 0, previous_writes = 0;
	size_t word_index = 0;
	u32 last_word = 0;
	u32 i;

	for (i = 0; i < request->event_count; ++i) {
		const struct rp1_gpclk_event *event = &request->events[i];
		u64 cumulative_writes, writes, j;

		elapsed_ns += event->duration_ns;
		cumulative_writes = rp1_gpclk_event_cumulative_writes(elapsed_ns);
		writes = cumulative_writes - previous_writes;
		previous_writes = cumulative_writes;
		for (j = 0; j < writes; ++j) {
			if (event->flags & RP1_GPCLK_EVENT_RF_ON) {
				const struct rp1_gpclk_symbol *tone =
					&request->tones[event->tone_index];
				u64 period = (u64)tone->lower_count + tone->upper_count;

				accumulators[event->tone_index] += tone->lower_count;
				if (accumulators[event->tone_index] >= period) {
					last_word = rp1_gpclk_pack_fraction(
						tone->lower_divider_word);
					accumulators[event->tone_index] -= period;
				} else {
					last_word = rp1_gpclk_pack_fraction(
						tone->upper_divider_word);
				}
			}
			provider->words[word_index++] = last_word;
		}
	}
	provider->event_write_count = word_index;
	return word_index ? 0 : -EINVAL;
}

static int submit_event_program(struct rp1_gpclk_provider *provider,
	const struct rp1_gpclk_event_program *request)
{
	struct dma_slave_config config = {};
	struct dma_async_tx_descriptor *descriptor;
	unsigned long flags;
	dma_cookie_t cookie;
	int ret;

	if (!rp1_gpclk_valid_event_program(request, provider->generation))
		return -EINVAL;
	if (provider->state == RP1_GPCLK_STATE_RUNNING ||
		provider->state == RP1_GPCLK_STATE_DRAINING)
		return -EBUSY;
	if (live_output) {
		ret = prepare_event_words(provider, request);
		if (ret)
			return ret;
		ret = rp1_gpclk_dma_lease_configure(&provider->lease, 3,
			provider->words[0]);
		if (ret)
			return ret;
		config.direction = DMA_MEM_TO_DEV;
		config.dst_addr = provider->lease.divider_phys_addr;
		config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		config.dst_maxburst = 1;
		ret = dmaengine_slave_config(provider->dma, &config);
		if (ret)
			return ret;
		descriptor = dmaengine_prep_slave_single(provider->dma,
			provider->words_dma, provider->event_write_count * sizeof(u32),
			DMA_MEM_TO_DEV, DMA_CTRL_ACK);
		if (!descriptor)
			return -EIO;
		cookie = dmaengine_submit(descriptor);
		ret = dma_submit_error(cookie);
		if (ret)
			return ret;
		ret = pinctrl_select_state(provider->pinctrl, provider->safe_state);
		if (ret)
			goto terminate;
		ret = rp1_gpclk_dma_lease_enable(provider->clk, &provider->lease);
		if (ret)
			goto terminate;
		provider->output_active = true;
		ret = rp1_gpclk_dma_lease_set_output(&provider->lease, false);
		if (ret)
			goto deactivate;
		ret = pinctrl_select_state(provider->pinctrl,
			provider->drive_states[provider->drive_index]);
		if (ret)
			goto deactivate;
	}
	memcpy(provider->event_program, request, sizeof(*request));
	spin_lock_irqsave(&provider->event_lock, flags);
	provider->generation = request->generation;
	provider->current_event = 0;
	provider->terminal_reason = RP1_GPCLK_TERMINAL_NONE;
	provider->event_submitted = true;
	provider->event_armed = true;
	provider->event_live = live_output;
	provider->state = RP1_GPCLK_STATE_IDLE;
	provider->event_deadline = ktime_add_ns(ktime_get(),
		RP1_GPCLK_EVENT_START_LEAD_NS);
	spin_unlock_irqrestore(&provider->event_lock, flags);
	if (live_output) {
		writel(RP1_GPCLK_TICK_DIVIDER, provider->ticks + TICKS_DMA0_CYCLES);
		writel(DMA_TICK_DWELL, provider->dma_tick + DMA_TICK0_CTRL);
		dma_async_issue_pending(provider->dma);
	}
	hrtimer_start(&provider->event_timer, provider->event_deadline,
		HRTIMER_MODE_ABS_SOFT);
	return 0;
deactivate:
	deactivate_output(provider);
terminate:
	dmaengine_terminate_sync(provider->dma);
	return ret;
}

static void stop_event_program(struct rp1_gpclk_provider *provider, u32 reason)
{
	unsigned long flags;
	int gate_ret = 0;

	hrtimer_cancel(&provider->event_timer);
	stop_tick(provider);
	if (provider->event_live) {
		gate_ret = gate_event_output(provider, false);
		dmaengine_terminate_sync(provider->dma);
	}
	spin_lock_irqsave(&provider->event_lock, flags);
	if (provider->event_submitted) {
		provider->state = RP1_GPCLK_STATE_DRAINING;
		provider->event_submitted = false;
		provider->event_armed = false;
		provider->event_live = false;
		if (gate_ret)
			fail_event_locked(provider,
				RP1_GPCLK_TERMINAL_ADAPTER_FAILED);
		else {
			provider->terminal_reason = reason;
			provider->state = RP1_GPCLK_STATE_COMPLETE;
		}
	}
	spin_unlock_irqrestore(&provider->event_lock, flags);
	deactivate_output(provider);
}

static void cleanup_event_program(struct work_struct *work)
{
	struct rp1_gpclk_provider *provider = container_of(work,
		struct rp1_gpclk_provider, event_cleanup_work);
	unsigned long flags;
	int gate_ret;

	mutex_lock(&provider->lock);
	if (provider->event_live && !provider->event_submitted) {
		gate_ret = gate_event_output(provider, false);
		if (gate_ret) {
			spin_lock_irqsave(&provider->event_lock, flags);
			fail_event_locked(provider,
				RP1_GPCLK_TERMINAL_ADAPTER_FAILED);
			spin_unlock_irqrestore(&provider->event_lock, flags);
		}
		dmaengine_terminate_sync(provider->dma);
		deactivate_output(provider);
		provider->event_live = false;
	}
	mutex_unlock(&provider->lock);
}

static int drive_index(u32 drive_ma)
{
	switch (drive_ma) {
	case 2: return 0;
	case 4: return 1;
	case 8: return 2;
	case 12: return 3;
	default: return -EINVAL;
	}
}

static int deactivate_output(struct rp1_gpclk_provider *provider)
{
	int ret = 0;

	if (provider->output_active) {
		rp1_gpclk_dma_lease_disable(provider->clk, &provider->lease);
		provider->output_active = false;
	}
	if (pinctrl_select_state(provider->pinctrl, provider->safe_state))
		ret = -EIO;
	return ret;
}

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
	ret = ret ?: deactivate_output(provider);
	provider->state = ret || provider->timing_failed || div_int != 3 ||
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
	u64 elapsed_ns = ktime_get_ns() - provider->started_ns;

	stop_tick(provider);
	provider->timing_failed = !rp1_gpclk_valid_frame_elapsed(elapsed_ns);
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
	unsigned int i, symbol_index;
	size_t word_index = 0;
	int ret;

	if (!rp1_gpclk_valid_header(request->version, request->size,
			sizeof(*request)))
		return -EPROTO;
	if (!rp1_gpclk_valid_program(request, provider->generation))
		return -EINVAL;
	if (provider->state == RP1_GPCLK_STATE_RUNNING ||
		provider->state == RP1_GPCLK_STATE_DRAINING)
		return -EBUSY;
	for (symbol_index = 0;
			symbol_index < request->symbol_count;
			++symbol_index) {
		const struct rp1_gpclk_symbol *symbol =
			&request->tones[request->symbols[symbol_index]];

		lower = rp1_gpclk_pack_fraction(symbol->lower_divider_word);
		upper = rp1_gpclk_pack_fraction(symbol->upper_divider_word);
		accumulator = 0;
		for (i = 0; i < RP1_GPCLK_WRITES_PER_SYMBOL; ++i) {
			accumulator += symbol->lower_count;
			if (accumulator >= RP1_GPCLK_WRITES_PER_SYMBOL) {
				provider->words[word_index++] = lower;
				accumulator -= RP1_GPCLK_WRITES_PER_SYMBOL;
			} else {
				provider->words[word_index++] = upper;
			}
		}
	}
	ret = rp1_gpclk_dma_lease_configure(&provider->lease, 3,
		provider->words[0]);
	if (ret)
		return ret;
	config.direction = DMA_MEM_TO_DEV;
	/* The DMA engine translates this CPU physical peripheral address. */
	config.dst_addr = provider->lease.divider_phys_addr;
	config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	config.dst_maxburst = 1;
	ret = dmaengine_slave_config(provider->dma, &config);
	if (ret)
		return ret;
	descriptor = dmaengine_prep_slave_single(provider->dma,
		provider->words_dma,
		(size_t)request->symbol_count * RP1_GPCLK_SYMBOL_BYTES,
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
	provider->expected_final = provider->words[
		(size_t)request->symbol_count * RP1_GPCLK_WRITES_PER_SYMBOL - 1];
	provider->state = RP1_GPCLK_STATE_RUNNING;
	provider->submitted = true;
	provider->timing_failed = false;
	if (!live_output)
		goto start_dma;
	ret = pinctrl_select_state(provider->pinctrl,
		provider->drive_states[provider->drive_index]);
	if (ret)
		goto terminate;
	ret = rp1_gpclk_dma_lease_enable(provider->clk, &provider->lease);
	if (ret) {
		pinctrl_select_state(provider->pinctrl, provider->safe_state);
		goto terminate;
	}
	provider->output_active = true;
start_dma:
	writel(RP1_GPCLK_TICK_DIVIDER, provider->ticks + TICKS_DMA0_CYCLES);
	writel(DMA_TICK_DWELL,
		provider->dma_tick + DMA_TICK0_CTRL);
	dma_async_issue_pending(provider->dma);
	provider->started_ns = ktime_get_ns();
	writel(DMA_TICK_REQ | DMA_TICK_SINGLE,
		provider->dma_tick + DMA_TICK0_EN);
	writel(1, provider->ticks + TICKS_DMA0_CTRL);
	return 0;

terminate:
	dmaengine_terminate_sync(provider->dma);
	provider->submitted = false;
	provider->state = RP1_GPCLK_STATE_FAILED;
	return ret;
}

static long provider_ioctl(struct file *file, unsigned int command,
	unsigned long argument)
{
	struct rp1_gpclk_provider *provider = file->private_data;
	void __user *user = (void __user *)argument;
	struct rp1_gpclk_generation generation;
	struct rp1_gpclk_acquire acquire;
	struct rp1_gpclk_program *program;
	struct rp1_gpclk_event_program *event_program;
	struct rp1_gpclk_event_state event_state;
	long ret = 0;

	mutex_lock(&provider->lock);
	switch (command) {
	case RP1_GPCLK_IOC_ACQUIRE:
		if (copy_from_user(&acquire, user, sizeof(acquire))) { ret = -EFAULT; break; }
		if (!rp1_gpclk_valid_header(acquire.version, acquire.size, sizeof(acquire))) { ret = -EPROTO; break; }
		if (acquire.flags || acquire.reserved || !rp1_gpclk_valid_drive(acquire.drive_ma)) { ret = -EINVAL; break; }
		if (provider->owner) { ret = -EBUSY; break; }
		ret = rp1_gpclk_dma_lease_get(provider->clk, &provider->lease);
		if (!ret) {
			provider->drive_index = drive_index(acquire.drive_ma);
			provider->owner = file; provider->lease_held = true;
			provider->generation = 0;
			provider->state = RP1_GPCLK_STATE_IDLE;
			provider->current_event = 0;
			provider->terminal_reason = RP1_GPCLK_TERMINAL_NONE;
			provider->event_submitted = false;
			provider->event_armed = false;
		}
		break;
	case RP1_GPCLK_IOC_SUBMIT:
		if (provider->owner != file) { ret = -EPERM; break; }
		program = memdup_user(user, sizeof(*program));
		if (IS_ERR(program)) { ret = PTR_ERR(program); break; }
		ret = submit_program(provider, program);
		kfree(program);
		break;
	case RP1_GPCLK_IOC_SUBMIT_EVENTS:
		if (provider->owner != file) { ret = -EPERM; break; }
		event_program = memdup_user(user, sizeof(*event_program));
		if (IS_ERR(event_program)) { ret = PTR_ERR(event_program); break; }
		ret = submit_event_program(provider, event_program);
		kfree(event_program);
		break;
	case RP1_GPCLK_IOC_EVENT_STATE:
		if (provider->owner != file) { ret = -EPERM; break; }
		if (copy_from_user(&event_state, user, sizeof(event_state))) { ret = -EFAULT; break; }
		if (event_state.version != RP1_GPCLK_EVENT_UAPI_VERSION || event_state.size != sizeof(event_state)) { ret = -EPROTO; break; }
		if (event_state.generation != provider->generation) { ret = -ESTALE; break; }
		spin_lock_irq(&provider->event_lock);
		event_state.state = provider->state;
		event_state.current_event = provider->current_event;
		event_state.terminal_reason = provider->terminal_reason;
		spin_unlock_irq(&provider->event_lock);
		if (copy_to_user(user, &event_state, sizeof(event_state))) ret = -EFAULT;
		break;
	case RP1_GPCLK_IOC_STOP:
	case RP1_GPCLK_IOC_STATE:
		if (provider->owner != file) { ret = -EPERM; break; }
		if (copy_from_user(&generation, user, sizeof(generation))) { ret = -EFAULT; break; }
		if (!rp1_gpclk_valid_header(generation.version, generation.size, sizeof(generation))) { ret = -EPROTO; break; }
		if (generation.generation != provider->generation) { ret = -ESTALE; break; }
		if (command == RP1_GPCLK_IOC_STOP && provider->event_submitted)
			stop_event_program(provider, RP1_GPCLK_TERMINAL_STOPPED);
		else if (command == RP1_GPCLK_IOC_STOP && provider->state == RP1_GPCLK_STATE_RUNNING)
			provider->state = RP1_GPCLK_STATE_DRAINING;
		generation.state = provider->state;
		if (command == RP1_GPCLK_IOC_STATE && copy_to_user(user, &generation, sizeof(generation))) ret = -EFAULT;
		break;
	case RP1_GPCLK_IOC_RELEASE:
		if (provider->owner != file) { ret = -EPERM; break; }
		if (provider->event_submitted ||
			provider->state == RP1_GPCLK_STATE_RUNNING ||
			provider->state == RP1_GPCLK_STATE_DRAINING) { ret = -EBUSY; break; }
		if (provider->event_live) {
			dmaengine_terminate_sync(provider->dma);
			deactivate_output(provider);
			provider->event_live = false;
		}
		if (provider->lease_held) rp1_gpclk_dma_lease_put(provider->clk, &provider->lease);
		provider->lease_held = false; provider->owner = NULL; provider->state = RP1_GPCLK_STATE_IDLE;
		provider->current_event = 0; provider->terminal_reason = RP1_GPCLK_TERMINAL_NONE;
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
	if (provider->owner == file && provider->event_submitted) {
		stop_event_program(provider, RP1_GPCLK_TERMINAL_OWNER_CLOSED);
		if (provider->lease_held)
			rp1_gpclk_dma_lease_put(provider->clk, &provider->lease);
		provider->lease_held = false; provider->owner = NULL;
	} else if (provider->owner == file && provider->state != RP1_GPCLK_STATE_RUNNING &&
		provider->state != RP1_GPCLK_STATE_DRAINING) {
		if (provider->event_live) {
			dmaengine_terminate_sync(provider->dma);
			deactivate_output(provider);
			provider->event_live = false;
		}
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
	spin_lock_init(&provider->event_lock);
	hrtimer_setup(&provider->event_timer, event_deadline,
		CLOCK_MONOTONIC, HRTIMER_MODE_ABS_SOFT);
	INIT_DELAYED_WORK(&provider->verify_work, verify_completion);
	INIT_WORK(&provider->event_cleanup_work, cleanup_event_program);
	provider->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(provider->pinctrl)) return PTR_ERR(provider->pinctrl);
	provider->safe_state = pinctrl_lookup_state(provider->pinctrl, "safe");
	provider->drive_states[0] = pinctrl_lookup_state(provider->pinctrl, "drive-2ma");
	provider->drive_states[1] = pinctrl_lookup_state(provider->pinctrl, "drive-4ma");
	provider->drive_states[2] = pinctrl_lookup_state(provider->pinctrl, "drive-8ma");
	provider->drive_states[3] = pinctrl_lookup_state(provider->pinctrl, "drive-12ma");
	if (IS_ERR(provider->safe_state) || IS_ERR(provider->drive_states[0]) ||
		IS_ERR(provider->drive_states[1]) || IS_ERR(provider->drive_states[2]) ||
		IS_ERR(provider->drive_states[3])) return -EINVAL;
	ret = pinctrl_select_state(provider->pinctrl, provider->safe_state);
	if (ret) return ret;
	provider->clk = devm_clk_get(&pdev->dev, "gpclk");
	if (IS_ERR(provider->clk)) return PTR_ERR(provider->clk);
	provider->ticks = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(provider->ticks)) return PTR_ERR(provider->ticks);
	provider->dma_tick = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(provider->dma_tick)) return PTR_ERR(provider->dma_tick);
	provider->dma = dma_request_chan(&pdev->dev, "tick0");
	if (IS_ERR(provider->dma)) return PTR_ERR(provider->dma);
	provider->words = dma_alloc_coherent(provider->dma->device->dev,
		RP1_GPCLK_BUFFER_BYTES, &provider->words_dma, GFP_KERNEL);
	if (!provider->words) { ret = -ENOMEM; goto release_dma; }
	provider->event_program = devm_kzalloc(&pdev->dev,
		sizeof(*provider->event_program), GFP_KERNEL);
	if (!provider->event_program) { ret = -ENOMEM; goto free_words; }
	provider->misc.minor = MISC_DYNAMIC_MINOR; provider->misc.name = "rp1-gpclk0";
	provider->misc.fops = &provider_fops; provider->misc.parent = &pdev->dev;
	ret = misc_register(&provider->misc);
	if (ret) goto free_words;
	platform_set_drvdata(pdev, provider);
	return 0;
free_words:
	dma_free_coherent(provider->dma->device->dev,
		RP1_GPCLK_BUFFER_BYTES, provider->words, provider->words_dma);
release_dma:
	dma_release_channel(provider->dma);
	return ret;
}

static void rp1_gpclk_provider_remove(struct platform_device *pdev)
{
	struct rp1_gpclk_provider *provider = platform_get_drvdata(pdev);
	misc_deregister(&provider->misc); cancel_delayed_work_sync(&provider->verify_work);
	stop_event_program(provider, RP1_GPCLK_TERMINAL_PROVIDER_REMOVED);
	cancel_work_sync(&provider->event_cleanup_work);
	stop_tick(provider);
	if (provider->submitted) {
		dmaengine_terminate_sync(provider->dma);
		provider->submitted = false;
	}
	deactivate_output(provider);
	if (provider->lease_held) rp1_gpclk_dma_lease_put(provider->clk, &provider->lease);
	dma_free_coherent(provider->dma->device->dev,
		RP1_GPCLK_BUFFER_BYTES, provider->words, provider->words_dma);
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
