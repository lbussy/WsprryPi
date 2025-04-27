/**
 * @file main.cpp
 * @brief Entry point for the Wsprry Pi application.
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
#include "main.hpp"

// Project headers
#include "arg_parser.hpp"
#include "scheduling.hpp"
#include "signal_handler.hpp"
#include "version.hpp"
#include "logging.hpp"
#include "singleton.hpp"

// Standard headers
#include <atomic>
#include <string_view>

// System headers
#include <unistd.h>

/**
 * @brief TCP port used for singleton instance checking.
 *
 * This constant defines the port number used for checking if a singleton
 * instance of the application is already running. It allows the program to
 * prevent multiple instances from running concurrently.
 *
 * @note This feature may become redundant with `tcp_server` running.
 * @see tcp_server
 */
constexpr int SINGLETON_PORT = 1234;

// Global unique instance of SignalHandler.
SignalHandler signalHandler;

/**
 * @brief Custom signal handling function.
 *
 * This function is called when a signal is received. It logs the signal and,
 * if critical, terminates immediately. Otherwise, it initiates a graceful shutdown.
 *
 * @param signum The signal number received.
 * @param is_critical Indicates whether the signal is critical.
 */
void callback_signal_handler(int signum, bool is_critical)
{
    std::string_view signal_name = SignalHandler::signalToString(signum);
    if (is_critical)
    {
        std::cerr << "[FATAL] Critical signal received: " << signal_name << ". Performing immediate shutdown." << std::endl;
        std::quick_exit(signum);
    }
    else
    {
        llog.logS(INFO, "Intercepted signal, shutdown will proceed:", signal_name);
        // Let wspr_loop know you're leaving
        exit_wspr_loop.store(true); // Set exit flag
        shutdown_cv.notify_all();   // Wake up any waiting threads
    }
}

/**
 * @brief Entry point for the WsprryPi application.
 *
 * This function initializes the application, parses command-line arguments,
 * loads the INI configuration, verifies NTP synchronization, and starts the
 * main WSPR transmission loop. It also sets system performance modes and
 * handles signal management for graceful shutdown.
 *
 * @param argc The number of command-line arguments.
 * @param argv Array of C-style strings representing the arguments.
 * @return int Exit status: 0 on success, non-zero on failure.
 *
 * @note Ensure that NTP synchronization is stable before proceeding.
 *       If NTP verification fails, the program exits immediately.
 *       The log level is set to INFO by default, but can be changed
 *       via a macro or configuration option.
 */
int main(int argc, char *argv[])
{
    int retval = EXIT_SUCCESS;
    // Sets up logger based on DEBUG flag: INFO or DEBUG
    initialize_logger();

    SingletonProcess singleton(SINGLETON_PORT);

    if (!singleton())
    {
        llog.logE(FATAL, "Another instance is running on port:", SINGLETON_PORT);
        std::exit(EXIT_FAILURE);
    }

    // Parse command line first allowing calls for -h or -v
    try
    {
        retval = parse_command_line(argc, argv);
    }
    catch (const std::exception &e)
    {
        // Handle any exceptions thrown during command-line parsing.
        std::string error_message = "Exception caught processing arguments: " + std::string(e.what());
        print_usage(error_message, EXIT_FAILURE);
    }

    // Make sure we are running as root
    if (getuid() != 0)
    {
        print_usage("This program must be run as root or with sudo.", EXIT_FAILURE);
    }

    // Display version, Raspberry Pi model, and process ID for context.
    llog.logS(INFO, get_version_string());
    llog.logS(INFO, "Running on:", get_pi_model(), ".");
    llog.logS(INFO, "Process PID:", getpid());

    // Validate configuration and ensure all required settings are present.
    if (!validate_config_data())
    {
        print_usage("Configuration validation failed.", EXIT_FAILURE);
    }

    // Display the final configuration after parsing arguments and INI file.
    show_config_values();

    // Register signal handlers for safe shutdown and terminal management.
    block_signals();
    signalHandler.setCallback(callback_signal_handler);
    signalHandler.start();
    signalHandler.setPriority(SCHED_RR, 10);

    // Startup WSPR loop
    try
    {
        if (!wspr_loop())
        {
            retval = EXIT_FAILURE;
        }
    }
    catch (const std::exception &e)
    {
        llog.logE(ERROR, "Unhandled exception in main(): ", e.what());
        retval = EXIT_FAILURE;
    }
    catch (...)
    {
        llog.logE(ERROR, "Unknown fatal error in main().");
        retval = EXIT_FAILURE;
    }

    llog.logS(INFO, get_project_name(), "exiting.");

    // Stop the SignalHandler.
    signalHandler.stop();

    return retval;
}
