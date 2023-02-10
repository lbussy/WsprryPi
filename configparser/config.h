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
    bool getreset()
    {
        return reset;
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
    std::string getFrequency1()
    {
        return frequency1;
    }
    std::string getFrequency2()
    {
        return frequency2;
    }
    std::string getFrequency3()
    {
        return frequency3;
    }
    std::string getFrequency4()
    {
        return frequency4;
    }
    std::string getFrequency5()
    {
        return frequency5;
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
            reset = reader.GetBoolean("Control", "Reset", false);

            // Common group
            callsign = reader.Get("Common", "Call Sign", "");
            gridsquare = reader.Get("Common", "Grid Square", "");;
            txpower = reader.GetInteger("Common", "TX Power", 0);
            frequency1 = reader.Get("Common", "Frequency", "");

            // Extended Group
            frequency2 = reader.Get("Extended", "Frequency 2", "");;
            frequency3 = reader.Get("Extended", "Frequency 3", "");;
            frequency4 = reader.Get("Extended", "Frequency 4", "");;
            frequency5 = reader.Get("Extended", "Frequency 5", "");;
            ppm = reader.GetReal("Extended", "PPM", 0.0);;
            freerun = reader.GetBoolean("Extended", "Free Run", false);;
            offset = reader.GetBoolean("Extended", "Offset", false);;
        }
    }

    bool isinitialized;
    // Control group
    bool transmit;
    bool reset;
    // Common group
    std::string callsign;
    std::string gridsquare;
    int txpower;
    // Extended group
    std::string frequency1;
    std::string frequency2;
    std::string frequency3;
    std::string frequency4;
    std::string frequency5;
    double ppm;
    bool freerun;
    bool offset;
};
