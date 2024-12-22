#!/usr/bin/env bash
set -uo pipefail # Setting -e is far too much work here
IFS=$'\n\t'

##
# @file
# @brief Comprehensive Bash script template with advanced functionality.
#
# @details
# This script provides a robust framework for managing installation processes
# with extensive logging, error handling, and system validation. It includes:
# - Detailed stack traces for debugging.
# - Dynamic logging configuration with support for various levels (DEBUG, INFO, etc.).
# - System checks for compatibility with OS versions, architectures, dependencies, and environment variables.
# - Internet connectivity validation with proxy support.
# - Git repository context retrieval and semantic versioning utilities.
#
# @author Lee Bussy
# @date December 21, 2024
# @version 1.0.0
#
# @copyright
# This script is open-source and can be modified or distributed under the terms
# of the MIT license.
#
# @par Usage:
# ```bash
# ./script_name.sh [OPTIONS]
# ```
# Run `./script_name.sh --help` for detailed options.
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
# Ensure this script is executed with appropriate permissions (e.g., sudo for installation tasks).
##

##
# @brief Trap unexpected errors during script execution.
# @details Captures any errors (via the ERR signal) that occur during script execution.
# Logs the function name and line number where the error occurred and exits the script.
# The trap calls an error-handling function for better flexibility.
#
# @global FUNCNAME Array containing function names in the call stack.
# @global LINENO Line number where the error occurred.
# @global THIS_SCRIPT Name of the script.
#
# @return None (exits the script with an error code).
##
#shellcheck disable=2329
trap_error() {
    local func="${FUNCNAME[1]:-main}"  # Get the calling function name (default: "main")
    local line="$1"                   # Line number where the error occurred
    local script="${THIS_SCRIPT:-$(basename "$0")}"  # Script name (fallback to current script)

    # Log the error message and exit
    printf "ERROR: An unexpected error occurred in function '%s' at line %s of script '%s'. Exiting." "$func" "$line" "$script" >&2
    exit 1
}

# Set the trap to call trap_error on any error
# Uncomment the next line to help with script debugging
# trap 'trap_error "$LINENO"' ERR

############
### Global Script Declarations
############

#/**
# * @var DRY_RUN
# * @brief Allows simulated execution of certain commands
# */
declare DRY_RUN="${DRY_RUN:-false}" # Use existing value, or default to "false".

#/**
# * @var THIS_SCRIPT
# * @brief The name of the script being executed.
# *
# * This variable is initialized to "install.sh" if not set. It can be dynamically
# * updated during execution based on the script's context.
# */
THIS_SCRIPT="${THIS_SCRIPT:-$(basename "$0")}"  # Default to the script's name if not set

#/**
# * @var IS_PATH
# * @brief Indicates whether the script was executed from a `PATH` location.
# *
# * Defaults to `false`. Dynamically set to `true` during execution if the script is
# * confirmed to have been executed from a directory listed in the `PATH` environment variable.
# */
declare IS_PATH="${IS_PATH:-false}"

#/**
# * @var IS_GITHUB_REPO
# * @brief Indicates whether the script resides in a GitHub repository or subdirectory.
# *
# * Defaults to `false`. Dynamically set to `true` during execution if the script is detected
# * to be within a GitHub repository (i.e., a `.git` folder exists in the directory hierarchy).
# */
declare IS_GITHUB_REPO="${IS_GITHUB_REPO:-false}"

##
# @brief Project metadata constants used throughout the script.
# @details These variables provide metadata about the script, including ownership,
# versioning, and project details.
##
declare REPO_ORG="${REPO_ORG:-lbussy}"
declare REPO_NAME="${REPO_NAME:-bash-installer}"
declare GIT_BRCH="${GIT_BRCH:-main}"
declare SEM_VER="${SEM_VER:-1.0.0}"

##
# @var USE_CONSOLE
# @brief Controls whether logging output is directed to the console.
#
# When set to `true`, log messages are displayed on the console in addition
# to being written to the log file (if enabled). When set to `false`, log
# messages are written only to the log file, making it suitable for
# non-interactive or automated environments.
#
# Example:
# - `USE_CONSOLE=true`: Logs to both console and file.
# - `USE_CONSOLE=false`: Logs only to file.
##
USE_CONSOLE="${USE_CONSOLE:-true}"

##
# @var TERSE
# @brief Enables or disables terse logging mode.
#
# In terse mode (`true`), log messages are minimal and optimized for
# automated environments. When set to `false`, log messages are verbose,
# providing detailed information for debugging or manual intervention.
#
# Example:
# - `TERSE=true`: Short, minimal log messages.
# - `TERSE=false`: Verbose log messages with detailed information.
##
TERSE="${TERSE:-false}"

##
# @var REQUIRE_SUDO
# @brief Indicates whether root privileges are required to run the script.
# @details Defaults to 'false' if not specified. Can be overridden by setting the
# `REQUIRE_SUDO` environment variable before running the script.
# @default false
##
readonly REQUIRE_SUDO="${REQUIRE_SUDO:-true}"  # Default to false if not specified.

##
# @var REQUIRE_INTERNET
# @type string
# @brief Flag indicating if internet connectivity is required.
# @details
# Controls whether the script should verify internet connectivity during initialization.
# This can be overridden by setting the `REQUIRE_INTERNET` environment variable.
#
# Possible values:
# - `"true"`: Internet connectivity is required.
# - `"false"`: Internet connectivity is not required.
#
# @default `"true"`
##
readonly REQUIRE_INTERNET="${REQUIRE_INTERNET:-true}"  # Default to true if not set

##
# @var MIN_BASH_VERSION
# @brief Specifies the minimum supported Bash version.
# @details Defaults to "4.0" if not specified. Can be overridden by setting the
# `MIN_BASH_VERSION` environment variable before running the script.
# Set to "none" to disable version checks.
# @default "4.0"
##
readonly MIN_BASH_VERSION="${MIN_BASH_VERSION:-4.0}"  # Default to "4.0" if not specified.

##
# @var MIN_OS
# @brief Specifies the minimum supported OS version.
# @details This variable defines the lowest OS version that the script can run on.
# It should be updated as compatibility requirements change.
# @default 11
##
readonly MIN_OS=11  # Minimum supported OS version.

##
# @var MAX_OS
# @brief Specifies the maximum supported OS version.
# @details Defines the highest OS version the script is expected to support.
# Use -1 to indicate no upper limit for supported OS versions.
# @default 15
##
readonly MAX_OS=15  # Maximum supported OS version (use -1 for no upper limit).

##
# @var SUPPORTED_BITNESS
# @brief Specifies the supported system bitness.
# @details Acceptable values are:
# - "32": Only supports 32-bit systems.
# - "64": Only supports 64-bit systems.
# - "both": Supports both 32-bit and 64-bit systems.
# Defaults to "32" if not otherwise specified.
# @default "32"
##
readonly SUPPORTED_BITNESS="32"  # Supported bitness ("32", "64", or "both").

##
# @var SUPPORTED_MODELS
# @brief Associative array of Raspberry Pi models and their support statuses.
# @details Each key is a string containing model identifiers (e.g., name, revision codes),
# and each value is the support status: "Supported" or "Not Supported".
# The array is marked as readonly to prevent modification at runtime.
##
declare -A SUPPORTED_MODELS=(
    # Unsupported Models
    ["Raspberry Pi 5|5-model-b|bcm2712"]="Not Supported"              # Raspberry Pi 5 Model B
    ["Raspberry Pi 400|400|bcm2711"]="Not Supported"                  # Raspberry Pi 400
    ["Raspberry Pi Compute Module 4|4-compute-module|bcm2711"]="Not Supported" # Compute Module 4
    ["Raspberry Pi Compute Module 3|3-compute-module|bcm2837"]="Not Supported" # Compute Module 3
    ["Raspberry Pi Compute Module|compute-module|bcm2835"]="Not Supported"     # Original Compute Module

    # Supported Models
    ["Raspberry Pi 4 Model B|4-model-b|bcm2711"]="Supported"          # Raspberry Pi 4 Model B
    ["Raspberry Pi 3 Model A+|3-model-a-plus|bcm2837"]="Supported"    # Raspberry Pi 3 Model A+
    ["Raspberry Pi 3 Model B+|3-model-b-plus|bcm2837"]="Supported"    # Raspberry Pi 3 Model B+
    ["Raspberry Pi 3 Model B|3-model-b|bcm2837"]="Supported"          # Raspberry Pi 3 Model B
    ["Raspberry Pi 2 Model B|2-model-b|bcm2836"]="Supported"          # Raspberry Pi 2 Model B
    ["Raspberry Pi Model A+|model-a-plus|bcm2835"]="Supported"        # Raspberry Pi Model A+
    ["Raspberry Pi Model B+|model-b-plus|bcm2835"]="Supported"        # Raspberry Pi Model B+
    ["Raspberry Pi Model B Rev 2|model-b-rev2|bcm2835"]="Supported"   # Raspberry Pi Model B Rev 2
    ["Raspberry Pi Model A|model-a|bcm2835"]="Supported"              # Raspberry Pi Model A
    ["Raspberry Pi Model B|model-b|bcm2835"]="Supported"              # Raspberry Pi Model B
    ["Raspberry Pi Zero 2 W|model-zero-2-w|bcm2837"]="Supported"      # Raspberry Pi Zero 2 W
    ["Raspberry Pi Zero|model-zero|bcm2835"]="Supported"              # Raspberry Pi Zero
    ["Raspberry Pi Zero W|model-zero-w|bcm2835"]="Supported"          # Raspberry Pi Zero W
)
readonly SUPPORTED_MODELS

##
# @var LOG_TO_FILE
# @brief Controls whether logging to a file is enabled.
# @details Possible values are:
# - "file":     Always log to the file.
# - "console":  Never log to the file.
# - "both":     For logging to both destinations
# - unset:      Both
# Defaults to blank if not set.
##
declare LOG_OUTPUT="${LOG_OUTPUT:-both}"  # Default to logging to both console and file

##
# @var LOG_FILE
# @brief Specifies the path to the log file.
# @details Defaults to blank if not provided. This can be set externally to
# Specify a custom log file path.
##
declare LOG_FILE="${LOG_FILE:-}"  # Use the provided LOG_FILE or default to blank.

