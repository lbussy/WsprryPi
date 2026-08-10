#include "system_clock_frequency_estimate.hpp"

#include <algorithm>
#include <cmath>

namespace
{
bool finite_bounded(double value)
{
    return std::isfinite(value) && value >= -200.0 && value <= 200.0;
}
}

SystemClockFrequencyEstimate FrequencyEstimateQualifier::evaluate(
    SystemClockFrequencyEstimate sample)
{
    const auto attach_stale_fallback = [&]()
    {
        if (!last_qualified_.has_value())
            return;
        const double age = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - last_qualified_at_).count();
        if (age <= kStaleMaximumAgeSeconds)
        {
            sample.last_qualified_frequency_ppm = last_qualified_->frequency_ppm;
            sample.last_qualified_age_seconds = age;
        }
    };
    sample.qualification = FrequencyEstimateQualification::Unavailable;

    if (!sample.frequency_ppm.has_value() || !finite_bounded(*sample.frequency_ppm))
    {
        sample.reason = "Provider did not return a valid frequency estimate.";
        attach_stale_fallback();
        return sample;
    }
    if (!sample.synchronized || !sample.selected_source || !sample.leap_normal)
    {
        sample.reason = "Provider is not synchronized to a selected source with normal leap status.";
        attach_stale_fallback();
        return sample;
    }
    if (sample.age_seconds > kStaleMaximumAgeSeconds)
    {
        sample.reason = "The last provider estimate is older than the stale fallback interval.";
        attach_stale_fallback();
        return sample;
    }
    if (sample.age_seconds > kCurrentMaximumAgeSeconds)
    {
        sample.qualification = FrequencyEstimateQualification::Stale;
        sample.reason = "The last qualified provider estimate is stale.";
        sample.last_qualified_frequency_ppm = sample.frequency_ppm;
        sample.last_qualified_age_seconds = sample.age_seconds;
        return sample;
    }
    if (!sample.skew_ppm.has_value() || !std::isfinite(*sample.skew_ppm) ||
        *sample.skew_ppm > kMaximumSkewPpm)
    {
        sample.qualification = FrequencyEstimateQualification::Converging;
        sample.reason = "Provider skew exceeds the qualification limit.";
        attach_stale_fallback();
        return sample;
    }
    if (!sample.residual_frequency_ppm.has_value() ||
        !std::isfinite(*sample.residual_frequency_ppm) ||
        std::abs(*sample.residual_frequency_ppm) > kMaximumResidualFrequencyPpm)
    {
        sample.qualification = FrequencyEstimateQualification::Converging;
        sample.reason = "Provider residual frequency exceeds the qualification limit.";
        attach_stale_fallback();
        return sample;
    }
    if (sample.retained_source_samples < kMinimumProviderRetainedSamples)
    {
        sample.qualification = FrequencyEstimateQualification::Converging;
        sample.reason = "The selected provider sources do not yet retain enough observations.";
        attach_stale_fallback();
        return sample;
    }

    if (sample.source_signature.empty() || sample.source_signature != source_signature_)
    {
        source_signature_ = sample.source_signature;
        frequency_history_.clear();
    }
    frequency_history_.push_back(*sample.frequency_ppm);
    while (frequency_history_.size() > kRequiredStableSamples)
    {
        frequency_history_.pop_front();
    }

    const auto bounds = std::minmax_element(
        frequency_history_.begin(), frequency_history_.end());
    if (frequency_history_.size() < kRequiredStableSamples ||
        *bounds.second - *bounds.first > kMaximumSampleSpreadPpm)
    {
        sample.qualification = FrequencyEstimateQualification::Converging;
        sample.reason = "Provider estimate has not completed the stability observation window.";
        attach_stale_fallback();
        return sample;
    }

    sample.qualification = FrequencyEstimateQualification::Qualified;
    sample.reason = "Provider estimate meets the configured quality and stability criteria.";
    last_qualified_ = sample;
    last_qualified_at_ = std::chrono::steady_clock::now();
    return sample;
}

void FrequencyEstimateQualifier::reset()
{
    source_signature_.clear();
    frequency_history_.clear();
    last_qualified_.reset();
    last_qualified_at_ = {};
}

GpioFrequencyCorrection select_gpio_frequency_correction(
    bool use_estimate,
    double residual_ppm,
    double manual_ppm,
    const SystemClockFrequencyEstimate &estimate)
{
    GpioFrequencyCorrection result;
    result.qualification = estimate.qualification;
    result.provider_name = estimate.provider_name;
    result.source_provenance = estimate.source_provenance;
    result.reason = estimate.reason;
    result.estimate_age_seconds = estimate.age_seconds;
    result.residual_ppm = residual_ppm;

    if (use_estimate && estimate.frequency_ppm.has_value() &&
        estimate.qualification == FrequencyEstimateQualification::Qualified)
    {
        result.estimate_ppm = estimate.frequency_ppm;
        result.effective_ppm = *estimate.frequency_ppm + residual_ppm;
        result.mode = GpioCorrectionMode::QualifiedEstimate;
        return result;
    }

    const std::optional<double> stale_estimate =
        estimate.qualification == FrequencyEstimateQualification::Stale
            ? estimate.frequency_ppm
            : estimate.last_qualified_frequency_ppm;
    if (use_estimate && stale_estimate.has_value())
    {
        result.estimate_ppm = stale_estimate;
        result.estimate_age_seconds = estimate.qualification == FrequencyEstimateQualification::Stale
            ? estimate.age_seconds
            : estimate.last_qualified_age_seconds;
        result.effective_ppm = *stale_estimate + residual_ppm;
        result.mode = GpioCorrectionMode::StaleEstimate;
        return result;
    }

    if (finite_bounded(manual_ppm) && (!use_estimate || manual_ppm != 0.0))
    {
        result.effective_ppm = manual_ppm;
        result.mode = GpioCorrectionMode::FixedManual;
        if (use_estimate)
        {
            result.reason = estimate.reason.empty()
                ? "Provider estimate is not qualified; using fixed manual correction."
                : estimate.reason + " Using fixed manual correction.";
        }
        return result;
    }

    result.effective_ppm = 0.0;
    result.mode = GpioCorrectionMode::Uncalibrated;
    result.reason = "No valid provider estimate or fixed manual correction is available.";
    return result;
}

const char *to_string(FrequencyEstimateQualification value) noexcept
{
    switch (value)
    {
    case FrequencyEstimateQualification::Qualified: return "qualified";
    case FrequencyEstimateQualification::Converging: return "converging";
    case FrequencyEstimateQualification::Stale: return "stale";
    case FrequencyEstimateQualification::Unavailable: return "unavailable";
    }
    return "unavailable";
}

const char *to_string(GpioCorrectionMode value) noexcept
{
    switch (value)
    {
    case GpioCorrectionMode::QualifiedEstimate: return "qualified_estimate_plus_residual";
    case GpioCorrectionMode::StaleEstimate: return "stale_estimate_plus_residual";
    case GpioCorrectionMode::FixedManual: return "fixed_manual";
    case GpioCorrectionMode::Uncalibrated: return "uncalibrated";
    }
    return "uncalibrated";
}
