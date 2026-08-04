#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include "json.hpp"

enum class TestToneRequestSource { LegacyDefault, LegacyExactRf, WsprBand, CustomRf };
struct ParsedTestToneRequest {
    TestToneRequestSource source = TestToneRequestSource::LegacyDefault;
    std::string band;
    std::optional<std::uint64_t> frequency_hz;
};
struct TestToneRequestParseResult {
    std::optional<ParsedTestToneRequest> request;
    std::string error;
    explicit operator bool() const { return request.has_value(); }
};
TestToneRequestParseResult parse_test_tone_request(const nlohmann::json &request);
