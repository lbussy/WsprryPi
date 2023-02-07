#!/bin/bash

# Copyright (C) 2023 Lee C. Bussy (@LBussy)

############
### Loop and keep Wsprry Pi running
############

loop() {
    # TODO:  Get options
    # TODO:  Concatenate path and exe
    # TODO:  Check for changes

    while :
    do
        # Run wspr
        true # DEBUG
    done
}

############
### Main function
############

main() {
    loop "$@" # Loop forever
}

main "$@" && exit 0
