/**
 * @file config_handler.cpp
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

#include "config_handler.hpp"

#include "ini_file.hpp"
#include "json.hpp"
#include "logging.hpp"

#include <iostream>
#include <string>
#include <vector>

/**
 * @brief Global INI handler instance.
 *
 * This instance of the IniFile class is used to load, save, and manage the
 * INI file configuration for the application.
 */
IniFile ini;

/**
 * @brief Global configuration object.
 *
 * This ArgParserConfig instance holds the application’s configuration settings,
 * typically loaded from an INI file or a JSON configuration.
 */
ArgParserConfig config;

/**
 * @brief Global JSON configuration object.
 *
 * This nlohmann::json object holds the application's configuration in JSON format.
 * It is used to merge and update configuration settings dynamically.
 */
nlohmann::json jConfig;

/**
 * @brief Parses a JSON value into a boolean.
 *
 * @details
 * This function checks if the given JSON value is a boolean or a string.
 * - If it is a boolean, it returns the boolean value directly.
 * - If it is a string, it converts it to lowercase and checks if it equals "true" or "1".
 * - If the JSON value is neither a boolean nor a string, the function returns false.
 *
 * @param j The JSON value to parse.
 * @return The boolean representation of the JSON value.
 */
bool parse_bool(const nlohmann::json &j)
{
    if (j.is_boolean())
        return j.get<bool>();

    if (j.is_string())
    {
        std::string s = j.get<std::string>();
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return (s == "true" || s == "1");
    }

    return false;
}

/**
 * @brief Parses a JSON value into a double.
 *
 * @details
 * This function checks if the given JSON value is a number or a string.
 * - If it is a number, it returns the double value directly.
 * - If it is a string, it attempts to convert the string to a double using std::stod.
 * - If the value is neither a number nor a string, the function throws a std::runtime_error.
 *
 * @param j The JSON value to parse.
 * @return The double representation of the JSON value.
 * @throws std::runtime_error if the JSON value cannot be converted to a double.
 */
double parse_double(const nlohmann::json &j)
{
    if (j.is_number())
    {
        return j.get<double>();
    }

    if (j.is_string())
    {
        return std::stod(j.get<std::string>());
    }

    throw std::runtime_error("Cannot convert JSON value to double.");
}

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
 * @endcode
 */
