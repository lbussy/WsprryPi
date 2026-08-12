#include "rp1_gpclk_provider_core.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

static int valid_header(__u16 version, __u16 size, __u16 expected)
{
	return version == RP1_GPCLK_UAPI_VERSION && size == expected;
}

static int valid_drive(__u32 drive)
{
	return drive == 2 || drive == 4 || drive == 8 || drive == 12;
}

static void fail_closed(struct rp1_gpclk_provider_core *core,
	const struct rp1_gpclk_event_adapter *adapter)
{
	if (core->fail_closed_done)
		return;
	core->fail_closed_done = 1;
	if (adapter && adapter->fail_closed)
		adapter->fail_closed(adapter->context);
}

static int finish_events(struct rp1_gpclk_provider_core *core,
	const struct rp1_gpclk_event_adapter *adapter, __u32 state, __u32 reason)
{
	fail_closed(core, adapter);
	core->event_active = 0;
	core->state = state;
	core->terminal_reason = reason;
	return state == RP1_GPCLK_STATE_FAILED ? -EIO : 0;
}

int rp1_gpclk_core_valid_frame_elapsed(__u64 elapsed_ns)
{
	const __u64 nominal = RP1_GPCLK_WSPR_FRAME_NOMINAL_NS;
	const __u64 tolerance = RP1_GPCLK_WSPR_FRAME_TOLERANCE_NS;

	return elapsed_ns >= nominal - tolerance &&
		elapsed_ns <= nominal + tolerance;
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
	core->generation = 0;
	core->state = RP1_GPCLK_STATE_IDLE;
	core->event_count = 0;
	core->current_event = 0;
	core->terminal_reason = RP1_GPCLK_TERMINAL_NONE;
	core->event_active = 0;
	core->fail_closed_done = 0;
	return 0;
}

int rp1_gpclk_core_valid_event_program(
	const struct rp1_gpclk_event_program *request, __u64 previous_generation)
{
	__u64 total = 0;
	__u32 i;

	if (request->version != RP1_GPCLK_EVENT_UAPI_VERSION ||
		request->size != sizeof(*request) || request->fractional_bits != 16 ||
		request->tick_divider != RP1_GPCLK_TICK_DIVIDER ||
		!request->generation || request->generation <= previous_generation ||
		request->flags || request->reserved || !request->tone_count ||
		request->tone_count > RP1_GPCLK_EVENT_MAX_TONES ||
		!request->event_count ||
		request->event_count > RP1_GPCLK_EVENT_MAX_EVENTS)
		return 0;
	for (i = 0; i < request->tone_count; ++i) {
		const struct rp1_gpclk_symbol *tone = &request->tones[i];
		if (!tone->lower_divider_word || !tone->upper_divider_word ||
			(!tone->lower_count && !tone->upper_count) ||
			tone->lower_count > UINT_MAX - tone->upper_count ||
			(tone->lower_divider_word >> 16) != 3 ||
			(tone->upper_divider_word >> 16) != 3)
			return 0;
	}
	for (i = 0; i < request->event_count; ++i) {
		const struct rp1_gpclk_event *event = &request->events[i];
		if (!event->duration_ns || event->reserved ||
			(event->flags & ~RP1_GPCLK_EVENT_RF_ON) ||
			((event->flags & RP1_GPCLK_EVENT_RF_ON) &&
			 event->tone_index >= request->tone_count) ||
			total > ULLONG_MAX - event->duration_ns)
			return 0;
		total += event->duration_ns;
	}
	return total == request->total_duration_ns;
}

int rp1_gpclk_core_submit_events(struct rp1_gpclk_provider_core *core,
	const struct rp1_gpclk_event_program *request)
{
	if (!core->owned || core->state == RP1_GPCLK_STATE_RUNNING ||
		core->state == RP1_GPCLK_STATE_DRAINING)
		return -EBUSY;
	if (!rp1_gpclk_core_valid_event_program(request, core->generation))
		return -EINVAL;
	memcpy(core->events, request->events,
		request->event_count * sizeof(request->events[0]));
	core->generation = request->generation;
	core->event_count = request->event_count;
	core->current_event = 0;
	core->terminal_reason = RP1_GPCLK_TERMINAL_NONE;
	core->event_active = 0;
	core->fail_closed_done = 0;
	core->state = RP1_GPCLK_STATE_IDLE;
	return 0;
}

