/**
 * @file gpio_input.cpp
 * @brief Handles shutdown button sensing.
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

#ifndef SHUTDOWN_HANDLER_HPP
#define SHUTDOWN_HANDLER_HPP

#include <atomic>
#include <condition_variable>
#include <functional>
#include <gpiod.hpp>
#include <memory>
#include <mutex>
#include <thread>

 /**
 * @class GPIOInput
 * @brief Monitors a GPIO pin using libgpiod with thread-based event handling.
 *
 * This class allows for edge-triggered monitoring of a GPIO pin using the
 * libgpiod C++ API. It supports edge detection (rising or falling), optional
 * internal pull-up or pull-down configuration, CPU priority control, debounce
 * management, and thread-safe lifecycle operations.
 *
 * Designed for use on the Raspberry Pi platform with BCM GPIO numbering,
 * the class is thread-based and suitable for global instantiation.
 *
 * Example usage:
 * @code
 * GPIOInput monitor;
 * monitor.enable(19, false, GPIOInput::PullMode::PullUp, []() {
 *     std::cout << "Shutdown button pressed." << std::endl;
 * });
 * @endcode
 *
 * The monitoring thread will invoke the callback only once per edge trigger
 * until resetTrigger() is called. The thread can be stopped, reconfigured,
 * and restarted as needed.
 *
 * Dependencies:
 *  - libgpiod >= 1.6
 *
 * Thread-safe: Yes
 * Reentrant: No
 */
class GPIOInput
{
public:
    /**
     * @brief GPIO internal pull configuration.
     */
    enum class PullMode
    {
        None,    ///< No pull resistor.
        PullUp,  ///< Enable internal pull-up resistor.
        PullDown ///< Enable internal pull-down resistor.
    };

    /**
     * @brief Operational state of the GPIO monitor.
     */
    enum class Status
    {
        NotConfigured, ///< GPIO is not configured yet.
        Running,       ///< Monitoring is currently active.
        Stopped,       ///< Monitoring was active but has been stopped.
        Error          ///< An error occurred during setup or runtime.
    };

    /**
     * @brief Constructs a GPIOInput instance.
     */
    GPIOInput();

    /**
     * @brief Destructor that stops monitoring and cleans up resources.
     */
    ~GPIOInput();

    // Delete copy operations to prevent hardware access duplication.
    GPIOInput(const GPIOInput &) = delete;
    GPIOInput &operator=(const GPIOInput &) = delete;

    /**
     * @brief Enable GPIO monitoring on a specified pin.
     *
     * @details
     * Configures the GPIO line, sets the trigger edge and pull mode, and
     * starts a thread that monitors the pin and invokes the callback when
     * a trigger is detected.
     *
     * @param pin          BCM GPIO pin number.
     * @param trigger_high `true` to trigger on rising edge; `false` for falling.
     * @param pull_mode    Desired pull configuration (up/down/none).
     * @param callback     Function to call on edge detection.
     *
     * @return `true` if successfully configured and monitoring started.
     */
    bool enable(int pin, bool trigger_high, PullMode pull_mode, std::function<void()> callback);

    /**
     * @brief Stop the GPIO monitoring thread.
     *
     * @return `true` if the monitor was previously running and is now stopped.
     */
    bool stop();

    /**
     * @brief Reset internal debounce state to allow re-triggering.
     *
     * @details
     * This clears the debounce flag so that another edge event can be recognized.
     */
    void resetTrigger();

    /**
     * @brief Sets the scheduling policy and priority of the monitor thread.
     *
     * @details
     * Applies a specified real-time policy and priority using `pthread_setschedparam()`.
     * Useful when GPIO response time is critical.
     *
     * @param schedPolicy Scheduling policy (`SCHED_FIFO`, `SCHED_RR`, etc.).
     * @param priority    Thread priority associated with the policy.
     *
     * @return `true` if the scheduling parameters were successfully applied,
     *         `false` otherwise.
     *
     * @note Requires `CAP_SYS_NICE` or root privileges for real-time policies.
     */
    bool setPriority(int schedPolicy, int priority);

    /**
     * @brief Get the current operational status of the GPIO monitor.
     *
     * @return A `Status` enum indicating the monitor's current state.
     */
    Status getStatus() const;

private:
    /**
     * @brief Internal thread loop for monitoring GPIO edge events.
     *
     * @details
     * Uses libgpiod to wait for edge events, handles debounce logic,
     * and invokes the callback function if a trigger condition is met.
     */
    void monitorLoop();

    // ------------------------------------------------------------------------
    // Configuration Parameters
    // ------------------------------------------------------------------------

    int gpio_pin_;                   ///< BCM GPIO pin number.
    bool trigger_high_;              ///< Trigger on rising edge if true, falling if false.
    PullMode pull_mode_;             ///< Pull resistor mode.
    std::function<void()> callback_; ///< User callback function on trigger.

    // ------------------------------------------------------------------------
    // Threading and Synchronization
    // ------------------------------------------------------------------------

    std::atomic<bool> debounce_triggered_; ///< Indicates if a trigger has been handled (debounce).
    std::atomic<bool> running_;            ///< Indicates if monitoring is currently running.
    std::atomic<bool> stop_thread_;        ///< Flag to request thread shutdown.

    std::thread monitor_thread_; ///< Thread used to monitor GPIO changes.
    std::mutex monitor_mutex_;   ///< Protects shared state for monitoring thread.
    std::condition_variable cv_; ///< Used to coordinate thread shutdown.

    // ------------------------------------------------------------------------
    // State and Resource Management
    // ------------------------------------------------------------------------

    Status status_;                     ///< Current operational state of the monitor.
    std::unique_ptr<gpiod::chip> chip_; ///< Represents the GPIO chip.
    std::unique_ptr<gpiod::line> line_; ///< Represents the GPIO line being monitored.
};

// Global instance
extern GPIOInput shutdownMonitor;

#endif // SHUTDOWN_HANDLER_HPP
