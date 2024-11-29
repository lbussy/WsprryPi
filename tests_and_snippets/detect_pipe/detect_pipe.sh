#!/bin/bash

# Check if the script is run directly or piped
if [ "$0" == "bash" ]; then
    if [ -p /dev/stdin ]; then
        echo "The script is being piped through bash."
    else
        echo "The script was run in an unusual way with 'bash'."
    fi
else
    echo "The script was run directly: $0"
fi
