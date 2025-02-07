/**
 * @file config.cpp
 * @brief Defines the WSPRConfig class for reading and managing configuration
 * data from an INI file.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is distributed under under
 * the MIT License. See LICENSE.MIT.md for more information.
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

// Unit tests:
// g++ -Wall -Werror -fmax-errors=5 -static-libgcc -Wno-psabi -lstdc++fs -std=c++17 -DDEBUG_MAIN_CONFIG config.cpp ini_reader.cpp ini.c -o config
// Test command:
// ./config wsprrypi.ini

#include "config.hpp"

/**
 * @brief Gets the transmission status.
 * @return True if transmission is enabled, false otherwise.
 */
bool WSPRConfig::getTransmit() const { return transmit; }

/**
 * @brief Gets the repeat transmission status.
 * @return True if repeat transmissions are enabled, false otherwise.
 */
bool WSPRConfig::getRepeat() const { return repeat; }

/**
 * @brief Gets the configured callsign.
 * @return The callsign as a string.
 */
std::string WSPRConfig::getCallsign() const { return callsign; }

/**
 * @brief Gets the configured grid square.
 * @return The grid square as a string.
 */
std::string WSPRConfig::getGridsquare() const { return gridsquare; }

/**
 * @brief Gets the configured transmission power.
 * @return The transmission power as a string.
 */
std::string WSPRConfig::getTxpower() const { return txpower; }

/**
 * @brief Gets the configured transmission frequency.
 * @return The frequency as a string.
 */
std::string WSPRConfig::getFrequency() const { return frequency; }

/**
 * @brief Gets the PPM correction value.
 * @return The PPM correction value as a double.
 */
double WSPRConfig::getPpm() const { return ppm; }

/**
 * @brief Gets the self-calibration status.
 * @return True if self-calibration is enabled, false otherwise.
 */
bool WSPRConfig::getSelfcal() const { return selfcal; }

/**
 * @brief Gets the offset setting status.
 * @return True if the offset setting is enabled, false otherwise.
 */
bool WSPRConfig::getOffset() const { return offset; }

/**
 * @brief Gets the LED usage status.
 * @return True if the LED is enabled, false otherwise.
 */
bool WSPRConfig::getUseLED() const { return use_led; }

/**
 * @brief Gets the power level setting.
 * @return The power level as an integer.
 */
int WSPRConfig::getPowerLevel() const { return power_level; }

/**
 * @brief Gets the server port.
 * @return The server port as an integer.
 */
int WSPRConfig::getServerPort() const { return port; }

/**
 * @brief Sets the transmission status.
 * @param value True to enable transmissions, false to disable.
 */
void WSPRConfig::setTransmit(bool value) { transmit = value; }

/**
 * @brief Sets the repeat transmission status.
 * @param value True to enable repeat transmissions, false to disable.
 */
void WSPRConfig::setRepeat(bool value) { repeat = value; }

/**
 * @brief Sets the callsign.
 * @param value The new callsign.
 */
void WSPRConfig::setCallsign(const std::string& value) { callsign = value; }

/**
 * @brief Sets the grid square.
 * @param value The new grid square.
 */
void WSPRConfig::setGridsquare(const std::string& value) { gridsquare = value; }

/**
 * @brief Sets the transmission power.
 * @param value The new transmission power.
 */
void WSPRConfig::setTxpower(const std::string& value) { txpower = value; }

/**
 * @brief Sets the transmission frequency.
 * @param value The new transmission frequency.
 */
void WSPRConfig::setFrequency(const std::string& value) { frequency = value; }

/**
 * @brief Sets the PPM correction value.
 * @param value The new PPM correction value.
 */
void WSPRConfig::setPpm(double value) { ppm = value; }

/**
 * @brief Sets the self-calibration status.
 * @param value True to enable self-calibration, false to disable.
 */
void WSPRConfig::setSelfcal(bool value) { selfcal = value; }

/**
 * @brief Sets the offset setting status.
 * @param value True to enable the offset setting, false to disable.
 */
void WSPRConfig::setOffset(bool value) { offset = value; }

/**
 * @brief Sets the LED usage status.
 * @param value True to enable the LED, false to disable.
 */
void WSPRConfig::setUseLED(bool value) { use_led = value; }

/**
 * @brief Sets the power level.
 * @param value The new power level.
 */
void WSPRConfig::setPowerLevel(int value) { power_level = value; }

/**
 * @brief Sets the server port.
 * @param value The new server port number.
 */
void WSPRConfig::setServerPort(int value) { port = value; }

#ifdef DEBUG_MAIN_CONFIG

#include <iostream>

/**
 * @brief Prints the configuration values to the console.
 * @param config The WSPRConfig instance containing the configuration.
 */
void printConfig(const WSPRConfig& config)
{
    std::cout << "Configuration Loaded:\n";
    std::cout << "  Transmit: " << (config.getTransmit() ? "Yes" : "No") << "\n";
    std::cout << "  Repeat: " << (config.getRepeat() ? "Yes" : "No") << "\n";
    std::cout << "  Callsign: " << config.getCallsign() << "\n";
    std::cout << "  Grid Square: " << config.getGridsquare() << "\n";
    std::cout << "  TX Power: " << config.getTxpower() << "\n";
    std::cout << "  Frequency: " << config.getFrequency() << "\n";
    std::cout << "  PPM: " << config.getPpm() << "\n";
    std::cout << "  Self Cal: " << (config.getSelfcal() ? "Yes" : "No") << "\n";
    std::cout << "  Offset: " << (config.getOffset() ? "Yes" : "No") << "\n";
    std::cout << "  Use LED: " << (config.getUseLED() ? "Yes" : "No") << "\n";
    std::cout << "  Power Level: " << config.getPowerLevel() << "\n";
}

/**
 * @brief Main function for unit testing the WSPRConfig class.
 *
 * This function initializes a WSPRConfig instance using a configuration
 * file specified as a command-line argument. It prints the configuration
 * values to the console for verification.
 *
 * @param argc The argument count.
 * @param argv The argument values.
 * @return Exit status.
 */
int main(int argc, const char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    std::string configFile = argv[1];

    std::cout << "Testing WSPRConfig with configuration file: " << configFile << "\n";

    WSPRConfig config;
    if (config.initialize(configFile)) {
        printConfig(config);
    } else {
        std::cerr << "Failed to initialize configuration.\n";
        return 1;
    }

    std::cout << "WSPRConfig test completed successfully.\n";
    return 0;
}

#endif // DEBUG_MAIN_CONFIG
