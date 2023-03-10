#ifndef _LOGGING_HPP
#define _LOGGING_HPP
#pragma once

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
        crush(printline);
        std::cout << printline << std::endl << std::flush;
        printline = "";
    }
    void prtStd(const char * t)
    {
        // Char argument
        printline = printline + t;
        crush(printline);
        std::cout << printline << std::endl << std::flush;
        printline = "";
    }
    template <typename T>
    void prtStd(T t)
    {
        // Numeric argument
        printline = printline + std::to_string(t);
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
        crush(printline);
        std::cerr << printline << std::endl << std::flush;
        printline = "";
    }
    void prtErr(const char * t)
    {
        // Char argument
        printline = printline + t;
        crush(printline);
        std::cerr << printline << std::endl << std::flush;
        printline = "";
    }
    template <typename T>
    void prtErr(T t)
    {
        // Numeric argument
        printline = printline + std::to_string(t);
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
        char dts[20];
        time_t t = time(0);
        struct tm *tm;

        tm = gmtime(&t);
        strftime(dts, sizeof(dts), "%F %T", tm); 
        std::string str(dts);
        return str;
    }
};

#endif // _LOGGING_HPP
