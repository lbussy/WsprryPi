// TODO:  Check Doxygen

/**
 * @file arg_parser.hpp
 * @brief Command-line argument parser and configuration handler.
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

#ifndef ARG_PARSER_HPP
#define ARG_PARSER_HPP

// Project headers
#include "ini_file.hpp"
#include "lcblog.hpp"
#include "monitorfile.hpp"
#include "version.hpp"
#include "wspr_message.hpp"
#include "wspr_band_lookup.hpp"
#include "transmit.hpp"

// Standard library headers
#include <optional>
#include <atomic>
#include <thread>

/**
 * @brief Global configuration instance for argument parsing.
 *
 * This global instance of `ArgParserConfig` holds the parsed
 * command-line arguments and configuration settings which are
 * not in the INI class, which are used throughout the application.
 * It is defined in `arg_parser.cpp` and declared as `extern` in
 * arg_parser.hpp` so it can be accessed globally.
 *
 * @note Ensure that `arg_parser.hpp` is included in any file that
 *       needs access to this configuration instance.
 *
 * @see ArgParserConfig
 */
struct ArgParserConfig
{
    ModeType mode;                       ///< Current operating mode.
    bool useini;                         ///< Unse INI file
    bool date_time_log;                  ///< Use Date/time prefix on output
    bool loop_tx;                        ///< Loop until canceled
    int tx_iterations;                   ///< How many frequency list iterations to loop through
    double test_tone;                    ///< Freqwuency for test tone
    std::vector<double> center_freq_set; ///< Vector of frequencies in Hz
    double f_plld_clk;                   ///< Phase-Locked Loop D clock, default is 500 MHz
    int mem_flag;                        ///< TODO:  Define this - Placeholder for memory management flags.
    /**
     * @brief Default constructor initializing all configuration parameters.
     *
     * Initializes the structure with default values:
     * - `f_plld_clk = 0.0`
     * - `mem_flag = 0`
     */
    ArgParserConfig() : mode(ModeType::WSPR),
                        useini(false),
                        date_time_log(false),
                        loop_tx(false),
                        tx_iterations(0),
                        test_tone(0.0),
                        center_freq_set({}),
                        f_plld_clk(0.0),
                        mem_flag(0)
    {
    }
};

// Extern declaration for global configuration object.
extern ArgParserConfig config;

extern WSPRBandLookup lookup;

/**
 * @brief Global instance of the IniFile configuration handler.
 *
 * The `ini` object provides an interface for reading, writing, and managing
 * INI-style configuration files. It supports key-value pair retrieval, type-safe
 * conversions, and file monitoring for changes.
 *
 * This instance is initialized globally to allow centralized configuration
 * management across all modules of the application.
 *
 * Example usage:
 * @code
 * std::string callsign = ini.get_string_value("Common", "Call Sign");
 * int power = ini.get_int_value("Common", "TX Power");
 * @endcode
 *
 * The `ini` object is commonly used alongside `iniMonitor` to detect and apply
 * configuration changes dynamically without restarting the application.
 *
 * @see https://github.com/lbussy/INI-Handler for detailed documentation and examples.
 */
extern IniFile ini;

/**
 * @brief Global instance of the MonitorFile for INI file change detection.
 *
 * The `iniMonitor` object continuously monitors the specified INI file for changes.
 * It provides real-time notifications when the file is modified, enabling the application
 * to reload configuration settings dynamically without requiring a restart.
 *
 * This instance is typically used alongside the `ini` object to automatically re-validate
 * and apply updated configuration settings.
 *
 * Example usage:
 * @code
 * if (iniMonitor.changed())
 * {
 *     llog.logS(INFO, "INI file changed. Reloading configuration.");
 *     validate_config_data();
 * }
 * @endcode
 *
 * The `iniMonitor` object works by checking the file's last modified timestamp and comparing
 * it with the previous known state. If a change is detected, it returns `true` on `changed()`.
 *
 * @see https://github.com/lbussy/MonitorFile for detailed documentation and examples.
 */
extern MonitorFile iniMonitor;

extern WsprMessage *message;

/**
 * @brief Atomic variable representing the current WSPR transmission interval.
 *
 * This variable defines the transmission interval for WSPR signals.
 * It can be set to one of the predefined constants:
 * - `WSPR_2` for a 2-minute interval.
 * - `WSPR_15` for a 15-minute interval.
 *
 * This value is updated dynamically based on the INI configuration
 * and influences when the scheduler triggers the next transmission.
 *
 * @note Access to this variable is thread-safe due to its atomic nature.
 */
extern std::atomic<int> wspr_interval;

