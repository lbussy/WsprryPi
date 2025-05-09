#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# -----------------------------------------------------------------------------
# @file    apache_tool.sh
# @brief   Wsprry Pi Apache configuration helper
# @details This script automates the setup and teardown of Apache settings for
#          the Wsprry Pi project. It can:
#            - Add or remove a ServerName directive to suppress FQDN warnings.
#            - Create a catch-all virtual host that redirects “/” to “/wsprrypi/”.
#            - Install the redirect as a Debian-style site under sites-available.
#            - Disable the stock 000-default site and enable your custom site.
#            - Test the Apache configuration for syntax errors.
#            - Restart Apache to apply changes.
#
#          Usage:
#            sudo ./apache_tool.sh [OPTIONS]
#
#          OPTIONS:
#              -u             Uninstall mode: remove ServerName and custom site.
#              -n             Dry-run: show actions without making changes.
#              -v             Verbose logging.
#              -l             Enable file logging to /var/log/apache_tool.log.
#              -t             Self-test: validate dependencies and config files.
#              -f <FILE>      Path to target index.html (default: /var/www/html/index.html).
#              -c <FILE>      Path to apache2.conf (default: /etc/apache2/apache2.conf).
#              -o <FILE>      Path to log file (default: /var/log/apache_tool.log).
#              -s <STRING>    ServerName directive (default: "ServerName localhost").
#              -h             Show this help message and exit.
#
#          Examples:
#            sudo ./apache_tool.sh
#              — Configure Apache with default settings.
#
#            sudo ./apache_tool.sh -u
#              — Remove custom settings and restore defaults.
#
#            DRY_RUN=1 VERBOSE=1 sudo ./apache_tool.sh -s "ServerName wspr4.local"
#              — Simulate adding a custom ServerName for wspr4.local.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @date 2025-05-09
# @copyright MIT License
#
# @license
# MIT License
#
# Copyright (c) 2023-2025 Lee C. Bussy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# -----------------------------------------------------------------------------

# Defaults
THIS_SCRIPT="${THIS_SCRIPT:-$(basename "${BASH_SOURCE[0]}")}"
FALLBACK_SCRIPT_NAME="apache_tool.sh"
declare DEFAULT_TARGET_FILE="/var/www/html/index.html"
declare DEFAULT_APACHE_CONF="/etc/apache2/apache2.conf"
declare DEFAULT_LOG_FILE="/var/log/apache_tool.log"
declare DEFAULT_SERVERNAME="ServerName localhost"
declare DEFAULT_DRY_RUN=0
declare DEFAULT_VERBOSE=0
declare DEFAULT_LOGGING_ENABLED=0
declare DEFAULT_SELF_TEST=0
readonly REQUIRE_SUDO="${REQUIRE_SUDO:-true}"

# Environment variables with defaults
TARGET_FILE="${TARGET_FILE:-$DEFAULT_TARGET_FILE}"
APACHE_CONF="${APACHE_CONF:-$DEFAULT_APACHE_CONF}"
LOG_FILE="${LOG_FILE:-$DEFAULT_LOG_FILE}"
SERVERNAME_DIRECTIVE="${SERVERNAME_DIRECTIVE:-$DEFAULT_SERVERNAME}"
DRY_RUN="${DRY_RUN:-$DEFAULT_DRY_RUN}"
VERBOSE="${VERBOSE:-$DEFAULT_VERBOSE}"
LOGGING_ENABLED="${LOGGING_ENABLED:-$DEFAULT_LOGGING_ENABLED}"
SELF_TEST="${SELF_TEST:-$DEFAULT_SELF_TEST}"

