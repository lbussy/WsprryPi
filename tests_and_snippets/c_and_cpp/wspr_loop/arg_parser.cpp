/**
 * @file arg_parser.cpp
 * @brief Command-line argument parser and configuration handler.
 *
 * This file is part of WsprryPi, a project originally created from @threeme3
 * WsprryPi projet (no longer on GitHub). However, now the original code
 * remains only as a memory and inspiration, and this project is no longer
 * a derivative work.
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
#include "config_handler.hpp"
#include "constants.hpp"
#include "gpio_input.hpp"
#include "gpio_output.hpp"
#include "logging.hpp"
#include "scheduling.hpp"
#include "signal_handler.hpp"
#include "wspr_band_lookup.hpp"
#include "wspr_message.hpp"
#include "wspr_transmit.hpp"

// Standard library headers
#include <algorithm>
#include <atomic>
#include <cstring>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// System headers
#include <getopt.h>

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
 * @brief Pointer to a WSPR message.
 *
 * This pointer is used to reference a WsprMessage object, which constructs a WSPR message
 * from a callsign, grid location, and power level. It is initialized to nullptr until
 * a valid WsprMessage instance is created.
 *
 * @note Remember to allocate memory for this pointer before use.
 */
WsprMessage *message = nullptr;

/**
 * @brief Instance of WSPRBandLookup.
 *
 * This instance of WSPRBandLookup is used to translate frequency representations:
 * - Converts from a short-hand (Hx) to a higher order (e.g., MHz) and vice versa.
 * - Validates frequency values.
 * - Translates terms (e.g., "20m") into a valid WSPR frequency.
 *
 * Use this instance for any operations requiring frequency conversions and validation
 * within the WSPR system.
 */
WSPRBandLookup lookup;

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
 * @brief List of allowed WSPR power levels.
 *
 * This constant vector stores the permitted power levels for WSPR transmissions.
 * Each element in the vector represents a discrete power setting (in dBm) that can be used
 * for a WSPR message. The defined levels ensure that only valid and recognized power levels
 * are applied during transmissions.
 *
 * The allowed power levels are: 0, 3, 7, 10, 13, 17, 20, 23, 27, 30, 33, 37, 40, 43, 47, 50, 53, 57, 60.
 */
const std::vector<int> wspr_power_levels = {0, 3, 7, 10, 13, 17, 20, 23, 27, 30, 33, 37, 40, 43, 47, 50, 53, 57, 60};

/**
 * @brief Callback for INI file change detection
 *
 * This function runs as a background thread, periodically checking
 * if the monitored INI file has been modified. When a change is detected:
 *
 * - It sets a deferred reload flag (`ini_reload_pending`) to apply the
 *   changes after the transmission completes.
 */
void callback_ini_changed()
{
    if (wspr_transmit.isTransmitting())
    {
        // Log detection of change
        ini_reload_pending.store(true);
        llog.logS(INFO, "INI file changed, reload after transmission.");
    }
    else
    {
        llog.logS(INFO, "INI file changed, reloading.");
        load_from_ini();
        validate_config_data();
        // TODO: Reset DMA/Symbols
    }
}

/**
 * @brief Monitors the INI configuration file for changes.
 *
 * This is called to check if the monitored INI file has been modified.
 * When a change is detected it reloads the configuration by calling
 * `validate_config_data()`.
 */
void apply_deferred_changes()
{
    // Apply deferred reload if transmission has ended
    if (ini_reload_pending.load() && !wspr_transmit.isTransmitting())
    {
        // Clear the pending flag and reload configuration
        ini_reload_pending.store(false);
        llog.logS(INFO, "Applying deferred INI changes.");
        load_from_ini();
        validate_config_data();
    }
}

