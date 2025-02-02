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

// Unit tests:
// g++ -std=c++17 -DDEBUG_MAIN_MONITORFILE monitorfile.cpp -o monitorfile
// Test command:
// ./monitorfile

#include "monitorfile.hpp"
#include <fstream> // Required for std::ofstream
#include <thread>
#include <chrono>

#ifdef DEBUG_MAIN_MONITORFILE

/**
 * @brief Main function for debugging and testing the MonitorFile class.
 */
int main()
{
    MonitorFile monitor;
    const std::string testFileName = "testfile.txt";

    // Create or modify the test file for demonstration purposes
    std::ofstream testFile(testFileName);
    testFile << "Initial content.\n";
    testFile.close();

    monitor.filemon(testFileName);

    std::cout << "Monitoring file: " << testFileName << "\n";

    for (int i = 0; i < 5; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Modify the file to trigger a change
        if (i % 2 == 1)
        {
            std::ofstream modifyFile(testFileName, std::ios::app);
            modifyFile << "Modification " << i << "\n";
            modifyFile.close();
        }

        if (monitor.changed())
        {
            std::cout << "File has been modified!\n";
        }
        else
        {
            std::cout << "No changes detected.\n";
        }
    }

    // Cleanup: Delete the test file
    fs::remove(testFileName);

    return 0;
}

#endif // DEBUG_MAIN_MONITORFILE
