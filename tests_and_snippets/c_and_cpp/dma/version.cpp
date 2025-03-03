/**
 * @file version.cpp
 * @brief Provides software and hardware version information.
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

// Primary header for this source file
#include "version.hpp"

// Standard library headers
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

/**
 * @brief Converts a value to a string view.
 *
 * This function takes a value and returns a `std::string_view` representation.
 * It is typically used for converting macro values or constant expressions into
 * a more manageable and efficient string view.
 *
 * @tparam T The type of the input value (usually a string literal or macro).
 * @param value The value or macro to convert.
 * @return A `std::string_view` representing the input value.
 */
template <typename T>
constexpr std::string_view to_string_view(T value) noexcept
{
    return std::string_view(value);
}

// Fallback definitions for version, branch, executable, and project
#ifndef MAKE_TAG
#define MAKE_TAG "unknown" ///< Fallback for the build tag.
#endif

#ifndef MAKE_BRH
#define MAKE_BRH "unknown" ///< Fallback for the branch name.
#endif

#ifndef MAKE_EXE
#define MAKE_EXE "unknown" ///< Fallback for the executable name.
#endif

#ifndef MAKE_PRJ
#define MAKE_PRJ "unknown" ///< Fallback for the project name.
#endif

// Compile-time string views for sanitized values
constexpr std::string_view SANITIZED_TAG = to_string_view(MAKE_TAG); ///< Sanitized build tag.
constexpr std::string_view SANITIZED_BRH = to_string_view(MAKE_BRH); ///< Sanitized branch name.
constexpr std::string_view SANITIZED_EXE = to_string_view(MAKE_EXE); ///< Sanitized executable name.
constexpr std::string_view SANITIZED_PRJ = to_string_view(MAKE_PRJ); ///< Sanitized project name.

/**
 * @brief Retrieves the executable version.
 *
 * This function returns the version of the executable as a string.
 * If the `MAKE_TAG` macro is not defined, it returns "unknown".
 *
 * @return A `std::string` representing the executable version.
 */
std::string exe_version()
{
    return std::string(SANITIZED_TAG);
}

/**
 * @brief Retrieves the current branch name.
 *
 * This function returns the name of the Git branch associated with the build.
 * If the `MAKE_BRH` macro is not defined, it returns "unknown".
 *
 * @return A `std::string` representing the branch name.
 */
std::string branch()
{
    return std::string(SANITIZED_BRH);
}

/**
 * @brief Retrieves the executable name.
 *
 * This function returns the name of the executable as defined by the build system.
 * If the `MAKE_EXE` macro is not defined, it returns "unknown".
 *
 * @return A `std::string` representing the executable name.
 */
std::string exe_name()
{
    return std::string(SANITIZED_EXE);
}

/**
 * @brief Retrieves the project name.
 *
 * This function returns the project name as defined by the build system.
 * If the `MAKE_PRJ` macro is not defined, it returns "unknown".
 *
 * @return A `std::string` representing the project name.
 */
std::string project_name()
{
    return std::string(SANITIZED_PRJ);
}

/**
 * @brief Retrieves the current debug state based on the build configuration.
 *
 * This function determines whether the current build is a debug or release
 * build by checking the presence of the `DEBUG_BUILD` macro. If the macro is
 * defined, it returns `"DEBUG"`. Otherwise, it returns `"INFO"` as the default
 * log level for non-debug builds.
 *
 * @return std::string The current debug state, either "DEBUG" or "INFO".
 *
 * @note The `DEBUG_BUILD` macro must be defined during compilation for the
 *       debug state to be set correctly. Use `-DDEBUG_BUILD` in the Makefile
 *       or build system when compiling a debug target.
 *
 * @example
 * std::string debug_state = get_debug_state();
 * std::cout << "Current debug state: " << debug_state << std::endl;
 */
std::string get_debug_state()
{
#ifdef DEBUG_BUILD
    return "DEBUG"; // Debug mode enabled.
#else
    return "INFO"; // Default to INFO for non-debug builds.
#endif
}

/**
 * @brief Represents a mapping between a processor type and its corresponding integer value.
 *
 * This structure is used to associate a specific processor type, represented
 * as a string, with an integer value. It is useful for identifying and
 * categorizing different Raspberry Pi processor models or other devices.
 *
 * @struct ProcessorMapping
 * @var type
 *      The processor type as a string (e.g., "BCM2835", "BCM2711").
 * @var value
 *      The corresponding integer value representing the processor type.
 *
 * @example
 * ```cpp
 * ProcessorMapping mapping = {"BCM2835", 1};
 * std::cout << "Processor: " << mapping.type << ", Value: " << mapping.value << std::endl;
 * ```
 */
struct ProcessorMapping
{
    std::string type; ///< The processor type (e.g., "BCM2835").
    int value;        ///< Integer value representing the processor type.
};

