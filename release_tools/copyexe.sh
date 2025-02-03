#!/usr/bin/env bash
set -uo pipefail
IFS=$'\n\t'

##
# @file copyexe.sh
# @brief Updates and manages WsprryPi scripts and services.
#
# @details
# This script updates WsprryPi-related files, manages the shutdown button service,
# and ensures proper setup and functionality of the `wspr` service.
#
# @author Lee Bussy
# @date December 21, 2024
# @version 1.0.0
#
# @par Usage:
# ```bash
# ./copyexe.sh
# ```
#
# @requirements
# - `systemctl` for managing services.
# - `sudo` privileges for file and service updates.
#
# @warning
# Ensure the script is executed in the context of a Git repository.
##

# -----------------------------------------------------------------------------
# @brief Get the root directory of the current Git repository.
# @return Sets the global variable `repo_root` with the root directory path.
# @retval 0 on success.
# @retval 1 if not inside a Git repository.
# -----------------------------------------------------------------------------
get_repo_root() {
    repo_root=$(git rev-parse --show-toplevel 2>/dev/null)
    if [ -z "$repo_root" ]; then
        echo "Not in a Git repository."
        exit 1
    fi
}

# -----------------------------------------------------------------------------
# @brief Check if WsprryPi is installed.
# @retval 0 if installed.
# @retval 1 if not installed.
# -----------------------------------------------------------------------------
check_wspri_installed() {
    if ! systemctl list-unit-files | grep -q "^wspr"; then
        echo "WsprryPi is not installed."
        echo "Please use install.sh to set up the environment for the first time."
        exit 1
    fi
}

# -----------------------------------------------------------------------------
# @brief Stop WsprryPi and shutdown-related services.
# -----------------------------------------------------------------------------
stop_services() {
    sudo systemctl stop wspr 2>/dev/null || true
    sudo systemctl stop shutdown-button 2>/dev/null || true
    sudo systemctl stop shutdown_button 2>/dev/null || true
}

# -----------------------------------------------------------------------------
# @brief Copy new WsprryPi-related files to their appropriate locations.
# -----------------------------------------------------------------------------
copy_files() {
    sudo cp -f "$repo_root"/scripts/wspr /usr/local/bin
    sudo cp -f "$repo_root"/scripts/wspr.ini /usr/local/etc
    sudo cp -f "$repo_root"/scripts/logrotate.d /etc/logrotate.d/wspr

    # Refresh shutdown button if it was installed
    sudo rm -f "/usr/local/bin/shutdown-watch.py"
    if systemctl list-unit-files | grep -q "^shutdown-button"; then
        sudo cp -f "$repo_root"/scripts/shutdown_button.py /usr/local/bin
    fi

    # Ensure log directory exists
    [[ -d "/var/log/wspr" ]] || sudo mkdir "/var/log/wspr"
}

# -----------------------------------------------------------------------------
# @brief Reload the systemd daemon and restart WsprryPi services.
# -----------------------------------------------------------------------------
restart_services() {
    sudo systemctl daemon-reload
    sudo systemctl start wspr

    # Handle shutdown button service
    if systemctl list-unit-files | grep -q "^shutdown-button"; then
        sudo rm -fr /etc/systemd/system/shutdown-button.service
    fi
    if systemctl list-unit-files | grep -q "^shutdown-watch"; then
        sudo rm -fr /etc/systemd/system/shutdown-watch.service
    fi
    if systemctl list-unit-files | grep -q "^shutdown_watch"; then
        sudo systemctl start shutdown_watch
    else
        echo "shutdown_watch.service is not installed, re-run installer."
    fi
}

# -----------------------------------------------------------------------------
# @brief Main function orchestrating the script execution.
# -----------------------------------------------------------------------------
main() {
    get_repo_root
    check_wspri_installed
    stop_services
    copy_files
    restart_services
    echo "Executables copied."
}

# Invoke the main function
main "$@"
