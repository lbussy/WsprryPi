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
const char* RPiVersion(int);
unsigned gpioBase();

bool isRaspbian() {
    std::ifstream file("/etc/os-release");
    if (!file.is_open()) {
        std::cerr << "Unable to open /etc/os-release" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Look for the "ID" field which should contain "raspbian"
        if (line.find("ID=raspbian") != std::string::npos) {
            return true;
        }
    }

    return false;
}

int ver()
{
    std::ifstream cpuinfo("/proc/cpuinfo"); // ifstream closes when out of scope
    if (!cpuinfo.is_open()) {
        std::cerr << "Unable to open /proc/cpuinfo" << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("Model") != std::string::npos || line.find("Hardware") != std::string::npos) {
            // Check the model directly from /proc/cpuinfo
            if (line.find("Raspberry Pi 1") != std::string::npos) return 1;
            if (line.find("Raspberry Pi 2") != std::string::npos) return 2;
            if (line.find("Raspberry Pi 3") != std::string::npos) return 3;
            if (line.find("Raspberry Pi 4") != std::string::npos) return 4;
            if (line.find("Raspberry Pi 5") != std::string::npos) return 5;
        }
    }

    return -1; // Return -1 if model is not found or unrecognized
}

const char* RPiVersion(int _ver)
{
    const char* vertext[7] = {
                "Unknown",
                "Raspberry Pi 1 or Zero Model (BCM2835)",
                "Raspberry Pi 2B (BCM2836)",
                "Raspberry Pi 2B or 3B (BCM2837)",
                "Raspberry Pi 4 (BCM2711)",
                "Raspberry Pi 5 (BCM2712)",
                "Unknown",
            };
    return vertext[ver()];
}

int getBitness() {
    if (sizeof(void*) == 4) {
        return 32;
    } else if (sizeof(void*) == 8) {
        return 64;
    } else {
        return 0;
    }
}

unsigned gpioBase() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        std::cerr << "Unable to open /proc/cpuinfo" << std::endl;
        return 0;
    }

    std::string line;
    while (std::getline(cpuinfo, line)) {
        // Look for the "Model" line in /proc/cpuinfo to identify the Raspberry Pi model
        if (line.find("Model") != std::string::npos) {
            // Identify the model and return the corresponding peripheral base address
            if (line.find("Raspberry Pi 5") != std::string::npos) {
                return 0x3F000000;  // Raspberry Pi 5 (BCM2712) peripheral base address
            } else if (line.find("Raspberry Pi 4") != std::string::npos) {
                return 0x3F000000;  // Raspberry Pi 4 (BCM2711) peripheral base address
            } else if (line.find("Raspberry Pi 3") != std::string::npos) {
                return 0x3F000000;  // Raspberry Pi 3 (BCM2837) peripheral base address
            } else if (line.find("Raspberry Pi 2") != std::string::npos) {
                return 0x3F000000;  // Raspberry Pi 2 (BCM2836) peripheral base address
            } else if (line.find("Raspberry Pi") != std::string::npos) {
                return 0x20000000;  // Raspberry Pi 1 (BCM2835) peripheral base address
            }
        }
    }

    std::cerr << "Unable to determine Raspberry Pi model" << std::endl;
    return 0;  // Default to 0 if model not found
}

#endif // _VERSION_H
