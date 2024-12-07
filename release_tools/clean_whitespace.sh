#!/bin/bash
#
# This file is part of WsprryPi.
#
# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
#
# WsprryPi is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

# Exit immediately if a command exits with a non-zero status
set -e

# Array of default file extensions to process
extensions=(c cpp py h hpp md txt ini sh)

# Logging function (no timestamp)
log() {
    local level="$1"
    local message="$2"
    case $level in
        INFO) echo -e "\e[32m[$level]\e[0m $message" ;;  # Green
        ERROR) echo -e "\e[31m[$level]\e[0m $message" ;; # Red
        *) echo "[$level] $message" ;;
    esac
}

# Build regex from extensions array
build_regex() {
    local regex=".*\\.("
    for ext in "${extensions[@]}"; do
        regex+="${ext}\|"
    done
    regex="${regex%\\|})\$"
    echo "$regex"
}

# Validate custom extensions input
validate_extensions() {
    local valid_regex='^[a-zA-Z0-9]+(,[a-zA-Z0-9]+)*$'
    if [[ ! $custom_extensions =~ $valid_regex ]]; then
        log "ERROR" "Invalid extensions format. Use a comma-separated list (e.g., txt,md)."
        exit 1
    fi
}

# Process a single file
process_file() {
    local file="$1"
    if $dry_run; then
        log "INFO" "Dry-run mode: Would process $file"
    else
        if sed -i -E 's/[[:space:]]*$//' "$file"; then
            ((success_count++))
            log "INFO" "Processed $file"
        else
            ((failure_count++))
            log "ERROR" "Failed to process $file"
        fi
    fi
}

# Process files in a directory
process_files() {
    local dir="$1"
    local recursive="$2"
    local regex

    regex=$(build_regex)  # Build the regex

    if $recursive; then
        log "INFO" "Processing files recursively from directory: $dir"
        find "$dir" -type f -iregex "$regex" "${exclude_args[@]}" -print0 | \
            xargs -0 -P "$(nproc)" -I {} bash -c 'process_file "$@"' _ {}
    else
        log "INFO" "Processing files in directory: $dir"
        find "$dir" -maxdepth 1 -type f -iregex "$regex" "${exclude_args[@]}" -print0 | \
            xargs -0 -P "$(nproc)" -I {} bash -c 'process_file "$@"' _ {}
    fi
}

# Display usage instructions
usage() {
    echo "Usage: $0 [-r] [-n] [-x exclude_dir] [-e extensions] [directory]"
    echo "  -r          Process files recursively"
    echo "  -n          Dry-run mode (no changes made)"
    echo "  -x dir      Exclude specified directory"
    echo "  -e ext      Comma-separated list of file extensions to process"
    echo "  directory   Directory to process (default: current directory)"
    exit 1
}

# Default values
recursive=false
dry_run=false
directory="."
excludes=()
custom_extensions=""

# Parse options
while getopts ":rnx:e:" opt; do
    case $opt in
        r) recursive=true ;;
        n) dry_run=true ;;
        x) excludes+=("$OPTARG") ;;
        e) custom_extensions="$OPTARG" ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

# Validate custom extensions if provided
if [[ -n $custom_extensions ]]; then
    validate_extensions
    IFS=',' read -r -a extensions <<< "$custom_extensions"  # Split extensions by comma
fi

# Set directory argument if provided
if [[ $# -ge 1 ]]; then
    directory="$1"
    if [[ ! -d "$directory" ]]; then
        log "ERROR" "$directory is not a valid directory."
        exit 1
    fi
fi

# Build exclusion arguments for find
exclude_args=()
for exclude in "${excludes[@]}"; do
    exclude_args+=(-not -path "$exclude/*")
done

# Initialize counters
success_count=0
failure_count=0

# Run the processing function
process_files "$directory" "$recursive"

# Summary of results
log "INFO" "Summary: $success_count files processed successfully, $failure_count failures."
