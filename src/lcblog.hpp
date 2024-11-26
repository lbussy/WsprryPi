// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file lcblog.hpp
 * @brief Provides unit tests for the LCBLog class.
 *
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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-9f5cad8 [refactoring].
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
     * @brief Enable or disable daemon mode for logging.
     * @param daemonmode True to enable daemon mode; false otherwise.
     */
    void setDaemon(bool daemonmode)
    {
        isDaemon = daemonmode;
    }

    /**
     * @brief Log a single message to standard output.
     * @tparam T The type of the message to log.
     * @param t The message to log.
     */
    template <typename T>
    void logS(T t) {
        if (!printline.empty()) { // If there's an incomplete log message, flush it first
            finalizeOutput(std::cout);
        }

        if (isDaemon) {
            std::cout << getStamp() << "\t"; // Add timestamp and tab for Daemon mode
        }
        prtStd(t);
        finalizeOutput(std::cout);
    }

    /**
     * @brief Log multiple concatenated messages to standard output.
     * @tparam T The type of the first message to log.
     * @tparam Args The types of additional messages to log.
     * @param t The first message to log.
     * @param args Additional messages to log.
     */
    template <typename T, typename... Args>
    void logS(T t, Args... args) {
        if (!printline.empty()) { // Flush any pending messages before starting new one
            finalizeOutput(std::cout);
        }

        if (isDaemon) {
            std::cout << getStamp() << "\t"; // Add timestamp and tab for Daemon mode
        }
        prtStd(t, args...);
        finalizeOutput(std::cout);
    }

    /**
     * @brief Log a single message to standard error.
     * @tparam T The type of the message to log.
     * @param t The message to log.
     */
    template <typename T>
    void logE(T t)
    {
        if (isDaemon)
            std::cerr << getStamp() << "\t";
        prtStd(t);
        finalizeOutput(std::cerr);
    }

    /**
     * @brief Log multiple concatenated messages to standard error.
     * @tparam T The type of the first message to log.
     * @tparam Args The types of additional messages to log.
     * @param t The first message to log.
     * @param args Additional messages to log.
     */
    template <typename T, typename... Args>
    void logE(T t, Args... args)
    {
        if (isDaemon)
            std::cerr << getStamp() << "\t";
        prtStd(t, args...);
        finalizeOutput(std::cerr);
    }

    /**
     * @brief Test the `crush` function directly with a given string.
     * @param s The string to process using `crush`.
     */
    void testCrush(std::string &s)
    {
        crush(s);
    }

private:
    bool isDaemon = false; /**< Indicates whether daemon mode is enabled. */
    std::string printline = ""; /**< Accumulated log message. */

    /**
     * @brief Append a message to the log buffer.
     * @param t The message to append.
     */
    void prtStd(const std::string &t)
    {
        if (!printline.empty())
            printline += " ";
        printline += t;
    }

    /**
     * @brief Append a message to the log buffer.
     * @param t The message to append.
     */
    void prtStd(const char *t)
    {
        if (!printline.empty())
            printline += " ";
        printline += t;
    }

    /**
     * @brief Append a numeric message to the log buffer.
     * @tparam T The numeric type of the message.
     * @param t The numeric message to append.
     */
    template <typename T>
    void prtStd(T t)
    {
        if (!printline.empty())
            printline += " ";
        printline += std::to_string(t);
    }

    /**
     * @brief Append multiple concatenated messages to the log buffer.
     * @tparam T The type of the first message.
     * @tparam Args The types of additional messages.
     * @param t The first message.
     * @param args Additional messages.
     */
    template <typename T, typename... Args>
    void prtStd(T t, Args... args)
    {
        prtStd(t);
        prtStd(args...);
    }

    /**
     * @brief Finalize and print the accumulated log message to the output stream.
     * @param os The output stream (e.g., std::cout or std::cerr).
     */
    void finalizeOutput(std::ostream &os) {
        if (printline.empty())
            return;

        crush(printline); // Clean up the accumulated log message
        os << printline << std::endl << std::flush; // Ensure newline
        printline.clear(); // Clear the buffer after output
    }

    /**
     * @brief Clean up a string by removing extraneous spaces and fixing formatting issues.
     * @param s The string to clean.
     */
    void crush(std::string &s)
    {
        // Trim leading and trailing spaces
        s = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");

        // Replace multiple spaces with a single space
        s = std::regex_replace(s, std::regex("\\s+"), " ");

        // Remove spaces directly inside parentheses/brackets
        s = std::regex_replace(s, std::regex("\\(\\s+"), "(");
        s = std::regex_replace(s, std::regex("\\s+\\)"), ")");
        s = std::regex_replace(s, std::regex("\\[\\s+"), "[");
        s = std::regex_replace(s, std::regex("\\s+\\]"), "]");

        // Remove spaces before punctuation
        s = std::regex_replace(s, std::regex("\\s+([.,!?])"), "$1");

        // Add space before opening parentheses/brackets if missing
        s = std::regex_replace(s, std::regex("([a-zA-Z0-9])\\("), "$1 (");
        s = std::regex_replace(s, std::regex("([a-zA-Z0-9])\\["), "$1 [");

        // Remove spaces before closing parentheses/brackets
        s = std::regex_replace(s, std::regex("\\s+([)}\\]])"), "$1");

        // Final cleanup
        s = std::regex_replace(s, std::regex("\\s+"), " ");
        s = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");
    }

    /**
     * @brief Get the current timestamp formatted as a string.
     * @return A string representing the current timestamp in the format "YYYY-MM-DD HH:MM:SS TZ".
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
