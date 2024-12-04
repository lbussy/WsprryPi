#!/bin/bash
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

############
### Global Logging Declarations
############

# Constants for the script
declare THISSCRIPT="${THISSCRIPT:-$(basename "$0")}"  # Use the provided THISSCRIPT or derive from basename

# Logging configuration
declare LOG_FILE="${LOG_FILE:-}"          # Use the provided LOG_FILE or default to blank.
declare LOG_LEVEL="${LOG_LEVEL:-DEBUG}"   # Default log level is DEBUG if not set.
declare NO_CONSOLE="${NO_CONSOLE:-false}" # Default to false if not set.  Turns off terminal logging.

# Global variable to control file logging
# LOG_TO_FILE options:
# - "true": Always log to the file
# - "false": Never log to the file
# - unset: Follow logic defined in the is_interactive() function
declare LOG_TO_FILE="${LOG_TO_FILE:-}"    # Default to blank if not set

# List of required external dependencies
declare DEPENDENCIES+=("getent" "date" "mktemp" "printf" "whoami")

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

    # Write to log file if enabled
    if [[ "${LOG_TO_FILE,,}" == "true" ]]; then
        printf "[%s]\t[%s]\t[%s:%d]\t%s\n" "$timestamp" "$level" "$THISSCRIPT" "$lineno" "$message" >&5
        [[ -n "$details" ]] && printf "[%s]\t[%s]\t[%s:%d]\tDetails: %s\n" "$timestamp" "$level" "$THISSCRIPT" "$lineno" "$details" >&5
    fi

    # Write to console if enabled
    if [[ "${NO_CONSOLE,,}" != "true" ]] && is_interactive; then
        echo -e "${BOLD}${color}[${level}]${RESET}\t${color}[$THISSCRIPT:$lineno]${RESET}\t$message"
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
#   THISSCRIPT (in) - Name of the current script, used to derive default log file name.
#
# Environment Variables:
#   SUDO_USER - Used to determine the home directory of the invoking user.
#
# @return void
##
init_log() {
    local scriptname="${THISSCRIPT%%.*}"  # Extract script name without extension
    local homepath                        # Home directory of the current user
    local log_dir                         # Directory of the log file

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
            NO_CONSOLE="false"
            logD "Console logging enabled."
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

    export NO_CONSOLE
    return 0
}

##
# @brief Display usage information for the script.
#
# Provides guidance on how to use the script, including options, environment
# variables, default behaviors, and examples.
##
usage() {
    cat <<EOF
Usage: $THISSCRIPT [options]

Options:
  -lf, --log-file <path>      Specify the log file location.
                              Default: User's home directory with the name <script_name>.log,
                              or a temporary file in /tmp if unavailable.
  -ll, --log-level <level>    Set the log level. Available levels:
                              DEBUG, INFO, WARNING, ERROR, CRITICAL.
                              Default: DEBUG.
  -tf, --log-to-file <bool>   Set whether to log to a file.
                              Options: true, false, unset (auto-detect based on interactivity).
                              Default: unset.
  -nc, --no-console           Disable console logging.
                              Default: Console logging is enabled.
  -h, --help                  Display this help message and exit.

Environment Variables:
  LOG_FILE                    Specify the log file path. Overrides the default location.
  LOG_LEVEL                   Set the logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL).
  LOG_TO_FILE                 Control file logging (true, false, unset).
  NO_CONSOLE                  Set to "true" to disable console logging.

Defaults:
  - If no log file is specified, the log file is created in the user's home directory as <script_name>.log.
  - If the home directory is unavailable or unwritable, a temporary log file is created in /tmp.

Example:
  $THISSCRIPT -lf /tmp/example.log -ll INFO -tf true
EOF
}

##
# @brief Parse command-line options and set corresponding variables.
#
# Processes the script's command-line arguments, sets global variables,
# and validates required arguments for specific options.
#
# @param $@ Command-line arguments.
##
parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --log-file|-lf)
                if [[ -z "$2" || "$2" =~ ^- ]]; then
                    echo "ERROR: Missing argument for $1. Please provide a valid file path." >&2
                    exit 1
                fi
                LOG_FILE="$2"
                shift 2
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
                shift 2
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
                shift 2
                ;;
            --no-console|-nc)
                NO_CONSOLE="true"
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                echo "ERROR: Invalid option '$1'. Use -h or --help for usage information." >&2
                exit 1
                ;;
        esac
    done

    # Set default values if not provided
    LOG_FILE="${LOG_FILE:-}"
    LOG_LEVEL="${LOG_LEVEL:-DEBUG}"
    LOG_TO_FILE="${LOG_TO_FILE:-unset}"
    NO_CONSOLE="${NO_CONSOLE:-false}"

    # Export and make relevant global variables readonly
    readonly DRY_RUN LOG_LEVEL LOG_TO_FILE USE_LOCAL
    export DRY_RUN LOG_LEVEL LOG_TO_FILE USE_LOCAL NO_CONSOLE
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
        echo "WARNING: Invalid LOG_LEVEL '$LOG_LEVEL'. Defaulting to 'INFO'." >&2
        LOG_LEVEL="INFO"  # Set LOG_LEVEL to INFO as a fallback
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
    init_colors
    init_log
    declare -gA LOG_PROPERTIES=(
        ["DEBUG"]="DEBUG|${FGCYN}|0"
        ["INFO"]="INFO|${FGGRN}|1"
        ["WARNING"]="WARN|${FGYLW}|2"
        ["ERROR"]="ERROR|${FGRED}|3"
        ["CRITICAL"]="CRIT|${FGMAG}|4"
        ["EXTENDED"]="EXTD|${FGCYN}|0"
    )
    validate_log_level
}

# TODO:
#
# This resides in "skeleton.sh", so remove after testing
#
die() {
    local message="$1"
    local exit_code="${2:-1}"
    echo "ERROR: $message" >&2
    exit "$exit_code"
}

# TODO:
#
# This resides in "skeleton.sh", so remove after testing
#
##
# @brief Main function to initialize settings and execute script logic.
##
main() {
    # Parse command-line arguments
    parse_args "$@"

    # Initialize the logging environment
    setup_logging_environment

    # Log the script start
    logI "Starting script: $THISSCRIPT"

    # Example log entries for demonstration purposes
    logD "This is a debug-level message."
    logW "This is a warning-level message."
    logE "This is an error-level message."
    logC "This is a critical-level message."
    logC "This is a critical-level message with extended details." "Additional information about the critical issue."
    toggle_console_log off
    logI "This is a message that will only be in the log file."
    toggle_console_log on

    # Log the script completion
    logI "Script execution complete."
}

# TODO:
#
# This resides in "skeleton.sh", so remove after testing
#
# Run the main function and exit with its return status
main "$@"
exit $?
