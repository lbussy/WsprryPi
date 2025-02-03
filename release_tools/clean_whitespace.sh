#!/usr/bin/env bash
set -uo pipefail # Setting -e is far too much work here
IFS=$'\n\t'

##
# @file clean_whitespace.sh
# @brief Cleans whitespace from files.
#
# @details
# This script processes files by cleaning trailing whitespace from them. It includes:
# - Robust logging for INFO and ERROR levels.
# - Recursive or non-recursive file processing options.
# - Customizable file extensions for processing.
# - Dry-run mode for previewing changes.
# - Exclusion of specific directories from processing.
#
# @author Lee Bussy
# @date December 21, 2024
# @version 1.0.0
#
# @par Usage:
# ```bash
# ./script_name.sh [-r] [-n] [-x exclude_dir] [-e extensions] [directory]
# ```
##

# -----------------------------------------------------------------------------
# @var extensions
# @brief Array of default file extensions to process.
# -----------------------------------------------------------------------------
extensions=(c cpp py h hpp md txt ini sh)

# -----------------------------------------------------------------------------
# @brief Log messages with specified severity levels.
# -----------------------------------------------------------------------------
log() {
    local level="$1"
    local message="$2"
    case $level in
        INFO) echo -e "\e[32m[$level]\e[0m $message" ;;  # Green
        ERROR) echo -e "\e[31m[$level]\e[0m $message" ;; # Red
        *) echo "[$level] $message" ;;
    esac
}

# -----------------------------------------------------------------------------
# @brief Build a regex pattern from the extensions array.
# -----------------------------------------------------------------------------
build_regex() {
    local regex=".*\\.("
    for ext in "${extensions[@]}"; do
        regex+="${ext}\|"
    done
    regex="${regex%\\|})\$"
    echo "$regex"
}

# -----------------------------------------------------------------------------
# @brief Validate custom extensions input.
# -----------------------------------------------------------------------------
validate_extensions() {
    local valid_regex='^[a-zA-Z0-9]+(,[a-zA-Z0-9]+)*$'
    if [[ ! $custom_extensions =~ $valid_regex ]]; then
        log "ERROR" "Invalid extensions format. Use a comma-separated list (e.g., txt,md)."
        exit 1
    fi
}

# -----------------------------------------------------------------------------
# @brief Process a single file to remove trailing whitespace.
# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# @brief Process files in a directory.
# -----------------------------------------------------------------------------
process_files() {
    local dir="$1"
    local recursive="$2"
    local regex

    regex=$(build_regex)

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

# -----------------------------------------------------------------------------
# @brief Display usage instructions.
# -----------------------------------------------------------------------------
usage() {
    echo "Usage: $0 [-r] [-n] [-x exclude_dir] [-e extensions] [directory]"
    echo "  -r          Process files recursively"
    echo "  -n          Dry-run mode (no changes made)"
    echo "  -x dir      Exclude specified directory"
    echo "  -e ext      Comma-separated list of file extensions to process"
    echo "  directory   Directory to process (default: current directory)"
    exit 1
}

# -----------------------------------------------------------------------------
# @brief Main function orchestrating the script execution.
# -----------------------------------------------------------------------------
main() {
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
        IFS=',' read -r -a extensions <<< "$custom_extensions"
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
}

# Invoke the main function
main "$@"
