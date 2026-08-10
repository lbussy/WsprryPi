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
	u32 i;

	if (!rp1_gpclk_valid_header(request->version, request->size,
			sizeof(*request)) || request->fractional_bits != 16 ||
		request->generation <= previous_generation || request->reserved ||
		request->writes_per_symbol != RP1_GPCLK_WRITES_PER_SYMBOL ||
		request->tick_divider != RP1_GPCLK_TICK_DIVIDER ||
		request->symbol_count != RP1_GPCLK_WSPR_SYMBOL_COUNT ||
		request->tone_count != 4)
		return false;

	for (i = 0; i < request->tone_count; ++i) {
		const struct rp1_gpclk_symbol *symbol = &request->tones[i];

		if (symbol->lower_count + symbol->upper_count !=
				RP1_GPCLK_WRITES_PER_SYMBOL ||
			(symbol->lower_divider_word >> 16) != 3 ||
			(symbol->upper_divider_word >> 16) != 3)
			return false;
	}
	for (i = 0; i < RP1_GPCLK_WSPR_SYMBOL_COUNT; ++i)
		if (request->symbols[i] >= request->tone_count)
			return false;
	return true;
}

#endif
