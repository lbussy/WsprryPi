#!/bin/bash

# Copyright (C) 2023 Lee C. Bussy (@LBussy)

declare GITROOT

############
### Init
############

init() {
    # TODO: Do we need to check anythign like file exists?
}

############
### Loop and keep Wsprry Pi running
############

loop() {
    local script python
    # TODO:  Get options
    # TODO:  Concatenate path and exe
    script="xxxxxxx"

    # TODO:  Maybe check for existence of card file for parameters?

    while :
    do
        # Run wspr
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