##
# @var LOG_LEVEL
# @brief Specifies the logging verbosity level.
# @details Defaults to "DEBUG" if not set. Other possible levels can be defined
# depending on script requirements (e.g., INFO, WARN, ERROR).
# @default "DEBUG"
##
declare LOG_LEVEL="${LOG_LEVEL:-DEBUG}" # Default log level is DEBUG if not set.

##
# @var DEPENDENCIES
# @type array
# @brief List of required external commands for the script.
# @details
# This array contains the names of commands that the script relies on. Each command is checked
# for availability at runtime. The script may fail if these dependencies are not installed or accessible.
# - Ensure all required commands are included.
# - Use a dependency-checking function to verify their presence.
# @default A predefined set of common system utilities.
# @note
# Update this list as needed to reflect the commands actually used by the script.
##
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

##
# @var ENV_VARS_BASE
# @type array
# @brief Base list of required environment variables.
# @details
# These variables are always required by the script, regardless of the runtime context.
# - `HOME`: Specifies the home directory of the current user.
# - `COLUMNS`: Defines the width of the terminal, used for formatting.
##
declare -ar ENV_VARS_BASE=(
    "HOME"       # Home directory of the current user
    "COLUMNS"    # Terminal width for formatting
)

##
# @var ENV_VARS
# @type array
# @brief Final list of required environment variables.
# @details
# This array extends `ENV_VARS_BASE` by conditionally including `SUDO_USER` if the script
# requires root privileges (`REQUIRE_SUDO=true`).
# - `SUDO_USER`: Identifies the user who invoked the script using sudo.
# - Dynamically constructed during runtime.
##
if [[ "$REQUIRE_SUDO" == true ]]; then
    readonly -a ENV_VARS=("${ENV_VARS_BASE[@]}" "SUDO_USER")
else
    readonly -a ENV_VARS=("${ENV_VARS_BASE[@]}")
fi

##
# @var COLUMNS
# @brief Terminal width in columns.
# @details
# The `COLUMNS` variable represents the width of the terminal in characters.
# If not already set by the environment, it defaults to 80 columns.
# This can be overridden externally by setting the `COLUMNS` environment variable.
# @default 80
##
COLUMNS="${COLUMNS:-80}"  # Default to 80 columns if unset

##
# @var SYSTEM_READS
# @type array
# @brief List of critical system files to check.
# @details
# Contains absolute paths to system files that the script depends on.
# These files must be both present and readable to ensure proper execution of the script.
# - `/etc/os-release`: Contains operating system identification data.
# - `/proc/device-tree/compatible`: Identifies the hardware compatibility (commonly used in embedded systems).
##
declare -ar SYSTEM_READS=(
    "/etc/os-release"               # OS identification file
    "/proc/device-tree/compatible"  # Hardware compatibility file
)
readonly SYSTEM_READS

##
# @var APT_PACKAGES
# @type array
# @brief List of required APT packages.
# @details
# Specifies the names of APT packages that the script depends on. These packages
# should be available in the system's default package repository and will be
# checked or installed if missing.
#
# Packages included:
# - `apache2`: Web server.
# - `php`: PHP interpreter for scripting.
# - `jq`: JSON parsing utility.
# - `libraspberrypi-dev`: Development libraries for Raspberry Pi.
# - `raspberrypi-kernel-headers`: Kernel headers for Raspberry Pi.
##
readonly APT_PACKAGES=(
)

##
# @var WARN_STACK_TRACE
# @type string
# @brief Flag to enable stack trace logging for warnings.
# @details
# Controls whether stack traces are printed alongside warning messages.
# This is useful for debugging and tracking the script's execution path.
#
# Possible values:
# - `"true"`: Enable stack trace logging for warnings.
# - `"false"`: Disable stack trace logging for warnings (default).
#
# @default `"false"`
##
readonly WARN_STACK_TRACE="${WARN_STACK_TRACE:-false}"  # Default to false if not set

############
### Common Functions
############

##
# @brief Pads a number with spaces.
# @details Pads the input number with spaces to the left. Defaults to 4 characters wide but accepts an optional width.
#
# @param $1 The number to pad (e.g., "7").
# @param $2 (Optional) The width of the output (default is 4).
# @return The padded number with spaces as a string.
##
pad_with_spaces() {
    local number="$1"       # Input number
    local width="${2:-4}"   # Optional width (default is 4)

    # Validate input
    if [[ -z "${number:-}" || ! "$number" =~ ^[0-9]+$ ]]; then
        die 1 "Input must be a valid non-negative integer."
    fi

    if [[ ! "$width" =~ ^[0-9]+$ || "$width" -lt 1 ]]; then
        die 1 "Error: Width must be a positive integer."
    fi

    # Strip leading zeroes to prevent octal interpretation
    number=$((10#$number))  # Forces the number to be interpreted as base-10

    # Format the number with leading spaces and return it as a string
    printf "%${width}d\n" "$number"
}

##
# @brief Print a detailed stack trace of the call hierarchy.
# @details Outputs the sequence of function calls leading up to the point
#          where this function was invoked. Supports optional error messages
#          and colorized output based on terminal capabilities.
#
# @param $1 Log level (e.g., DEBUG, INFO, WARN, ERROR, CRITICAL).
# @param $2 Optional error message to display at the top of the stack trace.
#
# @global BASH_LINENO Array of line numbers in the call stack.
# @global FUNCNAME Array of function names in the call stack.
# @global BASH_SOURCE Array of source file names in the call stack.
#
# @return None
##
stack_trace() {
    local level="$1"
    local message="$2"
    local color=""                   # Default: no color
    local label=""                   # Log level label for display
    local header="------------------ STACK TRACE ------------------"
    local tput_colors_available      # Terminal color support
    local lineno="${BASH_LINENO[0]}" # Line number where the error occurred
    lineno=$(pad_with_spaces "$lineno") # Pad with zeroes

    # Check terminal color support
    tput_colors_available=$(tput colors 2>/dev/null || printf "0\n")

    # Disable colors if terminal supports less than 8 colors
    if [[ "$tput_colors_available" -lt 8 ]]; then
        color="\033[0m"  # No color
    fi

    # Validate level or default to DEBUG
    case "$level" in
        "DEBUG"|"INFO"|"WARN"|"ERROR"|"CRITICAL")
            ;;
        *)
            # If the first argument is not a valid level, treat it as a message
            message="$level"
            level="DEBUG"
            ;;
    esac

    # Determine color and label based on the log level
    case "$level" in
        "DEBUG")
            [[ "$tput_colors_available" -ge 8 ]] && color="\033[0;36m"  # Cyan
            label="Debug"
            ;;
        "INFO")
            [[ "$tput_colors_available" -ge 8 ]] && color="\033[0;32m"  # Green
            label="Info"
            ;;
        "WARN")
            [[ "$tput_colors_available" -ge 8 ]] && color="\033[0;33m"  # Yellow
            label="Warning"
            ;;
        "ERROR")
            [[ "$tput_colors_available" -ge 8 ]] && color="\033[0;31m"  # Red
            label="Error"
            ;;
        "CRITICAL")
            [[ "$tput_colors_available" -ge 8 ]] && color="\033[0;31m"  # Bright Red
            label="Critical"
            ;;
    esac

    # Print stack trace header
    printf "%b%s%b\n" "$color" "$header" "\033[0m" >&2
    if [[ -n "$message" ]]; then
        # If a message is provided
        printf "%b%s: %s%b\n" "$color" "$label" "$message" "\033[0m" >&2
    else
        # Default message with the line number of the caller
        local lineno="${BASH_LINENO[1]}"
        lineno=$(pad_with_spaces "$lineno") # Pad with zeroes
        printf "%b%s stack trace called by line: %s%b\n" "$color" "$label" "$lineno" "\033[0m" >&2
    fi

    # Print each function in the stack trace
    for ((i = 2; i < ${#FUNCNAME[@]}; i++)); do
        local script="${BASH_SOURCE[i]##*/}"
        local lineno="${BASH_LINENO[i - 1]}"
        lineno=$(pad_with_spaces "$lineno") # Pad with zeroes
        printf "%b[%d] Function: %s called from [%s:%s]%b\n" \
            "$color" $((i - 1)) "${FUNCNAME[i]}" "$script" "$lineno" "\033[0m" >&2
    done

    # Print stack trace footer (line of "-" matching $header)
    # shellcheck disable=SC2183
    printf "%b%s%b\n" "$color" "$(printf '%*s' "${#header}" | tr ' ' '-')" "\033[0m" >&2
}

##
# @brief Logs a warning or error message with optional details and a stack trace.
# @details This function logs messages at WARNING or ERROR levels, with an optional
#          stack trace for warnings. The error level can also be specified and
#          appended to the log message.
#
# @param $1 [Optional] The log level (WARNING or ERROR). Defaults to WARNING.
# @param $2 [Optional] The error level (numeric). Defaults to 0 if not provided.
# @param $3 [Optional] The main log message. Defaults to "An issue was raised on this line."
# @param $4 [Optional] Additional details to log.
#
# @global WARN_STACK_TRACE Enables stack trace logging for warnings when set to true.
# @global BASH_LINENO Array of line numbers in the call stack.
# @global SCRIPT_NAME The name of the script being executed.
#
# @return None
##
warn() {
    local error_level="${1:-0}"  # Default error level is 0
    local level="${2:-WARNING}"  # Default log level is WARNING
    local details="${4:-}"  # Default to no additional details
    local lineno="${BASH_LINENO[1]:-0}"  # Line number where the function was called
    lineno=$(pad_with_spaces "$lineno") # Pad with zeroes
    local message="${3:-A warning was raised on this line $lineno}"  # Default log message
    message="${message}: ($error_level)"

    if [[ "$level" == "WARNING" ]]; then
        logW "$message" "$details"
    elif [[ "$level" == "ERROR" ]]; then
        logE "$message" "$details"
    fi

    # Include stack trace if WARN_STACK_TRACE is true
    if [[ "$WARN_STACK_TRACE" == "true" && "$level" == "WARNING" ]]; then
        stack_trace "$level" "Stack trace for $level at line $lineno: $message"
    fi
}

##
# @brief Log a critical error, print a stack trace, and exit the script.
#
# @param $1 Exit status code (optional, defaults to 1 if not numeric).
# @param $2 Main error message (optional).
# @param $@ Additional details for the error (optional).
#
# @global BASH_LINENO Array of line numbers in the call stack.
# @global SCRIPT_NAME Script name.
#
# @return Exits the script with the provided or default exit status.
##
die() {
    # Local variables
    local exit_status="$1"              # First parameter as exit status
    local message                       # Main error message
    local details                       # Additional details
    local lineno="${BASH_LINENO[0]}"    # Line number where the error occurred
    local script="$THIS_SCRIPT"         # Script name
    local level="CRITICAL"              # Error level
    local tag="CRIT "                   # Extracts the first 4 characters (e.g., "CRIT")
    lineno=$(pad_with_spaces "$lineno") # Pad with zeroes

    # Determine exit status and message
    if ! [[ "$exit_status" =~ ^[0-9]+$ ]]; then
        exit_status=1
        message="$1"
        shift
    else
        shift
        message="$1"
        shift
    fi
    details="$*" # Remaining parameters as details

    # Log the error message only if a message is provided
    if [[ -n "$message" ]]; then
        printf "[%s]\t[%s:%s]\t%s\n" "$tag" "$script" "$lineno" "$message" >&2
        if [[ -n "$details" ]]; then
            printf "[%s]\t[%s:%s]\tDetails: %s\n" "$tag" "$script" "$lineno" "$details" >&2
        fi
    fi

    # Log the unrecoverable error
    printf "[%s]\t[%s:%s]\tUnrecoverable error (exit status: %d).\n" \
        "$tag" "$script" "$lineno" "$exit_status" >&2

    # Call stack_trace with processed message and error level
    if [[ -z "${message:-}" ]]; then
        stack_trace "$level" "Stack trace from line $lineno."
    else
        stack_trace "$level" "Stack trace from line $lineno: $message"
    fi

    # Exit with the determined status
    exit "$exit_status"
}

##
# @brief Add a dot at the beginning of a string if it's missing.
# @details Ensures the input string starts with a dot (`.`). If the input is empty, it returns an error.
#
# @param $1 The input string to process.
# @return The modified string with a leading dot, or an error if the input is invalid.
##
add_dot() {
    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "${input:-}" ]]; then
        warn "Input to add_dot cannot be empty."
        return 1
    fi

    # Add a leading dot if it's missing
    if [[ "${input:-}" != .* ]]; then
        input=".$input"
    fi

    printf "%s\n" "$input"
}

##
# @brief Remove a leading dot from a string if present.
# @details Ensures the input string does not start with a dot (`.`). If the input is empty, it returns an error.
#
# @param $1 The input string to process.
# @return The modified string without a leading dot, or an error if the input is invalid.
##
remove_dot() {
    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "${input:-}" ]]; then
        warn "ERROR" "Input to remove_dot cannot be empty."
        return 1
    fi

    # Remove the leading dot if present
    if [[ "$input" == .* ]]; then
        input="${input#.}"
    fi

    printf "%s\n" "$input"
}

