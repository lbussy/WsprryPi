#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# -----------------------------------------------------------------------------
# @file copyexe.sh
# @brief Updates and manages WsprryPi scripts and services.
#
# @details
# This script updates WsprryPi-related files, manages the shutdown button service,
# and ensures proper setup and functionality of the `wsprrypi` service.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.2.1-update_ui+54.7716cd3-dirty
# @date 2025-02-17
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
# ./copydocs.sh
#
# @warning
# Ensure the script is executed in the context of a Git repository.
#
# @requirements
# - `systemctl` for managing services.
# - `sudo` privileges for file and service updates.
#
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# @brief Get the root directory of the current Git repository.
# @return Returns `repo_root` with the root directory path.
# @retval 0 on success.
# @retval 1 if not inside a Git repository.
# -----------------------------------------------------------------------------
get_repo_root() {
    local repo_root
    repo_root=$(git rev-parse --show-toplevel 2>/dev/null || true)

    if [ -z "$repo_root" ]; then
        printf "Error: Not inside a Git repository.\n" >&2
        return 1
    fi

    printf "%s\n" "$repo_root"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check if WsprryPi is installed.
# @details
# This function checks if the WsprryPi service is installed by looking for
# an active unit file. If not installed, it suggests using `install.sh` to
# set up the environment.
#
# @retval 0 if installed.
# @retval 1 if not installed.
# -----------------------------------------------------------------------------
check_wspri_installed() {
    # Check for wsprrypi service; if it fails, don't exit immediately due to `set -e`
    if ! systemctl list-units --type=service | grep -q "^wsprrypi"; then
        # Print error and return 1 if wsprrypi is not found
        printf "Error: WsprryPi is not installed.\n" >&2
        printf "Please use install.sh to set up the environment.\n" >&2
        return 1
    fi
    return 0
}

# -----------------------------------------------------------------------------
# @brief Stop WsprryPi and shutdown-related services.
# @details
# This function stops the following services:
# - `wsprrypi` service.
# - `shutdown-button` service.
# - `wspr_watch` service.
#
# If any service fails to stop, an error message is displayed, but the function
# continues attempting to stop the other services.
#
# @retval 0 if all services were stopped successfully.
# @retval 1 if any service failed to stop.
# -----------------------------------------------------------------------------
stop_services() {
    # Stop the Wspr service
    if ! sudo systemctl stop wsprrypi 2>/dev/null; then
        printf "Error: Failed to stop the WsprryPi service.\n" >&2
        return 1
    fi

    # Stop the shutdown-button service
    if ! sudo systemctl stop shutdown-button 2>/dev/null; then
        printf "Error: Failed to stop the shutdown-button service.\n" >&2
        return 1
    fi

    # Stop the wspr_watch service
    if ! sudo systemctl stop wspr_watch 2>/dev/null; then
        printf "Error: Failed to stop the wspr_watch service.\n" >&2
        return 1
    fi

    return 0
}

# -----------------------------------------------------------------------------
# @brief Copy new WsprryPi-related files to their appropriate locations.
# @details
# This function copies necessary files from the repository to their proper
# locations on the system. It handles:
# - Copying the `wsprrypi` script to `/usr/local/bin`.
# - Copying the `wsprrypi.ini` configuration file to `/usr/local/etc`.
# - Copying the logrotate configuration to `/etc/logrotate.d/wsprrypi`.
# - Refreshing the shutdown button file if the service is installed.
# - Ensuring that the log directory exists.
#
# If any operation fails, an error message is printed and the function returns
# with a non-zero status.
#
# @param repo_root The root directory of the Git repository.
#                  This is used to locate the necessary files for copying.
#
# @retval 0 if all files were copied and necessary directories created.
# @retval 1 if any operation failed.
# -----------------------------------------------------------------------------
copy_files() {
    local repo_root
    repo_root="${1:-}"

    # Copy Wspr script to /usr/local/bin
    if ! sudo cp -f "$repo_root/scripts/wsprrypi" /usr/local/bin; then
        printf "Error: Failed to copy wsprrypi script to /usr/local/bin.\n" >&2
        return 1
    fi

    # Copy Wspr configuration file to /usr/local/etc
    if ! sudo cp -f "$repo_root/scripts/wsprrypi.ini" /usr/local/etc; then
        printf "Error: Failed to copy wsprrypi.ini to /usr/local/etc.\n" >&2
        return 1
    fi

    # Copy logrotate configuration to /etc/logrotate.d/wsprrypi
    if ! sudo cp -f "$repo_root/scripts/logrotate.d" /etc/logrotate.d/wsprrypi; then
        printf "Error: Failed to copy logrotate configuration to /etc/logrotate.d/wsprrypi.\n" >&2
        return 1
    fi

    # Refresh shutdown button if it was installed
    sudo rm -f "/usr/local/bin/shutdown-watch.py" || return 1
    if systemctl list-unit-files --type=service | grep -q "^shutdown-button"; then
        if ! sudo cp -f "$repo_root/scripts/wspr_watch.py" /usr/local/bin; then
            printf "Error: Failed to copy wspr_watch.py to /usr/local/bin.\n" >&2
            return 1
        fi
    fi

    # Ensure log directory exists
    if [ ! -d "/var/log/wsprrypi" ]; then
        if ! sudo mkdir -p "/var/log/wsprrypi"; then
            printf "Error: Failed to create log directory /var/log/wsprrypi.\n" >&2
            return 1
        fi
    fi

    return 0
}

# -----------------------------------------------------------------------------
# @brief Reload the systemd daemon and restart WsprryPi services.
# @details
# This function performs the following actions:
# - Reloads the systemd daemon to apply any changes.
# - Starts the `wsprrypi` service.
# - If the shutdown button service is installed, it removes its service file.
# - Checks and restarts the `shutdown_watch` service if it's installed.
#
# @retval 0 on success.
# @retval 1 if any step fails.
# -----------------------------------------------------------------------------
restart_services() {
    # Reload the systemd daemon
    if ! sudo systemctl daemon-reload; then
        printf "Error: Failed to reload the systemd daemon.\n" >&2
        return 1
    fi

    # Start the WsprryPi service
    if ! sudo systemctl start wsprrypi; then
        printf "Error: Failed to start the WsprryPi service.\n" >&2
        return 1
    fi

    # Handle shutdown button service removal if installed
    if systemctl list-unit-files --type=service | grep -q "^shutdown-button"; then
        if ! sudo rm -fr /etc/systemd/system/shutdown-button.service; then
            printf "Error: Failed to remove shutdown-button service.\n" >&2
            return 1
        fi
    fi

    # Handle shutdown-watch service removal if installed
    if systemctl list-unit-files --type=service | grep -q "^shutdown-watch"; then
        if ! sudo rm -fr /etc/systemd/system/shutdown-watch.service; then
            printf "Error: Failed to remove shutdown-watch service.\n" >&2
            return 1
        fi
    fi

    # Handle shutdown_watch service
    if systemctl list-unit-files --type=service | grep -q "^shutdown_watch"; then
        if ! sudo systemctl start shutdown_watch; then
            printf "Error: Failed to start shutdown_watch service.\n" >&2
            return 1
        fi
    else
        printf "shutdown_watch.service is not installed. Please re-run the installer.\n" >&2
    fi

    return 0
}

# -----------------------------------------------------------------------------
# @brief Main function orchestrating the script execution.
# @details
# This function coordinates the overall flow of the script by performing the
# following tasks:
# - Retrieves the root directory of the Git repository using `get_repo_root`.
# - Checks if WsprryPi is installed using `check_wspri_installed`.
# - Stops necessary services using `stop_services`.
# - Copies the required files using `copy_files`.
# - Restarts the services using `restart_services`.
#
# If any step fails, an error message is printed and the script exits early.
#
# @retval 0 on successful execution of all tasks.
# @retval 1 if any task fails.
# -----------------------------------------------------------------------------
main() {
    local repo_root

    # Get the root directory of the Git repository
    repo_root=$(get_repo_root)
    if [ $? -ne 0 ]; then
        printf "Error: Failed to get the Git repository root.\n" >&2
        return 1
    fi

    # Check if WsprryPi is installed
    check_wspri_installed
    if [ $? -ne 0 ]; then
        printf "Error: WsprryPi is not installed.\n" >&2
        return 1
    fi

    # Stop necessary services
    stop_services
    if [ $? -ne 0 ]; then
        printf "Error: Failed to stop required services.\n" >&2
        return 1
    fi

    # Copy required files
    copy_files "$repo_root"
    if [ $? -ne 0 ]; then
        printf "Error: Failed to copy necessary files.\n" >&2
        return 1
    fi

    # Restart services
    restart_services
    if [ $? -ne 0 ]; then
        printf "Error: Failed to restart services.\n" >&2
        return 1
    fi

    printf "Executables copied and services restarted successfully.\n"
    return 0
}

# Invoke the main function and handle errors properly
main "$@"
retval="$?"
if [[ $retval -ne 0 ]]; then
    printf "Failed to copy executables.\n" >&2
    exit "$retval"
fi

# If the main function succeeds, exit normally
exit 0
