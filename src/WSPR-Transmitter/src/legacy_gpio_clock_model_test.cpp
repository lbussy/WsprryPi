#include "legacy_gpio_clock_model.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
    int failures = 0;

    void expect(bool condition, const std::string& message)
    {
        if (!condition)
        {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }

    bool nearlyEqual(double actual, double expected, double tolerance)
    {
        return std::fabs(actual - expected) <= tolerance;
    }

    template <typename Callback>
    void expectInvalid(Callback callback, const std::string& message)
    {
        bool rejected = false;
        try
        {
            callback();
        }
        catch (const std::invalid_argument&)
        {
            rejected = true;
        }
        expect(rejected, message);
    }

    void testExactClockModels()
    {
        using namespace wsprrypi;

        const auto bcm2835 = legacyGpioClockModel(
            LegacyGpioProcessorProfile::Bcm2835,
            LegacyGpioClockParent::PllD);
        expect(
            bcm2835.processor == LegacyGpioProcessorProfile::Bcm2835 &&
                bcm2835.parent == LegacyGpioClockParent::PllD &&
                bcm2835.nominal_rate_hz == 500000000.0 &&
                bcm2835.intrinsic_system_to_rf_difference_ppm == -2.5 &&
                bcm2835.intrinsic_evidence ==
                    LegacyGpioIntrinsicEvidence::HistoricalAuthoritative,
            "BCM2835 PLLD must preserve the authoritative 500 MHz, -2.5 PPM model");

        const auto later500 = legacyGpioClockModel(
            LegacyGpioProcessorProfile::Bcm2836Bcm2837,
            LegacyGpioClockParent::PllD);
        expect(
            later500.processor ==
                    LegacyGpioProcessorProfile::Bcm2836Bcm2837 &&
                later500.parent == LegacyGpioClockParent::PllD &&
                later500.nominal_rate_hz == 500000000.0 &&
                later500.intrinsic_system_to_rf_difference_ppm == 0.0 &&
                later500.intrinsic_evidence ==
                    LegacyGpioIntrinsicEvidence::DiscoveryBaseline,
            "BCM2836/BCM2837 PLLD must exclude the BCM2835 intrinsic constant");

        const auto bcm2711PllD = legacyGpioClockModel(
            LegacyGpioProcessorProfile::Bcm2711,
            LegacyGpioClockParent::PllD);
        const auto bcm2711Oscillator = legacyGpioClockModel(
            LegacyGpioProcessorProfile::Bcm2711,
            LegacyGpioClockParent::Oscillator);
        expect(
            bcm2711PllD.processor == LegacyGpioProcessorProfile::Bcm2711 &&
                bcm2711PllD.parent == LegacyGpioClockParent::PllD &&
                bcm2711PllD.nominal_rate_hz == 750000000.0 &&
                bcm2711PllD.intrinsic_system_to_rf_difference_ppm == 0.0,
            "BCM2711 PLLD must use its independent 750 MHz discovery baseline");
        expect(
            bcm2711Oscillator.processor ==
                    LegacyGpioProcessorProfile::Bcm2711 &&
                bcm2711Oscillator.parent ==
                    LegacyGpioClockParent::Oscillator &&
                bcm2711Oscillator.nominal_rate_hz == 54000000.0 &&
                bcm2711Oscillator.intrinsic_system_to_rf_difference_ppm == 0.0,
            "BCM2711 oscillator must use its independent 54 MHz discovery baseline");

        expectInvalid(
            [] {
                (void)legacyGpioClockModel(
                    LegacyGpioProcessorProfile::Bcm2835,
                    LegacyGpioClockParent::Oscillator);
            },
            "BCM2835 oscillator pairing must fail closed");
        expectInvalid(
            [] {
                (void)legacyGpioClockModel(
                    LegacyGpioProcessorProfile::Bcm2836Bcm2837,
                    LegacyGpioClockParent::Oscillator);
            },
            "BCM2836/BCM2837 oscillator pairing must fail closed");
        expectInvalid(
            [] {
                (void)legacyGpioClockModel(
                    static_cast<LegacyGpioProcessorProfile>(-1),
                    LegacyGpioClockParent::PllD);
            },
            "unknown processor identity must fail closed");
        expectInvalid(
            [] {
                (void)legacyGpioClockModel(
                    LegacyGpioProcessorProfile::Bcm2711,
                    static_cast<LegacyGpioClockParent>(-1));
            },
            "unknown parent identity must fail closed");
    }

    void testIntrinsicDifferenceSignAndValidation()
    {
        using namespace wsprrypi;

        expect(
            deriveLegacyGpioIntrinsicDifferencePpm(4.25, 1.5) == 2.75,
            "D = P - S must preserve the positive-fast sign convention");
        expect(
            deriveLegacyGpioIntrinsicDifferencePpm(-4.25, -1.5) == -2.75,
            "negative-fast inputs must preserve D = P - S without inversion");

        for (const double invalid : {
                 std::numeric_limits<double>::infinity(),
                 -std::numeric_limits<double>::infinity(),
                 std::numeric_limits<double>::quiet_NaN()})
        {
            expectInvalid(
                [invalid] {
                    (void)deriveLegacyGpioIntrinsicDifferencePpm(invalid, 0.0);
                },
                "non-finite P must fail closed");
            expectInvalid(
                [invalid] {
                    (void)deriveLegacyGpioIntrinsicDifferencePpm(0.0, invalid);
                },
                "non-finite S must fail closed");
        }
    }

    double rawDetectedForReference(double reference_hz)
    {
        return reference_hz + 0.4484 + reference_hz * 1.01012e-6;
    }

    void testExactSdrInverseAndRfError()
    {
        using namespace wsprrypi;

        constexpr double reference_hz = 10000000.0;
        const double raw_hz = rawDetectedForReference(reference_hz);
        const double calibrated_hz =
            calibrateIssue429SdrFrequencyHz(raw_hz);
        expect(
            nearlyEqual(
                calibrated_hz,
                reference_hz,
                5.0e-9),
            "exact SDR inverse must recover a synthetic reference frequency");
        expect(
            std::fabs(raw_hz - reference_hz) > 10.0,
            "omitting the SDR inverse must remain detectable in the fixture");
        expect(
            std::fabs(
                calibrateIssue429SdrFrequencyHz(calibrated_hz) -
                reference_hz) > 10.0,
            "applying the SDR inverse twice must remain detectable in the fixture");

        constexpr double requested_hz = 14097100.0;
        constexpr double physical_error_ppm = 3.25;
        const double actual_hz = requested_hz * (1.0 + physical_error_ppm * 1.0e-6);
        const auto measurement = analyzeIssue429SdrMeasurement(
            requested_hz,
            rawDetectedForReference(actual_hz),
            1);
        expect(
            nearlyEqual(measurement.calibrated_fundamental_hz, actual_hz, 1.0e-8) &&
                nearlyEqual(
                    measurement.calibrated_rf_error_hz,
                    actual_hz - requested_hz,
                    1.0e-8) &&
                nearlyEqual(
                    measurement.calibrated_rf_error_ppm,
                    physical_error_ppm,
                    1.0e-9),
            "measurement analysis must retain calibrated hertz and positive-fast PPM error");

        constexpr double negative_error_ppm = -4.5;
        const double slow_actual_hz =
            requested_hz * (1.0 + negative_error_ppm * 1.0e-6);
        const auto slow_measurement = analyzeIssue429SdrMeasurement(
            requested_hz,
            rawDetectedForReference(slow_actual_hz),
            1);
        expect(
            nearlyEqual(
                slow_measurement.calibrated_rf_error_ppm,
                negative_error_ppm,
                1.0e-9),
            "measurement analysis must preserve negative-fast RF error");
    }

    void testHarmonicCalibrationOrdering()
    {
        using namespace wsprrypi;

        constexpr double requested_hz = 1000000.0;
        constexpr std::uint32_t harmonic = 7;
        constexpr double actual_fundamental_hz = requested_hz + 0.125;
        const double actual_harmonic_hz = actual_fundamental_hz * harmonic;
        const double raw_harmonic_hz = rawDetectedForReference(actual_harmonic_hz);
        const auto measurement = analyzeIssue429SdrMeasurement(
            requested_hz,
            raw_harmonic_hz,
            harmonic);

        const double wrong_order =
            calibrateIssue429SdrFrequencyHz(raw_harmonic_hz / harmonic);
        expect(
            nearlyEqual(
                measurement.calibrated_fundamental_hz,
                actual_fundamental_hz,
                1.0e-9),
            "harmonic analysis must calibrate the raw harmonic before division");
        expect(
            std::fabs(wrong_order - actual_fundamental_hz) > 0.1,
            "the fixture must detect division before removal of the affine intercept");
    }

    void testMeasurementValidation()
    {
        using namespace wsprrypi;

        for (const double invalid : {
                 0.0,
                 -1.0,
                 std::numeric_limits<double>::infinity(),
                 std::numeric_limits<double>::quiet_NaN()})
        {
            expectInvalid(
                [invalid] { (void)calibrateIssue429SdrFrequencyHz(invalid); },
                "invalid raw detected frequency must fail closed");
            expectInvalid(
                [invalid] {
                    (void)analyzeIssue429SdrMeasurement(invalid, 1000000.0, 1);
                },
                "invalid requested fundamental must fail closed");
        }
        expectInvalid(
            [] {
                (void)analyzeIssue429SdrMeasurement(1000000.0, 1000000.0, 0);
            },
            "zero unauthenticated harmonic number must fail closed");
    }
}

int main()
{
    testExactClockModels();
    testIntrinsicDifferenceSignAndValidation();
    testExactSdrInverseAndRfError();
    testHarmonicCalibrationOrdering();
    testMeasurementValidation();

    if (failures != 0)
    {
        std::cerr << failures << " legacy GPIO clock-model test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Legacy GPIO clock-model tests passed\n";
    return EXIT_SUCCESS;
}
