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
# to the basename of the script.
#
# @global THISSCRIPT The name of the script.
##
declare -r THISSCRIPT="${THISSCRIPT:-$(basename "$0")}"  # Use existing value, or default to script basename.

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

##
# @brief Require root privileges to run the script.
# @details Use `true` if the script requires root privileges.
##
readonly REQUIRE_SUDO=false  # Set to true if the script requires root privileges

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
declare DEPENDENCIES+=("awk" "grep" "tput" "cut" "tr" "getconf" "cat" "sed" "basename")

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
declare -a ENV_VARS=(
    # "SUDO_USER"  # User invoking the script, especially with elevated privileges
    "HOME"       # Home directory of the current user
    "COLUMNS"    # Terminal width for formatting
)

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
)

##
# @brief Controls whether stack traces are printed for warning messages.
# @details Set to `true` to enable stack trace logging for warnings.
##
readonly WARN_STACK_TRACE="${WARN_STACK_TRACE:-false}"  # Default to false if not set

############
### Skeleton Functions
############

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
# @global THISSCRIPT The name of the script being executed.
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
    local script="$THISSCRIPT"     # Script name

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
# @global THISSCRIPT Script name.
#
# @return Exits the script with the provided or default exit status.
##
die() {
    # Local variables
    local exit_status="$1"              # First parameter as exit status
    local message                       # Main error message
    local details                       # Additional details
    local lineno="${BASH_LINENO[0]}"    # Line number where the error occurred
    local script="$THISSCRIPT"          # Script name
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
        # Ensure the script is not being run directly as root
        if [[ "$EUID" -eq 0 && -z "$SUDO_USER" ]]; then
            die 1 "This script requires 'sudo' privileges but should not be run as the root user directly."
        fi

        # Ensure the script is being run with sudo
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
# @brief Print the script version and optionally log it.
# @details This function displays the version of the script stored in the global
#          variable `VERSION`. It uses `echo` if called by `parse_args`, otherwise
#          it uses `logI`.
#
# @global THISSCRIPT The name of the script.
# @global VERSION The version of the script.
#
# @return None
##
print_version() {
    # Check the name of the calling function
    local caller="${FUNCNAME[1]}"

    if [[ "$caller" == "parse_args" ]]; then
        echo -e "$THISSCRIPT: version $VERSION" # Display the script name and version
    else
        logD "Running $THISSCRIPT version $VERSION"
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
# Checks if the variables in the ENV_VARS array are set in the current shell or exported environment.
# Logs any missing variables and exits if any are not set.
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
        # Check if the variable is set in the current shell or as an exported environment variable
        if ! declare -p "$var" &>/dev/null; then
            printf "ERROR: Missing environment variable: %s\n" "$var" >&2
            ((missing++)) # Increment the missing counter
        fi
    done

    if ((missing > 0)); then
        die "Missing $missing required environment variables. Ensure they are set and re-run the script." 1
    fi
}

##
# @brief Display usage information and examples for the script.
# @details Provides an overview of the script's available options, their purposes,
#          and practical examples for running the script. This is the primary reference
#          for users seeking guidance on how to interact with the script.
#
# @global THISSCRIPT The name of the script, typically derived from the script's filename.
#
# @return None Exits the script with a success code after displaying usage information.
##
usage() {
    cat << EOF
Usage: $THISSCRIPT [options]

Options:
  -dr, --dry-run      Simulate actions without making changes.
                      Useful for testing the script without side effects.
  -v, --version       Display the script version and exit.
  -h, --help          Display this help message and exit.

Examples:
  1. Run the script in dry-run mode:
       $THISSCRIPT --dry-run

  2. Check the script version:
       $THISSCRIPT --version

  3. Display detailed help information:
       $THISSCRIPT --help

EOF

    # Exit with success
    exit 0
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
            -*)
                # Handle unknown options
                echo "ERROR: Unknown option: $arg" >&2
                echo "Use -h or --help to see available options."
                exit 1
                ;;
            *)
                # Handle unexpected arguments
                echo "ERROR: Unexpected argument: $arg" >&2
                exit 1
                ;;
        esac
        # Move to the next argument
        shift
    done
}

# TODO - These are simple functions for testing to be replaced by log.sh
log_message() {
    local level="$1"
    shift
    local message="$1"
    shift
    local details="$*"

    # Get the script name and line number dynamically
    local script="$THISSCRIPT"
    local lineno="${BASH_LINENO[1]}"

    # Concatenate level, script name, and line number with a tab
    local formatted_level="${level}\t[${script}:${lineno}]"

    # Log the main message
    printf "%b\t%s\n" "$formatted_level" "$message"

    # Log the details separately with the level EXTD if provided
    if [[ -n "$details" ]]; then
        local details_level="[EXTD]\t[${script}:${lineno}]"
        printf "%b\tDetails: %s\n" "$details_level" "$details"
    fi
}
logD() {
    log_message "[DEBUG]" "$1" "$2"
}
logI() {
    log_message "[INFO]" "$1" "$2"
}
logW() {
    log_message "[WARN]" "$1" "$2"
}
logE() {
    log_message "[ERROR]" "$1" "$2"
}
logC() {
    log_message "[CRIT]" "$1" "$2"
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
            [[ "$tput_colors_available" -ge 8 ]] && color="\033[1;31m"  # Bright Red
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
        return
    fi

    # Compare the current Bash version against the required version
    if ((BASH_VERSINFO[0] < ${required_version%%.*} ||
         (BASH_VERSINFO[0] == ${required_version%%.*} &&
          BASH_VERSINFO[1] < ${required_version##*.}))); then
        die 1 "This script requires Bash version $required_version or newer."
    fi
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
        printf "[ERROR]\tThis script requires Bash. Please run it with Bash.\n" >&2
        exit 1
    fi
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
# @brief Main function of the script.
# @details Executes the primary flow of the script, including pre-execution checks,
#          system validation, and script-specific tasks. Logs key events and system
#          information for tracking and debugging purposes.
#
# @param $@ Command-line arguments passed to the script.
#
# @return None Exits the script if a critical error occurs during execution.
##
main() {
    # Perform essential checks:
    validate_dependencies
    validate_system_reads
    validate_env_vars
    # Check command line:
    parse_args "$@"

    # TODO: Logging init goes here

    # Check the rest of the environment:
    check_bash
    check_bash_version
    check_bitness
    check_release
    check_architecture
    enforce_sudo
    check_internet

    # Informational/debug lines
    print_system
    print_version

    # Log script start and system information

    logI "Script '$THISSCRIPT' started."

    ########
    # Begin script-specific tasks
    ########

    # TODO: Add script-specific functionality here.

    ########
    # End script-specific tasks
    ########

    # Log script completion
    logI "Script '$THISSCRIPT' complete."
}

# Run the main function and exit with its return status
main "$@"
exit $?
