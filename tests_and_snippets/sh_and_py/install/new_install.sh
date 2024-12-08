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

##
# @brief Trap unexpected errors during script execution.
# @details Captures any errors (via the ERR signal) that occur during script execution.
# Logs the function name and line number where the error occurred and exits the script.
# The trap calls an error-handling function for better flexibility.
#
# @global FUNCNAME Array containing function names in the call stack.
# @global LINENO Line number where the error occurred.
# @global SCRIPT_NAME Name of the script.
#
# @return None (exits the script with an error code).
##
# shellcheck disable=SC2329
trap_error() {
    local func="${FUNCNAME[1]:-main}"               # Get the calling function name (default: "main")
    local line="$1"                                 # Line number where the error occurred
    local script="${SCRIPT_NAME:-$(basename "$0")}" # Script name (fallback to current script)

    # Log the error message and exit
    echo "ERROR: An unexpected error occurred in function '$func' at line $line of script '$script'. Exiting." >&2
    exit 1
}

# Set the trap to call trap_error on any error
# Uncomment the next line to help with script debugging
# trap 'trap_error "$LINENO"' ERR

############
### Global Declarations
############

##
# @brief Project metadata constants used throughout the script.
# @details These variables provide metadata about the script, including ownership,
#          versioning, and project details. All constants are marked as read-only.
#
# @global readonly COPYRIGHT A copyright notice for the script.
# @global readonly PACKAGE The short project package name.
# @global readonly PACKAGENAME The formatted project package name.
# @global readonly OWNER The project owner or maintainer.
# @global readonly VERSION The current version of the script.
# @global readonly GIT_BRCH The current Git branch.
# @global readonly FALLBACK_NAME The default script name if the script is piped.
##

# TODO: Make sure we use this constant:
readonly COPYRIGHT="Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)" # Copyright notice
# TODO: Make sure we use this constant:
readonly PACKAGE="${PACKAGE:-WsprryPi}"                             # Project package name (short, default: WsprryPi)
# TODO: Make sure we use this constant:
readonly PACKAGENAME="Wsprry Pi"                                    # Project package name (formatted)
# TODO: Make sure we use this constant:
readonly OWNER="${OWNER:-lbussy}"                                   # Project owner or maintainer (default: lbussy)
# Current version of the script
readonly VERSION="1.2.1-version-files+91.3bef855-dirty"
# TODO: Make sure we use this constant:
readonly GIT_BRCH="${GIT_BRCH:-version_file}"                       # Git branch (default: version_file)
readonly FALLBACK_NAME="${FALLBACK_NAME:-install.sh}"               # Default fallback name if the script is piped

##
# @brief Flag to control verbose or terse output in the script.
# @details This variable determines whether the script produces terse (minimal) or verbose output.
#          Defaults to "true" for terse output unless explicitly overridden.
#
# @global TERSE This variable controls the verbosity of the script's output.
#
# @default true
##
declare TERSE="${TERSE:-true}" # Use existing value, or default to "true".

##
# @brief Logging-related constants for the script.
# @details Sets the script name (`SCRIPT_NAME`) based on the current environment.
# If `SCRIPT_NAME` is already defined, its value is retained; otherwise, it is set
# to the basename of the script.
#
# If the script is piped through bash (as in when it is curled) it will change
# to FALLBACK_NAME via check_pipe().
#
# @global SCRIPT_NAME The name of the script.
##
declare SCRIPT_NAME="${SCRIPT_NAME:-$(basename "$0")}"  # Use existing value, or default to script basename.

##
# @brief Configuration constants for script requirements and compatibility.
# @details Defines requirements for running the script, including root privileges,
# supported Bash version, OS versions, and system bitness.
##

##
# @brief Require root privileges to run the script.
# @details Use `true` if the script requires root privileges.
##
readonly REQUIRE_SUDO=true  # Set to true if the script requires root privileges

##
# @brief Minimum supported Bash version.
# @details Set to "none" to disable version checks.
##
readonly MIN_BASH_VERSION="${MIN_BASH_VERSION:-4.0}"  # Default to "4.0" if not specified

##
# @brief Supported OS versions (minimum and maximum).
# @details Defines required OS levels (Debian-specific).
##
readonly MIN_OS=11       # Minimum supported OS version
readonly MAX_OS=15       # Maximum supported OS version (use -1 for no upper limit)

##
# @brief Supported system bitness.
# @details Defines appropriate bitness for script execution.
##
readonly SUPPORTED_BITNESS="32"  # Supported bitness ("32", "64", or "both")

