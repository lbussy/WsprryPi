#!/usr/bin/python3
"""
Project Header Update Script for WsprryPi project.

This script automates the process of updating header information in project files, 
including copyright and versioning. It processes supported files, handles version 
and copyright updates, and ensures that certain sections (such as open-source license blocks)
are excluded from the updates. The script also handles logging, file backups,
and compilation if enabled in the configuration.

Usage:
------
The script can be run as a standalone program to update project files and headers.
Logging and file processing actions are controlled via the configuration settings.

Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)

Created for WsprryPi project, version 1.2.1-32d490b [refactoring].
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

# Now import the Config class from release_config
from release_config import Config
# If debug mode is enabled, disable the writing of bytecode files
if Config.DEBUG:
    os.environ["PYTHONDONTWRITEBYTECODE"] = "1"

# Configure logging
logger = logging.getLogger()

# Prevent duplicate logs by setting propagate to False
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

# Set the logging level (both handlers)
logger.setLevel(logging.INFO)

def run_command(command, cwd=None):
    """
    Run a shell command and return the output.

    This function runs the specified shell command, captures the output, and logs it.
    If the command fails, an error is logged.

    Parameters:
    -----------
    command : str
        The shell command to execute.
    cwd : Path, optional
        The working directory to execute the command in. Defaults to None.

    Returns:
    --------
    str
        The output of the command if successful, or None if the command failed.
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
    Retrieve Git repository details.

    This function fetches the Git repository's top-level directory, project name,
    current branch, latest tag, and the current commit hash.

    Returns:
    --------
    tuple
        A tuple containing the repository directory, project name, branch, tag, and commit hash.
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
    Update the copyright year in a line to reflect the start-current year format.

    This function updates copyright lines containing the NAME_TAG, ensuring the
    copyright year is correctly updated to show the range from the start year to the current year.

    Parameters:
    -----------
    line : str
        The line to update.
    current_year : int
        The current year to use in the copyright update.

    Returns:
    --------
    str
        The updated line.
    """
    if Config.NAME_TAG not in line or "Copyright" not in line:
        return line

    pattern = r'(Copyright\s+\(C\)\s+)(\d{4})(?:-(\d{4}))?'
    match = re.search(pattern, line)
    if match:
        start_year = int(match.group(2))
        end_year = int(match.group(3)) if match.group(3) else start_year
        updated_year = f"{start_year}-{current_year}" if start_year != current_year else f"{start_year}-{current_year}"
        return line.replace(match.group(0), f"{match.group(1)}{updated_year}")
    return line

def update_version_line(line, current_tag, commit_short_hash, current_branch, inside_excluded_section):
    """
    Update the version line in headers or configurations.

    This function updates the version line in supported files with the current tag,
    commit hash, and branch information, while skipping updates in excluded sections.

    Parameters:
    -----------
    line : str
        The line to update.
    current_tag : str
        The current Git tag.
    commit_short_hash : str
        The current short Git commit hash.
    current_branch : str
        The current Git branch.
    inside_excluded_section : bool
        Whether the line is inside an excluded section (e.g., license).

    Returns:
    --------
    str
        The updated line.
    """
    if inside_excluded_section:
        return line

    pattern = r"(version\s+)([\d.]+(?:-[a-z0-9]+)?(?:\s+\[[^\]]+\])?)"
    if re.search(r"version", line, re.IGNORECASE):
        replacement = rf"\g<1>{current_tag}-{commit_short_hash} [{current_branch}]"
        return re.sub(pattern, replacement, line, flags=re.IGNORECASE)
    return line

def backup_file(file_path):
    """
    Create a backup of the file before making changes.

    This function creates a backup of the specified file and stores it in the `scripts/bkp` directory.
    If backups are already enabled and the backup directory doesn't exist, it will be created.

    Parameters:
    -----------
    file_path : Path
        The path to the file to back up.
    """
    if not Config.ENABLE_BACKUP:
        logger.info("Backup is disabled. Skipping backup creation.")
        return

    script_dir = Path(__file__).parent
    backup_dir = script_dir / "bkp"  # Backup directory directly under the script directory

    logger.debug(f"Calculated backup directory: {backup_dir}")

    try:
        backup_dir.mkdir(parents=True, exist_ok=True)
        logger.debug(f"Backup directory created or already exists: {backup_dir}")
    except Exception as e:
        logger.error(f"Error creating backup directory: {e}")
        return

    backup_path = backup_dir / file_path.name
    logger.debug(f"Backup file path: {backup_path}")

    try:
        if not backup_path.exists():
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

    This function compares two strings and uses ANSI escape sequences to colorize added and removed lines.

    Parameters:
    -----------
    original : str
        The original content of the file.
    updated : str
        The updated content of the file.

    Returns:
    --------
    str
        The colorized diff between the original and updated content.
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

    if result and result[-1] != "":
        result.append("")

    return "\n".join(result)

def update_file_content(file_path, current_year, current_tag, commit_short_hash, current_branch):
    """
    Update the contents of a file, modifying copyright and version lines as necessary.

    This function processes the file, skipping over any lines with imports or constants
    in Python files, and stops processing Bash files after specific markers or certain lines.

    Parameters:
    -----------
    file_path : Path
        The path to the file being updated.
    current_year : int
        The current year for copyright updates.
    current_tag : str
        The current Git tag.
    commit_short_hash : str
        The current short Git commit hash.
    current_branch : str
        The current Git branch.

    Returns:
    --------
    list
        A list of updated lines in the file.
    """
    with file_path.open("r", encoding="utf-8") as file:
        lines = file.readlines()

    updated_lines = []
    inside_multiline_comment = False
    inside_excluded_section = False
    python_file = file_path.suffix == ".py"

    for i, line in enumerate(lines):
        stripped_line = line.strip()

        if python_file:
            if stripped_line.startswith(("import", "from")) or "=" in stripped_line:
                updated_lines.append(line)
                continue

        if stripped_line.startswith(("/*", '"')) and not inside_multiline_comment:
            inside_multiline_comment = True

        if inside_multiline_comment and stripped_line.endswith("*/"):
            inside_multiline_comment = False
            inside_excluded_section = False

        if any(keyword in stripped_line for keyword in Config.LICENSE_PATTERNS):
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
    Update script headers with Git information.

    This function updates the headers of files in the specified directories with the Git information.
    It also prints diffs of the updates when `DRY_RUN` is enabled.

    Parameters:
    -----------
    project_directory : Path
        The root directory of the project.
    current_branch : str
        The current Git branch.
    current_tag : str
        The current Git tag.
    commit_short_hash : str
        The current Git commit hash.
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

def compile_project(project_directory):
    """Compile the project using make."""
    if not Config.ENABLE_COMPILATION:
        logger.info("Compilation is disabled. Skipping...")
        return
    src_dir = project_directory / "src"
    logger.info("Cleaning build...")
    run_command("make clean", cwd=src_dir)  # Suppress the output
    logger.info("Compiling project...")
    run_command("make", cwd=src_dir)

def copy_files(project_directory, files):
    """Copy executable files to the scripts directory."""
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
    """Main function."""
    project_directory, project_name, current_branch, current_tag, commit_short_hash = get_git_info()

    logger.info(f"Preparing {project_name} version {current_tag}-{commit_short_hash} [{current_branch}] for release...")

    update_files(Path(project_directory), current_branch, current_tag, commit_short_hash)

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
