// TODO:  Check Doxygen

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
#include "logging.hpp"
#include "scheduling.hpp"
#include "signal_handler.hpp"
#include "transmit.hpp"
#include "wspr_band_lookup.hpp"
#include "wspr_message.hpp"

// Standard library headers
#include <algorithm>
#include <atomic>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

// System headers
#include <getopt.h>

bool useini = false;
std::string inifile = "";
bool date_time_log = true;

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

WsprMessage *message = nullptr; // Initialize to null

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

std::thread ini_thread;

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
 * @note This thread continues running until `exit_wspr_loop` is set to `true`.
 *       It checks for file changes every second.
 */
void ini_monitor_thread()
{
    while (!exit_wspr_loop.load())
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
    std::cerr << "\nUsage:\n"
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
    bool loaded = useini && ini.load();

    if (!loaded)
        return;

    // Print configuration details if successfully loaded
    llog.logS(DEBUG, "Config", (reload ? "re-loaded" : "loaded"), "from:", inifile);
    // [Control]
    llog.logS(DEBUG, "Transmit Enabled:", ini.get_bool_value("Control", "Transmit") ? "true" : "false");
    // [Common]
    llog.logS(DEBUG, "Call Sign:", ini.get_string_value("Common", "Call Sign"));
    llog.logS(DEBUG, "Grid Square:", ini.get_string_value("Common", "Grid Square"));
    llog.logS(DEBUG, "Transmit Power:", ini.get_int_value("Common", "TX Power"));
    llog.logS(DEBUG, "Frequencies:", ini.get_string_value("Common", "Frequency"));
    llog.logS(DEBUG, "Transmit Pin:", ini.get_int_value("Common", "Transmit Pin"));
    // [Extended]
    llog.logS(DEBUG, "PPM Offset:", ini.get_double_value("Extended", "PPM"));
    llog.logS(DEBUG, "Check NTP Each Run:", ini.get_bool_value("Extended", "Use NTP") ? "true" : "false");
    llog.logS(DEBUG, "Use Frequency Randomization:", ini.get_bool_value("Extended", "Offset") ? "true" : "false");
    llog.logS(DEBUG, "Power Level:", ini.get_int_value("Extended", "Power Level"));
    llog.logS(DEBUG, "Use LED:", ini.get_bool_value("Extended", "Use LED") ? "true" : "false");
    llog.logS(DEBUG, "LED on GPIO", ini.get_int_value("Extended", "LED Pin"));
    // [Server]
    llog.logS(DEBUG, "Server runs on port:", ini.get_int_value("Server", "Port"));
    llog.logS(DEBUG, "Use shutdown buton:", ini.get_bool_value("Server", "Use Shutdown") ? "true" : "false");
    llog.logS(DEBUG, "Shutdown button GPIO", ini.get_int_value("Server", "Shutdown Button"));
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

    while (frequency_list >> token)
    {
        try
        {
            // Parse frequency(ies) to double with validation
            double parsed_freq = lookup.parse_string_to_frequency(token, true);
            center_freq_set.push_back(parsed_freq);
        }
        catch (const std::invalid_argument &e)
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
    // Get LED use choice
    bool use_led = false;
    try
    {
        use_led = ini.get_bool_value("Extended", "Use LED");
        // Get PIN number
        int new_led_pin_number = led_pin_number; // Use default value
        try
        {
            new_led_pin_number = ini.get_int_value("Extended", "LED Pin");
            // Enable LED only if valid and desired
            if (use_led && (new_led_pin_number >= 0 && new_led_pin_number <= 27))
            {
                enable_led_pin(new_led_pin_number);
            }
            else
            {
                llog.logS(DEBUG, "Failure retrieving LED values, disabling LED.");
                disable_led_pin();
            }
        }
        catch (const std::exception &e)
        {
            llog.logS(DEBUG, "Failed to get LED Pin value, using default.");
        }
    }
    catch (const std::exception &e)
    { // Added exception type
        llog.logS(DEBUG, "Failed to get LED use value, disabling LED.");
        disable_led_pin();
    }

    // Turn on shutdown pin functionality
    //
    // Get shutdown button use choice
    bool use_shutdown = false;
    try
    {
        use_shutdown = ini.get_bool_value("Server", "Use Shutdown") || false;
        // Get PIN number
        int new_shutdown_pin_number = shutdown_pin_number; // Use default value
        try
        {
            new_shutdown_pin_number = ini.get_int_value("Server", "Shutdown Button");

            // Enable shutdown button only if valid and desired
            if (use_shutdown && (new_shutdown_pin_number >= 0 && new_shutdown_pin_number <= 27))
            {
                enable_shutdown_pin(new_shutdown_pin_number);
            }
            else
            {
                disable_shutdown_pin();
            }
        }
        catch (const std::exception &e)
        {
            llog.logS(DEBUG, "Failed to get shutdown pin preferences, disabling functionality.");
            disable_shutdown_pin();
        }
    }
    catch (const std::exception &e)
    { // Added exception type
        llog.logS(DEBUG, "Failed to get shutdown pin use value, disabling shutdown.");
        disable_shutdown_pin();
    }

    // Handle test tone mode (TONE mode does not require callsign, grid, etc.)
    if (mode == ModeType::TONE)
    {
        if (test_tone <= 0)
        {
            llog.logE(FATAL, "Test tone frequency must be positive.");
            return false;
        }

        // Log test tone frequency
        llog.logS(INFO, "A test tone will be generated at",
            lookup.freq_display_string(test_tone), ".");

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
    else if (mode == ModeType::WSPR)
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

        bool no_frequencies = center_freq_set.empty();

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
                ini.get_string_value("Common", "Call Sign"),
                ini.get_string_value("Common", "Grid Square"),
                ini.get_int_value("Common", "TX Power"));

            // Validate message
            if (message)
            {
                llog.logS(DEBUG, "WSPR message initialized.");
            }
            else
            {
                return false;
            }
        }

        // Log WSPR packet details
        llog.logS(INFO, "WSPR packet payload:");
        llog.logS(INFO, "- Callsign:", ini.get_string_value("Common", "Call Sign"));
        llog.logS(INFO, "- Locator:", ini.get_string_value("Common", "Grid Square"));
        llog.logS(INFO, "- Power:", ini.get_int_value("Common", "TX Power"), " dBm");
        llog.logS(INFO, "Requested TX frequencies:");

        // Concatenate frequency messages for logging
        for (const auto &freq : center_freq_set)
        {
            if (freq == 0.0)
            {
                llog.logS(INFO, "- Skip");
            }
            else
            {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(6) << (freq / 1e6); // Format the frequency
                llog.logS(INFO, "- ", oss.str(), " MHz");
            }
        }

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

        // Set termination count (defaults to 1 if unset) if not in loop mode
        if (loop_tx)
        {
            llog.logS(INFO, "Transmissions will continue until stopped with CTRL-C.");
        }
        else
        {
            tx_iterations = tx_iterations.value_or(1);
            llog.logS(INFO, "TX will stop after:", tx_iterations.value(), "iteration(s) of the frequency list.");
        }

        // Handle frequency offset
        if (offset_enabled)
        {
            llog.logS(INFO, "- A random offset will be added to all transmissions.");
        }
    }
    else
    {
        llog.logE(FATAL, "Mode must be either WSPR or TONE.");
        std::exit(EXIT_FAILURE);
    }

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
bool parse_command_line(int argc, char *const argv[])
{
    // First pass: Look for "-i <file>" before processing other options
    for (int i = 1; i < argc; ++i)
    {
        if ((std::string(argv[i]) == "-i" || std::string(argv[i]) == "--ini-file") && i + 1 < argc)
        {
            inifile = argv[i + 1];
            useini = true;
            ini.set_filename(inifile);
            iniMonitor.filemon(inifile);
            break;
        }
    }

    static struct option long_options[] = {
        // No arguments
        {"help", no_argument, nullptr, 'h'},
        {"help", no_argument, nullptr, '?'},
        {"version", no_argument, nullptr, 'v'},
        {"use-ntp", no_argument, nullptr, 'n'},
        {"repeat", no_argument, nullptr, 'r'},
        {"offset", no_argument, nullptr, 'o'},
        {"led", no_argument, nullptr, 'l'},
        {"date-time-log", no_argument, nullptr, 'D'},
        // Arguments required
        {"ppm", required_argument, nullptr, 'p'},
        {"terminate", required_argument, nullptr, 'x'},
        {"test-tone", required_argument, nullptr, 't'},
        {"led_pin", required_argument, nullptr, 'b'},
        {"power_level", required_argument, nullptr, 'd'},
        {"port", required_argument, nullptr, 'e'},

        {nullptr, 0, nullptr, 0}};

    std::regex callsign_regex(R"(^([A-Za-z]{1,2}[0-9][A-Za-z0-9]{1,3}|[A-Za-z][0-9][A-Za-z]|[0-9][A-Za-z][0-9][A-Za-z0-9]{2,3})$)");
    std::regex gridsquare_regex(R"(^[A-Za-z]{2}[0-9]{2}$)");

    // Parse options
    while (true)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "h?vnrlDp:x:t:b:d:e:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
        // No arguments
        case 'h': // Print usage (help)
        case '?':
            print_usage();
            std::exit(EXIT_SUCCESS);

        case 'v': // Print version
            std::cout << version_string() << std::endl;
            std::exit(EXIT_SUCCESS);

        case 'n':
            ini.set_bool_value("Extended", "Use NTP", true);
            break;

        case 'r':
            loop_tx = true;
            break;

        case 'o':
            ini.set_bool_value("Extended", "Offset", true);
            break;

        case 'l':
            ini.set_bool_value("Extended", "Use LED", true);
            break;

        case 'D':
            date_time_log = true;
            llog.enableTimestamps(date_time_log);
            break;

        // Options that require arguments
        case 'p': // PPM Value
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

        case 'x': // Terminate after x iterations
            try
            {
                tx_iterations = std::stoi(optarg);
                if (tx_iterations.value() < 1)
                    throw std::invalid_argument("Termination count must be >= 1");
            }
            catch (const std::exception &)
            {
                llog.logE(WARN, "Invalid termination parameter, defaulting to 1.");
                tx_iterations = 1;
            }
            break;

        case 't': // Test-tone
            try
            {
                // Parse frequency(ies) to double without ham band validation
                test_tone = lookup.parse_string_to_frequency(optarg, false);
                mode = ModeType::TONE;

                if (test_tone <= 0.0)
                {
                    llog.logE(ERROR, "Invalid test tone frequency (<=0).");
                    return false;
                }
            }
            catch (const std::exception &e)
            {
                llog.logE(ERROR, "Invalid test tone frequency: ", optarg);
                return false;
            }
            break;

        case 'b': // LED Pin
            try
            {
                int led_pin = std::stoi(optarg);
                led_pin = std::clamp(led_pin, 0, 27);
                ini.set_int_value("Extended", "LED Pin", led_pin);
            }
            catch (const std::exception &)
            {
                llog.logE(ERROR, "Invalid LED pin.");
                return false;
            }
            break;

        case 'd': // Power level on GPIO pin
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

        case 'e': // Services port number
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

    // Capture and validate positional arguments if we are not using an INI file
    if (!useini && mode == ModeType::WSPR)
    {
        if (optind >= argc)
        {
            throw std::runtime_error("Missing required argument: Call Sign.");
        }

        // Validate and store callsign
        std::string callsign(argv[optind]);
        if (!std::regex_match(callsign, callsign_regex))
        {
            throw std::invalid_argument("Invalid callsign format: " + callsign);
        }
        ini.set_string_value("Common", "Call Sign", callsign);
        ++optind;

        if (optind >= argc)
        {
            throw std::runtime_error("Missing required argument: Grid Square.");
        }

        // Validate and store grid square
        std::string gridsquare(argv[optind]);
        if (!std::regex_match(gridsquare, gridsquare_regex))
        {
            throw std::invalid_argument("Invalid grid square format: " + gridsquare);
        }
        ini.set_string_value("Common", "Grid Square", gridsquare);
        ++optind;

        if (optind >= argc)
        {
            throw std::runtime_error("Missing required argument: Transmit Power.");
        }

        // Validate and store transmit power
        std::string tx_power_str(argv[optind]);
        int tx_power;
        try
        {
            tx_power = std::stoi(tx_power_str);
            if (tx_power < -10 || tx_power > 62)
            {
                throw std::out_of_range("Transmit power out of range (-10-62): " + tx_power_str);
            }
        }
        catch (const std::exception &)
        {
            throw std::invalid_argument("Invalid transmit power: " + tx_power_str);
        }
        ini.set_int_value("Common", "TX Power", tx_power);
        ++optind;

        // Parse frequencies
        center_freq_set.clear();
        while (optind < argc)
        {
            std::string_view freq_input = argv[optind]; // No unnecessary copies

            try
            {
                double parsed_freq = lookup.parse_string_to_frequency(freq_input, true);
                center_freq_set.push_back(parsed_freq);
            }
            catch (const std::invalid_argument &e)
            {
                throw std::invalid_argument("Ignoring invalid frequency: " + std::string(freq_input));
            }

            ++optind;
        }

        if (center_freq_set.empty())
        {
            throw std::runtime_error("No valid frequencies provided.");
        }

        // Debug print center_freq_set
        std::ostringstream freq_stream;
        freq_stream << "Frequencies in frequency set [";
        freq_stream << center_freq_set.size() << "]: ";

        for (const auto &freq : center_freq_set)
        {
            // Apply formatting explicitly for each number
            freq_stream << std::fixed << std::setprecision(6) << (freq / 1e6) << " MHz, ";
        }

        // Remove trailing comma and space (if present)
        std::string output = freq_stream.str();
        if (!center_freq_set.empty())
        {
            output.pop_back(); // Remove last space
            output.pop_back(); // Remove last comma
        }
        freq_stream << ".";
        llog.logS(DEBUG, output);

        return true;
    }
    else if (mode == ModeType::TONE)
    {
        return true;
    }
    else if (mode == ModeType::WSPR)
    {
        return true;
    }

    // Unknown conditions
    throw std::runtime_error("Unknown argument set.");
    return false;
}
