// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file ini_reader.cpp
 * @brief Implementation of the INIReader class for reading and managing INI files.
 *
 * WsprryPi
 * Updated and maintained by Lee C. Bussy
 *
 * Originally forked from threeme3/WsprryPi (no longer active), this
 * project has been significantly updated, improved, and documented for
 * ease of use.
 *
 * Inspired by a conversation with Bruce Raymond of TAPR, this fork has
 * diverged substantially from its origins and operates as an independent
 * project.
 *
 * Contributors:
 *   - threeme3 (Original Author)
 *   - Bruce Raymond (Inspiration and Guidance)
 *   - Lee Bussy, aa0nt@arrl.net
 *
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-7f4c707 [refactoring].
 */

// Unit testing:
// g++ -Wall -Werror -fmax-errors=5 -std=c++17 -DDEBUG_MAIN_READER ini_reader.cpp ini.c -o ini_reader
// Test command:
// ./ini_reader wspr.ini

#include "ini_reader.hpp"
#include "ini.h"
#include <algorithm>  // For std::transform
#include <cctype>     // For ::tolower
#include <iostream>
#include <fstream>

/**
 * @brief Constructs an INIReader and parses the specified file.
 *
 * If the file does not exist, it is created with default values and comments.
 *
 * @param filename The path to the INI file to parse.
 */
INIReader::INIReader(const std::string& filename) {
    std::ifstream file_check(filename);
    if (!file_check) {
        CreateDefaultINI(filename);  // Create the file with default values
    }
    file_check.close();
    _error = ini_parse(filename.c_str(), ValueHandler, this);  // Parse the INI file
}

/**
 * @brief Constructs an INIReader using an open file stream.
 *
 * Parses the INI data from the provided FILE pointer.
 *
 * @param file Pointer to the open FILE stream containing the INI data.
 */
INIReader::INIReader(FILE* file) {
    _error = ini_parse_file(file, ValueHandler, this);
}

/**
 * @brief Retrieves the parsing error state.
 *
 * @return 0 if parsing succeeded, a positive line number for the first parse error, or -1 if the file could not be opened.
 */
int INIReader::ParseError() const {
    return _error;
}

/**
 * @brief Gets all sections found in the INI file.
 *
 * @return A constant reference to a set of section names.
 */
const std::set<std::string>& INIReader::Sections() const {
    return _sections;
}

/**
 * @brief Retrieves a value as a string.
 *
 * @param section The section name.
 * @param name The key name.
 * @param default_value The default value if the key is not found.
 * @return The value associated with the key or the default value if the key is not found.
 */
std::string INIReader::Get(const std::string& section, const std::string& name, const std::string& default_value) const {
    std::string key = MakeKey(section, name);
    return _values.count(key) ? _values.at(key) : default_value;
}

/**
 * @brief Retrieves a value as an integer.
 *
 * @param section The section name.
 * @param name The key name.
 * @param default_value The default value if the key is not found or invalid.
 * @return The integer value or the default value if the key is not found or invalid.
 */
long INIReader::GetInteger(const std::string& section, const std::string& name, long default_value) const {
    std::string valstr = Get(section, name, "");
    const char* value = valstr.c_str();
    char* end;
    long n = strtol(value, &end, 0);
    return end > value ? n : default_value;
}

/**
 * @brief Retrieves a value as a double.
 *
 * @param section The section name.
 * @param name The key name.
 * @param default_value The default value if the key is not found or invalid.
 * @return The double value or the default value if the key is not found or invalid.
 */
double INIReader::GetReal(const std::string& section, const std::string& name, double default_value) const {
    std::string valstr = Get(section, name, "");
    const char* value = valstr.c_str();
    char* end;
    double n = strtod(value, &end);
    return end > value ? n : default_value;
}

/**
 * @brief Retrieves a value as a boolean.
 *
 * @param section The section name.
 * @param name The key name.
 * @param default_value The default value if the key is not found or invalid.
 * @return True or False based on the parsed value or the default value if invalid.
 */
bool INIReader::GetBoolean(const std::string& section, const std::string& name, bool default_value) const {
    std::string valstr = Get(section, name, "");
    std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
    if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1") {
        return true;
    }
    if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0") {
        return false;
    }
    return default_value;
}

/**
 * @brief Checks if a key exists in a section.
 *
 * @param section The section name.
 * @param name The key name.
 * @return True if the key exists, otherwise False.
 */
