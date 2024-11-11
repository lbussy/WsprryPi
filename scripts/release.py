#!/usr/bin/python3
"""Module compiles the project and prepares script headers to match the environment."""

import subprocess
import os
import sys
import locale
from fileinput import FileInput

# Text-based and executable files in project
PROJECT_FILES = ["install.sh", "uninstall.sh", "shutdown-watch.py", "shutdown_watch.py"]
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
                file.write(replace_string)
                print(f"Replacing '{line.strip()}' with '{replace_string}'.")
            else:
                file.write(line)


def edit_files(project_directory, project_name, project_branch, project_tag, files):
    """Function to iterate a list of files to replace file and script properties with Git info."""
    for file in files:
        print(f"Updating {file}.")
        file_name = project_directory + "/scripts/" + file
        replace_in_file(file_name, "# Created for ", "# Created for " + project_name + " version " + project_tag)
        replace_in_file(file_name, "BRANCH=", "BRANCH=" + project_branch)
        replace_in_file(file_name, "VERSION=", "VERSION=" + project_tag)


def compile_project(project_directory, project_name, project_tag):
    """Function to make current project."""
    current_dir = os.getcwd()
    source_dir = project_directory + "/src"
    os.chdir(source_dir)
    print(f"Compiling {project_name} version {project_tag}.")
    compile_command = "make clean && make"
    subprocess.check_output(compile_command, shell=True)
    os.chdir(current_dir)


def copy_files(project_directory, files):
    """Function to stage exe files to scroipts directory."""
    for file in files:
        copy_command = (
            "cp -f "
            + project_directory
            + "src/"
            + file
            + " "
            + project_directory
            + "scripts/"
        )
        print(f"Copying {file} to scripts location.")
        subprocess.check_output(copy_command, shell=True)


def main():
    """Function handles project compile and file edits."""
    project_files = PROJECT_FILES
    project_exes = PROJECT_EXES
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

    # Update ASCII files with current project info
    edit_files(project_directory, project_name, project_branch, project_tag, project_files)
    # Make executable
    #compile_project(project_directory, project_name, project_tag)
    # Stage executable and INI to script directory
    #copy_files(project_directory, project_exes)


if __name__ == "__main__":
    main()
