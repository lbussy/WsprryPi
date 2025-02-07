#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# -----------------------------------------------------------------------------
# @file old_version_cleanup.sh
# @brief Cleanup Wsprry Pi versions < 1.3.0.
# @details Cleans all files and folders used by versions before 1.3.0.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.2.1-remove_bcm+109.59592e9
# @date 2025-02-05
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
#
# @usage
# sudo ./old_version_cleanup.sh
#
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# @brief Removes specified files and directories from the system.
#
# @details This function attempts to remove files and directories listed in
#          the predefined array or passed in as arguments. It checks whether
#          each file or directory exists, and confirms with the user before
#          removing items if unexpected dependencies or locations are detected.
#          All errors are suppressed and warnings are printed to the terminal.
#
# @param files_and_dirs Array of files and directories to remove. If no
#                       arguments are provided, default files and directories
#                       will be used.
#
# @return 0 if all files and directories were successfully removed, 1 if any
# failure occurred.
#
# -----------------------------------------------------------------------------
remove_files_and_dirs() {
    # Accept files and directories as arguments or use a default list if none provided
    local files_and_dirs=("$@")
    if [ ${#files_and_dirs[@]} -eq 0 ]; then
        files_and_dirs=(
            "/usr/local/bin/wspr"
            "/usr/local/etc/wspr.ini"
            "/usr/local/bin/shutdown-button.py"
            "/usr/local/bin/shutdown-watch.py"
            "/usr/local/bin/shutdown_watch.py"
            "/var/www/html/wspr/"
            "/var/log/wspr/"
            "/var/log/wsprrypi/"
            "/etc/logrotate.d/wspr/"
            "/etc/logrotate.d/wsprrypi/"
        )
    fi

    local retval=0  # Initialize return value

    # Loop through each file or directory in the array
    for item in "${files_and_dirs[@]}"; do
        # Check if the item exists
        if [ -e "$item" ]; then
            printf "Removing: %s\n" "$item"

            # Ask for confirmation if the item is a directory and it's not in the default list
            if [ -d "$item" ]; then
                read -p "Do you want to continue and remove directory $item? (y/n): " confirm < /dev/tty || true
                if [[ "$confirm" != "y" ]]; then
                    printf "Skipping removal of directory: %s\n" "$item"
                    continue
                fi
            fi

            # Remove the item (file or directory)
            if ! rm -rf "$item" 2>/dev/null; then
                printf "Warning: Failed to remove %s. It may not exist or may not be removable.\n" "$item"
                retval=1
            fi
        else
            printf "Item '%s' does not exist or is not found.\n" "$item"
        fi
    done

    # Return the status of the operation
    return "$retval"
}

# -----------------------------------------------------------------------------
# @brief Removes specified services from systemd if they exist and are enabled.
#
# @details This function attempts to stop the service only if it is running,
#          disable it if it is enabled, remove the service unit file (even if
#          the service was never installed), and always resets the failed state
#          for each service. The user will be asked for confirmation if any
#          unexpected dependencies are found. After all services are processed,
#          the systemd daemon is reloaded to apply the changes.
#
# @param services List of service names to remove. If no arguments are provided,
#                 default services will be used.
#
# @return 0 if all services were successfully processed, 1 if any failure
# occurred.
#
# -----------------------------------------------------------------------------
remove_services() {
    # Accept services as arguments or use a default list if none provided
    local services=("$@")
    if [ ${#services[@]} -eq 0 ]; then
        services=("wspr" "shutdown-button" "shutdown-watch" "shutdown_watch")
    fi

    local retval=0  # Initialize return value

    # Loop through each service name in the array
    for service_name in "${services[@]}"; do
        # Check if the service exists in systemd
        if systemctl list-units --all | grep -q "$service_name.service"; then
            printf "Removing service: '%s'\n" "$service_name"

            # Check for dependencies using systemctl list-dependencies
            dependencies=$(systemctl list-dependencies "$service_name.service" 2>/dev/null)

            # Filter out self-references from the dependencies list
            dependencies=$(echo "$dependencies" | grep -v "^  $service_name.service")

            if [ -n "$dependencies" ]; then
                printf "Warning: '%s.service' has dependencies. Please review before removing.\n" "$service_name"
                read -p "Do you want to continue and remove this service? (y/n): " confirm
                if [[ "$confirm" != "y" ]]; then
                    printf "Skipping removal of '%s.service' due to dependencies.\n" "$service_name"
                    continue
                fi
            fi

            # Stop the service only if it is running
            if systemctl is-active --quiet "$service_name.service"; then
                if ! systemctl stop "$service_name.service"; then
                    printf "Error: Failed to stop '%s.service': %s\n" "$service_name" "$(systemctl status "$service_name.service" 2>/dev/null)"
                    retval=1
                fi
            fi

            # Disable the service only if it is enabled
            if systemctl is-enabled --quiet "$service_name.service"; then
                if ! systemctl disable "$service_name.service"; then
                    printf "Error: Failed to disable '%s.service': %s\n" "$service_name" "$(systemctl status "$service_name.service" 2>/dev/null)"
                    retval=1
                fi
            fi
        else
            # If the service does not exist or is not loaded, skip it entirely
            printf "Service '%s.service. does not exist or is not loaded. Skipping.\n" "$service_name"
            continue  # Skip this service if it doesn't exist
        fi
    done

    # Move service file deletion outside the loop to handle if the service was never installed
    for service_name in "${services[@]}"; do
        for dir in /etc/systemd/system /lib/systemd/system /usr/lib/systemd/system; do
            # Check if the service file exists
            if [ -f "$dir/$service_name.service" ]; then
                printf "Removing service file: %s\n" "$dir/$service_name.service"
                if ! rm -f "$dir/$service_name.service"; then
                    printf "Warning: Failed to remove '%s.service'. It may not exist.\n" "$service_name"
                    retval=1
                fi
            fi
        done
    done

    # Always run systemctl reset-failed, suppress errors if it fails
    printf "Resetting failed states.\n"
    for service_name in "${services[@]}"; do
        systemctl reset-failed "$service_name.service" 2>/dev/null
    done

    # Reload systemd to apply changes
    printf "Reloading systemd daemon.\n"
    systemctl daemon-reload
    printf "Systemd daemon reloaded.\n"

    return "$retval"  # Return the status of the operation
}

# -----------------------------------------------------------------------------
# @brief Main execution function for the script.
# @details This function orchestrates the uninstall process for legacy
#          Wsprry Pi serbices, files, and folders.
#
# @param $@ Command-line arguments passed to the script.
#
# @return 0 on successful execution, non-zero on failure.
#
# @note This function performs the removal of files, directories, and services
#       by calling appropriate sub-functions and returns the status of their
#       execution.
# -----------------------------------------------------------------------------
_main() {
    local retval=0  # Initialize the return value to 0 for successful execution.

    printf "Beginning cleanup of older versions of Wsprry Pi.\n"

    # Call remove_files_and_dirs and capture its return value
    remove_files_and_dirs "$@"
    local retval_files_and_dirs=$?  # Capture the return value of remove_files_and_dirs
    retval=$((retval + retval_files_and_dirs))  # Accumulate the result

    # Call remove_services and capture its return value
    remove_services "$@"
    local retval_services=$?  # Capture the return value of remove_services
    retval=$((retval + retval_services))  # Accumulate the result

    # Return the final status: 0 if all steps succeeded, non-zero if any failed
    if [ "$retval" -gt 0 ]; then
        return 1  # If any failure occurred, return non-zero status
    else
        return 0  # All succeeded
    fi
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
main() {
    _main "$@"
    return "$?"  # Ensure the return value from _main is passed along correctly
}

main "$@"
retval="$?"
exit "$retval" # Exit with the status returned by main
