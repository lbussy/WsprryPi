#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# -----------------------------------------------------------------------------
# @file update_versions.py
# @brief Update versions in scripts and source files.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.2.1-update_release_scripts+95.8843fc0-dirty
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
# Returns a correctly formatted semantic version string. Defaults to
# declarations in header.
#
# -----------------------------------------------------------------------------

import os
import subprocess
from datetime import datetime

from get_semantic_version import SemanticVersion

__version__ = "1.2.1-update_release_scripts+95.8843fc0-dirty"


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
    "# @version",
    "__version__ =",
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


def find_files_to_update(git_root):
    files_to_update = []
    for directory in target_file_directories:
        target_dir = os.path.join(git_root, directory)
        if os.path.isdir(target_dir):
            for root, _, files in os.walk(target_dir):
                for file in files:
                    if any(file.endswith(ext) for ext in update_file_extension):
                        files_to_update.append(os.path.join(root, file))
    return files_to_update


def replace_line_in_file(file_path, line_num, new_line):
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()

    # Replace the specific line with the new line
    lines[line_num - 1] = new_line + "\n"

    # Write the updated lines back to the file
    with open(file_path, 'w', encoding='utf-8') as file:
        file.writelines(lines)


def parse_file_for_version_and_date(file_path, new_version):
    changed_lines = 0
    changed = False

    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()
        for line_num, line in enumerate(lines, start=1):
            # Handle version line patterns
            for pattern in version_line_patterns:
                if line.startswith(pattern):
                    if pattern == "__version__ =":
                        new_line = f"{pattern.strip()} \"{new_version}\""
                    else:
                        new_line = f"{pattern.strip()} {new_version}"

                    # Compare the original line with the new line
                    if line.strip() != new_line.strip():  # Check if there's a difference
                        # Replace the line in the file
                        replace_line_in_file(file_path, line_num, new_line)
                        changed_lines += 1
                        changed = True

            # Handle date line patterns
            for pattern in date_line_patterns:
                if line.startswith(pattern):
                    current_date = datetime.now().strftime("%Y-%m-%d")
                    new_line = f"{pattern.strip()} {current_date}"

                    # Compare the original line with the new line
                    if line.strip() != new_line.strip():  # Check if there's a difference
                        # Replace the line in the file
                        replace_line_in_file(file_path, line_num, new_line)
                        changed_lines += 1
                        changed = True

    return changed, changed_lines


def main():
    # Get the root directory of the Git repository
    git_root = get_git_root()
    if not git_root:
        print("Error: Not inside a Git repository.")
        return

    print(f"Git root directory: {git_root}")

    # Generate the new version string
    version_generator = SemanticVersion(debug=False)
    new_version = version_generator.get_sem_ver()

    # Find files to update
    files_to_update = find_files_to_update(git_root)

    # Track files where changes were made and the number of lines changed
    files_changed = []
    total_lines_changed = 0

    # Parse files for matching lines and replace them
    for file_path in files_to_update:
        changed, changed_lines = parse_file_for_version_and_date(file_path, new_version)

        if changed:
            files_changed.append(file_path)
            total_lines_changed += changed_lines
            print(f"{file_path}: Change was made, {changed_lines} lines updated.")

    # Print the results only if there were changes
    if files_changed:

        print(f"\nTotal lines changed: {total_lines_changed}")
    else:
        print("No files needed to be updated.")


if __name__ == "__main__":
    # Run the main function
    main()