/**
 * @file arg_parser.cpp
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

// Primary header for this source file
#include "arg_parser.hpp"

// Project headers
#include "constants.hpp"
#include "scheduling.hpp"
#include "signal_handler.hpp"
#include "transmit.hpp"

// Standard library headers
#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <string>

// System headers
#include <getopt.h>

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
ArgParserConfig config;

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
LCBLog llog;

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
IniFile ini;

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
 * The `iniMonitor` object works by checking the file's last modified timestamp and comparing
 * it with the previous known state. If a change is detected, it returns `true` on `changed()`.
 *
 * @see https://github.com/lbussy/MonitorFile for detailed documentation and examples.
 */
MonitorFile iniMonitor;

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
std::atomic<int> wspr_interval(WSPR_2);

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
std::atomic<bool> ini_reload_pending(false);

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
 * Example usage:
 * @code
 * // Start the INI monitoring thread.
 * iniMonitorThread = std::thread(ini_monitor_thread);
 *
 * // Join the thread during shutdown.
 * if (iniMonitorThread.joinable())
 * {
 *     iniMonitorThread.join();
 * }
 * @endcode
 *
 * @note This thread should be properly joined during shutdown to avoid
 * potential race conditions or dangling threads.
 */
std::thread iniMonitorThread;

/**
 * @brief Initializes the logger with the appropriate log level.
 *
 * This function sets the log level based on the current debug state. If the
 * build is compiled with the DEBUG_BUILD macro, the log level is set to DEBUG.
 * Otherwise, it defaults to INFO.
 *
 * @note Ensure that the `get_debug_state()` function correctly reflects the
 *       build configuration for accurate log level assignment.
 *
 * @example
 * initialize_logger();
 * // Sets llog to DEBUG or INFO depending on the build mode.
 */
void initialize_logger()
{
    // Determine the log level based on the build mode.
    const std::string debug_state = get_debug_state();

    // Set the appropriate log level.
    if (debug_state == "DEBUG")
    {
        llog.setLogLevel(DEBUG);  // Enable detailed debug logging.
    }
    else
    {
        llog.setLogLevel(INFO);   // Default to informational logging.
    }
}

/**
 * @brief Monitors the INI configuration file for changes.
 *
 * This function runs as a background thread, periodically checking
 * if the monitored INI file has been modified. When a change is detected:
 *
 * - If the system is not currently transmitting, it immediately reloads the configuration
 *   by calling `validate_config_data()`.
 * - If a transmission is active, it sets a deferred reload flag (`ini_reload_pending`)
 *   to apply the changes after the transmission completes.
 *
 * Additionally, if a reload was deferred and the system is no longer transmitting,
 * the configuration is reloaded immediately.
 *
 * @note This thread continues running until `exit_scheduler` is set to `true`.
 *       It checks for file changes every second.
 */
