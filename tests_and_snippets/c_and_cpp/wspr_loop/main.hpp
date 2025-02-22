/**
 * @file main.cpp
 * @brief
 *
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 *
 * Copyright (C) @threeme3 (unknown dates)
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * WsprryPi is free software: you can redistribute it and/or modify
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

#ifndef _MAIN_HPP
#define _MAIN_HPP

/**
 * @brief Entry point for the WsprryPi application.
 *
 * This function initializes the application, parses command-line arguments,
 * loads configuration settings from the INI file, verifies NTP synchronization,
 * and starts the main WSPR transmission loop. It also handles system performance
 * settings and signal management for graceful shutdown.
 *
 * @param argc The number of command-line arguments passed to the program.
 * @param argv An array of C-style strings representing the command-line arguments.
 * @return int Exit status: 0 on success, non-zero on failure.
 *
 * @note This function ensures NTP synchronization before starting transmission.
 *       If synchronization fails, the program exits immediately.
 *
 * @see parse_command_line(), validate_config_data(), enable_performance_mode(), wspr_loop()
 */
int main(const int argc, char *const argv[]);

#endif //_MAIN_HPP
