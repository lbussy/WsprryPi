/**
 * @file utils.cpp
 * @brief Miscellaneous functions for the project. 
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 *
 * However, this portion of new code added to the project is distributed under
 * under the MIT License. See LICENSE for more information.
 *
 * Copyright (c) 2009-2020, Ben Hoyt, All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
