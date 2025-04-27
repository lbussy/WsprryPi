/**
 * @file scheduling.cpp
 * @brief Manages transmit, INI monitoring and scheduling for Wsprry Pi
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
#include "gpio_input.hpp"
#include "gpio_output.hpp"
#include "logging.hpp"
#include "ppm_manager.hpp"
#include "signal_handler.hpp"
#include "web_server.hpp"
#include "web_socket.hpp"
#include "wspr_scheduler.hpp"
#include "wspr_transmit.hpp"

// Standard library headers
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

// System headers
#include <string.h>
#include <sys/reboot.h>    // for reboot()
#include <linux/reboot.h>  // for LINUX_REBOOT_CMD_* constants
#include <sys/resource.h>
#include <unistd.h>


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
 * @brief Flag indicating if a system reboot is in progress.
 *
 * @details
 * This atomic flag is used throughout the application to signal when a
 * full system reboot has been initiated. It is typically set from one
 * of the control points (REST or websockets).
 *
 * Other threads can poll or wait on this flag to terminate safely.
 */
std::atomic<bool> reboot_flag{false};

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
 * @brief Callback function for housekeeping tasks between transmissions.
 *
 * This function checks whether there are pending PPM or INI changes that need
 * to be integrated. If a pending PPM change is detected, it logs the event and
 * resets the flag. Similarly, if a pending INI change is detected, it applies the
 * deferred changes, logs the integration, and resets the flag. If any changes were
 * integrated, a flag is set to indicate that DMA/Symbol reconfiguration is required.
 *
 * @note The DMA/Symbol reconfiguration is not implemented in this function.
 *       A TODO comment indicates where the reset should occur.
 */
void callback_transmission_complete()
{
    // Flag indicating whether DMA/Symbol reconfiguration is required.
    bool do_reconfig_dma = false;

    // Check for a pending PPM change.
    if (ppm_reload_pending.load())
    {
        llog.logS(INFO, "Pending PPM change integrated.");
        ppm_reload_pending.store(false);
        do_reconfig_dma = true;
    }

    // Check for a pending INI change.
    if (ini_reload_pending.load())
    {
        apply_deferred_changes();
        llog.logS(INFO, "Pending INI change integrated.");
        do_reconfig_dma = true;
        // TODO: If transmit is disabled, stop wsprTransmit
    }

    // If either a PPM or INI change was integrated, perform DMA/Symbol reconfiguration.
    if (do_reconfig_dma)
    {
        // TODO: Reset DMA/Symbols
    }
}

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
        llog.logS(ERROR, "Measured PPM exceeds safe threshold.");
        return false;

    case PPMStatus::ERROR_CHRONY_NOT_FOUND:
        llog.logE(WARN, "Chrony not found. Falling back to clock drift measurement.");
        break;

    case PPMStatus::ERROR_UNSYNCHRONIZED_TIME:
        llog.logE(ERROR,
                  "System time is not synchronized. Unable to measure PPM accurately.\n"
                  "Transmission timing or frequencies may be negatively impacted.");
        config.use_ntp = false;
        config_to_json();
        break;

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
 * It logs the event and calls shutdown_system().
 */
void callback_shutdown_system()
{
    llog.logS(INFO, "Shutdown called by GPIO:", config.shutdown_pin);
    shutdown_system();
}

/**
 * @brief Perform a system shutdown sequence.
 *
 * @details
 * This function is intended to be called when a shutdown event is triggered.
 * It performs a visual blink pattern on the LED pin if configured, sets the
 * shutdown flags, and notifies all threads waiting on the shutdown condition
 * variable.
 *
 * Specifically:
 * - Toggles the LED 3 times with 200ms intervals.
 * - Sets `exit_wspr_loop` to break out of the main transmission loop.
 * - Notifies `shutdown_cv` to unblock any waiting threads.
 * - Sets `shutdown_flag` to mark that a full system shutdown is in progress.
 *
 * @note
 * The LED toggling uses `ledControl.toggle_gpio()` and assumes the hardware
 * supports it.
 */
