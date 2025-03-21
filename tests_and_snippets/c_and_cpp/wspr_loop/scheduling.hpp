// TODO:  Check Doxygen

/**
 * @file scheduling.hpp
 * @brief Manages transmit, INI monitoring and scheduling for Wsprry Pi
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

extern PPMManager ppmManager;

/**
 * @brief Condition variable used for thread synchronization.
 *
 * This condition variable allows the scheduler thread to wait for
 * specific conditions, such as the exit signal or the next transmission
 * interval.
 */
extern std::condition_variable shutdown_cv;

/**
 * @brief Atomic flag indicating when the scheduler should exit.
 *
 * This flag allows threads to gracefully exit by checking its value.
 * It is set to `true` when the program needs to terminate the scheduler
 * and associated threads.
 */
extern std::atomic<bool> exit_wspr_loop;

/**
 * @brief Manages the main WSPR transmission loop.
 *
 * This function runs the primary WSPR transmission loop, coordinating the
 * INI monitor thread and the scheduler thread. It continuously:
 * - Starts the `ini_monitor_thread` to detect INI file changes.
 * - Spawns the `scheduler_thread` to manage transmission intervals.
 * - Waits for the scheduler to signal transmission readiness.
 * - Initiates a transmission when the interval is reached.
 *
 * The loop exits when the `exit_wspr_loop` atomic flag is set.
 *
 * @note This function runs until externally signaled to stop.
 *
 * @return void
 */
extern void wspr_loop();

void shutdown_threads();

#endif // _SCHEDULING_HPP
