/**
 * @file lcblog.hpp
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

#ifndef LCBLOG_HPP
#define LCBLOG_HPP

#include <iostream> 
#include <sstream>
#include <mutex>
#include <string>
#include <ctime>
#include <iomanip> 

enum LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    FATAL
};

std::string logLevelToString(LogLevel level);

class LCBLog {
public:
    explicit LCBLog(std::ostream& outStream = std::cout, std::ostream& errStream = std::cerr);

    void setLogLevel(LogLevel level);
    bool shouldLog(LogLevel level) const;

    template <typename T, typename... Args>
    void log(LogLevel level, std::ostream& stream, T t, Args... args);

    template <typename T, typename... Args>
    void logS(LogLevel level, T t, Args... args) {
        log(level, out, t, args...);
    }

    template <typename T, typename... Args>
    void logE(LogLevel level, T t, Args... args) {
        log(level, err, t, args...);
    }

    void enableTimestamps(bool enable);

private:
    LogLevel logLevel;
    std::ostream& out;
    std::ostream& err;
    std::mutex logMutex;
    bool isDaemon = false;

    template <typename T, typename... Args>
    void logToStream(std::ostream& stream, LogLevel level, T t, Args... args);

    static void crush(std::string& s);
    std::string getStamp();
};

#include "lcblog.tpp"

#endif // LCBLOG_HPP