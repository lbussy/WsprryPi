#!/bin/bash
# shellcheck disable=SC2034
#
# This script is designed for Bash (>= 4.x) and uses Bash-specific features.
# It is not intended to be POSIX-compliant.
#
# This file is part of WsprryPi.
#
# Copyright (C) 2023-2025 Lee C. Bussy (@LBussy)
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

# TODO:
#   - Change the line number in log_message to be the one calling the message + scriptname
#   - Test string variables in arguments (move this back to log.sh)
#   - Implement variable expansion on log path argument (move this back to log.sh)
#   - Consider implementing DRY_RUN in future work

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
# shellcheck disable=SC2329
trap_error() {
    local func="${FUNCNAME[1]:-main}"  # Get the calling function name (default: "main")
    local line="$1"                   # Line number where the error occurred
    local script="${THIS_SCRIPT:-$(basename "$0")}"  # Script name (fallback to current script)

    # Log the error message and exit
    echo "ERROR: An unexpected error occurred in function '$func' at line $line of script '$script'. Exiting." >&2
    exit 1
}

# Set the trap to call trap_error on any error
# Uncomment the next line to help with script debugging
# trap 'trap_error "$LINENO"' ERR

############
### Global Script Declarations
############

##
# @brief Logging-related constants for the script.
# @details Sets the script name (`THIS_SCRIPT`) based on the current environment.
# If `THIS_SCRIPT` is already defined, its value is retained; otherwise, it is set
# to the basename of the script or defaults to "script.sh".
#
# @global THIS_SCRIPT The name of the script.
##
declare THIS_SCRIPT="${THIS_SCRIPT:-install.sh}" # Use existing value, or default to "install.sh".

##
# @brief Project metadata constants used throughout the script.
# @details These variables provide metadata about the script, including ownership,
# versioning, and project details. All are marked as read-only.
##
declare SEM_VER="1.2.1-version-files+91.3bef855-dirty"
declare GIT_BRCH="install_update"
declare REPO_ORG="${REPO_ORG:-lbussy}"
declare REPO_NAME="${REPO_NAME:-WsprryPi}"
declare GIT_BRCH="${GIT_BRCH:-main}"
declare SEM_VER="${SEM_VER:-1.0.0}"
declare GIT_DEF_BRCH=("main" "master")

##
# @var USE_CONSOLE
# @brief Controls whether console logging is enabled.
# @details Allows enabling or disabling console logging based on the environment variable.
# If the environment variable `USE_CONSOLE` is not set, it defaults to `true`.
# To disable console logging, set `USE_CONSOLE=false` in the environment or modify this variable.
# @default "true"
##
# TODO: Implement logic to disable console logging when USE_CONSOLE is set to "false".
USE_CONSOLE="${USE_CONSOLE:-false}"

##
# @var TERSE
# @brief Controls terse output mode.
# @details
# This variable determines whether the script operates in terse output mode.
# When enabled (`true`), the script produces minimal output.
# 
# Default Behavior:
# - If `TERSE` is unset or null, it defaults to `"true"`.
# - If `TERSE` is already set, its value is preserved.
#
# Example:
# - `TERSE="false"` disables terse output mode.
# - If `TERSE` is not explicitly set, it will default to `"true"`.
#
# @default "true"
##
TERSE="${TERSE:-true}"

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
# - "true": Always log to the file.
# - "false": Never log to the file.
# - unset: Follow the logic defined in the `is_interactive()` function.
# Defaults to blank if not set.
##
# TODO:  Fix this note
declare LOG_TO_FILE="${LOG_TO_FILE:-true}"  # Default to blank if not set.

##
# @var LOG_FILE
# @brief Specifies the path to the log file.
# @details Defaults to blank if not provided. This can be set externally to 
# specify a custom log file path.
##
declare LOG_FILE="${LOG_FILE:-}"  # Use the provided LOG_FILE or default to blank.

##
# @var FALLBACK_NAME
# @brief Specifies the fallback name for the script.
# @details Used when the script is piped or the original name cannot be determined.
# Defaults to "install.sh" if not provided.
# @default "install.sh"
##
readonly FALLBACK_NAME="${FALLBACK_NAME:-install.sh}"  # Default fallback name if the script is piped.

##
# @var LOG_LEVEL
# @brief Specifies the logging verbosity level.
# @details Defaults to "DEBUG" if not set. Other possible levels can be defined 
# depending on script requirements (e.g., INFO, WARN, ERROR).
# @default "DEBUG"
##
declare LOG_LEVEL="${LOG_LEVEL:-DEBUG}"  # Default log level is DEBUG if not set.

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
    "awk" "grep" "tput" "cut" "tr" "getconf" "cat" "sed" "basename"
    "getent" "date" "mktemp" "printf" "whoami" "mkdir" "touch" "echo"
    "dpkg" "git" "dpkg-reconfigure" "curl" "realpath"
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
    "/etc/os-release"                # OS identification file
    "/proc/device-tree/compatible"  # Hardware compatibility file
)
readonly SYSTEM_READS

