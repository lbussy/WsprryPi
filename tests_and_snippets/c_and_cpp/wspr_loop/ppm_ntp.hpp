// TODO:  Check Doxygen

/**
 * @file ppm_ntp.hpp
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

#ifndef PPM_NTP_HPP
#define PPM_NTP_HPP

#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

extern std::thread ppm_ntp_thread;
extern std::atomic<bool> stop_ppm_ntp_monitor;

extern std::mutex ppm_mtx;

/**
 * @brief Executes a shell command and returns its output.
 *
 * This function runs the specified shell command using `popen` and captures
 * the command's standard output as a string. If the command fails or produces
 * no output, an empty string is returned.
 *
 * @param command The shell command to execute.
 * @return std::string The output of the command, or an empty string if an error occurs.
 *
 * @note Avoid using this function with untrusted input due to potential
 * command injection risks.
 */
extern std::string run_command(const std::string &command);

/**
 * @brief Checks if the system is synchronized with an NTP server.
 *
 * This function runs `ntpq -c 'rv 0'` and searches for the `sync_ntp` keyword
 * in the output, indicating successful NTP synchronization.
 *
 * @return true if the system is synchronized with NTP, false otherwise.
 */
extern bool is_ntp_synchronized();

/**
 * @brief Checks the NTP status by analyzing `ntpq -pn` output.
 *
 * This function parses the output of `ntpq -pn` to identify synchronized
 * servers marked by `*` or `#`. If at least one synchronized server is found,
 * the function returns true.
 *
 * @return true if a synchronized NTP server is found, false otherwise.
 */
extern bool check_ntp_status();

/**
 * @brief Restarts the NTP service and verifies synchronization.
 *
 * This function attempts to restart the NTP service using `systemctl` and
 * verifies if synchronization is restored within a specified retry interval.
 *
 * @return true if synchronization is restored after the restart, false otherwise.
 */
extern bool restart_ntp_service();

/**
 * @brief Ensures that the system maintains stable NTP synchronization.
 *
 * This function checks the NTP status and performs multiple retries using an
 * exponential backoff strategy if synchronization is not detected.
 *
 * @return true if NTP is stable, false if retries are exhausted.
 */
extern bool ensure_ntp_stable();

/**
 * @brief Updates the PPM (parts per million) value based on NTP adjustment.
 *
 * This function retrieves the current NTP frequency adjustment using
 * `ntp_adjtime` and calculates the corresponding PPM value. If a significant
 * change is detected, it updates `config.last_ppm` accordingly.
 *
 * @return true if the PPM value is successfully updated, false otherwise.
 */
extern bool update_ppm();

extern void ppm_ntp_monitor_thread();

#endif // PPM_NTP_HPP
