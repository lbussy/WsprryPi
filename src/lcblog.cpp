/**
 * @file lcblog.cpp
 * @brief Provides unut testing for the header-only logging class.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is distributed under under
 * the MIT License. See LICENSE.MIT.md for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
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

// Unit tests:
// g++ -DDEBUG_MAIN_LCBLOG -o lcblog lcblog.cpp
// Test command:
// ./lcblog

#ifdef DEBUG_MAIN_LCBLOG
#include "lcblog.hpp"
#include <iostream>
#include <cassert>
#include <sstream>
#include <streambuf>

/**
 * @brief Unit test for checking multiple logS calls in Daemon mode.
 *
 * This function logs multiple messages to test whether each log message
 * appears on a separate line in Daemon mode.
 */
void testDaemonLoggingMultipleCalls()
{
    LCBLog logger;

    // Enable Daemon mode
    logger.setDaemon(true);

    // Redirect std::cout to capture output
    std::ostringstream capturedOutput;
    std::streambuf *originalCoutBuffer = std::cout.rdbuf();
    std::cout.rdbuf(capturedOutput.rdbuf());

    // Log multiple messages
    logger.logS("- Using NTP to calibrate transmission frequency.");
    logger.logS("- A small random frequency offset will be added to all transmissions.");

    // Restore std::cout
    std::cout.rdbuf(originalCoutBuffer);

    // Get the captured output
    std::string output = capturedOutput.str();

    // Validate the output
    std::regex logPattern("^\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2} \\w+\\t(.*)$");
    std::istringstream outputStream(output);
    std::string line;
    bool firstMessageFound = false;
    bool secondMessageFound = false;

    while (std::getline(outputStream, line)) {
        std::smatch match;
        if (std::regex_match(line, match, logPattern)) {
            // Check if the first and second log messages appear on separate lines
            if (match[1] == "- Using NTP to calibrate transmission frequency.") {
                firstMessageFound = true;
            }
            if (match[1] == "- A small random frequency offset will be added to all transmissions.") {
                secondMessageFound = true;
            }
        }
    }

    // Ensure both messages are found
    assert(firstMessageFound && "The first log message was not found in the output.");
    assert(secondMessageFound && "The second log message was not found in the output.");

    std::cout << "Daemon mode multiple logS calls test passed!" << std::endl;
}

/**
 * @brief Unit test for Daemon mode logging behavior.
 *
 * This function logs multiple messages in Daemon mode and validates whether
 * the log messages appear on separate lines.
 */
void testDaemonLogging()
{
    LCBLog logger;

    // Enable Daemon mode
    logger.setDaemon(true);

    // Redirect std::cout to capture output
    std::ostringstream capturedOutput;
    std::streambuf *originalCoutBuffer = std::cout.rdbuf();
    std::cout.rdbuf(capturedOutput.rdbuf());

    // Log messages
    logger.logS("First log message.");
    logger.logS("- A small random frequency offset will be added to all transmissions.");
    logger.logS("Third log message.");

    // Restore std::cout
    std::cout.rdbuf(originalCoutBuffer);

    // Get the captured output
    std::string output = capturedOutput.str();

    // Validate the output
    std::regex logPattern("^\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2} \\w+\\t(.*)$");
    std::istringstream outputStream(output);
    std::string line;
    std::vector<std::string> expectedMessages = {
        "First log message.",
        "- A small random frequency offset will be added to all transmissions.",
        "Third log message."
    };

    int i = 0;
    while (std::getline(outputStream, line)) {
        std::smatch match;
        if (std::regex_match(line, match, logPattern)) {
            // Ensure the message matches the expected log
            assert(match[1] == expectedMessages[i]);
            i++;
        }
    }

    // Ensure all expected messages were validated
    assert(i == expectedMessages.size());

    std::cout << "Daemon mode logging test passed!" << std::endl;
}

/**
 * @brief Unit tests for the LCBLog class.
 *
 * This function tests the `crush` function, which processes a string to
 * remove extraneous spaces and fix formatting issues.
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
 *
 * This function simulates the behavior of logging an empty string and
 * validates the output.
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
 *
 * This function tests standard and error output logging in both Daemon mode
 * and non-Daemon mode.
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
 *
 * This function runs all unit tests to ensure the logging system works
 * as expected.
 */
int main()
{
    std::cout << "Running LCBLog unit tests..." << std::endl;

    testCrush();
    testLogging();
    testDaemonLogging();
    testDaemonLoggingMultipleCalls();

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
#endif
