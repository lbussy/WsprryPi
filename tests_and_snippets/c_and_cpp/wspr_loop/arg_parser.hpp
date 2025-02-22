/**
 * @file arg_parser.hpp
 * @brief Command-line argument parser and configuration handler.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is licensed under the
 * MIT License. See LICENSE.MIT.md for more information.
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

// Project headers
#include "ini_file.hpp"
#include "lcblog.hpp"
#include "monitorfile.hpp"
#include "version.hpp"

// Standard library headers
#include <optional>
#include <atomic>
#include <thread>

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
 * @brief Global configuration instance for argument parsing.
 *
 * This global instance of `ArgParserConfig` holds the parsed
 * command-line arguments and configuration settings used throughout
 * the application. It is defined in `arg_parser.cpp` and declared
 * as `extern` in `arg_parser.hpp` so it can be accessed globally.
 *
 * @note Ensure that `arg_parser.hpp` is included in any file that
 *       needs access to this configuration instance.
 *
 * @see ArgParserConfig
 */
struct ArgParserConfig
{
    bool useini;                                          ///< Flag indicating if an INI file is used for configuration.
    std::string inifile;                                  ///< Path to the INI configuration file.
    bool repeat;                                          ///< Flag to enable repeated transmission cycles.
    std::vector<double> center_freq_set;                  ///< List of transmission frequencies.
    bool no_delay;                                        ///< Flag to disable WSPR TX window synchronization.
    std::optional<int> terminate;                         ///< Number of transmissions before termination (if set).
    bool daemon_mode;                                     ///< Flag for enabling daemon (terse) mode.
    double f_plld_clk;                                    ///< Phase-Locked Loop D clock, default is 500 MHz
    int mem_flag;                                         ///< TODO:  Define this - Placeholder for memory management flags.
    float test_tone;                                      ///< Frequency for test tone mode.
    ModeType mode;                                        ///< Current operating mode (WSPR or test tone).
    double last_ppm;                                      ///< Stores the previous PPM value.
    std::map<std::string, std::string> previous_governor; ///< Stores the original CPU frequency governor value.

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
                        mode(ModeType::WSPR),
                        last_ppm(0.0)
    {
    }
};

// Extern declaration for global configuration object.
extern ArgParserConfig config;

/**
 * @brief Global instance of the LCBLog logging utility.
 *
 * The `llog` object provides thread-safe logging functionality with support for
 * multiple log levels, including DEBUG, INFO, WARN, ERROR, and FATAL.
 * It is used throughout the application to log messages for debugging,
 * monitoring, and error reporting.
 *
 * This instance is initialized globally to allow consistent logging across all
 * modules. Log messages can include timestamps and are output to standard streams
 * or log files depending on the configuration.
 *
 * Example usage:
 * @code
 * llog.logS(INFO, "Application started.");
 * llog.logE(ERROR, "Failed to open configuration file.");
 * @endcode
 *
 * @see https://github.com/lbussy/LCBLog for detailed documentation and examples.
 */
extern LCBLog llog;

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
 * @brief Thread for monitoring INI file changes.
 *
 * The `iniMonitorThread` is responsible for running the INI file monitoring
 * loop. This thread continuously checks for changes in the monitored INI file
 * and triggers appropriate actions when changes are detected.
 *
 * When an INI file change is detected:
 * - If no transmission is active, it immediately reloads the configuration
 *   using `validate_config_data()`.
 * - If a transmission is active, it sets the `ini_reload_pending` flag,
 *   deferring the reload until after the transmission completes.
 *
 * This thread runs independently of the main program loop and ensures
 * configuration changes are processed safely without affecting ongoing operations.
 *
 * @note This thread should be properly joined during shutdown to avoid
 * potential race conditions or dangling threads.
 */
extern std::thread iniMonitorThread;

/**
 * @brief Monitors the INI file for changes and handles configuration reload.
 *
 * This function runs as a background thread, periodically checking for changes
 * in the INI configuration file. When a change is detected:
 * - If the system is not transmitting, it immediately reloads the configuration
 *   by calling `validate_config_data()`.
 * - If a transmission is ongoing, it sets the `ini_reload_pending` flag to defer
 *   the reload until the transmission completes.
 *
 * The thread runs continuously until the `exit_scheduler` flag is set to true,
 * signaling shutdown.
 *
 * @note This function is intended to be executed as a separate thread and does
 *       not return until shutdown is requested.
 *
 * @see validate_config_data(), ini_reload_pending, exit_scheduler
 */
extern void ini_monitor_thread();

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
extern std::string to_uppercase(const std::string &str);

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
extern std::optional<double> string_to_frequency(std::string option);

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
extern void print_usage();

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
extern bool parse_command_line(const int &argc, char *const argv[]);

#endif // ARG_PARSER_HPP