void shutdown_system()
{
    if (config.use_led)
    {
        // Flash LED three times if we are using it
        for (int i = 0; i < 3; ++i)
        {
            ledControl.toggle_gpio(true); // LED ON
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            ledControl.toggle_gpio(false); // LED OFF
            if (i < 2)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
    }

    exit_wspr_loop.store(true); // Signal WSPR loop to exit
    shutdown_cv.notify_all();   // Wake any threads blocked on shutdown
    shutdown_flag.store(true);  // Indicate shutdown sequence active
}

/**
 * @brief Perform a system reboot sequence.
 *
 * @details
 * This function is intended to be called when a reboot event is triggered.
 * It performs a visual blink pattern on the LED pin if configured, sets the
 * reboot flags, and notifies all threads waiting on the reboot condition
 * variable.
 *
 * Specifically:
 * - Toggles the LED 2 times with 100ms intervals.
 * - Sets `exit_wspr_loop` to break out of the main transmission loop.
 * - Notifies `shutdown_cv` to unblock any waiting threads.
 * - Sets `reboot_flag` to mark that a full system reboot is in progress.
 *
 * @note
 * The LED toggling uses `ledControl.toggle_gpio()` and assumes the hardware supports it.
 */
void reboot_system()
{
    if (config.use_led)
    {
        // Flash LED three times if we are using it
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
    }

    exit_wspr_loop.store(true); // Signal WSPR loop to exit
    shutdown_cv.notify_all();   // Wake any threads blocked on shutdown
    reboot_flag.store(true);    // Indicate shutdown sequence active
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
bool wspr_loop()
{
    // -------------------------------------------------------------------------
    // Optional: Start NTP-based PPM tracking
    // -------------------------------------------------------------------------
    if (config.use_ntp)
    {
        if (!ppm_init())
        {
            llog.logE(FATAL, "Unable to initialize NTP features.");
            return false;
        }
        else
        {
            config.ppm = ppmManager.getCurrentPPM();
            llog.logS(INFO, "Current PPM:", config.ppm);
        }
    }

    if (config.use_ini)
    {
        // Start INI monitor
        iniMonitor.filemon(config.ini_filename, callback_ini_changed);
        iniMonitor.setPriority(SCHED_RR, 10);
    }

    // Start web server and set priority
    webServer.start(config.web_port);
    webServer.set_thread_priority(SCHED_RR, 10);
    // Start socket server and set priority
    socketServer.start(config.socket_port, SOCKET_KEEPALIVE);
    socketServer.set_thread_priority(SCHED_RR, 10);

    // Set transmission thread and set priority
    wspr_scheduler.set_enabled(config.transmit);
    wspr_scheduler.set_thread_priority(SCHED_FIFO, 10);
    wspr_scheduler.start(WSPR_Scheduler::WSPR_2, callback_transmission_complete);

    // Wait for something to happen
    llog.logS(INFO, "WSPR loop running.");

    // -------------------------------------------------------------------------
    // Block until shutdown is triggered
    // -------------------------------------------------------------------------
    std::unique_lock<std::mutex> lock(cv_mtx);
    shutdown_cv.wait(lock, []
                     { return exit_wspr_loop.load(); });

    // -------------------------------------------------------------------------
    // Shutdown and cleanup
    // -------------------------------------------------------------------------
    wsprTransmitter.shutdown_transmitter();
    ppmManager.stop();      // Stop PPM manager (if active)
    iniMonitor.stop();      // Stop config file monitor
    ledControl.stop();      // Stop LED driver
    shutdownMonitor.stop(); // Stop shutdown GPIO monitor
    webServer.stop();       // Stop web server
    socketServer.stop();    // Stop the socket server
    wspr_scheduler.stop();   // Stop the transmit server

    llog.logS(DEBUG, "Checking all threads before exiting wspr_loop.");

    if (reboot_flag.load())
    {
        llog.logS(INFO, "Rebooting.");
        reboot_machine();
    }
    if (shutdown_flag.load())
    {
        llog.logS(INFO, "Shutting down.");
        shutdown_machine();
    }

    return true;
}

/**
 * @brief Synchronize disk and reboot the machine.
 *
 * This function calls sync() to flush filesystem buffers, then
 * invokes the reboot(2) syscall directly. The process must have
 * the CAP_SYS_BOOT capability (typically run as root).
 */
void reboot_machine()
{
    // Flush all file system buffers to disk
    sync();

    // Attempt to reboot; LINUX_REBOOT_CMD_RESTART is the same as RB_AUTOBOOT
    if (::reboot(LINUX_REBOOT_CMD_RESTART) < 0)
    {
        llog.logE(ERROR, "Reboot failed:", std::strerror(errno));
    }
}


/**
 * @brief Flush filesystems and power off the machine.
 *
 * Calls sync() to ensure all disk buffers are written, then invokes
 * the reboot(2) syscall with the POWER_OFF command. Requires root or
 * the CAP_SYS_BOOT capability.
 */
void shutdown_machine()
{
    // 1) Flush all pending disk writes
    sync();

    // Power off the system
    // LINUX_REBOOT_CMD_POWER_OFF is equivalent to RB_POWER_OFF
    if (::reboot(LINUX_REBOOT_CMD_POWER_OFF) < 0)
    {
        llog.logE(ERROR, "Shutdown failed:", std::strerror(errno));
    }
}
