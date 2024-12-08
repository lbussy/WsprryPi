#!/bin/bash
# shellcheck disable=SC2034
#
# This script is designed for Bash (>= 4.x) and uses Bash-specific features.
# It is not intended to be POSIX-compliant.
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

# TODO:
#   - Change the line number in log_message to be the one calling the message
#   - Test string variables in arguments (move this back to log.sh)
#   - Implement variable expansion on log path argument (move this back to log.sh)
#   - Consider implementing DRY_RUN in future work
#   - Find a way to show pending / completed actions (look at Fermentrack), e.g.:
#       - "Start (this command)." message
#       - "(this command) complete." message

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
### Global Skeleton Declarations
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
declare USE_LOCAL="${USE_LOCAL:-false}"
declare GIT_DEF_BRCH=("main" "master")

##
## @brief Flag to disable console logging.
##
## Possible values:
## - "true": Disables logging to the terminal.
## - "false": Enables logging to the terminal (default).
##
declare NO_CONSOLE="${NO_CONSOLE:-true}" # TODO: Disable console logging

##
# @brief Configuration constants for script requirements and compatibility.
# @details Defines requirements for running the script, including root privileges,
# supported Bash version, OS versions, and system bitness.
##

# Require Internet to run the script
readonly REQUIRE_INTERNET="${REQUIRE_INTERNET:-true}"  # Set to true if the script requires root privileges

# Require root privileges to run the script
# Set to true if the script requires root privileges
readonly REQUIRE_SUDO="${REQUIRE_SUDO:-false}"   # Default to false if not specified

# Minimum supported Bash version (set to "none" to disable version checks)
readonly MIN_BASH_VERSION="${MIN_BASH_VERSION:-4.0}"  # Default to "4.0" if not specified

# Supported OS versions (minimum and maximum)
readonly MIN_OS=11       # Minimum supported OS version
readonly MAX_OS=15       # Maximum supported OS version (use -1 for no upper limit)

# Supported system bitness
readonly SUPPORTED_BITNESS="32"  # Supported bitness ("32", "64", or "both")

# Declare an associative array to hold Raspberry Pi models and their support statuses.
# Key: Model identifier (name and revision codes)
# Value: Support status ("Supported" or "Not Supported")
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

# Global variable to control file logging
# LOG_TO_FILE options:
# - "true": Always log to the file
# - "false": Never log to the file
# - unset: Follow logic defined in the is_interactive() function
declare LOG_TO_FILE="${LOG_TO_FILE:-}"    # Default to blank if not set
# Logging configuration
declare LOG_FILE="${LOG_FILE:-}"          # Use the provided LOG_FILE or default to blank
declare LOG_LEVEL="${LOG_LEVEL:-DEBUG}"   # Default log level is DEBUG if not set

# Required packages
readonly APTPACKAGES="apache2 php jq libraspberrypi-dev raspberrypi-kernel-headers"

# List of required external dependencies for skeleton
declare DEPENDENCIES=("awk" "grep" "tput" "cut" "tr" "getconf" "cat" "sed")
# List of required external dependencies for logging
declare DEPENDENCIES+=("getent" "date" "mktemp" "printf" "whoami" "realpath")
# List of required external dependencies for installer
declare DEPENDENCIES+=("dpkg" "git" "dpkg-reconfigure" "curl")
readonly DEPENDENCIES

############
### Skeleton Functions
############

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
    # Skip check if REQUIRE_INTERNET is not true
    if [ "$REQUIRE_INTERNET" != "true" ]; then
        return
    fi

    # Check for internet connectivity using curl
    if curl -s --head http://google.com | grep "HTTP/1\.[01] [23].." > /dev/null; then
        logD "Internet is available."
    else
        die 1 "No Internet connection detected."
    fi
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
# @global THIS_SCRIPT The name of the script being executed.
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
    local script="$THIS_SCRIPT"     # Script name

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
# @global THIS_SCRIPT Script name.
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