/**
 * @brief Semaphore indicating a pending INI file reload.
 *
 * The `ini_reload_pending` atomic flag acts as a semaphore to signal when an
 * INI file change has been detected and a configuration reload is required.
 * This ensures that the reload process does not conflict with an ongoing
 * transmission.
 *
 * - `true` indicates that an INI reload is pending.
 * - `false` indicates that no reload is currently required.
 *
 * This flag is typically set when the `iniMonitor` detects a file change and
 * is checked periodically by the INI monitoring thread. If a transmission is
 * in progress, the reload is deferred until the transmission completes.
 *
 * @note The atomic nature ensures thread-safe access across multiple threads.
 */
extern std::atomic<bool> ini_reload_pending;

/**
 * @brief Callback for INI file change detection
 *
 * This function runs as a background thread, periodically checking
 * if the monitored INI file has been modified. When a change is detected:
 *
 * - It sets a deferred reload flag (`ini_reload_pending`) to apply the
 *   changes after the transmission completes.
 */
void callback_ini_changed();

/**
 * @brief Monitors the INI configuration file for changes.
 *
 * This is called to check if the monitored INI file has been modified.
 * When a change is detected it reloads the configuration by calling
 * `validate_config_data()`.
 */
extern void apply_deferred_changes();

/**
 * @brief Displays the usage information for the WsprryPi application.
 *
 * This function prints a brief help message to `std::cerr`, outlining
 * the command-line syntax and key options available. Optionally, an error
 * message can be displayed before the usage information.
 *
 * The function also determines whether the program exits or continues
 * running based on the `exit_code` parameter.
 *
 * @note This function **always terminates the program**, unless `exit_code`
 *       is `3`, in which case it simply returns.
 *
 * @param message An optional error message to display before the usage
 *        information. If empty, only the usage message is shown.
 * @param exit_code Determines the program's exit behavior:
 *        - `0` → Exits with `EXIT_SUCCESS`.
 *        - `1` → Exits with `EXIT_FAILURE`.
 *        - `3` → Returns from the function without exiting.
 *        - Any other value → Calls `std::exit(exit_code)`.
 *
 * @example
 * **Returning (does not exit):**
 * @code
 * print_usage();               // Prints usage and returns (default: exit_code = 3).
 * print_usage("Invalid args"); // Prints error message, then usage, returns.
 * print_usage(3);              // Prints usage and returns.
 * @endcode
 *
 * **Exiting the program:**
 * @code
 * print_usage(0);              // Prints usage and exits with EXIT_SUCCESS.
 * print_usage(1);              // Prints usage and exits with EXIT_FAILURE.
 * print_usage("Fatal error", 1); // Prints error message, then exits with EXIT_FAILURE.
 * print_usage("Custom exit", 5); // Prints error message, then exits with code 5.
 * @endcode
 */
void print_usage(const std::string &message = "", int exit_code = 3);

/**
 * @brief Displays usage information and exits or returns based on the exit code.
 *
 * This overload allows calling `print_usage()` with just an exit code.
 * It redirects to the main `print_usage()` function with an empty message.
 *
 * @note This function follows the same exit behavior as the primary function:
 *        - `0` → Exits with `EXIT_SUCCESS`.
 *        - `1` → Exits with `EXIT_FAILURE`.
 *        - `3` → Returns from the function without exiting.
 *        - Any other value → Calls `std::exit(exit_code)`.
 *
 * @param exit_code The exit code to use for termination or return behavior.
 *
 * @example
 * @code
 * print_usage(1); // Outputs usage and exits with EXIT_FAILURE.
 * print_usage(3); // Outputs usage and returns.
 * print_usage(0); // Outputs usage and exits with EXIT_SUCCESS.
 * @endcode
 */
inline void print_usage(int exit_code);

/**
 * @brief Displays the current configuration values from the INI file.
 *
 * If an INI file is loaded, this function logs the current configuration
 * settings such as transmit settings, power levels, and additional options.
 * The output format is structured for readability.
 *
 * @param reload If true, indicates that the configuration has been reloaded.
 */
extern void show_config_values(bool reload = false);

/**
 * @brief Validates and loads configuration data from the INI file.
 *
 * This function extracts configuration values from the INI file, ensuring that
 * required parameters such as callsign, grid square, transmit power, and
 * frequency list are properly set. If any critical parameter is missing or invalid,
 * the function logs the error and exits the program.
 *
 * The function also processes the test tone mode, self-calibration settings,
 * and frequency offset configuration.
 *
 * @return true if the configuration is valid, false otherwise.
 */
extern bool validate_config_data();

/**
 * @brief Parses command-line arguments and configures the program settings.
 *
 * This function processes command-line options using `getopt_long()`, applying
 * values to the program configuration. It first checks for an INI file (`-i`)
 * before processing other options to ensure that command-line arguments can
 * override INI file settings.
 *
 * It validates required parameters and logs any errors, ensuring proper
 * configuration before execution.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 * @return true if parsing is successful, false if an error occurs.
 */
bool parse_command_line(int argc, char *argv[]);

bool load_config(int argc, char *argv[]);

#endif // ARG_PARSER_HPP
