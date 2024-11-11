#!/usr/bin/python3
"""Module compiles the project and prepares script headers to match the environment."""

# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi version 1.2.1 [devel).

import subprocess
import os
import sys
import locale
import shutil
from fileinput import FileInput

# Text-based and executable files in project
PROJECT_FILES = ["release.py", "install.sh", "uninstall.sh", "shutdown-watch.py", "shutdown_watch.py", "logrotate.d", "copydocs.sh", "copyexe.sh", "copysite.sh"]
PROJECT_EXES = ["wspr", "wspr.ini"]


def get_git_repo_directory():
    """Function returns Git repo directory from environment."""
    try:
        # Get the root directory of the Git repository
        repo_dir = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"], text=True
        ).strip()
        return repo_dir
    except subprocess.CalledProcessError:
        print("Error: Not a Git repository.")
        return None


def get_git_project_name():
    """Function returns Git project name from environment."""
    try:
        # Get the remote origin URL
        url = subprocess.check_output(
            ["git", "config", "--get", "remote.origin.url"], text=True
        ).strip()
        # Extract the project name from the URL
        project_name = url.rsplit("/", maxsplit=1)[-1].replace(".git", "")
        return project_name
    except subprocess.CalledProcessError:
        print("Error: Not a Git repository or unable to retrieve project name.")
        return None


def get_latest_git_tag():
    """Function returns latest Git tag from environment."""
    try:
        # Get the latest tag only
        tag = subprocess.check_output(
            ["git", "describe", "--tags", "--abbrev=0"], text=True
        ).strip()
        return tag
    except subprocess.CalledProcessError:
        print("Error: No tags found or not a Git repository.")
        return None


def get_current_git_short_commit():
    """Function returns current Git commit hash from environment."""
    try:
        # Get the short commit hash
        short_commit = subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"], text=True
        ).strip()
        return short_commit
    except subprocess.CalledProcessError:
        print("Error: Not a Git repository or unable to retrieve commit.")
        return None


def get_current_git_branch():
    """Function returns current Git branch from environment."""
    try:
        # Get the current branch name
        branch = subprocess.check_output(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"], text=True
        ).strip()
        return branch
    except subprocess.CalledProcessError:
        print("Error: Not a Git repository or unable to retrieve branch.")
        return None


def replace_in_file(filename, search_string, replace_string):
    """Function to replace Git info in file headers."""
    local_directory = os.path.dirname(filename)
    base_name = os.path.basename(filename)
    locale_encoding = locale.getpreferredencoding(False)

    # Load file in memory as lines
    try:
        with open(filename, "r", encoding = locale_encoding) as file:
            lines = file.readlines()
    except FileNotFoundError:
        # This is needed because the shutdown script was chanegd to snake-case
        if base_name not in ["shutdown-watch.py", "shutdown_watch.py"]:
            print(f"Error: File '{base_name}' not found in {local_directory}.")
            sys.exit(1)
        else:
            if base_name == "shutdown-watch.py":
                if not os.path.exists(local_directory + "/" + "shutdown_watch.py"):
                    print(f"Error: File 'shutdown_watch.py' not found in {local_directory}.")
                    sys.exit(1)
                else:
                    return
            else:
                print(f"Error: File {base_name} not found in {local_directory}.")
                sys.exit(1)

    # Modify lines that start with 'string'
    with open(filename, "w", encoding = locale_encoding) as file:
        for line in lines:
            if line.startswith(search_string):
                file.write(replace_string + os.linesep)
            else:
                file.write(line)


def edit_files(project_directory, project_branch, project_tag, files, version_string):
    """Function to iterate a list of files to replace file and script properties with Git info."""
    for file in files:
        print(f"Updating {file}.")
        file_name = project_directory + "/scripts/" + file
        replace_in_file(file_name, "# Created for ", f"# Created for {version_string}.")
        replace_in_file(file_name, "BRANCH=", "BRANCH=" + project_branch)
        replace_in_file(file_name, "VERSION=", "VERSION=" + project_tag)


def compile_project(project_directory, version_string):
    """Function to make current project."""
    current_dir = os.getcwd()
    source_dir = project_directory + "/src/"
    clean_command = "make clean"
    make_command = "make"
    os.chdir(source_dir)

    # Make clean
    try:
        # Run the command and capture output
        result = subprocess.run(
            clean_command, shell=True, check=True, text=True, capture_output=True
        )
    except FileNotFoundError:
        print(f"Command '{clean_command}' not found: Ensure the command is correct and available in the environment.")
    except subprocess.CalledProcessError as e:
        print(f"'make clean'' failed: {e}.")

    print(f"Compiling {version_string}.")
    try:
        # Execute the command and capture the output
        result = subprocess.run(make_command, check=True, text=True, capture_output=True)
        print("Make command succeeded.")
        print("Output:")
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print("Make command failed.")
        print("Error output:")
        print(e.stderr)
        sys.exit(1)
    os.chdir(current_dir)


def copy_files(project_directory, files):
    """Function to stage exe files to scripts directory."""
    for file in files:
        # Define the source and destination paths
        source_file = os.path.abspath(f"{project_directory}/src/{file}")
        destination_file = os.path.abspath(f"{project_directory}/scripts/{file}")

        # Copy the file
        try:
            shutil.copy(source_file, destination_file)
            print(f"Copying {file} to scripts location.")
        except FileNotFoundError:
            print(f"Error: The file {source_file} does not exist.")
        except PermissionError:
            print(f"Error: You do not have permission to copy {source_file}.")
        except shutil.Error as e:
            print(f"An unexpected error occurred copying {source_file}: {e}")


def main():
    """Function handles project compile and file edits."""
    # Get constants from header of file
    project_files = PROJECT_FILES
    project_exes = PROJECT_EXES

    # Get Git project info
    project_directory = get_git_repo_directory()
    if project_directory is None:
        sys.exit(1)
    project_name = get_git_project_name()
    if project_name is None:
        sys.exit(1)
    project_tag = get_latest_git_tag()
    if project_tag is None:
        sys.exit(1)
    project_hash = get_current_git_short_commit()
    if project_hash is None:
        sys.exit(1)
    project_branch = get_current_git_branch()
    if project_branch is None:
        sys.exit(1)

    # Concatenate version string
    version_string = f"{project_name} version {project_tag} [{project_branch})"

    # Begin
    print(f"Preparing {version_string} for release.")
    # Update ASCII files with current project info
    edit_files(project_directory, project_branch, project_tag, project_files, version_string)
    # Make executable
    compile_project(project_directory, version_string)
    # Stage executable and INI to script directory
    copy_files(project_directory, project_exes)

    # End
    print(f"Project prep for {version_string} complete.")


if __name__ == "__main__":
    main()
