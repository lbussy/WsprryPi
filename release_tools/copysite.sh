#!/usr/bin/env bash
set -uo pipefail
IFS=$'\n\t'

##
# @file copysite.sh
# @brief Updates and deploys the WsprryPi website files.
#
# @details
# This script automates the deployment of website files for WsprryPi. It ensures:
# - Proper cleanup and recreation of the target directory.
# - Copying of data files to the web directory.
# - Creation of symbolic links for configuration files.
# - Setting appropriate ownership for the web server.
#
# @author Lee Bussy
# @date December 21, 2024
# @version 1.0.0
#
# @par Usage:
# ```bash
# ./copysite.sh
# ```
#
# @requirements
# - Git must be installed and accessible.
# - `sudo` privileges for managing web server directories.
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
# @brief Deploy website files to the web server directory.
# @details Cleans up the target directory, copies files, and sets up ownership.
# -----------------------------------------------------------------------------
deploy_website() {
    local web_dir="/var/www/html/wspr"

    # Remove existing directory and recreate it
    sudo rm -fr "$web_dir"
    sudo mkdir -p "$web_dir"

    # Copy data files to the web directory
    sudo cp -R "$repo_root"/data/* "$web_dir/"

    # Create a symbolic link to the configuration file
    sudo ln -sf /usr/local/etc/wspr.ini "$web_dir/wspr.ini"

    # Set appropriate ownership
    sudo chown -R www-data:www-data "$web_dir"

    echo "Website copied."
}

# -----------------------------------------------------------------------------
# @brief Main function orchestrating the script execution.
# -----------------------------------------------------------------------------
main() {
    get_repo_root
    deploy_website
}

# Invoke the main function
main "$@"
