/**
 * @file monitorfile.cpp
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
