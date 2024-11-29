#!/bin/bash

declare VERSION_STRING

# Check if we are in a git repository
if ! git rev-parse --is-inside-work-tree &>/dev/null; then
  # If not in a git repository, set default values
  VERSION_STRING="0.0.0-detached"
else
    # Get sanitized branch name (replace non-alphanumeric chars with hyphens)
    branch_name=$(git symbolic-ref --short HEAD 2>/dev/null || echo "detached")
    branch_name=$(echo "$branch_name" | sed -E 's/^$/none/; s/[^0-9A-Za-z./-]/-/g; s/--+/-/g; s/^/-/; s/^-+|-+$//g')

    # Get semantic version based on last tag, default to 0.0.0 if not found
    sem_ver=$(git describe --tags --abbrev=0 2>/dev/null)
    if [ -z "$sem_ver" ]; then
        sem_ver="0.0.0"
    fi

    # Extract pre-release info, sanitized
    pre_release=$(git describe --tags --abbrev=0 2>/dev/null | sed -En 's/^[^0-9]*//; s/^(([0-9]+\.){2}[0-9]+)-([-0-9A-Za-z.-]+).*$/\3/p')
    pre_release=$(echo "$pre_release" | sed -E 's/[^0-9A-Za-z-]//g; s/([0-9]+)\.0*/\1/g; s/^\.+//; s/\.+$//; s/\.+/\./g; s/(\b0+([1-9][0-9]*)?\b)/\2/g')

    # Get number of commits since last tag (only if describe succeeds)
    num_commits=""
    if [ "$sem_ver" != "0.0.0" ]; then
        num_commits=$(git rev-list --count "$sem_ver"..HEAD)
    else
        num_commits=0
    fi

    # If there are no commits, set num_commits to empty string
    if [ -z "$num_commits" ] || [ "$num_commits" -eq 0 ]; then
        num_commits=""
    fi

    # Get short hash of HEAD commit
    short_hash=$(git rev-parse --short HEAD 2>/dev/null || echo "")

    # Check for uncommitted changes using git status --porcelain
    dirty=""
    if [ -n "$(git status --porcelain)" ]; then
        dirty="-dirty"  # Uncommitted changes or untracked files exist
    fi

    # Construct version string
    VERSION_STRING="$sem_ver"

    # If on main or master, and no commits or changes, just use the base version
    if [[ ("$branch_name" == "main" || "$branch_name" == "master") && -z "$num_commits" && -z "$dirty" ]]; then
        VERSION_STRING="$sem_ver"  # Use base version only
    else
        # Append branch name if not on main or master
        if [[ "$branch_name" != "main" && "$branch_name" != "master" ]]; then
            VERSION_STRING+="-${branch_name}"
        fi

        # Append pre-release info if available
        if [ -n "$pre_release" ]; then
            VERSION_STRING+="$pre_release"
        fi

        # Append commit count and hash if applicable
        if [[ -n "$num_commits" && -n "$short_hash" ]]; then
            VERSION_STRING+="+$num_commits.$short_hash"
        elif [[ -n "$num_commits" ]]; then
            VERSION_STRING+="+$num_commits"
        fi

        # Append dirty flag if applicable
        VERSION_STRING+="$dirty"
    fi

fi

# Output the final version string
echo $VERSION_STRING

