// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file version.cpp
 * @brief Unit tests for versioning and Raspberry Pi-specific hardware information.
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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-36ba1cd-dirty [sigterm].
 */

// Unit testing:
/*
    g++ -Wall -Werror -fmax-errors=5 -static-libgcc -Wno-psabi -lstdc++fs -std=c++17 \
        -DDEBUG_MAIN_VERSION \
        -DMAKE_TAG="1.2.1-abc123" \
        -DMAKE_BRH="main" \
        -DMAKE_EXE="wspr" \
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
