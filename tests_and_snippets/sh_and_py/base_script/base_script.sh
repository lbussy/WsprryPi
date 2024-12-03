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
#   - Test string variables in arguments (move this back to log.sh)
#   - Implement variable expansion on log path argument (move this back to log.sh)
#   - Consider implementing DRY_RUN in future work
#   - Find a way to show pending / completed actions, e.g.:
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
# @global THISSCRIPT Name of the script.
#
# @return None (exits the script with an error code).
##
# shellcheck disable=SC2329
trap_error() {
    local func="${FUNCNAME[1]:-main}"  # Get the calling function name (default: "main")
    local line="$1"                   # Line number where the error occurred
    local script="${THISSCRIPT:-$(basename "$0")}"  # Script name (fallback to current script)

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
# @details Sets the script name (`THISSCRIPT`) based on the current environment.
# If `THISSCRIPT` is already defined, its value is retained; otherwise, it is set
# to the basename of the script or defaults to "script.sh".
#
# @global THISSCRIPT The name of the script.
##
declare -r THISSCRIPT="${THISSCRIPT:-$(basename "$0")}"  # Use existing value, script basename, or default to "script.sh"

##
# @brief Project metadata constants used throughout the script.
# @details These variables provide metadata about the script, including ownership,
# versioning, and project details. All are marked as read-only.
##
readonly COPYRIGHT="Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)"  # Copyright notice
readonly PACKAGE="WsprryPi"                                          # Project package name (short)
readonly PACKAGENAME="Wsprry Pi"                                     # Project package name (formatted)
readonly OWNER="lbussy"                                              # Project owner or maintainer
readonly VERSION="1.2.1-version-files+91.3bef855-dirty"              # Current script version
readonly GIT_BRCH="version_files"                                    # Current Git branch

##
# @brief Configuration constants for script requirements and compatibility.
# @details Defines requirements for running the script, including root privileges,
# supported Bash version, OS versions, and system bitness.
##

# Require root privileges to run the script
readonly REQUIRE_SUDO=false  # Set to true if the script requires root privileges

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

# List of required external dependencies for skeleton
declare DEPENDENCIES=("awk" "grep" "tput" "cut" "tr" "getconf" "cat" "sed")
# List of required external dependencies for logging
declare DEPENDENCIES+=("getent" "date" "mktemp" "printf" "whoami")

############
### Skeleton Functions
############

##
# @brief Print a warning message with optional details.
# @details Logs and prints a warning message along with any additional details provided.
#
# @param $1 The main warning message.
# @param $@ Additional details for the warning (optional).
#
# @global BASH_LINENO Array containing line numbers where the warning occurred.
# @global log_message Function to log messages at various severity levels.
#
# @return None
##
# shellcheck disable=SC2329
warn() {
    local lineno="${BASH_LINENO[0]}"  # Line number where the warning occurred
    local message="$1"               # Warning message
    shift
    local details="$*"               # Additional details (optional)

    # Log and print the warning message
    logW "$message" "$details"
}

##
# @brief Print a fatal error message, log it, and exit the script.
# @details Logs a critical error message with optional details, provides context,
# prints a stack trace, and exits the script with the specified or default exit status.
#
# @param $1 Exit status code (default: last command's exit status).
# @param $2 The main error message.
# @param $@ Additional details for the error (optional).
#
# @global BASH_LINENO Array containing line numbers where the error occurred.
# @global THISSCRIPT The name of the script (used in log messages).
# @global log_message Function to log messages at various severity levels.
# @global stack_trace Function to print the stack trace.
#
# @return None (exits the script with the provided or default exit status).
##
die() {
    local lineno="${BASH_LINENO[0]}"    # Line number where the error occurred
    local exit_status="${1:-$?}"        # Exit status code (default: last command's exit status)
    shift
    local message="$1"                  # Error message
    shift
    local details="$*"                  # Additional details (optional)

    # Log the fatal error message
    logC "$message" "$details"

    # Provide context for the critical error
    logC "Script '$THISSCRIPT' did not complete due to a critical error."

    # Print the stack trace
    stack_trace

    # Exit with the provided or default exit status
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
        die 1 "Input to ${FUNCNAME[0]} cannot be empty." >&2
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
        die 1 "Input to ${FUNCNAME[0]} cannot be empty." >&2
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
        die 1 "Input to ${FUNCNAME[0]} cannot be empty." >&2
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
        die 1 "Input to ${FUNCNAME[0]} cannot be empty." >&2
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
check_architecture() {
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
# @global THISSCRIPT The name of the script, typically derived from the script's filename.
#
# @return None Exits the script with a success code after displaying usage information.
##
usage() {
    cat << EOF
Usage: $THISSCRIPT [options]

Options:
  -dr, --dry-run              Simulate actions without making changes.
                              Useful for testing the script without side effects.
  -v, --version               Display the script version and exit.
  -h, --help                  Display this help message and exit.
  -lf, --log-file <path>      Specify the log file location (default: environment variable LOG_FILE or auto-generated).
  -ll, --log-level <level>    Set the log level (DEBUG, INFO, WARNING, ERROR, CRITICAL).
  -tf, --log-to-file <bool>   Set whether to log to file (true, false, or unset to auto-detect interactive).
  -h, --help                  Display this help message and exit.

Environment Variables:
  LOG_FILE                    Path to the log file.
  LOG_LEVEL                   Log level (DEBUG, INFO, WARNING, ERROR, CRITICAL).
  LOG_TO_FILE                 Whether to log to file (true, false, or unset).

Examples:
  1. Run the script in dry-run mode:
     $THISSCRIPT --dry-run
  2. Check the script version:
     $THISSCRIPT --version
  3. Log to /tmp/example.log at INFO level and log to file even if interactive
     $THISSCRIPT -lf /tmp/example.log -ll INFO -tf true
EOF

    # Exit with success
    exit 0
}

##
# @brief Print the script version and optionally log it.
# @details This function displays the version of the script stored in the global 
# variable `VERSION` and optionally logs it if an argument is passed equal to "true".
#
# @global THISSCRIPT The name of the script.
# @global VERSION The version of the script.
#
# @param $1 (optional) If set to "true", logs the version using logI.
#
# @return None (exits the script).
##
print_version() {
    local log_version="${1:-false}" # Default to false if no argument is passed

    if [[ "$log_version" == "true" ]]; then
        logI "Running $THISSCRIPT version $VERSION"
    else
        echo -e "$THISSCRIPT: version $VERSION" # Display the script name and version
    fi
}

##
# @brief Parse command-line arguments.
# @details Processes the command-line arguments passed to the script. Supports options for
#          dry-run mode, displaying the script version, and showing the usage message.
#
# @param $@ The command-line arguments passed to the script.
#
# @global DRY_RUN Sets this variable to true if the dry-run option is specified.
# @global print_version Function to display the script version and exit.
# @global usage Function to display the usage message and exit.
#
# @return None Exits the script with an error if an unknown or unexpected argument is provided.
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
                LOG_FILE="$2"
                shift 2
                ;;
            --log-level|-ll)
                LOG_LEVEL="$2"
                shift 2
                ;;
            --log-to-file|-tf)
                LOG_TO_FILE="$2"
                shift 2
                ;;
            -*)
                # Handle unknown options
                echo -e "ERROR: Unknown option: $arg in ${FUNCNAME[0]}" >&2
                echo -e "Use -h or --help to see available options."
                exit 1
                ;;
            *)
                # Handle unexpected arguments
                echo -e "ERROR: Unexpected argument: $arg in ${FUNCNAME[0]}" >&2
                exit 1
                ;;
        esac
        # Move to the next argument
        shift
    done
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
            echo -e "ERROR" "Missing dependency: $dep" in ${FUNCNAME[0]} # Log the missing dependency
            ((missing++)) # Increment the missing counter
        fi
    done

    if ((missing > 0)); then
        echo -e "ERROR: Missing $missing dependencies in ${FUNCNAME[0]}. Install them and re-run the script."
        exit 1
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
    else
        if ! is_interactive; then
            should_log_to_file=true
        else
            should_log_to_file=false
        fi
    fi

    # Log to file if applicable
    if "$should_log_to_file"; then
        printf "[%s]\t[%s]\t[%s:%d]\t%s\n" "$timestamp" "$level" "$THISSCRIPT" "$lineno" "$message" >> "$LOG_FILE"
        [[ -n "$details" ]] && printf "[%s]\t[%s]\t[%s:%d]\tDetails: %s\n" "$timestamp" "$level" "$THISSCRIPT" "$lineno" "$details" >> "$LOG_FILE"
    fi

    # Always print to the terminal if in an interactive shell
    if is_interactive; then
        # Print the main log message
        echo -e "${BOLD}${color}[${level}]${RESET}\t${color}[$THISSCRIPT:$lineno]${RESET}\t$message"

        # Print the details if provided, using the EXTENDED log level color and format
        if [[ -n "$details" && -n "${LOG_PROPERTIES[EXTENDED]}" ]]; then
            IFS="|" read -r extended_label extended_color _ <<< "${LOG_PROPERTIES[EXTENDED]}"
            echo -e "${BOLD}${extended_color}[${extended_label}]${RESET}\t${extended_color}[$THISSCRIPT:$lineno]${RESET}\tDetails: $details"
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
    local scriptname="${THISSCRIPT%%.*}"  # Extract script name without extension
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
# whether standard output (file descriptor 1) is a terminal.
#
# @return 0 (true) if the script is running interactively; non-zero otherwise.
##
is_interactive() {
    [[ -t 1 ]]  # Check if stdout is a terminal
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
### More Skeleton Functions
############

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

# Main function
main() {
    # Perform essential checks
    parse_args "$@"
    validate_dependencies
    setup_logging_environment
    check_bash
    check_bash_version
    check_bitness
    check_release
    check_architecture
    enforce_sudo
    print_version true

    # Log script start and system information
    logI "Syten: $(grep 'PRETTY_NAME' /etc/os-release | cut -d '=' -f2 | tr -d '"')."
    logI "Script '$THISSCRIPT' started."

    # Example log entries for demonstration purposes
    logD "This is a debug-level message."
    logW "This is a warning-level message."
    logE "This is an error-level message."
    logC "This is a critical-level message."
    logC "This is a critical-level message with extended details." "Additional information about the critical issue."

    # Log script completion
    logI "Script '$THISSCRIPT' complete."
}

# Run the main function and exit with its return status
main "$@"
exit $?
