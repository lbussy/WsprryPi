#include "legacy_gpio_clock_model.hpp"
#include "chipset_offsets.hpp"

#include <cmath>
#include <stdexcept>

namespace wsprrypi
{
    namespace
    {
        constexpr double kBcm2835PllDHz = 500000000.0;
        constexpr double kLaterLegacyPllDHz = 500000000.0;
        constexpr double kBcm2711PllDHz = 750000000.0;
        constexpr double kBcm2711OscillatorHz = 54000000.0;

        constexpr double kIssue429SdrInterceptHz = 0.4484;
        constexpr double kIssue429SdrProportionalPpm = 1.01012;

        void requireFinite(double value, const char *name)
        {
            if (!std::isfinite(value))
                throw std::invalid_argument(name);
        }

        void requirePositiveFrequency(double value, const char *name)
        {
            if (!std::isfinite(value) || value <= 0.0)
                throw std::invalid_argument(name);
        }
    }

    LegacyGpioClockModel legacyGpioClockModel(
        LegacyGpioProcessorProfile processor,
        LegacyGpioClockParent parent)
    {
        switch (processor)
        {
        case LegacyGpioProcessorProfile::Bcm2835:
            if (parent == LegacyGpioClockParent::PllD)
            {
                return {
                    processor,
                    parent,
                    kBcm2835PllDHz,
                    chipsetIntrinsicOffsetPpm(ClockChipset::Bcm2835),
                    LegacyGpioIntrinsicEvidence::HistoricalAuthoritative};
            }
            break;
        case LegacyGpioProcessorProfile::Bcm2836Bcm2837:
            if (parent == LegacyGpioClockParent::PllD)
            {
                return {
                    processor,
                    parent,
                    kLaterLegacyPllDHz,
                    chipsetIntrinsicOffsetPpm(ClockChipset::Bcm2836Bcm2837),
                    LegacyGpioIntrinsicEvidence::DiscoveryBaseline};
            }
            break;
        case LegacyGpioProcessorProfile::Bcm2711:
            if (parent == LegacyGpioClockParent::PllD)
            {
                return {
                    processor,
                    parent,
                    kBcm2711PllDHz,
                    chipsetIntrinsicOffsetPpm(ClockChipset::Bcm2711),
                    LegacyGpioIntrinsicEvidence::DiscoveryBaseline};
            }
            if (parent == LegacyGpioClockParent::Oscillator)
            {
                return {
                    processor,
                    parent,
                    kBcm2711OscillatorHz,
                    chipsetIntrinsicOffsetPpm(ClockChipset::Bcm2711),
                    LegacyGpioIntrinsicEvidence::DiscoveryBaseline};
            }
            break;
        }

        throw std::invalid_argument(
            "Unsupported legacy GPIO processor/parent clock model.");
    }

    double deriveLegacyGpioIntrinsicDifferencePpm(
        double rf_parent_error_ppm,
        double frozen_system_clock_correction_ppm)
    {
        requireFinite(
            rf_parent_error_ppm,
            "RF parent error PPM must be finite.");
        requireFinite(
            frozen_system_clock_correction_ppm,
            "Frozen system-clock correction PPM must be finite.");

        const double difference =
            rf_parent_error_ppm - frozen_system_clock_correction_ppm;
        requireFinite(
            difference,
            "Derived intrinsic system-to-RF difference must be finite.");
        return difference;
    }

    double calibrateIssue429SdrFrequencyHz(double raw_detected_hz)
    {
        requirePositiveFrequency(
            raw_detected_hz,
            "Raw detected SDR frequency must be finite and positive.");

        const double calibrated =
            (raw_detected_hz - kIssue429SdrInterceptHz) /
            (1.0 + kIssue429SdrProportionalPpm * 1.0e-6);
        requirePositiveFrequency(
            calibrated,
            "Calibrated SDR frequency must be finite and positive.");
        return calibrated;
    }

    LegacyGpioSdrMeasurement analyzeIssue429SdrMeasurement(
        double requested_fundamental_hz,
        double raw_detected_hz,
        std::uint32_t authenticated_harmonic_number)
    {
        requirePositiveFrequency(
            requested_fundamental_hz,
            "Requested fundamental frequency must be finite and positive.");
        if (authenticated_harmonic_number == 0)
        {
            throw std::invalid_argument(
                "Authenticated harmonic number must be positive.");
        }

        const double calibrated_detected_hz =
            calibrateIssue429SdrFrequencyHz(raw_detected_hz);
        const double calibrated_fundamental_hz =
            calibrated_detected_hz /
            static_cast<double>(authenticated_harmonic_number);
        requirePositiveFrequency(
            calibrated_fundamental_hz,
            "Calibrated fundamental frequency must be finite and positive.");

        const double calibrated_rf_error_hz =
            calibrated_fundamental_hz - requested_fundamental_hz;
        const double calibrated_rf_error_ppm =
            calibrated_rf_error_hz / requested_fundamental_hz * 1.0e6;
        requireFinite(
            calibrated_rf_error_hz,
            "Calibrated RF error must be finite.");
        requireFinite(
            calibrated_rf_error_ppm,
            "Calibrated RF error PPM must be finite.");

        return {
            raw_detected_hz,
            calibrated_detected_hz,
            calibrated_fundamental_hz,
            calibrated_rf_error_hz,
            calibrated_rf_error_ppm,
            authenticated_harmonic_number};
    }
}