/**
 * @brief Validates a WSPR callsign and normalizes it to uppercase.
 *
 * This function checks whether a given callsign meets the criteria for a valid WSPR Type 1
 * callsign. Validation includes:
 * - Ensuring the callsign length is between 3 and 6 characters.
 * - Matching the callsign against a regular expression pattern that enforces the WSPR Type 1 format.
 *
 * The regex pattern used is:
 * @verbatim
 * ^(?:[A-Za-z0-9]?[A-Za-z0-9][0-9][A-Za-z]?[A-Za-z]?[A-Za-z]?|[A-Za-z][0-9][A-Za-z]|[A-Za-z0-9]{3}[0-9][A-Za-z]{2})$
 * @endverbatim
 * This pattern is evaluated in a case-insensitive manner.
 *
 * If the callsign is valid, the function converts it to uppercase.
 *
 * @param callsign A reference to the callsign string to validate. If valid, the string is modified in place to uppercase.
 * @return true if the callsign is valid, false otherwise.
 */
bool is_valid_callsign(std::string &callsign)
{
    // WSPR Type 1 callsign regex pattern (case-insensitive)
    static const std::regex callsign_pattern(
        R"(^(?:[A-Za-z0-9]?[A-Za-z0-9][0-9][A-Za-z]?[A-Za-z]?[A-Za-z]?|[A-Za-z][0-9][A-Za-z]|[A-Za-z0-9]{3}[0-9][A-Za-z]{2})$)",
        std::regex::icase);

    // Check that the callsign length is within the valid range (3-6 characters)
    if (callsign.length() < 3 || callsign.length() > 6)
    {
        return false;
    }

    // Validate the callsign using the regex pattern
    if (std::regex_match(callsign, callsign_pattern))
    {
        // Convert the valid callsign to uppercase
        std::transform(callsign.begin(), callsign.end(), callsign.begin(), ::toupper);
        return true;
    }

    return false;
}

/**
 * @brief Rounds an input power level to the nearest valid WSPR power level.
 *
 * This function compares the given power level to the list of allowed WSPR power levels
 * and returns the level with the smallest absolute difference. It ensures that the
 * transmitted power level is one of the permitted values.
 *
 * @param power The input power level to round.
 * @return The nearest valid WSPR power level.
 */
int round_to_nearest_wspr_power(int power)
{
    // Start by assuming the first allowed power level is the closest.
    int closest = wspr_power_levels[0];
    int min_diff = std::abs(power - closest);

    // Iterate through the list of allowed power levels.
    for (int level : wspr_power_levels)
    {
        int diff = std::abs(power - level);
        // Update closest if the current level has a smaller difference.
        if (diff < min_diff)
        {
            closest = level;
            min_diff = diff;
        }
    }
    return closest;
}

/**
 * @brief Validates and truncates a Maidenhead grid locator.
 *
 * This function validates the input grid locator by checking if it matches a valid 4-character
 * format (two letters followed by two digits). The locator is first converted to uppercase.
 * - If the locator is exactly 4 characters and valid, it is accepted as-is.
 * - If the locator is 6 or 8 characters long and its first 4 characters are valid,
 *   the locator is truncated to these 4 characters.
 *
 * @param locator A reference to the grid locator string. The string is modified in place
 *                if it needs to be truncated.
 * @return true if the locator is valid or has been successfully truncated to a valid format,
 *         false otherwise.
 */
bool validate_and_truncate_locator(std::string &locator)
{
    static const std::regex locator_pattern(R"(^[A-Za-z]{2}[0-9]{2})");

    // Convert to uppercase before validation
    std::transform(locator.begin(), locator.end(), locator.begin(), ::toupper);

    if (locator.length() == 4 && std::regex_match(locator, locator_pattern))
    {
        return true; // Already valid
    }

    // If locator is 6 or 8 characters, check first 4 characters
    if ((locator.length() == 6 || locator.length() == 8) && std::regex_match(locator.substr(0, 4), locator_pattern))
    {
        locator = locator.substr(0, 4); // Truncate to first 4 characters
        return true;
    }

    return false; // Invalid locator
}

