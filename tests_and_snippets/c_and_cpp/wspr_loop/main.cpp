/**
 * @file main.cpp
 * @brief
 *
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 *
 * Copyright (C) @threeme3 (unknown dates)
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * WsprryPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

// Primary header for this source file
#include "main.hpp"

// Project headers
#include "arg_parser.hpp"
#include "constants.hpp"
#include "ppm_ntp.hpp"
#include "scheduling.hpp"
#include "signal_handler.hpp"
#include "version.hpp"

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
 *       via a TODO macro or configuration option.
 */
int main(const int argc, char *const argv[])
{
    // Set the default log level to INFO. Consider making this configurable.
    llog.setLogLevel(INFO);  // TODO: Enable DEBUG via Macro or Config

    // Parse command-line arguments and exit if invalid.
    if (!parse_command_line(argc, argv))
    {
        llog.logE(ERROR, "Failed to parse command-line arguments.");
        return EXIT_FAILURE;
    }

    // Display the final configuration after parsing arguments and INI file.
    show_config_values();

    // Validate configuration and ensure all required settings are present.
    if (!validate_config_data())
    {
        llog.logE(ERROR, "Configuration validation failed.");
        return EXIT_FAILURE;
    }

    // Display version, Raspberry Pi model, and process ID for context.
    llog.logS(INFO, version_string());
    llog.logS(INFO, "Running on:", getRaspberryPiModel(), ".");
    llog.logS(INFO, "Process PID:", getpid());

    // Register signal handlers for safe shutdown and terminal management.
    register_signal_handlers();

    // Enable CPU performance mode to ensure accurate transmission timing.
    enable_performance_mode();

    // Verify NTP synchronization before proceeding. Exit if unstable.
    if (!ensure_ntp_stable())
    {
        llog.logE(ERROR, "NTP synchronization failed. Exiting.");
        std::exit(EXIT_FAILURE);
    }

    llog.logS(DEBUG, "NTP verified at startup.");

    // Update PPM (Parts Per Million) calibration for accurate frequency.
    update_ppm();

    // Set the default WSPR interval to 2 minutes.
    wspr_interval = WSPR_2;

    // Start the main WSPR transmission loop.
    wspr_loop();

    // Return success when the application exits gracefully.
    return EXIT_SUCCESS;
}
