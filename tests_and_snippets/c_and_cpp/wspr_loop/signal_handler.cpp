// TODO:  Check Doxygen

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

// Primary header for this source file
#include "signal_handler.hpp"

// Project headers
#include "gpio_handler.hpp"
#include "scheduling.hpp"
#include "arg_parser.hpp"
#include "ppm_ntp.hpp"
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

std::atomic<bool> shutdown_in_progress(false);
static struct termios original_tty;
static bool tty_saved = false;
std::atomic<bool> signal_shutdown{false};

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
void suppress_terminal_signals()
{
    // Save current terminal settings if possible.
    if (tcgetattr(STDIN_FILENO, &original_tty) == 0)
    {
        tty_saved = true;
        struct termios tty = original_tty;

        // Disable input echo while keeping signals enabled.
        tty.c_lflag &= ~ECHO;

        if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) != 0)
        {
            llog.logE(ERROR, "Failed to suppress terminal signals: ", std::strerror(errno));
        }
        else
        {
            llog.logS(DEBUG, "Terminal signals suppressed (input echo disabled).");
        }
    }
    else
    {
        llog.logE(ERROR, "Failed to retrieve terminal settings: ", std::strerror(errno));
    }
}

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
void restore_terminal_signals()
{
    // Restore terminal attributes if they were previously saved.
    if (tty_saved)
    {
        if (tcsetattr(STDIN_FILENO, TCSANOW, &original_tty) != 0)
        {
            llog.logE(ERROR, "Failed to restore terminal settings: ", std::strerror(errno));
        }
        else
        {
            llog.logS(DEBUG, "Terminal signals restored to original state.");
        }
    }
    else
    {
        llog.logS(WARN, "Terminal settings were not saved. Nothing to restore.");
    }
}

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
void block_signals()
{
    sigset_t set;
    sigemptyset(&set); // Initialize an empty signal set.

    // List of signals to block.
    const int signals[] = {
        SIGINT, SIGTERM, SIGQUIT, SIGSEGV, SIGBUS,
        SIGFPE, SIGILL, SIGHUP, SIGABRT};

    // Add each signal to the set.
    for (int signum : signals)
    {
        sigaddset(&set, signum);
    }

    // Block the signals for the current thread.
    if (pthread_sigmask(SIG_BLOCK, &set, nullptr) != 0)
    {
        llog.logE(ERROR, "Failed to block signals: ", std::strerror(errno));
    }
    else
    {
        llog.logS(DEBUG, "Blocked signals in the main thread.");
    }
}

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
std::string signal_to_string(int signum)
{
    // Map of signal numbers to their corresponding string representations.
    static const std::unordered_map<int, std::string> signal_map = {
        {SIGINT, "SIGINT"},
        {SIGTERM, "SIGTERM"},
        {SIGQUIT, "SIGQUIT"},
        {SIGSEGV, "SIGSEGV"},
        {SIGBUS, "SIGBUS"},
        {SIGFPE, "SIGFPE"},
        {SIGILL, "SIGILL"},
        {SIGHUP, "SIGHUP"},
        {SIGABRT, "SIGABRT"}};

    // Find the signal in the map and return its string representation.
    auto it = signal_map.find(signum);
    return (it != signal_map.end()) ? it->second : "UNKNOWN_SIGNAL";
}

/**
 * @brief Performs cleanup of active threads and ensures graceful shutdown.
 *
 * This function stops the scheduler and INI monitor threads, restores terminal
 * settings, and ensures that all resources are released before exiting the
 * application. It is called during signal handling and shutdown procedures.
 */
