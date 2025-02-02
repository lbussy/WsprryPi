/**
 * @file version.hpp
 * @brief Provides versioning and Raspberry Pi-specific hardware information.
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

#ifndef VERSION_H
#define VERSION_H

#include <bcm_host.h>
#include <string>

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

/**
 * @brief Retrieves the executable's version tag from the build system.
 * @return A C-string representing the version tag.
 */
inline const char* exe_version() { return SANITIZED_TAG; }

/**
 * @brief Retrieves the branch name from the build system.
 * @return A C-string representing the branch name.
 */
inline const char* branch() { return SANITIZED_BRH; }

/**
 * @brief Retrieves the executable name from the build system.
 * @return A C-string representing the executable name.
 */
inline const char* exe_name() { return SANITIZED_EXE; }

/**
 * @brief Retrieves the project name from the build system.
 * @return A C-string representing the project name.
 */
inline const char* project_name() { return SANITIZED_PRJ; }

/**
 * @brief Constructs a full version string.
 * @return A C-string representing the full version string.
 */
inline const char* version_string() {
    static std::string version = std::string(project_name()) + " version " + exe_version() + " (" + branch() + ").";
    return version.c_str();
}

/**
 * @brief Retrieves the processor ID of the Raspberry Pi.
 * @return An integer representing the processor ID.
 */
inline int ver() { return bcm_host_get_processor_id(); }

/**
 * @brief Provides a description of the Raspberry Pi model based on its processor ID.
 *
 * This function maps processor IDs to descriptive text for different Raspberry Pi models.
 *
 * @return A C-string describing the Raspberry Pi version or "Unknown Raspberry Pi Version" if the ID is invalid.
 */
inline const char* RPiVersion()
{
    static const char* vertext[] = {
        "Raspberry Pi 1 or Zero Model (BCM2835)",
        "Raspberry Pi 2B (BCM2836)",
        "Raspberry Pi 2B or 3B (BCM2837)",
        "Raspberry Pi 4 (BCM2711)"
    };

    int id = ver();
    if (id < 0 || id >= static_cast<int>(sizeof(vertext) / sizeof(vertext[0]))) {
        return "Unknown Raspberry Pi Version";
    }

    return vertext[id];
}

/**
 * @brief Retrieves the GPIO base address for the Raspberry Pi.
 * @return An unsigned integer representing the GPIO base address.
 */
inline unsigned gpioBase() { return bcm_host_get_peripheral_address(); }

#endif // VERSION_H
