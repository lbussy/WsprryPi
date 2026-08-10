#include "rp1_gpclk_provider_core.h"

#include <errno.h>

static int valid_header(__u16 version, __u16 size, __u16 expected)
{
	return version == RP1_GPCLK_UAPI_VERSION && size == expected;
}

static int valid_drive(__u32 drive)
{
	return drive == 2 || drive == 4 || drive == 8 || drive == 12;
}

int rp1_gpclk_core_acquire(struct rp1_gpclk_provider_core *core,
	const struct rp1_gpclk_acquire *request)
{
	if (!valid_header(request->version, request->size, sizeof(*request)))
		return -EPROTO;
	if (request->flags || request->reserved || !valid_drive(request->drive_ma))
		return -EINVAL;
	if (core->owned)
		return -EBUSY;
	core->owned = 1;
	core->drive_ma = request->drive_ma;
	core->state = RP1_GPCLK_STATE_IDLE;
	return 0;
}

int rp1_gpclk_core_submit(struct rp1_gpclk_provider_core *core,
	const struct rp1_gpclk_program *request)
{
	if (!valid_header(request->version, request->size, sizeof(*request)))
		return -EPROTO;
	if (!core->owned || core->state == RP1_GPCLK_STATE_RUNNING ||
		core->state == RP1_GPCLK_STATE_DRAINING)
		return -EBUSY;
	if (request->fractional_bits != 16 || !request->generation ||
		request->generation <= core->generation ||
		request->writes_per_symbol != RP1_GPCLK_WRITES_PER_SYMBOL ||
		request->tick_divider != RP1_GPCLK_TICK_DIVIDER ||
		request->lower_count + request->upper_count !=
			RP1_GPCLK_WRITES_PER_SYMBOL ||
		(request->lower_divider_word >> 16) != 3 ||
		(request->upper_divider_word >> 16) != 3)
		return -EINVAL;
	/* Packing is provider-owned; the UAPI never exposes this representation. */
	core->lower_div_frac = (request->lower_divider_word & 0xffffU) << 16;
	core->upper_div_frac = (request->upper_divider_word & 0xffffU) << 16;
	core->generation = request->generation;
	core->state = RP1_GPCLK_STATE_RUNNING;
	return 0;
}

int rp1_gpclk_core_stop(struct rp1_gpclk_provider_core *core, __u64 generation)
{
	if (!core->owned || generation != core->generation)
		return -ESTALE;
	if (core->state == RP1_GPCLK_STATE_RUNNING)
		core->state = RP1_GPCLK_STATE_DRAINING;
	return core->state == RP1_GPCLK_STATE_DRAINING ? 0 : -EALREADY;
}

int rp1_gpclk_core_complete(struct rp1_gpclk_provider_core *core,
	__u64 generation, int failed)
{
	if (!core->owned || generation != core->generation)
		return -ESTALE;
	if (core->state != RP1_GPCLK_STATE_RUNNING &&
		core->state != RP1_GPCLK_STATE_DRAINING)
		return -EALREADY;
	core->state = failed ? RP1_GPCLK_STATE_FAILED : RP1_GPCLK_STATE_COMPLETE;
	return 0;
}

int rp1_gpclk_core_release(struct rp1_gpclk_provider_core *core)
{
	if (!core->owned)
		return 0;
	if (core->state == RP1_GPCLK_STATE_RUNNING ||
		core->state == RP1_GPCLK_STATE_DRAINING)
		return -EBUSY;
	core->owned = 0;
	core->drive_ma = 0;
	return 0;
}
