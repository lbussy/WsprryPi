#!/bin/bash

# @file git_utils.sh
# @brief A utility script for working with Git repositories, including retrieving repository details, versioning, and branch management.
# @version 1.0
# @date 2024-12-08

##
# @brief Logging-related constants for the script.
# @details Sets the script name (`THIS_SCRIPT`) based on the current environment.
# If `THIS_SCRIPT` is already defined, its value is retained; otherwise, it is set
# to the basename of the script or defaults to "script.sh".
#
# @global THIS_SCRIPT The name of the script.
##
declare THIS_SCRIPT="${THIS_SCRIPT:-install.sh}" # Use existing value, or default to "install.sh".

##
# @brief Project metadata constants used throughout the script.
# @details These variables provide metadata about the script, including ownership,
# versioning, and project details. All are marked as read-only.
##
#declare SEM_VER="1.2.1-version-files+91.3bef855-dirty"
#declare GIT_BRCH="install_update"
declare REPO_ORG="${REPO_ORG:-lbussy}"
declare REPO_NAME="${REPO_NAME:-WsprryPi}"
declare GIT_BRCH="${GIT_BRCH:-main}"
declare SEM_VER="${SEM_VER:-1.2.0}"
declare USE_LOCAL="${USE_LOCAL:-false}"
declare GIT_DEF_BRCH=("main" "master")

# @brief Retrieve the Git owner or organization name from the remote URL.
#
# This function fetches the remote URL of the current Git repository using the
# `git config --get remote.origin.url` command. It extracts the owner or
# organization name, which is the first path segment after the domain in the URL.
# If not inside a Git repository or no remote URL is configured, an error
# message is displayed, and the script exits with a non-zero status.
#
# @return Prints the owner or organization name to standard output if successful.
# @retval 0 Success: the owner or organization name is printed.
# @retval 1 Failure: prints an error message to standard error.
get_repo_org() {
    local url organization

    # Retrieve the remote URL from Git configuration.
    url=$(git config --get remote.origin.url)

    # Check if the URL is non-empty.
    if [[ -n "$url" ]]; then
        # Extract the owner or organization name.
        # Supports HTTPS and SSH Git URLs.
        organization=$(echo "$url" | sed -E 's#(git@|https://)([^:/]+)[:/]([^/]+)/.*#\3#')
        echo "$organization"
    else
        echo "Error: Not inside a Git repository or no remote URL configured." >&2
        exit 1
    fi
}

# @brief Retrieve the Git project name from the remote URL.
#
# This function fetches the remote URL of the current Git repository using the
# `git config --get remote.origin.url` command. It extracts the repository name
# from the URL, removing the `.git` suffix if present. If not inside a Git
# repository or no remote URL is configured, an error message is displayed,
# and the script exits with a non-zero status.
#
# @return Prints the project name to standard output if successful.
# @retval 0 Success: the project name is printed.
# @retval 1 Failure: prints an error message to standard error.
get_repo_name() {
    local url repo_name

    # Retrieve the remote URL from Git configuration.
    url=$(git config --get remote.origin.url)

    # Check if the URL is non-empty.
    if [[ -n "$url" ]]; then
        # Extract the repository name from the URL and remove the ".git" suffix if present.
        repo_name="${url##*/}"       # Remove everything up to the last `/`.
        repo_name="${repo_name%.git}" # Remove the `.git` suffix.
        echo "$repo_name"
    else
        echo "Error: Not inside a Git repository or no remote URL configured." >&2
        exit 1
    fi
}

# @brief Retrieve the current Git branch name or the branch this was detached from.
#
# This function fetches the name of the currently checked-out branch in a Git
# repository. If the repository is in a detached HEAD state, it attempts to
# determine the branch or tag the HEAD was detached from. If not inside a
# Git repository, it displays an appropriate error message.
#
# @return Prints the current branch name or detached source to standard output.
# @retval 0 Success: the branch or detached source name is printed.
# @retval 1 Failure: prints an error message to standard error.
get_git_branch() {
    local branch detached_from

    # Retrieve the current branch name using `git rev-parse`.
    branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)

    if [[ -n "$branch" && "$branch" != "HEAD" ]]; then
        # Print the branch name if available and not in a detached HEAD state.
        echo "$branch"
    elif [[ "$branch" == "HEAD" ]]; then
        # Handle the detached HEAD state: attempt to determine the source.
        detached_from=$(git reflog show --pretty='%gs' | grep -oE 'checkout: moving from [^ ]+' | head -n 1 | awk '{print $NF}')
        if [[ -n "$detached_from" ]]; then
            echo "Detached from branch: $detached_from"
        else
            echo "Detached HEAD state: Cannot determine the source branch." >&2
            exit 1
        fi
    else
        echo "Error: Not inside a Git repository." >&2
        exit 1
    fi
}

