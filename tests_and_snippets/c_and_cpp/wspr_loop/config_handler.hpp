/**
 * @file config_handler.hpp
 * @brief Provides an interface to ArgParserConfig and JSON config
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

#ifndef _CONFIG_HANDLER_HPP
#define _CONFIG_HANDLER_HPP

#include "ini_file.hpp"
#include "json.hpp"

#include <string>
#include <vector>

/**
 * @brief Global INI handler instance.
 *
 * This instance of the IniFile class is used to load, save, and manage the
 * INI file configuration for the application.
 */
extern IniFile ini;

/**
 * @brief Global JSON configuration object.
 *
 * This nlohmann::json object holds the application's configuration in JSON format.
 * It is used to merge and update configuration settings dynamically.
 */
extern nlohmann::json jConfig;

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
    // Control
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
    ModeType mode;                       ///< Current operating mode.
    bool use_ini;                        ///< Load configuration from INI file.
    std::string ini_filename;            ///< INI file name and path.
    std::vector<double> center_freq_set; ///< Parsed list of center frequencies in Hz.
    double f_plld_clk;                   ///< Clock speed (defaults to 500 MHz).
    int mem_flag;                        ///< Reserved for future memory management flags.

    /**
     * @brief Default constructor initializing all configuration parameters.
     */
    ArgParserConfig()
        : transmit(false),
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
          server_port(31415), // TODO: Should not need to set this
          use_shutdown(false),
          shutdown_pin(-1),
          date_time_log(false),
          loop_tx(false),
          tx_iterations(0),
          test_tone(0.0),
          mode(ModeType::WSPR),
          use_ini(false),
          ini_filename(""),
          center_freq_set({}),
          f_plld_clk(0.0),
          mem_flag(0)
    {
    }
};

/**
 * @brief Global configuration object.
 *
 * This ArgParserConfig instance holds the application’s configuration settings,
 * typically loaded from an INI file or a JSON configuration.
 */
extern ArgParserConfig config;

/**
 * @brief Creates a JSON object from the configuration struct.
 *
 * @details
 * This function overlays the configuration stored in an ArgParser struct
 * onto a JSON object. It uses the same structure as the original JSON file,
 * converting booleans and numbers into strings where needed (since the
 * parsing code expects strings).
 *
 * @param config The configuration struct to overlay.
 */
extern void build_json_from_config();

/**
 * @brief Saves the global JSON configuration back to the INI file.
 *
 * @details
 * If the configuration indicates that an INI file is being used (i.e. `config.use_ini`
 * is true), this function first updates the global JSON configuration by calling
 * `build_json_from_config()`. It then converts the JSON configuration (`jConfig`)
 * into an internal data structure (`newData`) suitable for the INI handler. Each
 * section in the JSON becomes a key in the map, with its value being an unordered map
 * of key/value pairs. If a JSON value is an array, it is converted to a string using
 * the `dump()` method; otherwise, the value is retrieved as a string.
 *
 * Finally, the new data is set into the global INI handler object (`ini`) using
 * `ini.setData(newData)` and saved to disk via `ini.save()`.
 *
 * @note This function assumes that all JSON values can be represented as strings.
 */
extern void save_json();

/**
 * @brief Loads the global JSON configuration by merging default JSON and INI file data.
 *
 * @details
 * This function performs a three-step process:
 *  1. Calls `init_config_json()` to create a base JSON configuration with default values.
 *  2. Calls `patch_ini_data(filename)` to overlay INI file data (from the given filename)
 *     onto the base JSON configuration.
 *  3. Calls `json_to_config()` to parse the updated JSON configuration into the global
 *     configuration structure (of type `ArgParser`).
 *
 * This layered approach allows default settings to be overridden by INI file values.
 *
 * @param filename The path to the INI file whose data will be merged into the JSON configuration.
 */
extern void load_json(std::string filename);

#endif // _CONFIG_HANDLER_HPP
