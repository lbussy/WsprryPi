/**
 * @file ppm_ntp.cpp
 * @brief Handles periodic NTP time synchronization duties as well as clock
 * calibration.
 *
 * This file is part of WsprryPi, a project originally created from @threeme3
 * WsprryPi projet (no longer on GitHub). However, now the original code
 * remains only as a memory and inspiration, and this project is no longer
 * a deriivative work.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
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

// Primary header for this source file
#include "ppm_ntp.hpp"

// Project headers
#include "arg_parser.hpp"

// Standard library headers
#include <array>
#include <cstring>
#include <string>

// System headers
#include <sys/timex.h>

/**
 * @brief Executes a shell command and returns the output as a string.
 *
 * This function runs the provided shell command using `popen` and captures
 * its standard output. If the command fails or produces no output, an empty
 * string is returned.
 *
 * @param command The shell command to execute.
 * @return std::string The output of the command, or an empty string if an error occurs.
 *
 * @note This function should not be used with untrusted input due to potential
 * command injection risks.
 */
std::string run_command(const std::string &command)
{
    std::array<char, 128> buffer{};
    std::string result;
    FILE *pipe = popen(command.c_str(), "r");

    if (!pipe)
    {
        llog.logE(ERROR, "Failed to run command:", command);
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }

    int status = pclose(pipe);
    if (status == -1)
    {
        llog.logE(ERROR, "Failed to close pipe for command:", command);
        return "";
    }

    return result.empty() ? "" : result;
}

/**
 * @brief Checks if the system is synchronized with an NTP server.
 *
 * This function runs `ntpq -c 'rv 0'` and checks for the `sync_ntp` keyword
 * in the output, indicating successful synchronization.
 *
 * @return true if the system is synchronized with NTP, false otherwise.
 */
bool is_ntp_synchronized()
{
    try
    {
        std::string result = run_command("ntpq -c 'rv 0'");

        if (result.find("sync_ntp") != std::string::npos)
        {
            llog.logS(INFO, "NTP is synchronized.");
            return true;
        }

        llog.logE(WARN, "NTP is NOT synchronized.");
        return false;
    }
    catch (const std::exception &e)
    {
        llog.logE(FATAL, "Failed to check NTP synchronization:", e.what());
        return false;
    }
}

/**
 * @brief Checks the NTP status by analyzing `ntpq -pn` output.
 *
 * This function parses the output of `ntpq -pn` to identify synchronized
 * servers marked by `*` or `#`. If at least one server is synchronized,
 * it returns true.
 *
 * @return true if a synchronized NTP server is found, false otherwise.
 */
bool check_ntp_status()
{
    std::string output = run_command("ntpq -pn");

    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.empty() || line.find("remote") != std::string::npos || line.find("=") != std::string::npos)
            continue;

        std::istringstream fields(line);
        std::vector<std::string> tokens;
        std::string token;

        while (fields >> token)
            tokens.push_back(token);

        if (!tokens.empty() && (tokens[0][0] == '*' || tokens[0][0] == '#'))
        {
            std::string sync_flag = tokens[0].substr(0, 1); // * or #
            std::string remote = tokens[0].substr(1);       // IP address

            tokens[0] = sync_flag;
            tokens.insert(tokens.begin() + 1, remote);

            if (tokens.size() >= 11)
            {
                std::string type = tokens[4];
                std::string when = tokens[5];
                std::string poll = tokens[6];
                std::string reach = tokens[7];
                std::string delay = tokens[8];
                std::string offset = tokens[9];
                std::string jitter = tokens[10];

                llog.logS(DEBUG, "NTP synchronized server:", remote, "\nNTP Offset:", offset,
                          "ms \nNTP Jitter:", jitter, "ms \nNTP Reach:", reach);
                llog.logS(INFO, "NTP synchronization verified.");
                return true;
            }
            else
            {
                llog.logE(WARN, "NTP insufficient fields in line:", line);
            }
        }
    }

    llog.logE(FATAL, "No synchronized NTP server found.");
    return false;
}

/**
 * @brief Restarts the NTPsec service and verifies synchronization.
 *
 * This function attempts to restart the NTPsec service and verifies
 * if synchronization is restored within a specified retry interval.
 *
 * @return true if synchronization is restored after the restart, false otherwise.
 */
bool restart_ntp_service()
{
    llog.logS(WARN, "Restarting NTPsec service...");

    if (system("sudo systemctl restart ntpsec") != 0)
    {
        llog.logE(ERROR, "Failed to restart NTPsec service.");
        return false;
    }

    llog.logS(INFO, "NTPsec service restarted. Verifying synchronization...");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (int i = 0; i < 5; ++i)
    {
        if (check_ntp_status())
        {
            llog.logS(INFO, "NTP synchronization restored after restart.");
            return true;
        }

        llog.logS(WARN, "Waiting for NTP synchronization...");
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    llog.logE(FATAL, "Failed to restore NTP synchronization after restart.");
    return false;
}

/**
 * @brief Ensures NTP stability by checking status with retries.
 *
 * This function performs multiple retries to ensure NTP synchronization,
 * using an exponential backoff strategy.
 *
 * @return true if NTP is stable, false if retries are exhausted.
 */
bool ensure_ntp_stable()
{
    constexpr int max_retries = 3;
    constexpr int initial_retry_interval = 10;
    int current_retry_interval = initial_retry_interval;

    for (int attempt = 1; attempt <= max_retries; ++attempt)
    {
        if (check_ntp_status())
        {
            return true;
        }

        llog.logE(WARN, "NTP check failed (attempt", attempt, "of", max_retries, ").");

        if (attempt < max_retries)
        {
            llog.logS(INFO, "Retrying NTP check in", current_retry_interval, "seconds...");
            std::this_thread::sleep_for(std::chrono::seconds(current_retry_interval));
            current_retry_interval *= 2;
        }
        else
        {
            llog.logE(FATAL, "NTP check failed after", max_retries, "attempts. Exiting.");
            std::exit(EXIT_FAILURE);
        }
    }

    return false;
}

/**
 * @brief Updates the PPM (parts per million) value based on NTP adjustment.
 *
 * This function retrieves the NTP frequency adjustment using `ntp_adjtime`
 * and calculates the PPM value. It updates `config.last_ppm` if a significant
 * change is detected.
 *
 * @return true if the PPM value is successfully updated, false otherwise.
 */
bool update_ppm()
{
    struct timex ntx{};
    errno = 0;

    int status = ntp_adjtime(&ntx);
    if (status == -1)
    {
        llog.logE(FATAL, "Failed to adjust NTP time for PPM check:", std::string(strerror(errno)));
        return false;
    }

    double ppm_new = static_cast<double>(ntx.freq) / (1 << 16);
    if (std::abs(config.last_ppm - ppm_new) > std::numeric_limits<double>::epsilon())
    {
        llog.logS(INFO, "PPM check new value obtained:", std::fixed, std::setprecision(10), ppm_new);
        config.last_ppm = ppm_new;
    }
    else
    {
        llog.logS(DEBUG, "PPM value unchanged at:", std::fixed, std::setprecision(10), ppm_new);
    }

    return true;
}
