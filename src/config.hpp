/**
 * @file config.hpp
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

#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include "ini_reader.hpp"

/**
 * @class WSPRConfig
 * @brief Handles loading and managing WSPR configuration data from an INI file.
 */
class WSPRConfig {
public:
    /**
     * @brief Default constructor for WSPRConfig.
     */
    WSPRConfig() : isinitialized(false), transmit(false), repeat(false), ppm(0.0),
                   selfcal(false), offset(false), use_led(false), power_level(7),
                   port(31415) {}

    /**
     * @brief Initializes the configuration by loading values from the given INI file.
     * @param configFile Path to the INI configuration file.
     * @return True if initialization succeeds, false otherwise.
     */
    bool initialize(const std::string& configFile)
    {
        valueHandler(configFile.c_str());
        return isinitialized;
    }

    /** @name Getters */
    ///@{
    bool getTransmit() const;          ///< Gets transmission status.
    bool getRepeat() const;            ///< Gets repeat transmission status.
    std::string getCallsign() const;   ///< Gets the configured callsign.
    std::string getGridsquare() const; ///< Gets the configured grid square.
    std::string getTxpower() const;    ///< Gets the configured transmission power.
    std::string getFrequency() const;  ///< Gets the configured transmission frequency.
    double getPpm() const;             ///< Gets the PPM correction value.
    bool getSelfcal() const;           ///< Gets self-calibration status.
    bool getOffset() const;            ///< Gets offset setting status.
    bool getUseLED() const;            ///< Gets LED usage status.
    int getPowerLevel() const;         ///< Gets the power level setting.
    int getServerPort() const;         ///< Gets the server port configuration.
    ///@}

    /** @name Setters */
    ///@{
    void setTransmit(bool value);          ///< Sets transmission status.
    void setRepeat(bool value);            ///< Sets repeat transmission status.
    void setCallsign(const std::string& value); ///< Sets the callsign.
    void setGridsquare(const std::string& value); ///< Sets the grid square.
    void setTxpower(const std::string& value);    ///< Sets the transmission power.
    void setFrequency(const std::string& value);  ///< Sets the transmission frequency.
    void setPpm(double value);              ///< Sets the PPM correction value.
    void setSelfcal(bool value);            ///< Sets self-calibration status.
    void setOffset(bool value);             ///< Sets offset setting status.
    void setUseLED(bool value);             ///< Sets LED usage status.
    void setPowerLevel(int value);          ///< Sets the power level setting.
    void setServerPort(int value);          ///< Sets the server port configuration.
    ///@}

private:
    /**
     * @brief Handles loading configuration values from the INI file.
     * @param configfile Path to the INI configuration file.
     */
    void valueHandler(const char* configfile)
    {
        INIReader reader(configfile);

        if (reader.ParseError() != 0) {
            std::cerr << "Can't load '" << configfile << "'\n";
            isinitialized = false;
        } else {
            isinitialized = true;
        }

        if (isinitialized) {
            // Control group
            transmit = reader.GetBoolean("Control", "Transmit", false);
            repeat = reader.GetBoolean("Control", "Repeat", false);

            // Common group
            callsign = reader.Get("Common", "Call Sign", "");
            gridsquare = reader.Get("Common", "Grid Square", "");
            txpower = std::to_string(reader.GetInteger("Common", "TX Power", 0));
            frequency = reader.Get("Common", "Frequency", "");

            // Extended group
            ppm = reader.GetReal("Extended", "PPM", 0.0);
            selfcal = reader.GetBoolean("Extended", "Self Cal", false);
            offset = reader.GetBoolean("Extended", "Offset", false);
            use_led = reader.GetBoolean("Extended", "Use LED", false);
            power_level = reader.GetInteger("Extended", "Power Level", 7);
            port = reader.GetInteger("Server", "Port", 31415);
        }
    }

    bool isinitialized;       ///< Indicates whether the configuration was successfully initialized.
    bool transmit;            ///< Indicates whether transmissions are enabled.
    bool repeat;              ///< Indicates whether repeat transmissions are enabled.
    std::string callsign;     ///< The configured callsign.
    std::string gridsquare;   ///< The configured grid square.
    std::string txpower;      ///< The configured transmission power.
    std::string frequency;    ///< The configured transmission frequency.
    double ppm;               ///< The PPM correction value.
    bool selfcal;             ///< Indicates whether self-calibration is enabled.
    bool offset;              ///< Indicates whether the offset setting is enabled.
    bool use_led;             ///< Indicates whether the LED is enabled.
    int power_level;          ///< The configured power level.
    int port;                 ///< The configured server port.
};

#endif // CONFIG_H