void ini_monitor_thread()
{
    while (!exit_scheduler.load())
    {
        // 1. Check if the INI file has changed
        if (iniMonitor.changed())
        {
            // Log detection of change
            llog.logS(INFO, "INI file changed.");

            // If not transmitting, reload configuration immediately
            if (!in_transmission.load())
            {
                llog.logS(INFO, "Reloading configuration now.");
                validate_config_data();
            }
            else
            {
                // Otherwise, defer reload until transmission completes
                llog.logS(INFO, "Configuration reload deferred until transmission completes.");
                ini_reload_pending.store(true);
            }
        }

        // 2. Apply deferred reload if transmission has ended
        if (ini_reload_pending.load() && !in_transmission.load())
        {
            // Clear the pending flag and reload configuration
            ini_reload_pending.store(false);
            llog.logS(INFO, "Applying deferred INI changes after transmission.");
            validate_config_data();
        }

        // 3. Sleep briefly before the next check
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Log thread exit
    llog.logS(DEBUG, "INI monitor thread exiting.");
}

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
std::string to_uppercase(const std::string &str)
{
    std::string upper_str = str; // Create a mutable copy
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(),
                   [](unsigned char c)
                   { return std::toupper(c); });
    return upper_str;
}

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
std::optional<double> string_to_frequency(std::string option)
{
    // Predefined frequency mappings in Hz for recognized band names.
    static const std::unordered_map<std::string, double> frequency_map = {
        {"LF", 137500.0}, {"LF-15", 137612.5}, {"MF", 475700.0}, {"MF-15", 475812.5}, {"160M", 1838100.0}, {"160M-15", 1838212.5}, {"80M", 3570100.0}, {"60M", 5288700.0}, {"40M", 7040100.0}, {"30M", 10140200.0}, {"20M", 14097100.0}, {"17M", 18106100.0}, {"15M", 21096100.0}, {"12M", 24926100.0}, {"10M", 28126100.0}, {"6M", 50294500.0}, {"4M", 70092500.0}, {"2M", 144490500.0}};

    // Trim leading and trailing whitespace from the input.
    option.erase(0, option.find_first_not_of(" \t\n\r"));
    option.erase(option.find_last_not_of(" \t\n\r") + 1);

    // Check if the input matches a predefined frequency band directly.
    auto it = frequency_map.find(option);
    if (it != frequency_map.end())
    {
        return it->second;
    }

    // Convert the input to uppercase and check again (for case insensitivity).
    std::string key = to_uppercase(option);
    it = frequency_map.find(key);
    if (it != frequency_map.end())
    {
        return it->second;
    }

    // Attempt to parse the input as a numeric frequency value.
    try
    {
        double parsed_freq = std::stod(option);

        // Ensure the frequency is positive.
        if (parsed_freq <= 0)
        {
            llog.logE(ERROR, "Invalid negative or zero frequency:", option);
            return std::nullopt;
        }
        return parsed_freq;
    }
    catch (const std::exception &)
    {
        // Log an error if parsing fails.
        llog.logE(ERROR, "Could not parse transmit frequency:", option);
        return std::nullopt;
    }
}

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
void print_usage()
{
    std::cerr << "Usage:\n"
              << "  wsprrypi [options] callsign gridsquare transmit_power frequency <f2> <f3> ...\n"
              << "    OR\n"
              << "  wsprrypi [options] --test-tone {frequency}\n\n"
              << "Options:\n"
              << "  -h, --help\n"
              << "    Display this help message.\n"
              << "  -v, --version\n"
              << "    Show the WsprryPi version.\n"
              << "  -i, --ini-file <file>\n"
              << "    Load parameters from an INI file. Provide the path and filename.\n\n"
              << "See the documentation for a complete list of available options.\n\n";
}

/**
 * @brief Displays the current configuration values from the INI file.
 *
 * If an INI file is loaded, this function logs the current configuration
 * settings such as transmit settings, power levels, and additional options.
 * The output format is structured for readability.
 *
 * @param reload If true, indicates that the configuration has been reloaded.
 */
