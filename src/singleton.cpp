// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file singleton.cpp
 * @brief Test suite for the SingletonProcess class.
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
 */

// Unit testing:
// g++ -Wall -Werror -fmax-errors=5 -static-libgcc -Wno-psabi -lstdc++fs -std=c++17 -DDEBUG_MAIN_SINGLETON singleton.cpp -o singleton

#ifdef DEBUG_MAIN_SINGLETON

#include <iostream>
#include <cstdlib> // For std::atoi
#include "singleton.hpp"

/**
 * @brief Tests the SingletonProcess class with a given port.
 * @param port The port number to use for testing.
 */
void test_singleton(uint16_t port)
{
    try {
        std::cout << "\nTesting SingletonProcess with port " << port << "...\n";

        wspr::SingletonProcess singleton(port);

        if (singleton()) {
            std::cout << "Singleton instance created successfully on " << singleton.GetLockFileName() << ".\n";
        } else {
            std::cerr << "Singleton instance creation failed.\n";
        }

        std::cout << "Testing binding to the same port (should fail)...\n";
        try {
            wspr::SingletonProcess singleton_fail(port);
            if (singleton_fail()) {
                std::cerr << "Error: Should not be able to bind to the same port!\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Expected failure: " << e.what() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

/**
 * @brief Simulates a permission error by trying to bind to a restricted port.
 */
void simulate_permission_error()
{
    try {
        constexpr uint16_t restricted_port = 80; // Typically requires root privileges.

        std::cout << "\nTesting SingletonProcess on a restricted port (" << restricted_port << ")...\n";

        wspr::SingletonProcess singleton(restricted_port);

        // Check return value to avoid nodiscard warnings.
        if (singleton()) {
            std::cerr << "Error: Should not be able to bind to a restricted port without root privileges!\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Expected failure (restricted port): " << e.what() << "\n";
    }
}

/**
 * @brief Main function to run SingletonProcess tests.
 * @param argc The argument count.
 * @param argv The argument values.
 * @return Exit status.
 */
int main(int argc, char* argv[])
{
    // Test using a user-specified port or default to 8080
    uint16_t test_port = 8080;
    if (argc > 1) {
        test_port = static_cast<uint16_t>(std::atoi(argv[1]));
        if (test_port == 0) {
            std::cerr << "Invalid port specified. Defaulting to port 8080.\n";
            test_port = 8080;
        }
    }

    std::cout << "===========================\n";
    std::cout << "Testing SingletonProcess...\n";
    std::cout << "===========================\n";

    test_singleton(test_port);
    simulate_permission_error();

    std::cout << "\nSingletonProcess test completed.\n";
    return 0;
}

#endif // DEBUG_MAIN_SINGLETON
