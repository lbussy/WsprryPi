#pragma once

#include <cstdint>

namespace wsprrypi
{
    /** Processor identities covered by the non-RP1 legacy GPIO clock model. */
    enum class LegacyGpioProcessorProfile
    {
        Bcm2835,
        Bcm2836Bcm2837,
        Bcm2711
    };

    /** Parent identities available to the legacy GPIO GPCLK0 path. */
    enum class LegacyGpioClockParent
    {
        PllD,
        Oscillator
    };

    enum class LegacyGpioIntrinsicEvidence
    {
        HistoricalAuthoritative,
        DiscoveryBaseline
    };

    struct LegacyGpioClockModel
    {
        LegacyGpioProcessorProfile processor;
        LegacyGpioClockParent parent;
        double nominal_rate_hz;
        double intrinsic_system_to_rf_difference_ppm;
        LegacyGpioIntrinsicEvidence intrinsic_evidence;
    };

    /**
     * Return authoritative Phase 1 data for an exact processor/parent pair.
     * Unsupported pairings fail closed instead of selecting a fallback.
     */
    LegacyGpioClockModel legacyGpioClockModel(
        LegacyGpioProcessorProfile processor,
        LegacyGpioClockParent parent);

    /**
     * Derive the intrinsic system-to-RF difference D = P - S.
     * All inputs use the positive-fast source-rate convention.
     */
    double deriveLegacyGpioIntrinsicDifferencePpm(
        double rf_parent_error_ppm,
        double frozen_system_clock_correction_ppm);

    struct LegacyGpioSdrMeasurement
    {
        double raw_detected_hz;
        double calibrated_detected_hz;
        double calibrated_fundamental_hz;
        double calibrated_rf_error_hz;
        double calibrated_rf_error_ppm;
        std::uint32_t harmonic_number;
    };

    /** Apply the Issue 429 wspr5 affine calibration inverse exactly once. */
    double calibrateIssue429SdrFrequencyHz(double raw_detected_hz);

    /**
     * Analyze an authenticated fundamental or harmonic observation.
     * SDR calibration is applied to the raw detected frequency before any
     * harmonic division. This campaign-only calculation is not a production
     * frequency correction or configuration input.
     */
    LegacyGpioSdrMeasurement analyzeIssue429SdrMeasurement(
        double requested_fundamental_hz,
        double raw_detected_hz,
        std::uint32_t authenticated_harmonic_number);
}
