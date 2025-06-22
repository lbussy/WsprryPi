/**
 * @file logging.hpp
 * @brief Handles management of the LCBLog logging utility.
 *
 * This project is is licensed under the MIT License. See LICENSE.md
 * for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
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

#include "lcblog.hpp"

/**
 * @brief Global instance of the LCBLog logging utility.
 *
 * The `llog` object provides thread-safe logging functionality with support for
 * multiple log levels, including DEBUG, INFO, WARN, ERROR, and FATAL.
 * It is used throughout the application to log messages for debugging,
 * monitoring, and error reporting.
 *
 * This instance is initialized globally to allow consistent logging across all
 * modules. Log messages can include timestamps and are output to standard streams
 * or log files depending on the configuration.
 *
 * Example usage:
 * @code
 * llog.logS(INFO, "Application started.");
 * llog.logE(ERROR, "Failed to open configuration file.");
 * @endcode
 *
 * @see https://github.com/lbussy/LCBLog for detailed documentation and examples.
 */
extern LCBLog llog;

/**
 * @brief Initializes the logger with the appropriate log level.
 *
 * This function sets the log level based on the current debug state. If the
 * build is compiled with the DEBUG_BUILD macro, the log level is set to DEBUG.
 * Otherwise, it defaults to INFO.
 *
 * @note Ensure that the `get_debug_state()` function correctly reflects the
 *       build configuration for accurate log level assignment.
 *
 * @example
 * initialize_logger();
 * // Sets llog to DEBUG or INFO depending on the build mode.
 */
extern void initialize_logger();

#endif // _LOGGING_HPP
