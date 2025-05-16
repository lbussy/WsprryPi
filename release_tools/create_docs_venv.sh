#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# @brief Setup Python virtual environment in the docs directory of a Git repo.
#
# This script determines the root of the current Git repository, navigates to
# the 'docs' subdirectory, installs python3-venv, creates and activates a
# virtual environment, and installs dependencies from requirements.txt.
# -----------------------------------------------------------------------------

set -euo pipefail

# Determine the Git repository root directory
repo_root=$(git rev-parse --show-toplevel) || {
    printf "Error: Not inside a Git repository.\n" >&2
    exit 1
}

# Change to the docs directory
cd "$repo_root/docs" || {
    printf "Error: 'docs' directory not found in repository root: %s\n" "$repo_root" >&2
    exit 1
}

# Install python3-venv
printf "Installing python3-venv.\n"
sudo apt install python3-venv -y

# Create the virtual environment
printf "Creating virtual environment in .venv.\n"
python -m venv .venv

# Activate the virtual environment
printf "Activating virtual environment.\n"
# shellcheck disable=SC1091
source .venv/bin/activate

# Install Python dependencies
if [[ -f requirements.txt ]]; then
    printf "Installing Python dependencies.\n"
    pip install -r requirements.txt
else
    printf "Requirements.txt not found.\n"
fi

printf "\nInstallation complete. To start working:\n"
printf "  cd %s/docs && source .venv/bin/activate\n" "$repo_root"
