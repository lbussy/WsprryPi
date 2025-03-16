// TODO:  Check Doxygen

/**
 * @file main.cpp
 * @brief Entry point for the Wsprry Pi application.
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
#include "main.hpp"

// Project headers
#include "arg_parser.hpp"
#include "scheduling.hpp"
#include "signal_handler.hpp"
#include "version.hpp"
#include "logging.hpp"
#include "singleton.hpp"

// System headers
#include <unistd.h>

constexpr const int SINGLETON_PORT = 1234;

void main_shutdown()
{
    // Let wspr_loop know you're leaving
    exit_wspr_loop.store(true); // Set exit flag
    shutdown_cv.notify_all();   // Wake up any waiting threads
    // Shutdown signal handler
    if (handler) // Ensure handler is valid
    {
        SignalHandlerStatus status = handler->request_shutdown();

        if (status == SignalHandlerStatus::ALREADY_STOPPED)
        {
            llog.logS(DEBUG, "Shutdown already in progress. Ignoring duplicate request.");
        }
        llog.logS(DEBUG, "Shutdown requested.");
    }
    else
    {
        llog.logE(ERROR, "Handler is null. Cannot request shutdown.");
    }
}

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
    std::string_view signal_name = SignalHandler::signal_to_string(signum);
    if (is_critical)
    {
        std::cerr << "[FATAL] Critical signal received: " << signal_name << ". Performing immediate shutdown." << std::endl;
        std::quick_exit(signum);
    }
    else
    {
        llog.logS(INFO, "Intercepted signal, shutdown will proceed:", signal_name);
        main_shutdown();
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
    // Sets up logger based on DEBUG flag: INFO or DEBUG
    initialize_logger();

    // Make sure we are running as root
    if (getuid() != 0)
    {
        print_usage("This program must be run as root or with sudo.", EXIT_FAILURE);
    }

    if (!load_config(argc, argv)) // Calls: ->parse_command_line() -> validate_config_data()
        print_usage("An unknown error occured loading the configuration.", EXIT_FAILURE);

    // Display version, Raspberry Pi model, and process ID for context.
    llog.logS(INFO, version_string());
    llog.logS(INFO, "Running on:", getRaspberryPiModel(), ".");
    llog.logS(INFO, "Process PID:", getpid());

    SingletonProcess singleton(SINGLETON_PORT);

    if (!singleton())
    {
        llog.logE(FATAL, "Another instance is running on port:", SINGLETON_PORT);
        std::exit(EXIT_FAILURE);
    }

    // Display the final configuration after parsing arguments and INI file.
    show_config_values();

    // Register signal handlers for safe shutdown and terminal management.
    handler = std::make_unique<SignalHandler>();
    handler->block_signals();
    handler->set_callback(callback_signal_handler);

    // Startup WSPR loop
    try
    {
        wspr_loop();
    }
    catch (const std::exception &e)
    {
        llog.logE(ERROR, "Unhandled exception in main(): ", e.what());
    }
    catch (...)
    {
        llog.logE(ERROR, "Unknown fatal error in main().");
    }

    llog.logS(INFO, project_name(), "exiting normally.");
    main_shutdown();

    // Cleanup signal handler and pointer
    handler->wait_for_shutdown();
    handler->stop();
    handler.reset();

    return EXIT_SUCCESS;
}