int rp1_gpclk_core_start_events(struct rp1_gpclk_provider_core *core,
	const struct rp1_gpclk_event_adapter *adapter, __u64 now_ns)
{
	if (!core->owned || !core->event_count || core->event_active)
		return -EINVAL;
	if (now_ns > ULLONG_MAX - core->events[0].duration_ns)
		return finish_events(core, adapter, RP1_GPCLK_STATE_FAILED,
			RP1_GPCLK_TERMINAL_DEADLINE_MISSED);
	core->state = RP1_GPCLK_STATE_RUNNING;
	core->event_active = 1;
	if (adapter && adapter->apply &&
		adapter->apply(adapter->context, &core->events[0], 0))
		return finish_events(core, adapter, RP1_GPCLK_STATE_FAILED,
			RP1_GPCLK_TERMINAL_ADAPTER_FAILED);
	core->deadline_ns = now_ns + core->events[0].duration_ns;
	return 0;
}

int rp1_gpclk_core_advance_events(struct rp1_gpclk_provider_core *core,
	const struct rp1_gpclk_event_adapter *adapter, __u64 generation, __u64 now_ns)
{
	if (!core->owned || generation != core->generation)
		return -ESTALE;
	if (!core->event_active || core->state != RP1_GPCLK_STATE_RUNNING)
		return -EALREADY;
	if (now_ns < core->deadline_ns)
		return 0;
	if (now_ns > core->deadline_ns)
		return finish_events(core, adapter, RP1_GPCLK_STATE_FAILED,
			RP1_GPCLK_TERMINAL_DEADLINE_MISSED);
	if (++core->current_event == core->event_count)
		return finish_events(core, adapter, RP1_GPCLK_STATE_COMPLETE,
			RP1_GPCLK_TERMINAL_COMPLETE);
	if (core->deadline_ns > ULLONG_MAX -
		core->events[core->current_event].duration_ns)
		return finish_events(core, adapter, RP1_GPCLK_STATE_FAILED,
			RP1_GPCLK_TERMINAL_DEADLINE_MISSED);
	if (adapter && adapter->apply && adapter->apply(adapter->context,
			&core->events[core->current_event], core->current_event))
		return finish_events(core, adapter, RP1_GPCLK_STATE_FAILED,
			RP1_GPCLK_TERMINAL_ADAPTER_FAILED);
	core->deadline_ns += core->events[core->current_event].duration_ns;
	return 0;
}

int rp1_gpclk_core_stop_events(struct rp1_gpclk_provider_core *core,
	const struct rp1_gpclk_event_adapter *adapter, __u64 generation, __u32 reason)
{
	if (!core->owned || generation != core->generation)
		return -ESTALE;
	if (!core->event_active && core->event_count &&
		core->state == RP1_GPCLK_STATE_IDLE)
		return finish_events(core, adapter, RP1_GPCLK_STATE_COMPLETE, reason);
	if (!core->event_active)
		return -EALREADY;
	core->state = RP1_GPCLK_STATE_DRAINING;
	return finish_events(core, adapter, RP1_GPCLK_STATE_COMPLETE, reason);
}

int rp1_gpclk_core_submit(struct rp1_gpclk_provider_core *core,
	const struct rp1_gpclk_program *request)
{
	__u32 i;

	if (!valid_header(request->version, request->size, sizeof(*request)))
		return -EPROTO;
	if (!core->owned || core->state == RP1_GPCLK_STATE_RUNNING ||
		core->state == RP1_GPCLK_STATE_DRAINING)
		return -EBUSY;
	if (request->fractional_bits != 16 || !request->generation ||
		request->generation <= core->generation ||
		request->reserved ||
		request->writes_per_symbol != RP1_GPCLK_WRITES_PER_SYMBOL ||
		request->tick_divider != RP1_GPCLK_TICK_DIVIDER ||
		request->symbol_count != RP1_GPCLK_WSPR_SYMBOL_COUNT ||
		request->tone_count != 4)
		return -EINVAL;
	for (i = 0; i < request->tone_count; ++i) {
		const struct rp1_gpclk_symbol *symbol = &request->tones[i];

		if (symbol->lower_count + symbol->upper_count !=
				RP1_GPCLK_WRITES_PER_SYMBOL ||
			(symbol->lower_divider_word >> 16) != 3 ||
			(symbol->upper_divider_word >> 16) != 3)
			return -EINVAL;
	}
	for (i = 0; i < request->symbol_count; ++i)
		if (request->symbols[i] >= request->tone_count)
			return -EINVAL;
	/* Packing is provider-owned; the UAPI never exposes this representation. */
	core->lower_div_frac =
		(request->tones[request->symbols[0]].lower_divider_word & 0xffffU) << 16;
	core->upper_div_frac =
		(request->tones[request->symbols[request->symbol_count - 1]].upper_divider_word &
			0xffffU) << 16;
	core->symbol_count = request->symbol_count;
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
