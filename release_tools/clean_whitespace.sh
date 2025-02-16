#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

declare DEBUG_MODE="${DEBUG_MODE:=false}"  # Set default if not set

# -----------------------------------------------------------------------------
# @file clean_whitespace.sh
# @brief Brief description of what the script does.
# @details A more detailed explanation of the script’s purpose, including its
#          functionality, expected input/output, and any relevant dependencies.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.2.1-config_lib+50.0985f26-dirty
# @date 2025-02-16
# @copyright MIT License
#
# @license
# MIT License
#
# Copyright (c) 2023-2025 Lee C. Bussy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# @usage
# ./clean_whitespace.sh [-r] [-n] [-x exclude_dir] [-e extensions] [directory]
#
# @example
# ./clean_whitespace.sh -r
# ./clean_whitespace.sh -n -x "logs" -e "txt,md"
#
# @param -r                 Process files recursively
# @param -n                 Dry-run mode (no changes made)
# @param -x <dir>           Exclude specified directory
# @param -e <ext1, ext2>    Comma-separated list of file extensions to process
# @param <directory>        Directory to process (default: current directory)
#
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# @var extensions
# @brief Array of default file extensions to process.
# -----------------------------------------------------------------------------
declare -g -a extensions=(c cpp py h hpp t tpp md txt ini sh py)

# -----------------------------------------------------------------------------
# @var exclude_args
# @brief Array of exclusion arguments for find.
# -----------------------------------------------------------------------------
declare -g -a exclude_args=()

# -----------------------------------------------------------------------------
# @var success_count
# @brief Counter for successfully processed files.
# -----------------------------------------------------------------------------
declare -g success_count=0

# -----------------------------------------------------------------------------
# @var failure_count
# @brief Counter for failed file processing attempts.
# -----------------------------------------------------------------------------
declare -g failure_count=0

# -----------------------------------------------------------------------------
# @brief Logs messages with color-coded severity levels.
# @details Prints messages to stdout/stderr based on log severity.
#          DEBUG messages appear only if DEBUG_MODE is enabled.
#
# @param $1 Log level (INFO, WARNING, ERROR, DEBUG)
# @param $2 Log message text.
#
# @global DEBUG_MODE Used to control debug message output.
#
# @return None
#
# @example
# log "INFO" "Process started."
# log "ERROR" "File not found."
# -----------------------------------------------------------------------------
log() {
    local level="$1"
    local message="$2"
    local timestamp
    timestamp=$(date +"%Y-%m-%d %H:%M:%S")

    local green yellow red cyan reset
    if [[ -t 1 ]]; then
        green=$(tput setaf 2 2>/dev/null || printf "")
        yellow=$(tput setaf 3 2>/dev/null || printf "")
        red=$(tput setaf 1 2>/dev/null || printf "")
        cyan=$(tput setaf 6 2>/dev/null || printf "")
        reset=$(tput sgr0 2>/dev/null || printf "")
    else
        green="" yellow="" red="" cyan="" reset=""
    fi

    case "${level^^}" in
        INFO) printf "%s%s [%s]%s %s\n" "$green" "$timestamp" "INFO " "$reset" "$message" ;;
        WARNING) printf "%s%s [%s]%s %s\n" "$yellow" "$timestamp" "WARN " "$reset" "$message" >&2 ;;
        ERROR) printf "%s%s [%s]%s %s\n" "$red" "$timestamp" "ERROR" "$reset" "$message" >&2 ;;
        DEBUG)
            if [[ "${DEBUG_MODE:-false}" == "true" ]]; then
                printf "%s%s [%s]%s %s\n" "$cyan" "$timestamp" "DEBUG" "$reset" "$message"
            fi
            ;;
        *) printf "%s [%s] %s\n" "$timestamp" "UNKNW" "$message" ;;
    esac
}

# -----------------------------------------------------------------------------
# @brief Process a single file to remove trailing whitespace.
#
# @param $1 File to process.
# @param $2 Boolean flag for dry-run mode.
#
# @global success_count Number of successfully processed files.
# @global failure_count Number of failed processed files.
#
# @return None
# -----------------------------------------------------------------------------
process_file() {
    local file="$1"
    local dry_run_mode="${2:-false}"

    if [[ ! -f "$file" ]]; then
        log "ERROR" "File not found: $file"
        ((failure_count++))
        return 1
    fi

    # Check if the file is empty
    if [[ ! -s "$file" ]]; then
        log "WARNING" "Skipping empty file: $file"
        return 0
    fi

    # Detect and skip binary files, but allow ASCII text
    if file -i "$file" | grep -q 'charset=binary'; then
        log "WARNING" "Skipping binary file: $file"
        return 0
    fi

    if [[ "$dry_run_mode" == true ]]; then
        log "INFO" "Dry-run mode: Would process $file"
        ((success_count++))
        return 0
    fi

    # Create a backup
    cp "$file" "$file.bak" || log "WARNING" "Failed to create backup for $file"

    if ! sed -i -E 's/[[:space:]]*$//' "$file" 2>/dev/null; then
        log "ERROR" "Failed to process file: $file"
        ((failure_count++))
        return 1
    fi

    ((success_count++))
    log "INFO" "SUCCESS: Processed $file"
}

