/**
 * @file gpio_handler.hpp
 * @brief Manages GPIO pins using the libgpiod library.
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

#ifndef GPIO_HANDLER_HPP
#define GPIO_HANDLER_HPP

// Standard library headers
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <chrono>

// System headers
#include <gpiod.hpp>

/**
 * @class GpioHandler
 * @brief Manages GPIO pins using the libgpiod library.
 *
 * This class provides functionality to configure, monitor, and control GPIO
 * pins on a Linux-based system using the gpiod C++ bindings.
 */
class GpioHandler
{
public:
    /**
     * @enum EdgeType
     * @brief Represents the type of GPIO edge event.
     */
    enum class EdgeType
    {
        RISING, ///< Rising edge event (low to high transition)
        FALLING ///< Falling edge event (high to low transition)
    };

    using Callback = std::function<void(EdgeType, bool)>;

    /**
     * @brief Constructs a GpioHandler instance.
     *
     * @param pin The GPIO pin number to manage.
     * @param is_input True if the pin is an input, false for output.
     * @param pull_up True to enable pull-up resistor, false for pull-down.
     * @param callback Optional callback for edge detection.
     * @param debounce Debounce duration for input events.
     */
    GpioHandler(int pin,
                bool is_input, bool pull_up,
                Callback callback = nullptr,
                std::chrono::milliseconds debounce = std::chrono::milliseconds(50));

    /**
     * @brief Destroys the GpioHandler instance.
     *
     * Cleans up the GPIO pin and stops monitoring threads.
     */
    ~GpioHandler();

    /**
     * @brief Checks if an event was detected on the GPIO pin.
     * @return True if an event was detected, false otherwise.
     */
    bool isEventDetected() const;

    /**
     * @brief Resets the input state and clears the event lock.
     */
    void resetInputState();

    /**
     * @brief Sets the output state of the GPIO pin.
     *
     * @param state True to set the pin HIGH, false for LOW.
     */
    void setOutput(bool state);

    /**
     * @brief Sets a callback for edge detection events.
     *
     * @param callback The callback function to invoke on edge events.
     */
    void setCallback(Callback callback);

    /**
     * @brief Retrieves the current input state of the GPIO pin.
     *
     * @return True if the pin is HIGH, false if LOW.
     */
    bool getInputState() const;

private:
    static constexpr const char *GPIO_CHIP_PATH = "/dev/gpiochip0"; ///< Path to the GPIO chip device.

    int pin_;                              ///< GPIO pin number.
    bool is_input_;                        ///< True if the pin is configured as input.
    bool pull_up_;                         ///< True if pull-up resistor is enabled.
    bool is_output = false;                ///< True if the pin is configured as output.
    std::function<void(EdgeType, bool)> callback_; ///< Callback for edge detection.
    std::chrono::milliseconds debounce_time_ = std::chrono::milliseconds(50); ///< Debounce time for events.
    std::atomic<bool> input_state{false};  ///< Current input state.
    std::atomic<bool> event_detected{false}; ///< Indicates if an event was detected.
    std::atomic<bool> event_lock{false};   ///< Prevents multiple triggers during debounce.
    gpiod::chip chip_;                     ///< GPIO chip handle.
    gpiod::line line_;                     ///< GPIO line handle.
    std::thread monitor_thread_;           ///< Thread for monitoring input events.
    std::mutex gpio_mutex;                 ///< Mutex for GPIO operations.
    std::atomic<bool> running_{true};      ///< Controls the monitoring thread.
    Callback edge_callback;                ///< Callback for edge detection events.
    std::chrono::steady_clock::time_point last_event_time; ///< Last detected event time.
    mutable std::mutex mutex_;             ///< Mutex for state access.
    bool input_state_ = false;             ///< Internal input state.

    /**
     * @brief Configures the GPIO pin with the specified parameters.
     *
     * Sets the pin direction, pull configuration, and event detection.
     */
    void configurePin();

    /**
     * @brief Monitors the GPIO pin for edge events.
     *
     * This method runs in a separate thread while monitoring is active.
     */
    void monitorInput();

    /**
     * @brief Releases the GPIO pin and cleans up resources.
     */
    void cleanupPin();
};

#endif // GPIO_HANDLER_HPP
