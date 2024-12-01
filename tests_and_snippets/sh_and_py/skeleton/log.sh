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
#
# TODOs:
# - Localization: Support different languages.
# - Posix: How can we make this Posix compliant?
# - Modularize common code into include files.

############
### Global Declarations
############

# Trap unexpected errors
trap 'echo "ERROR: An unexpected error occurred in ${FUNCNAME[0]:-main} at line $LINENO. Exiting." >&2; exit 1' ERR

# Global Constants
readonly THISSCRIPT=$(basename "$0")       # Name of the script
readonly VERSION="1.2.1-version-files+91.3bef855-dirty"  # Script version
readonly GIT_BRCH="version_files"         # Git branch

# Logging configuration
declare LOG_FILE=""                       # Path to the log file (modifiable)
declare LOG_LEVEL="DEBUG"                 # Default log level

# Global variable to control file logging
# If LOG_TO_FILE is set to "true", always log to the file
# If LOG_TO_FILE is set to "false", never log to the file
# If LOG_TO_FILE is unset, follow logic in is_interactive()
declare LOG_TO_FILE=""


############
### Functions
############

##
# @brief Default color handling for terminal attributes.
# @param $1 Terminal color code or attribute (e.g., sgr0, bold).
# @return The corresponding terminal value or an empty string if unsupported.
##
default_color() {
    tput "$@" 2>/dev/null || echo ""
}

##
# @brief Initialize terminal colors and text formatting.
# Sets up variables for foreground, background colors, and formatting styles.
##
init_colors() {
    if tput colors > /dev/null 2>&1; then
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
        # Fallback for unsupported terminals
        RESET=""; BOLD=""; SMSO=""; RMSO=""; UNDERLINE=""
        NO_UNDERLINE=""; BLINK=""; NO_BLINK=""; ITALIC=""; NO_ITALIC=""
        FGBLK=""; FGRED=""; FGGRN=""; FGYLW=""; FGBLU=""
        FGMAG=""; FGCYN=""; FGWHT=""; FGRST=""
        BGBLK=""; BGRED=""; BGGRN=""; BGYLW=""
        BGBLU=""; BGMAG=""; BGCYN=""; BGWHT=""; BGRST=""
        DOT=""; HHR=""; LHR=""
    fi
}

##
# @brief Generate a timestamp and line number for log entries.
# @return A pipe-separated string containing the timestamp and line number.
##
prepare_log_context() {
    local timestamp lineno
    timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    lineno="${BASH_LINENO[0]}"
    echo "$timestamp|$lineno"
}

##
# @brief Clean and truncate input strings.
# @param $1 Input string to clean.
# @param $2 Maximum length for the cleaned string (default: 60).
# @return Cleaned and truncated string.
##
clean() {
    local input="$1"
    local length="${2:-60}"
    local clean_input

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

    echo "$clean_input"
}

##
# @brief Add a timestamp to log entries.
# @param $1 Maximum line length (default: 60).
##
timestamp() {
    local length="${1:-60}"
    local clean_line

    while IFS= read -r line; do
        clean_line=$(clean "$line" "$length")
        [[ -n "$clean_line" ]] && printf '%(%Y-%m-%d %H:%M:%S)T %s\n' -1 "$clean_line"
    done
}

##
# @brief Ensure the log file exists and is writable.
##
log() {
    local scriptname="${THISSCRIPT%%.*}"
    local homepath
    local log_dir

    homepath=$(getent passwd "${SUDO_USER:-$(whoami)}" | { IFS=':'; read -r _ _ _ _ _ homedir _; echo "$homedir"; })
    LOG_FILE="${LOG_FILE:-$homepath/$scriptname.log}"

    log_dir=$(dirname "$LOG_FILE")
    if [[ ! -d "$log_dir" ]]; then
        echo "ERROR: Log directory does not exist: $log_dir"
        mkdir -p "$log_dir" || { echo "ERROR: Failed to create log directory: $log_dir"; exit 1; }
    fi

    if [[ ! -w "$log_dir" ]]; then
        echo "ERROR: Log directory is not writable: $log_dir"
        LOG_FILE=$(mktemp "/tmp/${scriptname}_log_XXXXXX.log")
        echo "Using fallback log file: $LOG_FILE"
    fi

    touch "$LOG_FILE" || { echo "ERROR: Cannot create log file: $LOG_FILE"; exit 1; }
}

