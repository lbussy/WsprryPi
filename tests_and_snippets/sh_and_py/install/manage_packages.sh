#!/bin/bash
#
# @file install_update_packages.sh
# @brief Script to manage package installation, upgrading, and uninstallation.
#
# @description
#   This script provides options to install/upgrade or uninstall the packages 
#   defined in the APTPACKAGES list. It suppresses command output and displays 
#   custom success or failure messages for each operation.
#
# @todo Extend support for dynamic package lists passed as arguments.
# @todo Add error handling for missing sudo privileges.
# @todo Optimize uninstallation cleanup to include logs and temporary files.
#
# @author [Your Name]
# @date [YYYY-MM-DD]

# Source the logging functions file
if [[ -f ./micro_log.sh ]]; then
    source ./micro_log.sh
else
    echo "Error: Logging functions file (micro_log.sh) not found."
    exit 1
fi

# Global variable: List of required packages
readonly APTPACKAGES=("apache2" "php" "jq" "libraspberrypi-dev" "raspberrypi-kernel-headers")

# @brief Executes a command silently and logs results.
#
# @param[in] command Command to execute.
#
# @return Returns 0 for success, 1 for failure.
run_command() {
    local command="$1"

    if eval "$command" &>/dev/null; then
        return 0
    else
        return 1
    fi
}

# @brief Installs or upgrades all packages in the APTPACKAGES list.
#
# @details
#   Updates the package list and resolves broken dependencies before proceeding.
#
# @return Logs the success or failure of each operation.
install_update_packages() {
    local package

    logI "Updating local apt cache and managing required packages (this may take a few minutes)."

    # Update package list and fix broken installs
    if ! run_command "sudo apt-get update -y && sudo apt-get install -f -y"; then
        logE "Failed to update package list or fix broken installs."
        return 1
    fi

    # Install or upgrade each package in the list
    for package in "${APTPACKAGES[@]}"; do
        if dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q "install ok installed"; then
            if ! run_command "sudo apt-get install --only-upgrade -y $package"; then
                logW "Failed to upgrade package: $package. Continuing with the next package."
            fi
        else
            if ! run_command "sudo apt-get install -y $package"; then
                logW "Failed to install package: $package. Continuing with the next package."
            fi
        fi
    done

    logI "Package Installation Summary: All operations are complete."
    return 0
}

# @brief Uninstalls all packages in the APTPACKAGES list.
#
# @details
#   Removes the specified packages and cleans up orphaned dependencies.
#
# @return Logs the success or failure of each operation.
uninstall_packages() {
    local package

    logI "Removing apt packages (this may take a few minutes)."

    # Remove each package in the list
    for package in "${APTPACKAGES[@]}"; do
        if dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q "install ok installed"; then
            if ! run_command "sudo apt-get remove -y $package"; then
                logW "Failed to uninstall package: $package. Continuing with the next package."
            fi
        else
            logD "Skipping package $package. Not installed."
        fi
    done

    # Clean up unnecessary packages
    logI "Cleaning up unnecessary packages."
    if ! run_command "sudo apt-get autoremove -y"; then
        logW "Failed to clean up unnecessary packages."
    fi

    logI "Package Uninstallation Summary: All operations are complete."
    return 0
}

# Main Script Execution
clear
echo -e "Select an option:"
echo
echo -e "1. Install/Upgrade Packages"
echo -e "2. Uninstall Packages"
echo
read -rp "Enter your choice (1/2): " choice
echo
if [[ "$choice" -eq 1 ]]; then
    if ! install_update_packages; then
        logE "Installation/Upgrade process encountered errors. Exiting."
        exit 1
    fi
elif [[ "$choice" -eq 2 ]]; then
    if ! uninstall_packages; then
        logE "Uninstallation process encountered errors. Exiting."
        exit 1
    fi
else
    logW "Invalid choice. Exiting."
    exit 1
fi
