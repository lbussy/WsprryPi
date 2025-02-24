#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# -----------------------------------------------------------------------------
# @file copydocs.sh
# @brief Updates and deploys documentation for a Git repository.
#
# @details
# This script automates the process of cleaning, building, and deploying documentation
# generated from a Git repository. It performs the following steps:
# - Verifies the script is executed inside a Git repository.
# - Cleans and rebuilds the documentation using `make` commands.
# - Deploys the generated documentation to a specified directory.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.2.1-timing_loop+67.477644f-dirty
# @date 2025-02-23
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
# ./copydocs.sh
#
# @warning
# This script requires `sudo` privileges for deploying the documentation.
#
# @requirements
# - `git` must be installed and available in the PATH.
# - `make` must be installed and configured to build the documentation.
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
# @brief Clean and build documentation.
# @details Executes `make clean` and `make html` in the `docs` directory.
# @retval 0 on success.
# @retval 1 if the `docs` directory is not found.
# -----------------------------------------------------------------------------
build_docs() {
    local repo_root docs_dir
    repo_root="${1:-}"
    docs_dir="$repo_root/docs"

    # Check if docs directory exists
    if [ ! -d "$docs_dir" ]; then
        printf "Error: Docs directory not found at %s.\n" "$docs_dir" >&2
        return 1
    fi

    # Clean and build documentation
    if ! (cd "$docs_dir" && make clean && make html); then
        printf "Error: Failed to clean and build documentation in %s.\n" "$docs_dir" >&2
        return 1
    fi

    return 0
}

# -----------------------------------------------------------------------------
# @brief Deploy the built documentation to the web server directory.
# @details
# This function copies the generated documentation from the specified
# source directory to the destination directory on the web server. It
# performs the following tasks:
# - Verifies that the source directory containing the built documentation
#   exists.
# - Removes any existing documentation at the destination directory.
# - Creates the destination directory if it doesn't already exist.
# - Copies the newly built documentation to the destination directory.
# - Sets appropriate ownership and permissions for the copied files and
#   directories.
#
# @param repo_root The root directory of the Git repository.
#                  This is used to locate the `docs/_build/html` source
#                  directory.
#
# @retval 0 on success.
# @retval 1 if the source directory is not found, or if any error occurs
#         during the deployment process.
#
# @example
# deploy_docs "/path/to/repo"
# -----------------------------------------------------------------------------
deploy_docs() {
    local repo_root src_dir dest_dir
    repo_root="${1:-}"
    dest_dir="/var/www/html/wsprrypi/docs"
    src_dir="$repo_root/docs/_build/html"

    # Check if the source directory exists
    if [ ! -d "$src_dir" ]; then
        printf "Error: Docs source not found at %s.\n" "$src_dir" >&2
        return 1
    fi

    # Remove existing destination and prepare for deployment
    sudo rm -fr "$dest_dir" || {
        printf "Error: Failed to remove existing destination directory %s.\n" "$dest_dir" >&2
        return 1
    }
    sudo mkdir -p "$dest_dir" || {
        printf "Error: Failed to create destination directory %s.\n" "$dest_dir" >&2
        return 1
    }

    # Copy the new docs to the destination directory
    sudo cp -R "$src_dir"/* "$dest_dir" || {
        printf "Error: Failed to copy docs from %s to %s.\n" "$src_dir" "$dest_dir" >&2
        return 1
    }

    # Set ownership for the entire directory
    sudo chown -R www-data:www-data "$dest_dir" || {
        printf "Error: Failed to set ownership for %s.\n" "$dest_dir" >&2
        return 1
    }

    # Set permissions for directories and files
    sudo find "$dest_dir" -type d -exec chmod 755 {} + || {
        printf "Error: Failed to set permissions for directories in %s.\n" "$dest_dir" >&2
        return 1
    }
    sudo find "$dest_dir" -type f -exec chmod 644 {} + || {
        printf "Error: Failed to set permissions for files in %s.\n" "$dest_dir" >&2
        return 1
    }

    printf "Docs successfully copied to %s.\n" "$dest_dir"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Main function orchestrating the script execution.
# @details
# This function is the entry point of the script, coordinating the following tasks:
# - Retrieves the root directory of the current Git repository using `get_repo_root`.
# - Initiates the documentation build process using `build_docs`.
# - Deploys the built documentation to the specified directory using `deploy_docs`.
#
# If any of these steps fail, the script exits early, returning an error code and
# printing the relevant error message to the user.
#
# @retval 0 on successful execution of all tasks.
# @retval 1 if any task (getting repo root, building docs, deploying docs) fails.
#
# @example
# main
# -----------------------------------------------------------------------------
main() {
    local repo_root
    repo_root=$(get_repo_root) || { printf "Error: Failed to get Git repository root.\n" >&2; return 1; }
    build_docs "$repo_root" || { printf "Error: Failed to build documentation.\n" >&2; return 1; }
    deploy_docs "$repo_root" || { printf "Error: Failed to deploy documentation.\n" >&2; return 1; }
    printf "Docs copied successfully.\n"
    return 0
}

# Invoke the main function and handle errors properly
main "$@"
retval="$?"
if [[ $retval -ne 0 ]]; then
    printf "Failed to copy docs.\n" >&2
    exit "$retval"
fi

# If the main function succeeds, exit normally
exit 0
