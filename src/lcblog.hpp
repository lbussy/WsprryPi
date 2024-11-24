// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-32d490b [refactoring].
 */

#ifndef _LOGGING_HPP
#define _LOGGING_HPP

#include <iostream>
#include <string>
#include <regex>
#include <ctime>

/**
 * @brief A logging class to handle standard output and error output with optional timestamping.
 */
class LCBLog
{
public:
    /**
     * @brief Set daemon mode to enable or disable timestamps.
     * @param daemonmode True to enable daemon mode; false otherwise.
     */
    void setDaemon(bool daemonmode)
    {
        isDaemon = daemonmode;
    }

    /**
     * @brief Log a single message to standard output.
     * @tparam T The type of the message.
     * @param t The message to log.
     */
    template <typename T>
    void logS(T t)
    {
        if (isDaemon)
            std::cout << getStamp() << "\t";
        prtStd(t);
    }

    /**
     * @brief Log multiple messages to standard output.
     * @tparam T The type of the first message.
     * @tparam Args The types of the additional messages.
     * @param t The first message.
     * @param args Additional messages.
     */
    template <typename T, typename... Args>
    void logS(T t, Args... args)
    {
        if (isDaemon)
            std::cout << getStamp() << "\t";
        prtStd(t, args...);
    }

    /**
     * @brief Log a single message to standard error.
     * @tparam T The type of the message.
     * @param t The message to log.
     */
    template <typename T>
    void logE(T t)
    {
        if (isDaemon)
            std::cerr << getStamp() << "\t";
        prtStd(t);
    }

    /**
     * @brief Log multiple messages to standard error.
     * @tparam T The type of the first message.
     * @tparam Args The types of the additional messages.
     * @param t The first message.
     * @param args Additional messages.
     */
    template <typename T, typename... Args>
    void logE(T t, Args... args)
    {
        if (isDaemon)
            std::cerr << getStamp() << "\t";
        prtStd(t, args...);
    }

    /**
     * @brief Public wrapper to test the private crush function.
     * @param s The string to be processed.
     */
    void testCrush(std::string &s)
    {
        crush(s);
    }

private:
    bool isDaemon = false; /**< Indicates whether daemon mode is enabled. */
    std::string printline = ""; /**< Buffer for the log message. */

    /**
     * @brief Print a single message to standard output.
     * @param t The message to print.
     */
    void prtStd(const std::string &t)
    {
        printline += t;
        finalizeOutput(std::cout);
    }

    /**
     * @brief Print a single message to standard output.
     * @param t The message to print.
     */
    void prtStd(const char *t)
    {
        printline += t;
        finalizeOutput(std::cout);
    }

    /**
     * @brief Print a single numeric message to standard output.
     * @tparam T The type of the message.
     * @param t The message to print.
     */
    template <typename T>
    void prtStd(T t)
    {
        printline += std::to_string(t);
        finalizeOutput(std::cout);
    }

    /**
     * @brief Handle multiple arguments for standard output.
     * @tparam T The type of the first argument.
     * @tparam Args The types of the additional arguments.
     * @param t The first argument.
     * @param args Additional arguments.
     */
    template <typename T, typename... Args>
    void prtStd(T t, Args... args)
    {
        prtStd(t);
        prtStd(args...);
    }

    /**
     * @brief Finalize the log output to the specified stream.
     * @param os The output stream (e.g., std::cout or std::cerr).
     */
    void finalizeOutput(std::ostream &os)
    {
        if (printline.empty())
            return; // Do not print if the line is empty
        if (isDaemon)
            crush(printline);
        os << printline << std::endl << std::flush;
        printline.clear();
    }

    /**
     * @brief Remove extraneous whitespace from a string.
     * @param s The string to clean.
     */
    void crush(std::string &s)
    {
        s = std::regex_replace(s, std::regex("\\s+"), " ");
        s = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");
    }

    /**
     * @brief Get the current timestamp as a string.
     * @return The formatted timestamp.
     */
    std::string getStamp()
    {
        char dts[24];
        time_t t = time(0);
        struct tm *tm = gmtime(&t);
        strftime(dts, sizeof(dts), "%F %T %Z", tm);
        return std::string(dts);
    }
};

#endif // _LOGGING_HPP
