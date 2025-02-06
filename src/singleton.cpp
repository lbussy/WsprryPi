/**
 * @file singleton.cpp
 * @brief Provides unit testing for a header-only class to enforce singleton
 * behavior.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 *
 * However, this portion of new code added to the project is distributed under
 * under the MIT License. See LICENSE for more information.
 *
 * Copyright (c) 2009-2020, Ben Hoyt, All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Unit testing:
// g++ -Wall -Werror -fmax-errors=5 -static-libgcc -Wno-psabi -lstdc++fs -std=c++17 -DDEBUG_MAIN_SINGLETON singleton.cpp -o singleton
// Test command:
// ./singleton

#ifdef DEBUG_MAIN_SINGLETON

#include <iostream>
#include <cstdlib> // For std::atoi
#include "singleton.hpp"

/**
 * @brief Tests the SingletonProcess class with a given port.
 *
 * Verifies whether the SingletonProcess instance can be created and whether
 * binding to the same port is prevented as expected.
 *
 * @param port The port number to use for testing.
 */
void test_singleton(uint16_t port)
{
    try {
        std::cout << "\nTesting SingletonProcess with port " << port << "...\n";

        wsprrypi::SingletonProcess singleton(port);

        if (singleton()) {
            std::cout << "Singleton instance created successfully on " << singleton.GetLockFileName() << ".\n";
        } else {
            std::cerr << "Singleton instance creation failed.\n";
        }

        std::cout << "Testing binding to the same port (should fail)...\n";
        try {
            wsprrypi::SingletonProcess singleton_fail(port);
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
 *
 * Attempts to create a SingletonProcess instance on a port that typically
 * requires root privileges (e.g., port 80) to ensure the proper error is raised.
 */
void simulate_permission_error()
{
    try {
        constexpr uint16_t restricted_port = 80; // Typically requires root privileges.

        std::cout << "\nTesting SingletonProcess on a restricted port (" << restricted_port << ")...\n";

        wsprrypi::SingletonProcess singleton(restricted_port);

        if (singleton()) {
            std::cerr << "Error: Should not be able to bind to a restricted port without root privileges!\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Expected failure (restricted port): " << e.what() << "\n";
    }
}

/**
 * @brief Main function to run SingletonProcess tests.
 *
 * Executes tests for SingletonProcess class functionality, including creating
 * an instance on a user-specified port and simulating a permission error on a
 * restricted port.
 *
 * @param argc The argument count.
 * @param argv The argument values.
 * @return Exit status (0 for success, non-zero for error).
 */
int main(int argc, char* argv[])
{
    uint16_t test_port = 8080; // Default test port
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
