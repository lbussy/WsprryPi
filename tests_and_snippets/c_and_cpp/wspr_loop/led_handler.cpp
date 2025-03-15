// TODO:  Check Doxygen

// Primary header for this source file
#include "led_handler.hpp"

// Project headers
#include "gpio_handler.hpp"
#include "scheduling.hpp"
#include "arg_parser.hpp"
#include "logging.hpp"

// Standard library headers
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

// System headers
#include <signal.h>
#include <termios.h>
#include <unistd.h>

/**
 * @brief Creates or reinitializes the led_pin GPIO output.
 * @param pin The GPIO pin number (default: 18).
 */
void enable_led_pin(int pin)
{
    // If already active, release and reconfigure.
    if (led_handler)
    {
        llog.logS(DEBUG, "Releasing existing LED Pin (GPIO", led_pin_number, ")");
        led_handler.reset();
    }

    led_pin_number = pin;
    led_handler = std::make_unique<GPIOHandler>(pin, false, false);
    llog.logS(INFO, "LED enabled on GPIO", led_pin_number, ".");
}

/**
 * @brief Disables the led_pin GPIO.
 */
void disable_led_pin()
{
    if (led_handler)
    {
        llog.logS(DEBUG, "Disabling LED pin on GPIO", led_pin_number, ".");
        led_handler.reset();
        llog.logS(INFO, "LED pin disabled.");
    }
}

/**
 * @brief Sets the LED state based on the given argument.
 * @param state True to turn the LED ON, false to turn it OFF.
 */
void toggle_led(bool state)
{
    if (led_handler)
    {
        led_handler->setOutput(state);
        llog.logS(DEBUG, "LED state set to", (state ? "ON" : "OFF"));
    }
    else
    {
        llog.logE(DEBUG, "LED GPIO is not active. Cannot set state.");
    }
}
