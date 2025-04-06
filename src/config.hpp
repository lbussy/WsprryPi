#ifndef _CONFIG_H
#define _CONFIG_H
#pragma once

// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/*
 * WsprryPi
 * Updated and maintained by Lee C. Bussy
 *
 * Originally forked from threeme3/WsprryPi (no longer active), this
 * project has been significantly updated, improved, and documented for
 * ease of use.a
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
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.2-babbc84 [current_dev].
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include "INIReader.h"
#include <string>

class WSPRConfig
{
public:
    WSPRConfig() {}
    bool initialize(const std::string &configFile)
    {
        valueHandler(configFile.c_str());
        return isinitialized;
    }
    bool getTransmit()
    {
        return transmit;
    }
    bool getRepeat()
    {
        return transmit;
    }
    std::string getCallsign()
    {
        return callsign;
    }
    std::string getGridsquare()
    {
        return gridsquare;
    }
    std::string getTxpower()
    {
        return txpower;
    }
    std::string getFrequency()
    {
        return frequency;
    }
    double getPpm()
    {
        return ppm;
    }
    bool getSelfcal()
    {
        return selfcal;
    }
    bool getOffset()
    {
        return offset;
    }
    bool getUseLED()
    {
        return use_led;
    }
    int getPowerLevel()
    {
        return power_level;
    }

private:
    void valueHandler(const char *configfile)
    {
        INIReader reader(configfile);

        if (reader.ParseError() != 0)
        {
            std::cerr << "Can't load '" << configfile << "'\n";
            isinitialized = false;
        }
        else
        {
            isinitialized = true;
        }
        if (isinitialized)
        {
            // Control group
            transmit = reader.GetBoolean("Control", "Transmit", false);
            repeat = reader.GetBoolean("Control", "Repeat", false);

            // Common group
            callsign = reader.Get("Common", "Call Sign", "");
            gridsquare = reader.Get("Common", "Grid Square", "");
            txpower = std::to_string(reader.GetInteger("Common", "TX Power", 0));
            frequency = reader.Get("Common", "Frequency", "");

            // Extended Group
            ppm = reader.GetReal("Extended", "PPM", 0.0);
            selfcal = reader.GetBoolean("Extended", "Self Cal", false);
            offset = reader.GetBoolean("Extended", "Offset", false);
            use_led = reader.GetBoolean("Extended", "Use LED", false);
            power_level = reader.GetInteger("Extended", "Power Level", 7);
        }
    }

    bool isinitialized;
    // Control group
    bool transmit;
    bool repeat;
    // Common group
    std::string callsign;
    std::string gridsquare;
    std::string txpower;
    std::string frequency;
    // Extended group
    double ppm;
    bool selfcal;
    bool offset;
    bool use_led;
    int power_level;
};

#endif // _CONFIG_H