##
# @brief Add a trailing slash to a string if it's missing.
# @details Ensures the input string ends with a slash (`/`). If the input is empty, it returns an error.
#
# @param $1 The input string to process.
# @return The modified string with a trailing slash, or an error if the input is invalid.
##
add_slash() {
    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "${input:-}" ]]; then
        warn "ERROR" "Input to add_slash cannot be empty."
        return 1
    fi

    # Add a trailing slash if it's missing
    if [[ "$input" != */ ]]; then
        input="$input/"
    fi

    printf "%s\n" "$input"
}

##
# @brief Remove a trailing slash from a string if present.
# @details Ensures the input string does not end with a slash (`/`). If the input is empty, it returns an error.
#
# @param $1 The input string to process.
# @return The modified string without a trailing slash, or an error if the input is invalid.
##
remove_slash() {
    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "${input:-}" ]]; then
        warn "ERROR" "Input to remove_slash cannot be empty."
        return 1
    fi

    # Remove the trailing slash if present
    if [[ "$input" == */ ]]; then
        input="${input%/}"
    fi

    printf "%s\n" "$input"
}

############
### Print/Display Environment Functions
############

##
# @brief Print the system information to the log.
# @details Extracts and logs the system's name and version using information
#          from `/etc/os-release`.
#
# @global None
# @return None
##
# shellcheck disable=SC2120
print_system() {
    # Declare local variables at the start of the function
    local system_name

    # Extract system name and version from /etc/os-release
    system_name=$(grep '^PRETTY_NAME=' /etc/os-release 2>/dev/null | cut -d '=' -f2 | tr -d '"')

    # Check if system_name is empty
    if [[ -z "${system_name:-}" ]]; then
        logW "System: Unknown (could not extract system information)." # Warning if system information is unavailable
    else
        logI "System: $system_name." # Log the system information
    fi
}

##
# @brief Print the script version and optionally log it.
# @details This function displays the version of the script stored in the global
#          variable `SEM_VER`. It uses `printf` if called by `parse_args`, otherwise
#          it uses `logI`.
#
# @global THIS_SCRIPT The name of the script.
# @global SEM_VER The version of the script.
#
# @return None
##
print_version() {
    # Declare local variables at the start of the function
    local caller

    # Check the name of the calling function
    caller="${FUNCNAME[1]}"

    if [[ "$caller" == "parse_args" ]]; then
        printf "%s: version %s" "$THIS_SCRIPT" "$SEM_VER" # Display the script name and version
    else
        logI "Running $(repo_to_title_case "$REPO_NAME")'s '$THIS_SCRIPT', version $SEM_VER" # Log the script name and version
    fi
}

############
### Check Environment Functions
############

#/**
# * @brief Determines the execution context of the script.
# *
# * This function identifies how the script was executed and returns a corresponding
# * context code. Errors are managed using `warn()` and `die()` where applicable.
# *
# * Context Return Codes:
# *   - 0: Script executed via pipe.
# *   - 1: Script executed with `bash` in an unusual way.
# *   - 2: Script executed directly (local or from PATH).
# *   - 3: Script executed from within a GitHub repository.
# *   - 4: Script executed from a PATH location.
# *
# * @return Context return codes (as described above).
# */
determine_execution_context() {
    local script_path   # Full path of the script
    local current_dir   # Temporary variable to traverse directories
    local max_depth=10  # Limit for directory traversal depth
    local depth=0       # Counter for directory traversal

    # Check if the script is executed via pipe
    if [[ "$0" == "bash" ]]; then
        if [[ -p /dev/stdin ]]; then
            return 0  # Execution via pipe
        else
            warn "Unusual bash execution detected."
            return 1  # Unusual bash execution
        fi
    fi

    # Get the script path
    script_path=$(realpath "$0" 2>/dev/null) || script_path=$(pwd)/$(basename "$0")
    if [[ ! -f "$script_path" ]]; then
        die 1 "Unable to resolve script path: $script_path"
    fi

    # Initialize current_dir with the directory part of script_path
    current_dir="${script_path%/*}"
    current_dir="${current_dir:-.}"

    # Safeguard against invalid current_dir during initialization
    if [[ ! -d "$current_dir" ]]; then
        die 1 "Invalid starting directory: $current_dir"
    fi

    # Traverse upwards to detect a GitHub repository
    while [[ "$current_dir" != "/" && $depth -lt $max_depth ]]; do
        if [[ -d "$current_dir/.git" ]]; then
            return 3  # Execution within a GitHub repository
        fi
        current_dir=$(dirname "$current_dir") # Move up one directory
        ((depth++))
    done

    # Handle loop termination conditions
    if [[ $depth -ge $max_depth ]]; then
        die 1 "Directory traversal exceeded maximum depth ($max_depth)."
    fi

    # Check if the script is executed from a PATH location
    local resolved_path
    resolved_path=$(command -v "$(basename "$0")" 2>/dev/null)
    if [[ "$resolved_path" == "$script_path" ]]; then
        return 4  # Execution from a PATH location
    fi

    # Default: Direct execution from the local filesystem
    return 2
}

#/**
# * @brief Handles the script execution context and performs relevant actions.
# *
# * This function determines the execution context by calling `determine_execution_context()`,
# * sets and exports global variables, and optionally outputs debug information about the script's
# * execution state. Errors are managed using `warn()` and `die()`.
# *
# * @param $1 Optional "debug" argument to enable debug output.
# * @global USE_LOCAL Indicates whether local mode is enabled.
# * @global IS_GITHUB_REPO Indicates whether the script resides in a GitHub repository.
# * @global THIS_SCRIPT Name of the script being executed.
# */
#shellcheck disable=2120
handle_execution_context() {
    local debug_enabled="false"
    [[ "${1:-}" == "debug" ]] && debug_enabled="true"

    # Call determine_execution_context and capture its output

    determine_execution_context
    local context=$?  # Capture the return code to determine context

    # Validate the context
    if ! [[ "$context" =~ ^[0-4]$ ]]; then
        die 1 "Invalid context code returned: $context"
    fi

    # Initialize and set global variables based on the context
    case $context in
        0)
            THIS_SCRIPT="piped_script"
            USE_LOCAL=false
            IS_GITHUB_REPO=false
            IS_PATH=false
            $debug_enabled && printf "Execution context: Script was piped (e.g., 'curl url | sudo bash').\n"
            ;;
        1)
            THIS_SCRIPT="piped_script"
            USE_LOCAL=false
            IS_GITHUB_REPO=false
            IS_PATH=false
            warn "Execution context: Script run with 'bash' in an unusual way."
            ;;
        2)
            THIS_SCRIPT=$(basename "$0")
            USE_LOCAL=true
            IS_GITHUB_REPO=false
            IS_PATH=false
            $debug_enabled && printf "Execution context: Script executed directly from %s.\n" "$THIS_SCRIPT"
            ;;
        3)
            THIS_SCRIPT=$(basename "$0")
            USE_LOCAL=true
            IS_GITHUB_REPO=true
            IS_PATH=false
            $debug_enabled && printf "Execution context: Script is within a GitHub repository.\n"
            ;;
        4)
            THIS_SCRIPT=$(basename "$0")
            USE_LOCAL=true
            IS_GITHUB_REPO=false
            IS_PATH=true
            $debug_enabled && printf "Execution context: Script executed from a PATH location (%s).\n" "$(command -v "$THIS_SCRIPT")"
            ;;
        *)
            die 99 "Unknown execution context."
            ;;
    esac
}

