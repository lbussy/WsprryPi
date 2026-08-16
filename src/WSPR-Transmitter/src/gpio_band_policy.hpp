#pragma once

#include <array>
#include <cmath>
#include <string>
#include <string_view>

#include "execution_plan.hpp"
#include "transmission_request.hpp"

namespace wsprrypi
{

enum class QualificationState
{
    QUALIFIED,
    UNQUALIFIED,
    UNTESTED,
    UNAVAILABLE
};

struct FrequencyPolicyDecision
{
    bool allowed{true};
    bool amateur_allocation{true};
    QualificationState qualification{QualificationState::QUALIFIED};
    std::string band;
    std::string error;
};

inline const char* qualification_state_name(QualificationState state) noexcept
{
    switch (state)
    {
    case QualificationState::QUALIFIED: return "qualified";
    case QualificationState::UNQUALIFIED: return "unqualified";
    case QualificationState::UNTESTED: return "untested";
    case QualificationState::UNAVAILABLE: return "unavailable";
    }
    return "unqualified";
}

namespace detail
{
struct BandRange { std::string_view name; double lower_hz; double upper_hz; };

// Union of the project's recognized US and international amateur-band buckets.
// These are safety-policy buckets, not a statement of an operator's authority.
inline constexpr std::array<BandRange, 26> amateur_bands{{
    {"2200 m",135700,137800},{"630 m",472000,479000},
    {"160 m",1800000,2000000},{"80 m",3500000,4000000},
    {"60 m",5250000,5450000},{"40 m",7000000,7300000},
    {"30 m",10100000,10150000},{"22 m",13000000,13600000},
    {"20 m",14000000,14350000},{"17 m",18068000,18168000},
    {"15 m",21000000,21450000},{"12 m",24890000,24990000},
    {"10 m",28000000,29700000},{"6 m",50000000,54000000},
    {"4 m",70000000,71000000},{"2 m",144000000,148000000},
    {"1.25 m",222000000,225000000},{"70 cm",420000000,450000000},
    {"33 cm",902000000,928000000},{"23 cm",1240000000,1300000000},
    {"13 cm",2300000000,2450000000},{"9 cm",3300000000,3500000000},
    {"6 cm",5650000000,5925000000},{"3 cm",10000000000,10500000000},
    {"1.25 cm",24000000000,24250000000},{"1 mm",241000000000,250000000000}
}};

inline const BandRange* find_band(double frequency_hz) noexcept
{
    for (const auto& band : amateur_bands)
        if (frequency_hz >= band.lower_hz && frequency_hz <= band.upper_hz)
            return &band;
    return nullptr;
}

inline QualificationState qualification_for(
    BackendKind backend, HardwareProfile profile, TransmissionMode mode,
    std::string_view band) noexcept
{
    if (backend == BackendKind::SIMULATED)
        return QualificationState::QUALIFIED;

    if (backend == BackendKind::SI5351)
    {
        if (band == "1.25 m")
            return QualificationState::UNAVAILABLE;
        return QualificationState::QUALIFIED;
    }

    if (backend != BackendKind::RPI_CLOCK_GPIO && backend != BackendKind::RP1_GPCLK)
        return QualificationState::UNAVAILABLE;

    if (band == "2200 m")
    {
        if (profile == HardwareProfile::BCM2711_750_MHZ_PLLD ||
            profile == HardwareProfile::RP1_GPCLK)
            return QualificationState::QUALIFIED;
        if (profile == HardwareProfile::LEGACY_500_MHZ_PLLD)
        {
            if (mode == TransmissionMode::TONE ||
                mode == TransmissionMode::QRSS ||
                mode == TransmissionMode::FSKCW ||
                mode == TransmissionMode::DFCW)
                return QualificationState::QUALIFIED;
        }
        return QualificationState::UNQUALIFIED;
    }

    const bool questionable = band == "12 m" || band == "6 m" || band == "4 m" ||
        band == "2 m" || band == "1.25 m" || band == "70 cm";
    if (!questionable)
        return QualificationState::QUALIFIED;

    if (profile == HardwareProfile::BCM2711_750_MHZ_PLLD)
    {
        if (band == "1.25 m" || band == "70 cm")
            return QualificationState::UNAVAILABLE;
        if (band == "6 m" &&
            (mode == TransmissionMode::TONE || mode == TransmissionMode::QRSS ||
             mode == TransmissionMode::FSKCW || mode == TransmissionMode::DFCW))
            return QualificationState::QUALIFIED;
        return QualificationState::UNQUALIFIED;
    }

    // Profiles without a completed mode-specific record remain fail-closed.
    // CW states become qualified only after the applicable qualification.
    if (mode == TransmissionMode::QRSS || mode == TransmissionMode::TONE ||
        mode == TransmissionMode::FSKCW || mode == TransmissionMode::DFCW)
        return QualificationState::UNTESTED;
    return QualificationState::UNQUALIFIED;
}
} // namespace detail

inline FrequencyPolicyDecision evaluate_frequency_policy(
    BackendKind backend,
    TransmissionMode mode,
    double frequency_hz,
    bool allow_unqualified_frequency = false,
    bool allow_non_amateur_frequency = false,
    HardwareProfile profile = HardwareProfile::UNSPECIFIED)
{
    FrequencyPolicyDecision decision;
    if (!std::isfinite(frequency_hz) || frequency_hz <= 0.0)
        return decision; // Numeric/backend validation remains authoritative.

    const auto* band = detail::find_band(frequency_hz);
    decision.amateur_allocation = band != nullptr;
    if (band == nullptr)
    {
        decision.qualification = QualificationState::UNTESTED;
        if (allow_unqualified_frequency && allow_non_amateur_frequency)
            return decision;
        decision.allowed = false;
        decision.error =
            "Transmission is outside recognized US and international amateur bands. "
            "Both --allow-unqualified-frequency and --allow-non-amateur-frequency are required.";
        return decision;
    }

    decision.band = std::string(band->name);
    decision.qualification = detail::qualification_for(backend, profile, mode, band->name);
    if (decision.qualification == QualificationState::QUALIFIED)
        return decision;
    if (decision.qualification != QualificationState::UNAVAILABLE &&
        allow_unqualified_frequency)
        return decision;

    decision.allowed = false;
    decision.error = "Transmission on the " + decision.band + " band is " +
        qualification_state_name(decision.qualification) +
        " for the selected backend and mode.";
    if (decision.qualification == QualificationState::UNAVAILABLE)
        decision.error += " This backend cannot safely construct the requested output.";
    else
        decision.error += " Use --allow-unqualified-frequency only if you accept the experimental risk.";
    return decision;
}

// Compatibility name retained while callers migrate from the former coarse GPIO rule.
inline FrequencyPolicyDecision evaluate_gpio_band_policy(
    BackendKind backend, double frequency_hz,
    TransmissionMode mode = TransmissionMode::WSPR,
    bool allow_unqualified_frequency = false,
    bool allow_non_amateur_frequency = false,
    HardwareProfile profile = HardwareProfile::UNSPECIFIED)
{
    return evaluate_frequency_policy(backend, mode, frequency_hz,
        allow_unqualified_frequency, allow_non_amateur_frequency, profile);
}

inline FrequencyPolicyDecision evaluate_gpio_band_policy(const ExecutionPlan& plan)
{
    for (const auto& event : plan.events)
    {
        if (!event.rf_on && event.type != RfEventType::SET_FREQUENCY)
            continue;
        const auto decision = evaluate_frequency_policy(
            plan.backend, plan.mode, event.frequency_hz,
            plan.policy.allow_unqualified_frequency,
            plan.policy.allow_non_amateur_frequency,
            plan.policy.hardware_profile);
        if (!decision.allowed)
            return decision;
    }
    return {};
}

} // namespace wsprrypi