##
# @var APTPACKAGES
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
readonly APTPACKAGES=(
    "apache2" "php" "jq" "libraspberrypi-dev" "raspberrypi-kernel-headers"
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
### Skeleton Functions
############

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

    # Check terminal color support
    tput_colors_available=$(tput colors 2>/dev/null || echo "0")

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
        local caller_lineno="${BASH_LINENO[1]}"
        printf "%b%s stack trace called by line: %d%b\n" "$color" "$label" "$caller_lineno" "\033[0m" >&2
    fi

    # Print each function in the stack trace
    for ((i = 2; i < ${#FUNCNAME[@]}; i++)); do
        local script="${BASH_SOURCE[i]##*/}"
        local caller_lineno="${BASH_LINENO[i - 1]}"
        printf "%b[%d] Function: %s called at %s:%d%b\n" \
            "$color" $((i - 1)) "${FUNCNAME[i]}" "$script" "$caller_lineno" "\033[0m" >&2
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
    # Initialize default values
    local error_level=0            # Default error level
    local level="WARNING"          # Default log level
    local message="An issue was raised on this line"  # Default log message
    local details=""               # Default to no additional details
    local lineno="${BASH_LINENO[1]}"  # Line number where the function was called
    local script="$SCRIPT_NAME"     # Script name

    # Parse arguments in order
    if [[ "$1" == "WARNING" || "$1" == "ERROR" ]]; then
        level="$1"
        shift
    fi

    if [[ "$1" =~ ^[0-9]+$ ]]; then
        error_level="$1"
        shift
    fi

    if [[ -n "$1" ]]; then
        message="$1"
        shift
    fi

    if [[ -n "$1" ]]; then
        details="$1"
    fi

    # Append error level to the log message
    message="${message}: ($error_level)"

    # Log the message with the appropriate level
    if [[ "$level" == "WARNING" ]]; then
        logW "$message" "$details"
    elif [[ "$level" == "ERROR" ]]; then
        logE "$message" "$details"
    fi

    # Optionally print a stack trace for warnings
    if [[ "$level" == "WARNING" && "$WARN_STACK_TRACE" == "true" ]]; then
        stack_trace "$level" "Stack trace for $level at line $lineno: $message"
    fi

    return
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
    local script="$SCRIPT_NAME"          # Script name
    local level="CRITICAL"              # Error level
    local tag="${level:0:4}"            # Extracts the first 4 characters (e.g., "CRIT")

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
        printf "[%s]\t[%s:%d]\t%s\n" "$tag" "$script" "$lineno" "$message" >&2
        if [[ -n "$details" ]]; then
            printf "[%s]\t[%s:%d]\tDetails: %s\n" "$tag" "$script" "$lineno" "$details" >&2
        fi
    fi

    # Log the unrecoverable error
    printf "[%s]\t[%s:%d]\tUnrecoverable error (exit status: %d).\n" \
        "$tag" "$script" "$lineno" "$exit_status" >&2

    # Call stack_trace with processed message and error level
    if [[ -z "$message" ]]; then
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
# shellcheck disable=SC2329
add_dot() {
    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "$input" ]]; then
        warn "ERROR" "Input to add_dot cannot be empty."
        return 1
    fi

    # Add a leading dot if it's missing
    if [[ "$input" != .* ]]; then
        input=".$input"
    fi

    echo "$input"
}

##
# @brief Remove a leading dot from a string if present.
# @details Ensures the input string does not start with a dot (`.`). If the input is empty, it returns an error.
#
# @param $1 The input string to process.
# @return The modified string without a leading dot, or an error if the input is invalid.
##
# shellcheck disable=SC2329
remove_dot() {
    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "$input" ]]; then
        warn "ERROR" "Input to remove_dot cannot be empty."
        return 1
    fi

    # Remove the leading dot if present
    if [[ "$input" == .* ]]; then
        input="${input#.}"
    fi

    echo "$input"
}

##
# @brief Add a trailing slash to a string if it's missing.
# @details Ensures the input string ends with a slash (`/`). If the input is empty, it returns an error.
#
# @param $1 The input string to process.
# @return The modified string with a trailing slash, or an error if the input is invalid.
##
# shellcheck disable=SC2329
add_slash() {
    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "$input" ]]; then
        warn "ERROR" "Input to add_slash cannot be empty."
        return 1
    fi

    # Add a trailing slash if it's missing
    if [[ "$input" != */ ]]; then
        input="$input/"
    fi

    echo "$input"
}

##
# @brief Remove a trailing slash from a string if present.
# @details Ensures the input string does not end with a slash (`/`). If the input is empty, it returns an error.
#
# @param $1 The input string to process.
# @return The modified string without a trailing slash, or an error if the input is invalid.
##
# shellcheck disable=SC2329
remove_slash() {
    local input="$1"  # Input string to process

    # Validate input
    if [[ -z "$input" ]]; then
        warn "ERROR" "Input to remove_slash cannot be empty."
        return 1
    fi

    # Remove the trailing slash if present
    if [[ "$input" == */ ]]; then
        input="${input%/}"
    fi

    echo "$input"
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
print_system() {
    # Declare local variables at the start of the function
    local system_name

    # Extract system name and version from /etc/os-release
    system_name=$(grep '^PRETTY_NAME=' /etc/os-release 2>/dev/null | cut -d '=' -f2 | tr -d '"')

    # Check if system_name is empty
    if [[ -z "$system_name" ]]; then
        logW "System: Unknown (could not extract system information)." # Warning if system information is unavailable
    else
        logD "System: $system_name." # Log the system information
    fi
}

##
# @brief Print the script version and optionally log it.
# @details This function displays the version of the script stored in the global
#          variable `SEM_VER`. It uses `echo` if called by `parse_args`, otherwise
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
        echo -e "$THIS_SCRIPT: version $SEM_VER" # Display the script name and version
    else
        logD "Running $THIS_SCRIPT version $SEM_VER" # Log the script name and version
    fi
}

############
### Check Environment Functions
############

