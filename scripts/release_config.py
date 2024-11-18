"""
Configuration File for Project Header Update Script

This module contains configuration constants used by the Project Header Update Script,
which processes project headers with environment details like Git info. It defines flags,
file types, directories, and other constants controlling the script's behavior.

Configuration Settings:
- DEBUG: Enable/disable detailed logs (True for verbose, False for standard)
- ENABLE_LOGGING: Enable/disable logging to a file
- SUPPORTED_EXTENSIONS: File types to process (e.g., .sh, .py)
- DIRECTORIES_TO_PROCESS: Directories to scan for file updates
- PROJECT_EXES: Executable files to copy after compilation
- EXCLUDED_SECTIONS: Sections to exclude from updates (e.g., licenses)
- NAME_TAG: Tag used for copyright headers in files
- DRY_RUN: Flag to simulate changes (no actual file modifications)
- ENABLE_COMPILATION: Flag to enable/disable project compilation
- ENABLE_COPY: Enable/disable copying of executables after compilation
- ENABLE_BACKUP: Enable/disable file backup before modifications
- MAX_PROCESS_LINES: Max lines to process in each file
- LOG_DIR: Directory for log files (e.g., ./logs)
- LOG_FILE_PREFIX: Prefix for log file names

Usage:
Import this module into `release.py` to use the configuration constants.
"""

# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.1-4eb6dd1 [new_release_proc].

from pathlib import Path

class Config:
    """
    Configuration class for the Project Header Update Script.

    This class holds constants and flags that control script behavior, such as logging, 
    file processing, compilation options, and more. Customize these settings to adjust 
    the script's operation.
    """

    # GENERAL SETTINGS
    DRY_RUN = False  # If True, no changes are made to files
    ENABLE_COMPILATION = True  # If True, the project will be compiled
    ENABLE_COPY = True  # If True, project executables will be copied after compilation
    ENABLE_BACKUP = False  # If True, backups will be created before file changes

    # LOGGING SETTINGS
    DEBUG = False  # If True, enables detailed debug logs
    ENABLE_LOGGING = False  # If True, logs will be written to a file
    LOG_DIR = Path(__file__).parent / "logs"  # Directory for storing logs
    LOG_FILE_PREFIX = "release_log_"  # Prefix for log file names

    # FILE PROCESSING SETTINGS
    SUPPORTED_EXTENSIONS = [".sh", ".py", ".d", ".h", ".c", ".hpp", ".cpp"]  # File types to process
    DIRECTORIES_TO_PROCESS = ["scripts", "src"]  # Directories to scan for file updates
    PROJECT_EXES = ["wspr", "wspr.ini"]  # Executable files to copy
    EXCLUDED_SECTIONS = ["GNU General Public License", "GPL"]  # Sections to exclude from header updates
    NAME_TAG = "@LBussy"  # Tag for copyright headers

    # FILE HANDLING SETTINGS
    MAX_PROCESS_LINES = 50  # Maximum lines to process in a file
