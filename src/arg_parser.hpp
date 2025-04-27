/**
 * @file arg_parser.hpp
 * @brief Command-line argument parser and configuration handler.
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

// Standard library headers
#include <atomic>
#include <optional>
#include <thread>

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
extern MonitorFile iniMonitor;

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
extern WSPRBandLookup lookup;

/**
 * @brief Pointer to a WSPR message.
 *
 * This pointer is used to reference a WsprMessage object, which constructs a WSPR message
 * from a callsign, grid location, and power level. It is initialized to nullptr until
 * a valid WsprMessage instance is created.
 *
 * @note Remember to allocate memory for this pointer before use.
 */
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
