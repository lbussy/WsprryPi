/**
 * @file scheduling.hpp
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
 * @brief Mutex for protecting the shutdown condition variable.
 *
 * @details
 * This mutex guards access to the `shutdown_cv` condition variable and
 * any associated shared state used to coordinate orderly shutdown of
 * the main WSPR loop. Threads must lock `cv_mtx` before waiting on or
 * notifying `shutdown_cv` to ensure thread safety.
 */
extern std::mutex cv_mtx;

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
 * @brief Callback function for housekeeping tasks between transmissions.
 *
 * This function checks whether there are pending PPM or INI changes that need
 * to be integrated. If a pending PPM change is detected, it logs the event and
 * resets the flag. Similarly, if a pending INI change is detected, it applies the
 * deferred changes, logs the integration, and resets the flag. If any changes were
 * integrated, a flag is set to indicate that DMA/Symbol reconfiguration is required.
 */
void callback_transmission_started(const std::string &msg);

/**
 * @brief Callback function for housekeeping tasks between transmissions.
 *
 * This function checks whether there are pending PPM or INI changes that need
 * to be integrated. If a pending PPM change is detected, it logs the event and
 * resets the flag. Similarly, if a pending INI change is detected, it applies the
 * deferred changes, logs the integration, and resets the flag. If any changes were
 * integrated, a flag is set to indicate that DMA/Symbol reconfiguration is required.
 */
void callback_transmission_complete(const std::string &msg);

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
 * - Toggles the LED 3 times with 100ms intervals.
 * - Sets `exit_wspr_loop` to break out of the main transmission loop.
 * - Notifies `shutdown_cv` to unblock any waiting threads.
 * - Sets `shutdown_flag` to mark that a full system shutdown is in progress.
 *
 * @note
 * The LED toggling uses `ledControl.toggle_gpio()` and assumes the hardware
 * supports it.
 */
void shutdown_system();

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
 * The LED toggling uses `ledControl.toggle_gpio()` and assumes the hardware
 * supports it.
 */
void reboot_system();

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
bool ppm_init();

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

/**
 * @brief Synchronize disk and reboot the machine.
 *
 * This function calls sync() to flush filesystem buffers, then
 * invokes the reboot(2) syscall directly. The process must have
 * the CAP_SYS_BOOT capability (typically run as root).
 */
void reboot_machine();

/**
 * @brief Flush filesystems and power off the machine.
 *
 * Calls sync() to ensure all disk buffers are written, then invokes
 * the reboot(2) syscall with the POWER_OFF command. Requires root or
 * the CAP_SYS_BOOT capability.
 */
void shutdown_machine();

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
void send_ws_message(std::string type, std::string state);

/**
 * @brief Retrieve the next center frequency, cycling through the configured list.
 *
 * This method returns the next frequency from `config.center_freq_set` in a
 * round-robin fashion.  It uses `freq_iterator_` modulo the list size to
 * index into the vector, then increments `freq_iterator_` for the subsequent call.
 *
 * @return double
 *   - Next frequency in Hz from the list.
 *   - Returns 0.0 if the list is empty.
 *
 * @note
 *   - `freq_iterator_` should be initialized to 0.
 *   - Wrapping is handled via the modulo operation.
 */
double next_frequency();

/**
 * @brief Apply updated transmission parameters and reinitialize DMA.
 *
 * Retrieves the current PPM value if NTP calibration is enabled, captures
 * the latest configuration settings, and reconfigures the WSPR transmitter
 * with the specified frequency and parameters.
 *
 * @param freq_hz Center frequency for the upcoming transmission, in Hertz.
 *
 * @throws std::runtime_error if DMA setup or mailbox operations fail within
 *         `setupTransmission()`.
 */
void set_config();

#endif // _SCHEDULING_HPP
