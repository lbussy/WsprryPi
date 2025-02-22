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

// Primary header for this source file
#include "transmit.hpp"

// Project headers
#include "scheduling.hpp"
#include "arg_parser.hpp"

// Standard library headers
#include <atomic>
#include <iomanip>

// System headers
#include <string.h>
#include <sys/resource.h>
#include <time.h>

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
std::atomic<bool> in_transmission(false);

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
void transmit()
{
    // Check if shutdown is requested before starting
    if (exit_scheduler.load())
    {
        llog.logS(WARN, "Shutdown requested. Skipping transmission.");
        return;
    }

    // Mark that transmission is in progress
    in_transmission.store(true);

    // Set real-time priority for transmission
    set_transmission_realtime();
    llog.logS(INFO, "Transmission started.");

    // Record the start time of the transmission
    timespec start_time{}, end_time{};
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1)
    {
        llog.logE(ERROR, "Failed to get start time:", std::string(strerror(errno)));
        in_transmission.store(false);
        return;
    }

    // Define the target end time for the transmission (110 seconds)
    timespec target_time = start_time;
    target_time.tv_sec += 110;

    // Perform the transmission with periodic checks for shutdown
    while (true)
    {
        if (exit_scheduler.load())
        {
            llog.logS(WARN, "Shutdown requested. Aborting transmission.");
            break; // Exit early if shutdown is requested
        }

        // Check the current time to determine if the transmission should end
        timespec now{};
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
        {
            llog.logE(ERROR, "Failed to get current time:", std::string(strerror(errno)));
            break;
        }

        // Break if the target time has been reached
        if ((now.tv_sec > target_time.tv_sec) ||
            (now.tv_sec == target_time.tv_sec && now.tv_nsec >= target_time.tv_nsec))
        {
            break; // Transmission time completed
        }

        // Sleep for a short interval (100 milliseconds) and recheck
        timespec sleep_interval = {0, 100'000'000}; // 100 ms in nanoseconds
        if (clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_interval, nullptr) != 0)
        {
            llog.logE(ERROR, "Sleep interrupted:", std::string(strerror(errno)));
            break;
        }
    }

    // Record the end time, even if interrupted
    if (clock_gettime(CLOCK_MONOTONIC, &end_time) == -1)
    {
        llog.logE(ERROR, "Failed to get end time:", std::string(strerror(errno)));
        in_transmission.store(false);
        return;
    }

    // Calculate the actual duration of the transmission
    double actual_duration = (end_time.tv_sec - start_time.tv_sec) +
                             (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    // Log the transmission result (completed or interrupted)
    if (exit_scheduler.load())
    {
        llog.logS(WARN, "Transmission aborted after ", std::fixed, std::setprecision(6),
                  actual_duration, " seconds.");
    }
    else
    {
        llog.logS(INFO, std::fixed, std::setprecision(6),
                  "Transmission complete: ", actual_duration, " seconds.");
    }

    // Mark transmission as complete
    in_transmission.store(false);

    // Reset process priority after transmission
    if (setpriority(PRIO_PROCESS, 0, 0) == -1)
    {
        llog.logE(ERROR, "Failed to reset transmission priority:", std::string(strerror(errno)));
    }
    else
    {
        llog.logS(DEBUG, "Transmission priority reset to normal.");
    }
}
