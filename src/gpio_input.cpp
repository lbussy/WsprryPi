/**
 * @file gpio_input.cpp
 * @brief Handles shutdown button sensing.
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

#include "gpio_input.hpp"

#include "logging.hpp"

#include <chrono>
#include <stdexcept>
#include <pthread.h>
#include <iostream>

// Global instance
GPIOInput shutdownMonitor;

/**
 * @brief Default constructor for GPIOInput.
 * @details Initializes internal state and flags to default values:
 *          - gpio_pin_ is set to -1 (unconfigured)
 *          - trigger_high_ is set to false (falling edge by default)
 *          - pull_mode_ is set to PullMode::None
 *          - debounce_triggered_ is cleared
 *          - running_ and stop_thread_ are set to false
 *          - status_ is set to Status::NotConfigured
 *
 * The object remains inactive until enable() is explicitly called.
 */
GPIOInput::GPIOInput() : gpio_pin_(-1),
                             trigger_high_(false),
                             pull_mode_(PullMode::None),
                             debounce_triggered_(false),
                             running_(false),
                             stop_thread_(false),
                             status_(Status::NotConfigured)
{
}

/**
 * @brief Default destructor for GPIOInput.
 * @details calls stop() and terminates threads.
 */
GPIOInput::~GPIOInput()
{
    stop();
}

/**
 * @brief Enable GPIO monitoring.
 *
 * Configures the GPIO pin (using BCM numbering) with the desired trigger condition
 * (true for trigger on rising/high, false for trigger on falling/low), sets the
 * internal pull mode, and registers a callback that is invoked upon detecting an edge.
 *
 * @param pin         BCM GPIO pin number.
 * @param trigger_high True to trigger on a rising edge; false to trigger on a falling edge.
 * @param pull_mode   Desired internal pull mode.
 * @param callback    Callback to be executed when the trigger edge is detected.
 * @return true if the monitor was successfully enabled.
 */
bool GPIOInput::enable(int pin, bool trigger_high, PullMode pull_mode, std::function<void()> callback)
{
    if (running_)
    {
        stop();
    }
    {
        std::lock_guard<std::mutex> lock(monitor_mutex_);
        gpio_pin_ = pin;
        trigger_high_ = trigger_high;
        pull_mode_ = pull_mode;
        callback_ = callback;
        debounce_triggered_ = false;
        stop_thread_ = false;
    }

    try
    {
        // Open the default GPIO chip.
        chip_ = std::make_unique<gpiod::chip>("gpiochip0");
        // Get the line corresponding to the BCM pin.
        line_ = std::make_unique<gpiod::line>(chip_->get_line(gpio_pin_));
        if (!line_)
        {
            throw std::runtime_error("Failed to get GPIO line");
        }

        // Create a line request object to set parameters including bias.
        gpiod::line_request req;
        req.consumer = "GPIOInput";
        // Use an event request type based on the trigger edge desired.
        if (trigger_high_)
        {
            req.request_type = gpiod::line_request::EVENT_RISING_EDGE;
        }
        else
        {
            req.request_type = gpiod::line_request::EVENT_FALLING_EDGE;
        }
        // Set the bias flag if supported.
        req.flags = (pull_mode_ == PullMode::PullUp) ? gpiod::line_request::FLAG_BIAS_PULL_UP : (pull_mode_ == PullMode::PullDown ? gpiod::line_request::FLAG_BIAS_PULL_DOWN : 0);

        // Request the line using the configuration.
        line_->request(req);
    }
    catch (const std::exception &e)
    {
        llog.logE(ERROR, "Error in GPIO initialization:", e.what());
        status_ = Status::Error;
        return false;
    }

    try
    {
        running_ = true;
        status_ = Status::Running;
        // Start the monitoring thread that waits for GPIO events.
        monitor_thread_ = std::thread(&GPIOInput::monitorLoop, this);
    }
    catch (const std::exception &e)
    {
        llog.logS(ERROR, "Error starting monitor thread:", e.what());
        status_ = Status::Error;
        return false;
    }
    return true;
}

/**
 * @brief Stop the monitoring thread.
 *
 * @return true if the monitor was running and is now stopped.
 */
bool GPIOInput::stop()
{
    if (!running_)
    {
        return false;
    }
    stop_thread_ = true;
    cv_.notify_all();
    if (monitor_thread_.joinable())
    {
        monitor_thread_.join();
    }
    running_ = false;
    status_ = Status::Stopped;
    return true;
}

/**
 * @brief Reset the debounce state.
 *
 * Clears the internal debounce flag so that another trigger may be detected.
 */
void GPIOInput::resetTrigger()
{
    debounce_triggered_ = false;
}

/**
 * @brief Sets the scheduling policy and priority of the signal handling thread.
 *
 * @details
 * Uses `pthread_setschedparam()` to adjust the real-time scheduling policy and
 * priority of the signal handling worker thread.
 *
 * This function is useful for raising the importance of the signal handling
 * thread under high system load, especially when using `SCHED_FIFO` or
 * `SCHED_RR`.
 *
 * @param schedPolicy The scheduling policy (e.g., `SCHED_FIFO`, `SCHED_RR`, `SCHED_OTHER`).
 * @param priority The thread priority value to assign (depends on policy).
 *
 * @return `true` if the scheduling parameters were successfully applied,
 *         `false` otherwise (e.g., thread not running or `pthread_setschedparam()` failed).
 *
 * @note
 * The caller may require elevated privileges (e.g., CAP_SYS_NICE) to apply real-time priorities.
 * It is the caller's responsibility to ensure the priority value is valid for the given policy.
 */
bool GPIOInput::setPriority(int schedPolicy, int priority)
{
    // Ensure that the worker thread is active and joinable
    if (!monitor_thread_.joinable())
    {
        return false;
    }

    // Set up the scheduling parameters
    sched_param sch_params;
    sch_params.sched_priority = priority;

    // Attempt to apply the scheduling policy and priority
    int ret = pthread_setschedparam(monitor_thread_.native_handle(), schedPolicy, &sch_params);

    return (ret == 0);
}

/**
 * @brief Retrieve the current status.
 *
 * @return Status enum indicating the current state.
 */
GPIOInput::Status GPIOInput::getStatus() const
{
    return status_;
}

// Main loop that waits for edge events using libgpiod.
void GPIOInput::monitorLoop()
{
    // This loop uses libgpiod's event_wait() and event_read() to block until an edge event occurs.
    while (!stop_thread_)
    {
        try
        {
            // Wait up to 1 second for an event.
            if (line_->event_wait(std::chrono::seconds(1)))
            {
                // Read the event. Since we requested a specific edge,
                // an event here should represent the desired trigger.
                auto event = line_->event_read();
                if (!debounce_triggered_)
                {
                    debounce_triggered_ = true;
                    if (callback_)
                    {
                        callback_();
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            // On error, update status.
            status_ = Status::Error;
            // Optionally you might break out or continue depending on your design.
        }
    }
}