##
# @brief Ensure the script is run with appropriate privileges based on REQUIRE_SUDO.
# @details Validates whether the script is executed under the correct privilege conditions:
#          - If REQUIRE_SUDO is true, the script must be run with `sudo` and not directly as root.
#          - If REQUIRE_SUDO is false, the script must not be run as root or with `sudo`.
#
# @global REQUIRE_SUDO A boolean indicating whether the script requires `sudo` privileges.
# @global EUID The effective user ID of the user running the script.
# @global die Function to handle critical errors and terminate the script.
#
# @return None Exits the script with an error if the privilege requirements are not met.
##
enforce_sudo() {
    # Check if the script requires sudo privileges
    if [[ "$REQUIRE_SUDO" == true ]]; then
        # Ensure the user is not directly logged in as root
        if [[ "$EUID" -eq 0 && -z "$SUDO_USER" ]]; then
            die 1 "This script requires 'sudo' privileges but should not be run as the root user directly."
        fi

        # Ensure the user is running the script with sudo
        if [[ -z "$SUDO_USER" ]]; then
            die 1 "This script requires 'sudo' privileges. Please re-run using 'sudo'."
        fi
    else
        # Ensure the script is not being run as root
        if [[ "$EUID" -eq 0 ]]; then
            die 1 "This script should not be run as root. Avoid using 'sudo'."
        fi
    fi
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
    local detected_model is_supported key full_name model chip  # Local variables

    # Attempt to read and process the compatible string
    if ! detected_model=$(cat /proc/device-tree/compatible 2>/dev/null | tr '\0' '\n' | grep "raspberrypi" | sed 's/raspberrypi,//'); then
        die 1 "Failed to read or process /proc/device-tree/compatible." \
            "Ensure the script is run on a compatible Raspberry Pi device."
    fi

    # Check if the detected model is empty
    if [[ -z "$detected_model" ]]; then
        die 1 "No Raspberry Pi model found in /proc/device-tree/compatible." \
            "This system may not be supported."
    fi

    # Initialize is_supported flag
    is_supported=false

    # Iterate through supported models to check compatibility
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

    # Log an error if no supported model was found
    if [[ "$is_supported" == false ]]; then
        die 1 "Detected Raspberry Pi model '$detected_model' is not recognized or supported."
    fi
}


##
# @brief Check Raspbian OS version compatibility.
# @details This function ensures that the Raspbian version is within the supported range
# and logs an error if the compatibility check fails.
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
        die 1 "Unable to read /etc/os-release." \
            "Ensure this script is run on a compatible system with a readable /etc/os-release file."
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
# @brief Check system bitness compatibility.
# @details Validates whether the current system's bitness matches the supported configuration.
#          - Supported values for SUPPORTED_BITNESS are "32", "64", or "both".
#          - Logs an error and exits if the system's bitness is unsupported or misconfigured.
#
# @global SUPPORTED_BITNESS Specifies the bitness supported by the script ("32", "64", or "both").
# @global die Function to handle critical errors and terminate the script.
# @global log_message Function to log informational messages.
#
# @return None Exits the script with an error if the system bitness is unsupported.
##
check_bitness() {
    local bitness  # Variable to store the detected system bitness

    # Retrieve the system's bitness
    bitness=$(getconf LONG_BIT)

    # Validate the system's bitness against the supported configuration
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
            logD "Detected $bitness-bit system, which is supported."
            ;;
        *)
            die 1 "Configuration error: Invalid value for SUPPORTED_BITNESS ('$SUPPORTED_BITNESS')."
            ;;
    esac
}

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
  -dr, --dry-run              Simulate actions without making changes.
                              Useful for testing the script without side effects.
  -v, --version               Display the script version and exit.
  -h, --help                  Display this help message and exit.
  -lf, --log-file <path>      Specify the log file location.
                              When set, implies --log-to-file true.
  -ll, --log-level <level>    Set the log level (DEBUG, INFO, WARNING, ERROR, CRITICAL).
  -tf, --log-to-file <value>  Enable or disable logging to a file explicitly (true/false).
                              If --log-file is set, this option is ignored.
  -l, --local                 Enable local mode for installation from a local git repository.
  -t, --terse                 Enable terse mode. Reduces output verbosity for a quieter run.