##
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
# @return None
# @exit 1 if the script is not executed correctly.
##
enforce_sudo() {
    if [[ "$REQUIRE_SUDO" == true ]]; then
        if [[ "$EUID" -eq 0 && -n "$SUDO_USER" && "$SUDO_COMMAND" == *"$0"* ]]; then
            return 0  # Script is properly executed with `sudo`
        elif [[ "$EUID" -eq 0 && -n "$SUDO_USER" ]]; then
            die 1 "This script should not be run from a root shell." \
                  "Run it with 'sudo $THIS_SCRIPT' as a regular user."
        elif [[ "$EUID" -eq 0 ]]; then
            die 1 "This script should not be run as the root user." \
                  "Run it with 'sudo $THIS_SCRIPT' as a regular user."
        else
            die 1 "This script requires 'sudo' privileges." \
                  "Please re-run it using 'sudo $THIS_SCRIPT'."
        fi
    fi
}

##
# @brief Check for required dependencies and report any missing ones.
# @details Iterates through the dependencies listed in the global array `DEPENDENCIES`,
#          checking if each one is installed. Logs missing dependencies and exits
#          the script with an error code if any are missing.
#
# @global DEPENDENCIES Array of required dependencies.
#
# @return None
# @exit 1 if any dependencies are missing.
##
validate_depends() {
    local missing=0  # Counter for missing dependencies
    local dep        # Iterator for dependencies

    for dep in "${DEPENDENCIES[@]}"; do
        if ! command -v "$dep" &>/dev/null; then
            logE "Missing dependency: $dep"
            ((missing++))
        fi
    done

    if ((missing > 0)); then
        logE "Missing $missing dependencies. Install them and re-run the script."
        exit_script 1
    fi
}

##
# @brief Check the availability of critical system files.
# @details Verifies that each file listed in the `SYSTEM_READS` array exists and is readable.
#          Logs an error for any missing or unreadable files and exits the script if any issues are found.
#
# @global SYSTEM_READS Array of critical system file paths to check.
#
# @return None
# @exit 1 if any required files are missing or unreadable.
##
validate_sys_accs() {
    local missing=0  # Counter for missing or unreadable files
    local file       # Iterator for files

    for file in "${SYSTEM_READS[@]}"; do
        if [[ ! -r "$file" ]]; then
            printf "ERROR: Missing or unreadable file: %s\n" "$file" >&2
            ((missing++))
        fi
    done

    if ((missing > 0)); then
        die 1 "Missing or unreadable $missing critical system files. Ensure they are accessible and re-run the script."
    fi
}

##
# @brief Validate the existence of required environment variables.
# @details Checks if the environment variables specified in the `ENV_VARS` array
#          are set. Logs any missing variables and exits the script if any are missing.
#
# @global ENV_VARS Array of required environment variables.
#
# @return None
# @exit 1 if any environment variables are missing.
##
validate_env_vars() {
    local missing=0  # Counter for missing environment variables
    local var        # Iterator for environment variables

    for var in "${ENV_VARS[@]}"; do
        if [[ -z "${!var:-}" ]]; then
            printf "ERROR: Missing environment variable: %s\n" "$var" >&2
            ((missing++))
        fi
    done

    if ((missing > 0)); then
        die 1 "Missing $missing required environment variables." \
              "Ensure they are set and re-run the script."
    fi
}

##
# @brief Check if the script is running in a Bash shell.
# @details Ensures the script is executed with Bash, as it may use Bash-specific features.
#
# @param $1 [Optional] "debug" to enable verbose output for all checks.
#
# @global BASH_VERSION The version of the Bash shell being used.
#
# @return None
# @exit 1 if not running in Bash.
##
# shellcheck disable=SC2120
check_bash() {
    local debug_enabled="false"
    [[ "${1:-}" == "debug" ]] && debug_enabled="true"

    # Debug logging: Start of check
    $debug_enabled && logD "Starting Bash environment check."

    # Ensure the script is running in a Bash shell
    if [[ -z "${BASH_VERSION:-}" ]]; then
        logE "This script requires Bash. Please run it with Bash."
        $debug_enabled && logD "BASH_VERSION is empty or undefined."
        exit_script 1
    fi

    # Debug logging: Successful check
    $debug_enabled && logD "Bash environment is valid. Detected Bash version: $BASH_VERSION."
}

##
# @brief Check if the current Bash version meets the minimum required version.
# @details Compares the current Bash version against a required version specified
#          in the global variable `MIN_BASH_VERSION`. If `MIN_BASH_VERSION` is "none",
#          the check is skipped. Outputs debug information if enabled.
#
# @param $1 [Optional] "debug" to enable verbose output for this check.
#
# @global MIN_BASH_VERSION Minimum required Bash version (e.g., "4.0") or "none".
# @global BASH_VERSINFO Array containing the major and minor versions of the running Bash.
#
# @return None
# @exit 1 if the Bash version is insufficient.
##
# shellcheck disable=SC2120
check_sh_ver() {
    local debug_enabled="false"
    [[ "${1:-}" == "debug" ]] && debug_enabled="true"

    local required_version="${MIN_BASH_VERSION:-none}"

    $debug_enabled && logD "Minimum required Bash version is set to '$required_version'."

    if [[ "$required_version" == "none" ]]; then
        $debug_enabled && logD "Bash version check is disabled (MIN_BASH_VERSION='none')."
        return 0
    fi

    # Extract required major and minor version components
    local required_major="${required_version%%.*}"
    local required_minor="${required_version##*.}"

    # Log the current Bash version
    $debug_enabled && logD "Current Bash version is ${BASH_VERSINFO[0]}.${BASH_VERSINFO[1]}."

    # Compare the current Bash version against the required version
    if (( BASH_VERSINFO[0] < required_major ||
          (BASH_VERSINFO[0] == required_major &&
           BASH_VERSINFO[1] < required_minor) )); then
        $debug_enabled && logD "Current Bash version does not meet the requirement."
        die 1 "This script requires Bash version $required_version or newer."
    fi

    $debug_enabled && logD "Current Bash version meets the requirement."
}

##
# @brief Check system bitness compatibility.
# @details Validates whether the current system's bitness matches the supported configuration.
#          Outputs debug information if debug mode is enabled.
#
# @param $1 [Optional] "debug" to enable verbose output for the check.
#
# @global SUPPORTED_BITNESS Specifies the bitness supported by the script ("32", "64", or "both").
#
# @return None
# @exit 1 if the system bitness is unsupported.
##
# shellcheck disable=SC2120
check_bitness() {
    local debug_enabled="false"
    [[ "${1:-}" == "debug" ]] && debug_enabled="true"

    local bitness  # Stores the detected bitness of the system.

    # Detect the system bitness
    bitness=$(getconf LONG_BIT)
    $debug_enabled && logD "Detected system bitness: $bitness-bit."

    case "$SUPPORTED_BITNESS" in
        "32")
            $debug_enabled && logD "Script supports only 32-bit systems."
            if [[ "$bitness" -ne 32 ]]; then
                die 1 "Only 32-bit systems are supported. Detected $bitness-bit system."
            fi
            ;;
        "64")
            $debug_enabled && logD "Script supports only 64-bit systems."
            if [[ "$bitness" -ne 64 ]]; then
                die 1 "Only 64-bit systems are supported. Detected $bitness-bit system."
            fi
            ;;
        "both")
            $debug_enabled && logD "Script supports both 32-bit and 64-bit systems."
            ;;
        *)
            $debug_enabled && logD "Invalid SUPPORTED_BITNESS configuration: '$SUPPORTED_BITNESS'."
            die 1 "Configuration error: Invalid value for SUPPORTED_BITNESS ('$SUPPORTED_BITNESS')."
            ;;
    esac

    $debug_enabled && logD "System bitness check passed for $bitness-bit system."
}

##
# @brief Check Raspbian OS version compatibility.
# @details This function ensures that the Raspbian version is within the supported range
#          and logs an error if the compatibility check fails.
#
# @global MIN_OS Minimum supported OS version.
# @global MAX_OS Maximum supported OS version (-1 indicates no upper limit).
# @global log_message Function for logging messages.
# @global die Function to handle critical errors and terminate the script.
#
# @return None Exits the script with an error code if the OS version is incompatible.
##
# shellcheck disable=SC2120
check_release() {
    local ver  # Holds the extracted version ID from /etc/os-release.
    local debug_enabled="false"
    [[ "${1:-}" == "debug" ]] && debug_enabled="true"

    # Ensure the file exists and is readable.
    if [[ ! -f /etc/os-release || ! -r /etc/os-release ]]; then
        die 1 "Unable to read /etc/os-release. Ensure this script is run on a compatible system."
    fi

    # Extract the VERSION_ID from /etc/os-release.
    if [[ -f /etc/os-release ]]; then
        ver=$(grep "VERSION_ID" /etc/os-release | awk -F "=" '{print $2}' | tr -d '"')
    else
        logE "File /etc/os-release not found."
        ver="unknown"
    fi

    # Ensure the extracted version is not empty.
    if [[ -z "${ver:-}" ]]; then
        die 1 "VERSION_ID is missing or empty in /etc/os-release."
    fi

    # Check if the version is older than the minimum supported version.
    if [[ "$ver" -lt "$MIN_OS" ]]; then
        die 1 "Raspbian version $ver is older than the minimum supported version ($MIN_OS)."
    fi

    # Check if the version is newer than the maximum supported version, if applicable.
    if [[ "$MAX_OS" -ne -1 && "$ver" -gt "$MAX_OS" ]]; then
        die 1 "Raspbian version $ver is newer than the maximum supported version ($MAX_OS)."
    fi

    # Log success if the version is within the supported range.
    $debug_enabled && logD "Raspbian version $ver is supported."
}

