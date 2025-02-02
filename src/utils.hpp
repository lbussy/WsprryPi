/**
 * @file utils.hpp
 * @brief Utility functions for WsprryPi.
 * 
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 * 
 * Copyright (C) @threeme3 (unknown dates)
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 * 
 * WsprryPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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