##
# @brief Determine how the script was executed.
# @details Checks if the script is being run directly, piped through bash,
#          or executed in an unusual manner.
#
# @global FALLBACK_NAME Name to use if script execution is piped or unusual.
# @global USE_LOCAL Boolean indicating whether local resources are used.
#
# @return None
##
check_pipe() {
    local this_script  # Local variable for script name

    if [[ "$0" == "bash" ]]; then
        if [[ -p /dev/stdin ]]; then
            # Script is being piped through bash
            THIS_SCRIPT="$FALLBACK_NAME"
        else
            # Script was run in an unusual way with 'bash'
            THIS_SCRIPT="$FALLBACK_NAME"
        fi
        USE_LOCAL=false
    else
        # Script run directly
        USE_LOCAL=true
    fi
    export THIS_SCRIPT USE_LOCAL
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
            return  # Script is properly executed with `sudo`
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
            printf "ERROR: Missing dependency: %s\n" "$dep" >&2
            ((missing++))
        fi
    done

    if ((missing > 0)); then
        die 1 "Missing $missing dependencies. Install them and re-run the script."
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
        if [[ -z "${!var}" ]]; then
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
# @global BASH_VERSION The version of the Bash shell being used.
#
# @return None
# @exit 1 if not running in Bash.
##
check_bash() {
    if [[ -z "$BASH_VERSION" ]]; then
        printf "ERROR: This script requires Bash. Please run it with Bash.\n" >&2
        exit 1
    fi
}

##
# @brief Check if the current Bash version meets the minimum required version.
# @details Compares the current Bash version against a required version specified
#          in the global variable `MIN_BASH_VERSION`. If `MIN_BASH_VERSION` is "none",
#          the check is skipped.
#
# @global MIN_BASH_VERSION Minimum required Bash version (e.g., "4.0") or "none".
# @global BASH_VERSINFO Array containing the major and minor versions of the running Bash.
#
# @return None
# @exit 1 if the Bash version is insufficient.
##
check_sh_ver() {
    local required_version="${MIN_BASH_VERSION:-none}"

    if [[ "$required_version" == "none" ]]; then
        return  # Skip version check
    fi

    if ((BASH_VERSINFO[0] < ${required_version%%.*} || 
         (BASH_VERSINFO[0] == ${required_version%%.*} && 
          BASH_VERSINFO[1] < ${required_version##*.}))); then
        die 1 "This script requires Bash version $required_version or newer."
    fi
}

##
# @brief Check system bitness compatibility.
# @details Validates whether the current system's bitness matches the supported configuration.
#
# @global SUPPORTED_BITNESS Specifies the bitness supported by the script ("32", "64", or "both").
#
# @return None
# @exit 1 if the system bitness is unsupported.
##
check_bitness() {
    local bitness  # Stores the detected bitness of the system.

    bitness=$(getconf LONG_BIT)

    case "$SUPPORTED_BITNESS" in
        "32")
            if [[ "$bitness" -ne 32 ]]; then
                die 1 "Only 32-bit systems are supported. Detected $bitness-bit system."
            fi
            ;;
        "64")
            if [[ "$bitness" -ne 64 ]]; then
                die 1 "Only 64-bit systems are supported. Detected $bitness-bit system."
            fi
            ;;
        "both")
            ;;
        *)
            die 1 "Configuration error: Invalid value for SUPPORTED_BITNESS ('$SUPPORTED_BITNESS')."
            ;;
    esac
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
check_release() {
    local ver  # Holds the extracted version ID from /etc/os-release.

    # Ensure the file exists and is readable.
    if [[ ! -f /etc/os-release || ! -r /etc/os-release ]]; then
        die 1 "Unable to read /etc/os-release. Ensure this script is run on a compatible system."
    fi

    # Extract the VERSION_ID from /etc/os-release.
    if ! ver=$(grep "VERSION_ID" /etc/os-release 2>/dev/null | awk -F "=" '{print $2}' | tr -d '"'); then
        die 1 "Failed to extract version information from /etc/os-release."
    fi

    # Ensure the extracted version is not empty.
    if [[ -z "$ver" ]]; then
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
    logD "Raspbian version $ver is supported."
}

##
# @brief Check if the detected Raspberry Pi model is supported.
# @details Reads the Raspberry Pi model from /proc/device-tree/compatible and checks
#          it against a predefined list of supported models. Logs an error if the
#          model is unsupported or cannot be detected.
#
# @global SUPPORTED_MODELS Associative array of supported and unsupported Raspberry Pi models.
# @global log_message Function for logging messages.
# @global die Function to handle critical errors and terminate the script.
#
# @return None Exits the script with an error code if the architecture is unsupported.
##
check_arch() {
    local detected_model is_supported key full_name model chip  # Local variables.

    # Attempt to read and process the compatible string.
    if ! detected_model=$(cat /proc/device-tree/compatible 2>/dev/null | tr '\0' '\n' | grep "raspberrypi" | sed 's/raspberrypi,//'); then
        die 1 "Failed to read or process /proc/device-tree/compatible. Ensure compatibility."
    fi

    # Check if the detected model is empty.
    if [[ -z "$detected_model" ]]; then
        die 1 "No Raspberry Pi model found in /proc/device-tree/compatible. This system may not be supported."
    fi

    # Initialize is_supported flag.
    is_supported=false

    # Iterate through supported models to check compatibility.
    for key in "${!SUPPORTED_MODELS[@]}"; do
        IFS='|' read -r full_name model chip <<< "$key"
        if [[ "$model" == "$detected_model" ]]; then
            if [[ "${SUPPORTED_MODELS[$key]}" == "Supported" ]]; then
                is_supported=true
                logD "Model: '$full_name' ($chip) is supported."
            else
                die 1 "Model: '$full_name' ($chip) is not supported."
            fi
            break
        fi
    done

    # Log an error if no supported model was found.
    if [[ "$is_supported" == false ]]; then
        die 1 "Detected Raspberry Pi model '$detected_model' is not recognized or supported."
    fi
}

##
# @brief Check if the system has an internet connection by making an HTTP request.
# @details Uses curl to send a request to google.com and checks the response status to determine if the system is online.
#          Skips the check if the global variable REQUIRE_INTERNET is not set to true.
#
# @global REQUIRE_INTERNET A flag indicating whether internet connectivity should be checked.
#
# @return 0 if the system is online or the internet check is skipped, 1 if the system is offline and REQUIRE_INTERNET is true.
##
check_internet() {
    # Skip check if REQUIRE_INTERNET is not true.
    if [ "$REQUIRE_INTERNET" != "true" ]; then
        return
    fi

    # Check for internet connectivity using curl.
    if curl -s --head http://google.com | grep "HTTP/1\.[01] [23].." > /dev/null; then
        logD "Internet is available."
    else
        die 1 "No Internet connection detected."
    fi
}

############
### Logging Functions
############

##
# @brief Print a log entry to the terminal and/or log file.
# @param $1 Timestamp of the log entry.
# @param $2 Log level (e.g., DEBUG, INFO, ERROR).
# @param $3 Color code for the log level.
# @param $4 Line number where the log was invoked.
# @param $5 Main log message.
# @param $6 Optional extended details for the log entry.
##
print_log_entry() {
    local timestamp="$1"
    local level="$2"
    local color="$3"
    local lineno="$4"
    local message="$5"
    local details="$6"

    # Determine if logging to file is enabled
    local should_log_to_file=false
    if [[ "${LOG_TO_FILE,,}" == "true" ]]; then
        should_log_to_file=true
    elif [[ "${LOG_TO_FILE,,}" == "false" ]]; then
        should_log_to_file=false
    elif [[ "${USE_CONSOLE,,}" == "false" ]]; then
        should_log_to_file=false
    else
        should_log_to_file=true
    fi

    # Log to file if applicable
    if "$should_log_to_file"; then
        printf "[%s]\t[%s]\t[%s:%d]\t%s\n" "$timestamp" "$level" "$THIS_SCRIPT" "$lineno" "$message" >> "$LOG_FILE"
        [[ -n "$details" ]] && printf "[%s]\t[%s]\t[%s:%d]\tDetails: %s\n" "$timestamp" "$level" "$THIS_SCRIPT" "$lineno" "$details" >> "$LOG_FILE"
    fi

    # Log to the terminal only if USE_CONSOLE is faltruese
    if [[ "${USE_CONSOLE}" == "true" ]]; then
        # Print the main log message
        echo -e "${BOLD}${color}[${level}]${RESET}\t${color}[$THIS_SCRIPT:$lineno]${RESET}\t$message"

        # Print the details if provided, using the EXTENDED log level color and format
        if [[ -n "$details" && -n "${LOG_PROPERTIES[EXTENDED]}" ]]; then
            IFS="|" read -r extended_label extended_color _ <<< "${LOG_PROPERTIES[EXTENDED]}"
            echo -e "${BOLD}${extended_color}[${extended_label}]${RESET}\t${extended_color}[$THIS_SCRIPT:$lineno]${RESET}\tDetails: $details"
        fi
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
    lineno="${BASH_LINENO[0]}"

    # Return the pipe-separated timestamp and line number
    echo "$timestamp|$lineno"
}

##
# @brief Log a message with the specified log level.
#
# This function logs a message with details such as timestamp, log level, line number,
# and optional extended details. It respects the configured log level threshold.
#
# @param $1 Log level (e.g., DEBUG, INFO, ERROR).
# @param $2 Main log message.
# @param $3 [Optional] Extended details for the log entry.
#
# @return 0 if the message is successfully logged, or 1 on error.
##
log_message() {
    # Convert log level to uppercase for consistency
    local level="${1^^}"
    local message="$2"
    local details="$3"

    # Context variables for logging
    local context timestamp lineno custom_level color severity config_severity

    # Generate context (timestamp and line number)
    context=$(prepare_log_context)
    IFS="|" read -r timestamp lineno <<< "$context"

    # Validate log level and message
    if [[ -z "$message" || -z "${LOG_PROPERTIES[$level]}" ]]; then
        echo -e "ERROR: Invalid log level or empty message in ${FUNCNAME[0]}." >&2 && exit 1
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

    # Print the log entry
    print_log_entry "$timestamp" "$custom_level" "$color" "$lineno" "$message" "$details"
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
    log_message "DEBUG" "$1" "$2"
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
    log_message "INFO" "$1" "$2"
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
    log_message "WARNING" "$1" "$2"
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
    log_message "ERROR" "$1" "$2"
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
    log_message "CRITICAL" "$1" "$2"
}

##
# @brief Ensure the log file exists and is writable.
#
# This function validates that the log directory exists and is writable. If the specified
# log directory is invalid or inaccessible, it attempts to create the directory. If all
# else fails, it falls back to creating a temporary log file in `/tmp`.
##
init_log() {
    # Local variables
    local scriptname="${THIS_SCRIPT%%.*}"  # Extract script name without extension
    local homepath                        # Home directory of the current user
    local log_dir                         # Directory of the log file

    # Get the home directory of the current user
    homepath=$(getent passwd "${SUDO_USER:-$(whoami)}" | { IFS=':'; read -r _ _ _ _ _ homedir _; echo "$homedir"; })

    # Determine the log file location
    LOG_FILE="${LOG_FILE:-$homepath/$scriptname.log}"

    # Extract the log directory from the log file path
    log_dir=$(dirname "$LOG_FILE")

    # Ensure the log directory exists
    if [[ ! -d "$log_dir" ]]; then
        echo -e "ERROR: Log directory does not exist: $log_dir in ${FUNCNAME[0]}" >&2 && exit 1
    fi

    # Check if the log directory is writable
    if [[ ! -w "$log_dir" ]]; then
        echo -e "ERROR: Log directory is not writable: $log_dir in ${FUNCNAME[0]}" >&2 && exit 1
    fi

    # Attempt to create the log file
    touch "$LOG_FILE" >&2 || (echo -e "ERROR: Cannot create log file: $LOG_FILE in ${FUNCNAME[0]}" && exit 1)

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
    tput "$@" 2>/dev/null || echo ""  # Fallback to an empty string on error
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
    result=$("$@" 2>/dev/null || echo "")
    echo "$result"
}

##
# @brief Initialize terminal colors and text formatting.
#
# This function sets up variables for foreground colors, background colors,
# and text formatting styles. It checks terminal capabilities and provides
# fallback values for unsupported or non-interactive environments.
##
init_colors() {
    # Local variable to check terminal color support
    local tput_colors_available
    # shellcheck disable=SC2034  # Intentional use for clarity
    tput_colors_available=$(tput colors 2>/dev/null || echo "0")

    # Initialize colors and formatting if interactive and the terminal supports at least 8 colors
    if [ "$tput_colors_available" -ge 8 ]; then
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

        # Additional formatting and separators
        DOT="$(tput sc)$(default_color setaf 0)$(default_color setab 0).$(default_color sgr0)$(tput rc)"
        # Generate heavy and light horizontal rules based on terminal width.
        HHR=$(printf '═%.0s' $(seq 1 "${COLUMNS:-$(tput cols 2>/dev/null || echo 80)}")) || true
        LHR=$(printf '─%.0s' $(seq 1 "${COLUMNS:-$(tput cols 2>/dev/null || echo 80)}")) || true
    fi

    # Set variables as readonly
    readonly RESET BOLD SMSO RMSO UNDERLINE NO_UNDERLINE BLINK NO_BLINK ITALIC NO_ITALIC MOVE_UP CLEAR_LINE
    readonly FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST FGGLD
    readonly BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST
    readonly DOT HHR LHR

    # Export variables globally
    export RESET BOLD SMSO RMSO UNDERLINE NO_UNDERLINE BLINK NO_BLINK ITALIC NO_ITALIC MOVE_UP CLEAR_LINE
    export FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST FGGLD
    export BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST
    export DOT HHR LHR
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
    if [[ -z "${LOG_PROPERTIES[$LOG_LEVEL]}" ]]; then
        echo -e "ERROR: Invalid LOG_LEVEL '$LOG_LEVEL'. Defaulting to 'INFO'." >&2 && exit 1
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
        ["INFO"]="INFO|${FGGRN}|1"
        ["WARNING"]="WARN|${FGYLW}|2"
        ["ERROR"]="ERROR|${FGRED}|3"
        ["CRITICAL"]="CRIT|${FGMAG}|4"
        ["EXTENDED"]="EXTD|${FGCYN}|0"
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
        organization=$(echo "$url" | sed -E 's#(git@|https://)([^:/]+)[:/]([^/]+)/.*#\3#')
        echo "$organization"
    else
        echo "Error: Not inside a Git repository or no remote URL configured." >&2
        exit 1
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
        echo "$repo_name"
    else
        echo "Error: Not inside a Git repository or no remote URL configured." >&2
        exit 1
    fi
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
        echo "$branch"
    elif [[ "$branch" == "HEAD" ]]; then
        # Handle the detached HEAD state: attempt to determine the source.
        detached_from=$(git reflog show --pretty='%gs' | grep -oE 'checkout: moving from [^ ]+' | head -n 1 | awk '{print $NF}')
        if [[ -n "$detached_from" ]]; then
            echo "Detached from branch: $detached_from"
        else
            echo "Detached HEAD state: Cannot determine the source branch." >&2
            exit 1
        fi
    else
        echo "Error: Not inside a Git repository." >&2
        exit 1
    fi
}

# @brief Get the most recent Git tag.
# @return The most recent Git tag, or nothing if no tags exist.
get_last_tag() {
    local tag

    # Retrieve the most recent Git tag
    tag=$(git describe --tags --abbrev=0 2>/dev/null)

    echo "$tag"
}

# @brief Check if a tag follows semantic versioning.
# @param tag The Git tag to validate.
# @return "true" if the tag follows semantic versioning, otherwise "false".
is_sem_ver() {
    local tag="$1"

    # Validate if the tag follows the semantic versioning format (major.minor.patch)
    if [[ "$tag" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo "true"
    else
        echo "false"
    fi
}

# @brief Get the number of commits since the last tag.
# @param tag The Git tag to count commits from.
# @return The number of commits since the tag, or 0 if the tag does not exist.
get_num_commits() {
    local tag="$1" commit_count

    # Count the number of commits since the given tag
    commit_count=$(git rev-list --count "${tag}..HEAD" 2>/dev/null || echo 0)

    echo "$commit_count"
}

# @brief Get the short hash of the current Git commit.
# @return The short hash of the current Git commit.
get_short_hash() {
    local short_hash

    # Retrieve the short hash of the current Git commit
    short_hash=$(git rev-parse --short HEAD 2>/dev/null)

    echo "$short_hash"
}

# @brief Check if there are uncommitted changes in the working directory.
# @return "true" if there are uncommitted changes, otherwise "false".
get_dirty() {
    local changes

    # Check for uncommitted changes in the repository
    changes=$(git status --porcelain 2>/dev/null)

    if [[ -n "$changes" ]]; then
        echo "true"
    else
        echo "false"
    fi
}

# @brief Generate a version string based on the state of the Git repository.
# @return The generated semantic version string.
get_sem_ver() {
    local branch_name git_tag num_commits short_hash dirty version_string

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

    echo "$version_string"
}

# @brief Configure local or remote mode based on the Git repository context.
#
# This function sets relevant variables for local mode if `USE_LOCAL` is true.
# Defaults to remote configuration if not in local mode.
#
# @global USE_LOCAL           Indicates whether local mode is enabled.
# @global THIS_SCRIPT          Name of the current script.
# @global REPO_ORG            Git organization or owner name.
# @global REPO_NAME           Git repository name.
# @global GIT_BRCH            Current Git branch name.
# @global SEM_VER             Generated semantic version string.
# @global LOCAL_SOURCE_DIR    Path to the root of the local repository.
# @global LOCAL_WWW_DIR       Path to the `data` directory in the repository.
# @global LOCAL_SCRIPTS_DIR   Path to the `scripts` directory in the repository.
# @global GIT_RAW             URL for accessing raw files remotely.
# @global GIT_API             URL for accessing the repository API.
get_proj_params() {
    if [[ "$USE_LOCAL" == "true" ]]; then
        THIS_SCRIPT=$(basename "$0")
        REPO_ORG=$(get_repo_org)
        REPO_NAME=$(get_repo_name)
        GIT_BRCH=$(get_git_branch)
        SEM_VER=$(get_sem_ver)

        # Get the root directory of the repository
        LOCAL_SOURCE_DIR=$(git rev-parse --show-toplevel 2>/dev/null)
        if [[ -z "$LOCAL_SOURCE_DIR" ]]; then
            echo "Error: Not inside a Git repository." >&2
            exit 1
        fi

        LOCAL_WWW_DIR="$LOCAL_SOURCE_DIR/data"
        LOCAL_SCRIPTS_DIR="$LOCAL_SOURCE_DIR/scripts"
    else
        GIT_RAW="https://raw.githubusercontent.com/$REPO_ORG/$REPO_NAME"
        GIT_API="https://api.github.com/repos/$REPO_ORG/$REPO_NAME"
    fi

    export THIS_SCRIPT REPO_ORG REPO_NAME GIT_BRCH SEM_VER LOCAL_SOURCE_DIR
    export LOCAL_WWW_DIR LOCAL_SCRIPTS_DIR GIT_RAW GIT_API
}

############
### Install Functions
############

##
# @brief Display installation instructions or log the start of the installation in non-interactive or terse mode.
# @details Provides an overview of the choices the user will encounter during installation
#          if running interactively and not in terse mode. If non-interactive or terse, skips the printing and logs the start.
#
# @global DOT, BGBLK, FGYLW, FGGRN, HHR, RESET Variables for terminal formatting.
# @global REPO_NAME The name of the package being installed.
# @global TERSE Indicates whether the script is running in terse mode.
#
# @return None Waits for the user to press any key before proceeding if running interactively.
##
start_script() {
    # Declare local variables
    local sp12 sp19 key

    # Add spaces for alignment
    sp12="$(printf ' %.0s' {1..12})"
    sp19="$(printf ' %.0s' {1..19})"

    # Check if the script is non-interactive or in terse mode
    if [ "$TERSE" = "true" ]; then
        logI "$REPO_NAME install beginning."
        return
    fi

    # Clear the screen
    clear

    # Display instructions
    cat << EOF

$DOT$BGBLK$FGYLW$sp12 __          __                          _____ _ $sp19
$DOT$BGBLK$FGYLW$sp12 \ \        / /                         |  __ (_)$sp19
$DOT$BGBLK$FGYLW$sp12  \ \  /\  / /__ _ __  _ __ _ __ _   _  | |__) | $sp19
$DOT$BGBLK$FGYLW$sp12   \ \/  \/ / __| '_ \| '__| '__| | | | |  ___/ |$sp19
$DOT$BGBLK$FGYLW$sp12    \  /\  /\__ \ |_) | |  | |  | |_| | | |   | |$sp19
$DOT$BGBLK$FGYLW$sp12     \/  \/ |___/ .__/|_|  |_|   \__, | |_|   |_|$sp19
$DOT$BGBLK$FGYLW$sp12                | |               __/ |          $sp19
$DOT$BGBLK$FGYLW$sp12                |_|              |___/           $sp19
$DOT$BGBLK$FGGRN$HHR$RESET

You will be presented with some choices during the install. Most frequently
you will see a 'yes or no' choice, with the default choice capitalized like
so: [y/N]. Default means if you hit <enter> without typing anything, you will
make the capitalized choice, i.e. hitting <enter> when you see [Y/n] will
default to 'yes.'

Yes/no choices are not case sensitive. However, passwords, system names, and
install paths are. Be aware of this. There is generally no difference between
'y', 'yes', 'YES', 'Yes', etc.

EOF

    # Wait for user input
    while true; do
        read -n 1 -s -r -p "Press any key when you are ready to proceed or 'Q' to quit. " key < /dev/tty
        echo
        case "$key" in
            [Qq]) 
                logI "Installation canceled by user."
                exit 0
                ;;
            *) 
                break
                ;;
        esac
    done

    # Clear the screen again
    clear
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
    local need_set current_date tz yn

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
        logt "Timezone detected as $tz."
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
    local exec_name="$1"         # The name/message for the operation
    local exec_process="$2"      # The command/process to execute
    local result                 # To store the exit status of the command
    local running_pre="Running:" # Prefix for running message
    local complete_pre="Complete:" # Prefix for success message
    local failed_pre="Failed:"     # Prefix for failure message

    # Log the "Running" message
    logD "$running_pre $exec_name"

    # Print the "[-] Running: $exec_name" message if USE_CONSOLE is false
    if [[ "${USE_CONSOLE}" == "false" ]]; then
        printf "%b[-]%b\t%s %s\n" "$FGGLD" "$RESET" "$running_pre" "$exec_name"
    fi

    # Execute the task command, suppress output, and capture result
    result=$({ eval "$exec_process" > /dev/null 2>&1; echo $?; })

    # Move the cursor up and clear the entire line if USE_CONSOLE is false
    if [[ "${USE_CONSOLE}" == "false" ]]; then
        printf "\033[A\033[2K"
    fi

    # Handle success or failure
    if [ "$result" -eq 0 ]; then
        # Success case
        if [[ "${USE_CONSOLE}" == "false" ]]; then
            printf "\033[1;32m[✔]\033[0m\tCompleted: my_program.sh\n"
        fi
        logI "$complete_pre $exec_name"
        return 0 # Success (true)
    else
        # Failure case
        if [[ "${USE_CONSOLE}" == "false" ]]; then
            printf "\033[1;31m[✘]\033[0m\tFailed to execute my_program.sh (%s)\n" "$result"
        fi
        logE "$failed_pre $exec_name"
        return 1 # Failure (false)
    fi
}

