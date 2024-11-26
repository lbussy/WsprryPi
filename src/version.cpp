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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-7f4c707 [refactoring].
 */

// Unit testing:
// g++ -Wall -Werror -fmax-errors=5 -static-libgcc -Wno-psabi -lstdc++fs -std=c++17 -DDEBUG_MAIN_VERSION -DMAKE_SRC_TAG=\"1.2.1-abc123\" -DMAKE_SRC_BRH=\"main\" version.cpp -o version -lbcm_host
// Test command:
// ./version

#ifdef DEBUG_MAIN_VERSION

#include <iostream>
#include "version.hpp"

/**
 * @brief Main function to test versioning and Raspberry Pi hardware information.
 *
 * This function tests various version-related functions and outputs their results
 * for validation. Specifically, it tests:
 * - Executable version and branch information.
 * - Raspberry Pi processor ID and version.
 * - GPIO base address.
 *
 * @return Exit status (0 for success, non-zero for failure).
 */
int main()
{
    std::cout << "===========================\n";
    std::cout << "Testing Version Functions...\n";
    std::cout << "===========================\n";

    // Test executable version and branch macros
    std::cout << "Executable Version: " << exeversion() << "\n";
    std::cout << "Branch: " << branch() << "\n";

    // Test Raspberry Pi-specific hardware information
    int processorId = ver();
    std::cout << "Processor ID: " << processorId << "\n";

    const char* piVersion = RPiVersion();
    std::cout << "Raspberry Pi Version: " << piVersion << "\n";

    unsigned gpioBaseAddr = gpioBase();
    std::cout << "GPIO Base Address: 0x" << std::hex << gpioBaseAddr << std::dec << "\n";

    std::cout << "\nVersion function tests completed successfully.\n";
    return 0;
}

#endif // DEBUG_MAIN_VERSION
