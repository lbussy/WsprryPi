#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Function to process files
process_files() {
    local dir="$1"
    local recursive="$2"

    if $recursive; then
        echo "Processing files recursively from directory: $dir"
        # Find files matching extensions case-insensitively and process them
        find "$dir" -type f -iregex '.*\.\(c\|cpp\|py\|h\|hpp\)$' -print0 | while IFS= read -r -d '' file; do
            echo "Processing $file..."
            sed -i 's/[[:space:]]\+$//' "$file"
        done
    else
        echo "Processing files in directory: $dir"
        # Find files in the specified directory only (non-recursive)
        find "$dir" -maxdepth 1 -type f -iregex '.*\.\(c\|cpp\|h\|hpp\)$' -print0 | while IFS= read -r -d '' file; do
            echo "Processing $file..."
            sed -i 's/[[:space:]]\+$//' "$file"
        done
    fi
}

# Display usage instructions
usage() {
    echo "Usage: $0 [-r] [directory]"
    echo "  -r          Process files recursively"
    echo "  directory   The directory to process (default: current directory)"
    exit 1
}

# Default values
recursive=false
directory="."

# Parse options
while getopts ":r" opt; do
    case $opt in
        r) recursive=true ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

# Get the directory argument if provided
if [[ $# -ge 1 ]]; then
    directory="$1"
    if [[ ! -d "$directory" ]]; then
        echo "Error: $directory is not a valid directory."
        exit 1
    fi
fi

# Run the processing function
process_files "$directory" "$recursive"

echo "Trailing whitespace removal completed."
