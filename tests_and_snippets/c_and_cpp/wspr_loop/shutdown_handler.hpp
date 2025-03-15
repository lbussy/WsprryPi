// TODO:  Handle doxygen

#ifndef SHUTDOWN_HANDLER_HPP
#define SHUTDOWN_HANDLER_HPP

// Project headers
#include "gpio_handler.hpp"

// Standard library headers
#include <string>
#include <atomic>

extern std::atomic<bool> signal_shutdown;

/**
 * @brief Creates or reinitializes the shutdown_pin GPIO input.
 * @param pin The GPIO pin number (default: 19).
 */
extern void enable_shutdown_pin(int pin);

/**
 * @brief Disables the shutdown_pin GPIO.
 */
void disable_shutdown_pin();

void shutdown_system(GPIOHandler::EdgeType edge, bool state);

#endif // SHUTDOWN_HANDLER_HPP