##
# @brief Raspberry Pi model compatibility map.
# @details Associative array mapping Raspberry Pi models to their support statuses.
##
declare -A SUPPORTED_MODELS=(
    # Unsupported Models
    ["Raspberry Pi 5|5-model-b|bcm2712"]="Not Supported"
    ["Raspberry Pi 400|400|bcm2711"]="Not Supported"
    ["Raspberry Pi Compute Module 4|4-compute-module|bcm2711"]="Not Supported"
    ["Raspberry Pi Compute Module 3|3-compute-module|bcm2837"]="Not Supported"
    ["Raspberry Pi Compute Module|compute-module|bcm2835"]="Not Supported"

    # Supported Models
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
readonly SUPPORTED_MODELS

##
# @brief Required dependencies.
#
# @details
# This section lists the external commands required by the script for proper execution.
# The listed dependencies must be available in the system's PATH for the script to function
# as intended. Missing dependencies may cause runtime errors.
#
# @var DEPENDENCIES
# @type array
# @default
#
# @note
# Ensure all required commands are included in this list. Use a dependency-checking
# function to verify their availability at runtime.
#
# @todo Check this list for completeness and update as needed.
##
declare DEPENDENCIES=(
    "awk" "grep" "tput" "cut" "tr" "getconf" "cat" "sed" "basename"
    "getent" "date" "mktemp" "printf" "whoami" "mkdir" "touch" "echo"
    "dpkg" "git" "dpkg-reconfigure" "curl" "realpath"
)

##
# @brief Array of required environment variables.
#
# This array specifies the environment variables that the script requires
# to function correctly. The `validate_env_vars` function checks for their
# presence during initialization.
#
# Environment Variables:
#   - SUDO_USER: Identifies the user who invoked the script using sudo.
#   - HOME: Specifies the home directory of the current user.
#   - COLUMNS: Defines the width of the terminal, used for formatting.
##
# Initialize the ENV_VARS array
declare -a ENV_VARS_BASE=(
    "HOME"       # Home directory of the current user
    "COLUMNS"    # Terminal width for formatting
)

# Conditionally add SUDO_USER if REQUIRE_SUDO is true
if [[ "$REQUIRE_SUDO" == true ]]; then
    readonly -a ENV_VARS=("${ENV_VARS_BASE[@]}" "SUDO_USER")
else
    readonly -a ENV_VARS=("${ENV_VARS_BASE[@]}")
fi

##
# @brief Set a default value for terminal width if COLUMNS is unset.
#
# The `COLUMNS` variable specifies the width of the terminal in columns.
# If not already set, this line assigns a default value of 80 columns.
# This ensures the script functions correctly in non-interactive environments
# where `COLUMNS` might not be automatically defined.
#
# Environment Variable:
#   COLUMNS - Represents the terminal width. Can be overridden externally.
##
COLUMNS="${COLUMNS:-80}"  # Default to 80 columns if unset

##
# @brief Array of critical system files to check for availability.
# @details These files must exist and be readable for the script to function properly.
##
declare SYSTEM_READS+=(
    "/etc/os-release"
    "/proc/device-tree/compatible"
    "/etc/modprobe.d/alsa-blacklist.conf"
)

##
# @brief Flag to indicate if internet connectivity is required.
#
# This variable determines whether the script should verify internet connectivity
# before proceeding. The default value is `false`, meaning the script does not
# require internet unless explicitly set to `true`.
#
# Possible values:
# - "true": Internet connectivity is required.
# - "false": Internet connectivity is not required (default).
#
# Environment Variable:
#   REQUIRE_INTERNET - Overrides this value if set before script execution.
##
readonly REQUIRE_INTERNET="${REQUIRE_INTERNET:-true}"  # Default to false if not set

##
# @brief Controls whether stack traces are printed for warning messages.
# @details Set to `true` to enable stack trace logging for warnings.
##
readonly WARN_STACK_TRACE="${WARN_STACK_TRACE:-false}"  # Default to false if not set

##
# @brief List of required APT packages for the script.
# @details Defines the packages required for the script's proper functionality.
#          Packages should be available in the system's default package manager.
#
# @type array
# @global readonly APTPACKAGES
##
declare -a APTPACKAGES=("apache2" "php" "jq" "libraspberrypi-dev" "raspberrypi-kernel-headers")
readonly APTPACKAGES

############
### Logging Declarations
############

##
## @brief Path to the log file.
##
## Specifies where logs will be stored. If unset, no logging to file is performed.
##
declare LOG_FILE="${LOG_FILE:-}"

##
## @brief Specifies the logging verbosity level.
##
## Default value is "DEBUG" unless overridden by an external variable.
##
declare LOG_LEVEL="${LOG_LEVEL:-DEBUG}"

##
## @brief Flag to disable console logging.
##
## Possible values:
## - "true": Disables logging to the terminal.
## - "false": Enables logging to the terminal (default).
##
declare NO_CONSOLE="${NO_CONSOLE:-true}"

##
## @brief Controls whether logs are written to a file.
##
## Possible values:
## - "true": Always log to the file.
## - "false": Never log to the file.
## - unset: Follow the logic defined in the `is_interactive()` function.
##
declare LOG_TO_FILE="${LOG_TO_FILE:-true}"

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
check_bash_version() {
    # Ensure MIN_BASH_VERSION is defined; default to "none"
    local required_version="${MIN_BASH_VERSION:-none}"

    # Skip the check if the minimum version is set to "none"
    if [[ "$required_version" == "none" ]]; then
        logD "Bash version check is disabled (MIN_BASH_VERSION='none')."
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
# @brief Enforce that the script is run directly with `sudo`.
#
# This function ensures the script is executed with `sudo` privileges and not:
# - From a `sudo su` shell.
# - As the root user directly (e.g., logged in as root).
#
# @returns None
# @exit Exits with status 1 if the script is not executed correctly.
##
enforce_sudo() {
    if [[ "$EUID" -eq 0 && -n "$SUDO_USER" && "$SUDO_COMMAND" == *"$0"* ]]; then
        # The script was invoked directly with `sudo`
        return
    elif [[ "$EUID" -eq 0 && -n "$SUDO_USER" ]]; then
        # The script is running as root, but within a `sudo su` shell
        die 1 "This script requires 'sudo' privileges but should not be run from a 'sudo su' shell." \
              "Please run it directly using 'sudo scriptname'."
    elif [[ "$EUID" -eq 0 ]]; then
        # The script is running as root directly (e.g., logged in as root)
        die 1 "This script requires 'sudo' privileges but should not be run as the root user directly." \
              "Please run it directly using 'sudo scriptname'."
    else
        # The script is not running with sufficient privileges
        die 1 "This script requires 'sudo' privileges." \
              "Please re-run it using 'sudo scriptname'."
    fi
}

##
# @brief Check if the detected Raspberry Pi model is supported.
# @details Reads the Raspberry Pi model from /proc/device-tree/compatible and checks
#          it against a predefined list of supported models. Logs an error if the
#          model is unsupported or cannot be detected.
#
# @global SUPPORTED_MODELS Associative array of supported and unsupported Raspberry Pi models.
# @global logD Function for detailed logging messages.
# @global die Function for handling critical errors and terminating the script.
#
# @return None Exits the script with an error code if the architecture is unsupported.
##
check_architecture() {
    local detected_model is_supported key full_name model chip  # Declare local variables

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
# @details Ensures that the Raspbian version is within the supported range defined by
#          `MIN_OS` and `MAX_OS`. Logs an error if the compatibility check fails.
#
# @global MIN_OS Minimum supported OS version.
# @global MAX_OS Maximum supported OS version (-1 indicates no upper limit).
# @global logD Function for logging detailed messages.
# @global die Function for handling critical errors and terminating the script.
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
    if ! ver=$(grep "VERSION_ID" /etc/os-release | awk -F "=" '{print $2}' | tr -d '"'); then
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
    logD "Raspbian version $ver is within the supported range ($MIN_OS - $MAX_OS)."
}

##
# @brief Check system bitness compatibility.
# @details Validates whether the current system's bitness matches the supported configuration.
#          - Supported values for SUPPORTED_BITNESS are "32", "64", or "both".
#          - Logs an error and exits if the system's bitness is unsupported or misconfigured.
#
# @global SUPPORTED_BITNESS Specifies the bitness supported by the script ("32", "64", or "both").
# @global logD Function to log informational or debug messages.
# @global die Function to handle critical errors and terminate the script.
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
            die 1 "Unsupported system: Only 32-bit systems are supported. Detected $bitness-bit system."
            fi
            ;;
        "64")
            if [[ "$bitness" -ne 64 ]]; then
            die 1 "Unsupported system: Only 64-bit systems are supported. Detected $bitness-bit system."
            fi
            ;;
        "both")
            logD "System compatibility check passed: Detected $bitness-bit system, which is supported."
            ;;
        *)
            die 1 "Configuration error: Invalid value for SUPPORTED_BITNESS ('$SUPPORTED_BITNESS')."
            ;;
    esac
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
        logI "System: Unknown (could not extract system information)."
    else
        logI "System: $system_name."
    fi
}

