#!/usr/bin/env python

import re
import os
import sys
import subprocess
from make_version import generate_version_string

__version__ = "1.2.3"


# List of files to update
update_file_list = [
    "install.sh",           # Installer script
    "uninstall.sh",         # Uninstaller script
    "logrotate.conf",       # Logrotate configuration file
    "shutdown_watch.py",    # Shutdown daemon Python script
]

# Directory containing the target files to update
target_file_directory = "scripts"


def get_git_root():
    """
    Get the root directory of the current Git repository.

    This function runs `git rev-parse --show-toplevel` to determine the 
    root directory of the current Git repository. If the current directory 
    is not inside a Git repository, it returns None.

    @return: The root directory of the Git repository or None if not inside a Git repo.
    @rtype: str or None
    """
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


def get_git_branch():
    """
    Get the current Git branch name.

    This function runs `git rev-parse --abbrev-ref HEAD` to retrieve the name 
    of the current branch of the repository. If the directory is not a Git 
    repository, it returns None.

    @return: The current Git branch name or None if not inside a Git repo.
    @rtype: str or None
    """
    try:
        # Run the Git command to get the current branch
        branch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])
        # Decode the byte string to get the branch name
        return branch.decode('utf-8').strip()
    except subprocess.CalledProcessError:
        return None  # Return None if not in a git repository

# Get and print the current branch
branch = get_git_branch()
if branch:
    print(f"Current Git branch: {branch}")
else:
    print("Not in a Git repository.")


def update_version_in_file(file_path, new_version, branch):
    """
    Update the VERSION and BRANCH in a given file.

    This function reads the file, searches for lines starting with `BRANCH=`, 
    `VERSION=`, and `__version__`, and updates their values with the new version
    and the current Git branch.

    @param file_path: Path to the file to update.
    @type file_path: str
    @param new_version: The new version string to set in the file.
    @type new_version: str
    @param branch: The current Git branch name to set in the file.
    @type branch: str
    """
    with open(file_path, 'r') as file:
        lines = file.readlines()

    updated_lines = []
    for line in lines:
        # Update BRANCH= line to reflect the current branch
        if line.startswith("BRANCH="):
            # Set the BRANCH line to the current branch, with quotes around it
            updated_line = f'BRANCH="{branch}"\n'
            updated_lines.append(updated_line)  # Add the updated line
            continue  # Skip the remaining part of the loop and move to the next line
        
        # Update VERSION= line to the new version string
        if line.startswith("VERSION="):
            line = f'VERSION="{new_version}"\n'

        # Update Python __version__ variable in the script
        elif "__version__" in line:
            line = f'__version__ = "{new_version}"\n'

        # Update Unix configuration version header
        elif line.strip().startswith("# Created for the WsprryPi project,"):
            line = re.sub(r"version .*$", f"version {new_version}.", line)

        updated_lines.append(line)

    # Write the updated content back to the file
    with open(file_path, 'w') as file:
        file.writelines(updated_lines)


def update_file(file_path, new_version):
    """
    Update a single file by setting its version and branch.

    This function fetches the current Git branch and calls `update_version_in_file` 
    to update the contents of the file with the new version and branch.

    @param file_path: The path to the file to update.
    @type file_path: str
    @param new_version: The new version string to update in the file.
    @type new_version: str
    """
    branch = get_git_branch()  # Call the function to get the current branch
    if not branch:
        print("Error: No git branch returned.", file=sys.stderr)
        sys.exit(1)  # Exit with a non-zero status code for failure
    update_version_in_file(file_path, new_version, branch)  # Update the file with the version and branch


def update_files(files_to_update, new_version):
    """
    Update multiple files by setting their version and branch.

    This function checks if the current directory is a Git repository, then 
    loops through a list of files and updates each of them using 
    `update_file`.

    @param files_to_update: List of file paths to update.
    @type files_to_update: list of str
    @param new_version: The new version string to set in the files.
    @type new_version: str
    """
    git_root = get_git_root()
    if not git_root:
        print("Error: This is not a Git repository.", file=sys.stderr)
        sys.exit(1)  # Exit with a non-zero status code for failure
    
    # Update each file in the provided list
    for file_path in files_to_update:
        full_path = os.path.join(git_root, target_file_directory, file_path)
        update_file(full_path, new_version)


def main(files_to_update):
    """
    Main function to generate a new version string and update the files.

    This function calls `generate_version_string` to get a new version string, 
    then updates the files in `files_to_update` with the new version and the 
    current Git branch.

    @param files_to_update: List of files to update.
    @type files_to_update: list of str
    """
    # Generate the new version string
    new_version = generate_version_string()
    print(f"Generated new version string: {new_version}")
    update_files(files_to_update, new_version)


if __name__ == "__main__":
    # Run the main function with the list of files to update
    main(update_file_list)
