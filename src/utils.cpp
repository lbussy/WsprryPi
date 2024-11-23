// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file utils.cpp
 * @brief Implementation of utility functions for WsprryPi.
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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-32d490b [refactoring].
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