##
# @brief Check if the detected Raspberry Pi model is supported.
# @details Reads the Raspberry Pi model from /proc/device-tree/compatible and checks
#          it against a predefined list of supported models. Logs an error if the
#          model is unsupported or cannot be detected. Optionally outputs debug information
#          about all models if `debug` is set to `true`.
#
# @param $1 [Optional] "debug" to enable verbose output for all supported/unsupported models.
#
# @global SUPPORTED_MODELS Associative array of supported and unsupported Raspberry Pi models.
# @global log_message Function for logging messages.
# @global die Function to handle critical errors and terminate the script.
#
# @return None Exits the script with an error code if the architecture is unsupported.
##
# shellcheck disable=SC2120
check_arch() {
    local detected_model is_supported key full_name model chip

    local debug_enabled="false"
    [[ "${1:-}" == "debug" ]] && debug_enabled="true"

    # Attempt to read and process the compatible string
    if ! detected_model=$(cat /proc/device-tree/compatible 2>/dev/null | tr '\0' '\n' | grep "raspberrypi" | sed 's/raspberrypi,//'); then
        die 1 "Failed to read or process /proc/device-tree/compatible. Ensure compatibility."
    fi

    # Check if the detected model is empty
    if [[ -z "${detected_model:-}" ]]; then
        die 1 "No Raspberry Pi model found in /proc/device-tree/compatible. This system may not be supported."
    fi

    # Initialize is_supported flag
    is_supported=false

    # Iterate through supported models to check compatibility
    for key in "${!SUPPORTED_MODELS[@]}"; do
        IFS='|' read -r full_name model chip <<< "$key"
        if [[ "$model" == "$detected_model" ]]; then
            if [[ "${SUPPORTED_MODELS[$key]}" == "Supported" ]]; then
                is_supported=true
                $debug_enabled && logD "Model: '$full_name' ($chip) is supported."
            else
                die 1 "Model: '$full_name' ($chip) is not supported."
            fi
            break
        fi
    done

    # Debug output of all models if requested
    if [[ "$debug_enabled" == "true" ]]; then
        for key in "${!SUPPORTED_MODELS[@]}"; do
            IFS='|' read -r full_name model chip <<< "$key"
            if [[ "${SUPPORTED_MODELS[$key]}" == "Supported" ]]; then
                $debug_enabled && logD "Model: '$full_name' ($chip) is supported."
            else
                $debug_enabled && logW "Model: '$full_name' ($chip) is not supported."
            fi
        done
    fi

    # Log an error if no supported model was found
    if [[ "$is_supported" == false ]]; then
        die 1 "Detected Raspberry Pi model '$detected_model' is not recognized or supported."
    fi
}

##
# @brief Validate proxy connectivity by testing a known URL.
# @details Uses `check_url` to verify connectivity through the provided proxy settings.
#
# @global http_proxy The HTTP proxy URL (if set).
# @global https_proxy The HTTPS proxy URL (if set).
#
# @param $1 Proxy URL to validate (optional; defaults to `http_proxy` or `https_proxy`).
# @return 0 if the proxy is functional, 1 otherwise.
##
# shellcheck disable=SC2120
validate_proxy() {
    local proxy_url="$1"

    # Default to global proxy settings if no proxy is provided
    [[ -z "${proxy_url:-}" ]] && proxy_url="${http_proxy:-$https_proxy}"

    # Validate that a proxy is set
    if [[ -z "${proxy_url:-}" ]]; then
        logW "No proxy URL configured for validation."
        return 1
    fi

    logI "Validating proxy: $proxy_url"

    # Test the proxy connectivity using check_url
    if check_url "$test_url" "curl" "--silent --head --max-time 10 --proxy $proxy_url"; then
        logI "Proxy $proxy_url is functional."
        return 0
    else
        logE "Proxy $proxy_url is unreachable or misconfigured."
        return 1
    fi
}

##
# @brief Check connectivity to a URL using a specified tool.
# @details Attempts to connect to a given URL with `curl` or `wget` based on the provided arguments.
#          Ensures that the tool's availability is checked and handles timeouts gracefully.
#
# @param $1 The URL to test.
# @param $2 The tool to use for the test (`curl` or `wget`).
# @param $3 Options to pass to the testing tool (e.g., `--silent --head` for `curl`).
# @return 0 if the URL is reachable, 1 otherwise.
##
check_url() {
    local url="$1"
    local tool="$2"
    local options="$3"

    # Validate inputs
    if [[ -z "${url:-}" ]]; then
        printf "ERROR: URL and tool parameters are required for check_url.\n" >&2
        return 1
    fi

    # Check tool availability
    if ! command -v "$tool" &>/dev/null; then
        printf "ERROR: Tool '%s' is not installed or unavailable.\n" "$tool" >&2
        return 1
    fi

    # Perform the connectivity check, allowing SSL and proxy errors
    #shellcheck disable=2086
    if $tool $options "$url" &>/dev/null; then
        return 0
    else
        return 1
    fi
}

##
# @brief Comprehensive internet and proxy connectivity check.
# @details Combines proxy validation and direct internet connectivity tests
#          using `check_url`. Validates proxy configuration first, then
#          tests connectivity with and without proxies. Outputs debug information if enabled.
#
# @param $1 [Optional] "debug" to enable verbose output for all checks.
#
# @global http_proxy Proxy URL for HTTP (if set).
# @global https_proxy Proxy URL for HTTPS (if set).
# @global no_proxy Proxy exclusions (if set).
#
# @return 0 if all tests pass, 1 if any test fails.
##
# shellcheck disable=SC2120
check_internet() {
    local debug_enabled="false"
    [[ "${1:-}" == "debug" ]] && debug_enabled="true"

    local primary_url="http://google.com"
    local secondary_url="http://1.1.1.1"
    local proxy_valid=false

    # Debug mode message
    $debug_enabled && logD "Starting internet connectivity checks."

    # Validate proxy settings
    if [[ -n "${http_proxy:-}" || -n "${https_proxy:-}" ]]; then
        $debug_enabled && logD "Proxy detected. Validating proxy configuration."
        if validate_proxy; then
            proxy_valid=true
            $debug_enabled && logD "Proxy validation succeeded."
        else
            logW "Proxy validation failed. Proceeding with direct connectivity checks."
        fi
    fi

    # Check connectivity using curl
    if command -v curl &>/dev/null; then
        $debug_enabled && logD "curl is available. Testing internet connectivity using curl."

        # Check with proxy
        if $proxy_valid && curl --silent --head --max-time 10 --proxy "${http_proxy:-${https_proxy:-}}" "$primary_url" &>/dev/null; then
            logI "Internet is available using curl with proxy."
            $debug_enabled && logD "curl successfully connected via proxy."
            return 0
        fi

        # Check without proxy
        if curl --silent --head --max-time 10 "$primary_url" &>/dev/null; then
            logI "Internet is available using curl without proxy."
            $debug_enabled && logD "curl successfully connected without proxy."
            return 0
        fi

        $debug_enabled && logD "curl failed to connect."
    else
        $debug_enabled && logD "curl is not available."
    fi

    # Check connectivity using wget
    if command -v wget &>/dev/null; then
        $debug_enabled && logD "wget is available. Testing internet connectivity using wget."

        # Check with proxy
        if $proxy_valid && wget --spider --quiet --timeout=10 --proxy="${http_proxy:-${https_proxy:-}}" "$primary_url" &>/dev/null; then
            logI "Internet is available using wget with proxy."
            $debug_enabled && logD "wget successfully connected via proxy."
            return 0
        fi

        # Check without proxy
        if wget --spider --quiet --timeout=10 "$secondary_url" &>/dev/null; then
            logI "Internet is available using wget without proxy."
            $debug_enabled && logD "wget successfully connected without proxy."
            return 0
        fi

        $debug_enabled && logD "wget failed to connect."
    else
        $debug_enabled && logD "wget is not available."
    fi

    # Final failure message
    logE "No internet connection detected after all checks."
    $debug_enabled && logD "All internet connectivity tests failed."
    return 1
}

############
### Logging Functions
############

# Adjust `print_log_entry` to handle combined logic:
print_log_entry() {
    local timestamp="$1"
    local level="$2"
    local color="$3"
    local lineno="$4"
    local message="$5"
    local details="$6"

    # Log to file if required
    if [[ "$LOG_OUTPUT" == "file" || "$LOG_OUTPUT" == "both" ]]; then
        printf "[%s] [%s] [%s:%d] %s\n" "$timestamp" "$level" "$THIS_SCRIPT" "$lineno" "$message" >> "$LOG_FILE"
        [[ -n "$details" ]] && printf "[%s] [%s] [%s:%d] Details: %s\n" "$timestamp" "$level" "$THIS_SCRIPT" "$lineno" "$details" >> "$LOG_FILE"
    fi

    # Log to console if required
    if [[ "$LOG_OUTPUT" == "console" || "$LOG_OUTPUT" == "both" ]]; then
        printf "%b[%s] [%s:%d] %s%b\n" "$color" "$level" "$THIS_SCRIPT" "$lineno" "$message" "$RESET"
        [[ -n "$details" ]] && printf "%bDetails: %s%b\n" "$color" "$details" "$RESET"
    fi
}

##
# @brief Generate a timestamp and line number for log entries.
#
# This function retrieves the current timestamp and the line number of the calling script.
#
# @return A pipe-separated string in the format: "timestamp|line_number".
##
prepare_log_context() {
    # Local variables for timestamp and line number
    local timestamp
    local lineno

    # Generate the current timestamp
    timestamp=$(date "+%Y-%m-%d %H:%M:%S")

    # Retrieve the line number of the caller
    lineno="${BASH_LINENO[2]}"
    lineno=$(pad_with_spaces "$lineno") # Pad with zeroes

    # Return the pipe-separated timestamp and line number
    printf "%s|%s\n" "$timestamp" "$lineno"
}

##
# @brief Log a message with the specified log level.
# @details Uses the global `USE_CONSOLE` to control console output.
#
# @param $1 Log level (e.g., DEBUG, INFO, ERROR).
# @param $2 Main log message.
# @param $3 [Optional] Extended details for the log entry.
#
# @global LOG_LEVEL The current logging verbosity level.
# @global LOG_PROPERTIES Associative array defining log level properties.
# @global LOG_FILE Path to the log file (if configured).
# @global USE_CONSOLE Boolean flag to enable or disable console output.
# @global LOG_OUTPUT Specifies where to log messages ("file", "console", "both").
#
# @return None
##
log_message() {
    local level="${1:-DEBUG}"  # Default to "DEBUG" if $1 is unset
    local message="${2:-<no message>}"  # Default to "<no message>" if $2 is unset
    local details="${3:-}"  # Default to an empty string if $3 is unset
    local context timestamp lineno custom_level color severity config_severity

    # Prepare log context (timestamp and line number)
    context=$(prepare_log_context)
    IFS="|" read -r timestamp lineno <<< "$context"

    # Validate the provided log level and message
    if [[ -z "${message:-}" || -z "${LOG_PROPERTIES[$level]:-}" ]]; then
        printf "Invalid log level or empty message in log_message.\n" >&2
        return 1
    fi

    # Extract log properties for the specified level
    IFS="|" read -r custom_level color severity <<< "${LOG_PROPERTIES[$level]}"
    severity="${severity:-0}"  # Default severity to 0 if not defined
    color="${color:-$RESET}"   # Default to reset color if not defined

    # Extract severity threshold for the configured log level
    IFS="|" read -r _ _ config_severity <<< "${LOG_PROPERTIES[$LOG_LEVEL]}"

    # Skip logging if the message's severity is below the configured threshold
    if (( severity < config_severity )); then
        return 0
    fi

    # Log to file if enabled
    if [[ "$LOG_OUTPUT" == "file" || "$LOG_OUTPUT" == "both" ]]; then
        printf "[%s] [%s] [%s:%s] %s\n" "$timestamp" "$custom_level" "$THIS_SCRIPT" "$lineno" "$message" >> "$LOG_FILE"
        [[ -n "$details" ]] && printf "[%s] [%s] [%s:%d] Details: %s\n" "$timestamp" "$custom_level" "$THIS_SCRIPT" "$lineno" "$details" >> "$LOG_FILE"
    fi

    # Log to console if enabled by `USE_CONSOLE`
    if [[ "$USE_CONSOLE" == "true" ]]; then
        printf "%b[%s] [%s:%s] %s%b\n" "$color" "$custom_level" "$THIS_SCRIPT" "$lineno" "$message" "$RESET"
        [[ -n "$details" ]] && printf "%bDetails: %s%b\n" "$color" "$details" "$RESET"
    fi
}