##
# @brief Print the script version and optionally log it.
# @details This function displays the version of the script stored in the global
#          variable `VERSION`. It uses `echo` if called by `parse_args`, otherwise
#          it uses `logI`.
#
# @global SCRIPT_NAME The name of the script.
# @global VERSION The version of the script.
#
# @return None
##
print_version() {
    # Check the name of the calling function
    local caller="${FUNCNAME[1]}"

    if [[ "$caller" == "parse_args" ]]; then
        echo -e "$SCRIPT_NAME: version $VERSION" # Display the script name and version
    else
        logD "Running $SCRIPT_NAME version $VERSION"
    fi
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
validate_dependencies() {
    local missing=0  # Counter for missing dependencies
    local dep        # Iterator for dependencies

    for dep in "${DEPENDENCIES[@]}"; do
        if ! command -v "$dep" &>/dev/null; then
            printf "ERROR: Missing dependency: %s\n" "$dep" >&2
            ((missing++)) # Increment the missing counter
        fi
    done

    if ((missing > 0)); then
        die 1 "Missing $missing dependencies. Install them and re-run the script."
    fi
}

##
# @brief Check the availability of critical system files.
# @details Verifies that each file listed in the `SYSTEM_READS` array exists and is readable.
# Logs an error for any missing or unreadable files and exits the script if any issues are found.
#
# @global SYSTEM_READS Array of critical system file paths to check.
# @global logE Function to log error messages.
# @global die Function to handle critical errors and exit.
#
# @return None Exits the script if any required files are missing or unreadable.
##
validate_system_reads() {
    local missing=0  # Counter for missing or unreadable files
    local file       # Iterator for files

    for file in "${SYSTEM_READS[@]}"; do
        if [[ ! -r "$file" ]]; then
            printf "ERROR: Missing or unreadable file: %s\n" "$file" >&2
            ((missing++)) # Increment the missing counter
        fi
    done

    if ((missing > 0)); then
        die 1 "Missing or unreadable $missing critical system files. Ensure they are accessible and re-run the script."
    fi
}

##
# @brief Validate the existence of required environment variables.
#
# This function checks if the environment variables specified in the ENV_VARS array
# are set. Logs any missing variables and exits the script if any are missing.
#
# Global Variables:
#   ENV_VARS - Array of required environment variables.
#
# @return void
##
validate_env_vars() {
    local missing=0  # Counter for missing environment variables
    local var        # Iterator for environment variables

    for var in "${ENV_VARS[@]}"; do
        if [[ -z "${!var}" ]]; then
            printf "ERROR: Missing environment variable: %s\n" "$var" >&2
            ((missing++)) # Increment the missing counter
        fi
    done

    if ((missing > 0)); then
        die 1 "Missing $missing required environment variables." "Ensure they are set and re-run the script."
    fi
}

##
# @brief Determine how the script was executed.
# @details Checks if the script is being run directly, piped through bash,
#          or executed in an unusual manner.
#
# @return 0 (true) if the script is being piped, 1 (false) otherwise.
##
check_pipe() {
    if [[ "$0" == "bash" ]]; then
        if [[ -p /dev/stdin ]]; then
            # Script is being piped through bash
            SCRIPT_NAME="$FALLBACK_NAME"
        else
            # Script was run in an unusual way with 'bash'
            SCRIPT_NAME="$FALLBACK_NAME"
        fi
    else
        # Script run directly
        return
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
    local funcname="$4"
    local lineno="$5"
    local message="$6"
    local details="$7"

    # Write to log file if enabled
    if [[ "${LOG_TO_FILE,,}" == "true" ]]; then
        if [[ -n "$details" && -n "${LOG_PROPERTIES[EXTENDED]}" ]]; then
            # Use CRITICAL for the main message
            printf "[%s]\t[%s]\t[%s/%s:%d]\t%s\n" "$timestamp" "$level" "$SCRIPT_NAME" "$funcname" "$lineno" "$message" >&5

            # Use EXTENDED for details
            IFS="|" read -r extended_label _ _ <<< "${LOG_PROPERTIES[EXTENDED]}"
            printf "[%s]\t[%s]\t[%s/%s:%d]\tDetails: %s\n" "$timestamp" "$extended_label" "$SCRIPT_NAME" "$funcname" "$lineno" "$details" >&5
        else
            # Standard log entry without extended details
            printf "[%s]\t[%s]\t[%s/%s:%d]\t%s\n" "$timestamp" "$level" "$SCRIPT_NAME" "$funcname" "$lineno" "$message" >&5
            [[ -n "$details" ]] && printf "[%s]\t[%s]\t[%s:%s:%d]\tDetails: %s\n" \
                "$timestamp" "$level" "$SCRIPT_NAME" "$funcname" "$lineno" "$details" >&5
        fi
    fi

    # Write to console if enabled
    if [[ "${NO_CONSOLE,,}" != "true" ]] && is_interactive; then
        echo -e "${BOLD}${color}[${level}]${RESET}\t${color}[$SCRIPT_NAME/$funcname:$lineno]${RESET}\t$message"
        if [[ -n "$details" && -n "${LOG_PROPERTIES[EXTENDED]}" ]]; then
            IFS="|" read -r extended_label extended_color _ <<< "${LOG_PROPERTIES[EXTENDED]}"
            echo -e "${BOLD}${extended_color}[${extended_label}]${RESET}\t${extended_color}[$SCRIPT_NAME:$funcname/$lineno]${RESET}\tDetails: $details"
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
    local funcname
    local lineno

    # Generate the current timestamp
    timestamp=$(date "+%Y-%m-%d %H:%M:%S")

    # Retrieve the calling function name
    funcname=${FUNCNAME[3]}

    # Retrieve the line number of the caller
    lineno="${BASH_LINENO[3]}"

    # Return the pipe-separated timestamp and line number
    echo "$timestamp|$funcname|$lineno"
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
    local context timestamp funcname lineno custom_level color severity config_severity

    # Generate context (timestamp and line number)
    context=$(prepare_log_context)
    IFS="|" read -r timestamp funcname lineno <<< "$context"

    # Validate log level and message
    if [[ -z "$message" || -z "${LOG_PROPERTIES[$level]}" ]]; then
        echo -e "ERROR: Invalid log level or empty message in ${FUNCNAME[2]}() at line ${BASH_LINENO[1]}." >&2 && die
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
    print_log_entry "$timestamp" "$custom_level" "$color" "$funcname" "$lineno" "$message" "$details"
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
# @brief Initialize the log file and ensure it's writable.
#
# Determines the log file location, ensures the directory exists,
# and verifies that the log file is writable. Falls back to a temporary log
# file in `/tmp` if necessary.
#
# Global Behavior:
# - If `LOG_FILE` is not explicitly specified:
#   - The log file is created in the current user's home directory.
#   - The default name of the log file is derived from the script's name (without extension),
#     e.g., `<script_name>.log`.
#   - If the home directory is unavailable or unwritable, a temporary file is created in `/tmp`.
#
# Global Variables:
#   LOG_FILE (out) - Path to the log file used by the script.
#   SCRIPT_NAME (in) - Name of the current script, used to derive default log file name.
#
# Environment Variables:
#   SUDO_USER - Used to determine the home directory of the invoking user.
#
# @return void
##
init_log() {
    local scriptname="${SCRIPT_NAME%%.*}"   # Extract script name without extension
    local homepath                          # Home directory of the current user
    local log_dir                           # Directory of the log file

    # Determine home directory
    homepath=$(getent passwd "${SUDO_USER:-$(whoami)}" | { IFS=':'; read -r _ _ _ _ _ homedir _; echo "$homedir"; }) || homepath="/tmp"

    # Set log file path
    LOG_FILE="${LOG_FILE:-$homepath/$scriptname.log}"
    log_dir=$(dirname "$LOG_FILE")

    # Ensure log directory exists and is writable
    if [[ ! -d "$log_dir" ]]; then
        echo "ERROR: Log directory does not exist: $log_dir" >&2
        if ! mkdir -p "$log_dir"; then
            echo "ERROR: Failed to create log directory. Falling back to /tmp." >&2
            LOG_FILE=$(mktemp "/tmp/${scriptname}_log_XXXXXX.log")
        fi
    fi

    if [[ ! -w "$log_dir" ]]; then
        echo "ERROR: Log directory is not writable: $log_dir. Falling back to /tmp." >&2
        LOG_FILE=$(mktemp "/tmp/${scriptname}_log_XXXXXX.log")
    fi

    # Ensure the log file is writable
    if ! touch "$LOG_FILE" 2>/dev/null; then
        echo "ERROR: Cannot create log file: $LOG_FILE. Falling back to /tmp." >&2
        LOG_FILE=$(mktemp "/tmp/${scriptname}_log_XXXXXX.log")
    fi

    # Initialize file descriptor for writing
    exec 5>>"$LOG_FILE" || die "Failed to open $LOG_FILE for writing."
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

    # Initialize colors and formatting if the terminal supports at least 8 colors
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
        MOVE_UP=$(default_color cuu 1)
        CLEAR_LINE=$(default_color el)

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
        FGGLD=$(default_color setaf 214)

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
        RESET=""; BOLD=""; SMSO=""; RMSO=""; UNDERLINE=""; NO_UNDERLINE="";
        BLINK=""; NO_BLINK=""; ITALIC=""; NO_ITALIC=""; MOVE_UP=""; CLEAR_LINE=""
        FGBLK=""; FGRED=""; FGGRN=""; FGYLW=""; FGBLU=""
        FGMAG=""; FGCYN=""; FGWHT=""; FGRST=""; FGGLD=""
        BGBLK=""; BGRED=""; BGGRN=""; BGYLW=""
        BGBLU=""; BGMAG=""; BGCYN=""; BGWHT=""; BGRST=""
        DOT=""; HHR=""; LHR=""
    fi

    # Set variables as readonly
    readonly RESET BOLD SMSO RMSO UNDERLINE NO_UNDERLINE BLINK NO_BLINK ITALIC
    readonly NO_ITALIC MOVE_UP CLEAR_LINE
    readonly FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST FGGLD
    readonly BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST
    readonly DOT HHR LHR

    # Export variables globally
    export RESET BOLD SMSO RMSO UNDERLINE NO_UNDERLINE BLINK NO_BLINK ITALIC
    export NO_ITALIC MOVE_UP CLEAR_LINE
    export FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST FGGLD
    export BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST
    export DOT HHR LHR
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
            logW "ERROR: Invalid argument for toggle_console_log()." >&2
            return 1
            ;;
    esac

    return 0
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
        echo -e "ERROR: Invalid LOG_LEVEL '$LOG_LEVEL'. Defaulting to 'INFO'." >&2 && die
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
setup_logging_environment() {
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
### Task Execution Functions
############

##
# @brief Print the task status with start and end messages.
# @details Displays a visual indicator of the task's execution status.
#          Shows "[✔]" for success or "[✘]" for failure and logs the result.
#          Temporarily disables console logging during execution.
#
# @param[in] command_text The description of the task to display.
# @param[in] command The command to execute.
##
execute_task() {
    # Local variable declarations
    local start_indicator="${FGGLD}[-]${RESET}"
    local end_indicator="${FGGRN}[✔]${RESET}"
    local fail_indicator="${FGRED}[✘]${RESET}"
    local status_message
    local command_text="$1"  # Task description
    local command="$2"       # Command to execute
    local running_pre="Executing:"
    local running_post=""
    local pass_pre="Completed:"
    local pass_post=""
    local fail_pre="Failed:"
    local fail_post=""
    local previous_value="$NO_CONSOLE"

    # TODO: Implement DRY_RUN

    # Ensure consistent single quotes around the command text
    command_text="'$command_text'"

    # Set the initial message with $running_pre $command_text $running_post
    status_message="$running_pre $command_text $running_post"

    # Remove trailing spaces and add a period to $status_message
    status_message=$(echo "$status_message" | sed 's/[[:space:]]*$//'). 

    # Temporarily disable console logging
    toggle_console_log "off"

    # Print the initial status
    printf "%s %s\n" "$start_indicator" "$status_message"
    logI "$status_message"

    # Execute the command and capture both stdout and stderr
    local output
    output=$(eval "$command" 2>&1)  # Capture both stdout and stderr

    # Capture the exit status of the command
    local exit_status=$?

    # Move the cursor up and clear the line before printing the final status
    printf "%s%s" "${MOVE_UP}" "${CLEAR_LINE}"

    # Determine the result of the command execution
    if [[ $exit_status -eq 0 ]]; then
        # Command succeeded
        status_message="$pass_pre $command_text $pass_post"
        # Remove trailing spaces and add a period to $status_message
        status_message=$(echo "$status_message" | sed 's/[[:space:]]*$//'). 
        printf "%s %s\n" "$end_indicator" "$status_message"
        logI "$status_message."
    else
        # Command failed
        status_message="$fail_pre $command_text $fail_post"
        # Remove trailing spaces and add a period to $status_message
        status_message=$(echo "$status_message" | sed 's/[[:space:]]*$//'). 
        printf "%s %s\n" "$fail_indicator" "$status_message"
        logE "$status_message"
    fi

    # Restore console logging if it was previously enabled
    if [[ "${previous_value,,}" == "false" ]]; then
        toggle_console_log "on"
    fi
}

############
### Command Line Functions
############

##
# @brief Display usage information and examples for the script.
# @details Provides an overview of the script's available options, their purposes,
#          and practical examples for running the script.
#
# @global SCRIPT_NAME The name of the script, typically derived from the script's filename.
#
# @return None Exits the script with a success code after displaying usage information.
##
usage() {
    cat << EOF
Usage: $SCRIPT_NAME [options]

Options:
  -dr, --dry-run              Enable dry-run mode, where no actions are performed.
                              Useful for testing the script without side effects.
  -v, --version               Display the script version and exit.
  -h, --help                  Display this help message and exit.
  -lf, --log-file <path>      Specify the log file location.
                              Default: <script_name>.log in the user's home directory,
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
  NO_CONSOLE                  Set to "true" to disable console logging.

Defaults:
  - If no log file is specified, the log file is created in the user's home
    directory as <script_name>.log.
  - If the home directory is unavailable or unwritable, a temporary log file
    is created in /tmp.

Examples:
  1. Run the script in dry-run mode:
     $SCRIPT_NAME --dry-run
  2. Check the script version:
     $SCRIPT_NAME --version
  3. Specify a custom log file and log level:
     $SCRIPT_NAME -lf /tmp/example.log -ll INFO
  4. Disable console logging while ensuring logs are written to a file:
     $SCRIPT_NAME -nc true -tf true
  5. Enable terse output mode:
     $SCRIPT_NAME --terse true

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
# @global NO_CONSOLE         Boolean flag indicating console output status.
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
            --no-console|-nc)
                if [[ -z "$2" || "$2" =~ ^- ]]; then
                    echo "ERROR: Missing argument for $1. Valid options are: true, false." >&2
                    exit 1
                fi
                NO_CONSOLE="$2"
                case "${NO_CONSOLE,,}" in
                    true|false) ;;  # Valid values
                    *)
                        echo "ERROR: Invalid value for $1: $NO_CONSOLE. Valid options are: true, false." >&2
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

    # Set default values if not provided
    LOG_FILE="${LOG_FILE:-}"
    LOG_LEVEL="${LOG_LEVEL:-DEBUG}"
    LOG_TO_FILE="${LOG_TO_FILE:-unset}"
    TERSE="${TERSE:-false}"
    NO_CONSOLE="${NO_CONSOLE:-false}"

    # Export and make relevant global variables readonly
    readonly DRY_RUN LOG_LEVEL LOG_TO_FILE TERSE
    export DRY_RUN LOG_FILE LOG_LEVEL LOG_TO_FILE TERSE NO_CONSOLE
}

############
### Installer Functions
############

##
# @brief Display installation instructions or log the start of the installation in non-interactive or terse mode.
# @details Provides an overview of the choices the user will encounter during installation
#          if running interactively and not in terse mode. If non-interactive or terse, skips the printing and logs the start.
#
# @global DOT, BGBLK, FGYLW, FGGRN, HHR, RESET Variables for terminal formatting.
# @global PACKAGE The name of the package being installed.
# @global TERSE Indicates whether the script is running in terse mode.
#
# @return None Waits for the user to press any key before proceeding if running interactively.
##
display_start() {
    # Check if the script is non-interactive or in terse mode
    if [ "$TERSE" = "true" ]; then
        logI "$PACKAGE install beginning."
        # TODO: Add a terse/no console mode logger here if needed.
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
    local needs_set current_date tz yn

    # Get the current date and time
    current_date="$(date)"
    tz="$(date +%Z)"

    # Determine if the timezone needs to be set
    if [[ ! "$tz" == "GMT" && ! "$tz" == "BST" ]]; then
        logI "Timezone is set to $tz."
        return
    else
        # Handle terse mode
        if [[ "$TERSE" == "true" ]]; then
            logW "Timezone detected as $tz, which may need to be updated."
            # TODO: Add a terse/no console mode logger here if needed.
            return
        fi
    fi

    # Inform the user about the current date and time
     echo -e "Current date and time: $current_date"

    # Interactive prompt for confirmation or reconfiguration
    while true; do
        read -rp "Is this correct? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]*) 
                logI "Timezone confirmed as $tz on $current_date."
                break  # User confirms timezone is correct
                ;;
            [Nn]* | "") 
                dpkg-reconfigure tzdata  # Reconfigure timezone
                break
                ;;
            *)
                logW "Invalid input. Please answer 'y' or 'n'."
                ;;
        esac
    done
}

