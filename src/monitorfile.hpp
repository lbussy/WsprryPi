/**
 * @file monitorfile.cpp
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

#ifndef _MONITORFILE_HPP
#define _MONITORFILE_HPP

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

/**
 * @class MonitorFile
 * @brief Monitors a file for changes based on its last write time.
 */
class MonitorFile
{
public:
    /**
     * @brief Initializes file monitoring for a specified file.
     * @param fileName The name of the file to monitor.
     */
    void filemon(const std::string &fileName)
    {
        file_name = fileName.c_str();
        start_monitoring();
    }

    /**
     * @brief Checks if the monitored file has been modified since the last check.
     * @return `true` if the file has changed; otherwise, `false`.
     */
    bool changed()
    {
        check_file();
        if (org_time == new_time)
        {
            return false;
        }
        else
        {
            org_time = new_time;
            return true;
        }
    }

private:
    /**
     * @brief Initializes the monitoring process by recording the file's current last write time.
     */
    void start_monitoring()
    {
        org_time = fs::last_write_time(file_name);
    }

    /**
     * @brief Updates the recorded last write time of the monitored file.
     */
    void check_file()
    {
        new_time = fs::last_write_time(file_name);
    }

    std::string file_name;           ///< Name of the file being monitored.
    fs::file_time_type org_time;     ///< Original recorded last write time of the file.
    fs::file_time_type new_time;     ///< Updated last write time of the file.
};

#endif // _MONITORFILE_HPP
