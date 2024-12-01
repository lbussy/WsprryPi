#!/bin/bash
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

############
### Global Declarations
############

# Global Constants
readonly REQUIREROOT=false  # Set to true if script requires root privileges
readonly THISSCRIPT=$(basename "$0")    # Name of the script
# readonly THISSCRIPT="install.sh"
readonly VERSION="1.2.1-version-files+91.3bef855-dirty"  # Script version
readonly GIT_BRCH="version_files"  # Git branch

# Logging configuration
declare LOG_FILE=""   # Path to the log file (modifiable)
declare LOG_LEVEL="DEBUG"  # Default log level

# Supported OS and models
readonly MIN_OS=11  # Minimum supported OS version
readonly MAX_OS=15  # Maximum supported OS version (use -1 for no upper limit)

# List of supported Raspberry Pi models
readonly SUPPORTED_MODELS=(
    "Raspberry Pi 1"
    "Raspberry Pi 2"
    "Raspberry Pi 3"
    "Raspberry Pi 4"
    "Raspberry Pi Zero"
    "Raspberry Pi Zero W"
)
readonly SUPPORTED_BITNESS="32"  # Supported bitness ("32", "64", or "both")

# Project metadata
readonly COPYRIGHT="Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)"
readonly PACKAGE="WsprryPi"
readonly PACKAGENAME="Wsprry Pi"
readonly OWNER="lbussy"

# List of required dependencies
readonly DEPENDENCIES=("awk" "grep" "tput" "cut" "tr")

# Global terminal color/formatting variables
declare RESET BOLD SMSO RMSO FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST
declare BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST DOT HHR LHR SYSVER

# Function: Check for required dependencies
dependency_check() {
    local missing=0  # Counter for missing dependencies
    local dep        # Iterator for dependencies

    for dep in "${DEPENDENCIES[@]}"; do
        if ! command -v "$dep" &>/dev/null; then
            log_message "ERROR" "Missing dependency: $dep"
            ((missing++))
        fi
    done

    if ((missing > 0)); then
        die 1 "Missing $missing dependencies. Install them and re-run the script."
    else
        log_message "INFO" "All dependencies are satisfied."
    fi
}

# Function: Check system bitness compatibility
check_bitness() {
    local bitness
    bitness=$(getconf LONG_BIT)

    case "$SUPPORTED_BITNESS" in
        "32")
            if [[ "$bitness" -ne 32 ]]; then
                log_message "CRITICAL" "This script only supports 32-bit systems. Detected $bitness-bit system."
                die 1 "Only 32-bit is supported. Exiting."
            fi
            ;;
        "64")
            if [[ "$bitness" -ne 64 ]]; then
                log_message "CRITICAL" "This script only supports 64-bit systems. Detected $bitness-bit system."
                die 1 "Only 64-bit is supported. Exiting."
            fi
            ;;
        "both")
            log_message "DEBUG" "Detected $bitness-bit system, which is supported."
            ;;
        *)
            log_message "CRITICAL" "Invalid SUPPORTED_BITNESS value: $SUPPORTED_BITNESS. Must be '32', '64', or 'both'."
            die 1 "Configuration error: Invalid SUPPORTED_BITNESS. Exiting."
            ;;
    esac
}

# Function: Check Raspbian OS version compatibility
check_release() {
    local ver
    ver=$(grep "VERSION_ID" /etc/os-release | awk -F "=" '{print $2}' | tr -d '"')

    if [[ "$ver" -lt "$MIN_OS" ]]; then
        die 1 "Raspbian version $ver is older than the minimum supported version ($MIN_OS)."
    fi

    if [[ "$MAX_OS" -ne -1 && "$ver" -gt "$MAX_OS" ]]; then
        die 1 "Raspbian version $ver is newer than the maximum supported version ($MAX_OS)."
    fi

    log_message "DEBUG" "Raspbian version $ver is supported."
}