##
# @brief Installs or upgrades all packages in the APTPACKAGES list.
# @details Updates the package list and resolves broken dependencies before proceeding.
#
# @return Logs the success or failure of each operation.
##
apt_packages() {
    # Declare local variables
    local package

    logI "Updating and managing required packages (this may take a few minutes)."

    # Update package list and fix broken installs
    if ! exec_command "Update local package index" "sudo apt-get update -y"; then
        logE "Failed to update package list."
        return 1
    fi

    # Update package list and fix broken installs
    if ! exec_command "Fixing broken or incomplete package installations" "sudo apt-get install -f -y"; then
        logE "Failed to fix broken installs."
        return 1
    fi

    # Install or upgrade each package in the list
    for package in "${APTPACKAGES[@]}"; do
        if dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q "install ok installed"; then
            if ! exec_command "Upgrade $package" "sudo apt-get install --only-upgrade -y $package"; then
                logW "Failed to upgrade package: $package. Continuing with the next package."
            fi
        else
            if ! exec_command "Install $package" "sudo apt-get install -y $package"; then
                logW "Failed to install package: $package. Continuing with the next package."
            fi
        fi
    done

    logI "Package Installation Summary: All operations are complete."
    return 0
}

############
### TODO:  More Installer Functions HEre
############

##
# @brief Check if the snd_bcm2835 module is available and blacklist it if necessary.
# @details Checks if the snd_bcm2835 module is present on the system and if it is not
#          already blacklisted, it adds it to the blacklist and sets the REBOOT flag.
#
# @global REBOOT The flag indicating if a reboot is required.
#
# @return 0 If the module is available and blacklisted (or already blacklisted).
# @return 1 If the module is not available, and processing is skipped.
##
blacklist_snd_bcm2835() {
    # Declare local variables
    local module="snd_bcm2835"
    local blacklist_file="/etc/modprobe.d/alsa-blacklist.conf"

    # Check if the module exists in the kernel
    if ! modinfo "$module" &>/dev/null; then
        return
    fi

    # Check if the module is blacklisted
    if grep -q "blacklist $module" "$blacklist_file" 2>/dev/null; then
        logD "Module $module is already blacklisted in $blacklist_file."
    else
        echo "blacklist $module" | sudo tee -a "$blacklist_file" > /dev/null
        REBOOT="true"
        readonly REBOOT
        export REBOOT

        if [ "$TERSE" = "true" ]; then
            logW "Module $module has been blacklisted. A reboot is required for changes to take effect."
        else
            cat << EOF

*Important Note:*

Wsprry Pi uses the same hardware as your Raspberry Pi's sound
system to generate radio frequencies. This soundcard has been
disabled. You must reboot the Pi after this install completes
for this to take effect:

EOF
            read -rp "Press any key to continue." < /dev/tty
            echo
        fi
    fi

    return 0
}

