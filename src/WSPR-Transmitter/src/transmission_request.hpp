#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include "transmission_payloads.hpp"

namespace wsprrypi
{

enum class BackendKind
{
    RPI_CLOCK_GPIO,
    RP1_GPCLK,
    SI5351,
    SIMULATED
};

enum class ClockSource
{
    UNSPECIFIED,
    GPIO_CLK,
    SI5351_CLK0,
    SI5351_CLK1,
    SI5351_CLK2
};

enum class HardwareProfile
{
    UNSPECIFIED,
    LEGACY_500_MHZ_PLLD,
    BCM2711_750_MHZ_PLLD,
    RP1_GPCLK,
    SI5351
};

struct RequestId
{
    std::uint64_t value{0};
};

struct ScheduledSlot
{
    std::chrono::system_clock::time_point start_time{};
    std::chrono::nanoseconds start_offset{};
};

struct CalibrationSnapshot
{
    double ppm{0.0};
    std::optional<double> reference_frequency_hz{};
};

struct OutputSelection
{
    BackendKind backend{BackendKind::RPI_CLOCK_GPIO};
    ClockSource output{ClockSource::UNSPECIFIED};
    int gpio{0};
};

struct ExecutionPolicy
{
    bool allow_quantization{false};
    bool allow_backend_approximation{false};
    bool allow_truncation_on_stop{true};
    bool allow_unqualified_frequency{false};
    bool allow_non_amateur_frequency{false};
    HardwareProfile hardware_profile{HardwareProfile::UNSPECIFIED};
};

struct RequestMetadata
{
    std::string label;
    std::string origin;
    std::string note;
};

struct TransmissionRequest
{
    RequestId id{};
    ScheduledSlot slot{};

    TransmissionMode mode{TransmissionMode::WSPR};
    TransmissionPayload payload{};

    OutputSelection output{};
    CalibrationSnapshot calibration{};
    ExecutionPolicy policy{};
    RequestMetadata metadata{};
};

} // namespace wsprrypi
