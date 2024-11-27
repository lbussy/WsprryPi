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

from pathlib import Path

"""
@file release_config.py
@brief Configuration settings for the Project Header Update Script.

This script holds configuration constants and flags for controlling
the behavior of the release script, such as logging, dry-run mode,
file extensions, directories to process, and exclusion patterns
for license blocks.

@note This module is part of the WsprryPi project and is referenced by `release.py`.
"""


class Config:
    """
    @class Config
    @brief Configuration class for the Project Header Update Script.

    This class contains all configuration constants and flags that control the behavior of the release script.
    """

    # ============================
    # General Configuration
    # ============================

    DRY_RUN = False
    """
    @var DRY_RUN
    @brief Enables dry-run mode.

    If True, the script will simulate file updates without making actual changes. Useful for testing purposes.
    """

    ENABLE_COMPILATION = True
    """
    @var ENABLE_COMPILATION
    @brief Enables project compilation after updates.

    If True, the script will compile the project after updating all file headers.
    """

    ENABLE_COPY = True
    """
    @var ENABLE_COPY
    @brief Enables copying of executables after compilation.

    If True, executable files specified in `PROJECT_EXES` will be copied to the designated script directory after compilation.
    """

    ENABLE_BACKUP = False
    """
    @var ENABLE_BACKUP
    @brief Enables file backups before updates.

    If True, a `.bak` file will be created for each modified file.
    """

    ENABLE_LOGGING = False
    """
    @var ENABLE_LOGGING
    @brief Enables logging to a file.

    If True, actions will be logged to a file in the directory specified by `LOG_DIR`.
    """

    DEBUG = False
    """
    @var DEBUG
    @brief Enables debug logging.

    If True, detailed debug logs will be generated to assist in troubleshooting.
    """

    # ============================
    # File Processing
    # ============================

    SUPPORTED_EXTENSIONS = [".sh", ".py", ".d", ".h", ".c", ".hpp", ".cpp"]
    """
    @var SUPPORTED_EXTENSIONS
    @brief List of supported file extensions.

    Files with these extensions will be processed by the script.
    """

    DIRECTORIES_TO_PROCESS = ["scripts", "src"]
    """
    @var DIRECTORIES_TO_PROCESS
    @brief List of directories to process.

    Files in these directories will be searched and processed based on their extensions.
    """

    PROJECT_EXES = ["wspr", "wspr.ini"]
    """
    @var PROJECT_EXES
    @brief List of executables to copy after compilation.

    These files will be copied from the `src` directory to the `scripts` directory after successful compilation.
    """

    NAME_TAG = "@LBussy"
    """
    @var NAME_TAG
    @brief Name tag for copyright headers.

    This tag identifies the author in file headers.
    """

    # ============================
    # Logging Configuration
    # ============================

    LOG_DIR = Path(__file__).parent / "logs"
    """
    @var LOG_DIR
    @brief Directory for log files.

    Log files will be stored in this directory.
    """

    LOG_FILE_PREFIX = "release_log_"
    """
    @var LOG_FILE_PREFIX
    @brief Prefix for log file names.

    Log files will include this prefix followed by a timestamp.
    """

    # ============================
    # License Exclusion Configuration
    # ============================

    LICENSE_PATTERNS = [
        r"MIT License",                   # MIT License
        r"GNU General Public License",    # GPL License
        r"Apache License",                # Apache License
        r"BSD License",                   # BSD License
        r"Mozilla Public License",        # Mozilla License
        r"Creative Commons Attribution",  # Creative Commons
        r"Public Domain",                 # Public Domain
    ]
    """
    @var LICENSE_PATTERNS
    @brief Patterns for license exclusions.

    Regular expressions used to detect and exclude open-source license sections from processing.
    """

    @staticmethod
    def get_license_exclusion_patterns():
        """
        @brief Get compiled regex patterns for excluding license sections.

        This method compiles the LICENSE_PATTERNS regular expressions for efficient matching.
        These patterns detect well-known open-source licenses and exclude them from processing.

        @return list
            A list of compiled regular expressions for license detection.
        """
        import re
        return [re.compile(pattern) for pattern in Config.LICENSE_PATTERNS]