Environment Variables:
  USE_LOCAL                   Enable local installation mode (equivalent to --local).
  LOG_FILE                    Path to the log file. Overrides default logging behavior.
  LOG_LEVEL                   Log level (DEBUG, INFO, WARNING, ERROR, CRITICAL).
  LOG_TO_FILE                 Whether to log to a file (true, false, or unset).
  TERSE                       Enable terse mode (equivalent to --terse).

Examples:
  1. Run the script in dry-run mode:
     $THIS_SCRIPT --dry-run
  2. Check the script version:
     $THIS_SCRIPT --version
  3. Log to /tmp/example.log at INFO level and explicitly log to file:
     $THIS_SCRIPT -lf /tmp/example.log -ll INFO -tf true
  4. Enable local installation mode:
     $THIS_SCRIPT --local
  5. Run the script in terse mode:
     $THIS_SCRIPT --terse

EOF

    # Exit with success
    exit 0
}

##
# @brief Print the system information to the log.
# @details Extracts and logs the system's name and version using information
#          from `/etc/os-release`.
#
# @global None
# @return None
##
print_system() {
    # Extract system name and version from /etc/os-release
    local system_name
    system_name=$(grep '^PRETTY_NAME=' /etc/os-release 2>/dev/null | cut -d '=' -f2 | tr -d '"')

    # Check if system_name is empty
    if [[ -z "$system_name" ]]; then
        logW "System: Unknown (could not extract system information)."
    else
        logD "System: $system_name."
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
    # Check the name of the calling function
    local caller="${FUNCNAME[1]}"

    if [[ "$caller" == "parse_args" ]]; then
        echo -e "$THIS_SCRIPT: version $SEM_VER" # Display the script name and version
    else
        logD "Running $THIS_SCRIPT version $SEM_VER"
    fi
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
# - `--log-file` or `-lf <path>`: Specify the path to the log file. Automatically enables logging to a file.
# - `--log-level` or `-ll <level>`: Set the logging verbosity level.
# - `--log-to-file` or `-tf <value>`: Enable or disable logging to a file explicitly (true/false).
# - `--local` or `-l`: Enable local mode.
# - `--terse` or `-t`: Enable terse mode, reducing output verbosity.
#
# @return
# - Exits with code 0 on success.
# - Exits with code 1 if an invalid argument or option is provided.
#
# @note
# - If both `--log-file` and `--log-to-file` are provided, `--log-file` takes precedence.
# - Paths provided to `--log-file` are resolved to their absolute forms.
#
# @global DRY_RUN            Boolean flag indicating dry-run mode (no actions performed).
# @global LOG_FILE           Path to the log file.
# @global LOG_LEVEL          Logging verbosity level.
# @global LOG_TO_FILE        Boolean or value indicating whether to log to a file.
# @global USE_LOCAL          Boolean flag indicating whether local mode is enabled.
# @global TERSE              Boolean flag indicating whether terse mode is enabled.
#
# @return None Exits with an error if invalid arguments or options are provided.
##
parse_args() {
    # Local variable declarations
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
                exit 0
                ;;
            --log-file|-lf)
                if [[ -n "$2" ]]; then
                    # Resolve full path and expand '~'
                    LOG_FILE=$(realpath -m "$2" 2>/dev/null)
                    if [[ -z "$LOG_FILE" ]]; then
                        echo "ERROR: Invalid path '$2' for --log-file." >&2
                        exit 1
                    fi
                    LOG_TO_FILE="true"  # Automatically enable logging to file
                    shift 2
                else
                    echo "ERROR: --log-file requires a file path argument." >&2
                    exit 1
                fi
                ;;
            --log-level|-ll)
                if [[ -n "$2" ]]; then
                    LOG_LEVEL="$2"
                    shift 2
                else
                    echo "ERROR: --log-level requires a level argument." >&2
                    exit 1
                fi
                ;;
            --log-to-file|-tf)
                if [[ -n "$LOG_FILE" ]]; then
                    # Skip processing --log-to-file if --log-file is set
                    echo "INFO: Ignoring --log-to-file because --log-file is set." >&2
                else
                    if [[ -n "$2" ]]; then
                        LOG_TO_FILE="$2"
                        shift 2
                    else
                        echo "ERROR: --log-to-file requires a value (e.g., true/false)." >&2
                        exit 1
                    fi
                fi
                ;;
            --local|-l)
                USE_LOCAL="true"
                ;;
            --terse|-t)
                TERSE="true"
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

    # Export and make relevant global variables readonly
    readonly DRY_RUN LOG_LEVEL LOG_TO_FILE TERSE # USE_LOCAL
    export DRY_RUN LOG_LEVEL LOG_TO_FILE TERSE USE_LOCAL
}