/**
 * @brief Joins frequency strings from a specified starting index.
 *
 * This function concatenates elements from the provided vector of strings into a single string,
 * starting from the specified index. Each element is separated by a space. If the starting index is
 * equal to or greater than the number of elements in the vector, an empty string is returned.
 *
 * @param args A vector containing frequency strings.
 * @param start_index The index from which to start joining the frequency strings.
 * @return A string composed of the joined frequency values, or an empty string if no frequencies are available.
 */
std::string join_frequencies(const std::vector<std::string> &args, size_t start_index)
{
    if (start_index >= args.size())
    {
        return ""; // Return empty string if there are no frequencies
    }

    std::ostringstream oss;
    for (size_t i = start_index; i < args.size(); ++i)
    {
        if (i > start_index)
        {
            oss << " "; // Add space between elements
        }
        oss << args[i];
    }
    return oss.str();
}

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
void print_usage(const std::string &message, int exit_code)
{
    if (!message.empty())
    {
        std::cout << message << std::endl;
    }

    std::cerr << "\nUsage:\n"
              << "  (sudo) wsprrypi [options] callsign gridsquare transmit_power frequency <f2> <f3> ...\n"
              << "    OR\n"
              << "  (sudo) wsprrypi --test-tone {frequency}\n\n"
              << "Options:\n"
              << "  -h, --help\n"
              << "    Display this help message.\n"
              << "  -v, --version\n"
              << "    Show the WsprryPi version.\n"
              << "  -i, --ini-file <file>\n"
              << "    Load parameters from an INI file. Provide the path and filename.\n\n"
              << "See the documentation for a complete list of available options.\n\n";

    // Handle exit behavior
    switch (exit_code)
    {
    case 0:
        std::exit(EXIT_SUCCESS);
        break;
    case 1:
        std::exit(EXIT_FAILURE);
        break;
    case 3:
        return; // Simply return without exiting
    default:
        std::exit(exit_code);
        break;
    }
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
    // Attempt to load INI file if used and reload is called
    if (reload && config.use_ini)
    {
        if (ini.load()) // Load INI and only proceed if successful
        {
            load_from_ini();
        }
    }

    // Print overlayed configuration details
    //
    // [Control]
    llog.logS(DEBUG, "Transmit Enabled:", config.transmit ? "true" : "false");
    // [Common]
    llog.logS(DEBUG, "Call Sign:", config.callsign);
    llog.logS(DEBUG, "Grid Square:", config.grid_square);
    llog.logS(DEBUG, "Transmit Power:", config.power_dbm);
    llog.logS(DEBUG, "Frequencies:", config.frequencies);
    llog.logS(DEBUG, "Transmit Pin:", config.tx_pin);
    // [Extended]
    llog.logS(DEBUG, "PPM Offset:", config.ppm);
    llog.logS(DEBUG, "Check NTP Each Run:", config.use_ntp ? "true" : "false");
    llog.logS(DEBUG, "Use Frequency Randomization:", config.use_offset ? "true" : "false");
    llog.logS(DEBUG, "Power Level:", config.power_level);
    llog.logS(DEBUG, "Use LED:", config.use_led ? "true" : "false");
    llog.logS(DEBUG, "LED on GPIO", config.led_pin);
    // [Server]
    llog.logS(DEBUG, "Web server runs on port:", config.web_port);
    llog.logS(DEBUG, "Socket server runs on port:", config.socket_port);
    llog.logS(DEBUG, "Use shutdown button:", config.use_shutdown ? "true" : "false");
    llog.logS(DEBUG, "Shutdown button GPIO", config.shutdown_pin);
}

/**
 * @brief Validates and loads configuration data from the INI class.
 *
 * This function extracts configuration values from the INI class, ensuring that
 * required parameters such as callsign, grid square, transmit power, and
 * frequency list are properly set. If any critical parameter is missing or invalid,
 * the function logs the error and exits the program.
 *
 * @return true if the configuration is valid, false otherwise.
 */
