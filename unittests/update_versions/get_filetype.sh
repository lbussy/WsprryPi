#!/bin/bash

# ANSI escape codes for formatting
GREEN="\033[32m"
GOLD="\033[33m"
BOLD="\033[1m"
RESET="\033[0m"

# Function: Check for known or unknown shebang
check_shebang() {
  local FILE="$1"
  local FIRST_LINE=$(head -n 1 "$FILE" 2>/dev/null)

  if [[ "$FIRST_LINE" == "#!"* ]]; then
    local SHEBANG_PATH="${FIRST_LINE:2}"
    if [[ "$SHEBANG_PATH" =~ ^/usr/bin/env[[:space:]]+ ]]; then
      local INTERPRETER=${SHEBANG_PATH#* }
      ENV_PREFIX="(via env)"
    else
      local INTERPRETER=$(basename "${SHEBANG_PATH%% *}")
      ENV_PREFIX=""
    fi

    case "$INTERPRETER" in
      bash) echo "Bash script $ENV_PREFIX";;
      sh) echo "POSIX shell script $ENV_PREFIX";;
      zsh) echo "Zsh script $ENV_PREFIX";;
      ksh) echo "Korn shell script $ENV_PREFIX";;
      python) echo "Python script $ENV_PREFIX";;
      python2) echo "Python 2 script $ENV_PREFIX";;
      python3) echo "Python 3 script $ENV_PREFIX";;
      perl) echo "Perl script $ENV_PREFIX";;
      ruby) echo "Ruby script $ENV_PREFIX";;
      node) echo "Node.js script $ENV_PREFIX";;
      php) echo "PHP script $ENV_PREFIX";;
      groovy) echo "Groovy script $ENV_PREFIX";;
      Rscript) echo "R script $ENV_PREFIX";;
      lua) echo "Lua script $ENV_PREFIX";;
      tclsh) echo "Tcl script $ENV_PREFIX";;
      awk) echo "Awk script $ENV_PREFIX";;
      sed) echo "Sed script $ENV_PREFIX";;
      *) echo "Unknown script ($INTERPRETER) $ENV_PREFIX";;
    esac
  else
    echo "Unknown"
  fi
}

# Function: Check for configuration patterns
check_ini_or_conf() {
  local FILE="$1"
  if grep -qE '^\[[^]]+\]$|^[^#;]+=[^#;]+$' "$FILE" 2>/dev/null; then
    echo "Configuration file"
  else
    echo "Unknown"
  fi
}

# Function: Determine if a file is empty
is_empty_file() {
  local FILE="$1"
  local FILE_SIZE=$(stat -c%s "$FILE" 2>/dev/null)
  local NON_EMPTY_LINES=$(grep -cve '^\s*$' "$FILE" 2>/dev/null)
  [[ $FILE_SIZE -eq 0 || $NON_EMPTY_LINES -eq 0 || ($NON_EMPTY_LINES -eq 1 && $(head -n 1 "$FILE") =~ ^#!) ]]
}

# Function: Determine file type
determine_file_type() {
  local FILE="$1"
  local FILEINFO=$(file -b "$FILE")
  local FIRST_LINE=$(head -n 1 "$FILE" 2>/dev/null)
  local BASENAME=$(basename "$FILE")
  local FILE_SIZE=$(stat -c%s "$FILE" 2>/dev/null)
  local TYPE=""
  local EMPTY=""

  # Check if the file is empty
  if is_empty_file "$FILE"; then
    EMPTY="(empty)"
  fi

  # Special case for files named "conf"
  if [[ "$BASENAME" == "conf" ]]; then
    TYPE="Configuration file"
  elif [[ "$FIRST_LINE" =~ ^#! ]]; then
    TYPE=$(check_shebang "$FILE")
  elif grep -qE '^\[[^]]+\]$|^[^#;]+=[^#;]+$' "$FILE" 2>/dev/null; then
    TYPE="Configuration file"
  else
    # Classify based on file extension first
    case "$FILE" in
      *.py) TYPE="Python script (no shebang)";;
      *.sh) TYPE="Bash script (no shebang)";;
      *.conf|*.ini) TYPE="Configuration file";;
      *.txt|*.text) TYPE="ASCII text file";;
      *)
        # Fallback: If file output starts with "ASCII text", classify as "Unknown ASCII file"
        if [[ "$FILEINFO" =~ ^ASCII\ text ]]; then
          TYPE="Unknown ASCII file"
        else
          TYPE="Unknown"
        fi
        ;;
    esac
  fi

  # Append (empty) to all classified file types if the file is empty
  echo "$TYPE $EMPTY [$FILE_SIZE bytes]" | sed 's/  / /g' | awk '{$1=$1; print}'
}

# Function: Process a single file
process_file() {
  local FILE="$1"
  local FILETYPE=$(determine_file_type "$FILE")
  echo -e "${GOLD}$FILE${RESET}: ${GREEN}$FILETYPE${RESET}"
}

# Function: Process files or directories
# Function: Process files or directories
process_files() {
  local TARGET="$1"
  
  # Print the header for the pattern or directory
  echo -e "${BOLD}Processing file(s):${RESET}"

  # If the target is a directory
  if [ -d "$TARGET" ]; then
    TARGET="${TARGET%/}"  # Remove trailing slash
    for FILE in "$TARGET"/*; do
      [ -f "$FILE" ] && process_file "$FILE"
    done
  
  # If the target is a file or matches a pattern
  else
    shopt -s nullglob
    local MATCHED_FILES=($TARGET)  # Expand the pattern
    if [[ ${#MATCHED_FILES[@]} -eq 0 ]]; then
      echo "No files matched pattern: $TARGET"
      return
    fi

    # Process each matched file
    for FILE in "${MATCHED_FILES[@]}"; do
      [ -f "$FILE" ] && process_file "$FILE" || echo -e "${GOLD}$FILE${RESET}: Not a valid file"
    done
  fi
}

#!/bin/bash

# Main function
main() {
  # Check if arguments are provided
  if [[ $# -eq 0 ]]; then
    echo "Usage: $0 <files, directories, or patterns>"
    exit 1
  fi

  # Pass all arguments as a single combined string without expanding them
  TARGET="$*"
  process_files "$TARGET"
}

# Only call main if the script is executed directly (not sourced)
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
fi
