/**
 * @file main.hpp
 * @brief Entry point for the Wsprry Pi application.
 *
 * This file is part of WsprryPi, a project originally branched from
 * @threeme3's WsprryPi project (no longer on GitHub). However, now the
 * original code remains only as a memory and inspiration, and this project
 * is no longer a derivative work.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
 * for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
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

#ifndef _MAIN_HPP
#define _MAIN_HPP

/**
 * @brief Entry point for the WsprryPi application.
 *
 * This function initializes the application, parses command-line arguments,
 * loads the INI configuration, verifies NTP synchronization, and starts the
 * main WSPR transmission loop. It also sets system performance modes and
 * handles signal management for graceful shutdown.
 *
 * @param argc The number of command-line arguments.
 * @param argv Array of C-style strings representing the arguments.
 * @return int Exit status: 0 on success, non-zero on failure.
 *
 * @note Ensure that NTP synchronization is stable before proceeding.
 *       If NTP verification fails, the program exits immediately.
 *       The log level is set to INFO by default, but can be changed
 *       via a macro or configuration option.
 */
void main_shutdown(bool from_main = false);

#endif //_MAIN_HPP
