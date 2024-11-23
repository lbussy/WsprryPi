// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file config.hpp
 * @brief Defines the WSPRConfig class for reading and managing configuration data from an INI file.
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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
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
                   selfcal(false), offset(false), use_led(false), power_level(7) {}

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

    /**
     * @brief Checks whether transmissions are enabled.
     * @return True if transmission is enabled, false otherwise.
     */
    bool getTransmit() const { return transmit; }

    /**
     * @brief Checks whether repeat transmissions are enabled.
     * @return True if repeat transmissions are enabled, false otherwise.
     */
    bool getRepeat() const { return repeat; }

    /**
     * @brief Gets the configured callsign.
     * @return The callsign as a string.
     */
    std::string getCallsign() const { return callsign; }

    /**
     * @brief Gets the configured grid square.
     * @return The grid square as a string.
     */
    std::string getGridsquare() const { return gridsquare; }

    /**
     * @brief Gets the configured transmission power.
     * @return The transmission power as a string.
     */
    std::string getTxpower() const { return txpower; }

    /**
     * @brief Gets the configured transmission frequency.
     * @return The frequency as a string.
     */
    std::string getFrequency() const { return frequency; }

    /**
     * @brief Gets the PPM correction value.
     * @return The PPM correction value as a double.
     */
    double getPpm() const { return ppm; }

    /**
     * @brief Checks whether self-calibration is enabled.
     * @return True if self-calibration is enabled, false otherwise.
     */
    bool getSelfcal() const { return selfcal; }

    /**
     * @brief Checks whether the offset setting is enabled.
     * @return True if the offset setting is enabled, false otherwise.
     */
    bool getOffset() const { return offset; }

    /**
     * @brief Checks whether the LED is enabled.
     * @return True if the LED is enabled, false otherwise.
     */
    bool getUseLED() const { return use_led; }

    /**
     * @brief Gets the power level setting.
     * @return The power level as an integer.
     */
    int getPowerLevel() const { return power_level; }

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
};

#endif // CONFIG_H
