/**
 * @file lcblog.hpp
 * @brief A logging class for handling log levels, formatting, and
 * timestamping within a C++ project.
 *
 * This logging class provides a flexible and thread-safe logging mechanism
 * with support for multiple log levels, timestamped logs, and customizable
 * output streams. include the header (`lcblog.hpp`), implementation
 * (`lcblog.cpp`), and template definitions (`lcblog.tpp`) when using in
 * a project.
 *
 * This software is distributed under the MIT License. See LICENSE.MIT.md for
 * details.
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

#ifndef LCBLOG_HPP
#define LCBLOG_HPP

#include <iostream>
#include <sstream>
#include <mutex>
#include <string>
#include <ctime>
#include <iomanip>

/**
 * @enum LogLevel
 * @brief Defines the severity levels for logging.
 *
 * This enumeration represents different log levels, which determine
 * the severity and importance of logged messages.
 */
enum LogLevel {
    DEBUG = 0, ///< Debug-level messages for detailed troubleshooting.
    INFO,      ///< Informational messages for general system state.
    WARN,      ///< Warnings indicating potential issues.
    ERROR,     ///< Errors that require attention but allow continued execution.
    FATAL      ///< Critical errors that result in program termination.
};

/**
 * @brief Converts a log level to its string representation.
 *
 * @param level The log level to convert.
 * @return A string representing the log level.
 */
std::string logLevelToString(LogLevel level);

/**
 * @class LCBLog
 * @brief A thread-safe logging class supporting multiple log levels.
 *
 * Provides a mechanism for logging messages with different severity levels,
 * timestamping, and customizable output streams.
 */
class LCBLog {
public:
    /**
     * @brief Constructs the logging class with specified output streams.
     *
     * @param outStream The output stream for standard logs (default: std::cout).
     * @param errStream The output stream for error logs (default: std::cerr).
     */
    explicit LCBLog(std::ostream& outStream = std::cout, std::ostream& errStream = std::cerr);

    /**
     * @brief Sets the minimum log level for message output.
     *
     * @param level The log level to set.
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief Checks if a message should be logged based on the current log level.
     *
     * @param level The log level to check.
     * @return True if the message should be logged, otherwise false.
     */
    bool shouldLog(LogLevel level) const;

    /**
     * @brief Logs a message to a specified stream.
     *
     * @tparam T First message argument type.
     * @tparam Args Variadic additional message arguments.
     * @param level The log level of the message.
     * @param stream The output stream to write to.
     * @param t First message content.
     * @param args Additional message content.
     */
    template <typename T, typename... Args>
    void log(LogLevel level, std::ostream& stream, T t, Args... args);

    /**
     * @brief Logs a message to the standard output stream.
     *
     * @tparam T First message argument type.
     * @tparam Args Variadic additional message arguments.
     * @param level The log level of the message.
     * @param t First message content.
     * @param args Additional message content.
     */
    template <typename T, typename... Args>
    void logS(LogLevel level, T t, Args... args) {
        log(level, out, t, args...);
    }

    /**
     * @brief Logs a message to the error output stream.
     *
     * @tparam T First message argument type.
     * @tparam Args Variadic additional message arguments.
     * @param level The log level of the message.
     * @param t First message content.
     * @param args Additional message content.
     */
    template <typename T, typename... Args>
    void logE(LogLevel level, T t, Args... args) {
        log(level, err, t, args...);
    }

    /**
     * @brief Enables or disables timestamping for log messages.
     *
     * @param enable If true, timestamps will be included in logs.
     */
    void enableTimestamps(bool enable);

private:
    LogLevel logLevel;              ///< The current log level threshold.
    std::ostream& out;              ///< Stream for standard log output.
    std::ostream& err;              ///< Stream for error log output.
    std::mutex logMutex;            ///< Mutex for thread-safe logging.
    bool printTimestamps = false;   ///< LOgger should print timestamp.

    /**
     * @brief Logs a formatted message to a specified stream.
     *
     * @tparam T First message argument type.
     * @tparam Args Variadic additional message arguments.
     * @param stream The output stream to write to.
     * @param level The log level of the message.
     * @param t First message content.
     * @param args Additional message content.
     */
    template <typename T, typename... Args>
    void logToStream(std::ostream& stream, LogLevel level, T t, Args... args);

    /**
     * @brief Sanitizes a string by removing unwanted characters.
     *
     * @param s The string to sanitize.
     */
    static void crush(std::string& s);

    /**
     * @brief Generates a timestamp string for log entries.
     *
     * @return A formatted timestamp string.
     */
    std::string getStamp();
};

#include "lcblog.tpp"

#endif // LCBLOG_HPP
