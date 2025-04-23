/**
 * @file scheduling.hpp
 * @brief Manages transmit, INI monitoring and scheduling for Wsprry Pi
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

#ifndef _SCHEDULING_HPP
#define _SCHEDULING_HPP

// Project headers
#include "arg_parser.hpp"
#include "ppm_manager.hpp"

// Standard library headers
#include <atomic>
#include <condition_variable>
#include <string>
#include <thread>

/**
 * @brief Flag indicating if a system reboot is in progress.
 *
 * @details
 * This atomic flag is used throughout the application to signal when a
 * full system reboot has been initiated. It is typically set from one
 * of the control points (REST or websockets).
 *
 * Other threads can poll or wait on this flag to terminate safely.
 */
extern std::atomic<bool> reboot_flag;

/**
 * @brief Condition variable used for shutdown synchronization.
 *
 * @details
 * Allows threads to block until a shutdown event occurs, such as a signal
 * from a GPIO input or user request. Typically used in conjunction with
 * `exit_wspr_loop`.
 */
extern std::condition_variable shutdown_cv;

/**
 * @brief Flag indicating if a system shutdown is in progress.
 *
 * @details
 * This atomic flag is used throughout the application to signal when a
 * full system shutdown has been initiated. It is typically set during a
 * GPIO-triggered shutdown or from a critical failure path.
 *
 * Other threads can poll or wait on this flag to terminate safely.
 */
extern std::atomic<bool> shutdown_flag;

/**
 * @brief Global instance of the PPM (parts-per-million) manager.
 *
 * @details
 * Responsible for calculating and managing the system's frequency
 * correction based on clock drift or NTP data. Provides runtime
 * adjustments for frequency stability during WSPR transmission.
 */
extern PPMManager ppmManager;

/**
 * @brief Flag used to signal exit from the main WSPR loop.
 *
 * @details
 * This atomic boolean flag is set to `true` when the WSPR loop and
 * associated subsystems should terminate. It is checked by the main
 * `wspr_loop()` and other threads to trigger graceful shutdown.
 */
extern std::atomic<bool> exit_wspr_loop;

/**
 * @brief Callback triggered by a shutdown GPIO event.
 *
 * @details
 * This function is executed when a shutdown pin is activated.
 * It performs visual signaling (e.g., LED blinks), sets shutdown flags,
 * and notifies waiting threads to begin system shutdown procedures.
 *
 * @note This is usually registered with a GPIO monitor.
 */
extern void callback_shutdown_system();

/**
 * @brief Runs the main WSPR scheduler and transmission loop.
 *
 * @details
 * Coordinates all core runtime components:
 * - Initializes optional NTP/PPM drift correction.
 * - Starts the TCP command server and sets its priority.
 * - Launches the WSPR transmission thread.
 * - Waits on the `shutdown_cv` condition variable for a shutdown signal.
 * - Performs full cleanup and shutdown sequence before exiting.
 *
 * @note This function blocks and runs until `exit_wspr_loop` is set to `true`.
 */
extern bool wspr_loop();

#endif // _SCHEDULING_HPP
