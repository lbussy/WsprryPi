/**
 * @file gpio_input.cpp
 * @brief Handles shutdown button sensing.
 *
 * This project is is licensed under the MIT License. See LICENSE.md
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
 * Configures the GPIO pin (BCM numbering) with the desired trigger condition,
 * sets the internal pull mode, registers a callback to run on the first edge,
 * and starts the monitoring thread.
 *
 * @param pin          BCM GPIO pin number.
 * @param trigger_high Trigger on rising if true, falling if false.
 * @param pull_mode    Internal pull resistor mode.
 * @param callback     Function to call once when the edge is detected.
 *
 * @return true if monitoring was started, false on error.
 */
bool GPIOInput::enable(int pin,
                       bool trigger_high,
                       PullMode pull_mode,
                       std::function<void()> callback)
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
        callback_ = std::move(callback);
        debounce_triggered_ = false;
        stop_thread_ = false;
    }

    try
    {
#if GPIOD_API_MAJOR >= 2
        // ---- libgpiod v2 path (Trixie) ----
        chip_ = std::make_unique<gpiod::chip>("/dev/gpiochip0");

        gpiod::line_settings ls;
        ls.set_direction(gpiod::line::direction::INPUT);
        ls.set_edge_detection(trigger_high_
            ? gpiod::line::edge::RISING
            : gpiod::line::edge::FALLING);

        // Kernel debounce in gpiod v2
        ls.set_debounce_period(std::chrono::milliseconds(100));

        switch (pull_mode_)
        {
            case PullMode::PullUp:
                ls.set_bias(gpiod::line::bias::PULL_UP);
                break;
            case PullMode::PullDown:
                ls.set_bias(gpiod::line::bias::PULL_DOWN);
                break;
            default:
                ls.set_bias(gpiod::line::bias::DISABLED);
                break;
        }

        auto builder = chip_->prepare_request().set_consumer("GPIOInput");
        gpiod::line::offset off = static_cast<gpiod::line::offset>(pin_);
        request_ = builder.add_line_settings(off, ls).do_request();

        llog.logS(DEBUG, "GPIOInput: v2 request on /dev/gpiochip0. offset:",
                  gpio_pin_, " edge:", trigger_high_ ? "RISING" : "FALLING",
                  " bias:",
                  (pull_mode_ == PullMode::PullUp)   ? "PULL_UP" :
                  (pull_mode_ == PullMode::PullDown) ? "PULL_DOWN" :
                                                       "DISABLED", ".");
#else
        // ---- libgpiod v1 path (Bookworm and earlier) ----
        chip_ = std::make_unique<gpiod::chip>("/dev/gpiochip0");

        // Store line by value
        line_ = chip_->get_line(gpio_pin_);

        gpiod::line_request req;
        req.consumer = "GPIOInput";
        req.request_type = trigger_high_
            ? gpiod::line_request::EVENT_RISING_EDGE
            : gpiod::line_request::EVENT_FALLING_EDGE;

        req.flags = (pull_mode_ == PullMode::PullUp)
            ? gpiod::line_request::FLAG_BIAS_PULL_UP
            : (pull_mode_ == PullMode::PullDown)
                ? gpiod::line_request::FLAG_BIAS_PULL_DOWN
                : 0;

        line_.request(req);

        llog.logS(DEBUG, "GPIOInput: v1 request on /dev/gpiochip0. offset:",
                  gpio_pin_, " edge:", trigger_high_ ? "RISING" : "FALLING",
                  " bias:",
                  (pull_mode_ == PullMode::PullUp)   ? "PULL_UP" :
                  (pull_mode_ == PullMode::PullDown) ? "PULL_DOWN" :
                                                       "DISABLED", ".");
#endif
    }
    catch (const std::exception& e)
    {
        llog.logE(ERROR, "GPIOInput: init error.", e.what());
        status_ = Status::Error;
        running_ = false;
        return false;
    }

    try
    {
        running_ = true;
        status_ = Status::Running;
        // Start the monitoring thread that waits for GPIO events
        monitor_thread_ = std::thread(&GPIOInput::monitorLoop, this);
    }
    catch (const std::exception& e)
    {
        llog.logS(ERROR, "GPIOInput: thread start error.", e.what());
        status_ = Status::Error;
        running_ = false;
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
 * thread under high system load, especially when using `SCHED_RR` or
 * `SCHED_FIFO`.
 *
 * @param schedPolicy The scheduling policy (e.g., `SCHED_FIFO`, `SCHED_RR`,
 *                    `SCHED_OTHER`).
 * @param priority The thread priority value to assign (depends on policy).
 *
 * @return `true` if the scheduling parameters were successfully applied,
 *         `false` otherwise (e.g., thread not running or
 *         `pthread_setschedparam()` failed).
 *
 * @note
 * The caller may require elevated privileges (e.g., CAP_SYS_NICE) to apply
 * real-time priorities.  It is the caller's responsibility to ensure the
 * priority value is valid for the given policy.
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

/**
 * @brief Monitor loop for GPIO edge events.
 *
 * Continuously waits for edge events and invokes the registered callback
 * once per detected edge until the debounce flag is reset. The behavior
 * adapts to the libgpiod API in use:
 *
 * - v2 (Trixie/libgpiod3): waits via
 *   request_.wait_edge_events() and reads events into an internal
 *   edge_event_buffer.
 * - v1 (Bookworm/libgpiod2): waits via line_->event_wait() and reads a
 *   single event with line_->event_read().
 *
 * On any caught exception the internal status is set to Status::Error and
 * a log entry is emitted. The loop continues unless stop has been
 * requested.
 *
 * @note Debounce is enforced by a simple one-shot flag
 *       (debounce_triggered_) and does not reset automatically.
 *       Call resetTrigger() to allow the next callback invocation.
 *
 * @return Nothing.
 */
void GPIOInput::monitorLoop()
{
    while (!stop_thread_)
    {
        try
        {
#if GPIOD_API_MAJOR >= 2
            // v2: Wait and read events from the active request
            if (request_.wait_edge_events(std::chrono::seconds(1)))
            {
                auto count = request_.read_edge_events(event_buf_);
                if (count > 0 && !debounce_triggered_)
                {
                    debounce_triggered_ = true;
                    if (callback_)
                    {
                        callback_();
                    }
                }
            }
#else
            // v1: Wait/read from the line handle
            if (line_->event_wait(std::chrono::seconds(1)))
            {
                (void)line_->event_read();  // We only care that an edge happened
                if (!debounce_triggered_)
                {
                    debounce_triggered_ = true;
                    if (callback_)
                    {
                        callback_();
                    }
                }
            }
#endif
        }
        catch (const std::exception& e)
        {
            status_ = Status::Error;
            llog.logE(ERROR, "GPIO loop error:", e.what());
        }
    }
}
