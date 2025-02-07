/**
 * @file lcblog.cpp
 * @brief Implementation of the LCBLog logging class.
 *
 * This logging class provides a flexible and thread-safe logging mechanism 
 * with support for multiple log levels, timestamped logs, and customizable 
 * output streams.
 *
 * This software is distributed under the MIT License. See LICENSE.MIT.md 
 * for details.
 *
 * @author Lee C. Bussy
 * @copyright 2023-2025
 * @license MIT License
 */

#include "lcblog.hpp"
#include <algorithm>
#include <regex>
#include <iostream>

/**
 * @brief Converts a log level to its string representation.
 * 
 * @param level The log level to convert.
 * @return A string representing the log level.
 */
std::string logLevelToString(LogLevel level) {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO ";
        case WARN: return "WARN ";
        case ERROR: return "ERROR";
        case FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Constructs the logging class with specified output streams.
 * 
 * @param outStream The output stream for standard logs (default: std::cout).
 * @param errStream The output stream for error logs (default: std::cerr).
 */
LCBLog::LCBLog(std::ostream& outStream, std::ostream& errStream)
    : logLevel(INFO), out(outStream), err(errStream) {}

/**
 * @brief Sets the minimum log level for message output.
 * 
 * @param level The log level to set.
 */
void LCBLog::setLogLevel(LogLevel level) {
    {
        std::lock_guard<std::mutex> lock(logMutex);
        logLevel = level;
    }

    logS(INFO, "Log level changed to:", logLevelToString(level));
}

/**
 * @brief Enables or disables timestamping for log messages.
 * 
 * @param enable If true, timestamps will be included in logs.
 */
void LCBLog::enableTimestamps(bool enable) {
    printTimestamps = enable;
}

/**
 * @brief Checks if a message should be logged based on the current log level.
 * 
 * @param level The log level to check.
 * @return True if the message should be logged, otherwise false.
 */
bool LCBLog::shouldLog(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(logLevel);
}

/**
 * @brief Generates a timestamp string for log entries.
 * 
 * @return A formatted timestamp string.
 */
std::string LCBLog::getStamp() {
    char dts[24];
    time_t t = time(nullptr);
    struct tm tm;
    gmtime_r(&t, &tm);
    strftime(dts, sizeof(dts), "%F %T UTC", &tm);
    return std::string(dts);
}

/**
 * @brief Cleans up a string by trimming whitespace and reducing consecutive spaces.
 * 
 * @param s The string to sanitize.
 */
void LCBLog::crush(std::string& s) {
    // Trim leading whitespace
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));

    // Trim trailing whitespace
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());

    try {
        // Replace multiple spaces with a single space
        s = std::regex_replace(s, std::regex("\\s{2,}"), " ");

        // Remove spaces immediately after '(' and before ')'
        s = std::regex_replace(s, std::regex("\\(\\s+"), "("); // Remove space after '('
        s = std::regex_replace(s, std::regex("\\s+\\)"), ")"); // Remove space before ')'

    } catch (const std::regex_error& e) {
        std::cerr << "[ERROR] Regex processing failed in crush(): " << e.what() << std::endl;
    }
}