#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# This file is part of WsprryPi.
#
# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
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
@file release.py
@brief Project Header Update Script for WsprryPi project.

This script automates the process of updating header information in project files, 
including copyright and versioning. It processes supported files, handles version 
and copyright updates, and ensures that certain sections (such as open-source license blocks)
are excluded from the updates. The script also handles logging, file backups,
and compilation if enabled in the configuration.

@note The script can be run as a standalone program to update project files and headers.
Logging and file processing actions are controlled via the configuration settings.
"""

import os
import sys
from pathlib import Path
import subprocess
import shutil
import logging
from datetime import datetime
import re
import difflib

# Add the directory containing release_config.py to the system path
sys.path.append(str(Path(__file__).parent))

# Import the Config class from release_config
from release_config import Config

# Disable bytecode file writing if debug mode is enabled
if Config.DEBUG:
    os.environ["PYTHONDONTWRITEBYTECODE"] = "1"

# Configure logging
logger = logging.getLogger()
logger.propagate = False  # Disable propagation to the root logger

# Console logging configuration (no date/time)
console_handler = logging.StreamHandler()
console_formatter = logging.Formatter('%(levelname)s - %(message)s')  # No date/time
console_handler.setFormatter(console_formatter)
logger.addHandler(console_handler)

# File logging configuration (with date/time)
if Config.ENABLE_LOGGING:
    log_dir = Config.LOG_DIR
    log_dir.mkdir(parents=True, exist_ok=True)
    log_file = log_dir / f"{Config.LOG_FILE_PREFIX}{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
    file_handler = logging.FileHandler(log_file)
    file_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')  # With date/time
    file_handler.setFormatter(file_formatter)
    logger.addHandler(file_handler)

# Set the logging level
logger.setLevel(logging.INFO)


def run_command(command, cwd=None):
    """
    @brief Run a shell command and return the output.

    @param command The shell command to execute.
    @param cwd The working directory to execute the command in. Defaults to None.

    @return The output of the command if successful, or None if the command failed.
    """
    try:
        result = subprocess.run(
            command, shell=True, check=True, text=True, capture_output=True, cwd=cwd, env=os.environ
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        logger.error(f"Command failed: {command}\nError: {e.stderr.strip()}")
        return None


def get_git_info():
    """
    @brief Retrieve Git repository details, including dirty state.

    @return A tuple containing the repository directory, project name, branch, tag, and commit hash.
    """
    try:
        repo_dir = run_command("git rev-parse --show-toplevel")
        if not repo_dir:
            raise ValueError("Git repository not found.")
        project_name = run_command("git config --get remote.origin.url").rsplit("/", 1)[-1].replace(".git", "")
        current_branch = run_command("git rev-parse --abbrev-ref HEAD")
        current_tag = run_command("git describe --tags --abbrev=0") or "No Tag"
        commit_short_hash = run_command("git describe --dirty --always")
        return repo_dir, project_name, current_branch, current_tag, commit_short_hash
    except ValueError as e:
        logger.error(f"Error: {e}")
        sys.exit(1)


def colorized_diff(original, updated):
    """
    @brief Generate a colorized diff between two strings (original and updated).

    @param original The original content of the file.
    @param updated The updated content of the file.

    @return A string representing the colorized diff.
    """
    diff = difflib.unified_diff(original.splitlines(), updated.splitlines(), lineterm="")
    result = []
    for line in diff:
        if line.startswith("+") and not line.startswith("+++"):
            result.append(f"\033[32m{line}\033[0m")  # Green for added lines
        elif line.startswith("-") and not line.startswith("---"):
            result.append(f"\033[31m{line}\033[0m")  # Red for removed lines
        else:
            result.append(line)
    return "\n".join(result)


def compile_project(project_directory):
    """
    @brief Compile the project using make.

    @param project_directory The root directory of the project.
    """
    if not Config.ENABLE_COMPILATION:
        logger.info("Compilation is disabled. Skipping...")
        return
    src_dir = project_directory / "src"
    logger.info("Cleaning build...")
    run_command("make clean", cwd=src_dir)  # Suppress the output
    logger.info("Compiling project...")
    run_command("make", cwd=src_dir)


def copy_files(project_directory, files):
    """
    @brief Copy executable files to the scripts directory.

    @param project_directory The root directory of the project.
    @param files A list of files to copy.
    """
    for file in files:
        src = project_directory / "src" / file
        dest = project_directory / "scripts" / file
        try:
            if not src.exists():
                logger.warning(f"Warning: {src} does not exist. Skipping...")
                continue
            shutil.copy(src, dest)
            logger.info(f"Copied {src} to {dest}")
        except FileNotFoundError as e:
            logger.error(f"Error: {e}")
        except Exception as e:
            logger.error(f"Unexpected error while copying {src}: {e}")


def update_version_line(line, repo_name, current_tag, commit_short_hash, current_branch, inside_excluded_section):
    """
    @brief Update the version line in headers or configurations.

    Reconstructs the entire version line to prevent duplication or format errors. The version string includes
    the repository name, current Git tag, commit hash (with -dirty if applicable), and branch name.

    @param line The line to update.
    @param repo_name The name of the Git repository.
    @param current_tag The current Git tag.
    @param commit_short_hash The current short Git commit hash, including -dirty if applicable.
    @param current_branch The current Git branch.
    @param inside_excluded_section Whether the line is inside an excluded section (e.g., license).

    @return The updated line if applicable, otherwise the original line.
    """
    if inside_excluded_section:
        return line

    # Pattern to match a comment or multiline comment starting with "Created for"
    pattern = r"^\s*(#|//|\*)\s*Created for"

    if re.match(pattern, line):
        # Reconstruct the version string
        new_version_string = f"{current_tag}-{commit_short_hash} [{current_branch}]"
        # Replace everything after "Created for" with the new version string
        return re.sub(
            r"(Created for).*",
            rf"\1 the {repo_name} project, version {new_version_string}",
            line,
        )

    return line


def update_file_content(file_path, repo_name, current_year, current_tag, commit_short_hash, current_branch):
    """
    @brief Update the contents of a file, modifying copyright and version lines.

    Handles updates for copyright years and version lines while respecting 
    excluded sections (e.g., license blocks).

    @param file_path The path to the file being updated.
    @param repo_name The Git repository name.
    @param current_year The current year for copyright updates.
    @param current_tag The current Git tag.
    @param commit_short_hash The current short Git commit hash, including -dirty if applicable.
    @param current_branch The current Git branch.

    @return A list of updated lines in the file.
    """
    with file_path.open("r", encoding="utf-8") as file:
        lines = file.readlines()

    updated_lines = []
    inside_multiline_comment = False
    inside_excluded_section = False

    for line in lines:
        stripped_line = line.strip()

        # Detect multi-line comments or license blocks
        if stripped_line.startswith(("/*", '"')) and not inside_multiline_comment:
            inside_multiline_comment = True
        if inside_multiline_comment and stripped_line.endswith("*/"):
            inside_multiline_comment = False
            inside_excluded_section = False
        if any(keyword in stripped_line for keyword in Config.LICENSE_PATTERNS):
            inside_excluded_section = True

        # Update version lines
        updated_lines.append(
            update_version_line(
                line, repo_name, current_tag, commit_short_hash, current_branch, inside_excluded_section
            )
        )

    return updated_lines


def backup_file(file_path):
    """
    @brief Create a backup of the file before making changes.

    If backups are enabled via configuration, this function creates a backup copy of the specified
    file in the `scripts/bkp` directory. If the directory does not exist, it will be created.

    @param file_path Path: The path to the file to back up.
    """
    if not Config.ENABLE_BACKUP:
        logger.info("Backup is disabled. Skipping backup creation.")
        return

    script_dir = Path(__file__).parent
    backup_dir = script_dir / "bkp"  # Backup directory under the `scripts` directory

    try:
        # Create backup directory if it doesn't exist
        backup_dir.mkdir(parents=True, exist_ok=True)
        logger.debug(f"Backup directory created or exists: {backup_dir}")
    except Exception as e:
        logger.error(f"Error creating backup directory: {e}")
        return

    backup_path = backup_dir / file_path.name
    try:
        if not backup_path.exists():
            shutil.copy(file_path, backup_path)
            logger.info(f"Backup created: {backup_path}")
        else:
            logger.info(f"Backup already exists: {backup_path}")
    except FileNotFoundError as e:
        logger.error(f"File not found for backup: {file_path} - {e}")
    except Exception as e:
        logger.error(f"Unexpected error during backup: {file_path} - {e}")


def update_files(project_directory, repo_name, current_branch, current_tag, commit_short_hash):
    """
    @brief Update script headers with Git information.

    This function updates the headers of files in the specified directories with the Git information.
    It also prints diffs of the updates when `DRY_RUN` is enabled.

    @param project_directory The root directory of the project.
    @param repo_name The Git repository name.
    @param current_branch The current Git branch.
    @param current_tag The current Git tag.
    @param commit_short_hash The current Git commit hash.
    """
    current_year = datetime.now().year

    for directory in Config.DIRECTORIES_TO_PROCESS:
        target_dir = project_directory / directory
        if target_dir.exists() and target_dir.is_dir():
            logger.info(f"Checking directory: {target_dir}")
            for file_path in target_dir.rglob("*"):
                if file_path.suffix in Config.SUPPORTED_EXTENSIONS:
                    try:
                        original_content = file_path.read_text(encoding="utf-8")
                        updated_lines = update_file_content(
                            file_path, repo_name, current_year, current_tag, commit_short_hash, current_branch
                        )
                        updated_content = "".join(updated_lines)

                        if updated_content != original_content:
                            logger.debug(f"Updating file: {file_path}")

                            if Config.ENABLE_BACKUP:
                                backup_file(file_path)

                            if Config.DRY_RUN:
                                diff = colorized_diff(original_content, updated_content)
                                print(f"\nChanges to {file_path}:")
                                print(diff)
                            else:
                                diff = colorized_diff(original_content, updated_content)
                                print(f"\nChanges to {file_path}:")
                                print(diff)
                                with file_path.open("w", encoding="utf-8") as file:
                                    file.writelines(updated_lines)
                                logger.info(f"Updated: {file_path}")
                        else:
                            logger.debug(f"No changes for: {file_path}")

                    except Exception as e:
                        logger.error(f"Error processing {file_path}: {e}")


def main():
    """
    @brief Main function to orchestrate the release process.

    Retrieves Git information, updates headers, and handles compilation and file copying.
    """
    project_directory, project_name, current_branch, current_tag, commit_short_hash = get_git_info()

    logger.info(f"Preparing {project_name} version {current_tag}-{commit_short_hash} [{current_branch}] for release...")

    update_files(Path(project_directory), project_name, current_branch, current_tag, commit_short_hash)

    src_dir = Path(project_directory) / "src"

    logger.info(f"Cleaning project in {src_dir}...")

    if not src_dir.exists():
        logger.error(f"Error: The src directory {src_dir} does not exist!")
    else:
        logger.info(f"Directory exists: {src_dir}")
        run_command("make clean", cwd=src_dir)  # Suppress the output

    # Compilation and file copying
    if Config.ENABLE_COMPILATION:
        compile_project(Path(project_directory))
        if Config.ENABLE_COPY:
            copy_files(Path(project_directory), Config.PROJECT_EXES)

    logger.info(f"Cleaning project again in {src_dir} after copying files...")
    run_command("make clean", cwd=src_dir)  # Suppress the output

    logger.info(f"Project prep for {project_name} version {current_tag}-{commit_short_hash} [{current_branch}] complete.")


if __name__ == "__main__":
    main()