##
# @brief Log a message at the DEBUG level.
#
# This function logs messages with the DEBUG log level, typically used for detailed
# debugging information useful during development or troubleshooting.
#
# @param $1 Main log message.
# @param $2 [Optional] Extended details for the log entry.
##
logD() {
    echo
    log_message "DEBUG" "${1:-}" "${2:-}"  # Use an empty string as the defaults
}

##
# @brief Log a message at the INFO level.
#
# This function logs messages with the INFO log level, generally used for
# normal operational information or status updates.
#
# @param $1 Main log message.
# @param $2 [Optional] Extended details for the log entry.
##
logI() {
    log_message "INFO" "${1:-}" "${2:-}"  # Use an empty string as the defaults
}

##
# @brief Log a message at the WARNING level.
#
# This function logs messages with the WARNING log level, used for
# non-critical issues that might require attention.
#
# @param $1 Main log message.
# @param $2 [Optional] Extended details for the log entry.
##
logW() {
    log_message "WARNING" "${1:-}" "${2:-}"  # Use an empty string as the defaults
}

##
# @brief Log a message at the ERROR level.
#
# This function logs messages with the ERROR log level, used to report
# significant issues that may impact functionality.
#
# @param $1 Main log message.
# @param $2 [Optional] Extended details for the log entry.
##
logE() {
    log_message "ERROR" "${1:-}" "${2:-}"  # Use an empty string as the defaults
}

##
# @brief Log a message at the CRITICAL level.
#
# This function logs messages with the CRITICAL log level, used for
# severe issues that require immediate attention or could cause system failure.
#
# @param $1 Main log message.
# @param $2 [Optional] Extended details for the log entry.
##
logC() {
    log_message "CRITICAL" "${1:-}" "${2:-}"  # Use an empty string as the defaults
}

##
# @brief Ensure the log file exists and is writable, with fallback to `/tmp` if necessary.
#
# @details This function validates the specified log file's directory to ensure it exists and is writable.
# If the directory is invalid or inaccessible, it attempts to create it. If all else fails, the log file
# is redirected to `/tmp`. A warning message is logged if fallback is used.
#
# @global LOG_FILE Path to the log file (modifiable to fallback location).
# @global THIS_SCRIPT The name of the script (used to derive fallback log file name).
# @return None
##
init_log() {
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
    if [[ -d "$log_dir" && -w "$log_dir" ]]; then
        # Attempt to create the log file
        if ! touch "$LOG_FILE" &>/dev/null; then
            warn "Cannot create log file: $LOG_FILE"
            log_dir="/tmp"
        fi
    else
        log_dir="/tmp"
    fi

    # Fallback to /tmp if the directory is invalid
    if [[ "$log_dir" == "/tmp" ]]; then
        fallback_log="/tmp/$scriptname.log"
        LOG_FILE="$fallback_log"
        warn "Falling back to log file in /tmp: $LOG_FILE"
    fi

    # Attempt to create the log file in the fallback location
    if ! touch "$LOG_FILE" &>/dev/null; then
        die 1 "Unable to create log file even in fallback location: $LOG_FILE"
    fi

    readonly LOG_FILE
    export LOG_FILE
}

##
# @brief Retrieve the terminal color code or attribute.
#
# This function uses `tput` to retrieve a terminal color code or attribute
# (e.g., sgr0 for reset, bold for bold text). If the attribute is unsupported
# by the terminal, it returns an empty string.
#
# @param $1 The terminal color code or attribute to retrieve.
# @return The corresponding terminal value or an empty string if unsupported.
##
default_color() {
    tput "$@" 2>/dev/null || printf "\n"  # Fallback to an empty string on error
}

##
# @brief Execute and combine complex terminal control sequences.
#
# This function executes `tput` commands and other shell commands
# to create complex terminal control sequences. It supports commands
# like moving the cursor, clearing lines, and resetting attributes.
#
# @param $@ Commands and arguments to evaluate (supports multiple commands).
# @return The resulting terminal control sequence or an empty string if unsupported.
##
generate_terminal_sequence() {
    local result
    # Execute the command and capture its output, suppressing errors.
    result=$("$@" 2>/dev/null || printf "\n")
    printf "%s" "$result"
}

##
# @brief Initialize terminal colors and text formatting.
#
# This function sets up variables for foreground colors, background colors,
# and text formatting styles. It checks terminal capabilities and provides
# fallback values for unsupported or non-interactive environments.
##
# shellcheck disable=SC2034
init_colors() {
    # General text attributes
    RESET=$(default_color sgr0)
    BOLD=$(default_color bold)
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
    FGBLU=$(default_color setaf 4)
    FGMAG=$(default_color setaf 5)
    FGCYN=$(default_color setaf 6)
    FGWHT=$(default_color setaf 7)
    FGRST=$(default_color setaf 9)
    FGGLD=$(default_color setaf 220)

    # Background colors
    BGBLK=$(default_color setab 0)
    BGRED=$(default_color setab 1)
    BGGRN=$(default_color setab 2)
    BGYLW=$(default_color setab 3)
    BGBLU=$(default_color setab 4)
    BGMAG=$(default_color setab 5)
    BGCYN=$(default_color setab 6)
    BGWHT=$(default_color setab 7)
    BGRST=$(default_color setab 9)

    # Set variables as readonly
    readonly RESET BOLD SMSO RMSO UNDERLINE NO_UNDERLINE
    readonly BLINK NO_BLINK ITALIC NO_ITALIC MOVE_UP CLEAR_LINE
    readonly FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST FGGLD
    readonly BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST
    readonly DOT HHR LHR
}

##
# @brief Generate a separator string for terminal output.
# @details Creates heavy or light horizontal rules based on terminal width.
#
# @param $1 Type of rule: "heavy" or "light".
# @return The generated rule string.
##
generate_separator() {
    local type="${1,,}"  # Normalize to lowercase
    local width="${COLUMNS:-80}"  # Default width if COLUMNS is unavailable
    case "$type" in
        heavy) printf '═%.0s' $(seq 1 "$width") ;;
        light) printf '─%.0s' $(seq 1 "$width") ;;
        *) printf "Invalid separator type: %s\n" "$type" >&2; return 1 ;;
    esac
}

##
# @brief Validate the logging configuration, including LOG_LEVEL.
#
# This function checks whether the current LOG_LEVEL is valid. If LOG_LEVEL is not
# defined in the `LOG_PROPERTIES` associative array, it defaults to "INFO" and
# displays a warning message.
#
# @return void
##
validate_log_level() {
    # Ensure LOG_LEVEL is a valid key in LOG_PROPERTIES
    if [[ -z "${LOG_PROPERTIES[$LOG_LEVEL]:-}" ]]; then
        printf "ERROR: Invalid LOG_LEVEL '%s'. Defaulting to 'INFO'.\n" "$LOG_LEVEL" >&2
    fi
}

##
# @brief Sets up the logging environment for the script.
#
# This function initializes terminal colors, configures the logging environment,
# defines log properties, and validates both the log level and properties.
# It must be called before any logging-related functions.
#
# @details
# - Initializes terminal colors using `init_colors`.
# - Sets up the log file and directory using `init_log`.
# - Defines global log properties (`LOG_PROPERTIES`), including severity levels, colors, and labels.
# - Validates the configured log level and ensures all required log properties are defined.
#
# @note This function should be called once during script initialization.
#
# @return void
##
setup_log() {
    # Initialize terminal colors
    init_colors

    # Initialize logging environment
    init_log

    # Define log properties (severity, colors, and labels)
    declare -gA LOG_PROPERTIES=(
        ["DEBUG"]="DEBUG|${FGCYN}|0"
        ["INFO"]="INFO |${FGGRN}|1"
        ["WARNING"]="WARN|${FGYLW}|2"
        ["ERROR"]="ERROR|${FGRED}|3"
        ["CRITICAL"]="CRIT |${FGMAG}|4"
        ["EXTENDED"]="EXTD |${FGCYN}|0"
    )

    # Validate the log level and log properties
    validate_log_level
}

##
# @brief Toggle the USE_CONSOLE variable on or off.
#
# This function updates the global USE_CONSOLE variable to either "true" (off)
# or "false" (on) based on the input argument.
#
# @param $1 The desired state: "on" (to enable console logging) or "off" (to disable console logging).
# @return 0 on success, 1 on invalid input.
##
toggle_console_log() {
    local state="${1,,}"  # Convert input to lowercase for consistency

    case "$state" in
        on)
            logD "Console logging enabled."
            USE_CONSOLE="true"
            ;;
        off)
            USE_CONSOLE="false"
            logD "Console logging disabled."
            ;;
        *)
            logW "ERROR: Invalid argument for toggle_console_log." >&2
            return 1
            ;;
    esac

    return 0
}

############
### Get Project Parameters Functions
############

