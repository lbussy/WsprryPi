#!/usr/bin/env bash
set -uo pipefail
IFS=$'\n\t'

##
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
# @author Lee Bussy
# @date December 21, 2024
# @version 1.0.0
#
# @par Usage:
# ```bash
# ./copydocs.sh
# ```
#
# @warning
# This script requires `sudo` privileges for deploying the documentation.
#
# @requirements
# - Git must be installed and available in the PATH.
# - `make` must be installed and configured to build the documentation.
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
# @brief Clean and build documentation.
# @details Executes `make clean` and `make html` in the `docs` directory.
# @retval 0 on success.
# @retval 1 if the `docs` directory is not found.
# -----------------------------------------------------------------------------
build_docs() {
    local docs_dir="$repo_root/docs"
    if [ ! -d "$docs_dir" ]; then
        echo "Docs directory not found at $docs_dir"
        exit 1
    fi

    (cd "$docs_dir" || exit; make clean)
    (cd "$docs_dir" || exit; make html)
}

# -----------------------------------------------------------------------------
# @brief Deploy the built documentation to the web server directory.
# @details Copies the generated documentation to `/var/www/html/wspr/docs/`.
# -----------------------------------------------------------------------------
deploy_docs() {
    local dest_dir="/var/www/html/wspr/docs"
    local src_dir="$repo_root/docs/_build/html"

    sudo rm -fr "$dest_dir"
    sudo mkdir -p "$dest_dir"
    sudo cp -R "$src_dir"/* "$dest_dir"
    sudo chown -R www-data:www-data "$dest_dir"
    echo "Docs copied to $dest_dir."
}

# -----------------------------------------------------------------------------
# @brief Main function orchestrating the script execution.
# -----------------------------------------------------------------------------
main() {
    get_repo_root
    build_docs
    deploy_docs
}

# Invoke the main function
main "$@"
