// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file config.cpp
 * @brief Provides unit tests for the WSPRConfig class.
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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-1a53e0c [logfile_wrap].
 */

// Unit tests:
// g++ -Wall -Werror -fmax-errors=5 -static-libgcc -Wno-psabi -lstdc++fs -std=c++17 -DDEBUG_MAIN_CONFIG config.cpp ini_reader.cpp ini.c -o config
// Test command:
// ./config wspr.ini

#ifdef DEBUG_MAIN_CONFIG

#include <iostream>
#include "config.hpp"

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
int main(int argc, char* argv[])
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
