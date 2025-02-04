#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# -----------------------------------------------------------------------------
# @file get_semantic_version.sh
# @brief Creates a semantic version from local repo.
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
# Returns a correctly formatted sematic version string.  Defaults to
# declarations in header.
#
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# @brief Defines global repository-related variables.
# @details These variables are used to determine the repository context,
#          including whether the script is running inside a local Git
#          repository or fetching data remotely. Default values are set
#          to ensure fallback behavior when values are unset.
#
# @global REPO_ORG The GitHub organization or user associated with the repository.
# @global REPO_NAME The name of the GitHub repository.
# @global REPO_TITLE A human-readable title for the repository.
# @global REPO_BRANCH The default branch of the repository.
# @global GIT_TAG The default Git tag/version to use.
# -----------------------------------------------------------------------------
declare REPO_ORG="${REPO_ORG:-lbussy}"
declare REPO_NAME="${REPO_NAME:-wsprrypi}"
declare REPO_TITLE="${REPO_TITLE:-Wsprry Pi}"
declare REPO_BRANCH="${REPO_BRANCH:-install_update}"
declare GIT_TAG="${GIT_TAG:-1.3.0}"

# -----------------------------------------------------------------------------
# @brief Starts the debug process.
# @details This function checks if the "debug" flag is present in the
#          arguments, and if so, prints the debug information including the
#          function name, the caller function name, and the line number where
#          the function was called.
#
# @param "$@" Arguments to check for the "debug" flag.
#
# @return Returns the "debug" flag if present, or an empty string if not.
#
# @example
# debug_start "debug"  # Prints debug information
# debug_start          # Does not print anything, returns an empty string
# -----------------------------------------------------------------------------
debug_start() {
    local debug=""
    local args=()  # Array to hold non-debug arguments

    # Look for the "debug" flag in the provided arguments
    for arg in "$@"; do
        if [[ "$arg" == "debug" ]]; then
            debug="debug"
            break  # Exit the loop as soon as we find "debug"
        fi
    done

    # Handle empty or unset FUNCNAME and BASH_LINENO gracefully
    local this_script
    this_script=$(basename "${THIS_SCRIPT:-main}")
    this_script="${this_script%.*}"
    local func_name="${FUNCNAME[1]:-main}"
    local caller_name="${FUNCNAME[2]:-main}"
    local caller_line=${BASH_LINENO[1]:-0}
    local current_line=${BASH_LINENO[0]:-0}

    # Print debug information if the flag is set
    if [[ "$debug" == "debug" ]]; then
        printf "[DEBUG]\t[%s:%s():%d] Starting function called by %s():%d.\n" \
            "$this_script" "$func_name" "$current_line"  "$caller_name" "$caller_line" >&2
    fi

    # Return the debug flag if present, or an empty string if not
    printf "%s\n" "${debug:-}"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Filters out the "debug" flag from the arguments.
# @details This function removes the "debug" flag from the list of arguments
#          and returns the filtered arguments. The debug flag is not passed
#          to other functions to avoid unwanted debug outputs.
#
# @param "$@" Arguments to filter.
#
# @return Returns a string of filtered arguments, excluding "debug".
#
# @example
# debug_filter "arg1" "debug" "arg2"  # Returns "arg1 arg2"
# -----------------------------------------------------------------------------
debug_filter() {
    local args=()  # Array to hold non-debug arguments

    # Iterate over each argument and exclude "debug"
    for arg in "$@"; do
        if [[ "$arg" != "debug" ]]; then
            args+=("$arg")
        fi
    done

    # Print the filtered arguments, safely quoting them for use in a command
    printf "%q " "${args[@]}"
}

# -----------------------------------------------------------------------------
# @brief Prints a debug message if the debug flag is set.
# @details This function checks if the "debug" flag is present in the
#          arguments. If the flag is present, it prints the provided debug
#          message along with the function name and line number from which the
#          function was called.
#
# @param "$@" Arguments to check for the "debug" flag and the debug message.
# @global debug A flag to indicate whether debug messages should be printed.
#
# @return None.
#
# @example
# debug_print "debug" "This is a debug message"
# -----------------------------------------------------------------------------
debug_print() {
    local debug=""
    local args=()  # Array to hold non-debug arguments

    # Loop through all arguments to identify the "debug" flag
    for arg in "$@"; do
        if [[ "$arg" == "debug" ]]; then
            debug="debug"
        else
            args+=("$arg")  # Add non-debug arguments to the array
        fi
    done

    # Restore the positional parameters with the filtered arguments
    set -- "${args[@]}"

    # Handle empty or unset FUNCNAME and BASH_LINENO gracefully
    local this_script
    this_script=$(basename "${THIS_SCRIPT:-main}")
    this_script="${this_script%.*}"
    local caller_name="${FUNCNAME[1]:-main}"
    local caller_line="${BASH_LINENO[0]:-0}"

    # Assign the remaining argument to the message, defaulting to <unset>
    local message="${1:-<unset>}"

    # Print debug information if the debug flag is set
    if [[ "$debug" == "debug" ]]; then
        printf "[DEBUG]\t[%s:%s:%d] '%s'.\n" \
               "$this_script" "$caller_name" "$caller_line" "$message" >&2
    fi
}

# -----------------------------------------------------------------------------
# @brief Ends the debug process.
# @details This function checks if the "debug" flag is present in the
#          arguments. If the flag is present, it prints debug information
#          indicating the exit of the function, along with the function name
#          and line number from where the function was called.
#
# @param "$@" Arguments to check for the "debug" flag.
# @global debug Debug flag, passed from the calling function.
#
# @return None
#
# @example
# debug_end "debug"
# -----------------------------------------------------------------------------
debug_end() {
    local debug=""
    local args=()  # Array to hold non-debug arguments

    # Loop through all arguments and identify the "debug" flag
    for arg in "$@"; do
        if [[ "$arg" == "debug" ]]; then
            debug="debug"
            break  # Exit the loop as soon as we find "debug"
        fi
    done

    # Handle empty or unset FUNCNAME and BASH_LINENO gracefully
    local this_script
    this_script=$(basename "${THIS_SCRIPT:-main}")
    this_script="${this_script%.*}"
    local func_name="${FUNCNAME[1]:-main}"
    local caller_name="${FUNCNAME[2]:-main}"
    local caller_line=${BASH_LINENO[1]:-0}
    local current_line=${BASH_LINENO[0]:-0}

    # Print debug information if the flag is set
    if [[ "$debug" == "debug" ]]; then
        printf "[DEBUG]\t[%s:%s():%d] Exiting function returning to %s():%d.\n" \
            "$this_script" "$func_name" "$current_line"  "$caller_name" "$caller_line" >&2
    fi
}

# -----------------------------------------------------------------------------
# @brief Retrieve the current Repo branch name or the branch this was detached
#        from.
# @details Attempts to dynamically fetch the branch name from the current Git
#          process. If not available, uses the global `$REPO_BRANCH` if set. If
#          neither is available, returns "unknown". Provides debugging output
#          when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @global REPO_BRANCH If set, uses this as the current Git branch name.
#
# @return Prints the branch name if available, otherwise "unknown".
# @retval 0 Success: the branch name is printed.
# @retval 1 Failure: prints an error message to standard error if the branch
#         name cannot be determined.
# -----------------------------------------------------------------------------
get_repo_branch() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local branch="${REPO_BRANCH:-}"  # Use existing $REPO_BRANCH if set
    local detached_from

    # Attempt to retrieve branch name dynamically from Git
    if [[ -z "$branch" ]]; then
        branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
        if [[ -n "$branch" && "$branch" != "HEAD" ]]; then
            debug_print "Retrieved branch name from Git: $branch" "$debug"
        elif [[ "$branch" == "HEAD" ]]; then
            # Handle detached HEAD state: attempt to determine the source
            detached_from=$(git reflog show --pretty='%gs' | grep -oE 'checkout: moving from [^ ]+' | head -n 1 | awk '{print $NF}')
            if [[ -n "$detached_from" ]]; then
                branch="$detached_from"
                debug_print "Detached HEAD state. Detached from branch: $branch" "$debug"
            else
                debug_print "Detached HEAD state. Cannot determine the source branch." "$debug"
                branch="unknown"
            fi
        fi
    fi

    # Use "unknown" if no branch name could be determined
    if [[ -z "$branch" ]]; then
        debug_print "Unable to determine Git branch. Returning 'unknown'." "$debug"
        branch="unknown"
    fi

    # Output the determined or fallback branch name
    printf "%s\n" "$branch"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Get the most recent Git tag.
# @details Attempts to dynamically fetch the most recent Git tag from the
#          current Git process. If not available, uses the global `$GIT_TAG` if
#          set. If neither is available, returns "0.0.1". Provides debugging
#          output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @global GIT_TAG If set, uses this as the most recent Git tag.
#
# @return Prints the tag name if available, otherwise "0.0.1".
# @retval 0 Success: the tag name is printed.
# -----------------------------------------------------------------------------
get_last_tag() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local tag

    # Attempt to retrieve the tag dynamically from Git
    tag=$(git describe --tags --abbrev=0 2>/dev/null)
    if [[ -n "$tag" ]]; then
        debug_print "Retrieved tag from Git: $tag" "$debug"
    else
        debug_print "No tag obtained from local repo." "$debug"
        # Try using GIT_TAG if it is set
        tag="${GIT_TAG:-}"
        # Fall back to "0.0.1" if both the local tag and GIT_TAG are unset
        if [[ -z "$tag" ]]; then
            tag="0.0.1"
            debug_print "No local tag and GIT_TAG is unset. Using fallback: $tag" "$debug"
        else
            debug_print "Using pre-assigned GIT_TAG: $tag" "$debug"
        fi
    fi

    # Output the tag
    printf "%s\n" "$tag"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Get the number of commits since the last tag.
# @details Counts commits since the provided Git tag using `git rev-list`. If
#          no tag is found, defaults to `0` commits. Debug messages are sent
#          only to `stderr`.
#
# @param $1 The Git tag to count commits from.
# @param $2 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return The number of commits since the tag, or 0 if the tag does not exist.
# -----------------------------------------------------------------------------
get_num_commits() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local tag="${1:-}"

    if [[ -z "$tag" || "$tag" == "0.0.1" ]]; then
        debug_print "No valid tag provided. Assuming 0 commits." "$debug"
        printf "0\n"
            debug_end "$debug"
        return 1
    fi

    local commit_count
    commit_count=$(git rev-list --count "${tag}..HEAD" 2>/dev/null || echo 0)

    printf "%s\n" "$commit_count"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Get the short hash of the current Git commit.
# @details Retrieves the short hash of the current Git commit. Provides
#          debugging output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return Prints the short hash of the current Git commit.
# @retval 0 Success: the short hash is printed.
# @retval 1 Failure: prints an error message to standard error if unable to
#         retrieve the hash.
# -----------------------------------------------------------------------------
get_short_hash() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local short_hash
    short_hash=$(git rev-parse --short HEAD 2>/dev/null)
    if [[ -z "$short_hash" ]]; then
        debug_print "No short hash available. Using 'unknown'." "$debug"
        short_hash=""
    else
        debug_print "Short hash of the current commit: $short_hash." "$debug"
    fi

    printf "%s\n" "$short_hash"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Check if there are uncommitted changes in the working directory.
# @details Checks for uncommitted changes in the current Git repository.
#          Provides debugging output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return Prints "true" if there are uncommitted changes, otherwise "false".
# @retval 0 Success: the dirty state is printed.
# @retval 1 Failure: prints an error message to standard error if unable to
#         determine the repository state.
# -----------------------------------------------------------------------------
get_dirty() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local changes

    # Check for uncommitted changes in the repository
    changes=$(git status --porcelain 2>/dev/null)

    if [[ -n "${changes:-}" ]]; then
        printf "true\n"
    else

        printf "false\n"
    fi

    if [[ -n "$changes" ]]; then
        debug_print "Changes detected." "$debug"
    else
        debug_print "No changes detected." "$debug"
    fi

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Generate a version string based on the state of the Git repository.
# @details Constructs a semantic version string using the most recent Git tag,
#          current branch name, number of commits since the tag, the short hash
#          of the latest commit, and whether the working directory is dirty.
#          Provides debugging output when the "debug" argument is passed.
#
# @param $1 [Optional] Pass "debug" to enable verbose debugging output.
#
# @return Prints the generated semantic version string.
# @retval 0 Success: the semantic version string is printed.
# @retval 1 Failure: prints an error message to standard error if any required
#         Git information cannot be determined.
# -----------------------------------------------------------------------------
get_sem_ver() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    local tag branch_name num_commits short_hash dirty version_string

    # Retrieve the most recent tag
    tag=$(get_last_tag "$debug")
    debug_print "Received tag: $tag from get_last_tag()." "$debug"
    if [[ -z "$tag" || "$tag" == "0.0.1" ]]; then
        debug_print "No semantic version tag found (or version is 0.0.1). Using default: 0.0.1" "$debug"
        version_string="0.0.1"
    else
        version_string="$tag"
    fi

    # Append branch name
    branch_name=$(get_repo_branch "$debug")
    version_string="$version_string-$branch_name"
    debug_print "Appended branch name to version: $branch_name" "$debug"

    # Append number of commits since the last tag
    num_commits=$(get_num_commits "$tag" "$debug")
    if [[ "$num_commits" -gt 0 ]]; then
        version_string="$version_string+$num_commits"
        debug_print "Appended commit count '$num_commits' to version." "$debug"
    fi

    # Append short hash of the current commit
    short_hash=$(get_short_hash "$debug")
    if [[ -n "$short_hash" ]]; then
        version_string="$version_string.$short_hash"
        debug_print "Appended short hash '$short_hash' to version." "$debug"
    fi

    # Check if the repository is dirty
    dirty=$(get_dirty "$debug")
    if [[ "$dirty" == "true" ]]; then
        version_string="$version_string-dirty"
        debug_print "Repository is dirty. Appended '-dirty' to version." "$debug"
    fi

    printf "%s\n" "$version_string"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Main execution function for the script.
# @details This function initializes the execution environment, processes
#          command-line arguments, verifies system compatibility, ensures
#          dependencies are installed, and executes either the install or
#          uninstall process for Wsprry Pi.
#
# @global ACTION Determines whether the script performs installation or
#                uninstallation.
#
# @param $@ Command-line arguments passed to the script.
#
# @throws Exits with an error code if any critical dependency is missing,
#         an unsupported environment is detected, or a step fails.
#
# @return 0 on successful execution, non-zero on failure.
#
# @note Debug mode (`"$debug"`) is propagated throughout all function calls.
# -----------------------------------------------------------------------------
_main() {
    local debug; debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"

    printf "%s\n" "$(get_sem_ver "$debug")"

    debug_end "$debug"
    return 0
}

# -----------------------------------------------------------------------------
# @brief Main function entry point.
# @details This function calls `_main` to initiate the script execution. By
#          calling `main`, we enable the correct reporting of the calling
#          function in Bash, ensuring that the stack trace and function call
#          are handled appropriately during the script execution.
#
# @param "$@" Arguments to be passed to `_main`.
# @return Returns the status code from `_main`.
# -----------------------------------------------------------------------------
main() { _main "$@"; return "$?"; }

debug=$(debug_start "$@"); eval set -- "$(debug_filter "$@")"
main "$@" "$debug"
retval="$?"
debug_end "$debug"
exit "$retval"