void show_config_values(bool reload)
{
    // Attempt to load INI file if used
    bool loaded = config.useini && ini.load();

    if (!loaded)
        return;

    // Print configuration details if successfully loaded
    llog.logS(INFO, "Config", (reload ? "re-loaded" : "loaded"), "from:", config.inifile);
    // [Control]
    llog.logS(INFO, "Transmit Enabled:", ini.get_bool_value("Control", "Transmit") ? "true" : "false");
    // [Common]
    llog.logS(INFO, "Call Sign:", ini.get_string_value("Common", "Call Sign"));
    llog.logS(INFO, "Grid Square:", ini.get_string_value("Common", "Grid Square"));
    llog.logS(INFO, "Transmit Power:", ini.get_int_value("Common", "TX Power"));
    llog.logS(INFO, "Frequencies:", ini.get_string_value("Common", "Frequency"));
    llog.logS(INFO, "Transmit Pin:", ini.get_int_value("Common", "Transmit Pin"));
    // [Extended]
    llog.logS(INFO, "PPM Offset:", ini.get_double_value("Extended", "PPM"));
    llog.logS(INFO, "Check NTP Each Run:", ini.get_bool_value("Extended", "Use NTP") ? "true" : "false");
    llog.logS(INFO, "Use Frequency Randomization:", ini.get_bool_value("Extended", "Offset") ? "true" : "false");
    llog.logS(INFO, "Power Level:", ini.get_int_value("Extended", "Power Level"));
    llog.logS(INFO, "Use LED:", ini.get_bool_value("Extended", "Use LED") ? "true" : "false");
    llog.logS(INFO, "LED on GPIO", ini.get_int_value("Extended", "LED Pin"));
    // [Server]
    llog.logS(INFO, "Server runs on port:", ini.get_int_value("Server", "Port"));
    llog.logS(INFO, "Use shutdown buton:", ini.get_bool_value("Server", "Use Shutdown") ? "true" : "false");
    llog.logS(INFO, "Shutdown button GPIO", ini.get_int_value("Server", "Shutdown Button"));
}

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
bool validate_config_data()
{
    config.center_freq_set.clear();

    std::istringstream frequency_list(
        [&]() -> std::string
        {
            try
            {
                return ini.get_string_value("Common", "Frequency");
            }
            catch (const std::exception &e)
            {
                return ""; // Default to empty if an error occurs
            }
        }());
    std::string token;

    // Parse frequency(ies) to double
    while (frequency_list >> token)
    {
        std::optional<double> parsed_freq = string_to_frequency(token);
        if (parsed_freq)
        {
            config.center_freq_set.push_back(parsed_freq.value());
        }
        else
        {
            llog.logE(WARN, "Invalid frequency ignored:", token);
        }
    }
    
    // Extract values from INI file, handling errors as needed
    bool offset_enabled = false;
    try
    {
        offset_enabled = ini.get_bool_value("Extended", "Offset");
    }
    catch (const std::exception &)
    {
        offset_enabled = false; // Default: Assume no offset if an error occurs
    }

    bool use_ntp = false;
    try
    {
        use_ntp = ini.get_bool_value("Extended", "Use NTP");
    }
    catch (const std::exception &)
    {
        use_ntp = true; // Default: Assume we use NTP if error occurs
    }

    double ppm_value = [&]() -> double
    {
        try
        {
            return ini.get_double_value("Extended", "PPM");
        }
        catch (const std::exception &)
        {
            return 0.0; // Default to 0.0 on failure
        }
    }();

    // Turn on LED functionality
    //
    // Get PIN number
    int new_led_pin_number = -1;  // Use an invalid default value for easier error detection
    try {
        new_led_pin_number = ini.get_int_value("Extended", "LED Pin");
    } catch (const std::exception& e) {
        llog.logE(ERROR, "Failed to get LED Pin value. Error:", e.what());
        disable_led_pin();
    }
    // Get LED use choice
    bool use_led = false;
    try {
        use_led = ini.get_bool_value("Extended", "Use LED");
    } catch (const std::exception& e) {  // Added exception type
        llog.logE(ERROR, "Failed to get LED use value. Error:", e.what());
        disable_led_pin();
    }
    // Enable LED only if valid and desired
    if (use_led && (new_led_pin_number >= 0 && new_led_pin_number <= 27)) {
        enable_led_pin(new_led_pin_number);
    } else {
        disable_led_pin();
    }

    // Turn on shutdown pin functionality
    //
    // Get PIN number
    int new_shutdown_pin_number = -1;  // Use an invalid default value for easier error detection
    try {
        new_shutdown_pin_number = ini.get_int_value("Server", "Shutdown Button");
    } catch (const std::exception& e) {
        llog.logE(ERROR, "Failed to get shutdown pin value. Error:", e.what());
        disable_shutdown_pin();
    }
    // Get shutdown button use choice
    bool use_shutdown = false;
    try {
        use_shutdown = ini.get_bool_value("Server", "Use Shutdown");
    } catch (const std::exception& e) {  // Added exception type
        llog.logE(ERROR, "Failed to get use shutdown pin value. Error:", e.what());
        disable_shutdown_pin();
    }
    // Enable shutdown button only if valid and desired
    if (use_shutdown && (new_shutdown_pin_number >= 0 && new_shutdown_pin_number <= 27)) {
        enable_shutdown_pin(new_shutdown_pin_number);
    } else {
        disable_shutdown_pin();
    }

    // Handle test tone mode (TONE mode does not require callsign, grid, etc.)
    if (config.mode == ModeType::TONE)
    {
        if (config.test_tone <= 0)
        {
            llog.logE(FATAL, "Test tone frequency must be positive.");
            std::exit(EXIT_FAILURE);
        }

        // Log test tone frequency
        llog.logS(INFO, "A test tone will be generated at",
                  std::fixed, std::setprecision(6), config.test_tone / 1e6, "MHz.");

        if (use_ntp)
        {
            llog.logS(INFO, "NTP will be used to calibrate the tone frequency.");
        }
        else if (ppm_value != 0.0)
        {
            llog.logS(INFO, "PPM value to be used for tone generation:",
                      std::fixed, std::setprecision(2), ppm_value);
        }
    }
    else if (config.mode == ModeType::WSPR)
    {
        // PPM value is ignored when using NTP
        if (ppm_value != 0.0 && use_ntp)
        {
            llog.logE(INFO, "PPM value is ignored when NTP is enabled.");
        }

        // Extract and validate required parameters
        bool missing_call_sign = [&]() -> bool
        {
            try
            {
                return ini.get_string_value("Common", "Call Sign").empty();
            }
            catch (const std::exception &)
            {
                return true; // Assume missing if an error occurs
            }
        }();

        bool missing_grid_square = [&]() -> bool
        {
            try
            {
                return ini.get_string_value("Common", "Grid Square").empty();
            }
            catch (const std::exception &)
            {
                return true; // Assume missing if an error occurs
            }
        }();

        bool invalid_tx_power = [&]() -> bool
        {
            try
            {
                return ini.get_int_value("Common", "TX Power") <= 0;
            }
            catch (const std::exception &)
            {
                return true; // Assume invalid if an error occurs
            }
        }();

        bool no_frequencies = config.center_freq_set.empty();

        // If any required parameter is missing, log error and exit
        if (missing_call_sign || missing_grid_square || invalid_tx_power || no_frequencies)
        {
            llog.logE(FATAL, "Missing required parameters.");

            if (missing_call_sign)
            {
                llog.logE(ERROR, " - Missing callsign.");
            }
            if (missing_grid_square)
            {
                llog.logE(ERROR, " - Missing grid square.");
            }
            if (invalid_tx_power)
            {
                llog.logE(ERROR, " - TX power must be greater than 0 dBm.");
            }
            if (no_frequencies)
            {
                llog.logE(ERROR, " - At least one frequency must be specified.");
            }

            llog.logE(ERROR, "Try: wsprrypi --help");
            std::cerr << std::endl;
            std::exit(EXIT_FAILURE);
        }

        // Log WSPR packet details
        llog.logS(INFO, "WSPR packet payload:");
        llog.logS(INFO, "- Callsign:", ini.get_string_value("Common", "Call Sign"));
        llog.logS(INFO, "- Locator:", ini.get_string_value("Common", "Grid Square"));
        llog.logS(INFO, "- Power:", ini.get_int_value("Common", "TX Power"), " dBm");
        llog.logS(INFO, "Requested TX frequencies:");

        // Concatenate frequency messages for logging
        std::ostringstream log_message;
        for (const auto &freq : config.center_freq_set)
        {
            log_message << "- " << std::setprecision(6) << std::fixed << (freq / 1e6) << " MHz";
        }

        // Log the formatted message
        llog.logS(INFO, log_message.str());

        // Handle calibration and frequency adjustments
        if (use_ntp)
        {
            llog.logS(INFO, "Using NTP to calibrate transmission frequency.");
        }
        else if (ppm_value != 0.0)
        {
            llog.logS(INFO, "PPM value for all transmissions:",
                      std::fixed, std::setprecision(2), ppm_value);
        }

        // Set termination count (defaults to 1 if unset) if ot in daemon mode
        if (!config.daemon_mode)
        {
            config.terminate = config.terminate.value_or(1);
            llog.logS(INFO, "TX will stop after:", config.terminate.value(), "iterations.");

            if (config.repeat)
            {
                llog.logS(INFO, "Transmissions will continue until stopped with CTRL-C.");
            }
        }

        // Handle frequency offset
        if (offset_enabled)
        {
            llog.logS(INFO, "- A small random frequency offset will be added to all transmissions.");
        }
    }
    else
    {
        llog.logE(FATAL, "Mode must be either WSPR or TONE.");
        std::exit(EXIT_FAILURE);
    }
    return true;
}

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
bool parse_command_line(const int &argc, char *const argv[])
{
    static struct option long_options[] = {
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'v'},
        {"ppm", required_argument, nullptr, 'p'},
        {"self-calibration", no_argument, nullptr, 's'},
        {"free-running", no_argument, nullptr, 'f'},
        {"repeat", no_argument, nullptr, 'r'},
        {"terminate", required_argument, nullptr, 'x'},
        {"offset", no_argument, nullptr, 'o'},
        {"test-tone", required_argument, nullptr, 't'},
        {"no-delay", no_argument, nullptr, 'n'},
        {"led", no_argument, nullptr, 'l'},
        {"led_pin", required_argument, nullptr, 'b'},
        {"ini-file", required_argument, nullptr, 'i'},
        {"daemon-mode", no_argument, nullptr, 'D'},
        {"power_level", required_argument, nullptr, 'd'},
        {"port", required_argument, nullptr, 'e'},
        {nullptr, 0, nullptr, 0}};

    // First pass: Look for "-i <file>" before processing other options
    for (int i = 1; i < argc; ++i)
    {
        if ((std::string(argv[i]) == "-i" || std::string(argv[i]) == "--ini-file") && i + 1 < argc)
        {
            config.inifile = argv[i + 1];
            config.useini = true;
            ini.set_filename(config.inifile);   // Load the INI file
            iniMonitor.filemon(config.inifile); // Set the INI file to monitored
            break;                              // We're done pre-scanning, hit the main checks
        }
    }

    while (true)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "hvp:sfrx:o:t:bnli:Dd:e:", long_options, &option_index);

        if (c == -1) // Exit loop when no more options are available
        {
            break;
        }

        char *endp = nullptr;

        switch (c)
        {
        case 'h':
        case '?':
            print_usage();
            return false;
        case 'v':
            std::cout << version_string() << std::endl;
            return false;
        case 'p': // PPM correction
            if (optarg == nullptr)
            {
                llog.logE(ERROR, "Missing argument for PPM correction.");
                return false;
            }
            try
            {
                double ppm = std::stod(optarg);
                ini.set_double_value("Extended", "PPM", ppm);
            }
            catch (const std::exception &)
            {
                llog.logE(ERROR, "Invalid PPM value.");
                return false;
            }
            break;
        case 's': // Enable self-calibration/Use NTP
            ini.set_bool_value("Extended", "Use NTP", true);
            break;
        case 'f': // Disable self-calibration (free-running)
            ini.set_bool_value("Extended", "Use NTP", false);
            break;
        case 'r': // Repeat mode
            config.repeat = true;
            break;
        case 'x': // Terminate after X transmissions
            if (optarg == nullptr)
            {
                llog.logE(ERROR, "Missing argument for termination count.");
                return false;
            }
            try
            {
                config.terminate = std::stoi(optarg);
                if (config.terminate.value() < 1)
                {
                    throw std::invalid_argument("Termination count must be >= 1");
                }
            }
            catch (const std::exception &)
            {
                llog.logE(WARN, "Invalid termination parameter, defaulting to 1.");
                config.terminate = 1; // Set a valid default value
            }
            break;
        case 'o': // Use random frequency offset
            ini.set_bool_value("Extended", "Offset", true);
            break;
        case 't': // Set test tone frequency
            if (optarg == nullptr)
            {
                llog.logE(ERROR, "Missing argument for test tone frequency.");
                return false;
            }

            errno = 0; // Reset errno before calling strtof()
            config.test_tone = strtof(optarg, &endp);
            config.mode = ModeType::TONE;

            // Check for invalid characters in optarg
            if (*endp != '\0')
            {
                llog.logE(ERROR, "Invalid test tone frequency: Non-numeric characters found.");
                return false;
            }

            // Check for conversion errors (out-of-range or invalid)
            if (errno == ERANGE || errno == EINVAL)
            {
                llog.logE(ERROR, "Invalid test tone frequency: Out of range.");
                return false;
            }

            // Reject negative or zero frequencies
            if (config.test_tone <= 0.0f)
            {
                llog.logE(ERROR, "Invalid test tone frequency: Must be greater than zero.");
                return false;
            }
            break;
        case 'i': // Already handled in the first pass
            break;
        case 'n': // No delay, transmit immediately
            config.no_delay = true;
            break;
        case 'l': // Enable LED indicator
            ini.set_bool_value("Extended", "Use LED", true);
            break;
        case 'D': // Daemon mode
            config.daemon_mode = true;
            config.repeat = true; // Daemon mode requires continuous operation
            llog.enableTimestamps(config.daemon_mode);
            break;
        case 'd': // Power Level (0-7)
            if (optarg == nullptr)
            {
                llog.logE(ERROR, "Missing argument for power level.");
                return false;
            }
            try
            {
                int power = std::stoi(optarg);
                power = std::clamp(power, 0, 7);
                ini.set_int_value("Extended", "Power Level", power);
            }
            catch (const std::exception &)
            {
                llog.logE(ERROR, "Invalid power level.");
                return false;
            }
            break;
        case 'e': // Set server port
            if (optarg == nullptr)
            {
                llog.logE(ERROR, "Missing argument for server port.");
                return false;
            }
            try
            {
                int port = std::stoi(optarg);
                if (port < 49152 || port > 65535)
                {
                    llog.logS(WARN, "Invalid port number. Using default: 31415.");
                    port = 31415;
                }
                ini.set_int_value("Server", "Port", port);
            }
            catch (const std::exception &)
            {
                llog.logE(ERROR, "Invalid port number.");
                return false;
            }
            break;
        default:
            llog.logE(ERROR, "Unknown argument: '", static_cast<char>(c), "'");
            return false;
        }
    }

    // Enable transmit if no INI file is used
    ini.set_bool_value("Control", "Transmit", !config.useini);

    return true;
}
