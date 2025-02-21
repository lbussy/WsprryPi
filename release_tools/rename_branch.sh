#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# -----------------------------------------------------------------------------
# @file rename_branch.sh
# @brief Script to rename a Git branch both locally and on the remote
#        repository.
#
# @details
# This script automates the process of renaming a Git branch. It dynamically
# detects the current branch name, prompts the user for the new branch name,
# and updates the branch name locally and on the remote repository.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.2.1-timing_loop+58.8eb9a00
# @date 2025-02-21
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
# ./rename_branch.sh
#
# -----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# @brief Get teh old branch name.
# ----------------------------------------------------------------------------
old_name=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || true)

# Check if the branch name is empty, meaning we are not in a Git repository
if [ -z "$old_name" ]; then
    printf "Error: Could not determine the current branch. Ensure you are in a Git repository.\n"
    exit 1
fi

# ----------------------------------------------------------------------------
# @brief Prompt the user for the new branch name.
# ----------------------------------------------------------------------------
read -rp "Enter the new branch name: " new_name
if [ -z "$new_name" ]; then
    printf "Error: New branch name cannot be empty.\n"
    exit 1
fi

# Remote repository name (default is 'origin')
remote="origin"

# ----------------------------------------------------------------------------
# @brief Rename the local branch to the new name.
# ----------------------------------------------------------------------------
if ! git branch -m "$old_name" "$new_name"; then
    printf "Error: Failed to rename the local branch from '%s' to '%s'.\n" "$old_name" "$new_name" >&2
    exit 1
fi

# ----------------------------------------------------------------------------
# @brief Delete the old branch on the remote repository.
# ----------------------------------------------------------------------------
if ! git push "$remote" --delete "$old_name"; then
    printf "Error: Failed to delete the old branch '%s' on the remote repository '%s'.\n" "$old_name" "$remote" >&2
    exit 1
fi

# ----------------------------------------------------------------------------
# @brief Unset upstream for the new branch to prevent conflicts.
# ----------------------------------------------------------------------------
if ! git branch --unset-upstream "$new_name"; then
    printf "Error: Failed to unset upstream for the new branch '%s'. The branch may not have an upstream set or there may be a configuration issue.\n" "$new_name" >&2
    exit 1
fi

# ----------------------------------------------------------------------------
# @brief Push the new branch to the remote repository.
# ----------------------------------------------------------------------------
if ! git push "$remote" "$new_name"; then
    printf "Error: Failed to push the new branch '%s' to the remote repository '%s'. This could be due to network issues or insufficient permissions.\n" "$new_name" "$remote" >&2
    exit 1
fi

# ----------------------------------------------------------------------------
# @brief Set upstream tracking for the new branch.
# ----------------------------------------------------------------------------
if ! git push "$remote" -u "$new_name"; then
    printf "Error: Failed to set upstream for the new branch '%s' on the remote repository '%s'. This could be due to network issues or insufficient permissions.\n" "$new_name" "$remote" >&2
    exit 1
fi

printf "Branch renamed from '%s' to '%s' successfully.\n" "$old_name" "$new_name"
exit 0
