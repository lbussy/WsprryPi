/**
 * @file version.cpp
 * @brief Provides software and hardware version information.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is distributed under under
 * the MIT License. See LICENSE.MIT.md for more information.
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

#include "version.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <sstream>  // For std::stringstream

/**
 * @brief Converts a value or macro to a string.
 * @param x The value or macro to stringify.
 * @return A string representation of the value or macro.
 */
#define stringify(x) #x

/**
 * @brief Converts a macro to a string.
 * @param x The macro to stringify.
 * @return A string representation of the macro.
 */
#define macro_to_string(x) stringify(x)

// Fallback values for version, branch, executable, and project
#ifndef MAKE_TAG
#define MAKE_TAG "unknown"
#endif

#ifndef MAKE_BRH
#define MAKE_BRH "unknown"
#endif

#ifndef MAKE_EXE
#define MAKE_EXE "unknown"
#endif

#ifndef MAKE_PRJ
#define MAKE_PRJ "unknown"
#endif

// Ensure sanitized version of macros (trim excess whitespace)
#define SANITIZED_TAG macro_to_string(MAKE_TAG)
#define SANITIZED_BRH macro_to_string(MAKE_BRH)
#define SANITIZED_EXE macro_to_string(MAKE_EXE)
#define SANITIZED_PRJ macro_to_string(MAKE_PRJ)

const char* exe_version() { return SANITIZED_TAG; }
const char* branch() { return SANITIZED_BRH; }
const char* exe_name() { return SANITIZED_EXE; }
const char* project_name() { return SANITIZED_PRJ; }

// Struct to store the processor type and its corresponding integer value
struct ProcessorMapping {
    const char* type;
    int value;
};

// Array of processor type mappings
const ProcessorMapping processorMappings[] = {
    {"BCM2835", 0},
    {"BCM2836", 1},
    {"BCM2837", 2},
    {"BCM2838", 3},
    {"BCM2711", 3}
};

// Function that returns an integer based on the processor type using an array for easy modification
int getProcessorTypeAsInt() {
    const char* processorType = getProcessorType();

    if (processorType == nullptr) {
        std::cerr << "Failed to get processor type." << std::endl;
        return -1;  // Return -1 to indicate an error
    }

    // Loop through the array to find the processor type and return the corresponding integer
    for (const auto& mapping : processorMappings) {
        if (strcmp(processorType, mapping.type) == 0) {
            return mapping.value;  // Return the associated integer value
        }
    }

    std::cerr << "Unknown processor type: " << processorType << std::endl;
    return -1;  // Return -1 for unknown processor types
}

// Function that returns the processor type as const char*
const char* getProcessorType() {
    std::ifstream cpuinfo("/proc/cpuinfo");

    if (!cpuinfo.is_open()) {
        std::cerr << "Failed to open /proc/cpuinfo file." << std::endl;
        return nullptr;
    }

    std::string line;
    static std::string processorType;  // Static variable to hold the processor type

    // Loop through each line in the file
    while (std::getline(cpuinfo, line)) {
        // Look for the line starting with "Hardware"
        if (line.find("Hardware") == 0) {  // Checks if line starts with "Hardware"
            // Extract the processor type after "Hardware        : "
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                processorType = line.substr(pos + 2);  // Skip ": " after "Hardware"
                break;  // Exit the loop after finding the processor type
            }
        }
    }

    // Close the file
    cpuinfo.close();

    return processorType.c_str();  // Return a const char* pointing to the static string
}

// Function that returns the Raspberry Pi model as const char*
const char* getRaspberryPiModel() {
    std::ifstream cpuinfo("/proc/cpuinfo");

    if (!cpuinfo.is_open()) {
        std::cerr << "Failed to open /proc/cpuinfo file." << std::endl;
        return nullptr;
    }

    std::string line;
    static std::string model;  // Static variable to hold the model string

    // Loop through each line in the file
    while (std::getline(cpuinfo, line)) {
        // Look for the line starting with "Model"
        if (line.find("Model") == 0) {  // Checks if line starts with "Model"
            // Extract the model information after "Model           : "
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                model = line.substr(pos + 2);  // Skip ": " after "Model"
                break;  // Exit the loop after finding the model
            }
        }
    }

    // Close the file
    cpuinfo.close();

    return model.c_str();  // Return a const char* pointing to the static string
}

const char* version_string() {
    static std::string version = std::string(project_name()) + " version " + exe_version() + " (" + branch() + ").";
    return version.c_str();
}

// Function to get the address from device tree
unsigned get_dt_ranges(const char *filename, unsigned offset)
{
   unsigned address = ~0;
   FILE *fp = fopen(filename, "rb");
   if (fp)
   {
      unsigned char buf[4];
      fseek(fp, offset, SEEK_SET);
      if (fread(buf, 1, sizeof buf, fp) == sizeof buf)
         address = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
      fclose(fp);
   }
   return address;
}

// Function to get the BCM host peripheral address
unsigned get_peripheral_address(void)
{
   unsigned address = get_dt_ranges("/proc/device-tree/soc/ranges", 4);
   if (address == 0)
      address = get_dt_ranges("/proc/device-tree/soc/ranges", 8);
   return address == static_cast<unsigned>(~0) ? 0x20000000 : address; // Cast to unsigned
}

#ifdef DEBUG_MAIN_VERSION

/**
 * @brief Main function to test versioning and Raspberry Pi hardware information.
 *
 * This function performs unit tests for the following features:
 * - Executable and project metadata (version, branch, executable name, project name).
 * - Raspberry Pi-specific hardware information (processor ID, version, GPIO base address).
 * - Version string construction.
 *
 * @return Exit status (0 for success, non-zero for failure).
 */
int main()
{
    std::cout << "===========================\n";
    std::cout << "Testing Version Functions...\n";
    std::cout << "===========================\n";

    // Test executable and branch macros
    std::cout << "Project Name: " << project_name() << "\n";
    std::cout << "Executable Name: " << exe_name() << "\n";
    std::cout << "Executable Version: " << exe_version() << "\n";
    std::cout << "Branch: " << branch() << "\n";

    // Test full version string
    std::cout << "Full Version String: " << version_string() << "\n";

    // Test Raspberry Pi-specific hardware information
    int processorId = getProcessorTypeAsInt();
    std::cout << "\nHardware Information:\n";
    std::cout << "Processor ID: " << processorId << "\n";

    const char* procType = getProcessorType();
    std::cout << "Processor type: " << procType << "\n";

    const char* piVersion = getRaspberryPiModel();
    std::cout << "Raspberry Pi Version: " << piVersion << "\n";

    unsigned gpioBaseAddr = get_peripheral_address();
    std::cout << "GPIO Base Address: 0x" << std::hex << gpioBaseAddr << std::dec << "\n";

    // End of tests
    std::cout << "\n===========================\n";
    std::cout << "Version function tests completed successfully.\n";
    std::cout << "===========================\n";

    return 0;
}

#endif // DEBUG_MAIN_VERSION
