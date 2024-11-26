from pathlib import Path
"""
Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)

Created for WsprryPi project, version 1.2.1-2bf574e [refactoring].
"""

class Config:
    """
    Configuration class for the Project Header Update Script.

    This class holds all configuration constants and flags for controlling
    the behavior of the script, such as logging, dry-run mode, file extensions,
    directories to process, and exclusion patterns for license blocks.

    Attributes:
    -----------
    DRY_RUN : bool
        If True, no changes are made to the files. Used for testing purposes.
    ENABLE_COMPILATION : bool
        If True, the project will be compiled after updates are applied.
    ENABLE_COPY : bool
        If True, executable files will be copied after compilation.
    ENABLE_BACKUP : bool
        If True, backups of modified files will be created before updates.
    ENABLE_LOGGING : bool
        If True, logging to a file will be enabled.
    DEBUG : bool
        If True, detailed debug logs will be generated.
    SUPPORTED_EXTENSIONS : list
        List of file extensions to process.
    DIRECTORIES_TO_PROCESS : list
        List of directories to search for files to update.
    PROJECT_EXES : list
        List of project executables to copy after compilation.
    NAME_TAG : str
        The name tag used in the copyright headers of the files.
    LOG_DIR : Path
        The directory where log files will be stored.
    LOG_FILE_PREFIX : str
        Prefix for the log file names.
    LICENSE_PATTERNS : list
        Regular expressions for open-source licenses that should be excluded from updates.
"""

    # ============================
    # General Configuration
    # ============================
    DRY_RUN = False
    """
    Flag to enable or disable dry-run mode.

    If True, the script will simulate file updates without making any changes to the actual files. This is useful for testing purposes.
    """

    ENABLE_COMPILATION = True
    """
    Flag to enable or disable project compilation.

    If True, the project will be compiled after all updates are applied. This can be used to automate compilation post-header updates.
    """

    ENABLE_COPY = True
    """
    Flag to enable or disable copying of executable files after compilation.

    If True, executable files specified in `PROJECT_EXES` will be copied to the designated script directory after compilation.
    """

    ENABLE_BACKUP = False
    """
    Flag to enable or disable the creation of backups before updating files.

    If True, the script will create a backup file (with a `.bak` suffix) for each file that is updated.
    """

    ENABLE_LOGGING = False
    """
    Flag to enable or disable logging to a file.

    If True, the script will log its actions to a file. The log file will be stored in the directory specified by `LOG_DIR`.
    """

    DEBUG = False
    """
    Flag to enable or disable detailed debug logs.

    If True, the script will generate detailed debug logs for troubleshooting and verbose information about the execution.
    """

    # ============================
    # File Processing
    # ============================
    SUPPORTED_EXTENSIONS = [".sh", ".py", ".d", ".h", ".c", ".hpp", ".cpp"]
    """
    List of file extensions to process.

    The script will only process files with extensions specified in this list. These files will have their headers updated
    based on the configuration settings.
    """

    DIRECTORIES_TO_PROCESS = ["scripts", "src"]
    """
    List of directories to scan for files to update.

    The script will search for files in these directories and update their headers based on the configuration settings.
    """

    PROJECT_EXES = ["wspr", "wspr.ini"]
    """
    List of executables to copy after compilation.

    The script will copy these executable files to the designated script directory once the project has been compiled.
    """

    NAME_TAG = "@LBussy"
    """
    Name tag used in the copyright headers.

    This tag is used to identify the author of the project in the header comments. It will be inserted or updated in the copyright lines.
    """

    # ============================
    # Logging Configuration
    # ============================
    LOG_DIR = Path(__file__).parent / "logs"
    """
    Directory where log files will be stored.

    The script will store its log files in this directory. By default, it points to a `logs` directory in the current working directory.
    """

    LOG_FILE_PREFIX = "release_log_"
    """
    Prefix for the log file names.

    This prefix will be added to the log file names created during execution. The script will append a timestamp to this prefix to create unique log files.
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
        r"Creative Commons Attribution", # Creative Commons
        r"Public Domain",                 # Public Domain
    ]
    """
    List of regular expressions for open-source licenses to exclude from file processing.

    These patterns are used to identify sections of a file containing open-source licenses. The script will exclude these sections from header updates.
    """

    # ============================
    # Helper Method for License Exclusions
    # ============================
    @staticmethod
    def get_license_exclusion_patterns():
        """
        Get the compiled regex patterns for excluding license sections.

        This method compiles the LICENSE_PATTERNS regular expressions for efficient
        matching when checking file contents. The patterns are used to detect
        well-known open-source licenses and exclude them from processing.

        Returns:
        --------
        list
            A list of compiled regular expressions for matching open-source license blocks.
        """
        import re
        return [re.compile(pattern) for pattern in Config.LICENSE_PATTERNS]
