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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-2bf574e [refactoring].
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
