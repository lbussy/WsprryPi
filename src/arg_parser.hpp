/**
 * @file arg_parser.hpp
 * @brief Command-line argument parser and configuration handler.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is distributed under under
 * the MIT License. See LICENSE.MIT.md for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
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

#include "lcblog.hpp"
#include "ini_file.hpp"
#include "monitorfile.hpp"
#include "version.hpp"

#include <algorithm>
#include <stdexcept>
#include <getopt.h>
#include <string>
#include <iostream>
#include <optional>
#include <unordered_map>

/**
 * @enum ModeType
 * @brief Specifies the mode of operation for the application.
 *
 * This enumeration defines the available modes for operation.
 * - `WSPR`: Represents the WSPR (Weak Signal Propagation Reporter) transmission mode.
 * - `TONE`: Represents a test tone generation mode.
 */
enum class ModeType
{
    WSPR, ///< WSPR transmission mode
    TONE  ///< Test tone generation mode
};

/**
 * @brief Configuration structure for WSPR and test tone transmissions.
 *
 * This structure holds various configuration parameters related to
 * WSPR transmissions and test tone generation. It stores values
 * obtained from the command line and INI file, providing runtime
 * settings for the application.
 */
struct ArgParserConfig
{
    bool useini;                         ///< Flag indicating if an INI file is used for configuration.
    std::string inifile;                 ///< Path to the INI configuration file.
    bool repeat;                         ///< Flag to enable repeated transmission cycles.
    std::vector<double> center_freq_set; ///< List of transmission frequencies.
    bool no_delay;                       ///< Flag to disable WSPR TX window synchronization.
    std::optional<int> terminate;        ///< Number of transmissions before termination (if set).
    bool daemon_mode;                    ///< Flag for enabling daemon (terse) mode.
    double f_plld_clk;                   ///< TODO:  Define this - PLLD clock frequency (unused in current implementation).
    int mem_flag;                        ///< TODO:  Define this - Placeholder for memory management flags.
    float test_tone;                     ///< Frequency for test tone mode.
    ModeType mode;                       ///< Current operating mode (WSPR or test tone).

    /**
     * @brief Default constructor initializing all configuration parameters.
     *
     * Initializes the structure with default values:
     * - `useini = false`
     * - `inifile = ""`
     * - `repeat = false`
     * - `center_freq_set = {}`
     * - `no_delay = false`
     * - `terminate = std::nullopt`
     * - `daemon_mode = false`
     * - `f_plld_clk = 0.0`
     * - `mem_flag = 0`
     * - `test_tone = 0.0f`
     * - `mode = ModeType::WSPR`
     */
    ArgParserConfig() : useini(false),
                        inifile(""),
                        repeat(false),
                        center_freq_set(),
                        no_delay(false),
                        daemon_mode(false),
                        f_plld_clk(0.0),
                        mem_flag(0),
                        test_tone(0.0f),
                        mode(ModeType::WSPR) {}
};

/**
 * @brief Global configuration instance for argument parsing.
 *
 * This global instance of `ArgParserConfig` stores the parsed
 * command-line arguments and configuration settings used throughout
 * the application. It is populated via `parse_command_line()`
 * and accessed by various components to retrieve runtime parameters.
 *
 * @note This variable is defined in `arg_parser.cpp` and declared as
 *       `extern` in `arg_parser.hpp` to ensure a single definition
 *       across the application.
 */
extern ArgParserConfig config;

/**
 * @brief Global logging instance for application-wide logging.
 *
 * This external instance of `LCBLog` is used for logging messages, including
 * informational messages, warnings, and errors. It provides structured logging
 * for debugging and runtime diagnostics.
 */
extern LCBLog llog;

/**
 * @brief Global INI file handler for configuration management.
 *
 * This external instance of `IniFile` is responsible for loading, parsing,
 * and retrieving configuration values from an INI file. It allows the application
 * to store and manage user-defined settings persistently.
 */
extern IniFile ini;

/**
 * @brief Global instance for monitoring file changes.
 *
 * This external instance of `MonitorFile` is responsible for detecting 
 * modifications to the INI configuration file. It allows the application 
 * to respond dynamically to configuration updates without requiring a restart.
 *
 * @note This is particularly useful for live configuration updates.
 */
extern MonitorFile iniMonitor;

/**
 * @brief Converts a string to uppercase.
 *
 * This function takes an input string and returns a new string where all
 * characters have been converted to uppercase. It ensures proper handling
 * of character conversion by using `std::toupper` with an explicit cast to `unsigned char`.
 *
 * @param str The input string to be converted.
 * @return A new string with all characters converted to uppercase.
 *
 * @example
 * @code
 * std::string result = to_uppercase("Hello");
 * // result = "HELLO"
 * @endcode
 */
std::string to_uppercase(const std::string &str);

/**
 * @brief Converts a frequency string to its corresponding numeric value.
 *
 * This function takes a string representation of a frequency, which may be a predefined
 * band name (e.g., "20M"), an explicitly specified numeric frequency (e.g., "14097100.0"),
 * or an invalid input. The function attempts to match the string to a predefined set
 * of frequency mappings. If no match is found, it attempts to parse the input as a
 * numeric value. If parsing fails or the value is non-positive, an error is logged.
 *
 * @param option The input frequency string, which can be a predefined band name
 *               or an explicit numeric frequency.
 * @return An `std::optional<double>` containing the parsed frequency in Hz if valid,
 *         or `std::nullopt` if the input is invalid.
 *
 * @example
 * @code
 * auto freq1 = string_to_frequency("20M");  // Returns 14097100.0
 * auto freq2 = string_to_frequency("14097100.0");  // Returns 14097100.0
 * auto freq3 = string_to_frequency("INVALID");  // Returns std::nullopt
 * @endcode
 */
std::optional<double> string_to_frequency(std::string option);

/**
 * @brief Displays the usage information for the WsprryPi application.
 *
 * This function prints out a brief help message to `std::cerr`, outlining
 * the command-line syntax and key options available. It provides a minimal
 * reference for users and suggests consulting the documentation for more
 * details on all available options.
 *
 * @note This function does not return but simply prints to `std::cerr`.
 *
 * @example
 * @code
 * print_usage(); // Outputs the help message to standard error.
 * @endcode
 */
void print_usage();

/**
 * @brief Displays the current configuration values from the INI file.
 *
 * If an INI file is loaded, this function logs the current configuration
 * settings such as transmit settings, power levels, and additional options.
 * The output format is structured for readability.
 *
 * @param reload If true, indicates that the configuration has been reloaded.
 */
void show_config_values(bool reload = false);

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
bool validate_config_data();

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
bool parse_command_line(const int &argc, char *const argv[]);

#endif // ARG_PARSER_HPP