# Function: Check if Raspberry Pi model is supported
check_architecture() {
    local model
    model=$(tr -d '\0' < /proc/device-tree/model)

    local is_supported=false
    for supported_model in "${SUPPORTED_MODELS[@]}"; do
        if [[ "$model" == *"$supported_model"* ]]; then
            is_supported=true
            break
        fi
    done

    if [[ "$is_supported" == false ]]; then
        die 1 "Unsupported Raspberry Pi model: $model. Supported models are: ${SUPPORTED_MODELS[*]}"
    fi

    log_message "DEBUG" "Detected Raspberry Pi model: $model is supported."
}

# Function: Clean input strings and limit length
clean() {
    local input="$1"                    # Input string
    local length="${2:-60}"             # Maximum length (default: 60)
    local clean_input=""

    # Remove ANSI escape sequences
    clean_input=$(printf "%s" "$input" | while read -r line; do
        line="${line//[$'\e'][0-9;]*[a-zA-Z]/}"  # Remove escape sequences
        echo "$line"
    done)

    # Trim leading and trailing whitespace
    clean_input="${clean_input#"${clean_input%%[![:space:]]*}"}"  # Remove leading spaces
    clean_input="${clean_input%"${clean_input##*[![:space:]]}"}"  # Remove trailing spaces

    # Limit to the specified number of characters
    clean_input="${clean_input:0:$length}"

    # Return the cleaned string
    echo "$clean_input"
}

# Function: Add a timestamp to log entries
timestamp() {
    local length="${1:-60}"             # Max line length (default: 60)
    local clean_line                    # Cleaned line of input

    while IFS= read -r line; do
        clean_line=$(clean "$line" "$length")
        # Output the timestamped log entry if the line is not empty
        [[ -n "$clean_line" ]] && printf '%(%Y-%m-%d %H:%M:%S)T %s\n' -1 "$clean_line"
    done
}

# Function: Set up logging
log() {
    local homepath                      # User's home directory
    local scriptname                    # Script name without extension
    local log_dir                       # Directory for log file

    scriptname="${THISSCRIPT%%.*}"
    homepath=$(getent passwd "${SUDO_USER:-$(whoami)}" | { IFS=':'; read -r _ _ _ _ _ homedir _; echo "$homedir"; })
    LOG_FILE="${LOG_FILE:-$homepath/$scriptname.log}"

    # Check if log directory exists, if not create it
    log_dir=$(dirname "$LOG_FILE")
    if [[ ! -d "$log_dir" ]]; then
        echo "ERROR: Log directory does not exist: $log_dir"
        mkdir -p "$log_dir" || { echo "ERROR: Failed to create log directory: $log_dir"; exit 1; }
    fi

    # Ensure the directory is writable
    if [[ ! -w "$log_dir" ]]; then
        echo "ERROR: Log directory is not writable: $log_dir"
        LOG_FILE=$(mktemp "/tmp/${scriptname}_log_XXXXXX.log")
        echo "Using fallback log file: $LOG_FILE"
    fi

    # Create or validate log file
    touch "$LOG_FILE" || { echo "ERROR: Cannot create log file: $LOG_FILE"; exit 1; }
}

# Function: Log messages with different levels
log_message() {
    local level="$1"                    # Log level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
    shift
    local message="$1"                  # Log message
    shift
    local details="$@"                  # Additional details (if any)
    local timestamp                     # Current timestamp
    local lineno="${BASH_LINENO[0]}"    # Line number for the log message
    local custom_level                  # Custom log level name

    # Map log levels to custom names
    local -A LOG_LEVEL_MAP=(
        ["DEBUG"]="DEBUG"
        ["INFO"]="INFO"
        ["WARNING"]="WARN"
        ["ERROR"]="ERROR"
        ["CRITICAL"]="CRIT"
    )
    custom_level="${LOG_LEVEL_MAP[$level]:-$level}"

    # Log levels for comparison
    local LEVELS=("DEBUG" "INFO" "WARNING" "ERROR" "CRITICAL")
    local LEVEL_NUMS=(0 1 2 3 4)  # Severity values
    local MESSAGE_LEVEL_NUM LOG_LEVEL_NUM

    # Map current log level to numeric value
    for i in "${!LEVELS[@]}"; do
        [[ "${LEVELS[$i]}" == "$level" ]] && MESSAGE_LEVEL_NUM="${LEVEL_NUMS[$i]}"
        [[ "${LEVELS[$i]}" == "$LOG_LEVEL" ]] && LOG_LEVEL_NUM="${LEVEL_NUMS[$i]}"
    done

    # Log the message if its level is >= configured log level
    if [[ "$MESSAGE_LEVEL_NUM" -ge "$LOG_LEVEL_NUM" ]]; then
        timestamp=$(date "+%Y-%m-%d %H:%M:%S")

        # Write log to the log file
        echo -e "[$timestamp]\t[$custom_level]\t[$THISSCRIPT:$lineno]\t$message" >> "$LOG_FILE"
        [[ -n "$details" ]] && echo -e "[$timestamp]\t[DATA]\t[$THISSCRIPT:$lineno]\tDetails: $details" >> "$LOG_FILE"

        # Print log to the terminal
        local -A COLOR_MAP=(
            ["DEBUG"]="${FGCYN}"
            ["INFO"]="${FGGRN}"
            ["WARNING"]="${FGYLW}"
            ["ERROR"]="${FGRED}"
            ["CRITICAL"]="${FGMAG}"
        )
        local color="${COLOR_MAP[$level]}"
        local level_msg="[$custom_level]"

        echo -e "${BOLD}${color}${level_msg}${RESET}\t${color}[$THISSCRIPT:$lineno]${RESET}\t$message"
        if [[ -n "$details" ]]; then
            echo -e "${FGCYN}[DATA]${RESET}\t${FGCYN}[$THISSCRIPT:$lineno]${RESET}\tDetails: $details"
        fi
    fi
}

# Function: Configure terminal codes for color and formatting
term() {
    local retval  # Terminal color support status

    # Check if the terminal supports colors
    tput colors > /dev/null 2>&1
    retval="$?"

    if [[ "$retval" -eq 0 ]]; then
        # Terminal supports colors; define formatting and color codes
        RESET=$(tput sgr0)
        BOLD=$(tput bold)
        SMSO=$(tput smso)
        RMSO=$(tput rmso)
        FGBLK=$(tput setaf 0)  # Foreground black
        FGRED=$(tput setaf 1)  # Foreground red
        FGGRN=$(tput setaf 2)  # Foreground green
        FGYLW=$(tput setaf 3)  # Foreground yellow
        FGBLU=$(tput setaf 4)  # Foreground blue
        FGMAG=$(tput setaf 5)  # Foreground magenta
        FGCYN=$(tput setaf 6)  # Foreground cyan
        FGWHT=$(tput setaf 7)  # Foreground white
        FGRST=$(tput setaf 9)  # Reset foreground
        BGBLK=$(tput setab 0)  # Background black
        BGRED=$(tput setab 1)  # Background red
        BGGRN=$(tput setab 2)  # Background green
        BGYLW=$(tput setab 3)  # Background yellow
        BGBLU=$(tput setab 4)  # Background blue
        BGMAG=$(tput setab 5)  # Background magenta
        BGCYN=$(tput setab 6)  # Background cyan
        BGWHT=$(tput setab 7)  # Background white
        BGRST=$(tput setab 9)  # Reset background
        DOT="$(tput sc)$(tput setaf 0)$(tput setab 0).$(tput sgr0)$(tput rc)"
        HHR="$(printf '═%.0s' $(seq 1 "${COLUMNS:-$(tput cols)}"))"  # Horizontal line
        LHR="$(printf '─%.0s' $(seq 1 "${COLUMNS:-$(tput cols)}"))"  # Smaller separator line
    else
        # Terminal does not support colors; leave variables empty
        RESET=""
        BOLD=""
        SMSO=""
        RMSO=""
        FGBLK=""
        FGRED=""
        FGGRN=""
        FGYLW=""
        FGBLU=""
        FGMAG=""
        FGCYN=""
        FGWHT=""
        FGRST=""
        BGBLK=""
        BGRED=""
        BGGRN=""
        BGYLW=""
        BGBLU=""
        BGMAG=""
        BGCYN=""
        BGWHT=""
        BGRST=""
        DOT=""
        HHR=""
        LHR=""
    fi
}

# Function: Print a warning message
warn() {
    local lineno="${BASH_LINENO[0]}"     # Line number where the warning occurred
    local message="$1"                  # Warning message
    shift
    local details="$@"                  # Additional details (optional)

    # Log and print the warning message
    log_message "WARNING" "$message" "$details"
}

# Function: Print a fatal error message and exit
die() {
    local lineno="${BASH_LINENO[0]}"     # Line number where the error occurred
    local exit_status="${1:-$?}"        # Exit status code (default: last command's exit status)
    shift
    local message="$1"                  # Error message
    shift
    local details="$@"                  # Additional details (optional)

    # Log the fatal error message
    log_message "CRITICAL" "$message" "$details"

    # Provide context for the critical error
    log_message "CRITICAL" "Script '$THISSCRIPT' did not complete due to a critical error."

    # Print the stack trace
    stack_trace

    # Exit with the provided or default exit status
    exit "$exit_status"
}

# Function: Print a stack trace of the running script
stack_trace() {
    local i=0                           # Frame counter
    local output                        # Temporary file for the stack trace

    # Create a temporary file for the stack trace
    output=$(mktemp)

    # Write the stack trace to the temporary file
    echo -e "Stack trace:" > "$output"
    echo -e "----------------------------" >> "$output"
    while caller "$i" > /dev/null; do
        local frame
        frame=$(caller "$i")
        echo -e "  Frame $i: $frame" >> "$output"
        ((i++))
    done
    echo -e "----------------------------" >> "$output"

    # Print the stack trace to the terminal in color
    echo -e "${FGMAG}${BOLD}Stack trace:${RESET}"
    echo -e "${FGMAG}----------------------------${RESET}"
    i=0
    while caller "$i" > /dev/null; do
        local frame
        frame=$(caller "$i")
        echo -e "${FGCYN}  Frame $i:${RESET} ${FGWHT}$frame${RESET}"
        ((i++))
    done
    echo -e "${FGMAG}----------------------------${RESET}"

    # Append the stack trace to the log file
    cat "$output" >> "$LOG_FILE"

    # Remove the temporary file
    rm -f "$output"
}

# Function: Display the script version
show_version() {
    echo "$THISSCRIPT: $VERSION"
}

# Function: Ensure the script is run with the appropriate privileges
checkroot() {
    if [[ "$REQUIREROOT" == true ]]; then
        # Check if the script is not run as root when required
        if [[ "$EUID" -ne 0 ]]; then
            die 1 "This script requires root privileges. Re-run using 'sudo'."
        fi
    else
        # Check if the script is run as root when it shouldn't be
        if [[ "$EUID" -eq 0 ]]; then
            die 1 "This script should not be run as root. Avoid using 'sudo'."
        fi
    fi
}

# Function: Add a dot at the beginning of a string if missing
add_dot() {
    local input="$1"  # Input string
    if [[ "$input" != .* ]]; then
        input=".$input"
    fi
    echo "$input"
}

# Function: Remove a leading dot from a string
remove_dot() {
    local input="$1"  # Input string
    if [[ "$input" == .* ]]; then
        input="${input#.}"
    fi
    echo "$input"
}

# Function: Add a trailing slash to a string if missing
add_slash() {
    local input="$1"  # Input string
    if [[ "$input" != */ ]]; then
        input="$input/"
    fi
    echo "$input"
}

# Function: Remove a trailing slash from a string
remove_slash() {
    local input="$1"  # Input string
    if [[ "$input" == */ ]]; then
        input="${input%/}"
    fi
    echo "$input"
}

# Function: Parse command-line arguments
parse_args() {
    while [[ "$#" -gt 0 ]]; do
        case "$1" in
            --log-file|-l)  # Specify a custom log file
                LOG_FILE="$2"
                if [[ -n "$LOG_FILE" && ! -d "$(dirname "$LOG_FILE")" ]]; then
                    echo "ERROR: Log directory does not exist: $(dirname "$LOG_FILE")"
                    exit 1
                fi
                shift  # Skip over the log file path argument
                ;;
            --log-level|-g)  # Set the log level
                case "$2" in
                    DEBUG|INFO|WARNING|ERROR|CRITICAL)
                        LOG_LEVEL="$2"  # Assign valid log level
                        ;;
                    *)
                        echo "ERROR: Invalid log level: $2"
                        echo "Valid log levels are: DEBUG, INFO, WARNING, ERROR, CRITICAL."
                        exit 1
                        ;;
                esac
                shift  # Skip over the log level argument
                ;;
            --dry-run|-d)  # Enable dry-run mode (simulate actions)
                DRY_RUN=true
                ;;
            --version|-v)  # Display the script version
                echo "$THISSCRIPT: version $VERSION"
                exit 0
                ;;
            --help|-h)  # Display help message
                echo "Usage: $THISSCRIPT [options]"
                echo "Options:"
                echo "  --log-file, -l    Specify a custom log file"
                echo "  --log-level, -g   Set the log level (DEBUG, INFO, WARNING, ERROR, CRITICAL)"
                echo "  --dry-run, -d     Simulate actions without making changes"
                echo "  --version, -v     Show the script version"
                echo "  --help, -h        Show this help message"
                exit 0
                ;;
            *)  # Handle unknown options
                echo "ERROR: Unknown option: $1"
                echo "Use -h or --help to see available options."
                exit 1
                ;;
        esac
        shift  # Process the next argument
    done
}

