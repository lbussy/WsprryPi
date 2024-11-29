#!/usr/bin/env python

"""
@file make_version.py
@brief Generate a version string based on Git repository state.

This script produces a version string based on the current Git branch, tag, commit count, 
short hash, and whether there are uncommitted changes. It adheres to semantic versioning 
principles when applicable.
"""

import subprocess
import re


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

    - If no tags exist on `main` or `master`, the version string will use an extended format.
    - For other branches or tagged states, it adheres to semantic versioning principles.

    @return Generated version string.
    """
    if not is_git_repository():
        return "0.0.0-detached"

    branch_name = get_branch_name()
    tag = get_last_tag()
    sem_ver = tag if is_semantic_version(tag) else "0.0.0"
    pre_release = get_pre_release_info(tag)
    num_commits = get_num_commits_since_tag(tag) or 0
    short_hash = get_short_hash()
    dirty = "-dirty" if has_uncommitted_changes() else ""

    # Use extended format when on main or master with no tags
    if branch_name in ["main", "master"] and not tag:
        version_string = f"0.0.0-{branch_name}"
        if num_commits > 0 and short_hash:
            version_string += f"+{num_commits}.{short_hash}"
        elif num_commits > 0:
            version_string += f"+{num_commits}"
        version_string += dirty
        return version_string

    # Construct the version string
    version_string = sem_ver

    if branch_name not in ["main", "master"]:
        version_string += f"-{branch_name}"

    if pre_release:
        version_string += pre_release

    if num_commits > 0 and short_hash:
        version_string += f"+{num_commits}.{short_hash}"
    elif num_commits > 0:
        version_string += f"+{num_commits}"

    version_string += dirty

    return version_string


if __name__ == "__main__":
    print(generate_version_string())
