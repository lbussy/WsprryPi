#!/bin/bash
# -*- coding: utf-8 -*-

# ----------------------------------------------------------------------------
# @file rename_branch.sh
# @brief Script to rename a Git branch both locally and on the remote repository.
#
# @details
# This script automates the process of renaming a Git branch. It dynamically detects the current branch name,
# prompts the user for the new branch name, and updates the branch name locally and on the remote repository.
#
# @par License:
# MIT License
# Copyright (c) 2024
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
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# @brief Get the name of the current branch and prompt for the new branch name.
# ----------------------------------------------------------------------------

# Get the current branch name
old_name=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
if [ -z "$old_name" ]; then
    echo "Error: Could not determine the current branch. Ensure you are in a Git repository."
    exit 1
fi

# Prompt the user for the new branch name
read -p "Enter the new branch name: " new_name
if [ -z "$new_name" ]; then
    echo "Error: New branch name cannot be empty."
    exit 1
fi

# Remote repository name (default is 'origin')
remote="origin"

# ----------------------------------------------------------------------------
# @brief Rename the local branch to the new name.
# ----------------------------------------------------------------------------
git branch -m "$old_name" "$new_name"

# ----------------------------------------------------------------------------
# @brief Delete the old branch on the remote repository.
# ----------------------------------------------------------------------------
git push "$remote" --delete "$old_name"

# ----------------------------------------------------------------------------
# @brief Unset upstream for the new branch to prevent conflicts.
# ----------------------------------------------------------------------------
git branch --unset-upstream "$new_name"

# ----------------------------------------------------------------------------
# @brief Push the new branch to the remote repository.
# ----------------------------------------------------------------------------
git push "$remote" "$new_name"

# ----------------------------------------------------------------------------
# @brief Set upstream tracking for the new branch.
# ----------------------------------------------------------------------------
git push "$remote" -u "$new_name"

echo "Branch renamed from '$old_name' to '$new_name' successfully."
