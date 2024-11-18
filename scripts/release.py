#!/usr/bin/python3
"""
Compile the project and update script headers with environment details.

This script automates the process of updating project header files with Git
information such as the current tag, branch, and commit hash, as well as updating
the copyright year. It also handles compilation and file copying based on configuration.
"""

# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.1-4eb6dd1 [new_release_proc].

import sys
from pathlib import Path
import subprocess
import shutil
import logging
from datetime import datetime
import re
import difflib
import os

# Add the directory containing release_config.py to the system path
sys.path.append(str(Path(__file__).parent))

# Now import the Config class from release_config
from release_config import Config

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
    log_dir.mkdir(parents=True, exist_ok=True)  # Ensure log directory exists
    log_file = log_dir / f"{Config.LOG_FILE_PREFIX}{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
    file_handler = logging.FileHandler(log_file)
    file_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')  # With date/time
    file_handler.setFormatter(file_formatter)
    logger.addHandler(file_handler)

# Set the logging level (both handlers)
logger.setLevel(logging.INFO)
if Config.DEBUG:
    logger.setLevel(logging.DEBUG)  # Set logging level to DEBUG to capture debug logs

def run_command(command, cwd=None):
    """
    Run a shell command and return its output.

    Args:
        command (str): The command to run.
        cwd (Path or str, optional): The working directory to run the command in.

    Returns:
        str: The output of the command.
    """
    try:
        result = subprocess.run(command, shell=True, check=True, text=True, capture_output=True, cwd=cwd)
        logger.debug(f"Command '{command}' executed successfully: {result.stdout.strip()}")
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        logger.error(f"Command failed: {command}\nError: {e.stderr.strip()}")
        return None

def get_git_info():
    """
    Retrieve Git repository details: repository directory, project name, current branch,
    current tag, and commit short hash.

    Returns:
        tuple: Contains repository directory, project name, current branch, current tag, and commit hash.
    """
    try:
        repo_dir = run_command("git rev-parse --show-toplevel")
        if not repo_dir:
            raise ValueError("Git repository not found.")
        project_name = run_command("git config --get remote.origin.url").rsplit("/", 1)[-1].replace(".git", "")
        current_branch = run_command("git rev-parse --abbrev-ref HEAD")
        current_tag = run_command("git describe --tags --abbrev=0") or "No Tag"
        commit_short_hash = run_command("git rev-parse --short HEAD")
        return repo_dir, project_name, current_branch, current_tag, commit_short_hash
    except ValueError as e:
        logger.error(f"Error: {e}")
        sys.exit(1)

def update_copyright_year(line, current_year):
    """
    Update the copyright year in a line to ensure it reflects the start-current year format.

    Args:
        line (str): The line to update.
        current_year (int): The current year to update the copyright.

    Returns:
        str: The updated line with the copyright year.
    """
    if Config.NAME_TAG not in line or "Copyright" not in line:
        return line  # Only update lines with the specific name tag and "Copyright".

    # Regex to match copyright year(s) in single or range format
    pattern = r'(Copyright\s+\(C\)\s+)(\d{4})(?:-(\d{4}))?'
    match = re.search(pattern, line)
    if match:
        start_year = int(match.group(2))  # Extract the start year
        updated_year = f"{start_year}-{current_year}" if start_year != current_year else f"{start_year}-{current_year}"
        return line.replace(match.group(0), f"{match.group(1)}{updated_year}")
    return line

def update_version_line(line, current_tag, commit_short_hash, current_branch, inside_excluded_section):
    """
    Update the version line in headers or configurations.

    Args:
        line (str): The line to update.
        current_tag (str): The current Git tag.
        commit_short_hash (str): The short Git commit hash.
        current_branch (str): The current Git branch.
        inside_excluded_section (bool): Whether the line is inside an excluded section.

    Returns:
        str: The updated version line.
    """
    if inside_excluded_section:
        return line  # Skip updates in excluded sections

    pattern = r"(version\s+)([\d.]+(?:-[a-z0-9]+)?(?:\s+\[[^\]]+\])?)"
    if re.search(r"version", line, re.IGNORECASE):
        replacement = rf"\g<1>{current_tag}-{commit_short_hash} [{current_branch}]"
        return re.sub(pattern, replacement, line, flags=re.IGNORECASE)
    return line

def backup_file(file_path):
    """
    Create a backup of the file before making changes.

    Args:
        file_path (Path): The path of the file to back up.
    """
    if not Config.ENABLE_BACKUP:
        return
    backup_path = file_path.with_suffix(file_path.suffix + ".bak")
    try:
        if not backup_path.exists():  # Check if the backup already exists
            shutil.copy(file_path, backup_path)
            logger.info(f"Backup created: {backup_path}")
        else:
            logger.info(f"Backup already exists: {backup_path}")
    except FileNotFoundError as e:
        logger.error(f"Error creating backup for {file_path}: {e}")
    except Exception as e:
        logger.error(f"Unexpected error while creating backup for {file_path}: {e}")

