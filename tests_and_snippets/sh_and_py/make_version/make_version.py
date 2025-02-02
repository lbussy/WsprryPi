#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# This file is part of WsprryPi.
#
# Copyright (C) 2023-2025 Lee C. Bussy (@LBussy)
#
# WsprryPi is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

"""
@file make_version.py
@brief Generate a version string based on Git repository state.

This script produces a version string based on the current Git branch, tag, commit count, 
short hash, and whether there are uncommitted changes. It adheres to semantic versioning 
principles when applicable.
"""

import subprocess
import re
import argparse
import os
import sys

__version__ = "1.2.3"
default_branches=["main", "master"]
default_version="0.0.0"


def get_script_name():
    """
    Get the name of the script being run, whether executed directly or imported.

    @return Name of the script.
    """
    if __name__ == "__main__":
        # Script is executed directly
        return os.path.basename(__file__)
    else:
        # Script is imported; return the entry-point script name
        return os.path.basename(sys.argv[0])


def run_git_command(command):
    """
    Run a Git command and return its output.

    @param command List of strings representing the Git command.
    @return Output of the command as a string, or None if the command fails.
    """
    try:
        result = subprocess.check_output(command, stderr=subprocess.DEVNULL, text=True).strip()
        return result
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


def get_pre_release_info(tag):
    """
    Extract pre-release information from a semantic version tag.

    @param tag Git tag string.
    @return Pre-release information as a string, or an empty string if not applicable.
    """
    if not tag or not is_semantic_version(tag):
        return ""
    match = re.search(r"^(([0-9]+\.){2}[0-9]+)-([-0-9A-Za-z.-]+)", tag)
    if match:
        pre_release = match.group(3)
        pre_release = re.sub(r"[^0-9A-Za-z-]", "", pre_release)
        pre_release = re.sub(r"\.+", ".", pre_release)
        return pre_release
    return ""


def get_num_commits_since_tag(tag):
    """
    Get the number of commits since the given Git tag.

    @param tag Git tag string.
    @return Number of commits since the tag as an integer, or None if the tag is None.
    """
    if not tag:
        return None
    try:
        return int(run_git_command(["git", "rev-list", "--count", f"{tag}..HEAD"]) or 0)
    except ValueError:
        return 0


def get_short_hash():
    """
    Get the short hash of the current Git commit.

    @return Short hash of the HEAD commit as a string, or None if unavailable.
    """
    return run_git_command(["git", "rev-parse", "--short", "HEAD"])


def has_uncommitted_changes():
    """
    Check if there are uncommitted changes in the repository.

    @return True if there are uncommitted changes, False otherwise.
    """
    return bool(run_git_command(["git", "status", "--porcelain"]))


def generate_version_string():
    """
    Generate a version string based on the Git repository state.

    - If no tags exist on default branches, the version string will use an extended format.
    - For other branches or tagged states, it adheres to semantic versioning principles.

    @return Generated version string.
    """
    if not is_git_repository():
        return "{default_version}-detached"

    branch_name = get_branch_name()
    tag = get_last_tag()
    sem_ver = tag if is_semantic_version(tag) else "{default_version}"
    pre_release = get_pre_release_info(tag)
    num_commits = get_num_commits_since_tag(tag) or 0
    short_hash = get_short_hash()
    dirty = "-dirty" if has_uncommitted_changes() else ""

    if branch_name in default_branches and not tag:
        version_string = f"{default_version}-{branch_name}"
        if num_commits > 0 and short_hash:
            version_string += f"+{num_commits}.{short_hash}"
        elif num_commits > 0:
            version_string += f"+{num_commits}"
        version_string += dirty
        return version_string

    version_string = sem_ver

    if branch_name not in default_branches:
        version_string += f"-{branch_name}"

    if pre_release:
        version_string += pre_release

    if num_commits > 0 and short_hash:
        version_string += f"+{num_commits}.{short_hash}"
    elif num_commits > 0:
        version_string += f"+{num_commits}"

    version_string += dirty

    return version_string


class CustomArgumentParser(argparse.ArgumentParser):
    def format_usage(self):
        """
        Override the format_usage method to capitalize the "Usage:" wording.
        """
        return f"Usage: {self.usage}\n"

    def format_help(self):
        """
        Override the format_help method to ensure the usage line in help output
        also uses the capitalized "Usage:".
        """
        help_text = self.format_usage()  # Use custom format_usage
        help_text += f"\n{self.description}\n\n"
        help_text += "Options:\n"
        help_text += self.format_options()
        return help_text

    def format_options(self):
        """
        Format the options section of the help message.
        """
        formatter = self._get_formatter()
        for action in self._actions:
            formatter.add_argument(action)
        return formatter.format_help()


def handle_arguments():
    """
    Handle command-line arguments.

    @return Parsed arguments.
    """
    parser = CustomArgumentParser(
        description="Generate a version string based on the Git repository state.",
        formatter_class=argparse.RawTextHelpFormatter,
        usage=f"{get_script_name()} [-h] [-d] [-v] [-w]",  # Custom usage line
        add_help=False,  # Disable default help argument
    )
    parser.add_argument(
        "-h", "--help",
        action="help",
        help="Display this help message and exit."
    )
    parser.add_argument(
        "-v", "--version",
        action="version",
        version=f"%(prog)s: {__version__}",
        help="Show the script version and exit."
    )
    parser.add_argument(
        "-g", "--generate",
        action="store_true",
        help=(
            "Generate a version string based on\n"
            "the current Git state."
        )
    )
    return parser.parse_args()


def process_arguments(args):
    """
    Process the parsed arguments.

    @param args Parsed arguments from the argument parser.
    """
    if args.generate:
        print(generate_version_string())
    else:
        print("Invalid or no arguments provided.")
        print("Use -h or --help to see available options.")


def main():
    """
    Main entry point of the script.
    """
    args = handle_arguments()
    process_arguments(args)


if __name__ == "__main__":
    main()
