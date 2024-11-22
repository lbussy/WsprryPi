#ifndef _MONITORFILE_H
#define _MONITORFILE_H

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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
*/

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class MonitorFile
{
public:
    void filemon(const std::string &fileName)
    {
        file_name = fileName.c_str();
        start_monitoring();
    }

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
    void start_monitoring()
    {
        org_time = fs::last_write_time(file_name);
    }

    void check_file()
    {
        new_time = fs::last_write_time(file_name);
    }

    std::string file_name;
    fs::file_time_type org_time;
    fs::file_time_type new_time;
};

#endif // _MONITORFILE_H
