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
#include "transmit.hpp"
#include "version.hpp"
#include "wspr_message.hpp"
#include "wspr_band_lookup.hpp"

// Standard library headers
#include <atomic>
#include <optional>
#include <thread>

/**
 * @brief Global configuration instance for argument parsing and runtime settings.
 *
 * @details
 * Holds all command-line and runtime configuration data not managed directly
 * by the INI file system. Initialized globally and used throughout the application.
 *
 * @see ArgParserConfig, ini, iniMonitor
 */
struct ArgParserConfig
{
    ModeType mode; ///< Current operating mode.

    // Control
    bool use_ini;  ///< Load configuration from INI file.
    bool transmit; ///< Transmission mode enabled.

    // Common
    std::string callsign;    ///< WSPR callsign.
    std::string grid_square; ///< 4- or 6-character Maidenhead locator.
    int power_dbm;           ///< Transmit power in dBm.
    std::string frequencies; ///< Comma-separated frequency list.
    int tx_pin;              ///< GPIO pin number for RF transmit control.

    // Extended
    double ppm;      ///< PPM frequency calibration.
    bool use_ntp;    ///< Apply NTP-based frequency correction.
    bool use_offset; ///< Enable random frequency offset.
    int power_level; ///< Power level for RF hardware (0–7).
    bool use_led;    ///< Enable TX LED indicator.
    int led_pin;     ///< GPIO pin for LED indicator.

    // Server
    int server_port;   ///< TCP server port number.
    bool use_shutdown; ///< Enable GPIO-based shutdown feature.
    int shutdown_pin;  ///< GPIO pin used to signal shutdown.

    // Command line only
    bool date_time_log; ///< Prefix logs with timestamp.
    bool loop_tx;       ///< Repeat transmission cycle.
    int tx_iterations;  ///< Number of transmission iterations (0 = infinite).
    double test_tone;   ///< Enable continuous tone mode (in Hz).

    // Runtime variables
    std::vector<double> center_freq_set; ///< Parsed list of center frequencies in Hz.
    double f_plld_clk;                   ///< Clock speed (defaults to 500 MHz).
    int mem_flag;                        ///< Reserved for future memory management flags.

    /**
     * @brief Default constructor initializing all configuration parameters.
     */
    ArgParserConfig()
        : mode(ModeType::WSPR),
          use_ini(false),
          transmit(false),
          callsign(""),
          grid_square(""),
          power_dbm(0),
          frequencies(""),
          tx_pin(-1),
          ppm(0.0),
          use_ntp(false),
          use_offset(false),
          power_level(7),
          use_led(false),
          led_pin(-1),
          server_port(31415),
          use_shutdown(false),
          shutdown_pin(-1),
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

// -----------------------------------------------------------------------------
// Global Configuration Declarations
// -----------------------------------------------------------------------------

extern ArgParserConfig config;               ///< Global runtime config.
extern IniFile ini;                          ///< Global INI configuration object.
extern MonitorFile iniMonitor;               ///< Watches INI file for changes.
extern WSPRBandLookup lookup;                ///< Global band/frequency resolver.
extern WsprMessage *message;                 ///< Global WSPR message builder.
extern std::atomic<int> wspr_interval;       ///< WSPR transmission interval (2 or 15 minutes).
extern std::atomic<bool> ini_reload_pending; ///< Semaphore for deferred INI reload.

/**
 * @brief Called when the INI file is modified.
 *
 * @details
 * Executed by the `MonitorFile` watcher thread. Sets a deferred reload flag
 * (`ini_reload_pending`) to apply changes after the current transmission finishes.
 */
void callback_ini_changed();

/**
 * @brief Applies configuration updates after transmission is complete.
 *
 * @details
 * Checks the `ini_reload_pending` flag and, if true, calls
 * `validate_config_data()` to re-apply configuration changes.
 */
extern void apply_deferred_changes();

/**
 * @brief Prints usage information to standard error.
 *
 * @details
 * Shows supported command-line options and optionally displays an error message.
 *
 * @param message Optional error message (shown above usage output).
 * @param exit_code Exit behavior:
 *   - `0`: Exit with `EXIT_SUCCESS`
 *   - `1`: Exit with `EXIT_FAILURE`
 *   - `3`: Print help and return without exiting
 *   - any other: Call `std::exit(exit_code)`
 */
void print_usage(const std::string &message = "", int exit_code = 3);

/**
 * @brief Overload for `print_usage()` that accepts only an exit code.
 *
 * @param exit_code Exit behavior (see above).
 */
inline void print_usage(int exit_code)
{
    print_usage("", exit_code);
}

/**
 * @brief Prints the current INI configuration values.
 *
 * @param reload If true, includes "reload" context in the log.
 */
extern void show_config_values(bool reload = false);

/**
 * @brief Validates and applies configuration values from the INI file.
 *
 * @return true if configuration is valid and applied.
 * @return false if validation fails (application exits).
 */
extern bool validate_config_data();

/**
 * @brief Loads INI and runtime configuration values.
 *
 * @details
 * Parses the INI file, validates it, and populates the global config structure.
 *
 * @return true on success, false if loading fails.
 */
extern bool load_from_ini();

/**
 * @brief Parses command-line arguments and applies overrides.
 *
 * @param argc Number of command-line arguments.
 * @param argv Argument vector.
 * @return true if parsing succeeds; false otherwise.
 */
bool parse_command_line(int argc, char *argv[]);

/**
 * @brief Wrapper for full configuration loading from all sources.
 *
 * @details
 * Loads the INI file (if enabled), then processes command-line arguments,
 * ensuring all values are validated and resolved into a final config.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @return true if loading and validation succeed.
 */
bool load_config(int argc, char *argv[]);

#endif // ARG_PARSER_HPP
