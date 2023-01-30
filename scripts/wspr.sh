#!/bin/bash

# Copyright (C) 2023 Lee C. Bussy (@LBussy)

declare GITROOT

############
### Init
############

init() {
    # Change to current dir (assumed to be in a repo) so we can get the git info
    pushd . &> /dev/null || exit 1
    SCRIPTPATH="$( cd "$(dirname "$0")" || exit 1 ; pwd -P )"
    cd "$SCRIPTPATH" || exit 1 # Move to where the script is
    GITROOT="$(git rev-parse --show-toplevel)" &> /dev/null
    if [ -z "$GITROOT" ]; then
        echo -e "\nERROR: Unable to find my repository, did you move this file or not run as root?"
        popd &> /dev/null || exit 1
        exit 1
    fi

    # Get project constants
    # shellcheck source=/dev/null
    . "$GITROOT/inc/const.inc" "$@"

    # Get BrewPi user directory
    USERROOT=$(echo "$GITROOT" | cut -d "/" -f-3)

    # Get error handling functionality
    # shellcheck source=/dev/null
    . "$GITROOT/inc/error.inc" "$@"

    # Get help and version functionality
    # shellcheck source=/dev/null
    . "$GITROOT/inc/help.inc" "$@"
}

############
### Loop and keep Brewpi running
############

loop() {
    local script python
    script="$GITROOT/brewpi.py"
    python="$USERROOT/venv/bin/python3"

    while :
    do
        if ("$python" -u "$script" --check --donotrun); then
            USE_TIMESTAMP_LOG=true "$python" -u "$script" --log --datetime
        else
            sleep 1
        fi
    done
}

############
### Main function
############

main() {
    init "$@" # Get environment information
    help "$@" # Process help and version requests
    loop "$@" # Loop forever
}

main "$@" && exit 0