# -----------------------------------------------------------------------------
# @brief Process files in a directory, optionally recursively.
#
# @param $1 Directory to process.
# @param $2 Boolean: true for recursive processing, false otherwise.
# @param $3 Boolean: true for dry-run mode, false otherwise.
#
# @global success_count Number of successfully processed files.
# @global failure_count Number of failed processed files.
#
# @return None
# -----------------------------------------------------------------------------
process_files() {
    local dir="$1"
    local recursive="$2"
    local dry_run_mode="$3"

    if [[ ! -d "$dir" ]]; then
        log "ERROR" "Directory not found: $dir"
        return 1
    fi

    # ✅ Build find filters for file extensions
    local find_filters=()
    for ext in "${extensions[@]}"; do
        find_filters+=(-iname "*.${ext}" -o)
    done
    unset 'find_filters[-1]'  # ✅ Remove trailing `-o`

    # ✅ Handle exclusions, including default hidden directories
    local exclude_filters=(-not -path "*/.*/*")  # Default: exclude hidden directories
    for exclude in "${exclude_args[@]}"; do
        # If the user explicitly included a hidden dir, remove the default exclusion
        if [[ "$exclude" == .* ]]; then
            exclude_filters=()
        fi
        exclude_filters+=(-not -path "$exclude/*")
    done

    # ✅ Construct find command
    local find_cmd=()
    if [[ "$recursive" == true ]]; then
        find_cmd=(find "$dir" -type f "${exclude_filters[@]}" \( "${find_filters[@]}" \))
    else
        find_cmd=(find "$dir" -maxdepth 1 -type f "${exclude_filters[@]}" \( "${find_filters[@]}" \))
    fi

    log "DEBUG" "Running: ${find_cmd[*]}" || true

    # ✅ Capture find output into an array to avoid failures from `set -e`
    mapfile -t files_found < <("${find_cmd[@]}" 2>/dev/null || true)

    # ✅ Ensure find didn't fail silently
    if [[ ${#files_found[@]} -eq 0 ]]; then
        log "WARNING" "No matching files found in $dir."
        return 0
    fi

    # ✅ Process each found file
    for file in "${files_found[@]}"; do
        log "DEBUG" "Found: $file"
        process_file "$file" "$dry_run_mode"
    done

    # ✅ Print summary results
    printf "\n%sSUCCESS:%s\t%d files processed correctly.\n" "$(tput setaf 2 2>/dev/null || printf '')" "$(tput sgr0 2>/dev/null || printf '')" "$success_count"
    printf "%sFAILURES:%s\t%d files could not be processed.\n\n" "$(tput setaf 1 2>/dev/null || printf '')" "$(tput sgr0 2>/dev/null || printf '')" "$failure_count"
}

# -----------------------------------------------------------------------------
# @brief Validate custom extensions format.
#
# @param $1 String containing comma-separated extensions.
#
# @return 0 if valid, 1 otherwise.
# -----------------------------------------------------------------------------
validate_extensions() {
    local input="$1"
    [[ "$input" =~ ^[a-zA-Z0-9,]+$ ]]
}

# -----------------------------------------------------------------------------
# @brief Recursively find and delete *.bak files, with user confirmation.
#
# @param $1 Directory to search.
# @param $2 Boolean: true for recursive search, false for current directory only.
# @return None
# -----------------------------------------------------------------------------
delete_backup_files() {
    local dir="$1"
    local recursive="$2"

    # Define the find command based on recursive flag
    local find_cmd
    if [[ "$recursive" == true ]]; then
        find_cmd=(find "$dir" -type f -name "*.bak")
    else
        find_cmd=(find "$dir" -maxdepth 1 -type f -name "*.bak")
    fi

    # Capture found files
    mapfile -t bak_files < <("${find_cmd[@]}" 2>/dev/null || true)

    # No .bak files found
    if [[ ${#bak_files[@]} -eq 0 ]]; then
        log "INFO" "No .bak files found to delete."
        return 0
    fi

    # List files to be deleted
    log "INFO" "The following .bak files were found:"
    for file in "${bak_files[@]}"; do
        log "INFO" "  $file"
    done

    # Prompt user for deletion confirmation
    printf "\nDo you want to delete these files? (y/N): "
    read -r confirm
    if [[ "$confirm" =~ ^[Yy]$ ]]; then
        for file in "${bak_files[@]}"; do
            rm -f "$file"
            log "INFO" "Deleted: $file"
        done
        log "INFO" "All .bak files have been deleted."
    else
        log "INFO" "Deletion cancelled. No files were removed."
    fi
}

# -----------------------------------------------------------------------------
# @brief Display usage instructions.
#
# @return None (Exits with status 1)
# -----------------------------------------------------------------------------
usage() {
    local green=""
    local reset=""

    if [[ -t 1 ]]; then
        green=$(tput setaf 2)
        reset=$(tput sgr0)
    fi

    printf "%sUsage:%s %s [-r] [-d] [-v] [-b] [-x exclude_dir] [-e extensions] [directory]\n\n" \
        "$green" "$reset" "$0"

    printf "Options:\n"
    printf "  %-14s %s\n" "-r" "Process files recursively"
    printf "  %-14s %s\n" "-d" "Dry-run mode (no changes made)"
    printf "  %-14s %s\n" "-v" "Verbose mode (enable DEBUG_MODE)"
    printf "  %-14s %s\n" "-b" "Keep backup files (.bak), otherwise remove"
    printf "  %-14s %s\n" "-x <dir>" "Exclude specified directory"
    printf "  %-14s %s\n" "-e <ext>" "Comma-separated list of file extensions to process"

    printf "\nArguments:\n"
    printf "  %-14s %s\n" "directory" "Directory to process (default: current directory)"

    printf "\nExamples:\n"
    printf "  %s %s\n" "$0" "-r                    # Process files recursively"
    printf "  %s %s\n" "$0" "-d -x logs -e txt,md  # Dry-run, exclude 'logs', only process txt/md files"
    printf "  %s %s\n" "$0" "-v                    # Enable verbose debug mode"
    printf "  %s %s\n" "$0" "-b                    # Keep backup files (.bak)"

    printf "\n"
    return 1
}

# -----------------------------------------------------------------------------
# @brief Main function orchestrating the script execution.
#
# @return None
# -----------------------------------------------------------------------------
main() {
    local recursive=false
    local dry_run=false
    local keep_backup=false
    local directory="."
    local custom_extensions=""
    local -a excludes=()
    DEBUG_MODE=false

    if [[ $# -eq 0 ]]; then
        log "ERROR" "No arguments provided. Use -h for help."
        return 1
    fi

    while getopts ":rdvbx:e:h" opt; do
        case "$opt" in
            r) recursive=true ;;
            d) dry_run=true ;;
            v) DEBUG_MODE=true ;;
            b) keep_backup=true ;;
            x) excludes+=("$OPTARG") ;;
            e) custom_extensions="$OPTARG" ;;
            h) usage; return 0 ;;
            *) log "ERROR" "Invalid option: -$OPTARG"; usage; return 1 ;;
        esac
    done
    shift $((OPTIND - 1))

    if [[ $# -ge 1 ]]; then
        directory="$1"
        if [[ ! -d "$directory" ]]; then
            log "ERROR" "Invalid directory: $directory"
            return 1
        fi
    fi

    if [[ -n "$custom_extensions" ]]; then
        if ! validate_extensions "$custom_extensions"; then
            log "ERROR" "Invalid extensions format."
            return 1
        fi
        IFS=',' read -r -a extensions <<< "$custom_extensions"
    fi

    exclude_args=()
    declare -A seen_excludes

    for exclude in "${excludes[@]}"; do
        local exclude_cleaned="${exclude%/}"
        local exclude_path
        exclude_path="$(realpath -m "$directory/$exclude_cleaned" 2>/dev/null || printf "")"

        if [[ -z "$exclude_path" ]]; then
            log "WARNING" "Failed to resolve path for exclusion: $exclude_cleaned"
            continue
        fi

        if [[ -z "${seen_excludes[$exclude_cleaned]:-}" ]]; then
            if [[ -d "$exclude_path" ]]; then
                exclude_args+=(-not -path "$exclude_path/*")
            else
                log "WARNING" "Exclusion '$exclude_path' does not exist or is not a directory."
            fi
            seen_excludes["$exclude_cleaned"]=1
        fi
    done

    success_count=0
    failure_count=0

    # Process whitespace cleanup
    if ! process_files "$directory" "$recursive" "$dry_run" "$keep_backup"; then
        log "ERROR" "File processing encountered errors."
        return 1
    fi

    # If -b is NOT set, prompt to delete .bak files
    if [[ "$keep_backup" == false ]]; then
        delete_backup_files "$directory" "$recursive"
    fi

    log "INFO" "Summary: $success_count files processed successfully, $failure_count failures."
    return 0
}

main "$@"
retval="$?"
if [[ $retval -ne 0 ]]; then
    printf "Failed to clean whitespace.\n" >&2
    exit "$retval"
fi

exit 0
