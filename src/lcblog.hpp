#ifndef _LOGGING_HPP
#define _LOGGING_HPP
#pragma once

/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-9e6e021 [new_release_proc].
*/

#include <iostream>
#include <string>
#include <regex>

class LCBLog
{
public:
    void setDaemon(bool daemonmode)
    {
        isDaemon = daemonmode;
    }

    template <typename T>
    void logS(T t)
    {
        if (isDaemon)
            std::cout << getStamp() << "\t";
        prtStd(t);
    }

    template<typename T, typename... Args>
    void logS(T t, Args... args)
    {
        if (isDaemon)
            std::cout << getStamp() << "\t";
        prtStd(t, args...);
    }

    template <typename T>
    void logE(T t)
    {
        if (isDaemon)
            std::cerr << getStamp() << "\t";
        prtStd(t);
    }

    template<typename T, typename... Args>
    void logE(T t, Args... args)
    {
        if (isDaemon)
            std::cerr << getStamp() << "\t";
        prtStd(t, args...);
    }

private:
    bool isDaemon = false;
    std::string printline = "";

    // Standard Out Printline
    //
    // Single arguments
    void prtStd(std::string t)
    {
        // String argument
        printline = printline + t;
        if (isDaemon)
            crush(printline);
        std::cout << printline << std::endl << std::flush;
        printline = "";
    }
    void prtStd(const char * t)
    {
        // Char argument
        printline = printline + t;
        if (isDaemon)
            crush(printline);
        std::cout << printline << std::endl << std::flush;
        printline = "";
    }
    template <typename T>
    void prtStd(T t)
    {
        // Numeric argument
        printline = printline + std::to_string(t);
        if (isDaemon)
            crush(printline);
        std::cout << printline << std::endl << std::flush;
        printline = "";
    }
    //
    // Multiple arguments
    template <typename... Args>
    void prtStd(std::string t, Args... args)
    {
        // First argument is a string
        printline = printline + t;
        prtStd(args...);
    }
    template <typename... Args>
    void prtStd(const char * t, Args... args)
    {
        // First argument is a char
        printline = printline + t;
        prtStd(args...);
    }
    template <typename T, typename... Args>
    void prtStd(T t, Args... args)
    {
        // First argument is numeric
        printline = printline + std::to_string(t);
        prtStd(args...);
    }

    // Standard Error Printline
    //
    // Single arguments
    void prtErr(std::string t)
    {
        // String argument
        printline = printline + t;
        if (isDaemon)
            crush(printline);
        std::cerr << printline << std::endl << std::flush;
        printline = "";
    }
    void prtErr(const char * t)
    {
        // Char argument
        printline = printline + t;
        if (isDaemon)
            crush(printline);
        std::cerr << printline << std::endl << std::flush;
        printline = "";
    }
    template <typename T>
    void prtErr(T t)
    {
        // Numeric argument
        printline = printline + std::to_string(t);
        if (isDaemon)
            crush(printline);
        std::cerr << printline << std::endl << std::flush;
        printline = "";
    }
    //
    // Multiple arguments
    template <typename... Args>
    void prtErr(std::string t, Args... args)
    {
        // First argument is a string
        printline = printline + t;
        prtErr(args...);
    }
    template <typename... Args>
    void prtErr(const char * t, Args... args)
    {
        // First argument is a char
        printline = printline + t;
        prtErr(args...);
    }
    template <typename T, typename... Args>
    void prtErr(T t, Args... args)
    {
        // First argument is numeric
        printline = printline + std::to_string(t);
        prtErr(args...);
    }

    // Clean string of extraeneous whitespace 
    void crush(std::string &s)
    {
        // Remove multiple whitespace down to a single space
        s = std::regex_replace(s, std::regex("\\s+"), " ");
        // Remove leading and trailing whitespace
        s = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");
    }

    std::string getStamp()
    {
        char dts[24];
        time_t t = time(0);
        struct tm *tm;

        tm = gmtime(&t);
        strftime(dts, sizeof(dts), "%F %T %Z", tm);
        std::string str(dts);
        return str;
    }
};

#endif // _LOGGING_HPP