##
# @brief Check for required dependencies and report any missing ones.
# @details Iterates through the dependencies listed in the global array `DEPENDENCIES`,
# checking if each one is installed. Logs missing dependencies and exits the script
# with an error code if any are missing.
#
# @global DEPENDENCIES Array of required dependencies.
# @global log_message Function to log messages at various severity levels.
# @global die Function to handle critical errors and exit.
#
# @return None (exits the script if dependencies are missing).
##
validate_depends() {
    local missing=0  # Counter for missing dependencies
    local dep        # Iterator for dependencies

    for dep in "${DEPENDENCIES[@]}"; do
        if ! command -v "$dep" &>/dev/null; then
            echo -e "ERROR" "Missing dependency: $dep" in ${FUNCNAME[0]} # Log the missing dependency
            ((missing++)) # Increment the missing counter
        fi
    done

    if ((missing > 0)); then
        echo -e "ERROR: Missing $missing dependencies in ${FUNCNAME[0]}. Install them and re-run the script."
        exit 1
    fi
}

##
# @brief Check if the current Bash version meets the minimum required version.
# @details Compares the current Bash version against a required version specified
# in the global variable `MIN_BASH_VERSION`. If `MIN_BASH_VERSION` is set to "none",
# the check is skipped.
#
# @global MIN_BASH_VERSION Specifies the minimum required Bash version (e.g., "4.0") or "none".
# @global BASH_VERSINFO Array containing the major and minor versions of the running Bash.
# @global die Function to handle fatal errors and exit the script.
#
# @return None (exits the script if the Bash version is insufficient, unless the check is disabled).
##
check_sh_ver() {
    # Ensure MIN_BASH_VERSION is defined; default to "none"
    local required_version="${MIN_BASH_VERSION:-none}"

    # Skip the check if the minimum version is set to "none"
    if [[ "$required_version" == "none" ]]; then
        logI "Bash version check is disabled (MIN_BASH_VERSION='none')."
        return
    fi

    # Compare the current Bash version against the required version
    if ((BASH_VERSINFO[0] < ${required_version%%.*} || 
         (BASH_VERSINFO[0] == ${required_version%%.*} && 
          BASH_VERSINFO[1] < ${required_version##*.}))); then
        die 1 "This script requires Bash version $required_version or newer."
    fi

    logD "Bash version check passed. Running Bash version: ${BASH_VERSINFO[0]}.${BASH_VERSINFO[1]}."
}

##
# @brief Check if the script is running in a Bash shell.
# @details Ensures the script is executed with Bash, as it may use Bash-specific features.
# Logs an error message and exits if not running in Bash.
#
# @global BASH_VERSION The version of the Bash shell being used.
#
# @return None (exits the script if not running in Bash).
##
check_bash() {
    if [ -z "$BASH_VERSION" ]; then
        echo -e "ERROR: This script requires Bash. Please run it with Bash." >&2
        exit 1
    fi
}

##
# @brief Toggle the NO_CONSOLE variable on or off.
#
# This function updates the global NO_CONSOLE variable to either "true" (off)
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
            NO_CONSOLE="false"
            ;;
        off)
            NO_CONSOLE="true"
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
    else
        if ! is_interactive; then
            should_log_to_file=true
        else
            should_log_to_file=false
        fi
    fi

    # Log to file if applicable
    if "$should_log_to_file"; then
        printf "[%s]\t[%s]\t[%s:%d]\t%s\n" "$timestamp" "$level" "$THIS_SCRIPT" "$lineno" "$message" >> "$LOG_FILE"
        [[ -n "$details" ]] && printf "[%s]\t[%s]\t[%s:%d]\tDetails: %s\n" "$timestamp" "$level" "$THIS_SCRIPT" "$lineno" "$details" >> "$LOG_FILE"
    fi

    # Always print to the terminal if in an interactive shell
    if is_interactive; then
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
# @brief Determine if the script is running in an interactive shell.
#
# This function checks if the script is connected to a terminal by testing
# whether standard input and output (file descriptor 1 & 0) is a terminal.
#
# @return 0 (true) if the script is running interactively; non-zero otherwise.
##
is_interactive() {
    [[ -t 1 ]] && [[ -t 0 ]]
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
    if is_interactive && [ "$tput_colors_available" -ge 8 ]; then
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
        HHR="$(printf '═%.0s' $(seq 1 "${COLUMNS:-$(tput cols)}"))"
        LHR="$(printf '─%.0s' $(seq 1 "${COLUMNS:-$(tput cols)}"))"
    else
        # Fallback for unsupported or non-interactive terminals
        RESET=""; BOLD=""; SMSO=""; RMSO=""; UNDERLINE=""
        NO_UNDERLINE=""; BLINK=""; NO_BLINK=""; ITALIC=""; NO_ITALIC=""
        FGBLK=""; FGRED=""; FGGRN=""; FGYLW=""; FGBLU=""
        FGMAG=""; FGCYN=""; FGWHT=""; FGRST=""
        BGBLK=""; BGRED=""; BGGRN=""; BGYLW=""
        BGBLU=""; BGMAG=""; BGCYN=""; BGWHT=""; BGRST=""
        DOT=""; HHR=""; LHR=""
    fi

    # Set variables as readonly
    readonly RESET BOLD SMSO RMSO UNDERLINE NO_UNDERLINE BLINK NO_BLINK
    readonly ITALIC NO_ITALIC FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST
    readonly BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST
    readonly DOT HHR LHR

    # Export variables globally
    export RESET BOLD SMSO RMSO UNDERLINE NO_UNDERLINE BLINK NO_BLINK
    export ITALIC NO_ITALIC FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST
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

############
### Install Functions
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
# @return Prints the current branch name or detached source to standard output.
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
display_start() {
    # Check if the script is non-interactive or in terse mode
    if ! is_interactive || [ "$TERSE" = "true" ]; then
        logI "$REPO_NAME install beginning."
        return
    fi

    # Local variable declarations
    local sp12 sp19 key

    # Add spaces for alignment
    sp12="$(printf ' %.0s' {1..12})"
    sp19="$(printf ' %.0s' {1..19})"

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

Yes/no choices are not case sensitive. However; passwords, system names and
install paths are. Be aware of this. There is generally no difference between
'y', 'yes', 'YES', 'Yes', etc.

EOF

    # Wait for user input
    while true; do
        read -n 1 -s -r -p "Press any key when you are ready to proceed or 'Q' to quit. " key < /dev/tty
        echo # Move to the next line
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
        # TODO: Add a terse mode logger
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
                break  # User confirms timezone is correct
                ;;
            [Nn]* | *) 
                dpkg-reconfigure tzdata  # Reconfigure timezone
                logI "Timezone reconfigured on $current_date"
                break
                ;;
        esac
    done
}

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
        return # Skip further processing if the module is not available
    fi

    # Check if the module is blacklisted
    if grep -q "blacklist $module" "$blacklist_file" 2>/dev/null; then
        logD "Module $module is already blacklisted in $blacklist_file."
    else
        # Blacklist the module if it is not already blacklisted
        echo "blacklist $module" | sudo tee -a "$blacklist_file" > /dev/null
        REBOOT="true"
        readonly REBOOT  # Mark REBOOT as readonly
        export REBOOT    # Export REBOOT to make it available to child processes
        
        # If TERSE mode is enabled, show a short warning
        if [ "$TERSE" = "true" ]; then
            logW "Module $module has been blacklisted. A reboot is required for changes to take effect."
        else
            # Display detailed message for non-terse mode
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
# @brief Executes a command silently and logs results.
#
# @param[in] command Command to execute.
#
# @return Returns 0 for success, 1 for failure.
##
run_command() {
    local command="$1"

    if eval "$command" &>/dev/null; then
        return 0
    else
        return 1
    fi
}

##
# @brief Installs or upgrades all packages in the APTPACKAGES list.
#
# @details
#   Updates the package list and resolves broken dependencies before proceeding.
#
# @return Logs the success or failure of each operation.
##
apt_packages() {
    local package

    logI "Updating local apt cache."

    # Update package list and fix broken installs
    logI "Updating and managing required packages (this may take a few minutes)."
    if ! run_command "sudo apt-get update -y && sudo apt-get install -f -y"; then
        logE "Failed to update package list or fix broken installs."
        return 1
    fi

    # Install or upgrade each package in the list
    for package in "${APTPACKAGES[@]}"; do
        if dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q "install ok installed"; then
            if ! run_command "sudo apt-get install --only-upgrade -y $package"; then
                logW "Failed to upgrade package: $package. Continuing with the next package."
            fi
        else
            if ! run_command "sudo apt-get install -y $package"; then
                logW "Failed to install package: $package. Continuing with the next package."
            fi
        fi
    done

    logI "Package Installation Summary: All operations are complete."
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

    # Check for REBOOT first, and then handle TERSE condition
    if [ "$REBOOT" = "false" ]; then
        if ! is_interactive || [ "$TERSE" = "true" ]; then
            logI "$REPO_NAME install complete."
            return
        fi
    elif [ "$REBOOT" = "true" ]; then
        if ! is_interactive; then
            logW "$REPO_NAME install complete. A reboot is necessary to complete functionality."
            return
        elif [ "$TERSE" = "true" ]; then
            logW "$REPO_NAME install complete. A reboot is necessary to complete functionality."
            
            # Skip prompting for a reboot in terse mode and just return
            return
        else
            # Prompt for reboot in non-terse mode
            echo "A reboot is required to complete functionality."
            read -rp "Reboot now? [Y/n]: " reboot_choice < /dev/tty
            case "$reboot_choice" in
                [Yy]* | "")  # Default to yes
                    logI "Rebooting the system as requested."
                    sudo reboot
                    ;;
                *)
                    logI "Reboot deferred by the user."
                    ;;
            esac
            return
        fi
    fi  # End of REBOOT check

    # Final check for TERSE and return if true
    if [ "$TERSE" = "true" ]; then
        logI "$REPO_NAME install complete."
        return
    fi

    # Add spaces for formatting
    sp7="$(printf ' %.0s' {1..7})"
    sp11="$(printf ' %.0s' {1..11})"
    sp18="$(printf ' %.0s' {1..18})"
    sp28="$(printf ' %.0s' {1..28})"
    sp49="$(printf ' %.0s' {1..49})"

    # Display the completion message
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
### Main Functions
############

# Main function
main() {
    # Perform essential checks
    parse_args "$@"     # Parse any command line arguments
    validate_depends    # Check availability of req. non-Bash builtins
    get_proj_params     # Get project and git parameters
    setup_log           # Set up logging
    check_bash          # Validate we are using Bash
    check_sh_ver        # Validate bash version
    check_bitness       # Validate bitness requirements
    check_release       # Validate release requirements
    check_arch          # Validate architecture requirements
    enforce_sudo        # Validate we have permissions to run
    check_internet      # Validate Internet access

    # Informational/debug lines
    print_system        # Log system info
    print_version       # Log script version

    # Script start
    display_start   
    set_time
    apt_packages

    # do_service "wspr" "" "/usr/local/bin" # Install/upgrade wspr daemon
    # do_shutdown_button "shutdown_watch" "py" "/usr/local/bin" # Handle TAPR shutdown button
    # do_www "/var/www/html/wspr" "$LOCAL_WWW_DIR" # Download website
    # do_apache_setup

    blacklist_snd_bcm2835

    # Script complete
    finish_script
}

# Run the main function and exit with its return status
main "$@"
exit $?
