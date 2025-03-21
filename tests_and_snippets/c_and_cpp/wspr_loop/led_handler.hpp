// TODO:  Handle doxygen

#ifndef LED_HANDLER_HPP
#define LED_HANDLER_HPP

// Project headers
#include "gpio_handler.hpp"

// Standard library headers
#include <string>
#include <atomic>

/**
 * @brief Creates or reinitializes the led_pin GPIO output.
 * @param pin The GPIO pin number (default: 18).
 */
void enable_led_pin(int pin);

/**
 * @brief Disables the led_pin GPIO.
 */
void disable_led_pin();

/**
 * @brief Sets the LED state based on the given argument.
 * @param state True to turn the LED ON, false to turn it OFF.
 */
void toggle_led(bool state);

#endif // LED_HANDLER_HPP
