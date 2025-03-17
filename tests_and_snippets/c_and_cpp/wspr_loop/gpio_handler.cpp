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

// Project headers
// ...

// Standard library headers
#include <gpiod.hpp>
#include <mutex>
#include <stdexcept>
#include <thread>

// System headers
// ...

std::unique_ptr<GPIOHandler> led_handler;
std::unique_ptr<GPIOHandler> shutdown_handler;
std::thread button_thread;
std::thread led_thread;

// Default GPIO pins.
int shutdown_pin_number = -1;
int led_pin_number = -1;

/**
 * @brief Default constructor for GPIOHandler.
 */
GPIOHandler::GPIOHandler()
    : pin_(-1), is_input_(false), pull_up_(false), debounce_time_(50), callback_(nullptr), chip_(GPIO_CHIP_PATH)
{
}

/**
 * @brief Parameterized constructor for GPIOHandler.
 */
GPIOHandler::GPIOHandler(int pin, bool is_input, bool pull_up, std::chrono::milliseconds debounce, Callback callback)
    : pin_(pin), is_input_(is_input), pull_up_(pull_up), debounce_time_(debounce), chip_(GPIO_CHIP_PATH)
{
    try
    {
        setup(pin, is_input, pull_up, debounce, callback);
        callback_ = std::move(callback); // Move only after successful setup.
    }
    catch (const std::exception &e)
    {

        throw std::runtime_error(std::string("Failed to construct GPIOHandler for pin ") +
                                 std::to_string(pin_) + ": " + e.what());
    }
}

/**
 * @brief Destructor for GPIOHandler.
 */
GPIOHandler::~GPIOHandler()
{
    stopMonitoring(); // Ensure thread exits before cleanup.
    cleanupPin();     // Release GPIO resources.
}

/**
 * @brief Configures the GPIO pin.
 */
void GPIOHandler::setup(int pin, bool is_input, bool pull_up, std::chrono::milliseconds debounce, Callback callback)
{
    pin_ = pin;
    is_input_ = is_input;
    pull_up_ = pull_up;
    debounce_time_ = debounce;

    try
    {
        configurePin();
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Exception in setup() for pin ") +
                                 std::to_string(pin_) + ": " + e.what());
    }
}

/**
 * @brief Configures the GPIO pin for input or output.
 */
void GPIOHandler::configurePin()
{
    std::lock_guard<std::mutex> lock(gpio_mutex_);

    if (pin_ < 0)
    {
        throw std::runtime_error("Invalid GPIO pin.");
    }

    try
    {
        chip_ = gpiod::chip(GPIO_CHIP_PATH);

        line_ = chip_.get_line(pin_);
        if (!line_)
        {
            throw std::runtime_error("Failed to get GPIO line for pin " + std::to_string(pin_));
        }

        gpiod::line_request request;
        request.consumer = "GPIOHandler";

        if (is_input_)
        {
            request.request_type = gpiod::line_request::EVENT_BOTH_EDGES;
            request.flags = pull_up_ ? gpiod::line_request::FLAG_BIAS_PULL_UP : gpiod::line_request::FLAG_BIAS_PULL_DOWN;
        }
        else
        {
            request.request_type = gpiod::line_request::DIRECTION_OUTPUT;
        }

        line_.request(request);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Exception in configurePin() for pin ") +
                                 std::to_string(pin_) + ": " + e.what());
    }
}

/**
 * @brief Starts monitoring the GPIO pin for edge events.
 */
void GPIOHandler::startMonitoring()
{
    if (running_.exchange(true))
    {
        return;
    }
    thread_exited_.store(false, std::memory_order_release);

    // Launch the monitoring thread
    std::thread([this]()
                {
        monitoringLoop();
        thread_exited_.store(true, std::memory_order_release); })
        .detach();
}

void GPIOHandler::monitoringLoop()
{
    while (running_.load(std::memory_order_acquire))
    {
        try
        {
            if (!line_.is_requested())
            {
                throw std::runtime_error(std::string("GPIO line not requested for pin ") +
                                         std::to_string(pin_) + ". Exiting loop.");
            }

            if (line_.event_wait(std::chrono::seconds(1)))
            {
                auto event = line_.event_read();
                EdgeType edge = (event.event_type == gpiod::line_event::RISING_EDGE) ? EdgeType::RISING : EdgeType::FALLING;

                // Invoke the callback if defined
                if (callback_)
                {
                    callback_(edge, line_.get_value());
                }
            }
        }
        catch (const std::system_error &e)
        {
            if (e.code() == std::errc::resource_deadlock_would_occur)
            {
                throw std::runtime_error(std::string("Resource deadlock in monitoring loop for GPIO pin ") +
                                         std::to_string(pin_));
                break;
            }
            throw std::runtime_error(std::string("Exception in GPIO monitoring: ") +
                                     e.what());
            break;
        }
    }
}

/**
 * @brief Stops monitoring the GPIO pin.
 */
void GPIOHandler::stopMonitoring()
{
    if (!running_.exchange(false))
    {
        return;
    }

    // Signal the monitoring thread to exit
    {
        std::unique_lock<std::mutex> lock(exit_mutex_);
        exit_cv_.notify_all();
    }

    // Wait for the thread to exit
    for (int i = 0; i < 20; ++i)
    {
        if (thread_exited_.load(std::memory_order_acquire))
        {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    throw std::runtime_error(std::string("Timeout waiting for thread exit for pin  ") +
                             std::to_string(pin_) + ". Forcing cleanup.");
    thread_exited_.store(true, std::memory_order_release);
}

/**
 * @brief Cleans up the GPIO pin, releasing resources.
 */
void GPIOHandler::cleanupPin()
{
    std::lock_guard<std::mutex> lock(gpio_mutex_);

    if (line_.is_requested())
    {
        line_.release();
    }
}

/**
 * @brief Sets the GPIO output state.
 */
void GPIOHandler::setOutput(bool state)
{
    if (is_input_)
        throw std::runtime_error("Cannot set value on input pin " + std::to_string(pin_));

    line_.set_value(state ? 1 : 0);
}

/**
 * @brief Gets the current input state.
 */
bool GPIOHandler::getInputState() const
{
    if (!is_input_)
        throw std::runtime_error("Cannot read value from output pin " + std::to_string(pin_));

    return line_.get_value();
}

/**
 * @brief Checks if the monitoring thread has exited.
 * @return True if the thread has exited, false otherwise.
 */
bool GPIOHandler::hasThreadExited() const
{
    return thread_exited_.load();
}
