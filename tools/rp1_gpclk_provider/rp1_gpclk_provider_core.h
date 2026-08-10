#ifndef RP1_GPCLK_PROVIDER_CORE_H
#define RP1_GPCLK_PROVIDER_CORE_H

#include "../../src/WSPR-Transmitter/src/rp1_gpclk_uapi.h"

struct rp1_gpclk_provider_core {
	__u64 generation;
	__u32 state;
	__u32 drive_ma;
	__u32 lower_div_frac;
	__u32 upper_div_frac;
	int owned;
};

int rp1_gpclk_core_acquire(struct rp1_gpclk_provider_core *,
	const struct rp1_gpclk_acquire *);
int rp1_gpclk_core_submit(struct rp1_gpclk_provider_core *,
	const struct rp1_gpclk_program *);
int rp1_gpclk_core_stop(struct rp1_gpclk_provider_core *, __u64 generation);
int rp1_gpclk_core_complete(struct rp1_gpclk_provider_core *, __u64 generation,
	int failed);
int rp1_gpclk_core_release(struct rp1_gpclk_provider_core *);

#endif
