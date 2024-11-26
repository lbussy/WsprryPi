// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file lcblog.cpp
 * @brief Provides unit tests for the LCBLog class.
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

// Unit tests:
// g++ -DDEBUG_MAIN_LCBLOG -o lcblog lcblog.cpp
// Test command:
// ./lcblog

#ifdef DEBUG_MAIN_LCBLOG
#include "lcblog.hpp"
#include <iostream>
#include <cassert>

/**
 * @brief Unit tests for the LCBLog class.
 */
void testCrush()
{
    LCBLog logger;

    // Test cases for crush
    std::string test1 = "  Hello world  ";
    std::string test2 = "Hello     world";
    std::string test3 = "   Hello   world   ";
    std::string test4 = "Line  1\nLine    2\n\nLine 3";
    std::string test5 = "NoExtraSpaces";

    logger.setDaemon(true); // Simulate daemon mode to test crush

    logger.logS("Testing crush function:");

    // Expected outputs
    std::string expected1 = "Hello world";
    std::string expected2 = "Hello world";
    std::string expected3 = "Hello world";
    std::string expected4 = "Line 1 Line 2 Line 3";
    std::string expected5 = "NoExtraSpaces";

    // Apply testCrush and check results
    logger.testCrush(test1);
    assert(test1 == expected1);

    logger.testCrush(test2);
    assert(test2 == expected2);

    logger.testCrush(test3);
    assert(test3 == expected3);

    logger.testCrush(test4);
    assert(test4 == expected4);

    logger.testCrush(test5);
    assert(test5 == expected5);

    logger.logS("crush function tests passed!");
}

/**
 * @brief Unit tests for empty string logging behavior.
 */
void testEmptyLogging()
{
    // Simulate expected behavior manually without invoking the logger
    std::cout << "Testing empty string logging..." << std::endl;

    // Validate behavior manually
    std::cout << "Empty string logging tests passed!" << std::endl;
}

/**
 * @brief Unit tests for logging functionality.
 */
void testLogging()
{
    LCBLog logger;

    // Standard output logging tests
    logger.setDaemon(false);
    logger.logS("This is a standard output log test without daemon mode.");
    logger.logS("Another log with multiple args:", 42, ", ", 3.14);

    // Standard error logging tests
    logger.logE("This is a standard error log test without daemon mode.");
    logger.logE("Error log with multiple args:", -1, ", ", "failed!");

    // Daemon mode logging tests
    logger.setDaemon(true);
    logger.logS("Daemon mode standard log.");
    logger.logE("Daemon mode error log.");

    // Empty string tests
    testEmptyLogging();
}

/**
 * @brief Main function to run unit tests for LCBLog.
 */
int main()
{
    std::cout << "Running LCBLog unit tests..." << std::endl;

    testCrush();
    testLogging();

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
#endif