bool INIReader::KeyExists(const std::string& section, const std::string& name) const {
    return _values.find(MakeKey(section, name)) != _values.end();
}

/**
 * @brief Combines section and name into a lowercase key.
 *
 * Used internally for creating keys for the internal `_values` map.
 *
 * @param section The section name.
 * @param name The key name.
 * @return A combined lowercase key string.
 */
std::string INIReader::MakeKey(const std::string& section, const std::string& name) {
    std::string key = section + "=" + name;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    return key;
}

/**
 * @brief Processes section, key, and value during INI parsing.
 *
 * Handles each section, key, and value encountered by the parser.
 *
 * @param user Pointer to the INIReader instance.
 * @param section The section name being parsed.
 * @param name The key name being parsed.
 * @param value The associated value.
 * @return 1 on success, otherwise 0.
 */
int INIReader::ValueHandler(void* user, const char* section, const char* name, const char* value) {
    INIReader* reader = reinterpret_cast<INIReader*>(user);
    std::string key = reader->MakeKey(section, name);
    if (!reader->_values[key].empty()) {
        reader->_values[key] += "\n";  // Handle multi-line values
    }
    reader->_values[key] += value;
    reader->_sections.insert(section);
    return 1;
}

/**
 * @brief Creates a new INI file with default values and comments.
 *
 * @param filename The name of the INI file to create.
 */
void INIReader::CreateDefaultINI(const std::string& filename) {
    std::ofstream ini_file(filename);
    if (ini_file.is_open()) {
        ini_file << "# Configuration file for WSPR program\n";
        ini_file << "# This file was automatically created because it was missing.\n";
        ini_file << "# Edit the values below as needed. Do not remove sections or keys.\n\n";

        ini_file << "[Control]\n";
        ini_file << "# Transmit: Set to True to enable transmitting, False to disable.\n";
        ini_file << "Transmit = False\n\n";

        ini_file << "[Common]\n";
        ini_file << "# Call Sign: Your ham radio call sign (maximum 7 characters).\n";
        ini_file << "Call Sign = NXXX\n";
        ini_file << "# Grid Square: Your location's Maidenhead grid square (4 characters).\n";
        ini_file << "Grid Square = ZZ99\n";
        ini_file << "# TX Power: Transmitter power in dBm (integer, e.g., 20).\n";
        ini_file << "TX Power = 20\n";
        ini_file << "# Frequency: Transmission frequency in meters (e.g., '20m') or Hz.\n";
        ini_file << "Frequency = 20m\n\n";

        ini_file << "[Extended]\n";
        ini_file << "# PPM: Frequency offset in parts per million.\n";
        ini_file << "PPM = 0.0\n";
        ini_file << "# Self Cal: Set to True to enable self-calibration.\n";
        ini_file << "Self Cal = True\n";
        ini_file << "# Offset: Set to True to enable frequency offset correction.\n";
        ini_file << "Offset = False\n";
        ini_file << "# Use LED: Set to True to enable LED usage.\n";
        ini_file << "Use LED = False\n";
        ini_file << "# Power Level: Output power level (integer from 0 to 7).\n";
        ini_file << "Power Level = 7\n";

        ini_file.close();
        std::cout << "INI file created with default values and comments: " << filename << std::endl;
    } else {
        std::cerr << "Error: Unable to create INI file '" << filename << "'." << std::endl;
    }
}

#ifdef DEBUG_MAIN_READER
#include <iostream>

/**
 * @brief Main function for unit testing the INIReader class.
 *
 * @return Exit status.
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ini-file>" << std::endl;
        return 1;
    }

    const char* ini_file = argv[1];  // Get the INI file name from the command line argument
    INIReader reader(ini_file);

    if (reader.ParseError() != 0) {
        std::cerr << "Error parsing INI file '" << ini_file << "'." << std::endl;
        return 1;
    }

    std::cout << "Sections in the INI file:" << std::endl;
    for (const auto& section : reader.Sections()) {
        std::cout << "[" << section << "]" << std::endl;
        for (const auto& key : {"Transmit", "Call Sign", "Grid Square", "TX Power", "Frequency", "PPM", "Self Cal", "Offset", "Use LED", "Power Level"}) {
            if (reader.KeyExists(section, key)) {
                std::cout << "  " << key << ": " << reader.Get(section, key, "<missing>") << std::endl;
            }
        }
    }

    return 0;
}
#endif
