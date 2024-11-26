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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-218cc4d [refactoring].
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
