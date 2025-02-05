/**
 * @file version.cpp
 * @brief Unit tests for versioning and Raspberry Pi-specific hardware information.
 *
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 *
 * Copyright (C) @threeme3 (unknown dates)
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * WsprryPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

// Unit testing:
/*
    g++ -Wall -Werror -fmax-errors=5 -static-libgcc -Wno-psabi -lstdc++fs -std=c++17 \
        -DDEBUG_MAIN_VERSION \
        -DMAKE_TAG="1.2.1-abc123" \
        -DMAKE_BRH="main" \
        -DMAKE_EXE="wsprrypi" \
        -DMAKE_PRJ="WsprryPi" \
        version.cpp -o version -lbcm_host
*/
// Test command:
// ./version

#ifdef DEBUG_MAIN_VERSION

#include <iostream>
#include <iomanip> // For std::hex and std::dec
#include "version.hpp"

/**
 * @brief Main function to test versioning and Raspberry Pi hardware information.
 *
 * This function performs unit tests for the following features:
 * - Executable and project metadata (version, branch, executable name, project name).
 * - Raspberry Pi-specific hardware information (processor ID, version, GPIO base address).
 * - Version string construction.
 *
 * @return Exit status (0 for success, non-zero for failure).
 */
int main()
{
    std::cout << "===========================\n";
    std::cout << "Testing Version Functions...\n";
    std::cout << "===========================\n";

    // Test executable and branch macros
    std::cout << "Project Name: " << project_name() << "\n";
    std::cout << "Executable Name: " << exe_name() << "\n";
    std::cout << "Executable Version: " << exe_version() << "\n";
    std::cout << "Branch: " << branch() << "\n";

    // Test full version string
    std::cout << "Full Version String: " << version_string() << "\n";

    // Test Raspberry Pi-specific hardware information
    int processorId = ver();
    std::cout << "\nHardware Information:\n";
    std::cout << "Processor ID: " << processorId << "\n";

    const char* piVersion = RPiVersion();
    std::cout << "Raspberry Pi Version: " << piVersion << "\n";

    unsigned gpioBaseAddr = gpioBase();
    std::cout << "GPIO Base Address: 0x" << std::hex << gpioBaseAddr << std::dec << "\n";

    // End of tests
    std::cout << "\n===========================\n";
    std::cout << "Version function tests completed successfully.\n";
    std::cout << "===========================\n";

    return 0;
}

#endif // DEBUG_MAIN_VERSION
