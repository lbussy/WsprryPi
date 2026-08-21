#pragma once
#include "test_tone_request.hpp"
#include <optional>
#include <string>
struct TestToneFrequencyPlan { TestToneRequestSource source; std::string band; std::optional<std::uint64_t> dial_frequency_hz; std::optional<std::uint64_t> audio_offset_hz; std::uint64_t actual_rf_frequency_hz=0; };
struct TestToneFrequencyPlanResult { std::optional<TestToneFrequencyPlan> plan; std::string error; explicit operator bool() const{return plan.has_value();} };
TestToneFrequencyPlanResult plan_explicit_test_tone_frequency(
    const ParsedTestToneRequest&,
    double audio_offset_hz,
    const std::string &frequency_profile = "existing_common");
