#!/bin/bash

# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.1-218cc4d [refactoring].

: <<'EOF'
This script manages Apache2 configurations and settings for the Wsprry Pi project.
It will:
  - Enable a rewrite from '/' to '/wspr' if the default Apache2 page is present
    in the root directory.
  - Add a 'ServerName' directive to localhost to suppress the warning:
    "Could not reliably determine the server's fully qualified domain name."
  - Uninstall these settings.
  - Test the Apache configuration after making changes.
  - Supports all options via environment variables or command-line arguments.
  - Automatically creates backups of modified configuration files.
  - Includes a self-test mode to validate setup before making changes.

EXAMPLES:
  - To execute the script directly:
      sudo ./apache_tool.sh -v -l -f /custom/index.html

  - To pass environment variables inline when executing directly:
      TARGET_FILE=/custom/index.html LOGGING_ENABLED=1 sudo ./apache_tool.sh

  - To pass environment variables when piping to bash:
      TARGET_FILE=/custom/index.html LOGGING_ENABLED=1 cat apache_tool.sh | sudo bash

  - To export environment variables globally:
      export TARGET_FILE=/custom/index.html
      export LOGGING_ENABLED=1
      cat apache_tool.sh | sudo bash

NOTE:
  - Command-line options take precedence over environment variables.
  - Environment variables override default values in the script.
EOF

# Ensure the script is run with sudo or as root
if [ "$(id -u)" -ne 0 ]; then
    echo "Error: This script must be run as root or with sudo privileges."
    exit 1
fi

# Defaults
DEFAULT_TARGET_FILE="/var/www/html/index.html"
DEFAULT_APACHE_CONF="/etc/apache2/apache2.conf"
DEFAULT_LOG_FILE="/var/log/apache_tool.log"
DEFAULT_SERVERNAME="ServerName localhost"
DEFAULT_DRY_RUN=0
DEFAULT_VERBOSE=0
DEFAULT_LOGGING_ENABLED=0
DEFAULT_SELF_TEST=0

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

# Help function
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

# Log function with date/time to the log file and colored console output
log() {
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
}

verbose_log() {
    [ "$VERBOSE" -eq 1 ] && log "$*"
}

rotate_logs() {
    local max_size=1048576  # 1MB
    if [ -f "$LOG_FILE" ] && [ "$(stat --format=%s "$LOG_FILE")" -gt "$max_size" ]; then
        mv "$LOG_FILE" "${LOG_FILE}-$(date '+%Y%m%d%H%M%S').backup"
        log "Log file rotated."
    fi
}

backup_file() {
    local file="$1"
    if [ -f "$file" ]; then
        local backup="${file}-$(date '+%Y%m%d%H%M%S').backup"
        cp "$file" "$backup"
        log "Backup created: $backup"
    else
        log "Warning: File $file does not exist. No backup created."
    fi
}

run_command() {
    if [ "$DRY_RUN" -eq 1 ]; then
        log "[Dry-Run] $*"
    else
        "$@" || { log "Error: Command '$*' failed."; exit 1; }
    fi
}

check_apache_installed() {
    if ! command -v apache2ctl >/dev/null 2>&1; then
        log "Error: Apache2 is not installed. Please install it and retry."
        exit 1
    fi
}

check_required_commands() {
    for cmd in apache2ctl sed grep; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            log "Error: Required command '$cmd' not found. Please install it and retry."
            exit 1
        fi
    done
}

self_test() {
    log "Running self-test mode..."
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
    exit 0
}

add_servername_directive() {
    backup_file "$APACHE_CONF"
    if grep -q "^$SERVERNAME_DIRECTIVE" "$APACHE_CONF"; then
        verbose_log "ServerName directive is already set in $APACHE_CONF."
    else
        log "Adding ServerName directive to $APACHE_CONF..."
        run_command sudo sed -i "1i $SERVERNAME_DIRECTIVE" "$APACHE_CONF"
        log "ServerName directive added."
    fi
}

remove_servername_directive() {
    backup_file "$APACHE_CONF"
    if grep -q "^$SERVERNAME_DIRECTIVE" "$APACHE_CONF"; then
        log "Removing ServerName directive from $APACHE_CONF..."
        run_command sudo sed -i "/^$SERVERNAME_DIRECTIVE/d" "$APACHE_CONF"
        log "ServerName directive removed."
    else
        verbose_log "ServerName directive not found in $APACHE_CONF."
    fi
}

test_apache_config() {
    log "Testing Apache configuration..."
    APACHE_TEST_OUTPUT=$(apache2ctl configtest 2>&1 | grep -v "Could not reliably determine the server's fully qualified domain name" | grep -v "Syntax OK")
    if apache2ctl configtest 2>&1 | grep -q "Syntax OK"; then
        log "Success: Apache configuration test passed."
        return 0
    else
        log "Error: Apache configuration test failed. Output:"
        echo "$APACHE_TEST_OUTPUT"
        return 1
    fi
}

restart_apache() {
    log "Restarting Apache..."
    run_command sudo systemctl restart apache2
    log "Success: Apache restarted successfully."
}

main() {
    local disable_mode=0

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
        log "Disabling configuration..."
        remove_servername_directive
        test_apache_config
        restart_apache
        exit 0
    fi

    # Default behavior: Configure Apache if the target file exists
    if [ -f "$TARGET_FILE" ]; then
        log "File $TARGET_FILE exists. Proceeding with configuration."
        add_servername_directive
        test_apache_config
        restart_apache
    else
        log "Warning: File $TARGET_FILE does not exist. No actions taken."
    fi

    exit 0
}

# Entry point
main "$@"
