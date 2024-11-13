#ifndef _VERSION_H
#define _VERSION_H
#pragma once

#include <fstream>

#define stringify(s) _stringifyDo(s)
#define _stringifyDo(s) #s

// const char *project() { return stringify(MAKE_SRC_NAM); }
const char *exeversion() { return stringify(MAKE_SRC_TAG); }
// const char *commit() { return stringify(MAKE_SRC_REV); }
const char *branch() { return stringify(MAKE_SRC_BRH); }

int ver();
const char* RPiVersion();
unsigned gpioBase();

int ver()
{
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        std::cerr << "Unable to open /proc/cpuinfo" << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("Model") != std::string::npos || line.find("Hardware") != std::string::npos) {
            // Check the model name directly
            if (line.find("Raspberry Pi 1") != std::string::npos) return 0;
            if (line.find("Raspberry Pi 2") != std::string::npos) return 1;
            if (line.find("Raspberry Pi 3") != std::string::npos) return 2;
            if (line.find("Raspberry Pi 4") != std::string::npos) return 3;
            if (line.find("Raspberry Pi 5") != std::string::npos) return 4;
            // Additional models can be added here if needed
        }
    }

    cpuinfo.close();
    return -1; // Return -1 if model is not found or unrecognized
}

const char* RPiVersion()
{
    const char* vertext[5] = {
                "Raspberry Pi 1 or Zero Model (BCM2835)",
                "Raspberry Pi 2B (BCM2836)",
                "Raspberry Pi 2B or 3B (BCM2837)",
                "Raspberry Pi 4 (BCM2711)",
                "Raspberry Pi 5 (BCM2712)"
            };
    return vertext[ver()];
}

unsigned gpioBase()
{
    //return bcm_host_get_peripheral_address();
    return 0x40000000; // DEBUG to get past this, bcm_host is dead
}

#endif // _VERSION_H
