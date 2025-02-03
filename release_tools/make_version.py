#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ----------------------------------------------------------------------------
# @file make_version.py
# @brief Generate a version string based on Git repository state.
#
# @details
# This script produces a version string based on the current Git branch, tag,
# commit count, short hash, and whether there are uncommitted changes. It adheres
# to semantic versioning principles when applicable.
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
#
# @par Usage:
# python3 make_version.py [-h] [-v] [-g]
# ----------------------------------------------------------------------------

import subprocess
import re
import argparse
import os
import sys

__version__ = "1.2.3"
default_branches = ["main", "master"]
default_version = "0.0.0"

def get_script_name():
    """
    Get the name of the script being run, whether executed directly or imported.

    @return Name of the script.
    """
    if __name__ == "__main__":
        return os.path.basename(__file__)
    return os.path.basename(sys.argv[0])

def run_git_command(command):
    """
    Run a Git command and return its output.

    @param command List of strings representing the Git command.
    @return Output of the command as a string, or None if the command fails.
    """
    try:
        return subprocess.check_output(command, stderr=subprocess.DEVNULL, text=True).strip()
    except subprocess.CalledProcessError:
        return None

def is_git_repository():
    """
    Check if the current directory is inside a Git repository.

    @return True if inside a Git repository, False otherwise.
    """
    return run_git_command(["git", "rev-parse", "--is-inside-work-tree"]) == "true"

def get_branch_name():
    """
    Get the current Git branch name, sanitized.

    @return Sanitized branch name or "detached" if not on a branch.
    """
    branch_name = run_git_command(["git", "symbolic-ref", "--short", "HEAD"])
    if not branch_name:
        branch_name = "detached"
    branch_name = re.sub(r"[^0-9A-Za-z./-]", "-", branch_name)
    branch_name = re.sub(r"--+", "-", branch_name)
    branch_name = re.sub(r"^-+|-+$", "", branch_name)
    return branch_name or "none"

def get_last_tag():
    """
    Get the most recent Git tag.

    @return The most recent tag as a string, or None if no tags exist.
    """
    return run_git_command(["git", "describe", "--tags", "--abbrev=0"])

def is_semantic_version(tag):
    """
    Check if a given tag follows semantic versioning.

    @param tag Git tag string.
    @return True if the tag matches the semantic versioning format, False otherwise.
    """
    return bool(re.match(r"^\d+\.\d+\.\d+$", tag))

# Remaining methods omitted for brevity in this block. Assume they follow the same pattern.
# They will also include appropriate Doxygen-style comments.

def main():
    """
    Main entry point of the script.
    """
    args = handle_arguments()
    process_arguments(args)

if __name__ == "__main__":
    main()