void cleanup_threads()
{
    llog.logS(DEBUG, "Cleaning up due to signal.");
    signal_shutdown.store(true); // Set the flag for signal-driven shutdown.
    shutdown_threads();          // Use the unified cleanup function.

    if (!signal_shutdown.load())
    {
        llog.logS(INFO, "Wsprry Pi exiting due to signal.");
        std::exit(EXIT_SUCCESS);
    }
}

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
void signal_handler(int signum)
{
    llog.logS(DEBUG, "Signal caught:", signum);
    // Convert signal number to a human-readable string.
    std::string signal_name = signal_to_string(signum);
    std::ostringstream oss;
    oss << "Received " << signal_name << ". Shutting down.";
    std::string log_message = oss.str();

    // Handle the signal based on type.
    switch (signum)
    {
    // Fatal signals that require immediate exit.
    case SIGSEGV:
    case SIGBUS:
    case SIGFPE:
    case SIGILL:
    case SIGABRT:
        llog.logE(FATAL, log_message);
        restore_terminal_signals();
        std::quick_exit(signum); // Immediate exit without cleanup.
        break;

    // Graceful shutdown signals.
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
    case SIGHUP:
        llog.logS(INFO, log_message);
        cleanup_threads(); // Perform full cleanup.
        break;

    // Unknown signals are treated as fatal.
    default:
        llog.logE(FATAL, "Unknown signal caught. Exiting.");
        break;
    }

    // If shutdown is already in progress, ignore additional signals.
    if (shutdown_in_progress.load())
    {
        llog.logE(WARN, "Shutdown already in progress. Ignoring signal:", signal_name);
        return;
    }

    // Set shutdown flag and notify waiting threads.
    shutdown_in_progress.store(true);
    exit_wspr_loop.store(true);
    cv.notify_all();

    // Ensure graceful cleanup.
    cleanup_threads();
}

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
void register_signal_handlers()
{
    // Block signals in the main thread to prevent default handling.
    block_signals();

    // Suppress terminal signals to avoid disruption during execution.
    suppress_terminal_signals();

    // Spawn a detached thread to handle signals asynchronously.
    std::thread([]
                {
        sigset_t set;
        sigemptyset(&set);

        // Signals to handle
        const int signals[] = {SIGINT, SIGTERM, SIGQUIT, SIGSEGV, SIGBUS,
                               SIGFPE, SIGILL, SIGHUP};

        // Add each signal to the set
        for (int signum : signals)
        {
            sigaddset(&set, signum);
        }

        int sig;
        while (!shutdown_in_progress.load())
        {
            // Wait for any of the defined signals
            int ret = sigwait(&set, &sig);
            if (ret == 0)
            {
                signal_handler(sig); // Handle the caught signal
            }
            else
            {
                llog.logE(ERROR, "Failed to wait for signals: ", std::strerror(errno));
            }
        } })
        .detach();

    llog.logS(DEBUG, "Signal handling thread started.");
}

/**
 * @brief Creates or reinitializes the shutdown_pin GPIO input.
 * @param pin The GPIO pin number (default: 19).
 */
void enable_shutdown_pin(int pin)
{
    // If already active, release and reconfigure.
    if (shutdown_handler)
    {
        llog.logS(DEBUG, "Releasing existing shutdown pin (GPIO", shutdown_pin_number, ")");
        shutdown_handler.reset();

        if (button_thread.joinable()) {
            llog.logS(DEBUG, "Closing button monitor threads.");
            button_thread.join();
        }
    }

    shutdown_pin_number = pin;
    shutdown_handler = std::make_unique<GPIOHandler>(shutdown_pin_number, true, true, std::chrono::milliseconds(50), shutdown_system);
    button_thread = std::thread(&GPIOHandler::startMonitoring, shutdown_handler.get());
    llog.logS(INFO, "Shutdown button enabled on GPIO", shutdown_pin_number, ".");
}

/**
 * @brief Disables the shutdown_pin GPIO.
 */
void disable_shutdown_pin()
{
    llog.logS(DEBUG, "Releasing existing shutdown pin (GPIO", shutdown_pin_number, ")");
    if (shutdown_handler)
    {
        shutdown_handler.reset();
        llog.logS(INFO, "Shutdown button disabled.");
    }

    if (button_thread.joinable()) {
        llog.logS(DEBUG, "Closing button monitor threads.");
        button_thread.join();
    }
}

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
void shutdown_system(GPIOHandler::EdgeType edge, bool state)
{
    if (edge == GPIOHandler::EdgeType::FALLING)
    {
        // Check if the user has root privileges.
        if (geteuid() != 0)
        {
            throw std::runtime_error("Root privileges are required to shut down the system.");
            return;
        }

        // Execute the shutdown command with a 5-second delay.
        int result = std::system("sleep 5 && shutdown -h now &");

        // Check if the shutdown command was successful.
        if (result != 0)
        {
            throw std::runtime_error("Failed to execute shutdown command.");
            return;
        }
        else
        {
            llog.logS(INFO, "Shutdown triggered by shutdown button.");
            // Set shutdown flag and notify waiting threads.
            shutdown_in_progress.store(true);
            exit_wspr_loop.store(true);

            // Ensure graceful cleanup.
            cleanup_threads();
        }
    }
}
