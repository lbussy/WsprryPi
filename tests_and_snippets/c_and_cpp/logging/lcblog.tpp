/**
 * @file lcblog.tpp
 * @brief A logging class for handling log levels, formatting, and
 * timestamping within a C++ project.
 *
 * This logging class provides a flexible and thread-safe logging mechanism
 * with support for multiple log levels, timestamped logs, and customizable
 * output streams. include the header (`lcblog.hpp`), implementation
 * (`lcblog.cpp`), and template definitions (`lcblog.tpp`)when using in
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

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

/**
 * @brief Logs a message at a given log level.
 *
 * @tparam T The first type of the message content.
 * @tparam Args Variadic template arguments for additional message content.
 * @param level The log level of the message.
 * @param stream The output stream to write the log message to.
 * @param t The first message content.
 * @param args Additional message content.
 */
template <typename T, typename... Args>
void LCBLog::log(LogLevel level, std::ostream& stream, T t, Args... args) {
    if (!shouldLog(level)) return;

    std::lock_guard<std::mutex> lock(logMutex);
    logToStream(stream, level, t, args...);
}

/**
 * @brief Logs a formatted message to a specified stream.
 *
 * Processes multi-line messages, applies optional timestamping, and formats
 * the output with log level tags.
 *
 * @tparam T The first type of the message content.
 * @tparam Args Variadic template arguments for additional message content.
 * @param stream The output stream to write the log message to.
 * @param level The log level of the message.
 * @param t The first message content.
 * @param args Additional message content.
 */
template <typename T, typename... Args>
void LCBLog::logToStream(std::ostream& stream, LogLevel level, T t, Args... args) {
    std::ostringstream oss;
    oss << t;  // First argument is directly added

    if constexpr (sizeof...(args) > 0) {
        auto shouldSkipSpace = [](const auto& arg) -> bool {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::string>) {
                return arg.size() == 1 && (arg == "." || arg == "," || arg == ";" || arg == ":");
            } else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, const char*>) {
                std::string strArg = arg;
                return strArg.size() == 1 && (strArg == "." || strArg == "," || strArg == ";" || strArg == ":");
            }
            return false;
        };

        // Fold expression to append arguments correctly
        ((shouldSkipSpace(args) ? oss << args : oss << " " << args), ...);
    }

    std::string logMessage = oss.str();
    std::istringstream messageStream(logMessage);
    std::string line;
    bool firstLine = true;

    constexpr int LOG_LEVEL_WIDTH = 5; // Maximum length of log level names (DEBUG, ERROR, FATAL = 5)
    std::string levelStr = logLevelToString(level);
    levelStr.append(LOG_LEVEL_WIDTH - levelStr.size(), ' '); // Pad to align all levels

    while (std::getline(messageStream, line)) {
        if (!firstLine) stream << std::endl;

        crush(line);  // Remove excessive whitespace
        if (printTimestamps) stream << getStamp() << "\t";

        stream << "[" << levelStr << "] " << line;
        firstLine = false;
    }
    stream << std::endl;
}
