#ifndef _WSPR_ENCODER_H
#define _WSPR_ENCODER_H

// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/*
 * WsprryPi
 * Updated and maintained by Lee C. Bussy
 *
 * Originally forked from threeme3/WsprryPi (no longer active), this
 * project has been significantly updated, improved, and documented for
 * ease of use.
 *
 * Inspired by a conversation with Bruce Raymond of TAPR, this fork has
 * diverged substantially from its origins and operates as an independent
 * project.
 *
 * Contributors:
 *   - threeme3 (Original Author)
 *   - Bruce Raymond (Inspiration and Guidance)
 *   - Lee Bussy, aa0nt@arrl.net
 *
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
*/

#include <string>

// Test compile:
// g++ -Wall -Werror -std=c++11 -DDEBUG_MAIN wspr_encoder.cpp utils.cpp -o wspr_encoder

/**
 * @brief Encodes WSPR payload data.
 *
 * Packs the callsign, grid square, and power level into a 50-bit payload,
 * performs convolutional encoding, and interleaves the resulting symbols
 * for transmission.
 *
 * @param call Callsign (e.g., "K1ABC").
 * @param l_pre Locator prefix (e.g., "FN42").
 * @param dbm Transmission power level in dBm.
 * @param symbols Output array to hold the encoded symbols.
 */
void wspr(const char* call, const char* l_pre, const char* dbm, unsigned char* symbols);

#endif
