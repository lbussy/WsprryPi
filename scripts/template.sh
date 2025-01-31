#!/usr/bin/env bash
set -uo pipefail
IFS=$'\n\t'
set +o noclobber

# -----------------------------------------------------------------------------
# @file
# @brief Comprehensive Bash script template with advanced functionality.
#
# @details
# This script provides a robust framework for managing installation processes
# with extensive logging, error handling, and system validation. It includes:
# - Detailed stack traces for debugging.
# - Dynamic logging configuration with support for various levels (DEBUG, INFO,
#   etc.).
# - System checks for compatibility with OS versions, architectures,
#   dependencies, and environment variables.
# - Internet connectivity validation with proxy support.
# - Git repository context retrieval and semantic versioning utilities.
#
# @author Lee Bussy
# @date January 03, 2025
# @version 1.2.0
#
# @copyright
# This script is open-source and can be modified or distributed under the terms
# of the MIT license.
#
# @par Usage:
# ```bash
# ./template.sh [OPTIONS]
# ```
# Run `./template.sh --help` for detailed options.
#
# @par Requirements:
# - Bash version 4.0 or higher.
# - Dependencies as specified in the `DEPENDENCIES` array.
#
# @par Features:
# - Comprehensive environment validation (Bash, OS, dependencies, etc.).
# - Automatic Git context resolution for local and remote repositories.
# - Semantic version generation based on Git tags and commit history.
# - Flexible logging with customizable verbosity and output locations.
#
# @see
# Refer to the repository README for detailed function-level explanations.
#
# @warning
# Ensure this script is executed with appropriate permissions (e.g., sudo for
# installation tasks).
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# @brief Trap unexpected errors during script execution.
# @details Captures any errors (via the ERR signal) that occur during script
#          execution.
# Logs the function name and line number where the error occurred and exits
# the script. The trap calls an error-handling function for better flexibility.
#
# @global FUNCNAME Array containing function names in the call stack.
# @global LINENO Line number where the error occurred.
# @global THIS_SCRIPT Name of the script.
#
# @return None (exits the script with an error code).
# -----------------------------------------------------------------------------
# shellcheck disable=2317
trap_error() {
    # Capture function name, line number, and script name
    local func="${FUNCNAME[1]:-main}" # Get the call function (default: "main")
    local line="${1:-}"                # Line number where the error occurred
    local script="${this_script:-$(basename "${BASH_SOURCE[0]:-$0}")}"

    # Use global THIS_SCRIPT if it exists and is not empty;
    # otherwise, determine the script name
    if [[ -n "${THIS_SCRIPT:-}" ]]; then
        script="$THIS_SCRIPT"
    else
        script="${this_script:-$(basename "${BASH_SOURCE[0]:-$0}")}"

        # Check if the script name is valid; fallback to "temp_script.sh" if
        # it's bash or invalid
        if [[ "$script" == "bash" || -z "$script" ]]; then
            script="temp_script.sh"
        fi
    fi

    # Validate that the line number is set
    if [[ -z "$line" ]]; then
        line="unknown"
    fi

    # Log the error message to stderr
    printf "\n" >&2
    printf "[ERROR] Error in '%s()' at line %s of '%s'. Exiting.\n" \
        "$func" "$line" "$script" >&2

    # Exit with a non-zero status code
    exit 1
}

# Set the trap to call trap_error on any error by uncommenting the next
# line to help with script debugging.
# trap 'trap_error "$LINENO"' ERR

############
### Global Script Declarations
############

# -----------------------------------------------------------------------------
# @var REQUIRE_SUDO
# @brief Indicates whether root privileges are required to run the script.
# @details This variable determines if the script requires execution with root
#          privileges. It defaults to `true`, meaning the script will enforce
#          that it is run with `sudo` or as a root user. This behavior can be
#          overridden by setting the `REQUIRE_SUDO` environment variable to
#          `false`.
#
# @default true
#
# @example
# REQUIRE_SUDO=false ./template.sh  # Run the script without enforcing root
#                                     privileges.
# -----------------------------------------------------------------------------
readonly REQUIRE_SUDO="${REQUIRE_SUDO:-true}"

# -----------------------------------------------------------------------------
# @brief Determines the script name to use.
# @details This block of code determines the value of `THIS_SCRIPT` based on
#          the following logic:
#          1. If `THIS_SCRIPT` is already set in the environment, it is used.
#          2. If `THIS_SCRIPT` is not set, the script checks if
#             `${BASH_SOURCE[0]}` is available:
#             - If `${BASH_SOURCE[0]}` is set and not equal to `"bash"`, the
#               script extracts the filename (without the path) using
#               `basename` and assigns it to `THIS_SCRIPT`.
#             - If `${BASH_SOURCE[0]}` is unbound or equals `"bash"`, it falls
#               back to using the value of `FALLBACK_SCRIPT_NAME`, which
#               defaults to `debug_print.sh`.
#
# @var FALLBACK_SCRIPT_NAME
# @brief Default name for the script in case `BASH_SOURCE[0]` is unavailable.
# @details This variable is used as a fallback value if `BASH_SOURCE[0]` is
#          not set or equals `"bash"`. The default value is `"debug_print.sh"`.
#
# @var THIS_SCRIPT
# @brief Holds the name of the script to use.
# @details The script attempts to determine the name of the script to use. If
#          `THIS_SCRIPT` is already set in the environment, it is used
#          directly. Otherwise, the script tries to extract the filename from
#          `${BASH_SOURCE[0]}` (using `basename`). If that fails, it defaults
#          to `FALLBACK_SCRIPT_NAME`.
# -----------------------------------------------------------------------------
declare FALLBACK_SCRIPT_NAME="${FALLBACK_SCRIPT_NAME:-template.sh}"
if [[ -z "${THIS_SCRIPT:-}" ]]; then
    if [[ -n "${BASH_SOURCE[0]:-}" && "${BASH_SOURCE[0]:-}" != "bash" ]]; then
        # Use BASH_SOURCE[0] if it is available and not "bash"
        THIS_SCRIPT=$(basename "${BASH_SOURCE[0]}")
    else
        # If BASH_SOURCE[0] is unbound or equals "bash", use
        # FALLBACK_SCRIPT_NAME
        THIS_SCRIPT="${FALLBACK_SCRIPT_NAME}"
    fi
fi

# -----------------------------------------------------------------------------
# @var DRY_RUN
# @brief Enables simulated execution of certain commands.
# @details When set to `true`, commands are not actually executed but are
#          simulated to allow testing or validation without side effects.
#          If set to `false`, commands execute normally.
#
# @example
# DRY_RUN=true ./template.sh  # Run the script in dry-run mode.
# -----------------------------------------------------------------------------
declare DRY_RUN="${DRY_RUN:-false}"

# -----------------------------------------------------------------------------
# @var IS_PATH
# @brief Indicates whether the script was executed from a `PATH` location.
# @details This variable is initialized to `false` by default. During
#          execution, it is dynamically set to `true` if the script is
#          determined to have been executed from a directory listed in the
#          `PATH` environment variable.
#
# @example
# if [[ "$IS_PATH" == "true" ]]; then
#     echo "Script was executed from a PATH directory."
# else
#     echo "Script was executed from a non-PATH directory."
# fi
# -----------------------------------------------------------------------------
declare IS_PATH="${IS_PATH:-false}"

# -----------------------------------------------------------------------------
# @var IS_REPO
# @brief Indicates whether the script resides in a GitHub repository or
#        subdirectory.
# @details This variable is initialized to `false` by default. During
#          execution, it
#          is dynamically set to `true` if the script is detected to be within
#          a GitHub repository (i.e., if a `.git` directory exists in the
#          directory hierarchy of the script's location).
#
# @example
# if [[ "$IS_REPO" == "true" ]]; then
#     echo "This script resides within a GitHub repository."
# else
#     echo "This script is not located within a GitHub repository."
# fi
# -----------------------------------------------------------------------------
declare IS_REPO="${IS_REPO:-false}"

# -----------------------------------------------------------------------------
# @brief Project metadata constants used throughout the script.
# @details These variables provide metadata about the script, including
#          ownership, versioning, project details, and GitHub URLs. They are
#          initialized with default values here and may be dynamically set
#          during execution to reflect the project's context.
#
# @vars
# - @var REPO_ORG The organization or owner of the repository (default:
#                 "lbussy").
# - @var REPO_NAME The name of the repository (default: "bash-template").
# - @var REPO_BRANCH The current Git branch name (default: "main").
# - @var GIT_TAG The current Git tag (default: "0.0.1").
# - @var SEM_VER The semantic version of the project (default: "0.0.1").
# - @var LOCAL_REPO_DIR The local source directory path (default: unset).
# - @var LOCAL_WWW_DIR The local web directory path (default: unset).
# - @var LOCAL_SCRIPTS_DIR The local scripts directory path (default: unset).
# - @var GIT_RAW The base URL for accessing raw GitHub content (default:
#                "https://raw.githubusercontent.com/$REPO_ORG/$REPO_NAME").
# - @var GIT_API The base URL for the GitHub API for this repository (default:
#                "https://api.github.com/repos/$REPO_ORG/$REPO_NAME").
# - @var GIT_CLONE The clone URL for the GitHub repository (default:
#                "https://api.github.com/repos/$REPO_ORG/$REPO_NAME").
#
# @example
# echo "Repository: $REPO_ORG/$REPO_NAME"
# echo "Branch: $REPO_BRANCH, Tag: $GIT_TAG, Version: $SEM_VER"
# echo "Source Directory: ${LOCAL_REPO_DIR:-Not Set}"
# echo "WWW Directory: ${LOCAL_WWW_DIR:-Not Set}"
# echo "Scripts Directory: ${LOCAL_SCRIPTS_DIR:-Not Set}"
# echo "Raw URL: $GIT_RAW"
# echo "API URL: $GIT_API"
# -----------------------------------------------------------------------------
declare REPO_ORG="${REPO_ORG:-lbussy}"
declare REPO_NAME="${REPO_NAME:-bash-template}"
declare REPO_BRANCH="${REPO_BRANCH:-main}"
declare GIT_TAG="${GIT_TAG:-1.2.1}"
declare SEM_VER="${GIT_TAG:-1.2.1-main.31256b0}"
declare LOCAL_REPO_DIR="${LOCAL_REPO_DIR:-}"
declare LOCAL_WWW_DIR="${LOCAL_WWW_DIR:-}"
declare LOCAL_SCRIPTS_DIR="${LOCAL_SCRIPTS_DIR:-}"
declare GIT_RAW="${GIT_RAW:-"https://raw.githubusercontent.com/$REPO_ORG/\
$REPO_NAME"}"
declare GIT_API="${GIT_API:-"https://api.github.com/repos/$REPO_ORG/\
$REPO_NAME"}"

# -----------------------------------------------------------------------------
# Declare Menu Variables
# -----------------------------------------------------------------------------
declare -A MENU_ITEMS       # Associative array of menu items
declare -a MAIN_MENU        # Array defining the main menu screen
declare -a SUB_MENU         # Array defining the sub-menu screen
declare MENU_HEADER="${MENU_HEADER:-Menu}"  # Global menu header

# -----------------------------------------------------------------------------
# Declare Arguments Variables
# -----------------------------------------------------------------------------
declare ARGUMENTS_LIST=()   # List of word arguments for command line parsing
declare OPTIONS_LIST=()     # List of -f--fl arguemtns for command line parsing

# -----------------------------------------------------------------------------
# @var GIT_DIRS
# @brief List of relevant directories for download.
# @details This array contains the names of the directories within the GitHub
#          repository that will be processed and downloaded. The directories
#          include 'man', 'scripts', and 'conf'. These directories are used
#          in the script to determine which content to fetch from the
#          repository.
# -----------------------------------------------------------------------------
readonly GIT_DIRS="${GIT_DIRS:-("man" "scripts" "conf")}"

# -----------------------------------------------------------------------------
# @var USER_HOME
# @brief Home directory of the current user.
# @details This variable stores the home directory of the user executing the script.
#          If the script is run with `sudo`, it uses the home directory of the
#          `SUDO_USER`, otherwise, it defaults to the current user's home directory.
# -----------------------------------------------------------------------------
declare USER_HOME
if [[ "$REQUIRE_SUDO" == true && -z "${SUDO_USER-}" ]]; then
    # Fail gracefully if REQUIRE_SUDO is true and SUDO_USER is not set
    USER_HOME=""
elif [[ -n "${SUDO_USER-}" ]]; then
    # Use SUDO_USER's home directory if it's set
    USER_HOME=$(eval echo "~$SUDO_USER")
else
    # Fallback to HOME if SUDO_USER is not set
    USER_HOME="$HOME"
fi

# -----------------------------------------------------------------------------
# @var USE_CONSOLE
# @brief Controls whether logging output is directed to the console.
# @details When set to `true`, log messages are displayed on the console in
#          addition to being written to the log file (if enabled). When set
#          to `false`, log messages are written only to the log file, making
#          it suitable for non-interactive or automated environments.
#
# @example
# - USE_CONSOLE=true: Logs to both console and file.
# - USE_CONSOLE=false: Logs only to file.
# -----------------------------------------------------------------------------
declare USE_CONSOLE="${USE_CONSOLE:-true}"

# -----------------------------------------------------------------------------
# @var CONSOLE_STATE
# @brief Tracks the original console logging state.
# @details This variable mirrors the value of USE_CONSOLE and provides a
#          consistent reference for toggling or querying the state of console
#          logging.
#
# @example
# - CONSOLE_STATE="USE_CONSOLE": Console logging matches USE_CONSOLE.
# -----------------------------------------------------------------------------
declare CONSOLE_STATE="${CONSOLE_STATE:-$USE_CONSOLE}"

# -----------------------------------------------------------------------------
# @var TERSE
# @brief Enables or disables terse logging mode.
# @details When `TERSE` is set to `true`, log messages are minimal and
#          optimized for automated environments where concise output is
#          preferred. When set to `false`, log messages are verbose, providing
#          detailed information suitable for debugging or manual intervention.
#
# @example
# TERSE=true  # Enables terse logging mode.
# ./template.sh
#
# TERSE=false # Enables verbose logging mode.
# ./template.sh
# -----------------------------------------------------------------------------
declare TERSE="${TERSE:-false}"

# -----------------------------------------------------------------------------
# @var REQUIRE_INTERNET
# @type string
# @brief Flag indicating if internet connectivity is required.
# @details Controls whether the script should verify internet connectivity
#          during initialization. This variable can be overridden by setting
#          the `REQUIRE_INTERNET` environment variable before running the
#          script.
#
# @values
# - `"true"`: Internet connectivity is required.
# - `"false"`: Internet connectivity is not required.
#
# @default "true"
#
# @example
# REQUIRE_INTERNET=false ./template.sh  # Run the script without verifying
#                                       # Internet connectivity.
# -----------------------------------------------------------------------------
readonly REQUIRE_INTERNET="${REQUIRE_INTERNET:-true}"

# -----------------------------------------------------------------------------
# @var MIN_BASH_VERSION
# @brief Specifies the minimum supported Bash version.
# @details Defines the minimum Bash version required to execute the script. By
#          default, it is set to `4.0`. This value can be overridden by setting
#          the `MIN_BASH_VERSION` environment variable before running the
#          script.
#          To disable version checks entirely, set this variable to `"none"`.
#
# @default "4.0"
#
# @example
# MIN_BASH_VERSION="none" ./template.sh  # Disable Bash version checks.
# MIN_BASH_VERSION="5.0" ./template.sh   # Require at least Bash 5.0.
# -----------------------------------------------------------------------------
readonly MIN_BASH_VERSION="${MIN_BASH_VERSION:-4.0}"

# -----------------------------------------------------------------------------
# @var MIN_OS
# @brief Specifies the minimum supported OS version.
# @details Defines the lowest OS version that the script supports. This value
#          should be updated as compatibility requirements evolve. It is used
#          to ensure the script is executed only on compatible systems.
#
# @default 11
#
# @example
# if [[ "$CURRENT_OS_VERSION" -lt "$MIN_OS" ]]; then
#     echo "This script requires OS version $MIN_OS or higher."
#     exit 1
# fi
# -----------------------------------------------------------------------------
readonly MIN_OS="${MIN_OS:-11}"

# -----------------------------------------------------------------------------
# @var MAX_OS
# @brief Specifies the maximum supported OS version.
# @details Defines the highest OS version that the script supports. If the
#          script is executed on a system with an OS version higher than this
#          value, it may not function as intended. Set this to `-1` to indicate
#          no upper limit on supported OS versions.
#
# @default 15
#
# @example
# if [[ "$CURRENT_OS_VERSION" -gt "$MAX_OS" && "$MAX_OS" -ne -1 ]]; then
#     echo "This script supports OS versions up to $MAX_OS."
#     exit 1
# fi
# -----------------------------------------------------------------------------
readonly MAX_OS="${MAX_OS:-15}"  # (use -1 for no upper limit)

# -----------------------------------------------------------------------------
# @var SUPPORTED_BITNESS
# @brief Specifies the supported system bitness.
# @details Defines the system architectures that the script supports.
#          Acceptable values are:
#          - `"32"`: Only supports 32-bit systems.
#          - `"64"`: Only supports 64-bit systems.
#          - `"both"`: Supports both 32-bit and 64-bit systems.
#          This variable ensures compatibility with the intended system architecture.
#          It defaults to `"32"` if not explicitly set.
#
# @default "32"
#
# @example
# if [[ "$BITNESS" != "$SUPPORTED_BITNESS" && \
#     "$SUPPORTED_BITNESS" != "both" ]]; then
#     echo "This script supports $SUPPORTED_BITNESS-bit systems only."
#     exit 1
# fi
# -----------------------------------------------------------------------------
readonly SUPPORTED_BITNESS="${SUPPORTED_BITNESS:-both}" # ("32", "64", or "both")

# -----------------------------------------------------------------------------
# @var SUPPORTED_MODELS
# @brief Associative array of Raspberry Pi models and their support statuses.
# @details This associative array maps Raspberry Pi model identifiers to their
#          corresponding support statuses. Each key is a pipe-delimited string
#          containing:
#          - The model name (e.g., "Raspberry Pi 4 Model B").
#          - A simplified identifier (e.g., "4-model-b").
#          - The chipset identifier (e.g., "bcm2711").
#
#          The value is the support status, which can be:
#          - `"Supported"`: Indicates the model is supported by the script.
#          - `"Not Supported"`: Indicates the model is not supported.
#
#          This array is marked as `readonly` to ensure it remains immutable at
#          runtime.
#
#          declare SUPPORTED_MODELS="all" at invocateion or in the environment
#          to accept any models.
#
# @example
# for model in "${!SUPPORTED_MODELS[@]}"; do
#     IFS="|" read -r full_name short_name chipset <<< "$model"
#     echo "Model: $full_name ($short_name, $chipset) - Status: \
# ${SUPPORTED_MODELS[$model]}"
# done
# -----------------------------------------------------------------------------
# Ensure SUPPORTED_MODELS is initialized
declare -A SUPPORTED_MODELS

