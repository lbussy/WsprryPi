// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>

#include "rp1-gpclk-contract.h"

static struct rp1_gpclk_program valid_program(u64 generation)
{
	return (struct rp1_gpclk_program) {
		.version = 1, .size = sizeof(struct rp1_gpclk_program),
		.fractional_bits = 16, .lower_divider_word = 232445,
		.upper_divider_word = 232446, .lower_count = 66312,
		.upper_count = 480, .writes_per_symbol = 66792,
		.tick_divider = 511, .generation = generation,
	};
}

static void header_and_drive_test(struct kunit *test)
{
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_header(1, 16, 16));
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_header(2, 16, 16));
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_header(1, 15, 16));
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_drive(2));
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_drive(4));
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_drive(8));
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_drive(12));
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_drive(6));
}

static void program_and_packing_test(struct kunit *test)
{
	struct rp1_gpclk_program program = valid_program(2);
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_program(&program, 1));
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_program(&program, 2));
	KUNIT_EXPECT_EQ(test, rp1_gpclk_pack_fraction(232445), 0x8bfd0000U);
	program.tick_divider = 510;
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_program(&program, 1));
}

static struct kunit_case provider_cases[] = {
	KUNIT_CASE(header_and_drive_test),
	KUNIT_CASE(program_and_packing_test),
	{}
};

static struct kunit_suite provider_suite = {
	.name = "rp1-gpclk-provider-contract",
	.test_cases = provider_cases,
};
kunit_test_suite(provider_suite);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RP1 GPCLK provider contract KUnit tests");
