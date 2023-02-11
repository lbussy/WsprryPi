#include <iostream>
#include <fstream>
#include <cstring>
#include "INIReader.h"
#include <string>

class WSPRConfig
{
public:
    WSPRConfig(const std::string &configFile)
    {
        valueHandler(configFile.c_str());
    }
    bool isInitialized()
    {
        return isinitialized;
    }
    bool getTransmit()
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
    int getTxpower()
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
    bool getFreerun()
    {
        return freerun;
    }
    bool getOffset()
    {
        return offset;
    }
    bool useLED()
    {
        return useled;
    }
    bool useDaemon()
    {
        return usedaemon;
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

            // Common group
            callsign = reader.Get("Common", "Call Sign", "");
            gridsquare = reader.Get("Common", "Grid Square", "");;
            txpower = reader.GetInteger("Common", "TX Power", 0);
            frequency = reader.Get("Common", "Frequency", "");

            // Extended Group
            ppm = reader.GetReal("Extended", "PPM", 0.0);;
            freerun = reader.GetBoolean("Extended", "Free Run", false);;
            offset = reader.GetBoolean("Extended", "Offset", false);;
            useled = reader.GetBoolean("Extended", "Use LED", false);;
            usedaemon = reader.GetBoolean("Extended", "Use Daemon", false);;
        }
    }

    bool isinitialized;
    // Control group
    bool transmit;
    // Common group
    std::string callsign;
    std::string gridsquare;
    int txpower;
    std::string frequency;
    // Extended group
    double ppm;
    bool freerun;
    bool offset;
    bool useled;
    bool usedaemon;
};
