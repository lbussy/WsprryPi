/**
 * @file test.cpp
 * @brief Hardware-free PPM provider snapshot parser test.
 *
 * Licensed under the repository-root LICENSE.md.
 * Copyright © 2025 - 2026 Lee C. Bussy (@LBussy). All rights reserved.
 */

#include "ppm_manager.hpp"

#include <cmath>
#include <iostream>

namespace
{
bool near(double actual, double expected)
{
    return std::abs(actual - expected) < 0.000001;
}
}

int main()
{
    const std::string tracking =
        "REF,1,2,3,4,5,6,-1.250,0.125,0.500,10,11,12,Normal\n";
    const std::string sources =
        "0,*,GPS,3,4,5,7.5\n"
        "0,+,pool.example,3,4,5,8.0\n";
    const std::string stats =
        "GPS,12,2,3600\n"
        "pool.example,9,2,1800\n";

    const auto snapshot =
        PPMManager::parseChronyReports(tracking, sources, stats);
    const bool valid = snapshot.frequency_ppm.has_value() &&
                       near(*snapshot.frequency_ppm, -1.250) &&
                       snapshot.residual_frequency_ppm.has_value() &&
                       near(*snapshot.residual_frequency_ppm, 0.125) &&
                       snapshot.skew_ppm.has_value() &&
                       near(*snapshot.skew_ppm, 0.500) &&
                       snapshot.synchronized && snapshot.selected_source &&
                       snapshot.combined_sources && snapshot.leap_normal &&
                       near(snapshot.age_seconds, 7.5) &&
                       snapshot.source_signature == "*:GPS,+:pool.example" &&
                       snapshot.source_provenance == "Mixed" &&
                       snapshot.retained_source_samples == 9 &&
                       near(snapshot.source_stability_span_seconds, 1800.0) &&
                       snapshot.error_reason.empty();

    const auto unavailable = PPMManager::parseChronyReports("", "", "");
    const bool unavailableValid =
        !unavailable.frequency_ppm.has_value() &&
        !unavailable.synchronized &&
        unavailable.error_reason ==
            "chrony tracking data is unavailable or incomplete";

    if (!valid || !unavailableValid)
    {
        std::cerr << "PPM provider snapshot parser test failed.\n";
        return 1;
    }
    return 0;
}
