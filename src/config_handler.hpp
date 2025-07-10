/**
 * @file config_handler.hpp
 * @brief Provides an interface to ArgParserConfig and JSON config
 *
 * This project is is licensed under the MIT License. See LICENSE.md
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

#include <atomic>
#include <string>
#include <vector>

/**
 * @brief  Construct the singleton IniFile instance.
 *
 * Provides a global reference `iniFile` that resolves to
 * `IniFile::instance()`.
 */
inline auto &iniFile = IniFile::instance();

/**
 * @brief Global JSON configuration object.
 *
 * This nlohmann::json object holds the application's configuration in JSON format.
 * It is used to merge and update configuration settings dynamically.
 */
extern nlohmann::json jConfig;

/**
 * @enum ModeType
 * @brief Represents the operational mode of the application.
 *
 * Enumerates the supported transmission or test modes available in the system.
 *
 * @var ModeType::WSPR
 * WSPR (Weak Signal Propagation Reporter) transmission mode.
 *
 * @var ModeType::TONE
 * Test tone generation mode used for calibration or diagnostics.
 *
 * @var ModeType::QRSS
 * QRSS mode for extremely slow speed CW (continuous wave) transmissions.
 *
 * @var ModeType::UNKNOWN
 * Sentinel value indicating an unrecognized or unsupported mode.
 */
enum class ModeType {
    WSPR,
    TONE,
    QRSS,
    UNKNOWN
};

/**
 * @brief Converts a string to its corresponding ModeType enumeration.
 *
 * Accepts a case-sensitive string view and returns the matching ModeType.
 * If the string does not match any known mode, ModeType::UNKNOWN is returned.
 *
 * @param str The input mode name as a string view (e.g., "WSPR", "QRSS").
 * @return The corresponding ModeType enum value.
 */
constexpr ModeType mode_from_string(std::string_view str) {
    if (str == "WSPR")  return ModeType::WSPR;
    if (str == "QRSS")  return ModeType::QRSS;
    if (str == "TONE")  return ModeType::TONE;
    return ModeType::UNKNOWN;
}

/**
 * @brief Converts a ModeType enumeration value to its string representation.
 *
 * Returns the string form of the given ModeType. If the mode is unknown,
 * the string "UNKNOWN" is returned.
 *
 * @param mode The ModeType enum value to convert.
 * @return A string view representing the mode (e.g., "WSPR", "QRSS").
 */
constexpr std::string_view mode_to_string(ModeType mode) {
    switch (mode) {
        case ModeType::WSPR:   return "WSPR";
        case ModeType::QRSS:   return "QRSS";
        case ModeType::TONE:   return "TONE";
        default:               return "UNKNOWN";
    }
}

/**
 * @enum QRSSMode
 * @brief Represents the modulation type used in QRSS transmissions.
 *
 * Defines the supported QRSS-related modes of operation.
 *
 * @var QRSSMode::QRSS
 * Traditional extremely slow-speed Morse code transmission.
 *
 * @var QRSSMode::FSKCW
 * Frequency Shift Keying CW mode, using two tones to encode bits.
 *
 * @var QRSSMode::DFCW
 * Dual Frequency CW mode, using alternate frequencies for dots and dashes.
 *
 * @var QRSSMode::UNKNOWN
 * Sentinel value for unrecognized or unsupported modes.
 */
enum class QRSSMode
{
    QRSS,  ///< Extreme slow speed CW
    FSKCW, ///< Frequency Shift Keying CW
    DFCW,  ///< Dual Frequency CW
    UNKNOWN,
};

/**
 * @brief Converts a string to a QRSSMode enumeration value.
 *
 * Maps a case-sensitive string to its corresponding QRSSMode. If the input does
 * not match any known mode, QRSSMode::UNKNOWN is returned.
 *
 * @param str The input string representing a QRSS mode (e.g., "QRSS", "FSKCW").
 * @return The corresponding QRSSMode enum value.
 */
constexpr QRSSMode qrss_mode_from_string(std::string_view str) {
    if (str == "QRSS")   return QRSSMode::QRSS;
    if (str == "FSKCW")  return QRSSMode::FSKCW;
    if (str == "DFCW")   return QRSSMode::DFCW;
    return QRSSMode::UNKNOWN;
}

/**
 * @brief Converts a QRSSMode enumeration value to its string representation.
 *
 * Converts the provided QRSSMode to a string view. Returns "UNKNOWN" if the
 * input is unrecognized.
 *
 * @param mode The QRSSMode enum value.
 * @return A string view corresponding to the mode name (e.g., "QRSS").
 */
