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
#include <sys/reboot.h>   // for reboot()
#include <linux/reboot.h> // for LINUX_REBOOT_CMD_* constants
#include <sys/resource.h>
#include <unistd.h>

/**
 * @brief Mutex to protect access to the shutdown flag for the WSPR loop.
 *
 * This mutex must be locked before reading or writing \c exitwspr_ready
 * to ensure thread-safe coordination between the signal handler callback
 * and the WSPR loop.
 */
std::mutex exitwspr_mtx;

/**
 * @brief Condition variable used to signal the WSPR loop to exit.
 *
 * The signal handler callback will notify this condition variable after
 * setting \c exitwspr_ready to \c true, causing the waiting WSPR loop
 * to wake up and perform shutdown.
 */
std::condition_variable exitwspr_cv;

/**
 * @brief Flag indicating whether the WSPR loop should terminate.
 *
 * Set to \c true by the signal handler callback under protection of
 * \c exitwspr_mtx, then \c exitwspr_cv is notified so that the WSPR
 * loop can break out of its wait and begin shutdown.
 */
bool exitwspr_ready = false;

/**
 * @brief Round‐robin index into the configured frequency list.
 *
 * Tracks which entry in the `config.center_freq_set` vector will be
 * used for the next WSPR transmission.  Wraps via modulo on each use.
 */
int freq_iterator = 0;

/**
 * @brief Currently active transmit frequency (in Hz).
 *
 * Holds the last frequency value retrieved by `next_frequency()`.
 * A zero value indicates that no frequency is configured or the list was empty.
 */
double current_frequency = 0.0;

/**
 * @brief Timestamp marking the start of the current transmission.
 *
 * Captured at the moment a transmission begins (inside the start callback),
 * and later used to compute the elapsed duration when the transmission
 * completes.
 */
static std::chrono::steady_clock::time_point g_start;

/**
 * @brief File-scope self-pipe descriptors for signal notifications.
 *
 * @details Declared `extern` here so that any TU (like scheduling.cpp)
 *          can refer to the same pipe ends.  The *definition* (no `extern`)
 *          remains in exactly one .cpp (main.cpp).
 */
extern int sig_pipe_fds[2];

/**
 * @brief Global mutex for coordinating shutdown and thread safety.
 *
 * Used to protect shared data during shutdown, ensuring only one thread
 * initiates and executes the shutdown procedure.
 */
std::mutex shutdown_mtx;

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
 * @brief Callback function for housekeeping tasks between transmissions.
 *
 * This function checks whether there are pending PPM or INI changes that need
 * to be integrated. If a pending PPM change is detected, it logs the event and
 * resets the flag. Similarly, if a pending INI change is detected, it applies the
 * deferred changes, logs the integration, and resets the flag. If any changes were
 * integrated, a flag is set to indicate that DMA/Symbol reconfiguration is required.
 */
void callback_transmission_started(const std::string &msg = {})
{
    // Record start time
    g_start = std::chrono::steady_clock::now();
    llog.logS(INFO, "Transmission started.");
    // Notify clients of start
    send_ws_message("transmit", "starting");
}

/**
 * @brief Callback function for housekeeping tasks between transmissions.
 *
 * This function checks whether there are pending PPM or INI changes that need
 * to be integrated. If a pending PPM change is detected, it logs the event and
 * resets the flag. Similarly, if a pending INI change is detected, it applies the
 * deferred changes, logs the integration, and resets the flag. If any changes were
 * integrated, a flag is set to indicate that DMA/Symbol reconfiguration is required.
 */
