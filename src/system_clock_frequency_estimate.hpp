#ifndef SYSTEM_CLOCK_FREQUENCY_ESTIMATE_HPP
#define SYSTEM_CLOCK_FREQUENCY_ESTIMATE_HPP

#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <vector>

enum class FrequencyEstimateQualification
{
    Qualified,
    Converging,
    Unavailable,
    Stale
};

enum class GpioCorrectionMode
{
    QualifiedEstimate,
    StaleEstimate,
    FixedManual,
    Uncalibrated
};

struct SystemClockFrequencyEstimate
{
    std::string provider_name;
    FrequencyEstimateQualification qualification = FrequencyEstimateQualification::Unavailable;
    std::optional<double> frequency_ppm;
    std::optional<double> last_qualified_frequency_ppm;
    double last_qualified_age_seconds = 0.0;
    bool synchronized = false;
    double age_seconds = 0.0;
    std::optional<double> residual_frequency_ppm;
    std::optional<double> skew_ppm;
    bool selected_source = false;
    bool combined_sources = false;
    bool leap_normal = false;
    std::string source_provenance;
    std::string source_signature;
    std::size_t retained_source_samples = 0;
    double source_stability_span_seconds = 0.0;
    std::string reason;
};

struct GpioFrequencyCorrection
{
    FrequencyEstimateQualification qualification = FrequencyEstimateQualification::Unavailable;
    GpioCorrectionMode mode = GpioCorrectionMode::Uncalibrated;
    std::string provider_name;
    std::string source_provenance;
    std::string reason;
    std::optional<double> estimate_ppm;
    double residual_ppm = 0.0;
    double effective_ppm = 0.0;
    double estimate_age_seconds = 0.0;
};

class FrequencyEstimateQualifier
{
public:
    static constexpr std::size_t kRequiredStableSamples = 3;
    static constexpr double kMaximumSkewPpm = 1.0;
    static constexpr double kMaximumResidualFrequencyPpm = 0.5;
    static constexpr double kMaximumSampleSpreadPpm = 0.1;
    static constexpr std::size_t kMinimumProviderRetainedSamples = 3;
    static constexpr double kCurrentMaximumAgeSeconds = 300.0;
    static constexpr double kStaleMaximumAgeSeconds = 900.0;

    SystemClockFrequencyEstimate evaluate(SystemClockFrequencyEstimate sample);
    void reset();

private:
    std::string source_signature_;
    std::deque<double> frequency_history_;
    std::optional<SystemClockFrequencyEstimate> last_qualified_;
    std::chrono::steady_clock::time_point last_qualified_at_{};
};

GpioFrequencyCorrection select_gpio_frequency_correction(
    bool use_estimate,
    double residual_ppm,
    double manual_ppm,
    const SystemClockFrequencyEstimate &estimate);

const char *to_string(FrequencyEstimateQualification value) noexcept;
const char *to_string(GpioCorrectionMode value) noexcept;

#endif
