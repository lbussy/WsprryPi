/**
 * @file monitorfile.hpp
 * @brief Monitors a file on disk and triggers an action when it is changed.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is licensed under the
 * MIT License. See LICENSE.MIT.md for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
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

// TODO:  Export this to a submodule

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