/**
 * @brief Array of processor type mappings for Raspberry Pi models.
 *
 * This array associates specific Raspberry Pi processor types with corresponding
 * integer values. It is used to identify the processor type and facilitate
 * conditional logic based on the hardware platform.
 *
 * Each entry in the array represents a known Broadcom processor used in
 * Raspberry Pi boards, with an associated integer value for easy reference.
 *
 * @var processorMappings
 * @details The integer value typically represents the generation or family of the
 * processor. For example, the BCM2835 corresponds to the original Raspberry Pi
 * models, while the BCM2711 is used in Raspberry Pi 4 and later.
 */
const ProcessorMapping processorMappings[] = {
    {"BCM2835", 0}, ///< Raspberry Pi 1 and Zero series
    {"BCM2836", 1}, ///< Raspberry Pi 2 series
    {"BCM2837", 2}, ///< Raspberry Pi 3 series
    {"BCM2838", 3}, ///< Raspberry Pi 4 and Pi 400 series (alias)
    {"BCM2711", 3}  ///< Raspberry Pi 4 and Pi 400 series
};

std::string sanitizeString(const std::string &str)
{
    std::string result;
    size_t first = std::string::npos, last = std::string::npos;

    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] != '\0') // Remove null characters
        {
            if (first == std::string::npos && str[i] != ' ' && str[i] != '\t' && str[i] != '\r' && str[i] != '\n')
                first = i; // First non-whitespace character

            if (str[i] != ' ' && str[i] != '\t' && str[i] != '\r' && str[i] != '\n')
                last = i; // Last non-whitespace character
        }
    }

    if (first == std::string::npos || last == std::string::npos)
        return ""; // String is all spaces or nulls

    for (size_t i = first; i <= last; ++i)
    {
        if (str[i] != '\0') // Avoid null characters
            result += str[i];
    }

    return result;
}

/**
 * @brief Retrieves the processor type as an integer value.
 *
 * This function determines the Raspberry Pi's processor type and returns an
 * associated integer value based on predefined mappings in the
 * `processorMappings` array. If the processor type cannot be identified or
 * is unknown, the function returns -1.
 *
 * @return int The integer value corresponding to the detected processor type,
 *             or -1 if the type is unknown or cannot be determined.
 *
 * @details The integer value returned corresponds to the generation or family
 *          of the Broadcom processor used in Raspberry Pi models. For example:
 *          - BCM2835 returns 0 (Raspberry Pi 1 and Zero)
 *          - BCM2836 returns 1 (Raspberry Pi 2)
 *          - BCM2837 returns 2 (Raspberry Pi 3)
 *          - BCM2711 returns 3 (Raspberry Pi 4 and Pi 400)
 *
 * @throws None This function does not throw exceptions but returns -1 if an
 *         error occurs.
 */
int getProcessorTypeAsInt()
{
    // Retrieve the processor type as a string
    std::string processorType = getProcessorType();

    // Check if the processor type is empty (indicating an error)
    if (processorType.empty())
    {
        std::cerr << "Failed to get processor type." << std::endl;
        return -1; ///< Return -1 to indicate an error
    }

    // Search for the processor type in the known mappings
    for (const auto &mapping : processorMappings)
    {
        if (processorType == mapping.type)
        {
            return mapping.value; ///< Return the associated integer value
        }
    }

    // If the type is not found, log a warning and return -1
    std::cerr << "Unknown processor type: " << processorType << std::endl;
    return -1;
}

/**
 * @brief Retrieves the processor type as a string.
 *
 * This function reads the `/sys/firmware/devicetree/base/compatible` file to
 * determine the processor type of the Raspberry Pi. It extracts the CPU model
 * (such as "BCM2835", "BCM2711") from the device tree and returns it as an
 * uppercase string.
 *
 * @return std::string The detected processor type, such as "BCM2835",
 *         "BCM2711", etc. If the processor type cannot be determined,
 *         it returns "Unknown CPU".
 *
 * @details The function reads the first line of the compatible file, searches
 *          for the "bcm" keyword, replaces commas with spaces for readability,
 *          and converts the result to uppercase for consistency.
 *
 * @throws None This function does not throw exceptions but returns an error
 *         message if the file cannot be read.
 */
std::string getProcessorType()
{
    static std::string cpuModel;

    std::ifstream dtCompatible("/sys/firmware/devicetree/base/compatible");
    if (!dtCompatible.is_open())
    {
        std::cerr << "Failed to open /sys/firmware/devicetree/base/compatible file." << std::endl;
        return "UNKNOWN CPU";
    }

    std::getline(dtCompatible, cpuModel);
    dtCompatible.close();

    // Remove spaces, newlines and null characters
    cpuModel = sanitizeString(cpuModel);

    // Find "bcm" substring
    size_t start = cpuModel.find("bcm");
    if (start != std::string::npos)
    {
        cpuModel = cpuModel.substr(start);
        std::replace(cpuModel.begin(), cpuModel.end(), ',', ' ');

        std::transform(cpuModel.begin(), cpuModel.end(), cpuModel.begin(), ::toupper);
    }
    else
    {
        return "UNKNOWN CPU";
    }

    std::cout << "Detected CPU Model (Processed): [" << cpuModel << "]" << std::endl; // Debug
    return cpuModel;
}

