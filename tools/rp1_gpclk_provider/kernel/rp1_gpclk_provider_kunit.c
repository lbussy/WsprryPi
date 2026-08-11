// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>

#include "rp1-gpclk-contract.h"

static struct rp1_gpclk_program *valid_program(
	struct kunit *test, u64 generation)
{
	struct rp1_gpclk_program *program = kunit_kzalloc(
		test, sizeof(*program), GFP_KERNEL);
	u32 i;

	if (!program)
		return NULL;
	program->version = 1;
	program->size = sizeof(*program);
	program->fractional_bits = 16;
	program->writes_per_symbol = 66792;
	program->tick_divider = 511;
	program->symbol_count = 162;
	program->tone_count = 4;
	program->generation = generation;
	for (i = 0; i < 4; ++i)
		program->tones[i] = (struct rp1_gpclk_symbol) {
			.lower_divider_word = 232445 + (i & 1),
			.upper_divider_word = 232446,
			.lower_count = 66312, .upper_count = 480,
		};
	for (i = 0; i < RP1_GPCLK_WSPR_SYMBOL_COUNT; ++i)
		program->symbols[i] = i % 4;
	return program;
}

static struct rp1_gpclk_event_program *valid_event_program(
	struct kunit *test, u64 generation)
{
	struct rp1_gpclk_event_program *program = kunit_kzalloc(
		test, sizeof(*program), GFP_KERNEL);

	if (!program)
		return NULL;
	program->version = RP1_GPCLK_EVENT_UAPI_VERSION;
	program->size = sizeof(*program);
	program->fractional_bits = 16;
	program->tick_divider = RP1_GPCLK_TICK_DIVIDER;
	program->tone_count = 2;
	program->event_count = 3;
	program->generation = generation;
	program->total_duration_ns = 60;
	program->tones[0] = (struct rp1_gpclk_symbol) {
		.lower_divider_word = 232445, .upper_divider_word = 232446,
		.lower_count = 12, .upper_count = 13,
	};
	program->tones[1] = (struct rp1_gpclk_symbol) {
		.lower_divider_word = 232444, .upper_divider_word = 232445,
		.lower_count = 11, .upper_count = 14,
	};
	program->events[0] = (struct rp1_gpclk_event) {
		.duration_ns = 10, .tone_index = 0,
		.flags = RP1_GPCLK_EVENT_RF_ON,
	};
	program->events[1] = (struct rp1_gpclk_event) {
		.duration_ns = 20, .tone_index = 1,
		.flags = RP1_GPCLK_EVENT_RF_ON,
	};
	program->events[2].duration_ns = 30;
	return program;
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
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_frame_elapsed(110592000000ULL));
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_frame_elapsed(110585250000ULL));
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_frame_elapsed(110598750000ULL));
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_frame_elapsed(110585249999ULL));
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_frame_elapsed(110598750001ULL));
}

static void program_and_packing_test(struct kunit *test)
{
	struct rp1_gpclk_program *program = valid_program(test, 2);
	KUNIT_ASSERT_NOT_NULL(test, program);
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_program(program, 1));
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_program(program, 2));
	KUNIT_EXPECT_EQ(test, rp1_gpclk_pack_fraction(232445), 0x8bfd0000U);
	program->tick_divider = 510;
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_program(program, 1));
	program = valid_program(test, 2);
	KUNIT_ASSERT_NOT_NULL(test, program);
	program->symbol_count = 161;
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_program(program, 1));
	program = valid_program(test, 2);
	KUNIT_ASSERT_NOT_NULL(test, program);
	program->tones[2].upper_count--;
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_program(program, 1));
	program = valid_program(test, 2);
	KUNIT_ASSERT_NOT_NULL(test, program);
	program->symbols[81] = 4;
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_program(program, 1));
}

static void lease_generation_test(struct kunit *test)
{
	struct rp1_gpclk_program *program = valid_program(test, 1);

	KUNIT_ASSERT_NOT_NULL(test, program);
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_program(program, 0));
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_program(program, 1));
	/* A new, exclusive lease resets its previous generation to zero. */
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_program(program, 0));
}

static void event_program_test(struct kunit *test)
{
	struct rp1_gpclk_event_program *program = valid_event_program(test, 2);

	KUNIT_ASSERT_NOT_NULL(test, program);
	KUNIT_EXPECT_TRUE(test, rp1_gpclk_valid_event_program(program, 1));
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_event_program(program, 2));
	program->events[0].duration_ns = 0;
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_event_program(program, 1));
	program = valid_event_program(test, 2);
	KUNIT_ASSERT_NOT_NULL(test, program);
	program->events[1].tone_index = 2;
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_event_program(program, 1));
	program = valid_event_program(test, 2);
	KUNIT_ASSERT_NOT_NULL(test, program);
	program->total_duration_ns--;
	KUNIT_EXPECT_FALSE(test, rp1_gpclk_valid_event_program(program, 1));
}

static struct kunit_case provider_cases[] = {
	KUNIT_CASE(header_and_drive_test),
	KUNIT_CASE(program_and_packing_test),
	KUNIT_CASE(lease_generation_test),
	KUNIT_CASE(event_program_test),
	{}
};

static struct kunit_suite provider_suite = {
	.name = "rp1-gpclk-provider-contract",
	.test_cases = provider_cases,
};
kunit_test_suite(provider_suite);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RP1 GPCLK provider contract KUnit tests");