# Main function
main() {
    # Initialize terminal settings and logging
    term
    log

    # Perform essential checks
    dependency_check
    check_bitness
    check_release
    check_architecture
    checkroot

    # Log script start and system information
    log_message "INFO" "Running on: $(grep 'PRETTY_NAME' /etc/os-release | cut -d '=' -f2 | tr -d '"')."
    log_message "INFO" "Script '$THISSCRIPT' started."

    ########
    # Begin script-specific tasks
    ########

    # Example: Logging messages with different levels
    echo -e "Rainbow tests:"
    log_message "DEBUG" "Example debug message."
    log_message "INFO" "Example info message."
    log_message "WARNING" "Example warning message."
    log_message "ERROR" "Example error message."
    log_message "CRITICAL" "Example critical message."

    # Example: Simulating actions based on dry-run flag
    echo -e "Testing actions:"
    if [[ "$DRY_RUN" == "true" ]]; then
        log_message "INFO" "Dry-run mode is active. Simulating actions."
        echo "Dry-run: This is a simulated action."
    else
        log_message "INFO" "Performing actual actions."
        echo "This is an actual action."
    fi

    # Example: Testing warning function
    warn "This is a test warning." "Testing the warn function."

    # Example: Checking for critical files
    local non_existent_file="/nonexistent/file"
    if [[ ! -f "$non_existent_file" ]]; then
        die 1 "Critical file missing." "Expected file: $non_existent_file"
    fi

    ########
    # End script-specific tasks
    ########

    # Log script completion
    log_message "INFO" "Script '$THISSCRIPT' complete."
}

# Run the main function
main "$@" && exit 0