# @brief Retrieve the Git owner or organization name from the remote URL.
#
# This function fetches the remote URL of the current Git repository using the
# `git config --get remote.origin.url` command. It extracts the owner or
# organization name, which is the first path segment after the domain in the URL.
# If not inside a Git repository or no remote URL is configured, an error
# message is displayed, and the script exits with a non-zero status.
#
# @return Prints the owner or organization name to standard output if successful.
# @retval 0 Success: the owner or organization name is printed.
# @retval 1 Failure: prints an error message to standard error.
get_repo_org() {
    local url organization

    # Retrieve the remote URL from Git configuration.
    url=$(git config --get remote.origin.url)

    # Check if the URL is non-empty.
    if [[ -n "$url" ]]; then
        # Extract the owner or organization name.
        # Supports HTTPS and SSH Git URLs.
        organization=$(printf "%s" "$url" | sed -E 's#(git@|https://)([^:/]+)[:/]([^/]+)/.*#\3#')
        printf "%s\n" "$organization"
    else
        die 1 "Not inside a Git repository or no remote URL configured."
    fi
}

# @brief Retrieve the Git project name from the remote URL.
#
# This function fetches the remote URL of the current Git repository using the
# `git config --get remote.origin.url` command. It extracts the repository name
# from the URL, removing the `.git` suffix if present. If not inside a Git
# repository or no remote URL is configured, an error message is displayed,
# and the script exits with a non-zero status.
#
# @return Prints the project name to standard output if successful.
# @retval 0 Success: the project name is printed.
# @retval 1 Failure: prints an error message to standard error.
get_repo_name() {
    local url repo_name

    # Retrieve the remote URL from Git configuration.
    url=$(git config --get remote.origin.url)

    # Check if the URL is non-empty.
    if [[ -n "$url" ]]; then
        # Extract the repository name from the URL and remove the ".git" suffix if present.
        repo_name="${url##*/}"       # Remove everything up to the last `/`.
        repo_name="${repo_name%.git}" # Remove the `.git` suffix.
        printf "%s\n" "$repo_name"
    else
        die 1 "Not inside a Git repository or no remote URL configured."
    fi
}

##
# @brief Convert a Git repository name to title case.
# @details Replaces underscores and hyphens with spaces and converts words to title case.
#
# @param $1 The Git repository name (e.g., "my_repo-name").
# @return The repository name in title case (e.g., "My Repo Name").
##
repo_to_title_case() {
    local repo_name="$1"  # Input repository name
    local title_case      # Variable to hold the formatted name

    # Validate input
    if [[ -z "${repo_name:-}" ]]; then
        die 1 "Error: Repository name cannot be empty."
    fi

    # Replace underscores and hyphens with spaces and convert to title case
    title_case=$(printf "%s" "$repo_name" | tr '_-' ' ' | awk '{for (i=1; i<=NF; i++) $i=toupper(substr($i,1,1)) substr($i,2)}1')

    # Output the result
    printf "%s\n" "$title_case"
}

# @brief Retrieve the current Git branch name or the branch this was detached from.
#
# This function fetches the name of the currently checked-out branch in a Git
# repository. If the repository is in a detached HEAD state, it attempts to
# determine the branch or tag the HEAD was detached from. If not inside a
# Git repository, it displays an appropriate error message.
#
# @return Prints the current branch name or detached source to standard ou.
# @retval 0 Success: the branch or detached source name is printed.
# @retval 1 Failure: prints an error message to standard error.
get_git_branch() {
    local branch detached_from

    # Retrieve the current branch name using `git rev-parse`.
    branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)

    if [[ -n "$branch" && "$branch" != "HEAD" ]]; then
        # Print the branch name if available and not in a detached HEAD state.
        printf "%s\n" "$branch"
    elif [[ "$branch" == "HEAD" ]]; then
        # Handle the detached HEAD state: attempt to determine the source.
        detached_from=$(git reflog show --pretty='%gs' | grep -oE 'checkout: moving from [^ ]+' | head -n 1 | awk '{print $NF}')
        if [[ -n "$detached_from" ]]; then
            die 1 "Detached from branch: $detached_from"
        else
            die 1 "Detached HEAD state: Cannot determine the source branch."
        fi
    else
        die 1 "Not inside a Git repository."
    fi
}

# @brief Get the most recent Git tag.
# @return The most recent Git tag, or nothing if no tags exist.
get_last_tag() {
    local tag

    # Retrieve the most recent Git tag
    tag=$(git describe --tags --abbrev=0 2>/dev/null)

    printf "%s\n" "$tag"
}

# @brief Check if a tag follows semantic versioning.
# @param tag The Git tag to validate.
# @return "true" if the tag follows semantic versioning, otherwise "false".
is_sem_ver() {
    local tag="$1"

    # Validate if the tag follows the semantic versioning format (major.minor.patch)
    if [[ "$tag" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        printf "true\n"
    else
        printf "false\n"
    fi
}

# @brief Get the number of commits since the last tag.
# @param tag The Git tag to count commits from.
# @return The number of commits since the tag, or 0 if the tag does not exist.
get_num_commits() {
    local tag="$1" commit_count

    # Count the number of commits since the given tag
    commit_count=$(git rev-list --count "${tag}..HEAD" 2>/dev/null || printf "0\n")

    printf "%s\n" "$commit_count"
}

# @brief Get the short hash of the current Git commit.
# @return The short hash of the current Git commit.
get_short_hash() {
    local short_hash

    # Retrieve the short hash of the current Git commit
    short_hash=$(git rev-parse --short HEAD 2>/dev/null)

    printf "%s\n" "$short_hash"
}

# @brief Check if there are uncommitted changes in the working directory.
# @return "true" if there are uncommitted changes, otherwise "false".
get_dirty() {
    local changes

    # Check for uncommitted changes in the repository
    changes=$(git status --porcelain 2>/dev/null)

    if [[ -n "$changes" ]]; then
        printf "true\n"
    else
        printf "false\n"
    fi
}

# @brief Generate a version string based on the state of the Git repository.
# @return The generated semantic version string.
get_sem_ver() {
    local branch_name num_commits short_hash dirty version_string

    # Determine if the latest tag is a semantic version
    tag=$(get_last_tag)
    if [[ "$(is_sem_ver "$tag")" == "true" ]]; then
        version_string="$tag"
    else
        version_string="1.0.0" # Use default version if no valid tag exists
    fi

    # Retrieve the current branch name
    branch_name=$(get_git_branch)
    version_string="$version_string-$branch_name"

    # Get the number of commits since the last tag and append it to the tag
    num_commits=$(get_num_commits "$tag")
    if [[ "$num_commits" -gt 0 ]]; then
        version_string="$version_string+$num_commits"
    fi

    # Get the short hash and append it to the tag
    short_hash=$(get_short_hash)
    if [[ -n "$short_hash" ]]; then
        version_string="$version_string.$short_hash"
    fi

    # Check for a dirty working directory
    dirty=$(get_dirty)
    if [[ "$dirty" == "true" ]]; then
        version_string="$version_string-dirty"
    fi

    printf "%s\n" "$version_string"
}

#/**
# * @brief Configure local or remote mode based on the Git repository context.
# *
# * This function sets relevant variables for local mode if `USE_LOCAL` is `true` and
# * the script is being executed from within a GitHub repository (`IS_GITHUB_REPO` is `true`).
# * Defaults to remote configuration if not in local mode or when the combined check fails.
# *
# * @global USE_LOCAL           Indicates whether local mode is enabled.
# * @global IS_GITHUB_REPO      Indicates whether the script resides in a GitHub repository.
# * @global THIS_SCRIPT         Name of the current script.
# * @global REPO_ORG            Git organization or owner name.
# * @global REPO_NAME           Git repository name.
# * @global GIT_BRCH            Current Git branch name.
# * @global SEM_VER             Generated semantic version string.
# * @global LOCAL_SOURCE_DIR    Path to the root of the local repository.
# * @global LOCAL_WWW_DIR       Path to the `data` directory in the repository.
# * @global LOCAL_SCRIPTS_DIR   Path to the `scripts` directory in the repository.
# * @global GIT_RAW             URL for accessing raw files remotely.
# * @global GIT_API             URL for accessing the repository API.
# * @throws Exits with a critical error if the combined check fails in local mode.
# */
get_proj_params() {
    if [[ "$USE_LOCAL" == "true" && "$IS_GITHUB_REPO" == "true" ]]; then
        THIS_SCRIPT=$(basename "$0")
        REPO_ORG=$(get_repo_org) || die 1 "Failed to retrieve repository organization."
        REPO_NAME=$(get_repo_name) || die 1 "Failed to retrieve repository name."
        GIT_BRCH=$(get_git_branch) || die 1 "Failed to retrieve current branch name."
        SEM_VER=$(get_sem_ver) || die 1 "Failed to generate semantic version."

        # Get the root directory of the repository
        LOCAL_SOURCE_DIR=$(git rev-parse --show-toplevel 2>/dev/null)
        if [[ -z "${LOCAL_SOURCE_DIR:-}" ]]; then
            die 1 "Not inside a valid Git repository. Ensure the repository is properly initialized."
        fi

        # Set local paths based on repository structure
        LOCAL_WWW_DIR="$LOCAL_SOURCE_DIR/data"
        LOCAL_SCRIPTS_DIR="$LOCAL_SOURCE_DIR/scripts"
    else
        # Configure remote access URLs
        if [[ -z "${REPO_ORG:-}" || -z "${REPO_NAME:-}" ]]; then
            die 1 "Remote mode requires REPO_ORG and REPO_NAME to be set."
        fi
        GIT_RAW="https://raw.githubusercontent.com/$REPO_ORG/$REPO_NAME"
        GIT_API="https://api.github.com/repos/$REPO_ORG/$REPO_NAME"
    fi

    # Export global variables for further use
    export THIS_SCRIPT REPO_ORG REPO_NAME GIT_BRCH SEM_VER LOCAL_SOURCE_DIR
    export LOCAL_WWW_DIR LOCAL_SCRIPTS_DIR GIT_RAW GIT_API
}

############
### Install Functions
############


##
# @brief Start the script, with optional timeout for non-interactive environments.
# @details Allows users to press a key to proceed, or defaults after 10 seconds.
#
# @global TERSE Indicates terse mode (skips interactive messages).
# @global TERSE Indicates interactivity of the environment.
# @return None
##
start_script() {
    if [[ "${TERSE:-false}" == "true" ]]; then
        logI "$(repo_to_title_case "${REPO_NAME:-Unknown}") installation beginning."
        return
    fi

    # Prompt user for input
    clear
    printf "\nStarting installation for: %s.\n" "$(repo_to_title_case "${REPO_NAME:-Unknown}")"
    printf "Press any key to continue or 'Q' to quit (defaulting in 10 seconds).\n"

    # Read a single key with a 10-second timeout
    if ! read -n 1 -sr -t 10 key < /dev/tty; then
        key=""  # Assign a default value on timeout
    fi

    # Handle user input
    case "${key}" in
        [Qq])  # Quit
            logD "Installation canceled by user."
            exit_script 0
            ;;
        "")  # Timeout or Enter
            logI "No key pressed (timeout or Enter). Proceeding with installation."
            ;;
        *)  # Any other key
            logI "Key pressed: '${key}'. Proceeding with installation."
            ;;
    esac
}

