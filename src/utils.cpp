/**
 * @file utils.cpp
 * @brief Implementation of utility functions for WsprryPi.
 * 
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 * 
 * Copyright (C) @threeme3 (unknown dates)
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
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

// Unit testing:
// g++ -Wall -Werror -std=c++17 -DDEBUG_MAIN_UTILS -g utils.cpp -o utils
// Test command:
// ./utils

#include <cctype>   // For std::toupper
#include <iostream> // For std::cout and std::endl
#include "utils.hpp"

/**
 * @brief Converts a C-string to uppercase in place.
 *
 * This function modifies the provided string by converting each character
 * to uppercase using `std::toupper`.
 *
 * @param str A pointer to a null-terminated C-string. The string is modified in place.
 */
void to_upper(char* str) {
    while (*str) {
        *str = static_cast<char>(std::toupper(*str)); // Safely cast to char
        ++str;
    }
}

#ifdef DEBUG_MAIN_UTILS

/**
 * @brief Debug-only main function to test the `to_upper` utility.
 *
 * This function tests the `to_upper` function with a sample string and outputs
 * the result to the console. Included only when `DEBUG_MAIN_UTILS` is defined.
 *
 * @return Exit status of the program.
 */
int main() {
    char testStr[] = "hello, world!";
    std::cout << "Before: " << testStr << std::endl;
    to_upper(testStr);
    std::cout << "After: " << testStr << std::endl;
    return 0;
}

#endif // DEBUG_MAIN_UTILS
