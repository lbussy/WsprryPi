#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# -----------------------------------------------------------------------------
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
# @par Usage:
# ./copysite.sh
#
# @requirements
# - `git` must be installed and accessible.
# - `sudo` privileges for managing web server directories.
#
# @warning
# Ensure the script is executed in the context of a Git repository.
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
# @brief Deploy website files to the web server directory.
# @details
# This function performs the following tasks:
# - Cleans up the target directory by removing and recreating it.
# - Copies the data files from the repository to the web server directory.
# - Creates a symbolic link to the `wsprrypi.ini` configuration file.
# - Sets the appropriate ownership for the files.
#
# If any step fails, an error message is printed and the function returns with a
# non-zero status.
#
# @param repo_root The root directory of the Git repository, used to locate
#                  the data files.
#
# @retval 0 on successful execution.
# @retval 1 if any step fails.
# -----------------------------------------------------------------------------
deploy_website() {
    local repo_root
    repo_root="${1:-}"
    local web_dir="/var/www/html/wsprrypi"

    # Remove existing web directory and recreate it
    if ! sudo rm -fr "$web_dir"; then
        printf "Error: Failed to remove existing directory %s.\n" "$web_dir" >&2
        return 1
    fi

    if ! sudo mkdir -p "$web_dir"; then
        printf "Error: Failed to create directory %s.\n" "$web_dir" >&2
        return 1
    fi

    # Copy data files to the web directory
    if ! sudo cp -R "$repo_root"/data/* "$web_dir/"; then
        printf "Error: Failed to copy data files to %s.\n" "$web_dir" >&2
        return 1
    fi

    # Create a symbolic link to the configuration file
    if ! sudo ln -sf /usr/local/etc/wsprrypi.ini "$web_dir/wsprrypi.ini"; then
        printf "Error: Failed to create symbolic link for wsprrypi.ini.\n" >&2
        return 1
    fi

    # Set appropriate ownership
    if ! sudo chown -R www-data:www-data "$web_dir"; then
        printf "Error: Failed to set ownership for %s.\n" "$web_dir" >&2
        return 1
    fi

    echo "Website deployed successfully."
    return 0
}

# -----------------------------------------------------------------------------
# @brief Main function orchestrating the script execution.
# @details
# This function coordinates the flow of the script, ensuring the following tasks:
# - Retrieves the root directory of the Git repository using `get_repo_root`.
# - Deploys the website files using `deploy_website`.
#
# If any task fails, an error message is printed and the script exits early.
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

    # Deploy the website
    deploy_website "$repo_root"
    if [ $? -ne 0 ]; then
        printf "Error: Failed to deploy website.\n" >&2
        return 1
    fi

    printf "Website deployed successfully.\n"
    return 0
}

# Invoke the main function and handle errors properly
main "$@"
retval="$?"
if [[ $retval -ne 0 ]]; then
    printf "Failed to copy website.\n" >&2
    exit "$retval"
fi

# If the main function succeeds, exit normally
exit 0
