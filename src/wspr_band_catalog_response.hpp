/**
 * @file wspr_band_catalog_response.hpp
 * @brief Builds read-only WebSocket responses for the WSPR band catalog.
 */

#ifndef WSPR_BAND_CATALOG_RESPONSE_HPP
#define WSPR_BAND_CATALOG_RESPONSE_HPP

#include <string>
#include <unordered_map>

class BandLookup;

std::string build_wspr_band_catalog_response_json(
    const BandLookup &lookup,
    double audio_offset_hz,
    const std::string &frequency_profile = "existing_common",
    const std::unordered_map<std::string, std::string> &band_preferences = {});

#endif // WSPR_BAND_CATALOG_RESPONSE_HPP
