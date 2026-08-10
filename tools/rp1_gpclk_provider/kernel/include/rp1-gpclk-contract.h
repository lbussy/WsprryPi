#ifndef RP1_GPCLK_CONTRACT_H
#define RP1_GPCLK_CONTRACT_H

#include <linux/rp1_gpclk.h>

static inline bool rp1_gpclk_valid_header(u16 version, u16 size,
	size_t expected)
{
	return version == RP1_GPCLK_UAPI_VERSION && size == expected;
}

static inline bool rp1_gpclk_valid_drive(u32 drive)
{
	return drive == 2 || drive == 4 || drive == 8 || drive == 12;
}

static inline u32 rp1_gpclk_pack_fraction(u64 divider_word)
{
	return (divider_word & 0xffffU) << 16;
}

static inline bool rp1_gpclk_valid_program(
	const struct rp1_gpclk_program *request, u64 previous_generation)
{
	return rp1_gpclk_valid_header(request->version, request->size,
			sizeof(*request)) && request->fractional_bits == 16 &&
		request->generation > previous_generation &&
		request->writes_per_symbol == RP1_GPCLK_WRITES_PER_SYMBOL &&
		request->tick_divider == RP1_GPCLK_TICK_DIVIDER &&
		request->lower_count + request->upper_count ==
			RP1_GPCLK_WRITES_PER_SYMBOL &&
		(request->lower_divider_word >> 16) == 3 &&
		(request->upper_divider_word >> 16) == 3;
}

#endif
