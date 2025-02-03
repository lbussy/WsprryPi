#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# -----------------------------------------------------------------------------
# @file update_versions.py
# @brief Update versions in scripts and source files.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.0.0
# @date 2025-02-03
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
# ./get_semantic_version.sh
#
# @returns
# Returns a correctly formatted sematic version string.  Defaults to
# declarations in header.
#
# -----------------------------------------------------------------------------

from get_semantic_version import SemanticVersion

__version__ = "1.2.3"


# List of files to update
update_file_extension = [
    ".conf",
    ".ini",
    ".py",
    ".sh",
    ".cpp",
    ".hpp",
    ".c",
    ".h",
    ".service",
    ".timer",
]

# Directory containing the target files to update
target_file_directories = [
    "config",
    "executables",
    "release_tools",
    "scripts",
    "src",
    "systemd",
]

# Line prefixes with versions
version_line_patterns = [
    "# @version ",
    "__version__ =,
]

date_line_patterns = [
    "# @date "
]

def get_git_root():
    try:
        # Run the Git command to get the root directory
        git_root = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"],
            stderr=subprocess.STDOUT
        ).strip().decode("utf-8")
        return git_root
    except subprocess.CalledProcessError:
        # Return None if not inside a Git repository
        return None


def main():
    # Generate the new version string
    version_generator = SemanticVersion(debug=False)
    new_version=version_generator.get_sem_ver()
    print(f"Generated new version string: {new_version}")
    # update_files(files_to_update, new_version)


if __name__ == "__main__":
    # Run the main function with the list of files to update
    main()
