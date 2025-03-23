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
#include "config_handler.hpp"
#include "constants.hpp"
#include "gpio_input.hpp"
#include "gpio_output.hpp"
#include "logging.hpp"
#include "ppm_manager.hpp"
#include "signal_handler.hpp"
#include "tcp_server.hpp"
#include "transmit.hpp"

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

/**
 * @brief Global mutex for coordinating shutdown and thread safety.
 *
 * Used to protect shared data during shutdown, ensuring only one thread
 * initiates and executes the shutdown procedure.
 */
std::mutex shutdown_mtx;

/**
 * @brief Atomic flag indicating that a new PPM value needs to be applied.
 *
 * Set to `true` when a new PPM value has been received, signaling that
 * subsystems should reload or reconfigure based on the new frequency offset.
 */
std::atomic<bool> ppm_reload_pending(false);

/**
 * @brief Atomic flag indicating that a shutdown sequence has begun.
 *
 * Set by GPIO or system-triggered shutdown paths to initiate coordinated
 * shutdown across all subsystems.
 */
std::atomic<bool> shutdown_flag{false};

/**
 * @brief Global instance of the PPMManager class.
 *
 * Responsible for measuring and managing frequency drift (PPM)
 * using either NTP data or internal estimation methods.
 */
PPMManager ppmManager;

/**
 * @brief Global instance of the TCP server.
 *
 * Listens for incoming TCP connections and processes supported
 * WSPR server commands (e.g., frequency, grid, power).
 */
TCP_Server server;

/**
 * @brief Global instance of the TCP command handler.
 *
 * Provides the interface for parsing and executing incoming TCP commands.
 * Works in coordination with `TCP_Server`.
 */
TCP_Commands handler;

/**
 * @brief Condition variable used to wait for shutdown signals.
 *
 * Used primarily by the main `wspr_loop()` to block execution
 * until an exit signal is triggered (e.g., via GPIO, CLI, or remote command).
 */
std::condition_variable shutdown_cv;

/**
 * @brief Mutex for protecting access to shared condition variables.
 *
 * Ensures safe concurrent access when using `shutdown_cv` or other
 * related state variables during wait/notify operations.
 */
std::mutex cv_mtx;

/**
 * @brief Atomic flag used to exit the main WSPR loop.
 *
 * This is the central control flag for stopping the main transmission loop.
 * Set to `true` when a shutdown is triggered to begin safe thread teardown.
 */
std::atomic<bool> exit_wspr_loop(false);

/**
 * @brief Callback function to update the configured PPM (frequency offset).
 *
 * @details
 * This function updates the global `config.ppm` value with a new PPM correction
 * value. It also sets the `ppm_reload_pending` flag to notify other parts of
 * the system that a reload or recalibration is necessary.
 *
 * This function may be triggered by user input, external calibration, or
 * another subsystem responsible for frequency adjustments.
 *
 * @param new_ppm The new PPM correction value to apply.
 */
void ppm_callback(double new_ppm)
{
    // TODO: Handle resetting the DMA subsystem if required after PPM change.
    config.ppm = new_ppm;
    ppm_reload_pending.store(true);
    return;
}

/**
 * @brief Initializes the PPM manager and registers a callback.
 *
 * @details
 * This function attempts to initialize the `ppmManager`, which is responsible
 * for calculating or retrieving the system's PPM (parts per million) drift
 * for accurate frequency generation.
 *
 * The initialization result is evaluated, and appropriate logging is performed
 * based on the returned `PPMStatus`. Critical failure conditions such as
 * high PPM or lack of time synchronization cause the function to return `false`.
 * If successful or recoverable, the PPM callback is registered.
 *
 * @return `true` if initialization succeeded or fallback is acceptable.
 * @return `false` if a critical error was detected (e.g., high PPM or unsynced time).
 *
 * @note
 * The `ppm_callback()` will be triggered later to handle live updates to PPM.
 */