constexpr std::string_view qrss_mode_to_string(QRSSMode mode) {
    switch (mode) {
        case QRSSMode::QRSS:   return "QRSS";
        case QRSSMode::FSKCW:  return "FSKCW";
        case QRSSMode::DFCW:   return "DFCW";
        default:               return "UNKNOWN";
    }
}

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
    ModeType mode; ///< Current operating mode.

    // Common
    std::string callsign;    ///< WSPR callsign.
    std::string grid_square; ///< 4- or 6-character Maidenhead locator.
    int power_dbm;           ///< Transmit power in dBm.
    std::string frequencies; ///< Space-separated frequency list.
    int tx_pin;              ///< GPIO pin number for RF transmit control.

    // QRSS
    QRSSMode qrss_mode;       ///< One of QRSS, FSKCW, DFCW
    int dot_length;           ///< Dot lngth in seconds
    double fsk_offset;        ///< Frequency shift offset in Hz
    double qrss_frequency;    ///< QRSS Frequency in Hz
    int tx_start_minute;      ///< Transmission start minute
    int tx_repeat_every;      ///< Repeat transmissions every N minutes
    std::string qrss_message; ///< Message to be transmitted

    // Extended
    double ppm;      ///< PPM frequency calibration.
    bool use_ntp;    ///< Apply NTP-based frequency correction.
    bool use_offset; ///< Enable random frequency offset.
    int power_level; ///< Power level for RF hardware (0–7).
    bool use_led;    ///< Enable TX LED indicator.
    int led_pin;     ///< GPIO pin for LED indicator.

    // Server
    int web_port;      ///< Web server port number.
    int socket_port;   ///< Socket server port number.
    bool use_shutdown; ///< Enable GPIO-based shutdown feature.
    int shutdown_pin;  ///< GPIO pin used to signal shutdown.

    // Command line only
    bool date_time_log;             ///< Prefix logs with timestamp.
    bool loop_tx;                   ///< Repeat transmission cycle.
    std::atomic<int> tx_iterations; ///< Number of transmission iterations (0 = infinite).
    double test_tone;               ///< Enable continuous tone mode (in Hz).

    // Runtime variables
    bool use_ini;                        ///< Load configuration from INI file.
    std::string ini_filename;            ///< INI file name and path.
    std::vector<double> center_freq_set; ///< Parsed list of center frequencies in Hz.
    bool ntp_good;                       ///< A more ualitative measurement of NTP vs simply running

    /**
     * @brief Default constructor initializing all configuration parameters.
     */
    ArgParserConfig()
        : transmit(true),
        mode(ModeType::WSPR),
        callsign(""),
        grid_square(""),
        power_dbm(0),
        frequencies(""),
        tx_pin(-1),
        qrss_mode(QRSSMode::QRSS),
        dot_length(10),
        fsk_offset(100),
        qrss_frequency(7039900.0),
        tx_start_minute(0),
        tx_repeat_every(10),
        qrss_message(""),
        ppm(0.0),
        use_ntp(false),
        use_offset(false),
        power_level(7),
        use_led(false),
        led_pin(-1),
        web_port(-1),
        socket_port(-1),
        use_shutdown(false),
        shutdown_pin(-1),
        date_time_log(false),
        loop_tx(false),
        tx_iterations(0),
        test_tone(0.0),
        use_ini(false),
        ini_filename(""),
        center_freq_set({}),
        ntp_good(false)
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
 * @brief Initializes the global configuration JSON object.
 *
 * @details
 * This function sets up a default configuration structure in the global
 * nlohmann::json object, `jConfig`. The JSON object is organized into several
 * sections: "Meta", "Common", "Control", "Extended", and "Server". Each section
 * contains key/value pairs that represent configuration parameters. In addition,
 * the "Center Frequency Set" under "Meta" is explicitly initialized as an empty array.
 *
 * @note The JSON values are stored as strings. Adjust the types as needed if numeric
 *       types are required in later processing.
 */
void init_config_json();

/**
 * @brief Patches the global JSON configuration with data from the INI file.
 *
 * @details
 * This function retrieves INI configuration data from the global INI handler object `ini`
 * and converts the data into a JSON object (named `patch`). Each INI section is converted
 * into a JSON object containing key/value pairs. It then adds the filename to the "Meta"
 * section under "INI Filename" and merges the resulting patch into the global JSON
 * configuration object `jConfig` using `merge_patch()`.
 *
 * If any exception is thrown while retrieving the INI data, the function catches the exception
 * and returns without modifying `jConfig`.
 *
 * @param filename The name of the INI file to record in the JSON configuration.
 */