##
# @brief Installs or upgrades all packages in the APTPACKAGES list.
#
# @details
#   Updates the package list and resolves broken dependencies before proceeding.
#   Each package in the APTPACKAGES list is installed or upgraded if already installed.
#
# @return Logs the success or failure of each operation.
##
install_update_packages() {
    local package failcount=0 successcount=0  # Track success and failure counts

    logI "Updating local apt cache and resolving broken dependencies."

    # Update package list and fix broken installs
    if ! execute_task "apt updates and fix broken" "sudo apt-get update -y && sudo apt-get install -f -y"; then
        die 1 "Failed to update package list or resolve broken installs. Aborting."
    fi

    # Check if the package list is defined
    if [[ -z "${APTPACKAGES[*]}" ]]; then
        die 1 "APTPACKAGES list is empty. No packages to install or upgrade."
    fi

    # Install or upgrade each package in the list
    for package in "${APTPACKAGES[@]}"; do
        logI "Processing package: $package"
        
        # Check if the package is already installed
        if dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q "install ok installed"; then
            logI "Package $package is already installed. Attempting to upgrade."
            if ! execute_task "upgrading package $package" "sudo apt-get install --only-upgrade -y $package"; then
                logW "Failed to upgrade package: $package. Continuing with the next package."
                ((failcount++))
            else
                logI "Successfully upgraded package: $package."
                ((successcount++))
            fi
        else
            logI "Package $package is not installed. Attempting to install."
            if ! execute_task "installing $package" "sudo apt-get install -y $package"; then
                logW "Failed to install package: $package. Continuing with the next package."
                ((failcount++))
            else
                logI "Successfully installed package: $package."
                ((successcount++))
            fi
        fi
    done

    # Package install
    if ((failcount > 0)); then
        # Log summary of results
        warn "Package installation errors, ${failcount} package(s) failed."
    else
        # Log summary of results
        logI "Package installation and upgrade process complete."
        logI "Summary: ${successcount} package(s) successfully installed/upgraded, ${failcount} package(s) failed."
    fi
}

