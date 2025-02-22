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

#ifndef VERSION_H
#define VERSION_H

#include <string>

/**
 * @brief Retrieves the executable version.
 *
 * This function returns the version of the executable as a string.
 * If the `MAKE_TAG` macro is not defined, it returns "unknown".
 *
 * @return A `std::string` representing the executable version.
 */
extern std::string exe_version();

/**
 * @brief Retrieves the current branch name.
 *
 * This function returns the name of the Git branch associated with the build.
 * If the `MAKE_BRH` macro is not defined, it returns "unknown".
 *
 * @return A `std::string` representing the branch name.
 */
extern std::string branch();

/**
 * @brief Retrieves the executable name.
 *
 * This function returns the name of the executable as defined by the build system.
 * If the `MAKE_EXE` macro is not defined, it returns "unknown".
 *
 * @return A `std::string` representing the executable name.
 */
extern std::string exe_name();

/**
 * @brief Retrieves the project name.
 *
 * This function returns the project name as defined by the build system.
 * If the `MAKE_PRJ` macro is not defined, it returns "unknown".
 *
 * @return A `std::string` representing the project name.
 */
extern std::string project_name();

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
extern std::string get_debug_state();

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
extern int getProcessorTypeAsInt();

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
extern std::string getProcessorType();

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
extern std::string getRaspberryPiModel();

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
extern std::string version_string();

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
extern unsigned get_dt_ranges(const char *filename, unsigned offset);

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
extern unsigned get_peripheral_address(void);

/**
 * @brief Retrieves the default (non-throttled) CPU frequency for the current processor type.
 *
 * This function identifies the Raspberry Pi processor type and returns the
 * nominal base operating frequency in Hertz, used as a reference for detecting
 * CPU throttling.
 *
 * @return int The default CPU frequency in Hertz. Returns 0 if the processor type
 * is unknown or if an error occurs while reading the device tree.
 */
extern int getDefaultCpuFrequencyHz();

#endif // VERSION_H