##
# @brief Finalize the script execution and display completion details.
# @details Displays reboot instructions if required and provides details about the WSPR daemon.
#          Skips the message if the script is running in non-interactive mode or terse mode.
#
# @global DOT, BGBLK, FGYLW, FGGRN, HHR, RESET Variables for terminal formatting.
# @global REBOOT Indicates whether a reboot is required.
# @global SEM_VER The release version of the software.
# @global TERSE Indicates whether the script is running in terse mode.
#
# @return None
##
finish_script() {
    # Declare local variables
    local sp7 sp11 sp18 sp28 sp49 rebootmessage

    if [ "$REBOOT" = "true" ]; then
        if [ "$TERSE" = "true" ]; then
            logW "$REPO_NAME install complete. A reboot is necessary to complete functionality."
        else
            echo "A reboot is required to complete functionality."
            read -rp "Reboot now? [Y/n]: " reboot_choice < /dev/tty
            case "$reboot_choice" in
                [Yy]* | "") 
                    logI "Rebooting the system as requested."
                    sudo reboot
                    ;;
                *)
                    logI "Reboot deferred by the user."
                    ;;
            esac
        fi
        return
    fi

    if [ "$TERSE" = "true" ]; then
        logI "$REPO_NAME install complete."
        return
    fi

    sp7="$(printf ' %.0s' {1..7})"
    sp11="$(printf ' %.0s' {1..11})"
    sp18="$(printf ' %.0s' {1..18})"
    sp28="$(printf ' %.0s' {1..28})"
    sp49="$(printf ' %.0s' {1..49})"

    cat << EOF