##
# @brief Check if the snd_bcm2835 module is available and blacklist it if necessary.
# @details Checks if the snd_bcm2835 module is present on the system and if it is not
#          already blacklisted, it adds it to the blacklist and sets the REBOOT flag.
#
# @global REBOOT The flag indicating if a reboot is required.
#
# @return None
##
check_snd_bcm2835_status() {
    # Declare local variables
    local module="snd_bcm2835"
    local blacklist_file="/etc/modprobe.d/alsa-blacklist.conf"

    # Check if the module exists in the kernel
    if ! modinfo "$module" &>/dev/null; then
        die 1 "Module $module is not available in the kernel."
        return 1
    fi

    # Check if the module is blacklisted
    if grep -q "blacklist $module" "$blacklist_file" 2>/dev/null; then
        logD "Module $module is already blacklisted in $blacklist_file."
    else
        # Blacklist the module if not already blacklisted
        if execute_task "Blacklisting module $module" "echo 'blacklist $module' | sudo tee '$blacklist_file' >/dev/null"; then
            logI "Module $module has been successfully blacklisted in $blacklist_file."
        else
            die 1 "Failed to write to $blacklist_file. Check permissions."
            return 1
        fi

        # Set REBOOT flag
        REBOOT="true"
        readonly REBOOT
        export REBOOT

        # Display appropriate message based on TERSE mode
        if [ "${TERSE:-false}" = "true" ]; then
            logI "Module $module has been blacklisted." "A reboot is required for changes to take effect."
            # TODO: Add a terse/no console mode logger here if needed.
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
# @global VERSION The release version of the software.
# @global TERSE Indicates whether the script is running in terse mode.
#
# @return None
##
finish_script() {
    # TODO:  Fully test all paths here
    # Declare local variables
    local sp7 sp11 sp18 sp28 sp49 wspr_url_ip wspr_url_local reboot_choice

    # Generate spacing for formatting
    generate_spacing() {
        printf ' %.0s' $(seq 1 "$1")
    }
    sp7=$(generate_spacing 7)
    sp11=$(generate_spacing 11)
    sp18=$(generate_spacing 18)
    sp28=$(generate_spacing 28)
    sp49=$(generate_spacing 49)

    # Check for REBOOT first, and then handle TERSE condition
    if [ "$REBOOT" = "false" ]; then
        if [ "$TERSE" = "true" ]; then
            logI "$PACKAGE install complete."
            # TODO: Add a terse/no console mode logger here if needed.
            return
        fi
    elif [ "$REBOOT" = "true" ]; then
        if [ "$TERSE" = "true" ]; then
            logW "$PACKAGE install complete. A reboot is necessary to complete functionality."
            # TODO: Add a terse/no console mode logger here if needed.
            return
        else
            # Prompt for reboot in non-terse mode
            echo "A reboot is required to complete functionality."
            read -rp "Reboot now? [Y/n]: " reboot_choice < /dev/tty
            case "$reboot_choice" in
                [Yy]* | "") 
                    logI "Rebooting the system as requested."
                    sudo reboot
                    ;;
                [Nn]*) 
                    logI "Reboot deferred by the user."
                    ;;
                *) 
                    echo "Invalid input. Please answer 'Y' or 'n'."
                    return
                    ;;
            esac
            return
        fi
    fi  # End of REBOOT check

    # Final check for TERSE and return if true
    if [ "$TERSE" = "true" ]; then
        logI "$PACKAGE install complete."
        # TODO: Add a terse/no console mode logger here if needed.
        return
    fi

    # Fetch WSPR URLs
    wspr_url_ip=$(hostname -I | awk '{print $1}') || wspr_url_ip="Unavailable"
    wspr_url_local=$(hostname).local || wspr_url_local="Unavailable"

    # Check if both are unavailable
    if [ "$wspr_url_ip" = "Unavailable" ] && [ "$wspr_url_local" = "Unavailable" ]; then
        logE "Both WSPR IP address and local hostname are unavailable."
        # TODO: Add a terse/no console mode logger here if needed.
    fi

    # Display the completion message
    cat << EOF