/**
 * @brief Retrieves the Raspberry Pi model as a string.
 *
 * This function reads the `/proc/device-tree/model` file to determine the
 * specific Raspberry Pi model. It returns the model as a string.
 *
 * @return std::string The Raspberry Pi model. If the model cannot be
 *         determined, an empty string is returned.
 *
 * @details
 * The function opens the `/proc/device-tree/model` file and reads the
 * first line, which typically contains the model description. If the file
 * cannot be opened, an error is logged, and an empty string is returned.
 *
 * @example
 * ```cpp
 * std::string piModel = getRaspberryPiModel();
 * std::cout << "Raspberry Pi Model: " << piModel << std::endl;
 * ```
 */
std::string getRaspberryPiModel()
{
    static std::string model; // Static variable to cache the model string.
    std::ifstream modelFile("/proc/device-tree/model");

    // Check if the file opened successfully.
    if (!modelFile.is_open())
    {
        std::cerr << "Failed to open /proc/device-tree/model file." << std::endl;
        return ""; // Return an empty string instead of nullptr.
    }

    // Read the first line from the file.
    std::getline(modelFile, model);
    modelFile.close();

    // Return the cached model string.
    return model;
}

/**
 * @brief Constructs a formatted version string for the project.
 *
 * This function combines the project name, executable version, and branch name
 * into a human-readable version string. It is useful for logging and version
 * identification purposes.
 *
 * @return std::string A formatted string containing project version information.
 * @example Example output: "MyProject version 1.2.3 (main)."
 */
std::string version_string()
{
    // Retrieve project details.
    std::string proj = project_name(); ///< Project name.
    std::string ver = exe_version();   ///< Executable version.
    std::string br = branch();         ///< Git branch name.

    // Construct and return the formatted version string.
    return proj + " version " + ver + " (" + br + ").";
}

/**
 * @brief Retrieves a 32-bit address from the device tree.
 *
 * This function reads a 4-byte address from a specified offset within a file,
 * commonly used to query memory-mapped addresses from the device tree.
 *
 * @param filename The path to the device tree file to read from.
 * @param offset The offset (in bytes) within the file to read the address.
 * @return unsigned The 32-bit address read from the file, or `~0` (all bits set)
 *         if the read operation fails.
 *
 * @details
 * The function opens the specified file in binary mode, seeks to the given offset,
 * and reads 4 bytes representing an address in big-endian format. If the read
 * operation succeeds, the address is returned as an unsigned integer. If any
 * operation fails, the function returns `~0` (equivalent to `0xFFFFFFFF`).
 */
unsigned get_dt_ranges(const char *filename, unsigned offset)
{
    unsigned address = ~0; ///< Default to an invalid address (all bits set).

    // Open the specified file in binary read mode.
    FILE *fp = fopen(filename, "rb");
    if (fp)
    {
        unsigned char buf[4]; ///< Buffer to hold the 4-byte address.

        // Seek to the specified offset within the file.
        if (fseek(fp, offset, SEEK_SET) == 0)
        {
            // Read 4 bytes and construct the address if successful.
            if (fread(buf, 1, sizeof(buf), fp) == sizeof(buf))
            {
                address = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
            }
        }

        // Close the file after reading.
        fclose(fp);
    }

    return address;
}

/**
 * @brief Retrieves the BCM host peripheral base address from the device tree.
 *
 * This function queries the device tree to determine the base address of the
 * Broadcom (BCM) host peripherals, which is essential for low-level hardware
 * access on Raspberry Pi systems.
 *
 * @return unsigned The base address of the BCM host peripherals.
 *         Returns `0x20000000` if the address cannot be determined.
 *
 * @details
 * The function first attempts to read the address from the `/proc/device-tree/soc/ranges`
 * file at offset 4. If that fails (returns 0), it tries again at offset 8. If both attempts
 * fail, it falls back to the default base address `0x20000000` (common for older Raspberry Pi models).
 */
unsigned get_peripheral_address(void)
{
    // Attempt to get the peripheral address from the device tree at offset 4.
    unsigned address = get_dt_ranges("/proc/device-tree/soc/ranges", 4);

    // If the address is zero, try again at offset 8.
    if (address == 0)
    {
        address = get_dt_ranges("/proc/device-tree/soc/ranges", 8);
    }

    // If still invalid, return the default base address (0x20000000).
    return (address == static_cast<unsigned>(~0)) ? 0x20000000 : address;
}