$DOT$BGBLK$FGYLW$sp7 ___         _        _ _    ___                _     _$sp18
$DOT$BGBLK$FGYLW$sp7|_ _|_ _  __| |_ __ _| | |  / __|___ _ __  _ __| |___| |_ ___ $sp11
$DOT$BGBLK$FGYLW$sp7 | || ' \(_-<  _/ _\` | | | | (__/ _ \ '  \| '_ \ / -_)  _/ -_)$sp11
$DOT$BGBLK$FGYLW$sp7|___|_|\_/__/\__\__,_|_|_|  \___\___/_|_|_| .__/_\___|\__\___|$sp11
$DOT$BGBLK$FGYLW$sp49|_|$sp28
$DOT$BGBLK$FGGRN$HHR$RESET

The WSPR daemon has started.
 - WSPR frontend URL   : http://$(hostname -I | awk '{print $1}')/wspr
                  -or- : http://$(hostname).local/wspr
 - Release version     : $SEM_VER
$rebootmessage
Happy DXing!
EOF
}

############
### TODO:  More Installer Functions Here
############

############
### Arguments Functions
############

##
# @brief Display usage information and examples for the script.
# @details Provides an overview of the script's available options, their purposes,
#          and practical examples for running the script.
#
# @global THIS_SCRIPT The name of the script, typically derived from the script's filename.
#
# @return None Exits the script with a success code after displaying usage information.
##
usage() {
    cat << EOF
Usage: $THIS_SCRIPT [options]

Options:
  -dr, --dry-run              Enable dry-run mode, where no actions are performed.
                              Useful for testing the script without side effects.
  -v, --version               Display the script version and exit.
  -h, --help                  Display this help message and exit.
  -lf, --log-file <path>      Specify the log file location.
                              Default: <THIS_SCRIPT>.log in the user's home directory,
                              or a temporary file in /tmp if unavailable.
  -ll, --log-level <level>    Set the logging verbosity level.
                              Available levels: DEBUG, INFO, WARNING, ERROR, CRITICAL.
                              Default: DEBUG.
  -tf, --log-to-file <value>  Enable or disable logging to a file explicitly.
                              Options: true, false, unset (auto-detect based on interactivity).
                              Default: unset.
  -t,  --terse <value>        Enable or disable terse output mode.
                              Options: true, false.
                              Default: false.
  -nc, --no-console <value>   Enable or disable console logging explicitly.
                              Options: true, false.
                              Default: false (console logging is enabled).

Environment Variables:
  LOG_FILE                    Specify the log file path. Overrides the default location.
  LOG_LEVEL                   Set the logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL).
  LOG_TO_FILE                 Control file logging (true, false, unset).
  TERSE                       Set to "true" to enable terse output mode.
  USE_CONSOLE                 Set to "true" to enable console logging.