$DOT$BGBLK$FGYLW$sp7 ___         _        _ _    ___                _     _$sp18
$DOT$BGBLK$FGYLW$sp7|_ _|_ _  __| |_ __ _| | |  / __|___ _ __  _ __| |___| |_ ___ $sp11
$DOT$BGBLK$FGYLW$sp7 | || ' \(_-<  _/ _\` | | | | (__/ _ \ '  \| '_ \ / -_)  _/ -_)$sp11
$DOT$BGBLK$FGYLW$sp7|___|_|\_/__/\__\__,_|_|_|  \___\___/_|_|_| .__/_\___|\__\___|$sp11
$DOT$BGBLK$FGYLW$sp49|_|$sp28
$DOT$BGBLK$FGGRN$HHR$RESET

EOF

    if [ "$wspr_url_ip" != "Unavailable" ] && [ "$wspr_url_local" = "Unavailable" ]; then
        # Use the IP address for the frontend URL
        cat << EOF
The WSPR daemon has started.
- WSPR frontend URL   : http://$wspr_url_ip/wspr
- Release version     : $VERSION

Happy DXing!
EOF
    else
        # Use the local hostname for the frontend URL
        cat << EOF
The WSPR daemon has started.
- WSPR frontend URL   : http://$wspr_url_local/wspr
- Release version     : $VERSION

