// TODO:  Check Doxygen

/**
 * @file scheduling.cpp
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

// Primary header for this source file
#include "scheduling.hpp"

// Project headers
#include "arg_parser.hpp"
#include "constants.hpp"
#include "signal_handler.hpp"
#include "transmit.hpp"
#include "logging.hpp"
#include "gpio_handler.hpp"
#include "ppm_manager.hpp"
#include "led_handler.hpp"
#include "shutdown_handler.hpp"

// Standard library headers
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

// System headers
#include <string.h>
#include <sys/resource.h>

std::mutex shutdown_mtx; // Global mutex for thread safety.

PPMManager ppmManager;

/**
 * @brief Condition variable used for thread synchronization.
 *
 * This condition variable allows the scheduler thread to wait for
 * specific conditions, such as the exit signal or the next transmission
 * interval.
 */
std::condition_variable shutdown_cv;

/**
 * @brief Mutex for protecting access to shared resources.
 *
 * This mutex is used to synchronize access to shared data between threads,
 * ensuring thread safety when accessing the condition variable.
 */
std::mutex cv_mtx;

/**
 * @brief Atomic flag indicating when the WSPR loop should exit.
 *
 * This flag allows threads to gracefully exit by checking its value.
 * It is set to `true` when the program needs to terminate the loop
 * and associated threads.
 */
std::atomic<bool> exit_wspr_loop(false);

bool ppm_init()
{
    // Initialize PPM Manager
    PPMStatus status = ppmManager.initialize();

    switch (status)
    {
    case PPMStatus::SUCCESS:
        llog.logS(DEBUG, "PPM Manager initialized sucessfully.");
        break;

    case PPMStatus::WARNING_HIGH_PPM:
        llog.logS(ERROR, "Warning: Measured PPM exceeds safe threshold.");
        return false;

    case PPMStatus::ERROR_CHRONY_NOT_FOUND:
        llog.logE(WARN, "Chrony not found. Falling back to clock drift measurement.");
        break;

    case PPMStatus::ERROR_UNSYNCHRONIZED_TIME:
        llog.logE(ERROR, "System time is not synchronized. Unable to measure PPM accurately.");
        return false;

    default:
        llog.logE(WARN, "Unknown PPM status.");
        break;
    }

    llog.logS(INFO, "Current PPM:", ppmManager.getCurrentPPM());
    return true;
}

void wspr_loop()
{
    // Begin tracking PPM clock variance
    if (ini.get_bool_value("Extended", "Use NTP")) // TODO:  Create in-memory only ini.
    {
        if (!ppm_init())
        {
            llog.logE(FATAL, "Unable to initialze NTP features.");
            return;
        }
    }

    llog.logS(INFO, "WSPR loop running.");

    // Start the transmit thread.
    transmit_thread = std::thread(transmit_loop);
    llog.logS(INFO, "Transmission handler thread started.");

    // Wait for exit signal (shutdown_cv).
    std::unique_lock<std::mutex> lock(cv_mtx);
    shutdown_cv.wait(lock, []
                     { return exit_wspr_loop.load(); });

    // Stop PPM Manager
    ppmManager.stop();
    // Stop INI Monitor
    iniMonitor.stop();
    // Signal shutdown.
    signal_shutdown.store(true);
    // Cleanup threads
    shutdown_threads();

    llog.logS(DEBUG, "Checking all threads before exiting wspr_loop.");

    if (led_handler)
        toggle_led(false);
    led_handler.reset();
    shutdown_handler.reset();

    // Identify any stuck threads
    if (button_thread.joinable()) // TODO: Update class
        llog.logS(WARN, "Button thread still running.");
    if (transmit_thread.joinable()) // TODO: Create new class
        llog.logS(WARN, "Transmit thread still running.");

    return;
}

void shutdown_threads()
{
    std::lock_guard<std::mutex> lock(shutdown_mtx);

    llog.logS(INFO, "Shutting down all active threads.");
    signal_shutdown.store(true);

    auto safe_join = [](std::thread &t, const std::string &name)
    {
        if (t.joinable())
        {
            llog.logS(DEBUG, "Joining:", name);
            t.join();
            llog.logS(DEBUG, name, "stopped.");
        }
        else
        {
            llog.logS(DEBUG, name, " already exited, skipping join.");
        }
    };

    safe_join(transmit_thread, "Transmit thread");     // TODO: Create a class
    safe_join(button_thread, "Button monitor thread"); // TODO: Change this class

    llog.logS(INFO, "All threads shut down safely.");
}
