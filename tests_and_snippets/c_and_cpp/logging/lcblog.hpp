/**
 * @file lcblog.hpp
 * @brief A logging class for handling log levels, formatting, and timestamping.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is distributed under the
 * MIT License. See LICENSE.MIT.md for more information.
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

#ifndef _LOGGING_HPP
#define _LOGGING_HPP

#include <iostream>
#include <sstream>
#include <mutex>
#include <string>
#include <ctime>
#include <regex>

/**
 * @enum LogLevel
 * @brief Enum for different log levels used in the logging system.
 * 
 * This enum represents the different log levels available. Log levels are 
 * used to filter log messages based on their severity.
 */
enum LogLevel {
    DEBUG = 0, /**< Debug level for detailed messages. */
    INFO,      /**< Informational messages. */
    WARN,      /**< Warning messages for potential issues. */
    ERROR,     /**< Error messages for failures. */
    FATAL      /**< Critical errors that cause the program to terminate. */
};

/**
 * @var currentLogLevel
 * @brief Global variable to store the current log level.
 * 
 * This variable holds the current log level used for filtering log messages.
 * The default value is set to INFO, meaning that only log messages of INFO 
 * level or higher (INFO, WARN, ERROR, FATAL) will be logged.
 */
LogLevel currentLogLevel = INFO;  /**< Default log level set to INFO. */

/**
 * @brief Convert a LogLevel enum to its string representation.
 * @param level The log level to convert.
 * @return A string corresponding to the given log level.
 * 
 * This function converts a `LogLevel` enum value into a human-readable string.
 * If an invalid or unknown log level is provided, the function returns "UNKNOWN".
 */
inline std::string logLevelToString(LogLevel level) {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO ";
        case WARN: return "WARN ";
        case ERROR: return "ERROR";
        case FATAL: return "FATAL";
        default: return "UNKWN";
    }
}

/**
 * @brief Set the log level for filtering log messages.
 * @param level The new log level to set.
 * 
 * This function sets the global log level for filtering log messages. Only
 * messages with a log level greater than or equal to the specified level
 * will be logged. It also prints the new log level to the console.
 */
inline void setLogLevel(LogLevel level) {
    std::cout << "[INFO] Log level changed to: " << logLevelToString(level) << std::endl;
    currentLogLevel = level;
}

/**
 * @brief Determine if a message should be logged based on its log level.
 * @param level The log level of the message.
 * @return `true` if the message should be logged, `false` otherwise.
 * 
 * This function compares the provided log level with the current log level.
 * A message will be logged if its log level is greater than or equal to
 * the current log level.
 */
inline bool shouldLog(LogLevel level) {
    return static_cast<int>(level) >= static_cast<int>(currentLogLevel);
}

/**
 * @class LCBLog
 * @brief A class for logging messages with different log levels.
 * 
 * The `LCBLog` class handles logging messages to an output stream (e.g., 
 * standard output or a log file) based on the current log level. It provides 
 * functions for logging messages at different log levels (DEBUG, INFO, WARN, 
 * ERROR, FATAL) and supports timestamping in daemon mode.
 */
class LCBLog {
public:
    /**
    * @brief Constructs an LCBLog object with optional output and error streams.
    * 
    * If no streams are provided, the default streams std::cout (for output) 
    * and std::cerr (for errors) will be used.
    * 
    * @param outStream The stream to write log messages to (default: std::cout).
    * @param errStream The stream to write error messages to (default: std::cerr).
    */
    explicit LCBLog(std::ostream& outStream = std::cout, std::ostream& errStream = std::cerr)
        : out(outStream), err(errStream) {}

    /**
    * @brief Log a message to the output stream at the specified log level.
    * @param level The log level of the message.
    * @param t The message to log.
    * 
    * This function logs a message to the specified output stream (default is 
    * `std::cout`). It only logs messages whose log level is greater than or 
    * equal to the current log level. The message is logged in a thread-safe manner 
    * using a mutex to prevent concurrent logging issues.
    */
    template <typename T>
    void logS(LogLevel level, T t) {
        if (!shouldLog(level)) {
            return;  // Skip logging if the level is too low
        }

        std::lock_guard<std::mutex> lock(logMutex);

        // Log the message to the output stream (console or log file)
        logToStream(out, level, t);
    }

    /**
    * @brief Log a message with multiple arguments to the output stream at the specified log level.
    * @param level The log level of the message.
    * @param t The first message to log.
    * @param args Additional messages to log.
    * 
    * This function logs a message to the specified output stream (default is `std::cout`) 
    * with multiple arguments. It only logs messages whose log level is greater than or equal 
    * to the current log level. The message is logged in a thread-safe manner using a mutex 
    * to prevent concurrent logging issues. Multiple arguments can be passed, which will be 
    * concatenated into a single log message.
    */
    template <typename T, typename... Args>
    void logS(LogLevel level, T t, Args... args) {
        if (!shouldLog(level)) {
            return;
        }

        std::lock_guard<std::mutex> lock(logMutex);

        // Log the message to the output stream (console or log file)
        logToStream(out, level, t, args...);
    }

    /**
    * @brief Log a message to the error stream at the specified log level.
    * @param level The log level of the message.
    * @param t The message to log.
    * 
    * This function logs a message to the specified error stream (default is 
    * `std::cerr`). It only logs messages whose log level is greater than or 
    * equal to the current log level. The message is logged in a thread-safe manner 
    * using a mutex to prevent concurrent logging issues.
    */
    template <typename T>
    void logE(LogLevel level, T t) {
        if (!shouldLog(level)) {
            return;  // Skip logging if the level is too low
        }

        std::lock_guard<std::mutex> lock(logMutex);

        // Log the message to the error stream (console or log file)
        logToStream(err, level, t);
    }

