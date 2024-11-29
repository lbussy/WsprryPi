#!/bin/bash

# Check if a path is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <path>"
    exit 1
fi

# Assign the provided path to a variable
TARGET_PATH="$1"

# Check if the path exists
if [ ! -d "$TARGET_PATH" ]; then
    echo "Error: The specified path does not exist or is not a directory."
    exit 1
fi

# Define color codes
GREEN="\033[32m"
GOLD="\033[33m"
RESET="\033[0m"

# Use `find` to locate .sh and .py files and process them
find "$TARGET_PATH" -type f \( -name "*.sh" -o -name "*.py" \) -print0 | while IFS= read -r -d '' file; do
    if [ -x "$file" ]; then
        # Already executable (green)
        echo -e "${GREEN}Already executable: $file${RESET}"
    else
        # Make executable and print in gold
        chmod +x "$file"
        echo -e "${GOLD}Made executable: $file${RESET}"
    fi
done
