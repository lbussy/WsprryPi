/**
 * @file signal_handler.cpp
 * @brief Manages signal handling, and cleanup.
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

#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

// Project headers
#include "gpio_handler.hpp"

// Standard library headers
#include <string>

// Global GPIO instances.
extern std::unique_ptr<GpioHandler> shutdown_pin;
extern std::unique_ptr<GpioHandler> led_pin;
extern std::mutex gpioMutex;

// Default GPIO pins.
extern int shutdown_pin_number;
extern int led_pin_number;

/**
 * @brief Suppresses terminal signals and disables echoing of input.
 *
 * This function modifies the terminal settings to suppress input echo while
 * preserving the ability to receive signals. It saves the original terminal
 * settings for later restoration by `restore_terminal_signals()`.
 *
 * @note This function should be called during application initialization to
 *       prevent terminal input from interfering with program execution.
 */
void suppress_terminal_signals();

/**
 * @brief Restores the terminal to its original signal handling state.
 *
 * This function resets the terminal attributes to the state saved by
 * `suppress_terminal_signals()`. It ensures that standard terminal behavior,
 * such as echoing input and handling signals, is restored after program execution.
 *
 * @note This function should be called during cleanup to avoid leaving the terminal
 *       in an inconsistent state.
 */
void restore_terminal_signals();

/**
 * @brief Blocks specific signals for the current thread.
 *
 * This function blocks common termination and fault signals, preventing the default
 * signal handling behavior. It ensures that signals like SIGINT and SIGSEGV do not
 * interrupt the main thread but can be handled by a dedicated signal-handling thread.
 *
 * @note This function should be called during application initialization to ensure
 *       proper signal management throughout the program's lifecycle.
 */
void block_signals();

/**
 * @brief Converts a signal number to its corresponding string representation.
 *
 * This function maps common signal numbers to their human-readable names, such as
 * SIGINT, SIGTERM, and SIGSEGV. If the signal is not recognized, it returns
 * "UNKNOWN_SIGNAL".
 *
 * @param signum The signal number to convert.
 * @return std::string The corresponding signal name or "UNKNOWN_SIGNAL" if unknown.
 */
std::string signal_to_string(int signum);

/**
 * @brief Performs cleanup of active threads and ensures graceful shutdown.
 *
 * This function stops the scheduler and INI monitor threads, restores terminal
 * settings, and ensures that all resources are released before exiting the
 * application. It is called during signal handling and shutdown procedures.
 */
void cleanup_threads();

/**
 * @brief Handles signals received by the application and triggers cleanup.
 *
 * This function processes signals such as SIGINT, SIGTERM, SIGSEGV, and others.
 * It logs the received signal, performs necessary cleanup, and ensures graceful
 * shutdown. Fatal signals like SIGSEGV trigger an immediate exit, while cleanup
 * signals like SIGINT allow threads to terminate properly.
 *
 * @param signum The signal number received by the application.
 */
void signal_handler(int signum);

/**
 * @brief Registers signal handlers to manage application termination and cleanup.
 *
 * This function sets up signal handling for common termination signals such as
 * SIGINT, SIGTERM, and fatal signals like SIGSEGV. It suppresses terminal signals,
 * blocks signals in the main thread, and spawns a separate thread to wait for
 * and handle signals using `sigwait()`.
 *
 * When a signal is caught, `signal_handler()` is called to perform cleanup and
 * ensure graceful shutdown.
 *
 * @note This function should be called during application initialization to
 *       ensure proper signal management throughout the program's lifecycle.
 */
void register_signal_handlers();

/**
 * @brief Creates or reinitializes the shutdown_pin GPIO input.
 * @param pin The GPIO pin number (default: 19).
 */
extern void enable_shutdown_pin(int pin);

/**
 * @brief Disables the shutdown_pin GPIO.
 */
void disable_shutdown_pin();

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

/**
 * @brief Shuts down the system after a 3-second delay.
 * @details This function checks if the user has root privileges and, if so,
 *          executes a shutdown command with a 3-second delay.
 *
 * @throws std::runtime_error If the user lacks root privileges or the shutdown
 *                            command fails to execute.
 *
 * @note This function requires root access to execute the shutdown command.
 *       Ensure the program runs with elevated privileges (e.g., using `sudo`).
 */
extern void shutdown_system(GpioHandler::EdgeType edge, bool state);

#endif // SIGNAL_HANDLER_HPP