# Colors for console output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'  # No color

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
    width=${COLUMNS:-80} # Max console width

    # Check if $1 is a valid level, otherwise treat it as the message
    case "$level" in
    DEBUG | INFO | WARN | WARNING | ERROR | CRIT | CRITICAL)
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
        elif ((current_length > longest_length)); then
            longest_length=$current_length
        fi

        displayed_stack=("$(printf "%s|%s" "$func()" "$line")" "${displayed_stack[@]}")
    done

    # General text attributes
    local reset="\033[0m" # Reset text formatting
    local bold="\033[1m"  # Bold text

    # Foreground colors
    local fgred="\033[31m"       # Red text
    local fggrn="\033[32m"       # Green text
    local fgblu="\033[34m"       # Blue text
    local fgmag="\033[35m"       # Magenta text
    local fgcyn="\033[36m"       # Cyan text
    local fggld="\033[38;5;220m" # Gold text (ANSI 256 color)

    # Determine color and label based on level
    local color label
    case "$level" in
    DEBUG)
        color=$fgcyn
        label="[DEBUG]"
        ;;
    INFO)
        color=$fggrn
        label="[INFO ]"
        ;;
    WARN | WARNING)
        color=$fggld
        label="[WARN ]"
        ;;
    ERROR)
        color=$fgmag
        label="[ERROR]"
        ;;
    CRIT | CRITICAL)
        color=$fgred
        label="[CRIT ]"
        ;;
    esac

    # Create header and footer
    local dash_count=$(((width - ${#header_name} - 2) / 2))
    local header_l header_r
    header_l="$(printf '%*s' "$dash_count" '' | tr ' ' "$char")"
    header_r="$header_l"
    [[ $(((width - ${#header_name}) % 2)) -eq 1 ]] && header_r="${header_r}${char}"
    local header
    header=$(
        printf "%b%s%b %b%b%s%b %b%s%b" \
            "$color" \
            "$header_l" \
            "$reset" \
            "$color" \
            "$bold" \
            "$header_name" \
            "$reset" \
            "$color" \
            "$header_r" \
            "$reset"
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
    local indent=$(((width / 2) - ((longest_length + 28) / 2)))
    indent=$((indent < 0 ? 0 : indent))
    if [[ -z "${displayed_stack[*]}" ]]; then
        printf "%b[WARN ]%b Stack trace is empty.\n" "$fggld" "$reset" >&2
    else
        for ((i = ${#displayed_stack[@]} - 1, idx = 0; i >= 0; i--, idx++)); do
            IFS='|' read -r func line <<<"${displayed_stack[i]}"
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
    local script="${FALLBACK_SCRIPT_NAME:-unknown}" # This script's name
    local func_name="${FUNCNAME[1]:-main}"          # Calling function
    local caller_line=${BASH_LINENO[0]:-0}          # Calling line

    # Get valid error code
    local error_code
    if [[ -n "${1:-}" && "$1" =~ ^[0-9]+$ ]]; then
        error_code=$((10#$1)) # Convert to numeric
        shift
    else
        error_code=1 # Default to 1 if not numeric
    fi

    # Configurable delimiter
    local delimiter="${WRAP_DELIMITER:-␞}"

    # Get the primary message
    local message
    message=$(sed -E 's/^[[:space:]]*//;s/[[:space:]]*$//' <<<"${1:-A warning was raised on this line}")
    [[ $# -gt 0 ]] && shift

    # Process details
    local details
    details=$(sed -E 's/^[[:space:]]*//;s/[[:space:]]*$//' <<<"$*")

    # Recalculate terminal columns
    COLUMNS=$( (command -v tput >/dev/null && tput cols) || printf "80")
    COLUMNS=$((COLUMNS > 0 ? COLUMNS : 80)) # Ensure COLUMNS is a positive number
    local width
    width=${COLUMNS:-80} # Max console width

    # Escape sequences for colors and attributes
    local reset="\033[0m"        # Reset text
    local bold="\033[1m"         # Bold text
    local fggld="\033[38;5;220m" # Gold text
    local fgcyn="\033[36m"       # Cyan text
    local fgblu="\033[34m"       # Blue text

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
        done <<<"$overflow"
    fi

    # Print secondary details
    if [[ -n "$secondary" ]]; then
        while IFS= read -r line; do
            printf "%s%s\n" "$dets_prefix" "$line" >&2
        done <<<"$secondary"
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
    local script="${FALLBACK_SCRIPT_NAME:-unknown}" # This script's name
    local func_name="${FUNCNAME[1]:-main}"          # Calling function
    local caller_line=${BASH_LINENO[0]:-0}          # Calling line

    # Get valid error code
    if [[ -n "${1:-}" && "$1" =~ ^[0-9]+$ ]]; then
        error_code=$((10#$1)) # Convert to numeric
        shift
    else
        error_code=1 # Default to 1 if not numeric
    fi

    # Configurable delimiter
    local delimiter="${WRAP_DELIMITER:-␞}"

    # Process the primary message
    local message
    message=$(sed -E 's/^[[:space:]]*//;s/[[:space:]]*$//' <<<"${1:-Critical error}")

    # Only shift if there are remaining arguments
    [[ $# -gt 0 ]] && shift

    # Process details
    local details
    details=$(sed -E 's/^[[:space:]]*//;s/[[:space:]]*$//' <<<"$*")

    # Recalculate terminal columns
    COLUMNS=$( (command -v tput >/dev/null && tput cols) || printf "80")
    COLUMNS=$((COLUMNS > 0 ? COLUMNS : 80)) # Ensure COLUMNS is a positive number
    local width
    width=${COLUMNS:-80} # Max console width

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
        done <<<"$overflow"
    fi

    # Print secondary details
    if [[ -n "$secondary" ]]; then
        while IFS= read -r line; do
            printf "%s%s\n" "$dets_prefix" "$line" >&2
        done <<<"$secondary"
    fi

    # Execute stack trace
    stack_trace "CRITICAL" "${message}" "${secondary}"

    # Exit with the specified error code
    exit "$error_code"
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
    local args=() # Array to hold non-debug arguments

    # Look for the "debug" flag in the provided arguments
    for arg in "$@"; do
        if [[ "$arg" == "debug" ]]; then
            debug="debug"
            break # Exit the loop as soon as we find "debug"
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
            "$this_script" "$func_name" "$current_line" "$caller_name" "$caller_line" >&2
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
    local args=() # Array to hold non-debug arguments

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
    local args=() # Array to hold non-debug arguments

    # Loop through all arguments to identify the "debug" flag
    for arg in "$@"; do
        if [[ "$arg" == "debug" ]]; then
            debug="debug"
        else
            args+=("$arg") # Add non-debug arguments to the array
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
    local args=() # Array to hold non-debug arguments

    # Loop through all arguments and identify the "debug" flag
    for arg in "$@"; do
        if [[ "$arg" == "debug" ]]; then
            debug="debug"
            break # Exit the loop as soon as we find "debug"
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
            "$this_script" "$func_name" "$current_line" "$caller_name" "$caller_line" >&2
    fi
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
# shellcheck disable=SC2317
enforce_sudo() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

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
# @brief    Display the script’s usage information and help text.
# @details  Prints the command synopsis, available OPTIONS (and their
#           corresponding environment-variable overrides), examples of usage,
#           and notes specific to the Wsprry Pi project’s Apache configuration.
#
# @global   DEFAULT_TARGET_FILE      Default path to the Apache target file.
# @global   DEFAULT_APACHE_CONF      Default path to the Apache configuration file.
# @global   DEFAULT_LOG_FILE         Default path to the log file.
# @global   DEFAULT_SERVERNAME       Default ServerName directive string.
#
# @return   0 always (exits after printing help via stdout).
# -----------------------------------------------------------------------------
show_help() {
    local script_name="${0##*/}"
    [ "$script_name" = "bash" ] && script_name="apache_tool.sh"

    cat <<EOF
Usage: $script_name [OPTIONS]

This script manages Apache2 configurations and settings for the Wsprry Pi project.

OPTIONS (can also be set via environment variables):
  -u                 Uninstall settings: Removes rewrite rules and 'ServerName localhost'.
                     (Environment variable: DISABLE_MODE=1)
  -n                 Dry-run mode: Simulates actions without making changes.
                     (Environment variable: DRY_RUN=1)
  -v                 Verbose mode: Provides detailed output of actions.
                     (Environment variable: VERBOSE=1)
  -l                 Enable logging to /var/log/apache_tool.log.
                     (Environment variable: LOGGING_ENABLED=1)
  -t                 Self-test mode: Validates setup without applying changes.
                     (Environment variable: SELF_TEST=1)
  -f FILE            Path to the Apache2 target file (default: $DEFAULT_TARGET_FILE).
                     (Environment variable: TARGET_FILE)
  -c FILE            Path to the Apache2 configuration file (default: $DEFAULT_APACHE_CONF).
                     (Environment variable: APACHE_CONF)
  -o FILE            Path to the log file (default: $DEFAULT_LOG_FILE).
                     (Environment variable: LOG_FILE)
  -s STRING          ServerName directive (default: "$DEFAULT_SERVERNAME").
                     (Environment variable: SERVERNAME_DIRECTIVE)
  -h                 Display this help message.

EXAMPLES:
  sudo $script_name                         Configure Apache2 with default settings.
  sudo $script_name -u                      Uninstall settings.
  sudo $script_name -t                      Run self-test mode.
  sudo $script_name -n -v -l                Simulate actions with verbose output and logging.
  sudo $script_name -f /custom/index.html   Use a custom Apache index.html file.

NOTE: Environment variables override defaults. Command-line options take precedence over
      environment variables.

This script is tailored for Wsprry Pi project configurations.
EOF
}

# -----------------------------------------------------------------------------
# @brief    Log messages to console with color and to a file if enabled.
# @details  Determines log level by message prefix (Error, Warning, Success),
#           applies the corresponding ANSI color code, echoes the colored
#           message to the console, and if $LOGGING_ENABLED=1 prepends a
#           timestamp and appends the message to $LOG_FILE.
#
# @global   LOGGING_ENABLED   Flag (1/0) controlling whether to write to file.
# @global   LOG_FILE          Path to the log file for storing entries.
# @global   RED               ANSI color code for error messages.
# @global   YELLOW            ANSI color code for warning messages.
# @global   GREEN             ANSI color code for success messages.
# @global   NC                ANSI reset code to clear console color.
#
# @param    $@                The message and any additional text to log.
#
# @return   0 always.
# -----------------------------------------------------------------------------
log() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    local message="$*"
    local color="$NC"

    # Detect log level and set color
    case "$message" in
        "Error:"*) color="$RED" ;;
        "Warning:"*) color="$YELLOW" ;;
        "Success:"*) color="$GREEN" ;;
    esac

    # Log to console with color
    echo -e "${color}${message}${NC}"

    # Log to file if enabled
    if [ "$LOGGING_ENABLED" -eq 1 ]; then
        echo "$(date '+%Y-%m-%d %H:%M:%S') $message" >> "$LOG_FILE"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Log a message if verbose mode is enabled.
# @details  Checks the global VERBOSE flag, and if it is set to 1, forwards
#           the provided arguments to the log function for output.
#
# @global   VERBOSE    Flag indicating whether verbose logging is active.
# @param    $@         The message components to pass to the log function.
#
# @return   0 always.
# -----------------------------------------------------------------------------
verbose_log() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    [ "$VERBOSE" -eq 1 ] && log "$*"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Rotate the log file when it exceeds the maximum size.
# @details  Checks if the file at $LOG_FILE exists and is larger than 1 MB
#           (1,048,576 bytes). If the threshold is exceeded, renames the log
#           by appending a timestamp suffix and logs that rotation occurred.
#
# @global   LOG_FILE  Path to the log file to monitor and rotate.
#
# @return   0 always (no error on missing or small files).
# -----------------------------------------------------------------------------
rotate_logs() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    local max_size=1048576  # 1MB
    if [ -f "$LOG_FILE" ] && [ "$(stat --format=%s "$LOG_FILE")" -gt "$max_size" ]; then
        mv "$LOG_FILE" "${LOG_FILE}-$(date '+%Y%m%d%H%M%S').backup"
        log "Log file rotated."
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Back up a file by copying it with a timestamped suffix.
# @details  Checks if the specified file exists. If it does, creates a copy
#           named `<file>-YYYYMMDDHHMMSS.backup` in the same directory and
#           logs the backup creation. If the file does not exist, logs a
#           warning and does nothing.
#
# @param    $1  Path to the file to back up.
#
# @return   0 always (warnings or errors are logged but do not change the exit code).
# -----------------------------------------------------------------------------
backup_file() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    local file="$1"
    if [ -f "$file" ]; then
        local backup="${file}-$(date '+%Y%m%d%H%M%S').backup"
        cp "$file" "$backup"
        log "Backup created: $backup"
    else
        log "Warning: File $file does not exist. No backup created."
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Execute a command or log it if running in dry-run mode.
# @details  Checks the global DRY_RUN flag:  
#           - If DRY_RUN=1, logs the command prefixed with “[Dry-Run]” without executing.  
#           - Otherwise runs the command; on failure logs an error and exits the script.
#
# @global   DRY_RUN    If set to 1, commands are only logged and not executed.
# @param    $@         The command and its arguments to execute.
#
# @return   0 on success; exits with status 1 if the command fails.
# -----------------------------------------------------------------------------
run_command() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    if [ "$DRY_RUN" -eq 1 ]; then
        log "[Dry-Run] $*"
    else
        "$@" || { log "Error: Command '$*' failed."; exit 1; }
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Check if the Apache HTTP Server is installed.
# @details  Verifies whether the `apache2ctl` command is available in the
#           system’s PATH. Logs an error message if Apache is not found.
#
# @return   0 if Apache is installed; 1 if it is not.
# -----------------------------------------------------------------------------
check_apache_installed() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    if ! command -v apache2ctl >/dev/null 2>&1; then
        log "Error: Apache2 is not installed. Please install it and retry."
        debug_end "$debug"
        return 1
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Verify that required commands are available in the system.
# @details  Iterates over a list of essential commands (apache2ctl, sed, grep)
#           and checks whether each is in the user’s PATH. Logs an error
#           and returns failure if any command is missing.
#
# @return   0 if all required commands are found; 1 if any are missing.
# -----------------------------------------------------------------------------
check_required_commands() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    for cmd in apache2ctl sed grep; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            log "Error: Required command '$cmd' not found. Please install it and retry."
            debug_end "$debug"
            return 1
        fi
    done

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Run the script’s self-test routine.
# @details  Performs a series of checks to ensure required components are
#           available: verifies Apache is installed, required commands exist,
#           and that key files ($APACHE_CONF and $TARGET_FILE) are present.
#           Logs success or error for each check.
#
# @global   APACHE_CONF  Path to the Apache configuration file.
# @global   TARGET_FILE  Path to the file to be served by Apache.
#
# @return   0 if all checks complete (non-fatal warnings do not cause failure);
#           exits 1 if a critical error is encountered (e.g., missing config).
# -----------------------------------------------------------------------------
self_test() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    log "Running self-test mode."
    check_apache_installed
    check_required_commands
    if [ -f "$APACHE_CONF" ]; then
        log "Success: Apache configuration file exists at $APACHE_CONF."
    else
        log "Error: Apache configuration file not found at $APACHE_CONF."
        exit 1
    fi
    if [ -f "$TARGET_FILE" ]; then
        log "Success: Target file exists at $TARGET_FILE."
    else
        log "Warning: Target file not found at $TARGET_FILE."
    fi
    log "Self-test completed successfully."

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Add the ServerName directive to the Apache configuration.
# @details  Checks if the ServerName directive defined in $SERVERNAME_DIRECTIVE
#           is present in the file specified by $APACHE_CONF. If it is not
#           found, inserts the directive at the top of the configuration file.
#
# @global   APACHE_CONF            Path to the Apache configuration file.
# @global   SERVERNAME_DIRECTIVE   The ServerName directive string to add.
#
# @return   0 on success.
# -----------------------------------------------------------------------------
add_servername_directive() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    if grep -qE '^[[:space:]]*'"$SERVERNAME_DIRECTIVE" "$APACHE_CONF"; then
        verbose_log "ServerName directive is already set in $APACHE_CONF."
    else
        log "Adding ServerName directive to $APACHE_CONF."
        run_command sudo sed -i "1i $SERVERNAME_DIRECTIVE" "$APACHE_CONF"
        log "ServerName directive added."
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Remove the ServerName directive from the Apache configuration.
# @details  Creates a backup of the Apache config file and deletes any line
#           matching the ServerName directive defined in $SERVERNAME_DIRECTIVE.
#
# @global   APACHE_CONF            Path to the Apache configuration file.
# @global   SERVERNAME_DIRECTIVE   The ServerName directive string to remove.
#
# @return   0 on success.
# -----------------------------------------------------------------------------
remove_servername_directive() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    if grep -q "^$SERVERNAME_DIRECTIVE" "$APACHE_CONF"; then
        log "Removing ServerName directive from $APACHE_CONF."
        run_command sudo sed -i "/^$SERVERNAME_DIRECTIVE/d" "$APACHE_CONF"
        log "ServerName directive removed."
    else
        verbose_log "ServerName directive not found in $APACHE_CONF."
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Create and enable a dedicated Apache site for Wsprry Pi.
# @details  Writes /etc/apache2/sites-available/wsprrypi.conf containing a
#           <VirtualHost *:80> definition that redirects “/” to “/wsprrypi/”.
#           Disables the default Debian site (000-default.conf) and enables
#           wsprrypi.conf so your redirect becomes the active virtual host.
#
# @param    $1 [Optional] Debug flag ("debug") to enable debug output.
#
# @return   0 on success; non-zero on failure.
# -----------------------------------------------------------------------------
setup_wsprrypi_site() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    local site_conf="/etc/apache2/sites-available/wsprrypi.conf"
    log "Creating site definition at $site_conf"
    # Suppress tee’s echo
    run_command sudo tee "$site_conf" <<'EOF' >/dev/null
<VirtualHost *:80>
    ServerName localhost
    ServerAlias *
    DocumentRoot /var/www/html
    RedirectMatch 301 ^/$ /wsprrypi/
</VirtualHost>
EOF

    log "Disabling default site (000-default.conf)"
    # Suppress a2dissite’s messages
    run_command sudo a2dissite 000-default.conf >/dev/null

    log "Enabling wsprrypi site"
    # Suppress a2ensite’s messages
    run_command sudo a2ensite wsprrypi.conf >/dev/null

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Disable the Wsprry Pi Apache site and restore the default.
# @details  Runs a2dissite on wsprrypi.conf, re-enables the Debian default
#           site (000-default.conf), and removes the wsprrypi.conf file from
#           sites-available to clean up after uninstall.
#
# @param    $1 [Optional] Debug flag ("debug") to enable debug output.
#
# @return   0 on success; non-zero on failure.
# -----------------------------------------------------------------------------
disable_wsprrypi_site() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    # Make sure we reference the same path as setup_wsprrypi_site
    local site_conf="/etc/apache2/sites-available/wsprrypi.conf"

    log "Disabling wsprrypi site"
    run_command sudo a2dissite wsprrypi.conf >/dev/null

    log "Re-enabling default site"
    run_command sudo a2ensite 000-default.conf >/dev/null

    log "Removing site file $site_conf"
    run_command sudo rm -f "$site_conf"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Test the Apache HTTP Server configuration for syntax correctness.
# @details  Runs `apache2ctl configtest`, filters out benign warnings
#           (e.g., FQDN notices), and logs whether the configuration is valid.
#           On failure, it captures and echoes the error output for review.
#
# @return   0 if the configuration test passes; 1 if it fails.
# -----------------------------------------------------------------------------
test_apache_config() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    log "Testing Apache configuration."
    # Run configtest once, capturing both stdout+stderr
    local output
    if output="$(apache2ctl configtest 2>&1)"; then
        # Success exit-code → test passed
        local extra
        # Drop the FQDN warning and Syntax OK, but never error
        extra="$(printf "%s\n" "$output" \
            | grep -v "Could not reliably determine the server's fully qualified domain name" \
            | grep -v "Syntax OK" \
            || true)"

        if [[ -n "$extra" ]]; then
            log "Warning: Apache test reported:"
            printf "%s\n" "$extra"
        fi
        log "Success: Apache configuration is syntactically OK."
        debug_end "$debug"
        return 0
    else
        log "Error: Apache configuration test failed. Output:"
        printf "%s\n" "$output"
        debug_end "$debug"
        return 1
    fi
}

# -----------------------------------------------------------------------------
# @brief    Restart the Apache HTTP Server service.
# @details  Invokes systemctl to restart the Apache2 service, ensuring that
#           any configuration changes take effect and the web server is
#           running with the latest settings.
#
# @return   0 on success; non-zero if the restart command fails.
# -----------------------------------------------------------------------------
restart_apache() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    log "Restarting Apache."
    run_command sudo systemctl restart apache2
    log "Success: Apache restarted successfully."

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief    Check if an HTML file is the stock Apache landing page.
# @param    $1  Path to the file to check (defaults to /var/www/html/index.html).
# @return   0 if it looks like the stock Apache page, 1 otherwise.
# -----------------------------------------------------------------------------
is_stock_apache_page() {
    local file="${1:-/var/www/html/index.html}"

    # must exist and be readable
    [[ -r "$file" ]] || return 1

    # common stock-page phrases (Ubuntu/Debian, RHEL/CentOS, generic)
    if grep -qiE \
       'It works!|Apache2 (Ubuntu|Debian) Default Page|If you see this page, the Apache HTTP Server must be installed correctly' \
       "$file"
    then
        return 0
    else
        return 1
    fi
}

# -----------------------------------------------------------------------------
# @brief Main execution function for the script.
# @details This function initializes the execution environment, processes
#          command-line arguments, verifies system compatibility, ensures
#          dependencies are installed, and executes either the install or
#          uninstall process for Wsprry Pi.
#
# @global ACTION Determines whether the script performs installation or
#                uninstallation.
#
# @param $@ Command-line arguments passed to the script.
#
# @throws Exits with an error code if any critical dependency is missing,
#         an unsupported environment is detected, or a step fails.
#
# @return 0 on successful execution, non-zero on failure.
#
# @note Debug mode (`"$debug"`) is propagated throughout all function calls.
# -----------------------------------------------------------------------------
# shellcheck disable=SC2317
_main() {
    local debug
    debug=$(debug_start "$@")
    eval set -- "$(debug_filter "$@")"

    local disable_mode=0

    # Make sure we are running with `sudo`
    enforce_sudo

    # Parse command-line options
    while getopts "unvlthf:c:o:s:" opt; do
        case $opt in
            u) disable_mode=1 ;;                     # Enable uninstall mode
            n) DRY_RUN=1 ;;                          # Enable dry-run mode
            v) VERBOSE=1 ;;                          # Enable verbose logging
            l) LOGGING_ENABLED=1 ;;                  # Enable logging
            t) SELF_TEST=1 ;;                        # Enable self-test mode
            f) TARGET_FILE="$OPTARG" ;;              # Custom target file
            c) APACHE_CONF="$OPTARG" ;;              # Custom Apache configuration file
            o) LOG_FILE="$OPTARG" ;;                 # Custom log file
            s) SERVERNAME_DIRECTIVE="$OPTARG" ;;     # Custom ServerName directive
            h) show_help; exit 0 ;;                  # Show help and exit
            *) show_help; exit 1 ;;                  # Show help for invalid options
        esac
    done

    # Run self-test mode if enabled
    if [ "$SELF_TEST" -eq 1 ]; then
        self_test
    fi

    # Ensure required commands and dependencies
    check_apache_installed
    check_required_commands

    # Rotate logs if logging is enabled
    if [ "$LOGGING_ENABLED" -eq 1 ]; then
        rotate_logs
    fi

    # Handle uninstall mode
    if [ "$disable_mode" -eq 1 ]; then
        log "Disabling configuration."
        backup_file "$APACHE_CONF"
        remove_servername_directive
        disable_wsprrypi_site
        test_apache_config
        restart_apache
        exit 0
    fi

    # Default behavior: Configure Apache if the target file exists
    if [ -f "$TARGET_FILE" ]; then
        if is_stock_apache_page; then
            log "File $TARGET_FILE exists. Proceeding with configuration."
            backup_file "$APACHE_CONF"
            add_servername_directive
            setup_wsprrypi_site
            test_apache_config
            restart_apache
        else
            log "Warning: File $TARGET_FILE exists but is not the stock Apache page. No actions taken."
        fi
    else
       log "Warning: File $TARGET_FILE does not exist. No actions taken."
    fi

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
# shellcheck disable=SC2317
main() {
    _main "$@"
    return "$?"
}

debug=$(debug_start "$@")
eval set -- "$(debug_filter "$@")"
main "$@" "$debug"
retval="$?"
debug_end "$debug"
exit "$retval"