# @brief Get the most recent Git tag.
# @return The most recent Git tag, or nothing if no tags exist.
get_last_tag() {
    local tag

    # Retrieve the most recent Git tag
    tag=$(git describe --tags --abbrev=0 2>/dev/null)

    echo "$tag"
}

# @brief Check if a tag follows semantic versioning.
# @param tag The Git tag to validate.
# @return "true" if the tag follows semantic versioning, otherwise "false".
is_sem_ver() {
    local tag="$1"

    # Validate if the tag follows the semantic versioning format (major.minor.patch)
    if [[ "$tag" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo "true"
    else
        echo "false"
    fi
}

# @brief Get the number of commits since the last tag.
# @param tag The Git tag to count commits from.
# @return The number of commits since the tag, or 0 if the tag does not exist.
get_num_commits() {
    local tag="$1" commit_count

    # Count the number of commits since the given tag
    commit_count=$(git rev-list --count "${tag}..HEAD" 2>/dev/null || echo 0)

    echo "$commit_count"
}

# @brief Get the short hash of the current Git commit.
# @return The short hash of the current Git commit.
get_short_hash() {
    local short_hash

    # Retrieve the short hash of the current Git commit
    short_hash=$(git rev-parse --short HEAD 2>/dev/null)

    echo "$short_hash"
}

# @brief Check if there are uncommitted changes in the working directory.
# @return "true" if there are uncommitted changes, otherwise "false".
get_dirty() {
    local changes

    # Check for uncommitted changes in the repository
    changes=$(git status --porcelain 2>/dev/null)

    if [[ -n "$changes" ]]; then
        echo "true"
    else
        echo "false"
    fi
}

# @brief Generate a version string based on the state of the Git repository.
# @return The generated semantic version string.
get_sem_ver() {
    local branch_name git_tag num_commits short_hash dirty version_string

    # Determine if the latest tag is a semantic version
    tag=$(get_last_tag)
    if [[ "$(is_sem_ver "$tag")" == "true" ]]; then
        version_string="$tag"
    else
        version_string="1.0.0" # Use default version if no valid tag exists
    fi

    # Retrieve the current branch name
    branch_name=$(get_git_branch)
    version_string="$version_string-$branch_name"

    # Get the number of commits since the last tag and append it to the tag
    num_commits=$(get_num_commits "$tag")
    if [[ "$num_commits" -gt 0 ]]; then
        version_string="$version_string+$num_commits"
    fi

    # Get the short hash and append it to the tag
    short_hash=$(get_short_hash)
    if [[ -n "$short_hash" ]]; then
        version_string="$version_string.$short_hash"
    fi

    # Check for a dirty working directory
    dirty=$(get_dirty)
    if [[ "$dirty" == "true" ]]; then
        version_string="$version_string-dirty"
    fi

    echo "$version_string"
}

# @brief Configure local or remote mode based on the Git repository context.
#
# This function sets relevant variables for local mode if `USE_LOCAL` is true.
# Defaults to remote configuration if not in local mode.
#
# @global USE_LOCAL           Indicates whether local mode is enabled.
# @global THIS_SCRIPT          Name of the current script.
# @global REPO_ORG            Git organization or owner name.
# @global REPO_NAME           Git repository name.
# @global GIT_BRCH            Current Git branch name.
# @global SEM_VER             Generated semantic version string.
# @global LOCAL_SOURCE_DIR    Path to the root of the local repository.
# @global LOCAL_WWW_DIR       Path to the `data` directory in the repository.
# @global LOCAL_SCRIPTS_DIR   Path to the `scripts` directory in the repository.
# @global GIT_RAW             URL for accessing raw files remotely.
# @global GIT_API             URL for accessing the repository API.
get_proj_params() {
    if [[ "$USE_LOCAL" == "true" ]]; then
        THIS_SCRIPT=$(basename "$0")
        REPO_ORG=$(get_repo_org)
        REPO_NAME=$(get_repo_name)
        GIT_BRCH=$(get_git_branch)
        SEM_VER=$(get_sem_ver)

        # Get the root directory of the repository
        LOCAL_SOURCE_DIR=$(git rev-parse --show-toplevel 2>/dev/null)
        if [[ -z "$LOCAL_SOURCE_DIR" ]]; then
            echo "Error: Not inside a Git repository." >&2
            exit 1
        fi

        LOCAL_WWW_DIR="$LOCAL_SOURCE_DIR/data"
        LOCAL_SCRIPTS_DIR="$LOCAL_SOURCE_DIR/scripts"
    else
        GIT_RAW="https://raw.githubusercontent.com/$REPO_ORG/$REPO_NAME"
        GIT_API="https://api.github.com/repos/$REPO_ORG/$REPO_NAME"
    fi

    export THIS_SCRIPT REPO_ORG REPO_NAME GIT_BRCH SEM_VER LOCAL_SOURCE_DIR
    export LOCAL_WWW_DIR LOCAL_SCRIPTS_DIR GIT_RAW GIT_API
}

##
# @brief Main function to test Git utility functions.
#
# This function performs tests in both local and remote modes. In local mode,
# it retrieves repository details (e.g., repository name, organization, branch name,
# and version). In remote mode, it constructs URLs for accessing raw files and the
# GitHub API. The function ensures that output is cleanly formatted and easy to read.
#
# @global USE_LOCAL           Boolean flag to toggle between local and remote modes.
# @global THIS_SCRIPT          Name of the current script.
# @global REPO_ORG            Git organization or owner name.
# @global REPO_NAME           Git repository name.
# @global GIT_BRCH            Current Git branch name.
# @global SEM_VER             Generated semantic version string.
# @global LOCAL_SOURCE_DIR    Path to the root of the local repository.
# @global LOCAL_WWW_DIR       Path to the `data` directory in the repository.
# @global LOCAL_SCRIPTS_DIR   Path to the `scripts` directory in the repository.
# @global GIT_RAW             URL for accessing raw files remotely.
# @global GIT_API             URL for accessing the repository API.
#
# @return None Outputs the results of the tests to standard output.
##
main() {
    # Clear the terminal for clean output
    clear

    # Test Default Mode
    echo
    echo "=== Default Mode ==="
    echo "Use Local: $USE_LOCAL"
    echo "Script: $THIS_SCRIPT"
    echo "Repo Org: $REPO_ORG"
    echo "Repo Name: $REPO_NAME"
    echo "Branch: $GIT_BRCH"
    echo "Semantic Version: $SEM_VER"
    echo "Raw URL: $GIT_RAW"
    echo "API URL: $GIT_API"
    echo

    # Test Remote Mode
    USE_LOCAL=false
    get_proj_params
    echo
    echo "=== Online Mode ==="
    echo "Use Local: $USE_LOCAL"
    echo "Script: $THIS_SCRIPT"
    echo "Repo Org: $REPO_ORG"
    echo "Repo Name: $REPO_NAME"
    echo "Branch: $GIT_BRCH"
    echo "Semantic Version: $SEM_VER"
    echo "Raw URL: $GIT_RAW"
    echo "API URL: $GIT_API"
    echo

    # Test Local Mode
    USE_LOCAL=true
    get_proj_params
    echo
    echo "=== Local Mode ==="
    echo "Use Local: $USE_LOCAL"
    echo "Script: $THIS_SCRIPT"
    echo "Repo Org: $REPO_ORG"
    echo "Repo Name: $REPO_NAME"
    echo "Branch: $GIT_BRCH"
    echo "Semantic Version: $SEM_VER"
    echo "Source Dir: $LOCAL_SOURCE_DIR"
    echo "WWW Dir: $LOCAL_WWW_DIR"
    echo "Scripts Dir: $LOCAL_SCRIPTS_DIR"
    echo
}

# Run the main function and exit with its return status
main "$@"
exit $?
