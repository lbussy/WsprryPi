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

template <typename T, typename... Args>
void LCBLog::log(LogLevel level, std::ostream& stream, T t, Args... args) {
    if (!shouldLog(level)) return;

    std::lock_guard<std::mutex> lock(logMutex);
    logToStream(stream, level, t, args...);
}

template <typename T, typename... Args>
void LCBLog::logToStream(std::ostream& stream, LogLevel level, T t, Args... args) {
    std::ostringstream oss;
    oss << t;
    ((oss << " " << args), ...);

    std::string logMessage = oss.str();
    std::istringstream messageStream(logMessage);
    std::string line;
    bool firstLine = true;

    while (std::getline(messageStream, line)) {
        if (!firstLine) stream << std::endl;

        crush(line);
        if (isDaemon) stream << getStamp() << "\t";

        stream << "[" << logLevelToString(level) << "] " << line;
        firstLine = false;
    }
    stream << std::endl;
}