##
# @brief Check if the script is running in an interactive shell.
# @return 0 (true) if the script is running interactively; non-zero otherwise.
##
is_interactive() {
    [[ -t 1 ]]
}

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
        should_log_to_file=! is_interactive
    fi

    # Log to file if applicable
    if $should_log_to_file; then
        printf "[%s]\t[%s]\t[%s:%d]\t%s\n" "$timestamp" "$level" "$THISSCRIPT" "$lineno" "$message" >> "$LOG_FILE"
        [[ -n "$details" ]] && printf "[%s]\t[%s]\t[%s:%d]\tDetails: %s\n" "$timestamp" "$level" "$THISSCRIPT" "$lineno" "$details" >> "$LOG_FILE"
    fi

    # Always print to the terminal if in an interactive shell
    if is_interactive; then
        echo -e "${BOLD}${color}[${level}]${RESET}\t[$THISSCRIPT:$lineno]\t$message"
        [[ -n "$details" ]] && echo -e "${BOLD}${color}[${level}]${RESET}\t[$THISSCRIPT:$lineno]\tDetails: $details"
    fi
}

##
# @brief Log a message with the specified level.
# @param $1 Log level (e.g., DEBUG, INFO, ERROR).
# @param $2 Main log message.
# @param $3 Optional extended details for the log entry.
##
log_message() {
    local level="${1^^}"  # Convert to uppercase
    local message="$2"
    local details="$3"
    local context timestamp lineno custom_level color severity

    context=$(prepare_log_context)
    IFS="|" read -r timestamp lineno <<< "$context"

    # Validate log level and message
    if [[ -z "$message" || -z "${LOG_PROPERTIES[$level]}" ]]; then
        echo "ERROR: Invalid log level or empty message." >&2
        return 1
    fi

    # Extract log properties
    IFS="|" read -r custom_level color severity <<< "${LOG_PROPERTIES[$level]}"
    severity="${severity:-0}"  # Default severity to 0 if not set
    color="${color:-$RESET}"

    # Compare severity against the configured log level
    local config_severity
    IFS="|" read -r _ _ config_severity <<< "${LOG_PROPERTIES[$LOG_LEVEL]}"
    if (( severity < config_severity )); then
        return 0  # Skip logging for levels below the configured threshold
    fi

    # Print log entry
    print_log_entry "$timestamp" "$custom_level" "$color" "$lineno" "$message" "$details"
}

##
# @brief Log a message at the DEBUG level.
# @param $1 Main log message.
# @param $2 Optional extended details for the log entry.
##
logD() { log_message "DEBUG" "$1" "$2"; }

##
# @brief Log a message at the INFO level.
# @param $1 Main log message.
# @param $2 Optional extended details for the log entry.
##
logI() { log_message "INFO" "$1" "$2"; }

##
# @brief Log a message at the WARNING level.
# @param $1 Main log message.
# @param $2 Optional extended details for the log entry.
##
logW() { log_message "WARNING" "$1" "$2"; }

##
# @brief Log a message at the ERROR level.
# @param $1 Main log message.
# @param $2 Optional extended details for the log entry.
##
logE() { log_message "ERROR" "$1" "$2"; }

##
# @brief Log a message at the CRITICAL level.
# @param $1 Main log message.
# @param $2 Optional extended details for the log entry.
##
logC() { log_message "CRITICAL" "$1" "$2"; }

##
# @brief Validate that all required log levels are defined in LOG_PROPERTIES.
##
validate_log_properties() {
    for level in DEBUG INFO WARNING ERROR CRITICAL EXTENDED; do
        if [[ -z "${LOG_PROPERTIES[$level]}" ]]; then
            echo "ERROR: Missing log property for $level" >&2
            exit 1
        fi
    done
}

##
# @brief Validate that the configured LOG_LEVEL is valid.
##
validate_log_level() {
    if [[ -z "${LOG_PROPERTIES[$LOG_LEVEL]}" ]]; then
        echo "WARNING: Invalid LOG_LEVEL '$LOG_LEVEL'. Defaulting to 'INFO'."
        LOG_LEVEL="INFO"
    fi
}

##
# @brief Main function to initialize settings and run the script logic.
##
main() {
    init_colors
    log

    # Define log properties (severity, colors, and labels)
    declare -A LOG_PROPERTIES=(
        ["DEBUG"]="DEBUG|${FGCYN}|0"
        ["INFO"]="INFO|${FGGRN}|1"
        ["WARNING"]="WARN|${FGYLW}|2"
        ["ERROR"]="ERROR|${FGRED}|3"
        ["CRITICAL"]="CRIT|${FGMAG}|4"
        ["EXTENDED"]="EXTD|${FGCYN}|0"
    )

    validate_log_level
    validate_log_properties

    # Example log entries
    logI "Starting script: $THISSCRIPT"
    logD "Debug message example."
    logW "Warning message example."
    logE "Error message example."
    logC "Critical error example."
    logC "Critical error example with extended text." "Extended details."
    logI "Script complete."
}

# Run the main function
main "$@" && exit 0