void callback_transmission_complete(const std::string &msg)
{
    if (!msg.empty())
    {
        // “skipping transmission” or passed-in message
        std::string lower = msg;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });

        if (lower.find("skipping transmission") != std::string::npos)
        {
            llog.logS(INFO, msg);
        }
        else
        {
            // Some other non-skip message
            llog.logS(INFO, msg);
        }
    }
    else
    {
        // No message path ⇒ an actual WSPR window completion
        auto end = std::chrono::steady_clock::now();
        double secs = std::chrono::duration<double>(end - g_start).count();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << secs;
        llog.logS(INFO, "Transmission complete (", oss.str(), " secs).");
        send_ws_message("transmit", "finished");
    }

    if (ini_reload_pending.load())
        apply_deferred_changes();
    if (ppm_reload_pending.load())
        apply_deferred_changes();

    if (!config.use_ini && !config.loop_tx)
    {
        // Atomically decrement and grab the new value:
        int remaining = --config.tx_iterations; // uses atomic<int>::operator--

        if (remaining <= 0)
        {
            llog.logS(DEBUG, "Reached 0 iterations, signalling shutdown.");

            // Tell wspr_loop to break out:
            {
                std::lock_guard<std::mutex> lk(exitwspr_mtx);
                exitwspr_ready = true;
            }
            exitwspr_cv.notify_one();
        }
        else
        {
            llog.logS(INFO, "WSPR transmissions remaining:", remaining);
        }
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
    ppm_reload_pending.store(true, std::memory_order_relaxed);
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
        llog.logE(WARN,
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
 * - Sets `exitwspr_cv` to break out of the main transmission loop.
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

    // Let wspr_loop know to break out
    {
        std::lock_guard<std::mutex> lk(exitwspr_mtx);
        exitwspr_ready = true;
    }
    exitwspr_cv.notify_one();
    // Set shutdown flag
    shutdown_flag.store(true, std::memory_order_relaxed);
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
 * - Sets `exitwspr_cv` to break out of the main transmission loop.
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

    // Let wspr_loop know to break out
    {
        std::lock_guard<std::mutex> lk(exitwspr_mtx);
        exitwspr_ready = true;
    }
    exitwspr_cv.notify_one();
    // Set reboot flag
    reboot_flag.store(true, std::memory_order_relaxed);
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
 * @note This function blocks until `exitwspr_cv` is set by another thread.
 */
bool wspr_loop()
{
    if (config.use_ini)
    {
        // Start INI monitor
        iniMonitor.filemon(config.ini_filename, callback_ini_changed);
        iniMonitor.setPriority(SCHED_RR, 10);
    }

    // Start web server and set priority
    if (config.web_port >= 1024 && config.web_port <= 49151)
    {
        webServer.start(config.web_port);
        webServer.setThreadPriority(SCHED_RR, 10);
    }
    else
    {
        llog.logS(DEBUG, "Skipping web server.");
    }

    // Start socket server and set priority
    if (config.socket_port >= 1024 && config.socket_port <= 49151)
    {
        socketServer.start(config.socket_port, SOCKET_KEEPALIVE);
        socketServer.setThreadPriority(SCHED_RR, 10);
    }
    else
    {
        llog.logS(DEBUG, "Skipping socket server.");
    }

    // Set transmission server and set priority
    wsprTransmitter.setThreadScheduling(SCHED_RR, 40);
    using CB = WsprTransmitter::Callback;
    wsprTransmitter.setTransmissionCallbacks(
        CB{callback_transmission_started},
        CB{callback_transmission_complete});
    set_config(); // Handles get next (or only) frequency, PPM, and setup

    // Wait for something to happen
    llog.logS(INFO, "WSPR loop running.");

    // -------------------------------------------------------------------------
    // Loop (block wspr_loop only) until shutdown is triggered
    // -------------------------------------------------------------------------
    {
        std::unique_lock<std::mutex> lk(exitwspr_mtx);
        exitwspr_cv.wait(lk, []
                         { return exitwspr_ready; });
    }

    llog.logS(DEBUG, "WSPR Loop terminating.");

    // -------------------------------------------------------------------------
    // Shutdown and cleanup
    // -------------------------------------------------------------------------
    wsprTransmitter.shutdownTransmitter();
    ppmManager.stop();      // Stop PPM manager (if active)
    iniMonitor.stop();      // Stop config file monitor
    ledControl.stop();      // Stop LED driver
    shutdownMonitor.stop(); // Stop shutdown GPIO monitor
    webServer.stop();       // Stop web server
    socketServer.stop();    // Stop the socket server

    llog.logS(DEBUG, "Checking all threads before exiting wspr_loop().");

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

/**
 * @brief Broadcasts a JSON-formatted WebSocket message to all connected clients.
 *
 * Builds a JSON object containing a message type, state, and current UTC
 * timestamp (ISO 8601), serializes it, and sends it over the WebSocket server.
 *
 * @param[in] type   The message category (e.g., "transmit", "status").
 * @param[in] state  The message state or payload (e.g., "starting", "finished").
 *
 * @note Requires <nlohmann/json.hpp>, <chrono>, <ctime>, <iomanip>, and <sstream>.
 */
void send_ws_message(std::string type, std::string state)
{
    // Build JSON payload
    nlohmann::json j;
    j["type"] = type;
    j["state"] = state;

    // Capture current UTC time and format as ISO 8601 (YYYY-MM-DDThh:mm:ssZ)
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};
    gmtime_r(&now_t, &tm_utc);

    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    j["timestamp"] = oss.str();

    // Serialize and send to all WebSocket clients
    const std::string message = j.dump();
    socketServer.sendAllClients(message);
}

/**
 * @brief Retrieve the next center frequency, cycling through the configured list.
 *
 * This method returns the next frequency from `config.center_freq_set` in a
 * round-robin fashion.  It uses `freq_iterator` modulo the list size to
 * index into the vector, then increments `freq_iterator` for the subsequent call.
 *
 * @return double
 *   - Next frequency in Hz from the list.
 *   - Returns 0.0 if the list is empty.
 *
 * @note
 *   - `freq_iterator` should be initialized to 0.
 *   - Wrapping is handled via the modulo operation.
 */
double next_frequency()
{
    const auto &freqs = config.center_freq_set;
    if (freqs.empty())
    {
        return 0.0;
    }

    // Compute index in [0, freqs.size())
    const size_t idx = freq_iterator % freqs.size();

    // Fetch and advance iterator
    double freq = freqs[idx];
    ++freq_iterator;

    return freq;
}

/**
 * @brief Apply updated transmission parameters and reinitialize DMA.
 *
 * Retrieves the current PPM value if NTP calibration is enabled, captures
 * the latest configuration settings, and reconfigures the WSPR transmitter
 * with the specified frequency and parameters.
 *
 * @throws std::runtime_error if DMA setup or mailbox operations fail within
 *         `setupTransmission()`.
 */
void set_config()
{
    if (config.use_ntp)
    {
        llog.logS(DEBUG, "Retrieving PPM.");
        config.ppm = ppmManager.getCurrentPPM();
    }

    // Validate configuration and ensure all required settings are present.
    if (!validate_config_data())
    {
        llog.logE(ERROR, "Configuration validation failed.");
        // Let wspr_loop know to break out
        {
            std::lock_guard<std::mutex> lk(exitwspr_mtx);
            exitwspr_ready = true;
        }
        exitwspr_cv.notify_one();
        return;
    }

    // Disable before we (re)set config
    wsprTransmitter.disableTransmission();

    freq_iterator = 0;       // Reset iterator
    current_frequency = 0.0; // Zero out freq

    llog.logS(DEBUG, "Setting DMA.");
    wsprTransmitter.setupTransmission(
        next_frequency(),
        config.power_level,
        config.ppm,
        config.callsign,
        config.grid_square,
        config.power_dbm,
        config.use_offset);

    // Enable/disable transmit if/as needed
    static bool last_transmit = false;
    if (config.transmit != last_transmit)
    {
        if (config.transmit)
            wsprTransmitter.enableTransmission();
        else
            wsprTransmitter.disableTransmission();
        last_transmit = config.transmit;
    }

    // Clear pending config flags
    ini_reload_pending.store(false, std::memory_order_relaxed);
    ppm_reload_pending.store(false, std::memory_order_relaxed);
    llog.logS(INFO, "Setup complete, waiting for next transmission window.");
}
