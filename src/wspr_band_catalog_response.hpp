/**
 * @file wspr_band_catalog_response.hpp
 * @brief Builds read-only WebSocket responses for the WSPR band catalog.
 */

#ifndef WSPR_BAND_CATALOG_RESPONSE_HPP
#define WSPR_BAND_CATALOG_RESPONSE_HPP

#include <string>

class BandLookup;

std::string build_wspr_band_catalog_response_json(
    const BandLookup &lookup,
    double audio_offset_hz);

#endif // WSPR_BAND_CATALOG_RESPONSE_HPP