def colorized_diff(original, updated):
    """
    Generate a colorized diff between two strings (original and updated).

    Args:
        original (str): The original content.
        updated (str): The updated content.

    Returns:
        str: The colorized diff as a string.
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

    # Add a blank line if there is already one at the end
    if result and result[-1] != "":
        result.append("")

    return "\n".join(result)

def update_file_content(file_path, current_year, current_tag, commit_short_hash, current_branch):
    """
    Update the contents of a file, modifying copyright and version lines as necessary.

    Args:
        file_path (Path): The path to the file to update.
        current_year (int): The current year to update the copyright.
        current_tag (str): The current Git tag.
        commit_short_hash (str): The short Git commit hash.
        current_branch (str): The current Git branch.

    Returns:
        list: The updated lines of the file.
    """
    with file_path.open("r", encoding="utf-8") as file:
        lines = file.readlines()

    updated_lines = []
    inside_multiline_comment = False
    inside_excluded_section = False

    for i, line in enumerate(lines):
        if i >= Config.MAX_PROCESS_LINES:
            updated_lines.append(line)
            continue

        stripped_line = line.strip()
        if stripped_line.startswith(("/*", '"')) and not inside_multiline_comment:
            inside_multiline_comment = True

        if inside_multiline_comment and stripped_line.endswith("*/"):
            inside_multiline_comment = False
            inside_excluded_section = False

        if any(keyword in stripped_line for keyword in Config.EXCLUDED_SECTIONS):
            inside_excluded_section = True

        if inside_multiline_comment or stripped_line.startswith(("#", "//")):
            if "Copyright (C)" in line:
                updated_lines.append(update_copyright_year(line, current_year))
            elif re.search(r"version", stripped_line, re.IGNORECASE):
                updated_lines.append(update_version_line(line, current_tag, commit_short_hash, current_branch, inside_excluded_section))
            else:
                updated_lines.append(line)
        else:
            updated_lines.append(line)

    return updated_lines

def update_files(project_directory, current_branch, current_tag, commit_short_hash):
    """
    Update the script files in the specified project directory with Git information.

    Args:
        project_directory (Path): The project directory to process.
        current_branch (str): The current Git branch.
        current_tag (str): The current Git tag.
        commit_short_hash (str): The short Git commit hash.
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
                        updated_lines = update_file_content(file_path, current_year, current_tag, commit_short_hash, current_branch)
                        updated_content = "".join(updated_lines)

                        if updated_content != original_content:
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

def compile_project(project_directory):
    """
    Compile the project using make.

    Args:
        project_directory (Path): The project directory to compile.
    """
    if not Config.ENABLE_COMPILATION:
        logger.info("Compilation is disabled. Skipping...")
        return
    src_dir = project_directory / "src"
    logger.info("Cleaning build...")
    run_command("make clean", cwd=src_dir)
    logger.info("Compiling project...")
    run_command("make", cwd=src_dir)

def copy_files(project_directory, files):
    """
    Copy executable files to the scripts directory.

    Args:
        project_directory (Path): The project directory to copy from.
        files (list): The list of files to copy.
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

def main():
    """
    Main function that orchestrates the execution of the script, updating files,
    compiling the project, and copying executables based on the configuration.
    """
    if Config.DEBUG:
        logger.debug(f"Configuration class settings:")
        logger.debug(f"DRY_RUN: {'Enabled' if Config.DRY_RUN else 'Disabled'}")
        logger.debug(f"ENABLE_LOGGING: {'Enabled' if Config.ENABLE_LOGGING else 'Disabled'}")
        logger.debug(f"ENABLE_COMPILATION: {'Enabled' if Config.ENABLE_COMPILATION else 'Disabled'}")
        logger.debug(f"ENABLE_COPY: {'Enabled' if Config.ENABLE_COPY else 'Disabled'}")
        logger.debug(f"ENABLE_BACKUP: {'Enabled' if Config.ENABLE_BACKUP else 'Disabled'}")
        logger.debug(f"MAX_PROCESS_LINES: {Config.MAX_PROCESS_LINES}")
        logger.debug(f"SUPPORTED_EXTENSIONS: {Config.SUPPORTED_EXTENSIONS}")
        logger.debug(f"DIRECTORIES_TO_PROCESS: {Config.DIRECTORIES_TO_PROCESS}")
        logger.debug(f"EXCLUDED_SECTIONS: {Config.EXCLUDED_SECTIONS}")
        logger.debug(f"PROJECT_EXES: {Config.PROJECT_EXES}")
        logger.debug(f"LOG_DIR: {Config.LOG_DIR}")
        logger.debug(f"LOG_FILE_PREFIX: {Config.LOG_FILE_PREFIX}")
        logger.debug(f"NAME_TAG: {Config.NAME_TAG}")

    project_directory, project_name, current_branch, current_tag, commit_short_hash = get_git_info()

    logger.info(f"Preparing {project_name} version {current_tag}-{commit_short_hash} [{current_branch}] for release...")

    update_files(Path(project_directory), current_branch, current_tag, commit_short_hash)

    # Compilation and file copying
    if Config.ENABLE_COMPILATION:
        compile_project(Path(project_directory))
        if Config.ENABLE_COPY:
            copy_files(Path(project_directory), Config.PROJECT_EXES)
    else:
        logger.info("Compilation is disabled. Skipping compilation and file copying.")

    logger.info(f"Project prep for {project_name} version {current_tag}-{commit_short_hash} [{current_branch}] complete.")

if __name__ == "__main__":
    main()