    /**
    * @brief Log a message with multiple arguments to the error stream at the specified log level.
    * @param level The log level of the message.
    * @param t The first message to log.
    * @param args Additional messages to log.
    * 
    * This function logs a message to the specified error stream (default is `std::cerr`) 
    * with multiple arguments. It only logs messages whose log level is greater than or equal 
    * to the current log level. The message is logged in a thread-safe manner using a mutex 
    * to prevent concurrent logging issues. Multiple arguments can be passed, which will be 
    * concatenated into a single log message.
    */
    template <typename T, typename... Args>
    void logE(LogLevel level, T t, Args... args) {
        if (!shouldLog(level)) {
            return;
        }

        std::lock_guard<std::mutex> lock(logMutex);

        // Log the message to the error stream (console or log file)
        logToStream(err, level, t, args...);
    }

    /**
    * @brief Set the log level for filtering log messages.
    * @param level The log level to set.
    * 
    * This function sets the global log level for filtering log messages. Only 
    * messages with a log level greater than or equal to the specified level 
    * will be logged. It updates the `currentLogLevel` and prints the new log 
    * level to the console.
    */
    void setLogLevel(LogLevel level);

    /**
    * @brief Enable or disable daemon mode for logging.
    * @param daemonmode True to enable daemon mode, false to disable.
    * 
    * This function controls whether daemon mode is enabled for logging. 
    * In daemon mode, log messages are prepended with a timestamp. 
    * If daemon mode is disabled, only the log level and message are shown.
    */
    void setDaemon(bool daemonmode) {
        isDaemon = daemonmode;
    }

private:
    /**
    * @brief The output stream for standard log messages.
    * 
    * This stream is used for logging standard messages (default is `std::cout`).
    */
    std::ostream& out;

    /**
    * @brief The error stream for logging error messages.
    * 
    * This stream is used for logging error messages (default is `std::cerr`).
    */
    std::ostream& err;

    /**
    * @brief Mutex to ensure thread-safety for logging.
    * 
    * This mutex is used to synchronize access to the log streams to prevent
    * concurrent logging issues.
    */
    std::mutex logMutex;

    /**
    * @brief Flag indicating whether daemon mode is enabled.
    * 
    * If set to `true`, timestamps will be prepended to log messages.
    * If set to `false`, no timestamps will be added.
    */
    bool isDaemon = false;

    /**
    * @brief Log a message to a specified stream with log level filtering and optional timestamp.
    * @tparam T The type of the first message to log.
    * @tparam Args The types of additional messages to log.
    * @param stream The output stream where the log message will be sent (e.g., `std::cout` or `std::cerr`).
    * @param level The log level of the message.
    * @param t The first message to log.
    * @param args Additional messages to log.
    * 
    * This function logs a message to the specified output stream. The log message is
    * created by concatenating the provided arguments. The log level is checked, and 
    * only messages with a log level greater than or equal to the current log level will
    * be logged. If daemon mode is enabled, each line of the log message will be prepended 
    * with a timestamp. The message is split at any newline characters and processed 
    * line by line.
    */
    template <typename T, typename... Args>
    void logToStream(std::ostream& stream, LogLevel level, T t, Args... args) {
        if (!shouldLog(level)) {
            return;  // Skip logging if the level is too low
        }

        // Concatenate all arguments into a single string
        std::ostringstream oss;
        oss << t;
        ((oss << " " << args), ...);

        std::string logMessage = oss.str();

        // Split the log message into individual lines at each newline
        std::istringstream messageStream(logMessage);
        std::string line;
        bool firstLine = true;

        // Process each line after splitting by '\n'
        while (std::getline(messageStream, line)) {
            // If it's not the first line, add a newline
            if (!firstLine) {
                stream << std::endl;  // Add newline for subsequent lines
            }

            // Crush the message for each line to clean up extra spaces
            crush(line);

            // Add the timestamp for each line in daemon mode
            if (isDaemon) {
                stream << getStamp() << "\t";  // Add timestamp to each line in daemon mode
            }

            // Add the log level and the line content
            stream << "[" << logLevelToString(level) << "] " << line;
            firstLine = false;  // Disable timestamp for subsequent lines
        }

        stream << std::endl;  // End with a newline after the complete log message
    }

    /**
    * @brief Clean up a string by removing extraneous spaces and fixing formatting issues.
    * @param s The string to clean.
    * 
    * This function processes the given string to:
    * - Trim leading and trailing spaces.
    * - Replace multiple consecutive spaces with a single space.
    * 
    * It ensures that the string is well-formatted by removing unnecessary whitespace
    * and ensuring consistent spacing.
    */
    static void crush(std::string& s) {
        // Trim leading and trailing spaces
        s = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");

        // Replace multiple spaces with a single space
        s = std::regex_replace(s, std::regex("\\s{2,}"), " ");
    }

    /**
    * @brief Get the current timestamp formatted as a string.
    * @return A string representing the current timestamp in the format "YYYY-MM-DD HH:MM:SS TZ".
    * 
    * This function retrieves the current time and formats it as a string. The string 
    * includes the date, time (hour, minute, second), and timezone. The format used is:
    * "YYYY-MM-DD HH:MM:SS TZ".
    */
    std::string getStamp() {
        char dts[24];
        time_t t = time(0);
        struct tm *tm = gmtime(&t);
        strftime(dts, sizeof(dts), "%F %T %Z", tm);
        return std::string(dts);
    }
};

#endif // _LOGGING_HPP