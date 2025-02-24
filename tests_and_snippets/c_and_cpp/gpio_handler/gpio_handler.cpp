/**
 * @file gpio_handler.cpp
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

// Primary header for this source file
#include "gpio_handler.hpp"

/**
 * @brief Constructs a GpioHandler object to manage GPIO pin operations.
 *
 * This constructor initializes a GPIO pin as either an input or output,
 * configures the pin with optional pull-up or pull-down resistors, and
 * starts monitoring for edge events if the pin is configured as an input.
 *
 * @param pin The GPIO pin number to manage.
 * @param is_input Set to true to configure the pin as an input; false for output.
 * @param pull_up Set to true to enable a pull-up resistor; false for pull-down.
 * @param logger Reference to an LCBLog instance for logging operations.
 * @param callback Optional callback function to invoke on edge detection.
 * @param debounce Duration to debounce GPIO events, preventing false triggers.
 *
 * @throws std::runtime_error If GPIO initialization or pin configuration fails.
 *
 * This constructor will attempt to configure the specified GPIO pin and,
 * if successful, start a monitoring thread for edge events. If initialization
 * fails, an exception is thrown, and the object is not instantiated.
 */
GpioHandler::GpioHandler(int pin,
                         bool is_input,
                         bool pull_up,
                         std::function<void(EdgeType, bool)> callback,
                         std::chrono::milliseconds debounce)
    : pin_(pin),
      is_input_(is_input),
      pull_up_(pull_up),
      callback_(callback),
      debounce_time_(debounce),
      chip_(GpioHandler::GPIO_CHIP_PATH)
{
    try
    {
        running_ = true;
        configurePin(); // This function will throw if it fails.

        if (is_input_)
        {
            monitor_thread_ = std::thread(&GpioHandler::monitorInput, this);
        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Unable to create instance of GPIO Handler.");
    }
}

/**
 * @brief Destructor for the GpioHandler class.
 * @details
 * Cleans up GPIO resources and ensures graceful shutdown of the monitoring thread.
 * This involves setting the `running_` flag to false, joining the thread if active,
 * and releasing the associated GPIO line to prevent resource leaks.
 *
 * The destructor guarantees that all GPIO operations are safely terminated
 * before the object is destroyed.
 *
 * @note This destructor should not be explicitly called. It is automatically
 *       invoked when a `GpioHandler` object goes out of scope.
 */
GpioHandler::~GpioHandler()
{
    // Signal the monitoring thread to stop.
    running_ = false;

    // Join the monitoring thread if it is still running.
    if (monitor_thread_.joinable())
    {
        monitor_thread_.join();
    }

    // Release the GPIO line and clean up resources.
    cleanupPin();
}

/**
 * @brief Configures the GPIO pin for input or output mode.
 * @details
 * This function initializes the GPIO chip, acquires the specified GPIO line,
 * and configures it according to the object's parameters. If the pin is set as
 * an input, it enables edge detection and applies the appropriate pull-up or
 * pull-down bias. If configured as an output, it sets the pin direction accordingly.
 *
 * This method ensures exclusive access to GPIO resources using a mutex and
 * prevents conflicts by checking if the line is already requested.
 *
 * @throws std::runtime_error
 *         - If the GPIO chip cannot be opened.
 *         - If the GPIO line is already requested.
 *         - If any other error occurs during configuration.
 *
 * @note This function should only be called during initialization. Repeated calls
 *       without proper cleanup may result in resource conflicts.
 */
void GpioHandler::configurePin()
{
    // Ensure exclusive access to GPIO resources.
    std::lock_guard<std::mutex> lock(gpio_mutex);

    try
    {
        // Open the GPIO chip.
        chip_ = gpiod::chip(GPIO_CHIP_PATH);
        if (chip_.num_lines() == 0)
        {
            throw std::runtime_error("Unable to open GPIO chip.");
        }

        // Acquire the GPIO line for the specified pin.
        line_ = chip_.get_line(pin_);
        if (line_.is_requested())
        {
            throw std::runtime_error("GPIO line already in use.");
        }

        // Prepare the line request configuration.
        gpiod::line_request request;
        request.consumer = "GpioHandler";

        if (is_input_)
        {
            // Configure as input with edge detection.
            request.request_type = gpiod::line_request::EVENT_BOTH_EDGES;
            request.flags = pull_up_ ? gpiod::line_request::FLAG_BIAS_PULL_UP
                                     : gpiod::line_request::FLAG_BIAS_PULL_DOWN;
        }
        else
        {
            // Configure as output.
            request.request_type = gpiod::line_request::DIRECTION_OUTPUT;
        }

        // Request the GPIO line with the specified configuration.
        line_.request(request);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to configure GPIO " + std::to_string(pin_) + ": " + e.what());
    }
}

/**
 * @brief Monitors the GPIO pin for edge events.
 * @details
 * This function continuously monitors the GPIO pin for rising or falling edge
 * events. When an edge is detected, it applies a debounce filter to prevent
 * false triggers, logs the event, and invokes the user-provided callback if set.
 *
 * The function runs in a separate thread while the `running_` flag remains true.
 * It safely exits when the flag is cleared during object destruction.
 *
 * @note This function operates as part of a background thread.
 *       Ensure proper cleanup by destroying the object or stopping the thread.
 *
 * @throws std::runtime_error
 *         - If an error occurs while reading GPIO events.
 *         - If the GPIO line becomes invalid during monitoring.
 */
void GpioHandler::monitorInput()
{
    // Continuously monitor the GPIO pin until stopped.
    while (running_)
    {
        try
        {
            // Wait for an edge event with a 1-second timeout.
            if (line_.event_wait(std::chrono::seconds(1)))
            {
                // Read the detected event.
                auto event = line_.event_read();
                auto now = std::chrono::steady_clock::now();

                // Apply debounce filter to avoid false triggers.
                if (now - last_event_time < debounce_time_)
                {
                    continue;
                }
                last_event_time = now;

                // Determine the edge type (RISING or FALLING).
                EdgeType edge_type = (event.event_type == gpiod::line_event::RISING_EDGE)
                                         ? EdgeType::RISING
                                         : EdgeType::FALLING;

                // Invoke the user-defined callback if available.
                if (callback_)
                {
                    callback_(edge_type, line_.get_value());
                }
            }
        }
        catch (const std::exception &e)
        {
            // Throw exception on error and terminate monitoring.
            throw std::runtime_error("Error monitoring GPIO " + std::to_string(pin_) + ": " + e.what());
        }
    }
}

/**
 * @brief Checks if a GPIO edge event has been detected.
 * @details This function returns whether an edge event has been detected
 *          on the monitored GPIO pin.
 *
 * @return true if an edge event has been detected, false otherwise.
 */
bool GpioHandler::isEventDetected() const
{
    return event_detected;
}

/**
 * @brief Resets the input state for the GPIO pin.
 * @details This function clears the event detection and event lock flags,
 *          allowing the GPIO to detect new edge events.
 *
 * @return void
 */
void GpioHandler::resetInputState()
{
    // Reset event detection and lock flags.
    event_detected = false;
    event_lock = false;
}

/**
 * @brief Sets the output state of the GPIO pin.
 * @details
 * This function sets the GPIO pin to the specified state (HIGH or LOW) if the
 * pin is configured as an output. If the pin is configured as an input, an
 * exception is thrown, and the state change is not applied.
 *
 * This method ensures thread safety by locking the GPIO mutex during execution.
 *
 * @param state A boolean value representing the desired output state:
 *              - `true`: Set the GPIO pin to HIGH.
 *              - `false`: Set the GPIO pin to LOW.
 *
 * @throws std::runtime_error
 *         - If the GPIO pin is configured as an input.
 */
void GpioHandler::setOutput(bool state)
{
    // Ensure thread safety by locking the GPIO mutex.
    std::lock_guard<std::mutex> lock(gpio_mutex);

    // Verify that the pin is configured as an output.
    if (!is_input_)
    {
        // Set the GPIO pin to the specified state.
        line_.set_value(state ? 1 : 0);
        input_state = state;
    }
    else
    {
        // Throw an exception if attempting to set an input pin as output.
        throw std::runtime_error("Attempted to set an input pin as output: GPIO " + std::to_string(pin_));
    }
}

/**
 * @brief Sets the callback function for GPIO edge events.
 * @details This function assigns a user-provided callback function to be
 *          triggered when a rising or falling edge is detected on the GPIO pin.
 *
 * @param callback A Callback function to be executed upon an edge event.
 *                 The callback takes two parameters:
 *                 - GpioHandler::EdgeType: Indicates the type of edge (RISING or FALLING).
 *                 - bool: Represents the current state of the GPIO pin (HIGH or LOW).
 *
 * @return void
 *
 * @throws None
 */
void GpioHandler::setCallback(Callback callback)
{
    // Lock the GPIO mutex to ensure thread-safe access.
    std::lock_guard<std::mutex> lock(gpio_mutex);

    // Assign the provided callback to the edge callback member.
    edge_callback = callback;
}

/**
 * @brief Releases the GPIO line associated with the specified pin.
 * @details This function ensures that the GPIO line is properly released
 *          when it is no longer needed. If the line has already been released
 *          or is invalid, a warning is logged.
 *
 * @return void
 *
 * @throws None
 */
void GpioHandler::cleanupPin()
{
    // Lock the GPIO mutex to ensure thread-safe access.
    std::lock_guard<std::mutex> lock(gpio_mutex);

    // Check if the GPIO line is currently requested.
    if (line_.is_requested())
    {
        // Release the GPIO line and log the successful release.
        line_.release();
    }
}

/**
 * @brief Retrieves the current state of the GPIO input.
 * @details This function returns the current logical state of the GPIO pin
 *          if it is configured as an input. Thread safety is ensured by
 *          locking the associated mutex during access.
 *
 * @return bool - True if the input state is HIGH, false if LOW.
 *
 * @throws None
 */
bool GpioHandler::getInputState() const
{
    // Lock the mutex to ensure thread-safe access to the input state.
    std::lock_guard<std::mutex> lock(mutex_);

    // Return the current input state.
    return input_state_;
}
