"""
Configuration File for Project Header Update Script

This module contains the configuration constants used by the Project Header Update Script.
It defines several behavior flags, directory paths, file types, and other constants that control
the script's functionality. The settings in this module can be adjusted to customize the script's
operation (e.g., enabling/disabling features like backup, compilation, or dry-run mode).

Configuration Settings:
- DEBUG: Enable/disable debugging logs (True for detailed logs, False for standard logs)
- ENABLE_LOGGING: Enable/disable logging to a file
- SUPPORTED_EXTENSIONS: List of supported file extensions to process
- DIRECTORIES_TO_PROCESS: Directories to scan for file updates
- PROJECT_EXES: List of project executables to copy after compilation
- EXCLUDED_SECTIONS: List of sections to exclude from updates (e.g., licenses)
- NAME_TAG: Name tag used in the files (e.g., in copyright headers)
- DRY_RUN: Flag to enable/disable dry-run mode (if True, no changes are made to files)
- ENABLE_COMPILATION: Enable/disable project compilation
- ENABLE_COPY: Enable/disable copying of executable files after compilation
- ENABLE_BACKUP: Enable/disable file backup (if True, backup files are created before changes)
- MAX_PROCESS_LINES: Maximum number of lines to process in a file
- LOG_DIR: Directory for log files
- LOG_FILE_PREFIX: Prefix for the log file names

Usage:
Import this module into the release.py script to use the configuration constants.
"""

from pathlib import Path

class Config:
    """
    Configuration class for the Project Header Update Script.

    This class holds all configuration constants and flags for controlling
    the behavior of the script, such as logging, dry-run mode, file extensions,
    directories to process, etc.
    """

    # Flag to enable/disable dry-run mode (if True, no changes are made to files)
    DRY_RUN = False
    
    # Enable or disable debugging (True = detailed logs, False = standard logs)
    DEBUG = True

    # Enable or disable logging (True = logging enabled, False = logging disabled)
    ENABLE_LOGGING = False

    # Flag to enable/disable project compilation
    ENABLE_COMPILATION = False

    # Flag to enable/disable copying of executable files after compilation
    ENABLE_COPY = False

    # Flag to enable/disable file backup (if True, backup files are created before changes)
    ENABLE_BACKUP = False
    
    # Supported file extensions to process
    SUPPORTED_EXTENSIONS = [".sh", ".py", ".d", ".h", ".c", ".hpp", ".cpp"]

    # Directories to process for file updates
    DIRECTORIES_TO_PROCESS = ["scripts", "src"]

    # List of project executables to copy
    PROJECT_EXES = ["wspr", "wspr.ini"]

    # List of sections to exclude from updates (e.g., licenses)
    EXCLUDED_SECTIONS = ["GNU General Public License", "GPL"]

    # Name tag used in the files (e.g., in copyright headers)
    NAME_TAG = "@LBussy"

    # Maximum number of lines to process in a file
    MAX_PROCESS_LINES = 50

    # Log directory (updated to ./logs)
    LOG_DIR = Path(__file__).parent / "logs"  # This points to ./logs instead of ./scripts/logs

    # Prefix for the log file name
    LOG_FILE_PREFIX = "release_log_"