Happy DXing!
EOF
    fi
}

##
# @brief Main function for script execution.
# @details Performs initialization, validation, and core operations of the script.
#          Executes tasks while logging actions and results.
#
# @param[in] "$@" Command-line arguments passed to the script.
#
# @return None Exits with the return status of the main function.
##
main() {
    # Get fallback name if piped through bash
    check_pipe

    # Perform essential checks
    enforce_sudo                         # Ensure proper privileges for script execution
    validate_dependencies                # Ensure required dependencies are installed
    validate_system_reads                # Verify critical system files are accessible
    validate_env_vars                    # Check for required environment variables

    # Parse command-line arguments
    parse_args "$@"

    # Setup logging environment
    setup_logging_environment

    # Check the script's runtime environment
    check_bash                           # Ensure the script is executed in a Bash shell
    check_bash_version                   # Verify the current Bash version meets minimum requirements
    check_bitness                        # Validate system bitness compatibility
    check_release                        # Check Raspbian OS version compatibility
    check_architecture                   # Validate Raspberry Pi model compatibility
    check_internet                       # Verify internet connectivity if required

    # Log system and script version details
    print_system                         # Log system information
    print_version                        # Log the script version

    # Script start
    display_start
    set_time
    #install_update_packages

    # do_service "wspr" "" "/usr/local/bin" # Install/upgrade wspr daemon
    # do_shutdown_button "shutdown_watch" "py" "/usr/local/bin" # Handle TAPR shutdown button
    # do_www "/var/www/html/wspr" "$LOCAL_WWW_DIR" # Download website
    # do_apache_setup

    check_snd_bcm2835_status

    # Script complete
    finish_script

    # Log script completion
    logI "Script '$SCRIPT_NAME' complete."
}

# Run the main function and exit with its return status
main "$@"
exit $?
