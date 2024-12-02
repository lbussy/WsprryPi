#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# This file is part of WsprryPi.
#
# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
#
# WsprryPi is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

"""
WsprryPi - Header Update Configuration Script

This script is part of the WsprryPi project and manages configurations for updating
headers in project files, including dry-run functionality, logging, and license exclusions.

Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
"""

from pathlib import Path

class Config:
    """
    Configuration class for the WsprryPi project header update script.

    This class contains all configuration constants and methods for controlling the behavior
    of the script, including logging, compilation, file processing, and license exclusions.

    Attributes
    ----------
    DRY_RUN : bool
        If True, simulates updates without making changes to files.
    ENABLE_COMPILATION : bool
        If True, enables project compilation after updates.
    ENABLE_COPY : bool
        If True, copies executables after compilation.
    ENABLE_BACKUP : bool
        If True, creates backups of modified files.
    ENABLE_LOGGING : bool
        If True, logs actions to a file.
    DEBUG : bool
        If True, enables detailed debug logging.
    SUPPORTED_EXTENSIONS : list
        List of file extensions to process.
    DIRECTORIES_TO_PROCESS : list
        Directories to scan for files to update.
    PROJECT_EXES : list
        List of project executables to copy post-compilation.
    NAME_TAG : str
        Name tag used in copyright headers.
    LOG_DIR : Path
        Directory for storing log files.
    LOG_FILE_PREFIX : str
        Prefix for log file names.
    LICENSE_PATTERNS : list
        Regular expressions to exclude license blocks from updates.

    Methods
    -------
    get_license_exclusion_patterns():
        Compiles and returns regex patterns for license exclusion.
    """

    # General Configuration
    DRY_RUN = True
    ENABLE_COMPILATION = True
    ENABLE_COPY = True
    ENABLE_BACKUP = False
    ENABLE_LOGGING = False
    DEBUG = True

    # File Processing Configuration
    SUPPORTED_EXTENSIONS = [".sh", ".py", ".d", ".h", ".c", ".hpp", ".cpp"]
    DIRECTORIES_TO_PROCESS = ["scripts", "src"]
    PROJECT_EXES = ["wspr", "wspr.ini"]
    NAME_TAG = "@LBussy"

    # Logging Configuration
    LOG_DIR = Path(__file__).parent / "logs"
    LOG_FILE_PREFIX = "release_log_"

    # License Exclusion Configuration
    LICENSE_PATTERNS = [
        r"MIT License",
        r"GNU General Public License",
        r"Apache License",
        r"BSD License",
        r"Mozilla Public License",
        r"Creative Commons Attribution",
        r"Public Domain",
    ]

    @staticmethod
    def get_license_exclusion_patterns():
        """
        Compile regex patterns for excluding license sections.

        Compiles LICENSE_PATTERNS into regex objects for efficient matching
        during file processing to exclude open-source license blocks.

        Returns
        -------
        list
            A list of compiled regex patterns.
        """
        import re
        return [re.compile(pattern) for pattern in Config.LICENSE_PATTERNS]


def main():
    """
    Main function to list all properties of the Config class.

    Iterates through all attributes of the Config class and prints their names and values.
    """
    print("Listing all configuration properties of the Config class:")
    for attribute in dir(Config):
        # Filter out methods and special attributes
        if not attribute.startswith("__") and not callable(getattr(Config, attribute)):
            value = getattr(Config, attribute)
            print(f"{attribute}: {value}")


if __name__ == "__main__":
    main()