Defaults:
  - If no log file is specified, the log file is created in the user's home
    directory as <THIS_SCRIPT>.log.
  - If the home directory is unavailable or unwritable, a temporary log file
    is created in /tmp.

Examples:
  1. Run the script in dry-run mode:
     $THIS_SCRIPT --dry-run
  2. Check the script version:
     $THIS_SCRIPT --version
  3. Specify a custom log file and log level:
     $THIS_SCRIPT -lf /tmp/example.log -ll INFO
  4. Disable console logging while ensuring logs are written to a file:
     $THIS_SCRIPT -nc true -tf true
  5. Enable terse output mode:
     $THIS_SCRIPT --terse true

EOF

    # Exit with success
    exit 0
}

##
# @brief Parse command-line arguments and set configuration variables.
# @details Processes command-line arguments passed to the script and assigns
#          values to corresponding global variables.
#
# @param[in] "$@" The command-line arguments passed to the script.
#
# Supported options:
# - `--dry-run` or `-dr`: Enable dry-run mode, where no actions are performed.
# - `--version` or `-v`: Display the version information and exit.
# - `--help` or `-h`: Show the usage information and exit.
# - `--log-file` or `-lf <path>`: Specify the log file location.
# - `--log-level` or `-ll <level>`: Set the logging verbosity level.
# - `--log-to-file` or `-tf <value>`: Enable or disable logging to a file explicitly (true/false).
# - `--terse` or `-t <value>`: Enable or disable terse output mode (true/false).
# - `--no-console` or `-nc <value>`: Enable or disable console logging (true/false).
#
# @return
# - Exits with code 0 on success.
# - Exits with code 1 if an invalid argument or option is provided.
#
# @note
# - If both `--log-file` and `--log-to-file` are provided, `--log-file` takes precedence.
#
# @global DRY_RUN            Boolean flag indicating dry-run mode (no actions performed).
# @global LOG_FILE           Path to the log file.
# @global LOG_LEVEL          Logging verbosity level.
# @global LOG_TO_FILE        Boolean or value indicating whether to log to a file.
# @global TERSE              Boolean flag indicating terse output mode.
# @global USE_CONSOLE        Boolean flag indicating console output status.
##
parse_args() {
    local arg  # Iterator for arguments

    # Iterate through the provided arguments
    while [[ "$#" -gt 0 ]]; do
        arg="$1"
        case "$arg" in
            --dry-run|-dr)
                DRY_RUN=true
                ;;
            --version|-v)
                print_version
                exit 0
                ;;
            --help|-h)
                usage
                ;;
            --log-file|-lf)
                # Resolve full path and expand '~'
                LOG_FILE=$(realpath -m "$2" 2>/dev/null)
                if [[ -z "$LOG_FILE" ]]; then
                    echo "ERROR: Invalid path '$2' for --log-file." >&2
                    exit 1
                fi
                LOG_TO_FILE="true"  # Automatically enable logging to file
                shift
                ;;
            --log-level|-ll)
                if [[ -z "$2" || "$2" =~ ^- ]]; then
                    echo "ERROR: Missing argument for $1. Valid options are: DEBUG, INFO, WARNING, ERROR, CRITICAL." >&2
                    exit 1
                fi
                LOG_LEVEL="$2"
                case "$LOG_LEVEL" in
                    DEBUG|INFO|WARNING|ERROR|CRITICAL) ;;  # Valid levels
                    *)
                        echo "ERROR: Invalid log level: $LOG_LEVEL. Valid options are: DEBUG, INFO, WARNING, ERROR, CRITICAL." >&2
                        exit 1
                        ;;
                esac
                shift
                ;;
            --log-to-file|-tf)
                if [[ -z "$2" || "$2" =~ ^- ]]; then
                    echo "ERROR: Missing argument for $1. Valid options are: true, false, unset." >&2
                    exit 1
                fi
                LOG_TO_FILE="$2"
                case "${LOG_TO_FILE,,}" in
                    true|false|unset) ;;  # Valid values
                    *)
                        echo "ERROR: Invalid value for $1: $LOG_TO_FILE. Valid options are: true, false, unset." >&2
                        exit 1
                        ;;
                esac
                shift
                ;;
            --terse|-t)
                if [[ -z "$2" || "$2" =~ ^- ]]; then
                    echo "ERROR: Missing argument for $1. Valid options are: true, false." >&2
                    exit 1
                fi
                TERSE="$2"
                case "${TERSE,,}" in
                    true|false) ;;  # Valid values
                    *)
                        echo "ERROR: Invalid value for $1: $TERSE. Valid options are: true, false." >&2
                        exit 1
                        ;;
                esac
                shift
                ;;
            --log-console|-lc)
                if [[ -z "$2" || "$2" =~ ^- ]]; then
                    echo "ERROR: Missing argument for $1. Valid options are: true, false." >&2
                    exit 1
                fi
                USE_CONSOLE="$2"
                case "${USE_CONSOLE,,}" in
                    true|false) ;;  # Valid values
                    *)
                        echo "ERROR: Invalid value for $1: $USE_CONSOLE. Valid options are: true, false." >&2
                        exit 1
                        ;;
                esac
                shift
                ;;
            -*)
                echo "ERROR: Unknown option '$arg'. Use -h or --help to see available options." >&2
                exit 1
                ;;
            *)
                echo "ERROR: Unexpected argument '$arg'." >&2
                exit 1
                ;;
        esac
        shift
    done

    # Export global variables
    # TODO export DRY_RUN LOG_FILE LOG_LEVEL LOG_TO_FILE TERSE USE_CONSOLE
}

############
### Main Functions
############

# Main function
main() {
    # Check Environment Functions
    check_pipe          # Get fallback name if piped through bash

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

    # More: Check Environment Functions
    check_bash          # Ensure the script is executed in a Bash shell
    check_sh_ver        # Verify the current Bash version meets minimum requirements
    check_bitness       # Validate system bitness compatibility
    check_release       # Check Raspbian OS version compatibility
    check_arch          # Validate Raspberry Pi model compatibility
    check_internet      # Verify internet connectivity if required

    # Print/Display Environment Functions
    print_system        # Log system information
    print_version       # Log the script version

    # Install Functions
}

# Run the main function and exit with its return status
main "$@"
exit $?