# Safely check if SUPPORTED_MODELS is empty
if [[ -z "${SUPPORTED_MODELS+x}" || ${#SUPPORTED_MODELS[@]} -eq 0 ]]; then
    # If SUPPORTED_MODELS is not set or empty, define the default supported models
    SUPPORTED_MODELS=(
        # Unsupported models
        ["Raspberry Pi 5|5-model-b|bcm2712"]="Supported"
        ["Raspberry Pi 400|400|bcm2711"]="Supported"
        ["Raspberry Pi Compute Module 4|4-compute-module|bcm2711"]="Supported"
        ["Raspberry Pi Compute Module 3|3-compute-module|bcm2837"]="Supported"
        ["Raspberry Pi Compute Module|compute-module|bcm2835"]="Supported"
        # Supported models
        ["Raspberry Pi 4 Model B|4-model-b|bcm2711"]="Supported"
        ["Raspberry Pi 3 Model A+|3-model-a-plus|bcm2837"]="Supported"
        ["Raspberry Pi 3 Model B+|3-model-b-plus|bcm2837"]="Supported"
        ["Raspberry Pi 3 Model B|3-model-b|bcm2837"]="Supported"
        ["Raspberry Pi 2 Model B|2-model-b|bcm2836"]="Supported"
        ["Raspberry Pi Model A+|model-a-plus|bcm2835"]="Supported"
        ["Raspberry Pi Model B+|model-b-plus|bcm2835"]="Supported"
        ["Raspberry Pi Model B Rev 2|model-b-rev2|bcm2835"]="Supported"
        ["Raspberry Pi Model A|model-a|bcm2835"]="Supported"
        ["Raspberry Pi Model B|model-b|bcm2835"]="Supported"
        ["Raspberry Pi Zero 2 W|model-zero-2-w|bcm2837"]="Supported"
        ["Raspberry Pi Zero|model-zero|bcm2835"]="Supported"
        ["Raspberry Pi Zero W|model-zero-w|bcm2835"]="Supported"
    )
elif [[ ${SUPPORTED_MODELS[all]+_} ]]; then
    # If SUPPORTED_MODELS contains "all", set it as a special value
    SUPPORTED_MODELS=( ["all"]="Supported" )
fi

# -----------------------------------------------------------------------------
# @var LOG_OUTPUT
# @brief Controls where log messages are directed.
# @details Specifies the logging destination(s) for the script's output. This
#          variable can be set to one of the following values:
#          - `"file"`: Log messages are written only to a file.
#          - `"console"`: Log messages are displayed only on the console.
#          - `"both"`: Log messages are written to both the console and a file.
#          - `unset`: Defaults to `"both"`.
#
#          This variable allows flexible logging behavior depending on the
#          environment or use case.
#
# @default "both"
#
# @example
# LOG_OUTPUT="file" ./template.sh      # Logs to a file only.
# LOG_OUTPUT="console" ./template.sh   # Logs to the console only.
# LOG_OUTPUT="both" ./template.sh      # Logs to both destinations.
# -----------------------------------------------------------------------------
declare LOG_OUTPUT="${LOG_OUTPUT:-both}"

# -----------------------------------------------------------------------------
# @var LOG_FILE
# @brief Specifies the path to the log file.
# @details Defines the file path where log messages are written when logging
#          to a file is enabled. If not explicitly set, this variable defaults
#          to blank, meaning no log file will be used unless a specific path
#          is assigned at runtime or through an external environment variable.
#
# @default ""
#
# @example
# LOG_FILE="/var/log/my_script.log" ./template.sh  # Use a custom log file.
# -----------------------------------------------------------------------------
declare LOG_FILE="${LOG_FILE:-}"  # Use the provided LOG_FILE or default to blank.

# -----------------------------------------------------------------------------
# @var LOG_LEVEL
# @brief Specifies the logging verbosity level.
# @details Defines the verbosity level for logging messages. This variable
#          controls which messages are logged based on their severity. It
#          defaults to `"DEBUG"` if not set. Common log levels include:
#          - `"DEBUG"`: Detailed messages for troubleshooting and development.
#          - `"INFO"`: Informational messages about normal operations.
#          - `"WARN"`: Warning messages indicating potential issues.
#          - `"ERROR"`: Errors that require immediate attention.
#          - `"CRITICAL"`: Critical issues that may cause the script to fail.
#
# @default "DEBUG"
#
# @example
# LOG_LEVEL="INFO" ./template.sh  # Set the log level to INFO.
# -----------------------------------------------------------------------------
declare LOG_LEVEL="${LOG_LEVEL:-DEBUG}"

# -----------------------------------------------------------------------------
# @var DEPENDENCIES
# @type array
# @brief List of required external commands for the script.
# @details This array defines the external commands that the script depends on
#          to function correctly. Each command in this list is checked for
#          availability at runtime. If a required command is missing, the script
#          may fail or display an error message.
#
#          Best practices:
#          - Ensure all required commands are included.
#          - Use a dependency-checking function to verify their presence early
#            in the script.
#
# @default
# A predefined set of common system utilities
#
# @note Update this list as needed to reflect the actual commands used in the script.
#
# @example
# for cmd in "${DEPENDENCIES[@]}"; do
#     if ! command -v "$cmd" &>/dev/null; then
#         echo "Error: Missing required command: $cmd"
#         exit 1
#     fi
# done
# -----------------------------------------------------------------------------
declare -ar DEPENDENCIES=(
    "awk"
    "grep"
    "tput"
    "cut"
    "tr"
    "getconf"
    "cat"
    "sed"
    "basename"
    "getent"
    "date"
    "printf"
    "whoami"
    "touch"
    "dpkg"
    "git"
    "dpkg-reconfigure"
    "curl"
    "wget"
    "realpath"
)
readonly DEPENDENCIES

# -----------------------------------------------------------------------------
# @var ENV_VARS_BASE
# @type array
# @brief Base list of required environment variables.
# @details Defines the core environment variables that the script relies on,
#          regardless of the runtime context. These variables must be set to
#          ensure the script functions correctly.
#
#          - `HOME`: Specifies the home directory of the current user.
#          - `COLUMNS`: Defines the width of the terminal, used for formatting.
#
# @example
# for var in "${ENV_VARS_BASE[@]}"; do
#     if [[ -z "${!var}" ]]; then
#         echo "Error: Required environment variable '$var' is not set."
#         exit 1
#     fi
# done
# -----------------------------------------------------------------------------
declare -ar ENV_VARS_BASE=(
    "HOME"       # Home directory of the current user
    "COLUMNS"    # Terminal width for formatting, often dynamic in the OS
)

# -----------------------------------------------------------------------------
# @var ENV_VARS
# @type array
# @brief Final list of required environment variables.
# @details This array extends `ENV_VARS_BASE` to include additional variables
#          required under specific conditions. If the script requires root
#          privileges (`REQUIRE_SUDO=true`), the `SUDO_USER` variable is added
#          dynamically during runtime. Otherwise, it inherits only the base
#          environment variables.
#
#          - `SUDO_USER`: Identifies the user who invoked the script using `sudo`.
#
# @note Ensure `ENV_VARS_BASE` is properly defined before constructing `ENV_VARS`.
#
# @example
# for var in "${ENV_VARS[@]}"; do
#     if [[ -z "${!var}" ]]; then
#         echo "Error: Required environment variable '$var' is not set."
#         exit 1
#     fi
# done
# -----------------------------------------------------------------------------
if [[ "$REQUIRE_SUDO" == true ]]; then
    readonly -a ENV_VARS=("${ENV_VARS_BASE[@]}" "SUDO_USER")
else
    readonly -a ENV_VARS=("${ENV_VARS_BASE[@]}")
fi

# -----------------------------------------------------------------------------
# @var COLUMNS
# @brief Terminal width in columns.
# @details The `COLUMNS` variable represents the width of the terminal in
#          characters. It is used for formatting output to fit within the
#          terminal's width. If not already set by the environment, it defaults
#          to `80` columns. This value can be overridden externally by setting
#          the `COLUMNS` environment variable before running the script.
#
# @default 80
#
# @example
# echo "The terminal width is set to $COLUMNS columns."
# -----------------------------------------------------------------------------
COLUMNS="${COLUMNS:-80}"

# -----------------------------------------------------------------------------
# @var SYSTEM_READS
# @type array
# @brief List of critical system files to check.
# @details Defines the absolute paths to system files that the script depends on
#          for its execution. These files must be present and readable to ensure
#          the script operates correctly. The following files are included:
#          - `/etc/os-release`: Contains operating system identification data.
#          - `/proc/device-tree/compatible`: Identifies hardware compatibility,
#            commonly used in embedded systems like Raspberry Pi.
#
# @example
# for file in "${SYSTEM_READS[@]}"; do
#     if [[ ! -r "$file" ]]; then
#         echo "Error: Required system file '$file' is missing or not readable."
#         exit 1
#     fi
# done
# -----------------------------------------------------------------------------
declare -ar SYSTEM_READS=(
    "/etc/os-release"               # OS identification file
    "/proc/device-tree/compatible"  # Hardware compatibility file
)
readonly SYSTEM_READS

# -----------------------------------------------------------------------------
# @var APT_PACKAGES
# @type array
# @brief List of required APT packages.
# @details Defines the APT packages that the script depends on for its
#          execution. These packages should be available in the system's
#          default package repository. The script will check for their presence
#          and attempt to install any missing packages as needed.
#
#          Packages included:
#          - `jq`: JSON parsing utility.
#          - `git`: Version control system.
#
# @example
# for pkg in "${APT_PACKAGES[@]}"; do
#     if ! dpkg -l "$pkg" &>/dev/null; then
#         echo "Error: Required package '$pkg' is not installed."
#         exit 1
#     fi
# done
# -----------------------------------------------------------------------------
readonly APT_PACKAGES=(
    "jq"   # JSON parsing utility
    "git"  # Version control system
)

# -----------------------------------------------------------------------------
# @var WARN_STACK_TRACE
# @type string
# @brief Flag to enable stack trace logging for warnings.
# @details Controls whether stack traces are printed alongside warning
#          messages. This feature is particularly useful for debugging and
#          tracking the script's execution path in complex workflows.
#
#          Possible values:
#          - `"true"`: Enables stack trace logging for warnings.
#          - `"false"`: Disables stack trace logging for warnings (default).
#
# @default "false"
#
# @example
# WARN_STACK_TRACE=true ./template.sh  # Enable stack traces for warnings.
# WARN_STACK_TRACE=false ./template.sh # Disable stack traces for warnings.
# -----------------------------------------------------------------------------
readonly WARN_STACK_TRACE="${WARN_STACK_TRACE:-false}"

############
### Template Functions
############

# -----------------------------------------------------------------------------
# @brief Handles shell exit operations, displaying session statistics.
# @details This function is called automatically when the shell exits. It
#          calculates and displays the number of commands executed during
#          the session and the session's end timestamp. It is intended to
#          provide users with session statistics before the shell terminates.
#
# @global EXIT This signal is trapped to call the `egress` function upon shell
#              termination.
#
# @note The function uses `history | wc -l` to count the commands executed in
#       the current session and `date` to capture the session end time.
# -----------------------------------------------------------------------------
egress() {
    # TODO: Add any cleanup items here
    # shellcheck disable=SC2317
    true
}

# -----------------------------------------------------------------------------
# @brief Starts the debug process.
# @details This function checks if the "debug" flag is present in the
#          arguments, and if so, prints the debug information including the
#          function name, the caller function name, and the line number where
#          the function was called.
#
# @param "$@" Arguments to check for the "debug" flag.
#
# @return Returns the "debug" flag if present, or an empty string if not.
#
# @example
# debug_start "debug"  # Prints debug information
# debug_start          # Does not print anything, returns an empty string
# -----------------------------------------------------------------------------
debug_start() {
    local debug=""
    local args=()  # Array to hold non-debug arguments

    # Look for the "debug" flag in the provided arguments
    for arg in "$@"; do
        if [[ "$arg" == "debug" ]]; then
            debug="debug"
            break  # Exit the loop as soon as we find "debug"
        fi
    done

    # Handle empty or unset FUNCNAME and BASH_LINENO gracefully
    local this_script
    this_script=$(basename "${THIS_SCRIPT:-main}")
    this_script="${this_script%.*}"
    local func_name="${FUNCNAME[1]:-main}"
    local caller_name="${FUNCNAME[2]:-main}"
    local caller_line=${BASH_LINENO[1]:-0}
    local current_line=${BASH_LINENO[0]:-0}

    # Print debug information if the flag is set
    if [[ "$debug" == "debug" ]]; then
        printf "[DEBUG]\t[%s:%s():%d] Starting function called by %s():%d.\n" \
            "$this_script" "$func_name" "$current_line"  "$caller_name" "$caller_line" >&2
    fi

    # Return the debug flag if present, or an empty string if not
    printf "%s\n" "${debug:-}"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Filters out the "debug" flag from the arguments.
# @details This function removes the "debug" flag from the list of arguments
#          and returns the filtered arguments. The debug flag is not passed
#          to other functions to avoid unwanted debug outputs.
#
# @param "$@" Arguments to filter.
#
# @return Returns a string of filtered arguments, excluding "debug".
#
# @example
# debug_filter "arg1" "debug" "arg2"  # Returns "arg1 arg2"
# -----------------------------------------------------------------------------
debug_filter() {
    local args=()  # Array to hold non-debug arguments

    # Iterate over each argument and exclude "debug"
    for arg in "$@"; do
        if [[ "$arg" != "debug" ]]; then
            args+=("$arg")
        fi
    done

    # Print the filtered arguments, safely quoting them for use in a command
    printf "%q " "${args[@]}"
}

# -----------------------------------------------------------------------------
# @brief Prints a debug message if the debug flag is set.
# @details This function checks if the "debug" flag is present in the
#          arguments. If the flag is present, it prints the provided debug
#          message along with the function name and line number from which the
#          function was called.
#
# @param "$@" Arguments to check for the "debug" flag and the debug message.
# @global debug A flag to indicate whether debug messages should be printed.
#
# @return None.
#
# @example
# debug_print "debug" "This is a debug message"
# -----------------------------------------------------------------------------
debug_print() {
    local debug=""
    local args=()  # Array to hold non-debug arguments

    # Loop through all arguments to identify the "debug" flag
    for arg in "$@"; do
        if [[ "$arg" == "debug" ]]; then
            debug="debug"
        else
            args+=("$arg")  # Add non-debug arguments to the array
        fi
    done

    # Restore the positional parameters with the filtered arguments
    set -- "${args[@]}"

    # Handle empty or unset FUNCNAME and BASH_LINENO gracefully
    local this_script
    this_script=$(basename "${THIS_SCRIPT:-main}")
    this_script="${this_script%.*}"
    local caller_name="${FUNCNAME[1]:-main}"
    local caller_line="${BASH_LINENO[0]:-0}"

    # Assign the remaining argument to the message, defaulting to <unset>
    local message="${1:-<unset>}"

    # Print debug information if the debug flag is set
    if [[ "$debug" == "debug" ]]; then
        printf "[DEBUG]\t[%s:%s:%d] '%s'.\n" \
               "$this_script" "$caller_name" "$caller_line" "$message" >&2
    fi
}

# -----------------------------------------------------------------------------
# @brief Ends the debug process.
# @details This function checks if the "debug" flag is present in the
#          arguments. If the flag is present, it prints debug information
#          indicating the exit of the function, along with the function name
#          and line number from where the function was called.
#
# @param "$@" Arguments to check for the "debug" flag.
# @global debug Debug flag, passed from the calling function.
#
# @return None
#
# @example
# debug_end "debug"
# -----------------------------------------------------------------------------
debug_end() {
    local debug=""
    local args=()  # Array to hold non-debug arguments

    # Loop through all arguments and identify the "debug" flag
    for arg in "$@"; do
        if [[ "$arg" == "debug" ]]; then
            debug="debug"
            break  # Exit the loop as soon as we find "debug"
        fi
    done

    # Handle empty or unset FUNCNAME and BASH_LINENO gracefully
    local this_script
    this_script=$(basename "${THIS_SCRIPT:-main}")
    this_script="${this_script%.*}"
    local func_name="${FUNCNAME[1]:-main}"
    local caller_name="${FUNCNAME[2]:-main}"
    local caller_line=${BASH_LINENO[1]:-0}
    local current_line=${BASH_LINENO[0]:-0}

    # Print debug information if the flag is set
    if [[ "$debug" == "debug" ]]; then
        printf "[DEBUG]\t[%s:%s():%d] Exiting function returning to %s():%d.\n" \
            "$this_script" "$func_name" "$current_line"  "$caller_name" "$caller_line" >&2
    fi
}

# -----------------------------------------------------------------------------
# @brief Prints a stack trace with optional formatting and a message.
# @details This function generates and displays a formatted stack trace for
#          debugging purposes. It includes a log level and optional details,
#          with color-coded formatting and proper alignment.
#
# @param $1 [optional] Log level (DEBUG, INFO, WARN, ERROR, CRITICAL).
#           Defaults to INFO.
# @param $2 [optional] Primary message for the stack trace.
# @param $@ [optional] Additional context or details for the stack trace.
#
# @global FUNCNAME Array of function names in the call stack.
# @global BASH_LINENO Array of line numbers corresponding to the call stack.
# @global THIS_SCRIPT The name of the current script, used for logging.
# @global COLUMNS Console width, used for formatting output.
#
# @throws None.
#
# @return None. Outputs the stack trace and message to standard output.
#
# @example
# stack_trace WARN "Unexpected condition detected."
# -----------------------------------------------------------------------------
stack_trace() {
    # Determine log level and message
    local level="${1:-INFO}"
    local message=""
    # Block width and character for header/footer
    local char="-"

    # Recalculate terminal columns
    COLUMNS=$( (command -v tput >/dev/null && tput cols) || printf "80")
    COLUMNS=$((COLUMNS > 0 ? COLUMNS : 80)) # Ensure COLUMNS is a positive number
    local width
    width=${COLUMNS:-80}                    # Max console width

    # Check if $1 is a valid level, otherwise treat it as the message
    case "$level" in
        DEBUG|INFO|WARN|WARNING|ERROR|CRIT|CRITICAL)
            shift
            ;;
        *)
            message="$level"
            level="INFO"
            shift
            ;;
    esac

    # Concatenate all remaining arguments into $message
    for arg in "$@"; do
        message+="$arg "
    done
    # Trim leading/trailing whitespace
    message=$(printf "%s" "$message" | xargs)

    # Define functions to skip
    local skip_functions=("die" "warn" "stack_trace")
    local encountered_main=0 # Track the first occurrence of main()

    # Generate title case function name for the banner
    local raw_function_name="${FUNCNAME[0]}"
    local header_name header_level
    header_name=$(printf "%s" "$raw_function_name" | sed -E 's/_/ /g; s/\b(.)/\U\1/g; s/(\b[A-Za-z])([A-Za-z]*)/\1\L\2/g')
    header_level=$(printf "%s" "$level" | sed -E 's/\b(.)/\U\1/g; s/(\b[A-Za-z])([A-Za-z]*)/\1\L\2/g')
    header_name="$header_level $header_name"

    # Helper: Skip irrelevant functions
    should_skip() {
        local func="$1"
        for skip in "${skip_functions[@]}"; do
            if [[ "$func" == "$skip" ]]; then
                return 0
            fi
        done
        if [[ "$func" == "main" && $encountered_main -gt 0 ]]; then
            return 0
        fi
        [[ "$func" == "main" ]] && ((encountered_main++))
        return 1
    }

    # Build the stack trace
    local displayed_stack=()
    local longest_length=0
    for ((i = 1; i < ${#FUNCNAME[@]}; i++)); do
        local func="${FUNCNAME[i]}"
        local line="${BASH_LINENO[i - 1]}"
        local current_length=${#func}

        if should_skip "$func"; then
            continue
        elif (( current_length > longest_length )); then
            longest_length=$current_length
        fi

        displayed_stack=("$(printf "%s|%s" "$func()" "$line")" "${displayed_stack[@]}")
    done

    # General text attributes
    local reset="\033[0m"     # Reset text formatting
    local bold="\033[1m"      # Bold text

    # Foreground colors
    local fgred="\033[31m"    # Red text
    local fggrn="\033[32m"    # Green text
    local fgblu="\033[34m"    # Blue text
    local fgmag="\033[35m"    # Magenta text
    local fgcyn="\033[36m"    # Cyan text
    local fggld="\033[38;5;220m"  # Gold text (ANSI 256 color)

    # Determine color and label based on level
    local color label
    case "$level" in
        DEBUG) color=$fgcyn; label="[DEBUG]";;
        INFO) color=$fggrn; label="[INFO ]";;
        WARN|WARNING) color=$fggld; label="[WARN ]";;
        ERROR) color=$fgmag; label="[ERROR]";;
        CRIT|CRITICAL) color=$fgred; label="[CRIT ]";;
    esac

    # Create header and footer
    local dash_count=$(( (width - ${#header_name} - 2) / 2 ))
    local header_l header_r
    header_l="$(printf '%*s' "$dash_count" '' | tr ' ' "$char")"
    header_r="$header_l"
    [[ $(( (width - ${#header_name}) % 2 )) -eq 1 ]] && header_r="${header_r}${char}"
    local header; header=$(printf "%b%s%b %b%b%s%b %b%s%b" \
        "$color" \
        "$header_l" \
        "$reset" \
        "$color" \
        "$bold" \
        "$header_name" \
        "$reset" \
        "$color" \
        "$header_r" \
        "$reset" \
    )

    # Construct the footer
    local footer line
    # Generate the line of characters
    line="$(printf '%*s' "$width" '' | tr ' ' "$char")"
    footer="$(printf '%b%s%b' "$color" "$line" "$reset")"

    # Print header
    printf "%s\n" "$header"

    # Print the message, if provided
    if [[ -n "$message" ]]; then
        # Fallback mechanism for wrap_messages
        local result primary overflow secondary
        if command -v wrap_messages >/dev/null 2>&1; then
            result=$(wrap_messages "$width" "$message" || true)
            primary="${result%%"${delimiter}"*}"
            result="${result#*"${delimiter}"}"
            overflow="${result%%"${delimiter}"*}"
        else
            primary="$message"
        fi
        # Print the formatted message
        printf "%b%s%b\n" "${color}" "${primary}" "${reset}"
        printf "%b%s%b\n" "${color}" "${overflow}" "${reset}"
    fi

    # Print stack trace
    local indent=$(( (width / 2) - ((longest_length + 28) / 2) ))
    indent=$(( indent < 0 ? 0 : indent ))
    if [[ -z "${displayed_stack[*]}" ]]; then
        printf "%b[WARN ]%b Stack trace is empty.\n" "$fggld" "$reset" >&2
    else
        for ((i = ${#displayed_stack[@]} - 1, idx = 0; i >= 0; i--, idx++)); do
            IFS='|' read -r func line <<< "${displayed_stack[i]}"
            printf "%b%*s [%d] Function: %-*s Line: %4s%b\n" \
                "$color" "$indent" ">" "$idx" "$((longest_length + 2))" "$func" "$line" "$reset"
        done
    fi

    # Print footer
    printf "%s\n\n" "$footer"
}

# -----------------------------------------------------------------------------
# @brief Logs a warning message with optional details and stack trace.
# @details This function logs a warning message with color-coded formatting
#          and optional details. It adjusts the output to fit within the
#          terminal's width and supports message wrapping if the
#          `wrap_messages` function is available. If `WARN_STACK_TRACE` is set
#          to `true`, a stack trace is also logged.
#
# @param $1 [Optional] The primary warning message. Defaults to
#                      "A warning was raised on this line" if not provided.
# @param $@ [Optional] Additional details to include in the warning message.
#
# @global FALLBACK_SCRIPT_NAME The name of the script to use if the script
#                              name cannot be determined.
# @global FUNCNAME             Bash array containing the function call stack.
# @global BASH_LINENO          Bash array containing the line numbers of
#                              function calls in the stack.
# @global WRAP_DELIMITER       The delimiter used for separating wrapped
#                              message parts.
# @global WARN_STACK_TRACE     If set to `true`, a stack trace will be logged.
# @global COLUMNS              The terminal's column width, used to format
#                              the output.
#
# @return None.
#
# @example
# warn "Configuration file missing." "Please check /etc/config."
# warn "Invalid syntax in the configuration file."
#
# @note This function requires `tput` for terminal width detection and ANSI
#       formatting, with fallbacks for minimal environments.
# -----------------------------------------------------------------------------
warn() {
    # Initialize variables
    local script="${FALLBACK_SCRIPT_NAME:-unknown}"  # This script's name
    local func_name="${FUNCNAME[1]:-main}"          # Calling function
    local caller_line=${BASH_LINENO[0]:-0}          # Calling line

    # Get valid error code
    local error_code
    if [[ -n "${1:-}" && "$1" =~ ^[0-9]+$ ]]; then
        error_code=$((10#$1))  # Convert to numeric
        shift
    else
        error_code=1  # Default to 1 if not numeric
    fi

    # Configurable delimiter
    local delimiter="${WRAP_DELIMITER:-␞}"

    # Get the primary message
    local message
    message=$(sed -E 's/^[[:space:]]*//;s/[[:space:]]*$//' <<< "${1:-A warning was raised on this line}")
    [[ $# -gt 0 ]] && shift

    # Process details
    local details
    details=$(sed -E 's/^[[:space:]]*//;s/[[:space:]]*$//' <<< "$*")

    # Recalculate terminal columns
    COLUMNS=$( (command -v tput >/dev/null && tput cols) || printf "80")
    COLUMNS=$((COLUMNS > 0 ? COLUMNS : 80)) # Ensure COLUMNS is a positive number
    local width
    width=${COLUMNS:-80}                    # Max console width

    # Escape sequences for colors and attributes
    local reset="\033[0m"           # Reset text
    local bold="\033[1m"            # Bold text
    local fggld="\033[38;5;220m"    # Gold text
    local fgcyn="\033[36m"          # Cyan text
    local fgblu="\033[34m"          # Blue text

    # Format prefix
    format_prefix() {
        local color=${1:-"\033[0m"}
        local label="${2:-'[WARN ] [unknown:main:0]'}"
        # Create prefix
        printf "%b%b%s%b %b[%s:%s:%s]%b " \
            "${bold}" \
            "${color}" \
            "${label}" \
            "${reset}" \
            "${bold}" \
            "${script}" \
            "${func_name}" \
            "${caller_line}" \
            "${reset}"
    }

    # Generate prefixes
    local warn_prefix extd_prefix dets_prefix
    warn_prefix=$(format_prefix "$fggld" "[WARN ]")
    extd_prefix=$(format_prefix "$fgcyn" "[EXTND]")
    dets_prefix=$(format_prefix "$fgblu" "[DETLS]")

    # Strip ANSI escape sequences for length calculation
    local plain_warn_prefix adjusted_width
    plain_warn_prefix=$(printf "%s" "$warn_prefix" | sed -E 's/(\x1b\[[0-9;]*[a-zA-Z]|\x1b\([a-zA-Z])//g; s/^[[:space:]]*//; s/[[:space:]]*$//')
    adjusted_width=$((width - ${#plain_warn_prefix} - 1))

    # Fallback mechanism for `wrap_messages`
    local result primary overflow secondary
    if command -v wrap_messages >/dev/null 2>&1; then
        result=$(wrap_messages "$adjusted_width" "$message" "$details" || true)
        primary="${result%%"${delimiter}"*}"
        result="${result#*"${delimiter}"}"
        overflow="${result%%"${delimiter}"*}"
        secondary="${result#*"${delimiter}"}"
    else
        primary="$message"
        overflow=""
        secondary="$details"
    fi

    # Print the primary message
    printf "%s%s\n" "$warn_prefix" "$primary" >&2

    # Print overflow lines
    if [[ -n "$overflow" ]]; then
        while IFS= read -r line; do
            printf "%s%s\n" "$extd_prefix" "$line" >&2
        done <<< "$overflow"
    fi

    # Print secondary details
    if [[ -n "$secondary" ]]; then
        while IFS= read -r line; do
            printf "%s%s\n" "$dets_prefix" "$line" >&2
        done <<< "$secondary"
    fi

    # Execute stack trace if WARN_STACK_TRACE is enabled
    if [[ "${WARN_STACK_TRACE:-false}" == "true" ]]; then
        stack_trace "WARNING" "${message}" "${secondary}"
    fi
}

# -----------------------------------------------------------------------------
# @brief Terminates the script with a critical error message.
# @details This function is used to log a critical error message with optional
#          details and exit the script with the specified error code. It
#          supports formatting the output with ANSI color codes, dynamic
#          column widths, and optional multi-line message wrapping.
#
#          If the optional `wrap_messages` function is available, it will be
#          used to wrap and combine messages. Otherwise, the function falls
#          back to printing the primary message and details as-is.
#
# @param $1 [optional] Numeric error code. Defaults to 1.
# @param $2 [optional] Primary error message. Defaults to "Critical error".
# @param $@ [optional] Additional details to include in the error message.
#
# @global FALLBACK_SCRIPT_NAME The script name to use as a fallback.
# @global FUNCNAME             Bash array containing the call stack.
# @global BASH_LINENO          Bash array containing line numbers of the stack.
# @global WRAP_DELIMITER       Delimiter used when combining wrapped messages.
# @global COLUMNS              The terminal's column width, used to adjust
#                              message formatting.
#
# @return None. This function does not return.
# @exit Exits the script with the specified error code.
#
# @example
# die 127 "File not found" "Please check the file path and try again."
# die "Critical configuration error"
# -----------------------------------------------------------------------------
die() {
    # Initialize variables
    local script="${FALLBACK_SCRIPT_NAME:-unknown}"  # This script's name
    local func_name="${FUNCNAME[1]:-main}"          # Calling function
    local caller_line=${BASH_LINENO[0]:-0}          # Calling line

    # Get valid error code
    if [[ -n "${1:-}" && "$1" =~ ^[0-9]+$ ]]; then
        error_code=$((10#$1))  # Convert to numeric
        shift
    else
        error_code=1  # Default to 1 if not numeric
    fi

    # Configurable delimiter
    local delimiter="${WRAP_DELIMITER:-␞}"

    # Process the primary message
    local message
    message=$(sed -E 's/^[[:space:]]*//;s/[[:space:]]*$//' <<< "${1:-Critical error}")

    # Only shift if there are remaining arguments
    [[ $# -gt 0 ]] && shift

    # Process details
    local details
    details=$(sed -E 's/^[[:space:]]*//;s/[[:space:]]*$//' <<< "$*")

    # Recalculate terminal columns
    COLUMNS=$( (command -v tput >/dev/null && tput cols) || printf "80")
    COLUMNS=$((COLUMNS > 0 ? COLUMNS : 80)) # Ensure COLUMNS is a positive number
    local width
    width=${COLUMNS:-80}                    # Max console width

    # Escape sequences as safe(r) alternatives to global tput values
    # General attributes
    local reset="\033[0m"
    local bold="\033[1m"
    # Foreground colors
    local fgred="\033[31m" # Red text
    local fgcyn="\033[36m" # Cyan text
    local fgblu="\033[34m" # Blue text

    # Format prefix
    format_prefix() {
        local color=${1:-"\033[0m"}
        local label="${2:-'[CRIT ] [unknown:main:0]'}"
        # Create prefix
        printf "%b%b%s%b %b[%s:%s:%s]%b " \
            "${bold}" \
            "${color}" \
            "${label}" \
            "${reset}" \
            "${bold}" \
            "${script}" \
            "${func_name}" \
            "${caller_line}" \
            "${reset}"
    }

    # Generate prefixes
    local crit_prefix extd_prefix dets_prefix
    crit_prefix=$(format_prefix "$fgred" "[CRIT ]")
    extd_prefix=$(format_prefix "$fgcyn" "[EXTND]")
    dets_prefix=$(format_prefix "$fgblu" "[DETLS]")

    # Strip ANSI escape sequences for length calculation
    local plain_crit_prefix adjusted_width
    plain_crit_prefix=$(printf "%s" "$crit_prefix" | sed -E 's/(\x1b\[[0-9;]*[a-zA-Z]|\x1b\([a-zA-Z])//g; s/^[[:space:]]*//; s/[[:space:]]*$//')
    adjusted_width=$((width - ${#plain_crit_prefix} - 1))

    # Fallback mechanism for `wrap_messages` since it is external
    local result primary overflow secondary
    if command -v wrap_messages >/dev/null 2>&1; then
        result=$(wrap_messages "$adjusted_width" "$message" "$details" || true)
        primary="${result%%"${delimiter}"*}"
        result="${result#*"${delimiter}"}"
        overflow="${result%%"${delimiter}"*}"
        secondary="${result#*"${delimiter}"}"
    else
        primary="$message"
        overflow=""
        secondary="$details"
    fi

    # Print the primary message
    printf "%s%s\n" "$crit_prefix" "$primary" >&2

    # Print overflow lines
    if [[ -n "$overflow" ]]; then
        while IFS= read -r line; do
            printf "%s%s\n" "$extd_prefix" "$line" >&2
        done <<< "$overflow"
    fi

    # Print secondary details
    if [[ -n "$secondary" ]]; then
        while IFS= read -r line; do
            printf "%s%s\n" "$dets_prefix" "$line" >&2
        done <<< "$secondary"
    fi

    # Execute stack trace
    stack_trace "CRITICAL" "${message}" "${secondary}"

    # Exit with the specified error code
    exit "$error_code"
}

# -----------------------------------------------------------------------------
# @brief Add a dot (`.`) at the beginning of a string if it's missing.
# @details This function ensures the input string starts with a leading dot.
#          If the input string is empty, the function logs a warning and
#          returns an error code.
#
# @param $1 The input string to process.
#
# @return Outputs the modified string with a leading dot if it was missing.
# @retval 1 If the input string is empty.
#
# @example
# add_dot "example"   # Outputs ".example"
# add_dot ".example"  # Outputs ".example"
# add_dot ""          # Logs a warning and returns an error.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
add_dot() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local input=${1:-}  # Input string to process

    # Validate input
    if [[ -z "$input" ]]; then
        warn "Input to add_dot cannot be empty."
            debug_end "$debug"
        return 1
    fi

    # Add a leading dot if it's missing
    if [[ "$input" != .* ]]; then
        input=".$input"
    fi

    debug_end "$debug"
    printf "%s\n" "$input"
}

# -----------------------------------------------------------------------------
# @brief Removes a leading dot from the input string, if present.
# @details This function checks if the input string starts with a dot (`.`)
#          and removes it. If the input is empty, an error message is logged.
#          The function handles empty strings by returning an error and logging
#          an appropriate warning message.
#
# @param $1 [required] The input string to process.
#
# @return 0 on success, 1 on failure (when the input is empty).
#
# @example
# remove_dot ".hidden"  # Output: "hidden"
# remove_dot "visible"  # Output: "visible"
# -----------------------------------------------------------------------------
remove_dot() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local input=${1:-}  # Input string to process

    # Validate input
    if [[ -z "$input" ]]; then
        warn "ERROR" "Input to remove_dot cannot be empty."
        debug_end "$debug"
        return 1
    fi

    # Remove the leading dot if present
    if [[ "$input" == *. ]]; then
        input="${input#.}"
    fi

    debug_end "$debug"
    printf "%s\n" "$input"
}

# -----------------------------------------------------------------------------
# @brief Adds a period to the end of the input string if it doesn't already
#        have one.
# @details This function checks if the input string has a trailing period.
#          If not, it appends one. If the input is empty, an error is logged.
#
# @param $1 [required] The input string to process.
#
# @return The input string with a period added at the end (if missing).
# @return 1 If the input string is empty.
#
# @example
# result=$(add_period "Hello")
# echo "$result"  # Output: "Hello."
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
add_period() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local input=${1:-}  # Input string to process

    # Validate input
    if [[ -z "$input" ]]; then
        warn "Input to add_period cannot be empty."
        debug_end "$debug" # Next line must be a return/print/exit
        return 1
    fi

    # Add a trailing period if it's missing
    if [[ "$input" != *. ]]; then
        input="$input."
    fi

    debug_end "$debug"
    printf "%s\n" "$input"
}

# -----------------------------------------------------------------------------
# @brief Remove a trailing period (`.`) from a string if present.
# @details This function processes the input string and removes a trailing
#          period if it exists. If the input string is empty, the function logs
#          an error and returns an error code.
#
# @param $1 The input string to process.
#
# @return Outputs the modified string without a trailing period if one was
#         present.
# @retval 1 If the input string is empty.
#
# @example
# remove_period "example."  # Outputs "example"
# remove_period "example"   # Outputs "example"
# remove_period ""          # Logs an error and returns an error code.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
remove_period() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local input=${1:-}  # Input string to process

    # Validate input
    if [[ -z "$input" ]]; then
        warn "ERROR" "Input to remove_period cannot be empty."
            debug_end "$debug"
        return 1
    fi

    # Remove the trailing period if present
    if [[ "$input" == *. ]]; then
        input="${input%.}"
    fi

    debug_end "$debug"
    printf "%s\n" "$input"
}

# -----------------------------------------------------------------------------
# @brief Add a trailing slash (`/`) to a string if it's missing.
# @details This function ensures that the input string ends with a trailing
#          slash. If the input string is empty, the function logs an error and
#          returns an error code.
#
# @param $1 The input string to process.
#
# @return Outputs the modified string with a trailing slash if one was missing.
# @retval 1 If the input string is empty.
#
# @example
# add_slash "/path/to/directory"  # Outputs "/path/to/directory/"
# add_slash "/path/to/directory/" # Outputs "/path/to/directory/"
# add_slash ""                    # Logs an error and returns an error code.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
add_slash() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "${input:-}" ]]; then
        warn "ERROR" "Input to add_slash cannot be empty."
            debug_end "$debug"
        return 1
    fi

    # Add a trailing slash if it's missing
    if [[ "$input" != */ ]]; then
        input="$input/"
    fi

    debug_end "$debug"
    printf "%s\n" "$input"
}

# -----------------------------------------------------------------------------
# @brief Remove a trailing slash (`/`) from a string if present.
# @details This function ensures that the input string does not end with a
#          trailing slash. If the input string is empty, the function logs an
#          error and returns an error code.
#
# @param $1 The input string to process.
#
# @return Outputs the modified string without a trailing slash if one was
#         present.
# @retval 1 If the input string is empty.
#
# @example
# remove_slash "/path/to/directory/"  # Outputs "/path/to/directory"
# remove_slash "/path/to/directory"   # Outputs "/path/to/directory"
# remove_slash ""                     # Logs an error and returns an error code
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
remove_slash() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "${input:-}" ]]; then
        warn "ERROR" "Input to remove_slash cannot be empty."
            debug_end "$debug"
        return 1
    fi

    # Remove the trailing slash if present
    if [[ "$input" == */ ]]; then
        input="${input%/}"
    fi

    debug_end "$debug"
    printf "%s\n" "$input"
}

# -----------------------------------------------------------------------------
# @brief Pauses execution and waits for user input to continue.
# @details This function displays a message prompting the user to press any key
#          to continue. It waits for a key press, then resumes execution.
#
# @example
# pause
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
pause() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    printf "Press any key to continue.\n"
    read -n 1 -sr key < /dev/tty || true
    printf "\n"
    debug_print "$key" "$debug"

    debug_end "$debug"
    return 0
}

############
### Print/Display Environment Functions
############

# -----------------------------------------------------------------------------
# @brief Print the system information to the log.
# @details Extracts and logs the system's name and version using information
#          from `/etc/os-release`. If the information cannot be extracted, logs
#          a warning message. Includes debug output when the `debug` argument is provided.
#
# @param $1 [Optional] Debug flag to enable detailed output (`debug`).
#
# @global None
#
# @return None
#
# @example
# print_system debug
# Outputs system information with debug logs enabled.
# -----------------------------------------------------------------------------
# shellcheck disable=2329
print_system() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Declare local variables
    local system_name

    # Extract system name and version from /etc/os-release
    system_name=$(grep '^PRETTY_NAME=' /etc/os-release 2>/dev/null | cut -d '=' -f2 | tr -d '"')

    # Debug: Log extracted system name
    debug_print "Extracted system name: ${system_name:-<empty>}\n" "$debug"

    # Check if system_name is empty and log accordingly
    if [[ -z "${system_name:-}" ]]; then
        warn "System: Unknown (could not extract system information)."
        debug_print "System information could not be extracted." "$debug"
    else
        logI "System: $system_name."  # Log the system information
        debug_print "Logged system information: $system_name" "$debug"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Print the script version and optionally log it.
# @details This function displays the version of the script stored in the global
#          variable `SEM_VER`. If called by `process_args`, it uses `printf` to
#          display the version; otherwise, it logs the version using `logI`.
#          If the debug flag is set to "debug," additional debug information
#          will be printed.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @global THIS_SCRIPT The name of the script.
# @global SEM_VER The version of the script.
# @global REPO_NAME The name of the repository.
#
# @return None
#
# @example
# print_version debug
# -----------------------------------------------------------------------------
print_version() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Check the name of the calling function
    local caller="${FUNCNAME[1]}"

    if [[ "$caller" == "process_args" ]]; then
        printf "%s: version %s\n" "$THIS_SCRIPT" "$SEM_VER" # Display the script name and version
    else
        logI "Running $(repo_to_title_case "$REPO_NAME")'s '$THIS_SCRIPT', version $SEM_VER"
    fi

    debug_end "$debug"
    return 0
}

############
### Check Environment Functions
############

# -----------------------------------------------------------------------------
# @brief Determine the script's execution context.
# @details Identifies how the script was executed, returning one of the
#          predefined context codes. Handles errors gracefully and outputs
#          additional debugging information when the "debug" argument is
#          passed.
#
# Context Codes:
#   - `0`: Script executed via pipe.
#   - `1`: Script executed with `bash` in an unusual way.
#   - `2`: Script executed directly (local or from PATH).
#   - `3`: Script executed from within a GitHub repository.
#   - `4`: Script executed from a PATH location.
#
# @param $1 [Optional] Pass "debug" to enable verbose logging for debugging
#                      purposes.
#
# @throws Exits with an error if the script path cannot be resolved or
#         directory traversal exceeds the maximum depth.
#
# @return Returns a context code (described above) indicating the script's
#         execution context.
# -----------------------------------------------------------------------------
determine_execution_context() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local script_path   # Full path of the script
    local current_dir   # Temporary variable to traverse directories
    local max_depth=10  # Limit for directory traversal depth
    local depth=0       # Counter for directory traversal

    # Check if the script is executed via pipe
    if [[ "$0" == "bash" ]]; then
        if [[ -p /dev/stdin ]]; then
            debug_print "Execution context: Script executed via pipe." "$debug"
                    debug_end "$debug"
            return 0  # Execution via pipe
        else
            warn "Unusual bash execution detected."
                    debug_end "$debug"
            return 1  # Unusual bash execution
        fi
    fi

    # Get the script path
    script_path=$(realpath "$0" 2>/dev/null) || script_path=$(pwd)/$(basename "$0")
    if [[ ! -f "$script_path" ]]; then
            debug_end "$debug"
        die 1 "Unable to resolve script path: $script_path"
    fi
    debug_print "Resolved script path: $script_path" "$debug"

    # Initialize current_dir with the directory part of script_path
    current_dir="${script_path%/*}"
    current_dir="${current_dir:-.}"

    # Safeguard against invalid current_dir during initialization
    if [[ ! -d "$current_dir" ]]; then
            debug_end "$debug"
        die 1 "Invalid starting directory: $current_dir"
    fi

    # Traverse upwards to detect a GitHub repository
    while [[ "$current_dir" != "/" && $depth -lt $max_depth ]]; do
        if [[ -d "$current_dir/.git" ]]; then
            debug_print "GitHub repository detected at depth $depth: $current_dir" "$debug"
                    debug_end "$debug"
            return 3  # Execution within a GitHub repository
        fi
        current_dir=$(dirname "$current_dir") # Move up one directory
        ((depth++))
    done

    # Handle loop termination conditions
    if [[ $depth -ge $max_depth ]]; then
            debug_end "$debug"
        die 1 "Directory traversal exceeded maximum depth ($max_depth)"
    fi

    # Check if the script is executed from a PATH location
    local resolved_path
    resolved_path=$(command -v "$(basename "$0")" 2>/dev/null)
    if [[ "$resolved_path" == "$script_path" ]]; then
        debug_print "Script executed from a PATH location: $resolved_path." "$debug"
            debug_end "$debug"
        return 4  # Execution from a PATH location
    fi

    # Default: Direct execution from the local filesystem
    debug_print "Default context: Script executed directly." "$debug"

    debug_end "$debug"
    return 2
}

# -----------------------------------------------------------------------------
# @brief Handle the execution context of the script.
# @details Determines the script's execution context by invoking
#          `determine_execution_context` and sets global variables based on
#          the context. Outputs debug information if the "debug" argument is
#          passed. Provides safeguards for unknown or invalid context codes.
#
# @param $1 [Optional] Pass "debug" to enable verbose logging for debugging
#                      purposes.
#
# @throws Exits with an error if an invalid context code is returned.
#
# @return None
# -----------------------------------------------------------------------------
handle_execution_context() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Call determine_execution_context and capture its output
    determine_execution_context "$debug"
    local context=$?  # Capture the return code to determine context

    # Validate the context
    if ! [[ "$context" =~ ^[0-4]$ ]]; then
        debug_end "$debug"
        die 1 "Invalid context code returned: $context"
    fi

    # Initialize and set global variables based on the context
    case $context in
        0)
            THIS_SCRIPT="piped_script"
            USE_LOCAL=false
            IS_REPO=false
            IS_PATH=false
            debug_print "Execution context: Script was piped (e.g., 'curl url | sudo bash')." "$debug"
            ;;
        1)
            THIS_SCRIPT="piped_script"
            USE_LOCAL=false
            IS_REPO=false
            IS_PATH=false
            warn "Execution context: Script run with 'bash' in an unusual way."
            ;;
        2)
            THIS_SCRIPT=$(basename "$0")
            USE_LOCAL=true
            IS_REPO=false
            IS_PATH=false
            debug_print "Execution context: Script executed directly from $THIS_SCRIPT." "$debug"
            ;;
        3)
            THIS_SCRIPT=$(basename "$0")
            USE_LOCAL=true
            IS_REPO=true
            IS_PATH=false
            debug_print "Execution context: Script is within a GitHub repository." "$debug"
            ;;
        4)
            THIS_SCRIPT=$(basename "$0")
            USE_LOCAL=true
            IS_REPO=false
            IS_PATH=true
            debug_print "Execution context: Script executed from a PATH location ($(command -v "$THIS_SCRIPT"))" "$debug"
            ;;
        *)
                    debug_end "$debug"
            die 99 "Unknown execution context."
            ;;
    esac

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Enforce that the script is run directly with `sudo`.
# @details Ensures the script is executed with `sudo` privileges and not:
#          - From a `sudo su` shell.
#          - As the root user directly.
#
# @global REQUIRE_SUDO Boolean indicating if `sudo` is required.
# @global SUDO_USER User invoking `sudo`.
# @global SUDO_COMMAND The command invoked with `sudo`.
# @global THIS_SCRIPT Name of the current script.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @return None
# @exit 1 if the script is not executed correctly.
#
# @example
# enforce_sudo debug
# -----------------------------------------------------------------------------
enforce_sudo() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    if [[ "$REQUIRE_SUDO" == true ]]; then
        if [[ "$EUID" -eq 0 && -n "$SUDO_USER" && "$SUDO_COMMAND" == *"$0"* ]]; then
            debug_print "Sudo conditions met. Proceeding." "$debug"
            # Script is properly executed with `sudo`
        elif [[ "$EUID" -eq 0 && -n "$SUDO_USER" ]]; then
            debug_print "Script run from a root shell. Exiting." "$debug"
            die 1 "This script should not be run from a root shell." \
                  "Run it with 'sudo $THIS_SCRIPT' as a regular user."
        elif [[ "$EUID" -eq 0 ]]; then
            debug_print "Script run as root. Exiting." "$debug"
            die 1 "This script should not be run as the root user." \
                  "Run it with 'sudo $THIS_SCRIPT' as a regular user."
        else
            debug_print "Script not run with sudo. Exiting." "$debug"
            die 1 "This script requires 'sudo' privileges." \
                  "Please re-run it using 'sudo $THIS_SCRIPT'."
        fi
    fi

    debug_print "Function parameters:" \
        "\n\t- REQUIRE_SUDO='${REQUIRE_SUDO:-(not set)}'" \
        "\n\t- EUID='$EUID'" \
        "\n\t- SUDO_USER='${SUDO_USER:-(not set)}'" \
        "\n\t- SUDO_COMMAND='${SUDO_COMMAND:-(not set)}'" "$debug"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check for required dependencies and report any missing ones.
# @details Iterates through the dependencies listed in the global array
#          `DEPENDENCIES`, checking if each one is installed. Logs missing
#          dependencies and exits the script with an error code if any are
#          missing.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @global DEPENDENCIES Array of required dependencies.
#
# @return None
# @exit 1 if any dependencies are missing.
#
# @example
# validate_depends debug
# -----------------------------------------------------------------------------
validate_depends() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Declare local variables
    local missing=0  # Counter for missing dependencies
    local dep        # Iterator for dependencies

    # Iterate through dependencies
    for dep in "${DEPENDENCIES[@]}"; do
        if ! command -v "$dep" &>/dev/null; then
            warn "Missing dependency: $dep"
            ((missing++))
            debug_print "Missing dependency: $dep" "$debug"
        else
            debug_print "Found dependency: $dep" "$debug"
        fi
    done

    # Handle missing dependencies
    if ((missing > 0)); then
            debug_end "$debug"
        die 1 "Missing $missing dependencies. Install them and re-run the script."
    fi

    debug_print "All dependencies are present." "$debug"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check the availability of critical system files.
# @details Verifies that each file listed in the `SYSTEM_READS` array exists
#          and is readable. Logs an error for any missing or unreadable files
#          and exits the script if any issues are found.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @global SYSTEM_READS Array of critical system file paths to check.
#
# @return None
# @exit 1 if any required files are missing or unreadable.
#
# @example
# validate_sys_accs debug
# -----------------------------------------------------------------------------
validate_sys_accs() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Declare local variables
    local missing=0  # Counter for missing or unreadable files
    local file       # Iterator for files

    # Iterate through system files
    for file in "${SYSTEM_READS[@]}"; do
        if [[ ! -r "$file" ]]; then
            warn "Missing or unreadable file: $file"
            ((missing++))
            debug_print "Missing or unreadable file: $file" "$debug"
        else
            debug_print "File is accessible: $file" "$debug"
        fi
    done

    # Handle missing files
    if ((missing > 0)); then
            debug_end "$debug"
        die 1 "Missing or unreadable $missing critical system files."
    fi

    debug_print "All critical system files are accessible." "$debug"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Validate the existence of required environment variables.
# @details Checks if the environment variables specified in the `ENV_VARS`
#          array are set. Logs any missing variables and exits the script if
#          any are missing.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @global ENV_VARS Array of required environment variables.
#
# @return None
# @exit 1 if any environment variables are missing.
#
# @example
# validate_env_vars debug
# -----------------------------------------------------------------------------
validate_env_vars() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Declare local variables
    local missing=0  # Counter for missing environment variables
    local var        # Iterator for environment variables

    # Iterate through environment variables
    for var in "${ENV_VARS[@]}"; do
        if [[ -z "${!var:-}" ]]; then
            printf "ERROR: Missing environment variable: %s\n" "$var" >&2
            ((missing++))
            debug_print "Missing environment variable: $var" "$debug"
        else
            debug_print "Environment variable is set: $var=${!var}" "$debug"
        fi
    done

    # Handle missing variables
    if ((missing > 0)); then
        printf "ERROR: Missing %d required environment variables. Ensure all required environment variables are set and re-run the script.\n" "$missing" >&2
            debug_end "$debug"
        exit 1
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check if the script is running in a Bash shell.
# @details Ensures the script is executed with Bash, as it may use Bash-
#          specific features. If the "debug" argument is passed, detailed
#          logging will be displayed for each check.
#
# @param $1 [Optional] "debug" to enable verbose output for all checks.
#
# @global BASH_VERSION The version of the Bash shell being used.
#
# @return None
# @exit 1 if not running in Bash.
#
# @example
# check_bash
# check_bash debug
# -----------------------------------------------------------------------------
check_bash() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Ensure the script is running in a Bash shell
    if [[ -z "${BASH_VERSION:-}" ]]; then
            debug_end "$debug"
        die 1 "This script requires Bash. Please run it with Bash."
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check if the current Bash version meets the minimum required version.
# @details Compares the current Bash version against a required version
#          specified in the global variable `MIN_BASH_VERSION`. If
#          `MIN_BASH_VERSION` is "none", the check is skipped. Outputs debug
#          information if enabled.
#
# @param $1 [Optional] "debug" to enable verbose output for this check.
#
# @global MIN_BASH_VERSION Minimum required Bash version (e.g., "4.0") or
#                          "none".
# @global BASH_VERSINFO Array containing the major and minor versions of the
#         running Bash.
#
# @return None
# @exit 1 if the Bash version is insufficient.
#
# @example
# check_sh_ver
# check_sh_ver debug
# -----------------------------------------------------------------------------
check_sh_ver() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local required_version="${MIN_BASH_VERSION:-none}"

    # If MIN_BASH_VERSION is "none", skip version check
    if [[ "$required_version" == "none" ]]; then
        debug_print "Bash version check is disabled (MIN_BASH_VERSION='none')." "$debug"
    else
        debug_print "Minimum required Bash version is set to '$required_version'." "$debug"

        # Extract the major and minor version components from the required version
        local required_major="${required_version%%.*}"
        local required_minor="${required_version#*.}"
        required_minor="${required_minor%%.*}"

        # Log current Bash version for debugging
        debug_print "Current Bash version is ${BASH_VERSINFO[0]}.${BASH_VERSINFO[1]}." "$debug"

        # Compare the current Bash version with the required version
        if (( BASH_VERSINFO[0] < required_major ||
              (BASH_VERSINFO[0] == required_major && BASH_VERSINFO[1] < required_minor) )); then
                    debug_end "$debug"
            die 1 "This script requires Bash version $required_version or newer."
        fi
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check system bitness compatibility.
# @details Validates whether the current system's bitness matches the supported
#          configuration. Outputs debug information if debug mode is enabled.
#
# @param $1 [Optional] "debug" to enable verbose output for the check.
#
# @global SUPPORTED_BITNESS Specifies the bitness supported by the script
#         ("32", "64", or "both").
#
# @return None
# @exit 1 if the system bitness is unsupported.
#
# @example
# check_bitness
# check_bitness debug
# -----------------------------------------------------------------------------
check_bitness() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local bitness  # Stores the detected bitness of the system.

    # Detect the system bitness
    bitness=$(getconf LONG_BIT)

    # Debugging: Detected system bitness
    debug_print "Detected system bitness: $bitness-bit." "$debug"

    case "$SUPPORTED_BITNESS" in
        "32")
            debug_print "Script supports only 32-bit systems." "$debug"
            if [[ "$bitness" -ne 32 ]]; then
                debug_end "$debug"
                die 1 "Only 32-bit systems are supported. Detected $bitness-bit system."
            fi
            ;;
        "64")
            debug_print "Script supports only 64-bit systems." "$debug"
            if [[ "$bitness" -ne 64 ]]; then
                debug_end "$debug"
                die 1 "Only 64-bit systems are supported. Detected $bitness-bit system."
            fi
            ;;
        "both")
            debug_print "Script supports both 32-bit and 64-bit systems." "$debug"
            ;;
        *)
            debug_print "Invalid SUPPORTED_BITNESS configuration: '$SUPPORTED_BITNESS'." "$debug"
            debug_end "$debug"
            die 1 "Configuration error: Invalid value for SUPPORTED_BITNESS ('$SUPPORTED_BITNESS')."
            ;;
    esac

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check Raspbian OS version compatibility.
# @details This function ensures that the Raspbian version is within the
#          supported range and logs an error if the compatibility check fails.
#
# @param $1 [Optional] "debug" to enable verbose output for this check.
#
# @global MIN_OS Minimum supported OS version.
# @global MAX_OS Maximum supported OS version (-1 indicates no upper limit).
# @global log_message Function for logging messages.
# @global die Function to handle critical errors and terminate the script.
#
# @return None Exits the script with an error code if the OS version is
#         incompatible.
#
# @example
# check_release
# check_release debug
# -----------------------------------------------------------------------------
check_release() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local ver  # Holds the extracted version ID from /etc/os-release.

    # Ensure the file exists and is readable.
    if [[ ! -f /etc/os-release || ! -r /etc/os-release ]]; then
        die 1 "Unable to read /etc/os-release. Ensure this script is run on a compatible system."
    fi

    # Extract the VERSION_ID from /etc/os-release.
    if [[ -f /etc/os-release ]]; then
        ver=$(grep "VERSION_ID" /etc/os-release | awk -F "=" '{print $2}' | tr -d '"')
    else
        warn "File /etc/os-release not found."
        ver="unknown"
    fi
    debug_print "Raspbian version '$ver' detected." "$debug"

    # Ensure the extracted version is not empty.
    if [[ -z "${ver:-}" ]]; then
            debug_end "$debug"
        die 1 "VERSION_ID is missing or empty in /etc/os-release."
    fi

    # Check if the version is older than the minimum supported version.
    if [[ "$ver" -lt "$MIN_OS" ]]; then
            debug_end "$debug"
        die 1 "Raspbian version $ver is older than the minimum supported version ($MIN_OS)."
    fi

    # Check if the version is newer than the maximum supported version, if applicable.
    if [[ "$MAX_OS" -ne -1 && "$ver" -gt "$MAX_OS" ]]; then
            debug_end "$debug"
        die 1 "Raspbian version $ver is newer than the maximum supported version ($MAX_OS)."
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check if the detected Raspberry Pi model is supported.
# @details Reads the Raspberry Pi model from /proc/device-tree/compatible and
#          checks it against a predefined list of supported models. Logs an
#          error if the model is unsupported or cannot be detected. Optionally
#          outputs debug information about all models if `debug` is set to
#          `true`.
#
# @param $1 [Optional] "debug" to enable verbose output for all supported/
#                      unsupported models.
#
# @global SUPPORTED_MODELS Associative array of supported and unsupported
#         Raspberry Pi models.
# @global log_message Function for logging messages.
# @global die Function to handle critical errors and terminate the script.
#
# @return None Exits the script with an error code if the architecture is
#         unsupported.
#
# @example
# check_arch
# check_arch debug
# -----------------------------------------------------------------------------
check_arch() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local detected_model key full_name model chip
    local is_supported=false

    # Safely check if SUPPORTED_MODELS is set and if it allows all models
    if [[ "${SUPPORTED_MODELS[all]-}" == "Supported" ]]; then
        is_supported=true
        debug_print "All models are allowed, SUPPORTED_MODELS is set to 'all'" "$debug"
        debug_end "$debug"
        return 0
    fi

    # Read and process the compatible string
    if ! detected_model=$(tr '\0' '\n' < /proc/device-tree/compatible 2>/dev/null | sed -n 's/raspberrypi,//p'); then
        debug_end "$debug"
        die 1 "Failed to read or process /proc/device-tree/compatible. Ensure compatibility."
    fi

    # Check if the detected model is empty
    if [[ -z "${detected_model:-}" ]]; then
        debug_end "$debug"
        die 1 "No Raspberry Pi model found in /proc/device-tree/compatible. This system may not be supported."
    fi
    debug_print "Detected model: $detected_model" "$debug"

    # Iterate through supported models to check compatibility
    for key in "${!SUPPORTED_MODELS[@]}"; do
        IFS='|' read -r full_name model chip <<< "$key"
        if [[ "$model" == "$detected_model" ]]; then
            if [[ "${SUPPORTED_MODELS[$key]}" == "Supported" ]]; then
                is_supported=true
                debug_print "Model: '$full_name' ($chip) is supported." "$debug"
            else
                debug_end "$debug"
                die 1 "Model: '$full_name' ($chip) is not supported."
            fi
            break
        fi
    done

    # Debug output of all models if requested
    if [[ "$debug" == "debug" ]]; then
        # Arrays to hold supported and unsupported models
        declare -a supported_models=()
        declare -a unsupported_models=()

        # Group the models into supported and unsupported
        for key in "${!SUPPORTED_MODELS[@]}"; do
            IFS='|' read -r full_name model chip <<< "$key"
            if [[ "${SUPPORTED_MODELS[$key]}" == "Supported" ]]; then
                supported_models+=("$full_name ($chip)")
            else
                unsupported_models+=("$full_name ($chip)")
            fi
        done

        # Sort and print supported models
        if [[ ${#supported_models[@]} -gt 0 ]]; then
            printf "[DEBUG] Supported models:\n" >&2
            for model in $(printf "%s\n" "${supported_models[@]}" | sort); do
                printf "\t- %s\n" "$model" >&2
            done
        fi

        # Sort and print unsupported models
        if [[ ${#unsupported_models[@]} -gt 0 ]]; then
            printf "[DEBUG] Unsupported models:\n" >&2
            for model in $(printf "%s\n" "${unsupported_models[@]}" | sort); do
                printf "\t- %s\n" "$model" >&2
            done
        fi
    fi

    # Log an error if no supported model was found
    if [[ "$is_supported" == false ]]; then
        debug_end "$debug"
        die 1 "Detected Raspberry Pi model '$detected_model' is not recognized or supported."
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Validate proxy connectivity by testing a known URL.
# @details Uses `check_url` to verify connectivity through the provided proxy
#          settings.
#
# @param $1 [Optional] Proxy URL to validate (defaults to `http_proxy` or
#                      `https_proxy` if not provided).
# @param $2 [Optional] "debug" to enable verbose output for the proxy
#                      validation.
#
# @global http_proxy The HTTP proxy URL (if set).
# @global https_proxy The HTTPS proxy URL (if set).
#
# @return 0 if the proxy is functional, 1 otherwise.
#
# @example
# validate_proxy "http://myproxy.com:8080"
# validate_proxy debug
# -----------------------------------------------------------------------------
validate_proxy() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Check if proxy_url is passed
    local proxy_url=""
    # Check if proxy_url is the first argument (if set)
    if [[ -n "$1" && "$1" =~ ^https?:// ]]; then
        # First argument is proxy_url
        proxy_url="$1"
        shift  # Move to the next argument
    fi

    # Default to global proxy settings if no proxy is provided
    [[ -z "${proxy_url:-}" ]] && proxy_url="${http_proxy:-$https_proxy}"

    # Validate that a proxy is set
    if [[ -z "${proxy_url:-}" ]]; then
        warn "No proxy URL configured for validation."
            debug_end "$debug"
        return 1
    fi

    logI "Validating proxy: $proxy_url"

    # Test the proxy connectivity using check_url (passing the debug flag)
    if check_url "$proxy_url" "curl" "--silent --head --max-time 10 --proxy $proxy_url" "$debug"; then
        logI "Proxy $proxy_url is functional."
        debug_print "Proxy $proxy_url is functional." "$debug"
            debug_end "$debug"
        return 0
    else
        warn "Proxy $proxy_url is unreachable or misconfigured."
        debug_print "Proxy $proxy_url failed validation." "$debug"
            debug_end "$debug"
        return 1
    fi
}

# -----------------------------------------------------------------------------
# @brief Check connectivity to a URL using a specified tool.
# @details Attempts to connect to a given URL with `curl` or `wget` based on
#          the provided arguments. Ensures that the tool's availability is
#          checked and handles timeouts gracefully. Optionally prints debug
#          information if the "debug" flag is set.
#
# @param $1 The URL to test.
# @param $2 The tool to use for the test (`curl` or `wget`).
# @param $3 Options to pass to the testing tool (e.g., `--silent --head` for
#           `curl`).
# @param $4 [Optional] "debug" to enable verbose output during the check.
#
# @return 0 if the URL is reachable, 1 otherwise.
#
# @example
# check_url "http://example.com" "curl" "--silent --head" debug
# -----------------------------------------------------------------------------
check_url() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local url="$1"
    local tool="$2"
    local options="$3"

    # Validate inputs
    if [[ -z "${url:-}" ]]; then
        printf "ERROR: URL and tool parameters are required for check_url.\n" >&2
            debug_end "$debug"
        return 1
    fi

    # Check tool availability
    if ! command -v "$tool" &>/dev/null; then
        printf "ERROR: Tool '%s' is not installed or unavailable.\n" "$tool" >&2
            debug_end "$debug"
        return 1
    fi

    # Perform the connectivity check, allowing SSL and proxy errors
    local retval
    # shellcheck disable=2086
    if $tool $options "$url" &>/dev/null; then
        debug_print "Successfully connected to $#url using $tool." "$debug"
        retval=0
    else
        debug_print "Failed to connect to $url using $tool." "$debug"
        retval=1
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Comprehensive internet and proxy connectivity check.
# @details Combines proxy validation and direct internet connectivity tests
#          using `check_url`. Validates proxy configuration first, then tests
#          connectivity with and without proxies. Outputs debug information if
#          enabled.
#
# @param $1 [Optional] "debug" to enable verbose output for all checks.
#
# @global http_proxy Proxy URL for HTTP (if set).
# @global https_proxy Proxy URL for HTTPS (if set).
# @global no_proxy Proxy exclusions (if set).
#
# @return 0 if all tests pass, 1 if any test fails.
#
# @example
# check_internet debug
# -----------------------------------------------------------------------------
check_internet() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local primary_url="http://google.com"
    local secondary_url="http://1.1.1.1"
    local proxy_valid=false

    # Validate proxy settings
    if [[ -n "${http_proxy:-}" || -n "${https_proxy:-}" ]]; then
        debug_print "Proxy detected. Validating proxy configuration." "$debug"
        if validate_proxy "$debug"; then  # Pass debug flag to validate_proxy
            proxy_valid=true
            debug_print "Proxy validation succeeded." "$debug"
        else
            warn "Proxy validation failed. Proceeding with direct connectivity checks."
        fi
    fi

    # Check connectivity using curl
    if command -v curl &>/dev/null; then
        debug_print "curl is available. Testing internet connectivity using curl." "$debug"

        # Check with proxy
        if $proxy_valid && curl --silent --head --max-time 10 --proxy "${http_proxy:-${https_proxy:-}}" "$primary_url" &>/dev/null; then
            logI "Internet is available using curl with proxy."
            debug_print "curl successfully connected via proxy." "$debug"
                    debug_end "$debug"
            return 0
        fi

        # Check without proxy
        if curl --silent --head --max-time 10 "$primary_url" &>/dev/null; then
            debug_print "curl successfully connected without proxy." "$debug"
                    debug_end "$debug"
            return 0
        fi

        debug_print "curl failed to connect." "$debug"
    else
        debug_print "curl is not available." "$debug"
    fi

    # Check connectivity using wget
    if command -v wget &>/dev/null; then
        debug_print "wget is available. Testing internet connectivity using wget." "$debug"

        # Check with proxy
        if $proxy_valid && wget --spider --quiet --timeout=10 --proxy="${http_proxy:-${https_proxy:-}}" "$primary_url" &>/dev/null; then
            logI "Internet is available using wget with proxy."
            debug_print "wget successfully connected via proxy." "$debug"
                    debug_end "$debug"
            return 0
        fi

        # Check without proxy
        if wget --spider --quiet --timeout=10 "$secondary_url" &>/dev/null; then
            logI "Internet is available using wget without proxy."
            debug_print "wget successfully connected without proxy." "$debug"
                    debug_end "$debug"
            return 0
        fi

        debug_print "wget failed to connect." "$debug"
    else
        debug_print "wget is not available." "$debug"
    fi

    # Final failure message
    warn "No internet connection detected after all checks."
    debug_print "All internet connectivity tests failed." "$debug"
    debug_end "$debug"
    return 1
}

############
### Logging Functions
############

# -----------------------------------------------------------------------------
# @brief Pads a number with leading spaces to achieve the desired width.
# @details This function takes a number and a specified width, and returns the
#          number formatted with leading spaces if necessary. The number is
#          guaranteed to be a valid non-negative integer, and the width is
#          checked to ensure it is a positive integer. If "debug" is passed as
#          the second argument, it defaults the width to 4 and provides debug
#          information.
#
# @param $1 [required] The number to be padded (non-negative integer).
# @param $2 [optional] The width of the output (defaults to 4 if not provided).
#
# @return 0 on success.
#
# @example
# pad_with_spaces 42 6  # Output: "   42"
# pad_with_spaces 123 5  # Output: "  123"
# -----------------------------------------------------------------------------
pad_with_spaces() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Declare locals
    local number="${1:-0}"  # Input number (mandatory)
    local width="${2:-4}"   # Optional width (default is 4)

    # If the second parameter is "debug", adjust the arguments
    if [[ "$width" == "debug" ]]; then
        debug="$width"
        width=4  # Default width to 4 if "debug" was passed in place of width
    fi

    # Validate input for the number
    if [[ -z "${number:-}" || ! "$number" =~ ^[0-9]+$ ]]; then
        die 1 "Input must be a valid non-negative integer."
    fi

    # Ensure the width is a positive integer
    if [[ ! "$width" =~ ^[0-9]+$ || "$width" -lt 1 ]]; then
        die 1 "Error: Width must be a positive integer."
    fi

    # Strip leading zeroes to prevent octal interpretation
    number=$((10#$number))  # Forces the number to be interpreted as base-10

    # Format the number with leading spaces and return it as a string
    printf "%${width}d\n" "$number"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Wraps a message into lines with ellipses for overflow or continuation.
# @details This function splits the message into lines, appending an ellipsis
#          for overflowed lines and prepending it for continuation lines. The
#          primary and secondary messages are processed separately and combined
#          with a delimiter.
#
# @param $1 [required] The message string to wrap.
# @param $2 [required] Maximum width of each line (numeric).
# @param $3 [optional] The secondary message string to include (defaults to
#                      an empty string).
#
# @global None.
#
# @throws None.
#
# @return A single string with wrapped lines and ellipses added as necessary.
#         The primary and secondary messages are separated by a delimiter.
#
# @example
# wrapped=$(wrap_messages "This is a long message" 50)
# echo "$wrapped"
# -----------------------------------------------------------------------------
wrap_messages() {
    local line_width=$1
    local primary=$2
    local secondary=${3:-}
    local delimiter="␞"

    # Validate input
    if [[ -z "$line_width" || ! "$line_width" =~ ^[0-9]+$ || "$line_width" -le 1 ]]; then
        printf "Error: Invalid line width '%s' in %s(). Must be a positive integer.\n" \
            "$line_width" "${FUNCNAME[0]}" >&2
        return 1
    fi

    # Inner function to wrap a single message
    wrap_message() {
        local message=$1
        local width=$2
        local result=()
        # Address faulty width with a min of 10
        local adjusted_width=$((width > 10 ? width - 1 : 10))

        while IFS= read -r line; do
            result+=("$line")
        done <<< "$(printf "%s\n" "$message" | fold -s -w "$adjusted_width")"

        for ((i = 0; i < ${#result[@]}; i++)); do
            result[i]=$(printf "%s" "${result[i]}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
            if ((i == 0)); then
                result[i]="${result[i]}…"
            elif ((i == ${#result[@]} - 1)); then
                result[i]="…${result[i]}"
            else
                result[i]="…${result[i]}…"
            fi
        done

        printf "%s\n" "${result[@]}"
    }

    # Process primary message
    local overflow=""
    if [[ ${#primary} -gt $line_width ]]; then
        local wrapped_primary
        wrapped_primary=$(wrap_message "$primary" "$line_width")
        overflow=$(printf "%s\n" "$wrapped_primary" | tail -n +2)
        primary=$(printf "%s\n" "$wrapped_primary" | head -n 1)
    fi

    # Process secondary message
    if [[ -n ${#secondary} && ${#secondary} -gt $line_width ]]; then
        secondary=$(wrap_message "$secondary" "$line_width")
    fi

    # Combine results
    printf "%s%b%s%b%s" \
        "$primary" \
        "$delimiter" \
        "$overflow" \
        "$delimiter" \
        "$secondary"
}

# -----------------------------------------------------------------------------
# @brief Log a message with optional details to the console and/or file.
# @details Handles combined logic for logging to console and/or file,
#          supporting optional details. If details are provided, they are
#          logged with an "[EXTENDED]" tag.
#
# @param $1 Timestamp of the log entry.
# @param $2 Log level (e.g., DEBUG, INFO, WARN, ERROR).
# @param $3 Color code for the log level.
# @param $4 Line number where the log entry originated.
# @param $5 The main log message.
# @param $6 [Optional] Additional details for the log entry.
#
# @global LOG_OUTPUT Specifies where to output logs ("console", "file", or
#         "both").
# @global LOG_FILE File path for log storage if `LOG_OUTPUT` includes "file".
# @global THIS_SCRIPT The name of the current script.
# @global RESET ANSI escape code to reset text formatting.
#
# @return None
# -----------------------------------------------------------------------------
print_log_entry() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Declare local variables at the start of the function
    local timestamp="$1"
    local level="$2"
    local color="$3"
    local lineno="$4"
    local message="$5"

    # Skip logging if the message is empty
    if [[ -z "$message" ]]; then
            debug_end "$debug"
        return 1
    fi

    # Log to file if required
    if [[ "$LOG_OUTPUT" == "file" || "$LOG_OUTPUT" == "both" ]]; then
        printf "%s [%s] [%s:%d] %s\\n" "$timestamp" "$level" "$THIS_SCRIPT" "$lineno" "$message" >> "$LOG_FILE"
    fi

    # Log to console if required and USE_CONSOLE is true
    if [[ "$USE_CONSOLE" == "true" && ("$LOG_OUTPUT" == "console" || "$LOG_OUTPUT" == "both") ]]; then
        printf "%b[%s]%b %s\\n" "$color" "$level" "$RESET" "$message"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Generate a timestamp and line number for log entries.
#
# @details This function retrieves the current timestamp and the line number of
#          the calling script. If the optional debug flag is provided, it will
#          print debug information, including the function name, caller's name,
#          and the line number where the function was called.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @return A pipe-separated string in the format: "timestamp|line_number".
#
# @example
# prepare_log_context "debug"
# -----------------------------------------------------------------------------
prepare_log_context() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local timestamp
    local lineno

    # Generate the current timestamp
    timestamp=$(date "+%Y-%m-%d %H:%M:%S")

    # Retrieve the line number of the caller
    lineno="${BASH_LINENO[2]}"

    # Pass debug flag to pad_with_spaces
    lineno=$(pad_with_spaces "$lineno" "$debug") # Pass debug flag

    # Return the pipe-separated timestamp and line number
    printf "%s|%s\n" "$timestamp" "$lineno"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Log a message with the specified log level.
# @details Logs messages to both the console and/or a log file, depending on
#          the configured log output. The function uses the `LOG_PROPERTIES`
#          associative array to determine the log level, color, and severity.
#          If the "debug" argument is provided, debug logging is enabled for
#          additional details.
#
# @param $1 Log level (e.g., DEBUG, INFO, ERROR). The log level controls the
#           message severity.
# @param $2 Main log message to log.
# @param $3 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @global LOG_LEVEL The current logging verbosity level.
# @global LOG_PROPERTIES Associative array defining log level properties, such
#         as severity and color.
# @global LOG_FILE Path to the log file (if configured).
# @global USE_CONSOLE Boolean flag to enable or disable console output.
# @global LOG_OUTPUT Specifies where to log messages ("file", "console",
#         "both").
#
# @return None
#
# @example
# log_message "INFO" "This is a message"
# log_message "INFO" "This is a message" "debug"
# -----------------------------------------------------------------------------
log_message() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Ensure the calling function is log_message_with_severity()
    if [[ "${FUNCNAME[1]}" != "log_message_with_severity" ]]; then
        warn "log_message() can only be called from log_message_with_severity()."
            debug_end "$debug"
        return 1
    fi

    local level="UNSET"
    local message="<no message>"

    local context timestamp lineno custom_level color severity config_severity

    # Get level if it exists (must be one of the predefined values)
    if [[ -n "$1" && "$1" =~ ^(DEBUG|INFO|WARNING|ERROR|CRITICAL|EXTENDED)$ ]]; then
        level="$1"
        shift  # Move to the next argument
    fi

    # Get message if it exists and is not "debug"
    if [[ -n "$1" ]]; then
        message="$1"
        shift  # Move to the next argument
    fi

    # Validate the log level and message if needed
    if [[ "$level" == "UNSET" || -z "${LOG_PROPERTIES[$level]:-}" || "$message" == "<no message>" ]]; then
        warn "Invalid log level '$level' or empty message."
            debug_end "$debug"
        return 1
    fi

    # Prepare log context (timestamp and line number)
    context=$(prepare_log_context "$debug")  # Pass debug flag to sub-function
    IFS="|" read -r timestamp lineno <<< "$context"

    # Extract log properties for the specified level
    IFS="|" read -r custom_level color severity <<< "${LOG_PROPERTIES[$level]}"

    # Check if all three values (custom_level, color, severity) were successfully parsed
    if [[ -z "$custom_level" || -z "$color" || -z "$severity" ]]; then
        warn "Malformed log properties for level '$level'. Using default values."
        custom_level="UNSET"
        color="$RESET"
        severity=0
    fi

    # Extract severity threshold for the configured log level
    IFS="|" read -r _ _ config_severity <<< "${LOG_PROPERTIES[$LOG_LEVEL]}"

    # Check for valid severity level
    if [[ -z "$config_severity" || ! "$config_severity" =~ ^[0-9]+$ ]]; then
        warn "Malformed severity value for level '$LOG_LEVEL'."
            debug_end "$debug"
        return 1
    fi

    # Skip logging if the message's severity is below the configured threshold
    if (( severity < config_severity )); then
            debug_end "$debug"
        return 0
    fi

    # Call print_log_entry to handle actual logging (to file and console)
    print_log_entry "$timestamp" "$custom_level" "$color" "$lineno" "$message" "$debug"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Log a message with the specified severity level.
# @details This function logs messages at the specified severity level and
#          handles extended details and debug information if provided.
#
# @param $1 Severity level (e.g., DEBUG, INFO, WARNING, ERROR, CRITICAL).
# @param $2 Main log message.
# @param $3 [Optional] Extended details for the log entry.
# @param $4 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @return None
#
# @example
# log_message_with_severity "ERROR" /
#   "This is an error message" "Additional details" "debug"
# -----------------------------------------------------------------------------
log_message_with_severity() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Exit if the calling function is not one of the allowed ones.
    # shellcheck disable=2076
    if [[ ! "logD logI logW logE logC logX" =~ "${FUNCNAME[1]}" ]]; then
        warn "Invalid calling function: ${FUNCNAME[1]}"
            debug_end "$debug"
        exit 1
    fi

    # Initialize variables
    local severity="INFO" # Default to INFO
    local message=""
    local extended_message=""

    # Get level if it exists (must be one of the predefined values)
    if [[ -n "$1" && "$1" =~ ^(DEBUG|INFO|WARNING|ERROR|CRITICAL|EXTENDED)$ ]]; then
        severity="$1"
    fi

    # Process arguments
    if [[ -n "$2" ]]; then
        message="$2"
    else
        warn "Message is required."
        debug_end "$debug"
        return 1
    fi

    if [[ -n "$3" ]]; then
        extended_message="$3"
    fi

    # Print debug information if the flag is set
    debug_print "Logging message at severity '$severity' with message='$message'." "$debug"
    debug_print "Extended message: '$extended_message'" "$debug"

    # Log the primary message
    log_message "$severity" "$message" "$debug"

    # Log the extended message if present
    if [[ -n "$extended_message" ]]; then
        logX "$extended_message" "$debug"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Logging wrapper functions for various severity levels.
# @details These functions provide shorthand access to
#          `log_message_with_severity()` with a predefined severity level. They
#          standardize the logging process by ensuring consistent severity
#          labels and argument handling.
#
# @param $1 [string] The primary log message. Must not be empty.
# @param $2 [optional, string] The extended message for additional details
#           (optional), sent to logX.
# @param $3 [optional, string] The debug flag. If set to "debug", enables
#           debug-level logging.
#
# @global None
#
# @return None
#
# @functions
# - logD(): Logs a message with severity level "DEBUG".
# - logI(): Logs a message with severity level "INFO".
# - logW(): Logs a message with severity level "WARNING".
# - logE(): Logs a message with severity level "ERROR".
# - logC(): Logs a message with severity level "CRITICAL".
# - logX(): Logs a message with severity level "EXTENDED".
#
# @example
#   logD "Debugging application startup."
#   logI "Application initialized successfully."
#   logW "Configuration file is missing a recommended value."
#   logE "Failed to connect to the database."
#   logC "System is out of memory and must shut down."
#   logX "Additional debug information for extended analysis."
# -----------------------------------------------------------------------------
# shellcheck disable=2317
logD() { log_message_with_severity "DEBUG" "${1:-}" "${2:-}" "${3:-}"; }
# shellcheck disable=2317
logI() { log_message_with_severity "INFO" "${1:-}" "${2:-}" "${3:-}"; }
# shellcheck disable=2317
logW() { log_message_with_severity "WARNING" "${1:-}" "${2:-}" "${3:-}"; }
# shellcheck disable=2317
logE() { log_message_with_severity "ERROR" "${1:-}" "${2:-}" "${3:-}"; }
# shellcheck disable=2317
logC() { log_message_with_severity "CRITICAL" "${1:-}" "${2:-}" "${3:-}"; }
# shellcheck disable=2317
logX() { log_message_with_severity "EXTENDED" "${1:-}" "${2:-}" "${3:-}"; }

# -----------------------------------------------------------------------------
# @brief Ensure the log file exists and is writable, with fallback to `/tmp` if
#        necessary.
# @details This function validates the specified log file's directory to ensure
#          it exists and is writable. If the directory is invalid or
#          inaccessible, it attempts to create it. If all else fails, the log
#          file is redirected to `/tmp`. A warning message is logged if
#          fallback is used.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @global LOG_FILE Path to the log file (modifiable to fallback location).
# @global THIS_SCRIPT The name of the script (used to derive fallback log file
#         name).
#
# @return None
#
# @example
# init_log "debug"  # Ensures log file is created and available for writing
#                   # with debug output.
# -----------------------------------------------------------------------------
init_log() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local scriptname="${THIS_SCRIPT%%.*}"  # Extract script name without extension
    local homepath log_dir fallback_log

    # Get the home directory of the current user
    homepath=$(
        getent passwd "${SUDO_USER:-$(whoami)}" | {
            IFS=':' read -r _ _ _ _ _ homedir _
            printf "%s" "$homedir"
        }
    )

    # Determine the log file location
    LOG_FILE="${LOG_FILE:-$homepath/$scriptname.log}"

    # Extract the log directory from the log file path
    log_dir="${LOG_FILE%/*}"

    # Check if the log directory exists and is writable
    debug_print "Checking if log directory '$log_dir' exists and is writable." "$debug"

    if [[ -d "$log_dir" && -w "$log_dir" ]]; then
        # Attempt to create the log file
        if ! touch "$LOG_FILE" &>/dev/null; then
            warn "Cannot create log file: $LOG_FILE"
            log_dir="/tmp"
        else
            # Change ownership of the log file if possible
            if [[ -n "${SUDO_USER:-}" && "${REQUIRE_SUDO:-true}" == "true" ]]; then
                chown "$SUDO_USER:$SUDO_USER" "$LOG_FILE" &>/dev/null || warn "Failed to set ownership to SUDO_USER: $SUDO_USER"
            else
                chown "$(whoami):$(whoami)" "$LOG_FILE" &>/dev/null || warn "Failed to set ownership to current user: $(whoami)"
            fi
        fi
    else
        log_dir="/tmp"
    fi

    # Fallback to /tmp if the directory is invalid
    if [[ "$log_dir" == "/tmp" ]]; then
        fallback_log="/tmp/$scriptname.log"
        LOG_FILE="$fallback_log"
        debug_print "Falling back to log file in /tmp: $LOG_FILE" "$debug"
        warn "Falling back to log file in /tmp: $LOG_FILE"
    fi

    # Attempt to create the log file in the fallback location
    if ! touch "$LOG_FILE" &>/dev/null; then
            debug_end "$debug"
        die 1 "Unable to create log file even in fallback location: $LOG_FILE"
    fi

    # Final debug message after successful log file setup
    debug_print "Log file successfully created at: $LOG_FILE" "$debug"

    readonly LOG_FILE
    export LOG_FILE

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Retrieve the terminal color code or attribute.
#
# @details This function uses `tput` to retrieve a terminal color code or
#          attribute (e.g., `sgr0` for reset, `bold` for bold text). If the
#          attribute is unsupported by the terminal, it returns an empty
#          string.
#
# @param $1 The terminal color code or attribute to retrieve.
#
# @return The corresponding terminal value or an empty string if unsupported.
# -----------------------------------------------------------------------------
default_color() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    tput "$@" 2>/dev/null || printf "\n"  # Fallback to an empty string on error

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Initialize terminal colors and text formatting.
# @details This function sets up variables for foreground colors, background
#          colors, and text formatting styles. It checks terminal capabilities
#          and provides fallback values for unsupported or non-interactive
#          environments.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @return None
#
# @example
# init_colors "debug"  # Initializes terminal colors with debug output.
# -----------------------------------------------------------------------------
init_colors() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # General text attributes
    BOLD=$(default_color bold)
    DIM=$(default_color dim)
    SMSO=$(default_color smso)
    RMSO=$(default_color rmso)
    UNDERLINE=$(default_color smul)
    NO_UNDERLINE=$(default_color rmul)
    BLINK=$(default_color blink)
    NO_BLINK=$(default_color sgr0)
    ITALIC=$(default_color sitm)
    NO_ITALIC=$(default_color ritm)
    MOVE_UP=$(default_color cuu1)
    CLEAR_LINE=$(tput el)

    # Foreground colors
    FGBLK=$(default_color setaf 0)
    FGRED=$(default_color setaf 1)
    FGGRN=$(default_color setaf 2)
    FGYLW=$(default_color setaf 3)
    FGGLD=$(default_color setaf 220)
    FGBLU=$(default_color setaf 4)
    FGMAG=$(default_color setaf 5)
    FGCYN=$(default_color setaf 6)
    FGWHT=$(default_color setaf 7)
    FGRST=$(default_color setaf 9)
    FGRST=$(default_color setaf 39)

    # Background colors
    BGBLK=$(default_color setab 0)
    BGRED=$(default_color setab 1)
    BGGRN=$(default_color setab 2)
    BGYLW=$(default_color setab 3)
    BGGLD=$(default_color setab 220)
    [[ -z "$BGGLD" ]] && BGGLD="$BGYLW"  # Fallback to yellow
    BGBLU=$(default_color setab 4)
    BGMAG=$(default_color setab 5)
    BGCYN=$(default_color setab 6)
    BGWHT=$(default_color setab 7)
    BGRST=$(default_color setab 9)

    # Reset all
    RESET=$(default_color sgr0)

    # Set variables as readonly
    # shellcheck disable=SC2034
    readonly RESET BOLD SMSO RMSO UNDERLINE NO_UNDERLINE DIM
    # shellcheck disable=SC2034
    readonly BLINK NO_BLINK ITALIC NO_ITALIC MOVE_UP CLEAR_LINE
    # shellcheck disable=SC2034
    readonly FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST FGGLD
    # shellcheck disable=SC2034
    readonly BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Generate a separator string for terminal output.
# @details Creates heavy or light horizontal rules based on terminal width.
#          Optionally outputs debug information if the debug flag is set.
#
# @param $1 Type of rule: "heavy" or "light".
# @param $2 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @return The generated rule string or error message if an invalid type is
#         provided.
#
# @example
# generate_separator "heavy"
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
generate_separator() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Normalize separator type to lowercase
    local type="${1,,}"
    local width="${COLUMNS:-80}"

    # Validate separator type
    if [[ "$type" != "heavy" && "$type" != "light" ]]; then
        warn "Invalid separator type: '$1'. Must be 'heavy' or 'light'."
            debug_end "$debug"
        return 1
    fi

    # Generate the separator based on type
    case "$type" in
        heavy)
            # Generate a heavy separator (═)
            printf '═%.0s' $(seq 1 "$width")
            ;;
        light)
            # Generate a light separator (─)
            printf '─%.0s' $(seq 1 "$width")
            ;;
        *)
            # Handle invalid separator type
            warn "Invalid separator type: $type"
                    debug_end "$debug"
            return 1
            ;;
    esac

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Validate the logging configuration, including LOG_LEVEL.
# @details This function checks whether the current LOG_LEVEL is valid. If
#          LOG_LEVEL is not defined in the `LOG_PROPERTIES` associative array,
#          it defaults to "INFO" and displays a warning message.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @global LOG_LEVEL The current logging verbosity level.
# @global LOG_PROPERTIES Associative array defining log level properties.
#
# @return void
#
# @example
# validate_log_level "debug"  # Enables debug output
# validate_log_level          # No debug output
# -----------------------------------------------------------------------------
validate_log_level() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Ensure LOG_LEVEL is a valid key in LOG_PROPERTIES
    if [[ -z "${LOG_PROPERTIES[$LOG_LEVEL]:-}" ]]; then
        # Print error message if LOG_LEVEL is invalid
        warn "Invalid LOG_LEVEL '$LOG_LEVEL'. Defaulting to 'INFO'."
        LOG_LEVEL="INFO"  # Default to "INFO"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Sets up the logging environment for the script.
#
# @details
# This function initializes terminal colors, configures the logging
# environment, defines log properties, and validates both the log level and
# properties. It must be called before any logging-related functions.
#
# - Initializes terminal colors using `init_colors`.
# - Sets up the log file and directory using `init_log`.
# - Defines global log properties (`LOG_PROPERTIES`), including severity
#   levels, colors, and labels.
# - Validates the configured log level and ensures all required log properties
#   are defined.
#
# @note This function should be called once during script initialization.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @return void
# -----------------------------------------------------------------------------
setup_log() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Initialize terminal colors
    init_colors "$debug"

    # Initialize logging environment
    init_log "$debug"

    # Define log properties (severity, colors, and labels)
    declare -gA LOG_PROPERTIES=(
        ["DEBUG"]="DEBUG|${FGCYN}|0"
        ["INFO"]="INFO |${FGGRN}|1"
        ["WARNING"]="WARN |${FGGLD}|2"
        ["ERROR"]="ERROR|${FGMAG}|3"
        ["CRITICAL"]="CRIT |${FGRED}|4"
        ["EXTENDED"]="EXTD |${FGBLU}|0"
    )

    # Debug message for log properties initialization
    if [[ "$debug" == "debug" ]]; then
        printf "[DEBUG] Log properties initialized:\n" >&2

        # Iterate through LOG_PROPERTIES to print each level with its color
        for level in DEBUG INFO WARNING ERROR CRITICAL EXTENDED; do
            IFS="|" read -r custom_level color severity <<< "${LOG_PROPERTIES[$level]}"
            printf "[DEBUG] %s: %b%s%b\n" "$level" "$color" "$custom_level" "$RESET" >&2
        done
    fi

    # Validate the log level and log properties
    validate_log_level "$debug"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Toggle the USE_CONSOLE variable on or off.
# @details This function updates the global USE_CONSOLE variable to either
#          "true" (on) or "false" (off) based on the input argument. It also
#          prints debug messages when the debug flag is passed.
#
# @param $1 The desired state: "on" (to enable console logging) or "off" (to
#           disable console logging).
# @param $2 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @global USE_CONSOLE The flag to control console logging.
#
# @return 0 on success, 1 on invalid input.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
toggle_console_log() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Declare local variables
    local state="${1,,}"      # Convert input to lowercase for consistency

    # Validate $state
    if [[ "$state" != "on" && "$state" != "off" ]]; then
        warn "Invalid state: '$state'. Must be 'on' or 'off'."
            debug_end "$debug"
        return 1
    fi

    # Process the desired state
    case "$state" in
        on)
            USE_CONSOLE="true"
            debug_print "Console logging enabled. USE_CONSOLE='$USE_CONSOLE', CONSOLE_STATE='$CONSOLE_STATE'" "$debug"
            ;;
        off)
            USE_CONSOLE="false"
            debug_print "Console logging disabled. USE_CONSOLE='$USE_CONSOLE', CONSOLE_STATE='$CONSOLE_STATE'" "$debug"
            ;;
        *)
            warn "Invalid argument for toggle_console_log: $state"
                    debug_end "$debug"
            return 1
            ;;
    esac

    debug_end "$debug"
    return 0
}

############
### Get Project Parameters Functions
############

# -----------------------------------------------------------------------------
# @brief Retrieve the Git owner or organization name from the remote URL.
# @details Attempts to dynamically fetch the Git repository's organization
#          name from the current Git process. If not available, uses the global
#          `$REPO_ORG` if set. If neither is available, returns "unknown".
#          Provides debugging output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @global REPO_ORG If set, uses this as the repository organization.
#
# @return Prints the organization name if available, otherwise "unknown".
# @retval 0 Success: the organization name is printed.
# @retval 1 Failure: prints an error message to standard error if the
#           organization cannot be determined.
# -----------------------------------------------------------------------------
get_repo_org() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local repo_org
    local url

    # Attempt to retrieve organization dynamically from local Git environment
    url=$(git config --get remote.origin.url 2>/dev/null)
    if [[ -n "$url" ]]; then
        # Extract the owner or organization name from the Git URL
        repo_org=$(printf "%s" "$url" | sed -E 's#(git@|https://)([^:/]+)[:/]([^/]+)/.*#\3#')
        debug_print "Retrieved organization from local Git remote URL: $repo_org" "$debug"
    else
        warn "No remote origin URL retrieved."
    fi

    # If the organization is still empty, use $REPO_ORG (if set)
    if [[ -z "$repo_org" && -n "$REPO_ORG" ]]; then
        repo_org="$REPO_ORG"
        debug_print "Using global REPO_ORG: $repo_org" "$debug"
    fi

    # If organization is still empty, return "unknown"
    if [[ -z "$repo_org" ]]; then
        debug_print "Unable to determine organization. Returning 'unknown'." "$debug"
        repo_org="unknown"
    fi

    # Output the determined or fallback organization
    printf "%s\n" "$repo_org"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Retrieve the Git project name from the remote URL.
# @details Attempts to dynamically fetch the Git repository's name from the
#          current Git process. If not available, uses the global `$REPO_NAME`
#          if set. If neither is available, returns "unknown". Provides
#          debugging output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @global REPO_NAME If set, uses this as the repository name.
#
# @return Prints the repository name if available, otherwise "unknown".
# @retval 0 Success: the repository name is printed.
# @retval 1 Failure: prints an error message to standard error if the
#         repository name cannot be determined.
# -----------------------------------------------------------------------------
get_repo_name() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local repo_name="${REPO_NAME:-}"  # Use existing $REPO_NAME if set
    local url

    # Attempt to retrieve repository name dynamically from Git
    if [[ -z "$repo_name" ]]; then
        url=$(git config --get remote.origin.url 2>/dev/null)
        if [[ -n "$url" ]]; then
            # Extract the repository name and remove the ".git" suffix if present
            repo_name="${url##*/}"        # Remove everything up to the last `/`
            repo_name="${repo_name%.git}" # Remove the `.git` suffix
            debug_print "Retrieved repository name from remote URL: $repo_name" "$debug"
        fi
    fi

    # Use "unknown" if no repository name could be determined
    if [[ -z "$repo_name" ]]; then
        debug_print "Unable to determine repository name. Returning 'unknown'." "$debug"
        repo_name="unknown"
    fi

    # Output the determined or fallback repository name
    printf "%s\n" "$repo_name"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Convert a Git repository name to title case.
# @details Replaces underscores and hyphens with spaces and converts words to
#          title case.  Provides debugging output when the "debug" argument is
#          passed.
#
# @param $1 The Git repository name (e.g., "my_repo-name").
# @param $2 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return The repository name in title case (e.g., "My Repo Name").
# @retval 0 Success: the converted repository name is printed.
# @retval 1 Failure: prints an error message to standard error.
#
# @throws Exits with an error if the repository name is empty.
# -----------------------------------------------------------------------------
repo_to_title_case() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local repo_name="${1:-}"  # Input repository name
    local title_case  # Variable to hold the formatted name

    # Validate input
    if [[ -z "${repo_name:-}" ]]; then
        warn "Repository name cannot be empty."
            debug_end "$debug"
        return 1
    fi
    debug_print "Received repository name: $repo_name" "$debug"

    # Replace underscores and hyphens with spaces and convert to title case
    title_case=$(printf "%s" "$repo_name" | tr '_-' ' ' | awk '{for (i=1; i<=NF; i++) $i=toupper(substr($i,1,1)) substr($i,2)}1')

    local retval
    if [[ -n "${title_case:-}" ]]; then
        debug_print "onverted repository name to title case: $title_case" "$debug"
        printf "%s\n" "$title_case"
        retval=0
    else
        warn "Failed to convert repository name to title case."
        retval=1
    fi

    debug_end "$debug"
    return "$retval"
}

# -----------------------------------------------------------------------------
# @brief Retrieve the current Repo branch name or the branch this was detached
#        from.
# @details Attempts to dynamically fetch the branch name from the current Git
#          process. If not available, uses the global `$REPO_BRANCH` if set. If
#          neither is available, returns "unknown". Provides debugging output
#          when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @global REPO_BRANCH If set, uses this as the current Git branch name.
#
# @return Prints the branch name if available, otherwise "unknown".
# @retval 0 Success: the branch name is printed.
# @retval 1 Failure: prints an error message to standard error if the branch
#         name cannot be determined.
# -----------------------------------------------------------------------------
get_repo_branch() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local branch="${REPO_BRANCH:-}"  # Use existing $REPO_BRANCH if set
    local detached_from

    # Attempt to retrieve branch name dynamically from Git
    if [[ -z "$branch" ]]; then
        branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
        if [[ -n "$branch" && "$branch" != "HEAD" ]]; then
            debug_print "Retrieved branch name from Git: $branch" "$debug"
        elif [[ "$branch" == "HEAD" ]]; then
            # Handle detached HEAD state: attempt to determine the source
            detached_from=$(git reflog show --pretty='%gs' | grep -oE 'checkout: moving from [^ ]+' | head -n 1 | awk '{print $NF}')
            if [[ -n "$detached_from" ]]; then
                branch="$detached_from"
                debug_print "Detached HEAD state. Detached from branch: $branch" "$debug"
            else
                debug_print "Detached HEAD state. Cannot determine the source branch." "$debug"
                branch="unknown"
            fi
        fi
    fi

    # Use "unknown" if no branch name could be determined
    if [[ -z "$branch" ]]; then
        debug_print "Unable to determine Git branch. Returning 'unknown'." "$debug"
        branch="unknown"
    fi

    # Output the determined or fallback branch name
    printf "%s\n" "$branch"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Get the most recent Git tag.
# @details Attempts to dynamically fetch the most recent Git tag from the
#          current Git process. If not available, uses the global `$GIT_TAG` if
#          set. If neither is available, returns "0.0.1". Provides debugging
#          output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @global GIT_TAG If set, uses this as the most recent Git tag.
#
# @return Prints the tag name if available, otherwise "0.0.1".
# @retval 0 Success: the tag name is printed.
# -----------------------------------------------------------------------------
get_last_tag() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local tag

    # Attempt to retrieve the tag dynamically from Git
    tag=$(git describe --tags --abbrev=0 2>/dev/null)
    if [[ -n "$tag" ]]; then
        debug_print "Retrieved tag from Git: $tag" "$debug"
    else
        debug_print "No tag obtained from local repo." "$debug"
        # Try using GIT_TAG if it is set
        tag="${GIT_TAG:-}"
        # Fall back to "0.0.1" if both the local tag and GIT_TAG are unset
        if [[ -z "$tag" ]]; then
            tag="0.0.1"
            debug_print "No local tag and GIT_TAG is unset. Using fallback: $tag" "$debug"
        else
            debug_print "Using pre-assigned GIT_TAG: $tag" "$debug"
        fi
    fi

    # Output the tag
    printf "%s\n" "$tag"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check if a tag follows semantic versioning.
# @details Validates if a given Git tag follows the semantic versioning format
#          (major.minor.patch). Provides debugging output when the "debug"
#          argument is passed.
#
# @param $1 The Git tag to validate.
# @param $2 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return Prints "true" if the tag follows semantic versioning, otherwise
#         "false".
# @retval 0 Success: the validation result is printed.
# @retval 1 Failure: prints an error message to standard error if no tag is
#         provided.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
is_sem_ver() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local tag="${1:-}"

    # Validate input
    if [[ -z "${tag:-}" ]]; then
        warn "Tag cannot be empty."
            debug_end "$debug"
        return 1
    fi

    debug_print "Validating tag: $tag" "$debug"

    # Check if the tag follows the semantic versioning format
    if [[ "$tag" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        debug_print "Tag $tag follows semantic versioning." "$debug"
        printf "true\n"
    else
        debug_print "Tag $tag does not follow semantic versioning." "$debug"
        printf "false\n"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Get the number of commits since the last tag.
# @details Counts commits since the provided Git tag using `git rev-list`. If
#          no tag is found, defaults to `0` commits. Debug messages are sent
#          only to `stderr`.
#
# @param $1 The Git tag to count commits from.
# @param $2 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return The number of commits since the tag, or 0 if the tag does not exist.
# -----------------------------------------------------------------------------
get_num_commits() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local tag="${1:-}"

    if [[ -z "$tag" || "$tag" == "0.0.1" ]]; then
        debug_print "No valid tag provided. Assuming 0 commits." "$debug"
        printf "0\n"
            debug_end "$debug"
        return 1
    fi

    local commit_count
    commit_count=$(git rev-list --count "${tag}..HEAD" 2>/dev/null || echo 0)

    printf "%s\n" "$commit_count"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Get the short hash of the current Git commit.
# @details Retrieves the short hash of the current Git commit. Provides
#          debugging output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return Prints the short hash of the current Git commit.
# @retval 0 Success: the short hash is printed.
# @retval 1 Failure: prints an error message to standard error if unable to
#         retrieve the hash.
# -----------------------------------------------------------------------------
get_short_hash() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local short_hash
    short_hash=$(git rev-parse --short HEAD 2>/dev/null)
    if [[ -z "$short_hash" ]]; then
        debug_print "No short hash available. Using 'unknown'." "$debug"
        short_hash="unknown"
    else
        debug_print "Short hash of the current commit: $short_hash." "$debug"
    fi

    printf "%s\n" "$short_hash"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check if there are uncommitted changes in the working directory.
# @details Checks for uncommitted changes in the current Git repository.
#          Provides debugging output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return Prints "true" if there are uncommitted changes, otherwise "false".
# @retval 0 Success: the dirty state is printed.
# @retval 1 Failure: prints an error message to standard error if unable to
#         determine the repository state.
# -----------------------------------------------------------------------------
get_dirty() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local changes

    # Check for uncommitted changes in the repository
    changes=$(git status --porcelain 2>/dev/null)

    if [[ -n "${changes:-}" ]]; then
        printf "true\n"
    else

        printf "false\n"
    fi

    if [[ -n "$changes" ]]; then
        debug_print "Changes detected." "$debug"
    else
        debug_print "No changes detected." "$debug"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Generate a version string based on the state of the Git repository.
# @details Constructs a semantic version string using the most recent Git tag,
#          current branch name, number of commits since the tag, the short hash
#          of the latest commit, and whether the working directory is dirty.
#          Provides debugging output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return Prints the generated semantic version string.
# @retval 0 Success: the semantic version string is printed.
# @retval 1 Failure: prints an error message to standard error if any required
#         Git information cannot be determined.
# -----------------------------------------------------------------------------
get_sem_ver() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local tag branch_name num_commits short_hash dirty version_string

    # Retrieve the most recent tag
    tag=$(get_last_tag "$debug")
    debug_print "Received tag: $tag from get_last_tag()." "$debug"
    if [[ -z "$tag" || "$tag" == "0.0.1" ]]; then
        debug_print "No semantic version tag found (or version is 0.0.1). Using default: 0.0.1" "$debug"
        version_string="0.0.1"
    else
        version_string="$tag"
    fi

    # Append branch name
    branch_name=$(get_repo_branch "$debug")
    version_string="$version_string-$branch_name"
    debug_print "Appended branch name to version: $branch_name" "$debug"

    # Append number of commits since the last tag
    num_commits=$(get_num_commits "$tag" "$debug")
    if [[ "$num_commits" -gt 0 ]]; then
        version_string="$version_string+$num_commits"
        debug_print "Appended commit count '$num_commits' to version." "$debug"
    fi

    # Append short hash of the current commit
    short_hash=$(get_short_hash "$debug")
    version_string="$version_string.$short_hash"
    debug_print "Appended short hash '$short_hash' to version." "$debug"

    # Check if the repository is dirty
    dirty=$(get_dirty "$debug")
    if [[ "$dirty" == "true" ]]; then
        version_string="$version_string-dirty"
        debug_print "Repository is dirty. Appended '-dirty' to version." "$debug"
    fi

    printf "%s\n" "$version_string"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Configure local or remote mode based on the Git repository context.
# @details Sets relevant variables for local mode if `USE_LOCAL` is `true` and
#          the script is being executed from within a GitHub repository
#          (`IS_REPO` is `true`). Defaults to remote configuration if not in
#          local mode or when the combined check fails.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @global USE_LOCAL           Indicates whether local mode is enabled.
# @global IS_REPO             Indicates whether the script resides in a GitHub
#                              repository.
# @global THIS_SCRIPT         Name of the current script.
# @global REPO_ORG            Git organization or owner name.
# @global REPO_NAME           Git repository name.
# @global REPO_BRANCH         Current Git branch name.
# @global GIT_TAG             Generated semantic version string.
# @global LOCAL_REPO_DIR      Path to the root of the local repository.
# @global LOCAL_WWW_DIR       Path to the `data` directory in the repository.
# @global LOCAL_SCRIPTS_DIR   Path to the `scripts` dir in the repository.
# @global GIT_RAW             URL for accessing raw files remotely.
# @global GIT_API             URL for accessing the repository API.
#
# @throws Exits with a critical error if the combined check fails in local mode.
#
# @return None
# -----------------------------------------------------------------------------
get_proj_params() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    if [[ "$USE_LOCAL" == "true" && "$IS_REPO" == "true" ]]; then
        debug_print "Configuring local mode with GitHub repository context." "$debug"

        # Making sure THIS_SCRIPT is right
        THIS_SCRIPT=$(basename "$0")
        debug_print "THIS_SCRIPT set to: $THIS_SCRIPT" "$debug"

        # Retrieve repository details
        REPO_ORG=$(get_repo_org "${debug}")
        local retval=$?
        if [[ $retval -ne 0 ]]; then
                    debug_end "$debug"
            die 1 "Failed to retrieve repository organization."
        fi
        debug_print "REPO_ORG set to: $REPO_ORG" "$debug"

        REPO_NAME=$(get_repo_name "${debug}")
        local retval=$?
        if [[ $retval -ne 0 ]]; then
                    debug_end "$debug"
            die 1 "Failed to retrieve repository name."
        fi
        debug_print "REPO_NAME set to: $REPO_NAME" "$debug"

        REPO_BRANCH=$(get_repo_branch "${debug}")
        local retval=$?
        if [[ $retval -ne 0 ]]; then
                    debug_end "$debug"
            die 1 "Failed to retrieve respository branch."
        fi
        debug_print "REPO_BRANCH set to: $REPO_BRANCH" "$debug"

        GIT_TAG=$(get_last_tag "${debug}")
        local retval=$?
        if [[ $retval -ne 0 ]]; then
                    debug_end "$debug"
            die 1 "Failed to retrieve last tag."
        fi
        debug_print "GIT_TAG set to: $GIT_TAG" "$debug"

        SEM_VER=$(get_sem_ver "${debug}")
        local retval=$?
        if [[ $retval -ne 0 ]]; then
                    debug_end "$debug"
            die 1 "Failed to retrieve semantic version."
        fi
        debug_print "SEM_VER set to: $SEM_VER" "$debug"

        # Get the root directory of the repository
        LOCAL_REPO_DIR=$(git rev-parse --show-toplevel 2>/dev/null)
        if [[ -z "${LOCAL_REPO_DIR:-}" ]]; then
            debug_end "$debug"
            die 1 "Not inside a valid Git repository. Ensure the repository is properly initialized."
        fi
        debug_print "LOCAL_REPO_DIR set to: $LOCAL_REPO_DIR" "$debug"

        # Set local script path based on repository structure
        LOCAL_WWW_DIR="$LOCAL_REPO_DIR/data"
        if [[ -d "${LOCAL_WWW_DIR:-}" ]]; then
            debug_end "$debug"
            die 1 "HTML source directory does not exist."
        fi
        debug_print "LOCAL_WWW_DIR set to: $LOCAL_WWW_DIR" "$debug"

        # Set local script path based on repository structure
        LOCAL_SCRIPTS_DIR="$LOCAL_REPO_DIR/scripts"
        if [[ -d "${LOCAL_WWW_DIR:-}" ]]; then
                    debug_end "$debug"
            die 1 "Scripts source directory does not exist."
        fi
        debug_print "LOCAL_SCRIPTS_DIR set to: $LOCAL_SCRIPTS_DIR" "$debug"
    else
        # Configure remote access URLs
        debug_print "Configuring remote mode." "$debug"
        if [[ -z "${REPO_ORG:-}" || -z "${REPO_NAME:-}" ]]; then
                    debug_end "$debug"
            die 1 "Remote mode requires REPO_ORG and REPO_NAME to be set."
        fi

        # Get GitHub URLs
        GIT_RAW="https://raw.githubusercontent.com/$REPO_ORG/$REPO_NAME"
        GIT_API="https://api.github.com/repos/$REPO_ORG/$REPO_NAME"
        GIT_CLONE="https://github.com/$REPO_ORG/$REPO_NAME.git"
        debug_print "GIT_RAW set to: $GIT_RAW" "$debug"
        debug_print "GIT_API set to: $GIT_API" "$debug"
        debug_print "GIT_CLONE set to: $GIT_CLONE" "$debug"
    fi

    # Export global variables for further use
    export THIS_SCRIPT REPO_ORG REPO_NAME REPO_BRANCH GIT_TAG LOCAL_REPO_DIR
    export LOCAL_WWW_DIR LOCAL_SCRIPTS_DIR GIT_RAW GIT_API

    debug_end "$debug"
    return 0
}


############
### Git Functions
############

# -----------------------------------------------------------------------------
# @brief Downloads a single file from a Git repository's raw URL.
# @details Fetches a file from the raw content URL of the repository and saves
#          it to the specified local directory. Ensures the destination
#          directory exists before downloading.
#
# @param $1 The relative path of the file in the repository.
# @param $2 The local destination directory where the file will be saved.
#
# @global GIT_RAW The base URL for raw content access in the Git repository.
# @global REPO_BRANCH The branch name from which the file will be fetched.
#
# @throws Logs an error and returns non-zero if the file download fails.
#
# @return None. Downloads the file to the specified directory.
#
# @example
# download_file "path/to/file.txt" "/local/dir"
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
download_file() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local file_path="$1"
    local dest_dir="$2"

    mkdir -p "$dest_dir"

    local file_name
    file_name=$(basename "$file_path")
    file_name="${file_name//\'/}"

    logI "Downloading from: $GIT_RAW/$REPO_BRANCH/$file_path to $dest_dir/$file_name"

    wget -q -O "$dest_dir/$file_name" "$GIT_RAW/$REPO_BRANCH/$file_path" || {
        warn "Failed to download file: $file_path to $dest_dir/$file_name"
        return 1
    }

    local dest_file="$dest_dir/$file_name"
    mv "$dest_file" "${dest_file//\'/}"
    debug_end "$debug"
    return
}

# -----------------------------------------------------------------------------
# @brief Clones a GitHub repository to the specified local destination.
# @details This function clones the repository from the provided Git URL to the
#          specified local destination directory.
#
# @global GIT_CLONE The base URL for cloning the GitHub repository.
# @global USER_HOME The home directory of the user, used as the base for
#         storing files.
# @global REPO_NAME The name of the repository to clone.
#
# @throws Logs an error and returns non-zero if the repository cloning fails.
#
# @return None. Clones the repository into the local destination.
#
# @example
# git_clone
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
git_clone() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local dest_root="$USER_HOME/$REPO_NAME"
    mkdir -p "$dest_root"

    logI "Cloning repository from $GIT_CLONE to $dest_root"
    git clone "$GIT_CLONE" "$dest_root" || {
        warn "Failed to clone repository: $GIT_CLONE to $dest_root"
        return 1
    }

    logI "Repository cloned successfully to $dest_root"
    debug_end "$debug"
    return
}

# -----------------------------------------------------------------------------
# @brief Fetches the Git tree of a specified branch from a repository.
# @details Retrieves the SHA of the specified branch and then fetches the
#          complete tree structure of the repository, allowing recursive access
#          to all files and directories.
#
# @global GIT_API The base URL for the GitHub API, pointing to the repository.
# @global REPO_BRANCH The branch name to fetch the tree from.
#
# @throws Prints an error message and exits if the branch SHA cannot be
#         fetched.
#
# @return Outputs the JSON representation of the repository tree.
#
# @example
# fetch_tree
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
fetch_tree() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local branch_sha
    branch_sha=$(curl -s "$GIT_API/git/ref/heads/$REPO_BRANCH" | jq -r '.object.sha')

    if [[ -z "$branch_sha" || "$branch_sha" == "null" ]]; then
        warn "Failed to fetch branch SHA for branch: $REPO_BRANCH. Check repository details or API access."
        return 1
    fi

    curl -s "$GIT_API/git/trees/$branch_sha?recursive=1"
    debug_end "$debug"
    return
}

# -----------------------------------------------------------------------------
# @brief Downloads files from specified directories in a repository.
# @details This function retrieves a repository tree, identifies files within
#          specified directories, and downloads them to the local system.
#
# @param $1 The target directory to update.
#
# @global USER_HOME The home directory of the user, used as the base for
#         storing files.
# @global GIT_DIRS Array of directories in the repository to process.
#
# @throws Exits the script with an error if the repository tree cannot be
#         fetched.
#
# @return Downloads files to the specified directory structure under
#         $USER_HOME/apppop.
#
# @example
# download_files_in_directories
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
download_files_in_directories() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local dest_root="$USER_HOME/$REPO_NAME"
    logI "Fetching repository tree."
    local tree; tree=$( "$debug")

    if [[ $(printf "%s" "$tree" | jq '.tree | length') -eq 0 ]]; then
        die 1 "Failed to fetch repository tree. Check repository details or ensure it is public."
    fi

    for dir in "${GIT_DIRS[@]}"; do
        logI "Processing directory: $dir"

        local files
        files=$(printf "%s" "$tree" | jq -r --arg TARGET_DIR "$dir/" \
            '.tree[] | select(.type=="blob" and (.path | startswith($TARGET_DIR))) | .path')

        if [[ -z "$files" ]]; then
            logI "No files found in directory: $dir"
            continue
        fi

        local dest_dir="$dest_root/$dir"
        mkdir -p "$dest_dir"

        printf "%s\n" "$files" | while read -r file; do
            logI "Downloading: $file"
            download_file "$file" "$dest_dir"
        done

        logI "Files from $dir downloaded to: $dest_dir"
    done

    debug_end "$debug"
    logI "Files saved in: $dest_root"
}

############
### Common Script Functions
############

# -----------------------------------------------------------------------------
# @brief Start the script, with optional timeout for non-interactive
#        environments.
# @details Allows users to press a key to proceed, or defaults after 10
#          seconds. If the debug flag is provided, additional information
#          about the process will be printed.
#
# @param $1 [Optional] Debug flag to enable detailed output (true/false).
#
# @global TERSE Indicates terse mode (skips interactive messages).
# @global REPO_NAME The name of the repository used.
#
# @return None
#
# @example
# start_script debug
# -----------------------------------------------------------------------------
start_script() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Check terse mode
    if [[ "${TERSE:-false}" == "true" ]]; then
        logI "$(repo_to_title_case "${REPO_NAME:-Unknown}") execution beginning."
        debug_print "Skipping interactive message due to terse mode." "$debug"
        debug_end "$debug"
        return 0
    fi

    # Prompt user for input
    printf "\n"
    printf "Starting run for: %s.\n" "$(repo_to_title_case "${REPO_NAME:-Unknown}")"
    printf "Press any key to continue or 'Q' to quit (defaulting in 10 seconds).\n"

    # Read a single key with a 10-second timeout
    if ! read -n 1 -sr -t 10 key < /dev/tty; then
        key=""  # Assign a default value on timeout
    fi
    printf "\n"

    # Handle user input
    case "${key}" in
        [Qq])  # Quit
            debug_print "Quit key pressed. Ending run." "$debug"
            logI "Run canceled by user."
            exit_script "Script canceled" "$debug"
            ;;
        "")  # Timeout or Enter
            debug_print "No key pressed, proceeding with run." "$debug"
            ;;
        *)  # Any other key
            debug_print "Key pressed: '$key'. Proceeding with run." "$debug"
            ;;
    esac

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Sets the system timezone interactively or logs if already set.
# @details If the current timezone is not GMT or BST, logs the current date and
#          time, and exits. Otherwise, prompts the user to confirm or
#          reconfigure the timezone. If the debug flag is passed, additional
#          information about the process is logged.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable detailed output.
#
# @global TERSE Indicates terse mode (skips interactive messages).
#
# @return None Logs the current timezone or adjusts it if necessary.
#
# @example
# set_time debug
# -----------------------------------------------------------------------------
set_time() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Declare local variables
    local need_set=false
    local current_date tz yn

    # Get the current date and time
    current_date="$(date)"
    tz="$(date +%Z)"

    # Log and return if the timezone is not GMT or BST
    if [[ "$tz" != "GMT" && "$tz" != "BST" && "$tz" != "UTC" ]]; then
        need_set=true
        debug_print "Timezone '$tz' is neither GMT, BST, nor UTC" "$debug"
        debug_end "$debug"
        return 0
fi

    # Check if the script is in terse mode
    if [[ "$TERSE" == "true" && "$need_set" == "true" ]]; then
        logW "Timezone detected as $tz, which may need to be updated."
            debug_end "$debug"
        return 1
    else
        logI "Timezone detected as $tz."
    fi

    # Inform the user about the current date and time
    logI "Timezone detected as $tz, which may need to be updated."

    # Prompt for confirmation or reconfiguration
    while true; do
        read -rp "Is this correct? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]*)
                logI "Timezone confirmed on $current_date"
                debug_print "Timezone confirmed: $current_date" "$debug"
                break
                ;;
            [Nn]* | *)
                dpkg-reconfigure tzdata
                logI "Timezone reconfigured on $current_date"
                debug_print "Timezone reconfigured: $current_date" "$debug"
                break
                ;;
        esac
    done

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Execute a new shell operation (departs this script).
# @details Executes or simulates a shell command based on the DRY_RUN flag.
#          Supports optional debugging to trace the execution process.
#
# @param $1 Name of the operation or process (for reference in logs).
# @param $2 The shell command to execute.
# @param $3 Optional debug flag ("debug" to enable debug output).
#
# @global FUNCNAME Used to fetch the current and caller function names.
# @global BASH_LINENO Used to fetch the calling line number.
# @global DRY_RUN When set, simulates command execution instead of running it.
#
# @throws Exits with a non-zero status if the command execution fails.
#
# @return None.
#
# @example
# DRY_RUN=true exec_new_shell "ListFiles" "ls -l" "debug"
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
exec_new_shell() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local exec_name="${1:-Unnamed Operation}"
    local exec_process="${2:-true}"

    # Debug information
    debug_print "exec_name: $exec_name" "$debug"
    debug_print " exec_process: $exec_process" "$debug"

    # Simulate command execution if DRY_RUN is enabled
    if [[ -n "$DRY_RUN" ]]; then
        printf "[✔] Simulating: '%s'.\n" "$exec_process"
            debug_end "$debug"
        exit_script 0 "$debug"
    fi

    # Validate the command
    if [[ "$exec_process" == "true" || "$exec_process" == "" ]]; then
        printf "[✔] Running: '%s'.\n" "$exec_process"
            debug_end "$debug"
        exec true
    elif ! command -v "${exec_process%% *}" >/dev/null 2>&1; then
        warn "'$exec_process' is not a valid command or executable."
            debug_end "$debug"
        die 1 "Invalid command: '$exec_process'"
    else
        # Execute the actual command
        printf "[✔] Running: '%s'.\n" "$exec_process"
        debug_print "Executing command: '$exec_process' in function '$func_name()' at line ${LINENO}." "$debug"
        exec $exec_process || die 1 "Command '${exec_process}' failed"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Executes a command in a separate Bash process.
# @details This function manages the execution of a shell command, handling the
#          display of status messages. It supports dry-run mode, where the
#          command is simulated without execution. The function prints success
#          or failure messages and handles the removal of the "Running" line
#          once the command finishes.
#
# @param exec_name The name of the command or task being executed.
# @param exec_process The command string to be executed.
# @param debug Optional flag to enable debug messages. Set to "debug" to
#              enable.
#
# @return Returns 0 if the command was successful, non-zero otherwise.
#
# @note The function supports dry-run mode, controlled by the DRY_RUN variable.
#       When DRY_RUN is true, the command is only simulated without actual
#       execution.
#
# @example
# exec_command "Test Command" "echo Hello World" "debug"
# -----------------------------------------------------------------------------
exec_command() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local exec_name="$1"
    local exec_process="$2"

    # Debug information
    debug_print "exec_name: $exec_name" "$debug"
    debug_print "exec_process: $exec_process" "$debug"

    # Basic status prefixes
    local running_pre="Running"
    local complete_pre="Complete"
    local failed_pre="Failed"

    # If DRY_RUN is enabled, show that in the prefix
    if [[ "$DRY_RUN" == "true" ]]; then
        running_pre+=" (dry)"
        complete_pre+=" (dry)"
        failed_pre+=" (dry)"
    fi
    running_pre+=":"
    complete_pre+=":"
    failed_pre+=":"

    # 1) Print ephemeral “Running” line
    printf "%b[-]%b %s %s\n" "${FGGLD}" "${RESET}" "$running_pre" "$exec_name"
    # Optionally ensure it shows up (especially if the command is super fast):
    sleep 0.02

    # 2) If DRY_RUN == "true", skip real exec
    if [[ "$DRY_RUN" == "true" ]]; then
        # Move up & clear ephemeral line
        printf "%b%b" "$MOVE_UP" "$CLEAR_LINE"
        printf "%b[✔]%b %s %s.\n" "${FGGRN}" "${RESET}" "$complete_pre" "$exec_name"
        debug_end "$debug"
        return 0
    fi

    # 3) Check if exec_process is a function or a command
    local status=0
    if declare -F "$exec_process" &>/dev/null; then
        # It's a function, pass remaining arguments to the function
        "$exec_process" "$@" "$debug" || status=$?
    else
        # It's a command, pass remaining arguments to the command
        bash -c "$exec_process" &>/dev/null || status=$?
    fi

    # 4) Move up & clear ephemeral “Running” line
    printf "%b%b" "$MOVE_UP" "$CLEAR_LINE"

    # 5) Print final success/fail
    if [[ $status -eq 0 ]]; then
        printf "%b[✔]%b %s %s.\n" "${FGGRN}" "${RESET}" "$complete_pre" "$exec_name"
    else
        printf "%b[✘]%b %s %s.\n" "${FGRED}" "${RESET}" "$failed_pre" "$exec_name"
        # If specifically “command not found” exit code:
        if [[ $status -eq 127 ]]; then
            warn "Command not found: $exec_process"
        else
            warn "Command failed with status $status: $exec_process"
        fi
    fi

    debug_end "$debug"
    return $status
}

# -----------------------------------------------------------------------------
# @brief Installs or upgrades all packages in the APT_PACKAGES list.
# @details Updates the package list and resolves broken dependencies before
#          proceeding. Accumulates errors for each failed package and logs a
#          summary at the end. Skips execution if the APT_PACKAGES array is
#          empty.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable detailed output.
#
# @global APT_PACKAGES List of packages to install or upgrade.
#
# @return 0 if all operations succeed, 1 if any operation fails.
#
# @example
# handle_apt_packages debug
# -----------------------------------------------------------------------------
handle_apt_packages() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Check if APT_PACKAGES is empty
    if [[ ${#APT_PACKAGES[@]} -eq 0 ]]; then
        logI "No packages specified in APT_PACKAGES. Skipping package handling."
        debug_print "APT_PACKAGES is empty, skipping execution." "$debug"
            debug_end "$debug"
        return 0
    fi

    local package error_count=0  # Counter for failed operations

    logI "Updating and managing required packages (this may take a few minutes)."

    # Update package list and fix broken installs
    if ! exec_command "Update local package index" "sudo apt-get update -y" "$debug"; then
        warn "Failed to update package list."
        ((error_count++))
    fi
    if ! exec_command "Fixing broken or incomplete package installations" "sudo apt-get install -f -y" "$debug"; then
        warn "Failed to fix broken installs."
        ((error_count++))
    fi

    # Install or upgrade each package in the list
    for package in "${APT_PACKAGES[@]}"; do
        if dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q "install ok installed"; then
            if ! exec_command "Upgrade $package" "sudo apt-get install --only-upgrade -y $package"; then
                warn "Failed to upgrade package: $package."
                ((error_count++))
            fi
        else
            if ! exec_command "Install $package" "sudo apt-get install -y $package"; then
                warn "Failed to install package: $package."
                ((error_count++))
            fi
        fi
    done

    # Log summary of errors
    if ((error_count > 0)); then
        warn "APT package handling completed with $error_count errors."
        debug_print "APT package handling completed with $error_count errors." "$debug"
    else
        logI "APT package handling completed successfully."
        debug_print "APT package handling completed successfully." "$debug"
    fi

    debug_end "$debug"
    return $error_count
}

# -----------------------------------------------------------------------------
# @brief End the script with optional feedback based on logging configuration.
# @details Provides a clear message to indicate the script completed
#          successfully. If the debug flag is passed, additional debug
#          information will be logged.
#
# @param $1 [Optional] Debug flag. Pass "debug" to enable debug output.
#
# @global REBOOT Indicates if a reboot is required.
# @global USE_CONSOLE Controls whether console output is enabled.
#
# @return None
#
# @example
# finish_script debug
# -----------------------------------------------------------------------------
finish_script() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    if [[ "$TERSE" == "true" || "$TERSE" != "true" ]]; then
        logI "Run complete: $(repo_to_title_case "$REPO_NAME")."
        debug_print "Run complete message logged." "$debug"
    fi

    # Clear screen (optional if required)
    if [[ "$TERSE" == "true" ]]; then
        # clear
        printf "Run complete: %s.\n" "$(repo_to_title_case "$REPO_NAME")"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Handles script exit operations and logs the exit message.
# @details This function is designed to handle script exit operations by logging
#          the exit message along with the status code, function name, and line
#          number where the exit occurred. The function also supports an optional
#          message and exit status, with default values provided if not supplied.
#          After logging the exit message, the script will terminate with the
#          specified exit status.
#
# @param $1 [optional] Exit status code (default is 1 if not provided).
# @param $2 [optional] Message to display upon exit (default is "Exiting
#           script.").
#
# @return None.
#
# @example
# exit_script 0 "Completed successfully"
# exit_script 1 "An error occurred"
# -----------------------------------------------------------------------------
exit_script() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Local variables
    local exit_status="${1:-}"              # First parameter as exit status
    local message="${2:-Exiting script.}"   # Main error message wit default
    local details                           # Additional details
    local lineno="${BASH_LINENO[0]}"        # Line number of calling line
    lineno=$(pad_with_spaces "$lineno")     # Pad line number with spaces
    local caller_func="${FUNCNAME[1]}"      # Calling function name

    # Determine exit status if not numeric
    if ! [[ "$exit_status" =~ ^[0-9]+$ ]]; then
        exit_status=1
    else
        shift  # Remove the exit_status from the arguments
    fi

    # Remove trailing dot if needed
    message=$(remove_dot "$message")
    # Log the provided or default message
    printf "[EXIT ] '%s' from %s:%d status (%d).\n" "$message" "$caller_func" "$lineno" "$exit_status"

    debug_end "$debug"
    exit "$exit_status"  # Exit with the provided status
}

############
### Menu Functions
############

# -----------------------------------------------------------------------------
# @var MENU_ITEMS
# @brief Stores menu item details.
# @details Keys are unique identifiers for menu items, and values are formatted
#          strings containing display names and the corresponding function to
#          call.
# -----------------------------------------------------------------------------
MENU_ITEMS["option_one"]="Option One"
MENU_ITEMS["option_two"]="Option Two"
MENU_ITEMS["option_three"]="Option Three"
MENU_ITEMS["display_sub_menu"]="Display Sub Menu"
MENU_ITEMS["display_main_menu"]="Display Main Menu"

# -----------------------------------------------------------------------------
# @var MAIN_MENU
# @brief Array defining the main menu options.
# @details Contains keys that correspond to the `MENU_ITEMS` associative array.
#          These keys define the options available in the main menu.
#
# @example
# MAIN_MENU=(
#     "option_one"
#     "option_two"
#     "display_sub_menu"
# )
# -----------------------------------------------------------------------------
# shellcheck disable=SC2034
MAIN_MENU=(
    "option_one"
    "option_two"
    "display_sub_menu"
)

# -----------------------------------------------------------------------------
# @var SUB_MENU
# @brief Array defining the sub-menu options.
# @details Contains keys that correspond to the `MENU_ITEMS` associative array.
#          These keys define the options available in the sub-menu.
#
# @example
# SUB_MENU=(
#     "option_three"
#     "display_main_menu"
# )
# -----------------------------------------------------------------------------
# shellcheck disable=SC2034
SUB_MENU=(
    "option_three"
    "display_main_menu"
)

# -----------------------------------------------------------------------------
# @brief Test function one
# @details Executes a sample action for the "Option One" menu item.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
option_one() {
    # Debug declarations
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Execute menu action
    printf "\n"
    printf "Running %s().\n" "${FUNCNAME[0]}"
    pause "$debug"

    # Debug log: function exit
    debug_end "$debug"
}

# -----------------------------------------------------------------------------
# @brief Test function two
# @details Executes a sample action for the "Option Two" menu item.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
option_two() {
    # Debug declarations
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Execute menu action
    printf "\n"
    printf "Running %s().\n" "${FUNCNAME[0]}"
    pause

    # Debug log: function exit
    debug_end "$debug"
}

# -----------------------------------------------------------------------------
# @brief Test function three
# @details Executes a sample action for the "Option Three" menu item.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
option_three() {
    # Debug declarations
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Execute menu action
    printf "\n"
    printf "Running %s().\n" "${FUNCNAME[0]}"
    pause

    # Debug log: function exit
    debug_end "$debug"
}

# -----------------------------------------------------------------------------
# @brief Displays a menu based on the given menu array.
# @details The menu items are numbered sequentially, and the user is prompted
#          for input to select an option.
#
# @param $1 Array of menu keys to display.
# @param $2 Debug flag for optional debug output.
#
# @global MENU_ITEMS Uses this global array to retrieve menu details.
# @global MENU_HEADER Prints the global menu header.
#
# @throws Prints an error message if an invalid choice is made.
#
# @return Executes the corresponding function for the selected menu item.
# -----------------------------------------------------------------------------
display_menu() {
    # Debug declarations
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local choice
    local i=1
    local menu_array=("${!1}")  # Array of menu keys to display

    # Display the menu header
    printf "%s\n\n" "$MENU_HEADER"
    printf "Please select an option:\n\n"

    # Display the menu items
    for func in "${menu_array[@]}"; do
        # Fixed-width format for consistent alignment
        printf "%-4d%-30s\n" "$i" "${MENU_ITEMS[$func]}"
        ((i++))
    done
    printf "%-4d%-30s\n" 0 "Exit"

    # Read user choice
    printf "\n"
    printf "Enter your choice: "
    read -n 1 -sr choice < /dev/tty || true
    printf "%s\n" "$choice"

    # Validate input
    if [[ -z "$choice" ]]; then
        printf "No input provided. Please enter a valid choice.\n"
        return
    elif [[ "$choice" =~ ^[0-9]$ ]]; then
        if [[ "$choice" -eq 0 ]]; then
            printf "\n"
            printf "Exiting.\n"
            debug_end "$debug"
            exit 0
        elif [[ "$choice" -ge 1 && "$choice" -lt "$i" ]]; then
            local func="${menu_array[choice-1]}"
            "$func" "$debug"
        else
            printf "Invalid choice. Please try again.\n"
        fi
    else
        printf "Invalid input. Please enter a number.\n"
    fi

    # Debug log: function exit
    debug_end "$debug"
}

# -----------------------------------------------------------------------------
# @brief Displays the main menu.
# @details Calls the `display_menu` function with the main menu array.
#
# @param $1 Debug flag for optional debug output.
#
# @return Calls `display_menu` with the main menu array.
# -----------------------------------------------------------------------------
display_main_menu() {
    # Debug declarations
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Clear screen
    clear
    # Display the menu
    display_menu MAIN_MENU[@] "$debug"

    # Debug log: function exit
    debug_end "$debug"
}

# -----------------------------------------------------------------------------
# @brief Displays the sub-menu.
# @details Calls the `display_menu` function with the sub-menu array. Loops
#          within the sub-menu until the user chooses to exit.
#
# @param $1 Debug flag for optional debug output.
#
# @return Calls `display_menu` with the sub-menu array in a loop.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
display_sub_menu() {
    # Debug declarations
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    while true; do
        # Clear screen
        clear
        # Display the menu
        display_menu SUB_MENU[@] "$debug"
    done

    # Debug log: function exit
    debug_end "$debug"
}

# -----------------------------------------------------------------------------
# @brief Entry point for the menu.
# @details Initializes debugging if the "debug" flag is passed, starts the
#          main menu loop, and ensures proper debug logging upon exit.
#
# @param $@ Arguments passed to the script. Pass "debug" for debug mode.
#
# @example
# Execute the menu
#   do_menu "$debug"
# -----------------------------------------------------------------------------
do_menu() {
    # Debug declarations
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Main script execution starts here
    while true; do
        display_main_menu "$debug"
    done

    # Debug log: function exit
    debug_end "$debug"
}

############
### Arguments Functions
############

# -----------------------------------------------------------------------------
# @brief List of word arguments.
# @details Each entry in the list corresponds to a word argument and contains
#          the argument name, the associated function, a brief description,
#          and a flag indicating whether the function should exit after
#          processing the argument.
#
# @var ARGUMENTS_LIST
# @brief List of word arguments.
# @details The list holds the word arguments, their corresponding functions,
#          descriptions, and exit flags. Each word argument triggers a
#          specific function when encountered on the command line.
# -----------------------------------------------------------------------------
ARGUMENTS_LIST=(
    "word1 word_arg_one Handles word argument one 0"
    "word2 word_arg_two Handles word argument two 1"
)

# -----------------------------------------------------------------------------
# @brief List of flagged arguments.
# @details Each entry in the list corresponds to a flagged argument, containing
#          the flag(s), a complex flag indicating if a secondary argument is
#          required, the associated function, a description, and an exit flag
#          indicating whether the function should terminate after processing.
#
# @var OPTIONS_LIST
# @brief List of flagged arguments.
# @details This list holds the flags (which may include multiple pipe-delimited
#          options), the associated function to call, whether a secondary
#          argument is required, and whether the function should exit after
#          processing.
# -----------------------------------------------------------------------------
OPTIONS_LIST=(
    "-1|--flag_1 0 flag_arg_one Handles flag_arg_one 0"
    "-2|--flag_2 0 flag_arg_two Handles flag_arg_two 1"
    "-3|--flag_3 1 flag_arg_tre Handles flag_arg_tre 0"
    "-4|--flag_4 1 flag_arg_fwr Handles flag_arg_fwr 1"
    "-h|--help 0 usage Show these instructions 1"
)

# -----------------------------------------------------------------------------
# @brief Handles the first word argument.
# @details This function processes a plain word argument, constructs a message
#          based on its value, and prints the message. If the argument is not
#          provided, a default message is used. This function is designed as a
#          demo for handling word arguments, showing how to pass and process
#          arguments.
#
# @param $1 [optional] Turns on debug printing
#
# @return 0 on success, or the status of the last executed command.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
word_arg_one() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local retval=0
    local argument
    argument="${1:-}"  # Use the first argument or default to an empty string

    local message
    if [[ -n "$argument" ]]; then
        message="Argument: ${argument}."
    else
        message="No arguments."
    fi

    debug_print "$message" "$debug"
    retval="$?"

    debug_end "$debug"
    return "$retval"
}

# -----------------------------------------------------------------------------
# @brief Handles the first word argument.
# @details This function processes a plain word argument, constructs a message
#          based on its value, and prints the message. If the argument is not
#          provided, a default message is used. This function is designed as a
#          demo for handling word arguments, showing how to pass and process
#          arguments.
#
# @param $1 [optional] Turns on debug printing if == "debug"
#
# @return 0 on success, or the status of the last executed command.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
word_arg_two() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local retval=0
    local argument
    argument="${1:-}"  # Use the first argument or default to an empty string

    local message
    if [[ -n "$argument" ]]; then
        message="Argument: ${argument}."
    else
        message="No arguments."
    fi

    debug_print "$message" "$debug"
    retval="$?"

    debug_end "$debug"
    return "$retval"
}

# -----------------------------------------------------------------------------
# @brief Handles the first flag argument.
# @details This function processes the a flagged argument, constructs a
#          message based on whether the argument is provided. If an argument
#          is supplied, it constructs a message with the argument value.
#          Otherwise, a default message is used. The message is then printed
#          with the debug_print function, and the status is returned.
#
# @param $1 [optional] The first flag argument passed to the function. If not
#           provided, the function defaults to an empty string.
# @param $2 [optional] Turns on debug printing if == "debug"
#
# @return 0 on success, or the status of the last executed command.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
flag_arg_one() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local retval=0
    local argument
    argument="${1:-}"  # Use the first argument or default to an empty string

    local message
    if [[ -n "$argument" ]]; then
        message="Argument: ${argument}."
    else
        message="No arguments."
    fi

    debug_print "$message" "$debug"
    retval="$?"

    debug_end "$debug"
    return "$retval"
}

# -----------------------------------------------------------------------------
# @brief Handles the first flag argument.
# @details This function processes the a flagged argument, constructs a
#          message based on whether the argument is provided. If an argument
#          is supplied, it constructs a message with the argument value.
#          Otherwise, a default message is used. The message is then printed
#          with the debug_print function, and the status is returned.
#
# @param $1 [optional] The first flag argument passed to the function. If not
#           provided, the function defaults to an empty string.
# @param $2 [optional] Turns on debug printing if == "debug"
#
# @return 0 on success, or the status of the last executed command.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
flag_arg_two() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local retval=0
    local argument
    argument="${1:-}"  # Use the first argument or default to an empty string

    local message
    if [[ -n "$argument" ]]; then
        message="Argument: ${argument}."
    else
        message="No arguments."
    fi

    debug_print "$message" "$debug"
    retval="$?"

    debug_end "$debug"
    return "$retval"
}

# -----------------------------------------------------------------------------
# @brief Handles the first flag argument.
# @details This function processes the a flagged argument, constructs a
#          message based on whether the argument is provided. If an argument
#          is supplied, it constructs a message with the argument value.
#          Otherwise, a default message is used. The message is then printed
#          with the debug_print function, and the status is returned.
#
# @param $1 [optional] The first flag argument passed to the function. If not
#           provided, the function defaults to an empty string.
# @param $2 [optional] Turns on debug printing if == "debug"
#
# @return 0 on success, or the status of the last executed command.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
flag_arg_tre() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local retval=0
    local argument
    argument="${1:-}"  # Use the first argument or default to an empty string

    local message
    if [[ -n "$argument" ]]; then
        message="Argument: ${argument}."
    else
        message="No arguments."
    fi

    debug_print "$message" "$debug"
    retval="$?"

    debug_end "$debug"
    return "$retval"
}

# -----------------------------------------------------------------------------
# @brief Handles the first flag argument.
# @details This function processes the a flagged argument, constructs a
#          message based on whether the argument is provided. If an argument
#          is supplied, it constructs a message with the argument value.
#          Otherwise, a default message is used. The message is then printed
#          with the debug_print function, and the status is returned.
#
# @param $1 [optional] The first flag argument passed to the function. If not
#           provided, the function defaults to an empty string.
# @param $2 [optional] Turns on debug printing if == "debug"
#
# @return 0 on success, or the status of the last executed command.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
flag_arg_fwr() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local retval=0
    local argument
    argument="${1:-}"  # Use the first argument or default to an empty string

    local message
    if [[ -n "$argument" ]]; then
        message="Argument: ${argument}."
    else
        message="No arguments."
    fi

    debug_print "$message" "$debug"
    retval="$?"

    debug_end "$debug"
    return "$retval"
}

# -----------------------------------------------------------------------------
# @brief Processes command-line arguments.
# @details This function processes both word arguments (defined in
#          `ARGUMENTS_LIST`) and flagged options (defined in `OPTIONS_LIST`).
#          It handles complex flags that require a following argument, and
#          calls the associated functions for each valid argument. If an
#          invalid argument is encountered, it will trigger the `usage()`
#          function to display help instructions.
#
# @param $@ [optional] Command-line arguments passed to the function.
# @global ARGUMENTS_LIST List of valid word arguments and their associated
#                        functions.
# @global OPTIONS_LIST List of valid flagged options and their associated
#                      functions.
# @global debug_flag Optional debug flag to enable debugging information.
#
# @return 0 on success, or the status code of the last executed command.
# -----------------------------------------------------------------------------
process_args() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local retval=0
    local args=("$@")
    local invalid_argument=false

    # Loop through all the arguments passed to the function.
    while (( ${#args[@]} > 0 )); do
        local current_arg="${args[0]}"
        local processed_argument=false

        # Skip empty arguments.
        if [[ -z "${current_arg}" ]]; then
            args=("${args[@]:1}")  # Remove the blank argument and continue
            continue
        fi

        # Process flagged options (starting with "-").
        if [[ "${current_arg:0:1}" == "-" ]]; then
            # Loop through all flagged options (OPTIONS_LIST)
            for entry in "${OPTIONS_LIST[@]}"; do
                local flag
                local complex_flag
                local function_name
                local description
                local exit_flag
                flag=$(echo "$entry" | cut -d' ' -f1)
                complex_flag=$(echo "$entry" | cut -d' ' -f2)
                function_name=$(echo "$entry" | cut -d' ' -f3)
                description=$(echo "$entry" | cut -d' ' -f4- | rev | cut -d' ' -f2- | rev)
                exit_flag=$(echo "$entry" | awk '{print $NF}')

                # Split flags and check if current_arg matches.
                IFS='|' read -ra flag_parts <<< "$flag"  # Split the flag by "|"
                for part in "${flag_parts[@]}"; do
                    part=$(echo "$part" | xargs)  # Trim spaces

                    # Check if the current argument matches any of the flags
                    if [[ "$current_arg" == "$part" ]]; then
                        # If it's a complex flag, we expect a following argument
                        if (( complex_flag == 1 )); then
                            local next_arg
                            if [[ ${#args[@]} -ge 2 ]]; then
                                next_arg="${args[1]}"
                            else
                                die 1 "Error: Missing argument for flag '$part'."
                            fi

                            # Call the function with the next argument as a parameter
                            $function_name "$next_arg" "$debug"
                            retval="$?"

                            # Remove the processed flag and its argument
                            args=("${args[@]:2}")
                            processed_argument=true
                        else
                            # Call the function with no arguments
                            $function_name
                            retval="$?"
                            # Remove the processed flag
                            args=("${args[@]:1}")
                            processed_argument=true
                        fi

                        # Exit if exit_flag is set
                        if (( exit_flag == 1 )); then
                            debug_end "$debug"
                            exit_script "$retval"
                        fi
                        continue
                    fi
                done
            done
        elif [[ -n "${current_arg}" ]]; then
            # Process single-word arguments from ARGUMENTS_LIST.
            for entry in "${ARGUMENTS_LIST[@]}"; do
                local word
                local function_name
                local description
                local exit_flag
                word=$(echo "$entry" | cut -d' ' -f1)
                function_name=$(echo "$entry" | cut -d' ' -f2)
                description=$(echo "$entry" | cut -d' ' -f3- | rev | cut -d' ' -f2- | rev)
                exit_flag=$(echo "$entry" | awk '{print $NF}')

                # Check if the current argument matches the word argument
                if [[ "$current_arg" == "$word" ]]; then
                    # Call the associated function
                    $function_name "$debug"
                    retval="$?"

                    # Exit if exit_flag is set
                    if (( exit_flag == 1 )); then
                        debug_end "$debug"
                        exit_script "$retval"
                    fi

                    # Remove the processed argument from args
                    args=("${args[@]:1}")
                    processed_argument=true
                    break
                fi
            done
        fi

        # Handle invalid arguments by setting the flag.
        if [[ "$processed_argument" != true ]]; then
            args=("${args[@]:1}")
            invalid_argument=true
            continue
        fi
    done

    # If any invalid argument is found, show usage instructions.
    if [[ "$invalid_argument" == true ]]; then
        usage stderr
    fi

    debug_end "$debug"
    return "$retval"
}

# -----------------------------------------------------------------------------
# @brief Prints usage information for the script.
# @details This function prints out the usage instructions for the script,
#          including the script name, command-line options, and their
#          descriptions. The usage of word arguments and flag arguments is
#          displayed separately. It also handles the optional inclusion of
#          the `sudo` command based on the `REQUIRE_SUDO` environment variable.
#          Additionally, it can direct output to either stdout or stderr based
#          on the second argument.
#
# @param $@ [optional] Command-line arguments passed to the function,
#                      typically used for the debug flag.
# @global REQUIRE_SUDO If set to "true", the script name will include "sudo"
#                      in the usage message.
# @global THIS_SCRIPT The script's name, used for logging.
#
# @return 0 on success.
# -----------------------------------------------------------------------------
usage() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
    local output_redirect="1"  # Default to stdout (1)
    local args=()

    # Check for the "stderr" argument to redirect output to stderr.
    for arg in "$@"; do
        if [[ "$arg" == "stderr" ]]; then
            output_redirect="2"  # Set to stderr (2)
            shift
            break  # Exit the loop as soon as we find "stderr"
        fi
    done

    # Check if "sudo" should be appended to the script name
    local script_name
    [[ "${REQUIRE_SUDO:-}" == "true" ]] && script_name+="sudo "
    script_name+=" ./$THIS_SCRIPT"

    # Print the usage with the correct script name
    printf "\n"
    printf "Usage: %s [debug] <option1> [<option2> ...]\n\n" "$script_name" >&$output_redirect

    # Word Arguments section
    printf "Available Options\n\n" >&$output_redirect
    printf "Word Arguments:\n" >&$output_redirect

    local max_word_len=0
    # First pass to calculate the maximum lengths of the word arguments
    for entry in "${ARGUMENTS_LIST[@]}"; do
        local word; word=$(echo "$entry" | cut -d' ' -f1)
        local word_len=${#word}
        if (( word_len > max_word_len )); then
            max_word_len=$word_len
        fi
    done

    # Second pass to print with padded formatting
    for entry in "${ARGUMENTS_LIST[@]}"; do
        local word; word=$(echo "$entry" | cut -d' ' -f1)
        local description; description=$(echo "$entry" | cut -d' ' -f3- | rev | cut -d' ' -f2- | rev)
        local exit_flag=$((1 - $(echo "$entry" | awk '{print $NF}')))  # Invert the value

        printf "  %$((max_word_len))s: %s\n" "$word" "$description" >&$output_redirect
    done
    printf "\n" >&$output_redirect

    # Flag Arguments section
    printf "Flag Arguments:\n" >&$output_redirect
    local max_flag_len=0
    for entry in "${OPTIONS_LIST[@]}"; do
        local flag; flag=$(echo "$entry" | cut -d' ' -f1)
        local flag_len=${#flag}
        if (( flag_len > max_flag_len )); then
            max_flag_len=$flag_len
        fi
    done

    # Second pass to print with padded formatting for flag arguments
    for entry in "${OPTIONS_LIST[@]}"; do
        local flag; flag=$(echo "$entry" | cut -d' ' -f1)
        local complex_flag; complex_flag=$(echo "$entry" | cut -d' ' -f2)
        local description; description=$(echo "$entry" | cut -d' ' -f4- | rev | cut -d' ' -f2- | rev)
        local exit_flag=$((1 - $(echo "$entry" | awk '{print $NF}')))  # Invert the value

        printf "  %$((max_flag_len))s: %s\n" "$(echo "$flag" | tr '|' ' ')" "$description" >&$output_redirect
    done

    debug_end "$debug"
    return 0
}

############
### App-specific Functions Here
############

############
### Main Functions
############

_main() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    # Check and set up the environment
    handle_execution_context "$debug"  # Get execution context and set environment variables
    get_proj_params "$debug"           # Get project and git parameters
    process_args "$@"                  # Parse command-line arguments
    enforce_sudo "$debug"              # Ensure proper privileges for script execution
    validate_depends "$debug"          # Ensure required dependencies are installed
    validate_sys_accs "$debug"         # Verify critical system files are accessible
    validate_env_vars "$debug"         # Check for required environment variables
    setup_log "$debug"                 # Setup logging environment
    check_bash "$debug"                # Ensure the script is executed in a Bash shell
    check_sh_ver "$debug"              # Verify the Bash version meets minimum requirements
    check_bitness "$debug"             # Validate system bitness compatibility
    check_release "$debug"             # Check Raspbian OS version compatibility
    check_arch "$debug"                # Validate Raspberry Pi model compatibility
    check_internet "$debug"            # Verify internet connectivity if required

    # Print/display the environment
    print_system "$debug"              # Log system information
    print_version "$debug"             # Log the script version

    # Run template steps
    start_script "$debug"              # Start the script with instructions
    set_time "$debug"                  # Offer to change timezone if default
    process_args "$@" "$debug"         # Process command line argments
    handle_apt_packages "$debug"       # Perform APT maintenance and install/update packages
    do_menu "$debug"                   # Execute menu
    finish_script "$debug"             # Finish the script with final instructions

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Main function entry point.
# @details This function calls `_main` to initiate the script execution. By
#          calling `main`, we enable the correct reporting of the calling
#          function in Bash, ensuring that the stack trace and function call
#          are handled appropriately during the script execution.
#
# @param "$@" Arguments to be passed to `_main`.
# @return Returns the status code from `_main`.
# -----------------------------------------------------------------------------
main() { _main "$@"; return "$?"; }

# -----------------------------------------------------------------------------
# @brief Traps the `EXIT` signal to invoke the `egress` function.
# @details Ensures the `egress` function is called automatically when the shell
#          exits. This enables proper session cleanup and displays session
#          statistics to the user.
# -----------------------------------------------------------------------------
trap egress EXIT

debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
main "$@" "$debug"
debug_end "$debug"
exit $?