bool validate_config_data()
{
    // Inline/lambda: Safely reads a frequency list from the config class
    std::istringstream frequency_list(
        [&]() -> std::string
        {
            try
            {
                return config.frequencies;
            }
            catch (const std::exception &e)
            {
                return ""; // Default to empty if an error occurs
            }
        }());

    // Parse frequency list to valid frequency, skips those that do not validate
    std::string token;
    while (frequency_list >> token)
    {
        try
        {
            // Parse frequency(ies) to double with validation
            double parsed_freq = lookup.parse_string_to_frequency(token, true);
            config.center_freq_set.push_back(parsed_freq);
        }
        catch (const std::invalid_argument &e)
        {
            llog.logE(WARN, "Invalid frequency ignored:", token);
        }
    }

    // Extract PPM value from config class, default to 0.0
    if (config.ppm == 0.0)
    {
        llog.logS(DEBUG, "Invalid PPM value, defaulting to use NTP.");
        config.use_ntp = true;
    }

    // Enable LED only if set and the pin is valid
    if (config.use_led && (config.led_pin >= 0 && config.led_pin <= 27))
    {
        ledControl.enable_gpio_pin(config.led_pin, true);
    }
    else
    {
        llog.logS(DEBUG, "Invalid or disabled LED settings, turning off LED.");
        ledControl.stop();
    }

    // Enable shutdown button only if it's desired and the pin is valid
    if (config.use_shutdown && (config.shutdown_pin >= 0 && config.shutdown_pin <= 27))
    {
        shutdownMonitor.enable(config.shutdown_pin, false, GPIOInput::PullMode::PullUp, callback_shutdown_system);
        shutdownMonitor.setPriority(SCHED_RR, 10);
    }
    else
    {
        llog.logS(DEBUG, "Invalid or disabled shutdown settings, disabling functionality.");
        shutdownMonitor.stop();
    }

    // Handle test tone mode (TONE mode does not require callsign, grid, etc.)
    if (config.mode == ModeType::TONE)
    {
        if (config.test_tone <= 0.0)
        {
            llog.logE(FATAL, "Test tone frequency must be positive.");
            return false;
        }

        // Log test tone frequency
        llog.logS(INFO, "A test tone will be generated at",
                  lookup.freq_display_string(config.test_tone), ".");

        if (config.use_ntp)
        {
            llog.logS(INFO, "NTP will be used to calibrate the tone frequency.");
        }
        else if (config.ppm != 0.0)
        {
            llog.logS(INFO, "PPM value to be used for tone generation:",
                      std::fixed, std::setprecision(2), config.ppm);
        }
    }
    else if (config.mode == ModeType::WSPR)
    {
        // Extract and validate required parameters
        bool missing_call_sign = [&]() -> bool
        {
            try
            {
                return config.callsign.empty();
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
                return config.grid_square.empty();
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
                return config.power_dbm <= 0;
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
        else
        {
            // Initialize global WSPR message
            message = new WsprMessage(
                config.callsign,
                config.grid_square,
                config.power_dbm);

            // Validate message
            if (message)
            {
                llog.logS(DEBUG, "WSPR message initialized.");
                // Build stream for WSPR symbols
                std::ostringstream wspr_stream;
                wspr_stream << "Generated WSPR symbols:\n";

                for (int i = 0; i < WsprMessage::size; ++i)
                {
                    wspr_stream << static_cast<int>(message->symbols[i]);
                    if (i < WsprMessage::size - 1)
                    {
                        wspr_stream << ","; // Append a comma except for the last element
                    }
                }

                // Send the formatted string to logger
                llog.logS(DEBUG, wspr_stream.str());
            }
            else
            {
                return false;
            }
        }

        // Log WSPR packet details
        llog.logS(INFO, "WSPR packet payload:");
        llog.logS(INFO, "- Callsign:", config.callsign);
        llog.logS(INFO, "- Locator:", config.grid_square);
        llog.logS(INFO, "- Power:", config.power_dbm, " dBm");
        llog.logS(INFO, "Requested TX frequencies:");

        // Concatenate frequency messages for logging
        for (const auto &freq : config.center_freq_set)
        {
            if (freq == 0.0)
            {
                llog.logS(INFO, "- Skip");
            }
            else
            {
                llog.logS(INFO, "- ", lookup.freq_display_string(freq));
            }
        }

        // Handle calibration and frequency adjustments
        if (config.use_ntp)
        {
            llog.logS(INFO, "Using NTP to calibrate transmission frequency.");
        }
        else if (config.ppm != 0.0)
        {
            llog.logS(INFO, "PPM value for all transmissions:",
                      std::fixed, std::setprecision(2), config.ppm);
        }

        // Set termination count (defaults to 1 if unset) if not in loop_tx and use_ini mode
        if (!config.transmit)
        {
            if (config.loop_tx)
            {
                llog.logS(INFO, "Transmissions will continue until it receives a signal to stop.");
            }
            else
            {
                llog.logS(INFO, "TX will stop after:", config.tx_iterations, "iteration(s) of the frequency list.");
            }
        }

        // Handle frequency offset
        if (config.use_offset)
        {
            llog.logS(INFO, "- A random offset will be added to all transmissions.");
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
 * @brief Loads configuration values from an INI file.
 *
 * This function attempts to load settings from an INI file using the global `ini` object
 * and populates the global `config` structure with values retrieved from it.
 *
 * If `config.use_ini` is false or if the INI file fails to load, the function immediately
 * returns false. Otherwise, it attempts to read values from various INI sections:
 *
 * - **[Control]**: Transmit flag.
 * - **[Common]**: Callsign, Grid Square, TX Power, Frequency, Transmit Pin.
 * - **[Extended]**: PPM, Use NTP, Offset, Power Level, Use LED, LED Pin.
 * - **[Server]**: Web Port, Socket Port, Use Shutdown, Shutdown Button.
 *
 * Each key is read inside a `try` block to allow partial loading—if a key is missing or
 * causes an exception, it is silently skipped, and loading continues.
 *
 * After successful loading, the global `jConfig` JSON object is updated to reflect the
 * contents of the `config` structure.
 *
 * @return true if the INI file was used and loaded successfully, false otherwise.
 */
bool load_from_ini()
{
    // Attempt to load INI file if enabled
    bool loaded = config.use_ini && ini.load();

    if (!loaded)
    {
        return false;
    }

    // Load Control section
    try
    {
        config.transmit = ini.get_bool_value("Control", "Transmit");
    }
    catch (...)
    {
    }

    // Load Common section
    try
    {
        config.callsign = ini.get_string_value("Common", "Call Sign");
    }
    catch (...)
    {
    }
    try
    {
        config.grid_square = ini.get_string_value("Common", "Grid Square");
    }
    catch (...)
    {
    }
    try
    {
        config.power_dbm = ini.get_int_value("Common", "TX Power");
    }
    catch (...)
    {
    }
    try
    {
        config.frequencies = ini.get_string_value("Common", "Frequency");
    }
    catch (...)
    {
    }
    try
    {
        config.tx_pin = ini.get_int_value("Common", "Transmit Pin");
    }
    catch (...)
    {
    }

    // Load Extended section
    try
    {
        config.ppm = ini.get_double_value("Extended", "PPM");
    }
    catch (...)
    {
    }
    try
    {
        config.use_ntp = ini.get_bool_value("Extended", "Use NTP");
    }
    catch (...)
    {
    }
    try
    {
        config.use_offset = ini.get_bool_value("Extended", "Offset");
    }
    catch (...)
    {
    }
    try
    {
        config.power_level = ini.get_int_value("Extended", "Power Level");
    }
    catch (...)
    {
    }
    try
    {
        config.use_led = ini.get_bool_value("Extended", "Use LED");
    }
    catch (...)
    {
    }
    try
    {
        config.led_pin = ini.get_int_value("Extended", "LED Pin");
    }
    catch (...)
    {
    }

    // Load Server section
    try
    {
        config.web_port = ini.get_int_value("Server", "Web Port");
    }
    catch (...)
    {
    }
    try
    {
        config.socket_port = ini.get_int_value("Server", "Socket Port");
    }
    catch (...)
    {
    }
    try
    {
        config.use_shutdown = ini.get_bool_value("Server", "Use Shutdown");
    }
    catch (...)
    {
    }
    try
    {
        config.shutdown_pin = ini.get_int_value("Server", "Shutdown Button");
    }
    catch (...)
    {
    }

    // Synchronize config with global JSON object
    config_to_json();

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
bool parse_command_line(int argc, char *argv[])
{
    // Create original JSON
    init_config_json();
    std::vector<char *> args(argv, argv + argc); // Copy arguments for modification

    // First pass: Look for "-i <file>" before processing other options
    for (auto it = args.begin() + 1; it != args.end(); ++it)
    {
        if ((std::string(*it) == "-i" || std::string(*it) == "--ini-file") && (it + 1) != args.end())
        {
            config.ini_filename = *(it + 1);
            config.use_ini = true;
            config.loop_tx = true;
            // Create original JSON and Config struct, overlay INI contents
            ini.set_filename(config.ini_filename);
            config_to_json();
            ini_to_json(config.ini_filename);
            json_to_config();

            // Remove "-i <file>" from args
            args.erase(it, it + 2);
            break; // Exit loop after removing argument
        }
    }

    // Update argc and argv pointers for getopt_long()
    argc = args.size();
    argv = args.data();

    // Update argc and argv pointers for getopt_long()
    argc = args.size();
    argv = args.data();

    static struct option long_options[] = {
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'v'},
        {"use-ntp", no_argument, nullptr, 'n'},       // Via: [Extended] Use NTP = True
        {"repeat", no_argument, nullptr, 'r'},        // Global: config.loop_tx
        {"offset", no_argument, nullptr, 'o'},        // Via: [Extended] Offset = True
        {"date-time-log", no_argument, nullptr, 'D'}, // Global: config.date_time_log
        // Required arguments
        {"ppm", required_argument, nullptr, 'p'},             // Via: [Extended] PPM = 0.0
        {"terminate", required_argument, nullptr, 'x'},       // Global: config.tx_iterations
        {"test-tone", required_argument, nullptr, 't'},       // Global: config.test_tone
        {"transmit-pin", required_argument, nullptr, 'a'},    // Via: [Common] Transmit Pin = 4
        {"led_pin", required_argument, nullptr, 'l'},         // Via: [Extended] LED Pin = 18
        {"shutdown_button", required_argument, nullptr, 's'}, // Via: [Server] Shutdown Button = 19
        {"power_level", required_argument, nullptr, 'd'},     // Via: [Extended] Power Level = 7
        {"web-port", required_argument, nullptr, 'w'},        // Via: [Server] Port = 31415
        {"socket-port", required_argument, nullptr, 'k'},     // Via: [Server] Port = 31415
        {nullptr, 0, nullptr, 0}};

    while (true)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "h?vnrDp:x:t:a:l:s:d:w:k:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
        // No arguments
        case 'h': // Help/Usage
        case '?':
        {
            print_usage(EXIT_SUCCESS);
        }
        case 'v': // Version
        {
            std::cout << version_string() << std::endl;
            std::exit(EXIT_SUCCESS);
        }
        case 'n': // Use NTP
        {
            config.use_ntp = true;
            break;
        }
        case 'r': // Repeat
        {
            config.loop_tx = true;
            break;
        }
        case 'o': // Use Offset
        {
            config.use_offset = true;
            break;
        }
        case 'D': // Add date/time stamps to logging
        {
            config.date_time_log = true;
            llog.enableTimestamps(config.date_time_log);
            break;
        }
        // Required arguments
        case 'p': // Apply PPM
        {
            try
            {
                double ppm = std::stod(optarg);
                double clamped_ppm = std::clamp(ppm, -200.0, 200.0);

                if (ppm != clamped_ppm)
                {
                    llog.logE(ERROR, "PPM value is outside bounds (-200 to 200), applying clamped value:", clamped_ppm);
                }

                // Apply the clamped value
                config.ppm = clamped_ppm;
                config.use_ntp = false;
            }
            catch (const std::exception &)
            {
                config.use_ntp = true;
                llog.logE(ERROR, "Error parsing PPM value, defaulting to NTP:", optarg);
            }
            break;
        }
        case 'x': // Terminate after x iterations
        {
            if (!config.use_ini)
            {
                try
                {
                    config.tx_iterations = std::stoi(optarg);
                }
                catch (const std::invalid_argument &)
                {
                    llog.logE(ERROR, "Invalid number format for transmit iterations:", optarg, "- Using default (1).");
                }
                catch (const std::out_of_range &)
                {
                    llog.logE(ERROR, "Number out of range for transmit iterations:", optarg, "- Using default (1).");
                }
                // Set config.tx_iterations to at least 1
                config.tx_iterations = (config.tx_iterations == 0) ? 1 : config.tx_iterations; // Equal to at least 1
                config.loop_tx = false;
            }
            break;
        }
        case 't': // Use test-tone
        {
            if (!config.use_ini)
            {
                try
                {
                    config.test_tone = lookup.parse_string_to_frequency(optarg, false);
                    config.mode = ModeType::TONE;

                    if (config.test_tone <= 0.0)
                    {
                        print_usage("Invalid test tone frequency (<=0).", EXIT_FAILURE);
                    }
                }
                catch (const std::invalid_argument &e)
                {
                    std::string error_message = "Invalid test tone frequency input: " +
                                                std::string(optarg) +
                                                " Exception: " + e.what();
                    print_usage(error_message, EXIT_FAILURE);
                }
            }
            else
            {
                print_usage("Test tone is invalid when using INI file.", EXIT_FAILURE);
            }
            break;
        }
        case 'a': // Specify transmit pin
        {
            try
            {
                int transmit_pin = std::stoi(optarg);
                if (transmit_pin < 0 || transmit_pin > 27)
                    print_usage("Invalid transmit pin.", EXIT_FAILURE);
                else
                    config.tx_pin = transmit_pin;
            }
            catch (const std::exception &)
            {
                print_usage("Invalid transmit pin.", EXIT_FAILURE);
            }
            break;
        }
        case 'l': // LED Pin
        {
            try
            {
                int led_pin = std::stoi(optarg);
                if (led_pin < 0 || led_pin > 27)
                {
                    print_usage("Invalid LED pin.", EXIT_FAILURE);
                }

                else
                {
                    config.led_pin = led_pin;
                    config.use_led = true;
                }
            }
            catch (const std::exception &)
            {
                print_usage("Invalid LED pin.", EXIT_FAILURE);
            }
            break;
        }
        case 's': // Shutdown button/pin
        {
            try
            {
                int shutdown_pin = std::stoi(optarg);
                if (shutdown_pin < 0 || shutdown_pin > 27)
                {
                    print_usage("Invalid shutdown pin.", EXIT_FAILURE);
                }

                else
                {
                    config.shutdown_pin = shutdown_pin;
                    config.use_shutdown = true;
                }
            }
            catch (const std::exception &)
            {
                print_usage("Invalid shutdown pin.", EXIT_FAILURE);
            }
            break;
        }
        case 'd': // Power Level 0-7
        {
            try
            {
                int power = std::stoi(optarg);
                if (power < 0 or power > 7)
                {
                    config.power_level = 7;
                }
                else
                {
                    config.power_level = power;
                }
            }
            catch (const std::exception &)
            {
                llog.logE(WARN, "Invalid power level, defaulting to 7.");
                config.power_level = 7;
            }
            break;
        }
        case 'w': // Set web port number
        {
            try
            {
                int port = std::stoi(optarg);
                if (port < 1024 || port > 49151)
                {
                    llog.logS(WARN, "Invalid web number. Using default: 31415.");
                    config.web_port = 31415;
                }
                else
                {
                    config.web_port = port;
                }
            }
            catch (const std::exception &)
            {
                llog.logE(WARN, "Invalid web port, defaulting to 31415.");
                config.web_port = 31415;
            }
            break;
        }
        case 'k': // Set socket port number
        {
            try
            {
                int port = std::stoi(optarg);
                if (port < 1024 || port > 49151)
                {
                    llog.logS(WARN, "Invalid socket port number. Using default: 31416.");
                    config.web_port = 31416;
                }
                else
                {
                    config.web_port = port;
                }
            }
            catch (const std::exception &)
            {
                llog.logE(WARN, "Invalid socket port, defaulting to 31416.");
                config.web_port = 31415;
            }
            break;
        }
        default:
        {
            llog.logE(ERROR, "Unknown argument: '", static_cast<char>(c), "'");
            break;
        }
        }
    }
    // Re-save any config changes in the JSON
    config_to_json();

    if (config.mode == ModeType::WSPR)
    {
        if (!config.use_ini)
        {
            // Handle positional arguments after parsing options
            std::vector<std::string> positional_args;
            for (int i = optind; i < argc; ++i)
            {
                positional_args.push_back(argv[i]);
            }

            // Extract required positional arguments
            if (positional_args.size() < 4)
            {
                print_usage("Missing required positional arguments: callsign, gridsquare, power, and frequency.", EXIT_FAILURE);
            }

            // Validate callsign with REGEX
            if (is_valid_callsign(positional_args[0]))
            {
                config.callsign = positional_args[0];
            }
            else
            {
                print_usage("Invalid call sign '" + positional_args[0] + "' for type 1 WSPR message.", EXIT_FAILURE);
            }

            // Validate and truncate gridsquare
            std::string gridsquare = positional_args[1];
            if (validate_and_truncate_locator(gridsquare))
            {
                if (gridsquare != positional_args[1])
                {
                    llog.logS(DEBUG, "Grid square truncated to:", gridsquare);
                }
                config.grid_square = gridsquare;
            }
            else
            {
                print_usage("Invalid maidenhead locator: " + gridsquare, EXIT_FAILURE);
            }

            // Validate power to standard values
            try
            {
                int power = std::stoi(positional_args[2]);
                int rounded_power = round_to_nearest_wspr_power(power);
                if (power != rounded_power)
                {
                    llog.logS(DEBUG, "Power rounded to standard value:", rounded_power);
                }
                config.power_dbm = rounded_power;
            }
            catch (...)
            {
                print_usage("Invalid power value. Must be an integer.", EXIT_FAILURE);
            }

            // Put frequencies in string, validate later
            // Convert frequencies (positional_args[3] and beyond) into a space-separated string
            try
            {
                config.frequencies = join_frequencies(positional_args, 3);
            }
            catch (const std::exception &e)
            {
                print_usage(std::string("Failed to capture frequencies: ") + e.what(), EXIT_FAILURE);
            }
        }
    }
    // Re-save any config changes in the JSON
    config_to_json();

    return true;
}

/**
 * @brief Loads and validates the configuration from command-line arguments.
 *
 * This function processes command-line arguments to load the application's configuration.
 * It performs the following steps:
 * - Checks whether any arguments are provided (beyond the program name).
 * - Parses the command-line arguments.
 * - Validates the configuration data to ensure that all required settings are present.
 *
 * If any of these steps fail (e.g., no arguments provided, parsing errors, or validation errors),
 * the function calls @c print_usage with an appropriate error message and terminates the program.
 *
 * @param argc The count of command-line arguments.
 * @param argv The array of command-line argument strings.
 * @return true if the configuration was successfully loaded and validated; otherwise, the program exits.
 */
bool load_config(int argc, char *argv[])
{
    // Initialize return value to false.
    bool retval = false;

    // Check if any arguments (besides the program name) were provided.
    if (argc == 1) // No arguments or options provided.
    {
        print_usage("No arguments provided.", EXIT_FAILURE);
    }

    // Parse command-line arguments and exit if invalid.
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

    return retval;
}