##
# @brief Sets the system timezone interactively or logs if already set.
# @details If the current timezone is not GMT or BST, logs the current date and time,
#          and exits. Otherwise, prompts the user to confirm or reconfigure the timezone.
#
# @return None Logs the current timezone or adjusts it if necessary.
##
set_time() {
    # Declare local variables
    local need_set=false
    local current_date tz yn

    # Get the current date and time
    current_date="$(date)"
    tz="$(date +%Z)"

    # Log and return if the timezone is not GMT or BST
    if [ "$tz" != "GMT" ] && [ "$tz" != "BST" ]; then
        need_set=true
        return
    fi

    # Check if the script is in terse mode
    if [[ "$TERSE" == "true" && "$need_set" == "true" ]]; then
        logW "Timezone detected as $tz, which may need to be updated."
        return
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
                break
                ;;
            [Nn]* | *)
                dpkg-reconfigure tzdata
                logI "Timezone reconfigured on $current_date"
                break
                ;;
        esac
    done
}

##
# @brief Execute a command and return its success or failure.
#
# This function executes a given command, logs its status, and optionally
# prints status messages to the console depending on the value of `USE_CONSOLE`.
# It returns `true` for success or `false` for failure.
#
# @param $1 The name/message for the operation.
# @param $2 The command/process to execute.
# @return Returns 0 (true) if the command succeeds, or non-zero (false) if it fails.
##
exec_command() {
    local exec_name="$1"            # The name/message for the operation
    local exec_process="$2"         # The command/process to execute
    local result                    # To store the exit status of the command

    local running_pre="Running"    # Prefix for running message
    local complete_pre="Complete"  # Prefix for success message
    local failed_pre="Failed"      # Prefix for failure message
    if [[ "${DRY_RUN}" == "true" ]]; then
        local dry=" (dry)"
        running_pre+="$dry"
        complete_pre+="$dry"
        failed_pre+="$dry"
    fi
    running_pre+=":"
    complete_pre+=":"
    failed_pre+=":"

    logI "$running_pre '$exec_name'."

    # Log the running line
    if [[ "${USE_CONSOLE}" == "false" ]]; then
        printf "%b[-]%b\t%s %s.\n" "${FGGLD}${BOLD}" "$RESET" "$running_pre" "$exec_name"
    fi

    # If it's a DRY_RUN just use sleep
    if [[ "${DRY_RUN}" == "true" ]]; then
        sleep 1  # Simulate execution delay
        result=0 # Simulate success
    else
        # Execute the task command, suppress output, and capture result
        result=$(
            {
                result=0
                eval "$exec_process" > /dev/null 2>&1 || result=$?
                printf "%s" "$?"
            }
        )
    fi

    # Move the cursor up and clear the entire line if USE_CONSOLE is false
    if [[ "${USE_CONSOLE}" == "false" ]]; then
        printf "%s" "$MOVE_UP"
    fi

    # Handle success or failure
    if [ "$result" -eq 0 ]; then
        # Success case
        if [[ "${USE_CONSOLE}" == "false" ]]; then
            printf "%b[✔]%b\t %s.\n" "${FGGRN}${BOLD}" "${RESET}" "$complete_pre" "$exec_name"
        fi
        logI "$complete_pre $exec_name"
        return 0 # Success (true)
    else
        # Failure case
        if [[ "${USE_CONSOLE}" == "false" ]]; then
            printf "%b[✘]%b\t%s %s (%s).\n" "${FGRED}${BOLD}" "${RESET}" "$failed_pre" "$exec_name" "$result"
        fi
        logE "$failed_pre $exec_name"
        return 1 # Failure (false)
    fi
}

##
# @brief Installs or upgrades all packages in the APT_PACKAGES list.
# @details Updates the package list and resolves broken dependencies before proceeding.
#
# Accumulates errors for each failed package and logs a summary at the end.
#
# Skips execution if the APT_PACKAGES array is empty.
#
# @return Logs the success or failure of each operation.
##
handle_apt_packages() {
    # Check if APT_PACKAGES is empty
    if [[ ${#APT_PACKAGES[@]} -eq 0 ]]; then
        logI "No packages specified in APT_PACKAGES. Skipping package handling."
        return 0
    fi

    local package error_count=0  # Counter for failed operations

    logI "Updating and managing required packages (this may take a few minutes)."

    # Update package list and fix broken installs
    if ! exec_command "Update local package index" "sudo apt-get update -y"; then
        logE "Failed to update package list."
        ((error_count++))
    fi
    if ! exec_command "Fixing broken or incomplete package installations" "sudo apt-get install -f -y"; then
        logE "Failed to fix broken installs."
        ((error_count++))
    fi

    # Install or upgrade each package in the list
    for package in "${APT_PACKAGES[@]}"; do
        if dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q "install ok installed"; then
            if ! exec_command "Upgrade $package" "sudo apt-get install --only-upgrade -y $package"; then
                logW "Failed to upgrade package: $package."
                ((error_count++))
            fi
        else
            if ! exec_command "Install $package" "sudo apt-get install -y $package"; then
                logW "Failed to install package: $package."
                ((error_count++))
            fi
        fi
    done

    # Log summary of errors
    if ((error_count > 0)); then
        logE "APT package handling completed with $error_count errors."
    else
        logI "APT package handling completed successfully."
    fi

    return $error_count
}

##
# @brief End the script with optional feedback based on logging configuration.
# @details Provides a clear message to indicate the script completed successfully.
#
# @global REBOOT Indicates if a reboot is required.
# @global USE_CONSOLE Controls whether console output is enabled.
# @return None
##
finish_script() {
    if [[ "$TERSE" == "true" || "$TERSE" != "true" ]]; then
        logI "Installation complete: $(repo_to_title_case "$REPO_NAME")."
        return
    fi

    clear
    printf "Installation complete: %s.\n" "$(repo_to_title_case "$REPO_NAME")"
}

exit_script() {
    local message="${1:-Exiting.}"  # Default message if no argument is provided
    # clear
    printf "%s\n" "$message"  # Log the provided or default message
    exit 0
}

############
### Arguments Functions
############

##
# @brief Define script options and their properties.
# @details Each option includes its long form, short form, and description.
#
# @global OPTIONS Associative array of script options and their properties.
# @return None
##
declare -A OPTIONS=(
    ["--dry-run|-d"]="Enable dry-run mode (no actions performed)."
    ["--version|-v"]="Display script version and exit."
    ["--help|-h"]="Show this help message and exit."
    ["--log-file|-f <path>"]="Specify the log file location."
    ["--log-level|-l <level>"]="Set the logging verbosity level (DEBUG, INFO, WARNING, ERROR, CRITICAL)."
    ["--terse|-t"]="Enable terse output mode."
    ["--console|-c"]="Enable console logging."
)

##
# @brief Display script usage.
# @details Generates usage dynamically based on the `OPTIONS` array.
#
# @global OPTIONS Associative array of script options and their properties.
# @return None
##
usage() {
    printf "Usage: %s [options]\n\n" "$THIS_SCRIPT"
    printf "Options:\n"
    for key in "${!OPTIONS[@]}"; do
        printf "  %s: %s\n" "$key" "${OPTIONS[$key]}"
    done
}

##
# @brief Parse command-line arguments.
# @details Uses the `OPTIONS` array for validation and handling.
#
# @param "$@" The command-line arguments passed to the script.
# @global DRY_RUN, LOG_FILE, LOG_LEVEL, TERSE, USE_CONSOLE Updated based on input.
# @return None
##
# shellcheck disable=SC2034
parse_args() {
    while [[ "$#" -gt 0 ]]; do
        case "$1" in
            --dry-run|-d)
                DRY_RUN=true
                ;;
            --version|-v)
                print_version
                exit 0
                ;;
            --help|-h)
                usage
                exit 0
                ;;
            --log-file|-f)
                LOG_FILE=$(realpath -m "$2" 2>/dev/null)
                shift
                ;;
            --log-level|-l)
                LOG_LEVEL="$2"
                shift
                ;;
            --terse|-t)
                TERSE="true"
                ;;
            --console|-c)
                USE_CONSOLE="true"
                ;;
            *)
                printf "Unknown option: %s\n" "$1"
                usage
                exit 1
                ;;
        esac
        shift
    done
}

############
### More Installer Functions Here
############

############
### Main Functions
############

# Main function
main() {
    ############
    ### Check Environment Functions
    ############

    # Get execution context of the script and sets relevant environment variables.
    handle_execution_context # Pass "debug" to enable debug output

    # Get Project Parameters Functions
    get_proj_params     # Get project and git parameters

    # Arguments Functions
    parse_args "$@"     # Parse command-line arguments

    # Check Environment Functions
    enforce_sudo        # Ensure proper privileges for script execution
    validate_depends    # Ensure required dependencies are installed
    validate_sys_accs   # Verify critical system files are accessible
    validate_env_vars   # Check for required environment variables

    # Logging Functions
    setup_log           # Setup logging environment

    # Check Environment Functions with debug available, pass "debug" to enable debug output.
    check_bash          # Ensure the script is executed in a Bash shell
    check_sh_ver        # Verify the current Bash version meets minimum requirements
    check_bitness       # Validate system bitness compatibility
    check_release       # Check Raspbian OS version compatibility

    check_arch          # Validate Raspberry Pi model compatibility, pass "debug" to enable debug output.
    check_internet      # Verify internet connectivity if required, pass "debug" to enable debug output.

    # Print/Display Environment Functions
    print_system        # Log system information, pass "debug" to enable debug output.
    print_version       # Log the script version

    ############
    ### Installer Functions
    ############
    start_script
    set_time
    handle_apt_packages
    finish_script
}

# Run the main function and exit with its return status
main "$@"
exit $?
