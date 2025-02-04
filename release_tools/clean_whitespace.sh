#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# declare DEBUG_MODE=true

# -----------------------------------------------------------------------------
# @file clean_whitespace.sh
# @brief Brief description of what the script does.
# @details A more detailed explanation of the script’s purpose, including its
#          functionality, expected input/output, and any relevant dependencies.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.2.1-update_release_scripts+95.8843fc0-dirty
# @date 2025-02-03
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
declare -g -a extensions=(c cpp py h hpp md txt ini sh)

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
# @brief Log messages with specified severity levels.
#
# @param $1 Log level (INFO, WARNING, ERROR, DEBUG)
# @param $2 Log message
#
# @return None
# -----------------------------------------------------------------------------
log() {
    local level="$1"
    local message="$2"
    local timestamp
    timestamp=$(date +"%Y-%m-%d %H:%M:%S")

    local green yellow red cyan reset
    green=$(tput setaf 2 2>/dev/null || printf "")
    yellow=$(tput setaf 3 2>/dev/null || printf "")
    red=$(tput setaf 1 2>/dev/null || printf "")
    cyan=$(tput setaf 6 2>/dev/null || printf "")
    reset=$(tput sgr0 2>/dev/null || printf "")

    case "${level^^}" in
        INFO)
            level="INFO "
            printf "%s%s [%s]%s %s\n" "$green" "$timestamp" "$level" "$reset" "$message"
            ;;
        WARNING)
            level="WARN "
            printf "%s%s [%s]%s %s\n" "$yellow" "$timestamp" "$level" "$reset" "$message" >&2
            ;;
        ERROR)
            printf "%s%s [%s]%s %s\n" "$red" "$timestamp" "$level" "$reset" "$message" >&2
            ;;
        DEBUG)
            if [[ "${DEBUG_MODE:-false}" == "true" ]]; then
                printf "%s%s [%s]%s %s\n" "$cyan" "$timestamp" "$level" "$reset" "$message"
            fi
            ;;
        *)
            level="UNKNW"
            printf "%s [%s] %s\n" "$timestamp" "$level" "$message"
            ;;
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

    if [[ "$dry_run_mode" == true ]]; then
        log "INFO" "Dry-run mode: Would process $file"
        ((success_count++))  # ✅ Ensure count updates in dry-run mode
        return 0
    fi

    if ! sed -i -E 's/[[:space:]]*$//' "$file" 2>/dev/null; then
        log "ERROR" "Failed to process file: $file"
        ((failure_count++))
        return 1
    fi

    ((success_count++))  # ✅ Increment success count
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
        return 1  # Avoid script exit, allow logging and continuation
    fi

    # ✅ Build find filters for file extensions
    local find_filters=()
    for ext in "${extensions[@]}"; do
        find_filters+=(-iname "*.${ext}" -o)
    done
    unset 'find_filters[-1]'  # ✅ Remove trailing `-o`

    # ✅ Handle exclusions
    local exclude_filters=()
    for exclude in "${exclude_args[@]}"; do
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
        return 0  # Prevents breaking the script when no files are found
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
# @brief Display usage instructions.
#
# @return None (Exits with status 1)
# -----------------------------------------------------------------------------
usage() {
    local green=""
    local reset=""

    # Use colors only if output is a terminal
    if [[ -t 1 ]]; then
        green=$(tput setaf 2)  # Green for headers
        reset=$(tput sgr0)     # Reset color
    fi

    printf "%sUsage:%s %s [-r] [-n] [-x exclude_dir] [-e extensions] [directory]\n\n" \
        "$green" "$reset" "$0"

    printf "Options:\n"
    printf "  %-14s %s\n" "-r" "Process files recursively"
    printf "  %-14s %s\n" "-n" "Dry-run mode (no changes made)"
    printf "  %-14s %s\n" "-x <dir>" "Exclude specified directory"
    printf "  %-14s %s\n" "-e <ext>" "Comma-separated list of file extensions to process"

    printf "\nArguments:\n"
    printf "  %-14s %s\n" "directory" "Directory to process (default: current directory)"

    printf "\nExamples:\n"
    printf "  %s %s\n" "$0" "-r                    # Process files recursively"
    printf "  %s %s\n" "$0" "-n -x logs -e txt,md  # Dry-run, exclude 'logs', only process txt/md files"

    printf "\n"
    return 1  # Use 'return' instead of 'exit' inside functions
}

# -----------------------------------------------------------------------------
# @brief Main function orchestrating the script execution.
#
# @return None
# -----------------------------------------------------------------------------
main() {
    local recursive=false
    local dry_run=false
    local directory="."
    local custom_extensions=""
    local -a excludes=()

    # ✅ Ensure arguments are provided
    if [[ $# -eq 0 ]]; then
        log "ERROR" "No arguments provided. Use -h for help."
        return 1
    fi

    # ✅ Parse options safely
    while getopts ":rnx:e:h" opt; do
        case "$opt" in
            r) recursive=true ;;
            n) dry_run=true ;;
            x) excludes+=("$OPTARG") ;;
            e) custom_extensions="$OPTARG" ;;
            h)
                usage
                return 0
                ;;
            *)
                log "ERROR" "Invalid option: -$OPTARG"
                usage
                return 1
                ;;
        esac
    done
    shift $((OPTIND - 1))

    # ✅ Ensure a valid directory is provided or defaults to "."
    if [[ $# -ge 1 ]]; then
        directory="$1"
        if [[ ! -d "$directory" ]]; then
            log "ERROR" "Invalid directory: $directory"
            return 1
        fi
    fi

    # ✅ Validate custom extensions before processing
    if [[ -n "$custom_extensions" ]]; then
        if ! validate_extensions "$custom_extensions"; then
            log "ERROR" "Invalid extensions format."
            return 1
        fi
        IFS=',' read -r -a extensions <<< "$custom_extensions"
    fi

    # ✅ Initialize exclusion arguments safely
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

    # ✅ Reset counters before processing
    success_count=0
    failure_count=0

    # ✅ Call process_files with safety checks
    if ! process_files "$directory" "$recursive" "$dry_run"; then
        log "ERROR" "File processing encountered errors."
        return 1
    fi

    # ✅ Final summary log
    log "INFO" "Summary: $success_count files processed successfully, $failure_count failures."
    return 0
}

main "$@"
retval="$?"
if [[ $retval -ne 0 ]]; then
    printf "Failed to clean whitespace.\n" >&2
    exit "$retval"
fi

# If the main function succeeds, exit normally
exit 0
