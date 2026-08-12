#ifndef RP1_GPCLK_PROVIDER_CORE_H
#define RP1_GPCLK_PROVIDER_CORE_H

#include "../../src/WSPR-Transmitter/src/rp1_gpclk_uapi.h"

struct rp1_gpclk_provider_core {
	__u64 generation;
	__u32 state;
	__u32 drive_ma;
	__u32 lower_div_frac;
	__u32 upper_div_frac;
	__u32 symbol_count;
	__u32 event_count;
	__u32 current_event;
	__u32 terminal_reason;
	__u64 deadline_ns;
	struct rp1_gpclk_event events[RP1_GPCLK_EVENT_MAX_EVENTS];
	int event_active;
	int fail_closed_done;
	int owned;
};

struct rp1_gpclk_event_adapter {
	int (*apply)(void *context, const struct rp1_gpclk_event *, __u32 index);
	void (*fail_closed)(void *context);
	void *context;
};

int rp1_gpclk_core_acquire(struct rp1_gpclk_provider_core *,
	const struct rp1_gpclk_acquire *);
int rp1_gpclk_core_submit(struct rp1_gpclk_provider_core *,
	const struct rp1_gpclk_program *);
int rp1_gpclk_core_stop(struct rp1_gpclk_provider_core *, __u64 generation);
int rp1_gpclk_core_complete(struct rp1_gpclk_provider_core *, __u64 generation,
	int failed);
int rp1_gpclk_core_release(struct rp1_gpclk_provider_core *);
int rp1_gpclk_core_valid_frame_elapsed(__u64 elapsed_ns);
int rp1_gpclk_core_valid_event_program(
	const struct rp1_gpclk_event_program *, __u64 previous_generation);
int rp1_gpclk_core_submit_events(struct rp1_gpclk_provider_core *,
	const struct rp1_gpclk_event_program *);
int rp1_gpclk_core_start_events(struct rp1_gpclk_provider_core *,
	const struct rp1_gpclk_event_adapter *, __u64 now_ns);
int rp1_gpclk_core_advance_events(struct rp1_gpclk_provider_core *,
	const struct rp1_gpclk_event_adapter *, __u64 generation, __u64 now_ns);
int rp1_gpclk_core_stop_events(struct rp1_gpclk_provider_core *,
	const struct rp1_gpclk_event_adapter *, __u64 generation, __u32 reason);

#endif
