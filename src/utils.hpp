// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file utils.hpp
 * @brief Utility functions for WsprryPi.
 *
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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-7f4c707 [refactoring].
 */

#ifndef _UTILS_HPP
#define _UTILS_HPP

/**
 * @brief Converts a C-string to uppercase in place.
 *
 * This function modifies the input string by converting each character
 * to its uppercase equivalent using `std::toupper`. The string must be
 * null-terminated.
 *
 * @param str Pointer to the null-terminated string to convert.
 */
void to_upper(char* str);

#endif // _UTILS_HPP