void json_to_config()
{
    // Meta
    std::string modeStr = jConfig["Meta"]["Mode"].get<std::string>();
    if (modeStr == "WSPR")
    {
        config.mode = ModeType::WSPR;
    }
    else if (modeStr == "TONE")
    {
        config.mode = ModeType::TONE;
    }
    else
    {
        // Handle invalid mode values
        config.mode = ModeType::WSPR; // Default
    }
    config.use_ini = parse_bool(jConfig["Meta"]["Use INI"]);
    config.ini_filename = jConfig["Meta"]["INI Filename"].get<std::string>();
    config.date_time_log = parse_bool(jConfig["Meta"]["Date Time Log"]);
    config.loop_tx = parse_bool(jConfig["Meta"]["Loop TX"]);
    config.tx_iterations = std::stoi(jConfig["Meta"]["TX Iterations"].get<std::string>());
    config.test_tone = parse_double(jConfig["Meta"]["Test Tone"]);
    config.center_freq_set = jConfig["Meta"]["Center Frequency Set"].get<std::vector<double>>();

    // Control
    config.transmit = parse_bool(jConfig["Control"]["Transmit"]);

    // Common
    config.callsign = jConfig["Common"]["Call Sign"].get<std::string>();
    config.grid_square = jConfig["Common"]["Grid Square"].get<std::string>();
    config.power_dbm = std::stoi(jConfig["Common"]["TX Power"].get<std::string>());
    config.frequencies = jConfig["Common"]["Frequency"].get<std::string>();
    config.tx_pin = std::stoi(jConfig["Common"]["Transmit Pin"].get<std::string>());

    // Extended
    config.ppm = parse_double(jConfig["Extended"]["PPM"]);
    config.use_ntp = parse_bool(jConfig["Extended"]["Use NTP"]);
    config.use_offset = parse_bool(jConfig["Extended"]["Offset"]);
    config.use_led = parse_bool(jConfig["Extended"]["Use LED"]);
    config.led_pin = std::stoi(jConfig["Extended"]["LED Pin"].get<std::string>());
    config.power_level = std::stoi(jConfig["Extended"]["Power Level"].get<std::string>());

    // Server
    config.web_port = std::stoi(jConfig["Server"]["Web Port"].get<std::string>());
    config.socket_port = std::stoi(jConfig["Server"]["Socket Port"].get<std::string>());
    config.use_shutdown = parse_bool(jConfig["Server"]["Use Shutdown"]);
    config.shutdown_pin = std::stoi(jConfig["Server"]["Shutdown Button"].get<std::string>());
}

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
void config_to_json()
{
    // Meta
    jConfig["Meta"]["Mode"] = "WSPR";
    if (config.mode == ModeType::TONE)
    {
        jConfig["Meta"]["Mode"] = "TONE";
    }
    jConfig["Meta"]["Use INI"] = config.use_ini ? "True" : "False";
    jConfig["Meta"]["INI Filename"] = config.ini_filename;
    jConfig["Meta"]["Date Time Log"] = config.date_time_log ? "True" : "False";
    jConfig["Meta"]["Loop TX"] = config.loop_tx ? "True" : "False";
    jConfig["Meta"]["TX Iterations"] = std::to_string(config.tx_iterations);
    jConfig["Meta"]["Test Tone"] = std::to_string(config.test_tone);
    jConfig["Meta"]["Center Frequency Set"] = config.center_freq_set;

    // Control
    jConfig["Control"]["Transmit"] = config.transmit ? "True" : "False";

    // Common
    jConfig["Common"]["Call Sign"] = config.callsign;
    jConfig["Common"]["Grid Square"] = config.grid_square;
    jConfig["Common"]["TX Power"] = std::to_string(config.power_dbm);
    jConfig["Common"]["Frequency"] = config.frequencies;
    jConfig["Common"]["Transmit Pin"] = std::to_string(config.tx_pin);

    // Extended
    jConfig["Extended"]["PPM"] = std::to_string(config.ppm);
    jConfig["Extended"]["Use NTP"] = config.use_ntp ? "True" : "False";
    jConfig["Extended"]["Offset"] = config.use_offset ? "True" : "False";
    jConfig["Extended"]["Use LED"] = config.use_led ? "True" : "False";
    jConfig["Extended"]["LED Pin"] = std::to_string(config.led_pin);
    jConfig["Extended"]["Power Level"] = std::to_string(config.power_level);

    // Server
    jConfig["Server"]["Web Port"] = std::to_string(config.web_port);
    jConfig["Server"]["Socket Port"] = std::to_string(config.socket_port);
    jConfig["Server"]["Use Shutdown"] = config.use_shutdown ? "True" : "False";
    jConfig["Server"]["Shutdown Button"] = std::to_string(config.shutdown_pin);
}

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
void init_config_json()
{
    // Meta section: General configuration settings
    jConfig["Meta"] = {
        {"Mode", "WSPR"},
        {"Use INI", "False"},
        {"INI Filename", ""},
        {"Date Time Log", "False"},
        {"Loop TX", "False"},
        {"TX Iterations", "0"},
        {"Test Tone", "730000"}};
    // Initialize "Center Frequency Set" as an empty JSON array
    jConfig["Meta"]["Center Frequency Set"] = nlohmann::json::array();

    // Common section: Settings related to communication parameters
    jConfig["Common"] = {
        {"Call Sign", "NXXX"},
        {"Frequency", "20m"},
        {"Grid Square", "ZZ99"},
        {"TX Power", "20"},
        {"Transmit Pin", "4"}};

    // Control section: Enable/disable controls
    jConfig["Control"] = {
        {"Transmit", "False"}};

    // Extended section: Additional configuration options
    jConfig["Extended"] = {
        {"LED Pin", "18"},
        {"Offset", "True"},
        {"PPM", "0.0"},
        {"Power Level", "7"},
        {"Use LED", "False"},
        {"Use NTP", "True"}};

    // Server section: Settings for server communication
    jConfig["Server"] = {
        {"Web Port", "31415"},
        {"Socket Port", "31416"},
        {"Shutdown Button", "19"},
        {"Use Shutdown", "False"}};
}

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
void ini_to_json(std::string filename)
{
    nlohmann::json patch;
    std::map<std::string, std::unordered_map<std::string, std::string>> ini_data;

    try
    {
        // Retrieve INI data from the global INI handler.
        ini_data = ini.getData();

        // Convert the INI data into a JSON patch.
        // Each section becomes a JSON object with key/value pairs.
        for (const auto &sectionPair : ini_data)
        {
            const std::string &section = sectionPair.first;
            const auto &keyValues = sectionPair.second;
            for (const auto &kv : keyValues)
            {
                patch[section][kv.first] = kv.second;
            }
        }
    }
    catch (const std::exception &ex)
    {
        // Optionally log the error: std::cerr << "Error: " << ex.what() << std::endl;
        return;
    }

    // Add the INI filename to the "Meta" section.
    patch["Meta"]["INI Filename"] = filename;

    // Merge the patch into the global JSON configuration.
    jConfig.merge_patch(patch);
}

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
 * `ini.setData(newData)` and saved to disk via `ini.save()`.
 *
 * @note This function assumes that all JSON values can be represented as strings.
 */
void json_to_ini()
{
    if (config.use_ini)
    {
        // Convert JSON back to INI data.
        std::map<std::string, std::unordered_map<std::string, std::string>> newData;
        for (auto &section : jConfig.items())
        {
            const std::string sectionName = section.key();
            // Only process sections that are JSON objects.
            if (section.value().is_object())
            {
                for (auto &kv : section.value().items())
                {
                    if (kv.value().is_array())
                    {
                        // For arrays, dump the JSON array to a string representation.
                        newData[sectionName][kv.key()] = kv.value().dump();
                    }
                    else
                    {
                        // Otherwise, retrieve the value as a string.
                        newData[sectionName][kv.key()] = kv.value().get<std::string>();
                    }
                }
            }
            else
            {
                // Optionally, handle non-object sections if needed.
            }
        }

        // Set the new data into the INI file object and save the changes.
        ini.setData(newData);
        ini.save();
    }
}

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
void load_json(std::string filename)
{
    // Create the base JSON configuration with default values.
    init_config_json();

    // Merge the INI file configuration into the base JSON.
    ini_to_json(filename);

    // Parse the updated JSON configuration into the global configuration struct.
    json_to_config();
}

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
void dump_json(const nlohmann::json &j, std::string tag = "")
{
    llog.logS(DEBUG, tag, "JSON Dump:", j.dump());
}

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
void patch_all_from_web(const nlohmann::json &j)
{
    dump_json(j, "Incoming");      ///< Debug: Show incoming JSON
    jConfig.merge_patch(j);        ///< Patch new values into the global JSON config
    dump_json(jConfig, "Patched"); ///< Debug: Show patched result
    json_to_ini();                 ///< Write patched config to INI
    json_to_config();              ///< Write patched config into global struct
    config_to_json();              ///< Rebuild jConfig from sanitized struct
    dump_json(jConfig, "Final");   ///< Debug: Show cleaned final JSON
}
