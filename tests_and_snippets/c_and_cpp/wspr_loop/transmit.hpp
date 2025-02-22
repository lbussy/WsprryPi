/**
 * @file transmit.cpp
 * @brief Handles Wsprry Pi transmission.
 *
 * This file is part of WsprryPi, a project originally created from @threeme3
 * WsprryPi projet (no longer on GitHub). However, now the original code
 * remains only as a memory and inspiration, and this project is no longer
 * a deriivative work.
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

#ifndef _TRANSMIT_HPP
#define _TRANSMIT_HPP

#include <atomic>

/**
 * @brief Indicates whether a transmission is currently active.
 *
 * This atomic boolean flag is used to signal the start and end of a transmission.
 * It ensures thread-safe access across multiple threads, preventing race conditions.
 *
 * @details
 * - **true:** Transmission is in progress.
 * - **false:** No active transmission.
 *
 * This flag is primarily updated by the `transmit()` function and read by other threads
 * to determine if the system is currently transmitting.
 *
 * @note Using `std::atomic` ensures safe concurrent access without the need for external locks.
 *
 * Example usage:
 * @code
 * in_transmission.store(true);  // Mark transmission start
 * bool status = in_transmission.load();  // Check status
 * in_transmission.store(false); // Mark transmission end
 * @endcode
 */
extern std::atomic<bool> in_transmission;

/**
 * @brief Initiates and manages the transmission process.
 *
 * This function performs a transmission for 110 seconds unless interrupted by a shutdown request.
 * It ensures real-time priority during transmission and resets priority upon completion.
 * The transmission duration is logged, including early termination if requested.
 *
 * @details The function continuously checks for a shutdown signal while transmitting, sleeping
 *          in short intervals (100 ms) to allow interruption. It calculates the actual duration
 *          of the transmission and resets the process priority upon completion.
 *
 * @global exit_scheduler Atomic flag indicating if the scheduler should exit.
 * @global in_transmission Atomic flag indicating if a transmission is active.
 * @global llog Logger instance for logging messages.
 *
 * @throws Logs an error if priority reset fails after transmission.
 */
extern void transmit();

#endif // _TRANSMIT_HPP
