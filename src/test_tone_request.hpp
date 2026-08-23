#pragma once
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include "json.hpp"

enum class TestToneRequestSource { LegacyDefault, LegacyExactRf, WsprBand, CustomRf };
struct ParsedTestToneRequest {
    TestToneRequestSource source = TestToneRequestSource::LegacyDefault;
    std::string band;
    std::optional<std::uint64_t> frequency_hz;
    std::optional<std::chrono::nanoseconds> duration;
    struct Rp1DevelopmentConfirmation {
        bool enabled = false;
        int route_gpio = 0;
        bool physical_connection = false;
        bool attenuation_and_load = false;
        bool bounded_operation = false;
        bool non_radiating_topology = false;
        bool experimental_acknowledged = false;
        std::string operation_id;
    } rp1_development;
};
struct TestToneRequestParseResult {
    std::optional<ParsedTestToneRequest> request;
    std::string error;
    explicit operator bool() const { return request.has_value(); }
};
inline constexpr std::uint64_t MAX_BOUNDED_TONE_DURATION_MS = 60000;
struct ParsedBoundedTestToneRequest {
    ParsedTestToneRequest tone;
    std::string request_id;
    std::uint64_t duration_ms = 0;
};
struct BoundedTestToneRequestParseResult {
    std::optional<ParsedBoundedTestToneRequest> request;
    std::string error;
    explicit operator bool() const { return request.has_value(); }
};
TestToneRequestParseResult parse_test_tone_request(const nlohmann::json &request);
BoundedTestToneRequestParseResult parse_bounded_test_tone_request(
    const nlohmann::json &request);
