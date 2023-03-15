#ifndef _CONFIG_H
#define _CONFIG_H
#pragma once

#include <iostream>
#include <fstream>
#include <cstring>
#include "INIReader.h"
#include <string>

class WSPRConfig
{
public:
    WSPRConfig(){}
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
