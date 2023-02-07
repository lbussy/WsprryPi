#!/bin/bash
# Created for Wsprry Pi version 0.1

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
        sleep 1
    done
}

############
### Main function
############

main() {
    loop "$@" # Loop forever
}

main "$@" && exit 0
