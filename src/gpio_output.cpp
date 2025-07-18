/**
 * @file gpio_output.cpp
 * @brief Handles LED output.
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

#include "gpio_output.hpp"
#include <iostream>

// Global instance
GPIOOutput ledControl;

/**
 * @brief Default constructor for GPIOOutput.
 * @details Initializes the GPIOOutput object with the following defaults:
 *          - pin_ is set to -1 (no pin configured)
 *          - active_high_ is set to true (active-high logic)
 *          - enabled_ is false (GPIO not yet configured)
 *          - chip_ and line_ are initialized to nullptr
 *
 * The object must be explicitly configured using enableGPIOPin() before use.
 */
GPIOOutput::GPIOOutput() : pin_(-1),
                           active_high_(true),
                           enabled_(false),
                           chip_(nullptr),
                           line_(nullptr)
{
}

/**
 * @brief Default destructor for GPIOOutput.
 * @details Calls stop() and releases resources.
 */
GPIOOutput::~GPIOOutput()
{
    stop();
}

/**
 * @brief Enables a GPIO pin for output.
 * @details Configures the specified BCM GPIO pin as an output on the default chip
 *          (/dev/gpiochip0) using libgpiod. If the pin is already enabled, it is first
 *          disabled via stop_gpio_pin(). The function requests the GPIO line for output,
 *          sets its initial value (computed based on the active high/low configuration),
 *          and marks the pin as enabled.
 *
 * @param pin The BCM GPIO pin number to be enabled.
 * @param active_high If true, the pin is configured for active-high logic; if false,
 *                    for active-low (sink) logic.
 * @return True if the pin was successfully enabled.
 *
 * @throws std::runtime_error if the GPIO line cannot be obtained or if an error occurs
 *         during configuration.
 */
bool GPIOOutput::enableGPIOPin(int pin, bool active_high)
{
    // If already enabled, disable first.
    if (enabled_)
    {
        stop();
    }
    pin_ = pin;
    active_high_ = active_high;

    try
    {
        chip_ = std::make_unique<gpiod::chip>("/dev/gpiochip0");
        line_ = std::make_unique<gpiod::line>(chip_->get_line(pin_));
        if (!line_)
        {
            throw std::runtime_error("Failed to get GPIO line for pin " + std::to_string(pin_));
        }

        gpiod::line_request req;
        req.consumer = "GPIOOutput";
        req.request_type = gpiod::line_request::DIRECTION_OUTPUT;

        line_->request(req);

        int init_value = compute_physical_state(false);
        line_->set_value(init_value);

        enabled_ = true;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Error enabling GPIO pin " + std::to_string(pin_) +
                                 ": " + e.what());
    }
    return enabled_;
}

/**
 * @brief Disables the currently active GPIO pin.
 * @details Releases the GPIO line if it was previously enabled and resets the
 *          internal chip and line handles. After calling this function, the
 *          pin must be re-enabled via enableGPIOPin() before use.
 *
 * This function is safe to call even if no pin is currently enabled.
 */
void GPIOOutput::stop()
{
    if (enabled_ && line_)
    {
        toggleGPIO(false);
        // Release the line.
        line_->release();
        enabled_ = false;
    }
    // Reset pointers.
    line_.reset();
    chip_.reset();
}

/**
 * @brief Sets the output state of the GPIO pin.
 * @details Converts the requested logical state into the corresponding physical
 *          voltage level based on the configured active-high or active-low mode.
 *          If the pin is not enabled, the function returns false.
 *
 * @param state Logical state to apply to the pin:
 *              - true: active (high if active-high, low if active-low)
 *              - false: inactive (low if active-high, high if active-low)
 *
 * @return True if the state was successfully applied; false if the pin was not
 *         enabled or an error occurred during the write operation.
 */
bool GPIOOutput::toggleGPIO(bool state)
{
    if (!enabled_ || !line_)
    {
        return false;
    }
    try
    {
        int physical_state = compute_physical_state(state);
        line_->set_value(physical_state);
    }
    catch (const std::exception &e)
    {
        return false;
    }
    return true;
}

/**
 * @brief Converts a logical output state to a physical voltage level.
 * @details Applies the active-high or active-low configuration to map the
 *          logical state to the correct hardware signal level:
 *          - For active-high: true → 1 (high), false → 0 (low)
 *          - For active-low:  true → 0 (sink), false → 1 (idle)
 *
 * This method is used internally before writing a value to the GPIO line.
 *
 * @param logical_state Desired logical output state (true = active).
 * @return The corresponding physical value to write to the GPIO line (0 or 1).
 */
int GPIOOutput::compute_physical_state(bool logical_state) const
{
    // For an active-high pin, the physical state equals the logical state.
    // For an active-low (sink) pin, invert the logical state.
    return (active_high_ ? (logical_state ? 1 : 0) : (logical_state ? 0 : 1));
}
