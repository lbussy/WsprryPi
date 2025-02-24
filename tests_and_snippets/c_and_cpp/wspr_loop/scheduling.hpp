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

// Standard library headers
#include <atomic>
#include <condition_variable>
#include <string>
#include <thread>

/**
 * @brief Condition variable used for thread synchronization.
 *
 * This condition variable allows the scheduler thread to wait for
 * specific conditions, such as the exit signal or the next transmission
 * interval.
 */
extern std::condition_variable cv;

/**
 * @brief Atomic flag indicating when the scheduler should exit.
 *
 * This flag allows threads to gracefully exit by checking its value.
 * It is set to `true` when the program needs to terminate the scheduler
 * and associated threads.
 */
extern std::atomic<bool> exit_wspr_loop;

/**
 * @brief Checks if the directory name represents a real CPU directory (e.g. "cpu0").
 *
 * We do a quick check:
 * - Must start with "cpu"
 * - Must have at least 4 characters ("cpu0" is the shortest)
 * - All characters after "cpu" must be digits.
 */
extern bool is_cpu_directory(const std::string &dirName);

/**
 * @brief Populates the list of valid CPU core directories.
 *
 * This function scans `/sys/devices/system/cpu/` and stores directory names like
 * "cpu0", "cpu1", etc. in the global `all_cpu_cores` vector for later use.
 */
extern void discover_cpu_cores();

/**
 * @brief Checks if any CPU core is throttled due to temperature or frequency.
 *
 * This function evaluates CPU throttling by:
 * - Checking the highest CPU temperature from `/sys/class/thermal/thermal_zone0/temp`.
 * - Verifying the current frequency of each CPU core from
 *   `/sys/devices/system/cpu/n/cpufreq/scaling_cur_freq`.
 *
 * Logs warnings if the temperature exceeds 80°C or if any core runs below its
 * default frequency based on the detected processor type.
 *
 * @note Ensure the application has sufficient permissions to read the required
 *       system files. Missing permissions or invalid paths will log errors.
 *
 * @return true if any CPU throttling is detected (temperature or frequency).
 * @return false if no throttling is detected and all cores operate normally.
 */
extern bool is_cpu_throttled();

/**
 * @brief Sets the current transmission thread to real-time priority.
 *
 * This function configures the calling thread to use the `SCHED_FIFO`
 * scheduling policy with a mid-to-high priority level of 75. This ensures
 * that the transmission thread receives preferential CPU time, reducing
 * latency and jitter during critical operations.
 *
 * If setting the real-time priority fails, an error is logged with the
 * corresponding system error message.
 *
 * @note This function requires appropriate system privileges (e.g., CAP_SYS_NICE)
 *       to modify thread priorities. Without elevated permissions, the call
 *       to `pthread_setschedparam()` will fail.
 *
 */
extern void set_transmission_realtime();

/**
 * @brief Sets the scheduling priority for the current process.
 *
 * This function increases the process priority by setting its nice value to -10.
 * A lower nice value increases priority, ensuring the housekeeping thread receives
 * more CPU time when competing with other processes.
 *
 * If the priority change fails, an error message is logged with the corresponding
 * system error description.
 *
 * @note Changing priority requires appropriate system privileges. Without sufficient
 *       permissions (CAP_SYS_NICE), the call to `setpriority()` will fail.
 *
 * @return void
 */
extern void set_scheduling_priority();

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