void ini_to_json(std::string filename);

/**
 * @brief Parses configuration from a JSON object into an ArgParser struct.
 *
 * @param jConfig The JSON object containing configuration data.
 *
 * Expected JSON structure (example):
 * @code
 * {
 *   "Meta": {
 *       "Mode": "WSPR",
 *       "Use INI": true,
 *       "INI Filename": "",
 *       "Date Time Log": false,
 *       "Loop TX": true,
 *       "TX Iterations": 5,
 *       "Test Tone": 440.0,
 *       "Center Frequency Set": [ 12.2, 123.7, 98754.323 ]
 *   },
 *   "Control": {
 *       "Transmit": false
 *   },
 *   "Common": {
 *       "Call Sign": "NXXX",
 *       "Grid Square": "ZZ99",
 *       "TX Power": 20,
 *       "Frequency": "20m",
 *       "Transmit Pin": 4
 *   },
 *   "QRSS": {
 *      "Mode": "QRSS",
 *      "Dot Length":10,
 *      "FSK Offset": 100,
 *      "Frequency": 7039900.0,
 *      "TX Start Minute": tx_start_minute,
 *      "TX Repeat Every": tx_repeat_every,
 *      "Message": qrss_message,
 *   },
 *   "Extended": {
 *       "PPM": 0.0,
 *       "Use NTP": true,
 *       "Offset": true,
 *       "Use LED": false,
 *       "LED Pin": 18,
 *       "Power Level": 7
 *   },
 *   "Server": {
 *       "Web Port": 31415,
 *       "Web Port": 31416,
 *       "Use Shutdown": false,
 *       "Shutdown Button": 19
 *   }
 * }
 */
void json_to_config();

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
extern void config_to_json();

/**
 * @brief Saves the global JSON configuration back to the INI file.
 *
 * @details
 * If the configuration indicates that an INI file is being used (i.e. `config.use_ini`
 * is true), this function first updates the global JSON configuration by calling
 * `config_to_json()`. It then converts the JSON configuration (`jConfig`)
 * into an internal data structure (`newData`) suitable for the INI handler. Each
 * section in the JSON becomes a key in the map, with its value being an unordered map
 * of key/value pairs. If a JSON value is an array, it is converted to a string using
 * the `dump()` method; otherwise, the value is retrieved as a string.
 *
 * Finally, the new data is set into the global INI handler object (`ini`) using
 * `iniFile.setData(newData)` and saved to disk via `iniFile.save()`.
 *
 * @note This function assumes that all JSON values can be represented as strings.
 */
extern void json_to_ini();

/**
 * @brief Loads the global JSON configuration by merging default JSON and INI file data.
 *
 * @details
 * This function performs a three-step process:
 *  1. Calls `init_config_json()` to create a base JSON configuration with default values.
 *  2. Calls `ini_to_json(filename)` to overlay INI file data (from the given filename)
 *     onto the base JSON configuration.
 *  3. Calls `json_to_config()` to parse the updated JSON configuration into the global
 *     configuration structure (of type `ArgParser`).
 *
 * This layered approach allows default settings to be overridden by INI file values.
 *
 * @param filename The path to the INI file whose data will be merged into the JSON configuration.
 */
extern void load_json(std::string filename);

/**
 * @brief Prints a formatted JSON object to standard output.
 *
 * @details This function outputs the given JSON object to `std::cout` with
 *          an indent of 4 spaces and ensures key names are sorted.
 *          Useful for debugging or configuration output.
 *
 * @param j The JSON object to dump (will not be modified).
 *
 * @return void
 */
void dump_json(const nlohmann::json &j, std::string tag);

/**
 * @brief Applies a full patch update from incoming JSON.
 * @details This function receives a JSON object (typically from the web server),
 *          merges it into the current global JSON configuration (`jConfig`),
 *          updates the INI file and global config structure accordingly, and
 *          rebuilds the cleaned `jConfig` from the sanitized config values.
 *
 *          The flow is:
 *            1. Patch the input into `jConfig`.
 *            2. Update the INI file to reflect patched values.
 *            3. Update the config struct from patched values.
 *            4. Overwrite `jConfig` with sanitized config struct values.
 *            5. Dump final JSON (for debugging).
 *
 * @param j The incoming JSON object to patch into global configuration.
 *
 * @throws May throw exceptions from internal calls (e.g., parsing or write errors).
 */
void patch_all_from_web(const nlohmann::json &j);

#endif // _CONFIG_HANDLER_HPP