bool ppm_init()
{
    // Initialize the PPM Manager
    PPMStatus status = ppmManager.initialize();

    switch (status)
    {
    case PPMStatus::SUCCESS:
        llog.logS(DEBUG, "PPM Manager initialized successfully.");
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

    ppmManager.setPPMCallback(ppm_callback);
    return true;
}

/**
 * @brief Callback function triggered to perform a system shutdown sequence.
 *
 * @details
 * This function is intended to be called when a shutdown GPIO event is triggered.
 * It performs a visual blink pattern on the configured LED pin, sets the shutdown
 * flags, and notifies all threads waiting on the shutdown condition variable.
 *
 * Specifically:
 * - Logs that a shutdown was initiated from GPIO.
 * - Toggles the LED 3 times with 100ms intervals.
 * - Sets `exit_wspr_loop` to break out of the main transmission loop.
 * - Notifies `shutdown_cv` to unblock any waiting threads.
 * - Sets `shutdown_flag` to mark that a full system shutdown is in progress.
 *
 * @note
 * The LED toggling uses `ledControl.toggle_gpio()` and assumes the hardware supports it.
 */
void callback_shutdown_system()
{
    llog.logS(INFO, "Shutdown called by GPIO:", config.shutdown_pin);

    for (int i = 0; i < 3; ++i)
    {
        ledControl.toggle_gpio(true); // LED ON
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        ledControl.toggle_gpio(false); // LED OFF
        if (i < 2)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    exit_wspr_loop.store(true);  // Signal WSPR loop to exit
    shutdown_cv.notify_all();    // Wake any threads blocked on shutdown
    shutdown_flag.store(true);   // Indicate shutdown sequence active
}

/**
 * @brief Main control loop for WSPR operation.
 *
 * @details
 * This function initializes and coordinates all core components required for
 * WSPR transmission. It sets up optional NTP-based PPM correction, launches
 * the TCP server, spawns the transmission handler thread, and blocks until
 * a shutdown signal is received.
 *
 * When signaled to exit, it cleanly shuts down all background services and
 * threads in the correct order.
 *
 * Main responsibilities:
 * - Initialize and apply NTP-based PPM correction (if enabled).
 * - Start the TCP server and configure its thread priority.
 * - Launch the transmission loop in a background thread.
 * - Wait for shutdown signal via `shutdown_cv`.
 * - Stop all subsystems and perform thread cleanup.
 *
 * @note This function blocks until `exit_wspr_loop` is set by another thread.
 * @todo Consider encapsulating transmission thread in a proper class wrapper.
 * @todo Improve lifecycle/thread management with unified control.
 */
void wspr_loop()
{
    // -------------------------------------------------------------------------
    // Optional: Start NTP-based PPM tracking
    // -------------------------------------------------------------------------
    if (config.use_ntp)
    {
        if (!ppm_init())
        {
            llog.logE(FATAL, "Unable to initialize NTP features.");
            return;
        }
        else
        {
            config.ppm = ppmManager.getCurrentPPM();
            llog.logS(INFO, "Current PPM:", config.ppm);
        }
    }

    // -------------------------------------------------------------------------
    // Start TCP Server
    // -------------------------------------------------------------------------
    if (!server.start(config.server_port, &handler))
    {
        llog.logE(FATAL, "Unable to initialize TCP server.");
        return;
    }
    else
    {
        server.setPriority(SCHED_RR, 10);
        llog.logS(INFO, "TCP server running on port:", config.server_port);
    }

    llog.logS(INFO, "WSPR loop running.");

    // -------------------------------------------------------------------------
    // Start transmission thread
    // -------------------------------------------------------------------------
    // TODO: Encapsulate transmit_thread into a dedicated class
    transmit_thread = std::thread(transmit_loop);
    llog.logS(INFO, "Transmission handler thread started.");

    // -------------------------------------------------------------------------
    // Block until shutdown is triggered
    // -------------------------------------------------------------------------
    std::unique_lock<std::mutex> lock(cv_mtx);
    shutdown_cv.wait(lock, []
    {
        return exit_wspr_loop.load();
    });

    // -------------------------------------------------------------------------
    // Shutdown and cleanup
    // -------------------------------------------------------------------------
    ppmManager.stop();         // Stop PPM manager (if active)
    server.stop();             // Stop TCP server
    iniMonitor.stop();         // Stop config file monitor
    ledControl.stop();         // Stop LED driver
    shutdownMonitor.stop();    // Stop shutdown GPIO monitor

    shutdown_threads();        // Join and cleanup all threads

    llog.logS(DEBUG, "Checking all threads before exiting wspr_loop.");

    // -------------------------------------------------------------------------
    // Final check: confirm all threads have exited
    // -------------------------------------------------------------------------
    if (transmit_thread.joinable()) // TODO: Wrap with class
    {
        llog.logS(WARN, "Transmit thread still running.");
    }

    return;
}

/**
 * @brief Safely shuts down all active worker threads.
 *
 * @details
 * This function ensures a clean shutdown by joining any running threads
 * after acquiring a lock on `shutdown_mtx`. It uses a local `safe_join` lambda
 * to check if a thread is joinable before attempting to join it, avoiding
 * exceptions or undefined behavior.
 *
 * Currently, it handles:
 * - `transmit_thread`: The main signal transmission loop.
 *
 * This pattern ensures that threads are safely joined before the program exits.
 *
 * @note
 * If a thread is already joined or never started, it is skipped safely.
 * Additional threads should be added here as the application grows.
 */
void shutdown_threads()
{
    std::lock_guard<std::mutex> lock(shutdown_mtx);

    llog.logS(INFO, "Shutting down all active threads.");

    // Helper lambda to safely join threads with logging
    auto safe_join = [](std::thread& t, const std::string& name)
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

    safe_join(transmit_thread, "Transmit thread"); // TODO: Create a class

    llog.logS(INFO, "All threads shut down safely.");
}
