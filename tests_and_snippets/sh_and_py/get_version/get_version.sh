#!/bin/bash

# Function to check if the script is running on Raspbian OS
check_raspbian() {
  # Load OS information and verify it is Raspbian
  if [[ -f "/etc/os-release" ]]; then
    . /etc/os-release
    if [[ "$ID" != "raspbian" ]]; then
      echo "Error: This script is designed to run only on Raspbian OS. Detected OS: $PRETTY_NAME" >&2
      exit 1
    fi
  else
    echo "Error: Unable to detect the operating system. This script can only run on Raspbian OS." >&2
    exit 1
  fi
}

# Function to check if required dependencies are installed
check_dependencies() {
  local dependencies=("grep" "sed" "file" "strings" "git")
  local missing=()

  # Verify each dependency is installed
  for cmd in "${dependencies[@]}"; do
    if ! command -v "$cmd" &>/dev/null; then
      missing+=("$cmd")
    fi
  done

  # Exit with an error if any dependencies are missing
  if [[ ${#missing[@]} -gt 0 ]]; then
    echo "Error: Missing required commands: ${missing[*]}" >&2
    exit 1
  fi
}

# Function to display the help message
print_help() {
  echo "Usage: $0 [-s <string>] <file>"
  echo
  echo "This script scans files for semantic version information."
  echo
  echo "Options:"
  echo "  -s <string>   Specify a custom string to scan for version information."
  echo "  <file>        Path to the file to scan for version information."
  echo
  echo "Supported File Types:"
  echo "  - Bash scripts       (*.sh): Scans comments for version patterns."
  echo "  - Python scripts     (*.py): Scans comments and docstrings."
  echo "  - Systemd unit files (*.service): Checks for 'Version=' or comments."
  echo "  - Config files       (*.conf): Scans for 'Version=' or comments."
  echo "  - Executables        (ELF): Uses '--version' or embedded version strings."
  echo
  echo "Return:"
  echo "  - Semantic version (e.g., 1.2.3) if found."
  echo "  - 'unknown' if no version is detected."
  echo
  echo "Examples:"
  echo "  $0 script.sh"
  echo "  $0 app.py"
  echo "  $0 systemd_unit.service"
  echo "  $0 -s \"# Version 1.2.3\" config.conf"
  exit 0
}

# Function to parse command-line arguments
parse_args() {
  local OPTIND
  while getopts ":s:h" opt; do
    case "$opt" in
      s) string="$OPTARG" ;;  # Capture custom string if provided
      h) print_help ;;        # Display help if -h is passed
      *) echo "Invalid option: -$OPTARG" >&2; print_help ;;  # Handle invalid options
    esac
  done
  shift $((OPTIND - 1))

  # Ensure a file argument is provided
  [[ -z "$1" ]] && { echo "Error: No file specified." >&2; print_help; }
  FILE="$1"
}

# Function to extract semantic version from a string
extract_semantic_version() {
  local line="$1"
  echo "$line" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?(\+[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?' || echo "unknown"
}

# Function to retrieve the Git project name
get_project_name() {
  local url
  url=$(git config --get remote.origin.url)
  # Extract the repository name from the URL
  [[ -n "$url" ]] && echo "${url##*/}" | sed 's/.git$//' || echo "Error: Not inside a Git repository." && exit 1
}

# Function to scan Python script files for version information
scan_python_script_for_version() {
  local file="$1"
  local version_line
  version_line=$(grep -Po '(^|\s)#\s*Created for.*' "$file" || echo "unknown")
  if [[ "$version_line" == "unknown" ]]; then
    version_line=$(sed -n "/\"\"\"/,/\"\"\"/p" "$file" | grep -Po 'Created for.*' || echo "unknown")
  fi
  echo "$version_line"
}

# Function to scan Bash script files for version information
scan_bash_script_for_version() {
  local file="$1"
  local version_line
  version_line=$(grep -Po '#\s*Created for.*' "$file" || echo "unknown")

  if [[ "$version_line" == "unknown" ]]; then
    version_line=$(sed -n "/^: '/,/'$/p" "$file" | grep -Po 'Created for.*' || echo "unknown")
  fi
  echo "$version_line"
}

# Function to scan systemd unit files for version information
scan_systemd_unit_for_version() {
  local file="$1"
  local version_line
  version_line=$(grep -Po '#\s*Created for.*' "$file")

  if [[ -n "$version_line" ]]; then
    echo "$version_line"
  else
    version_line=$(grep -Po 'Version=.*' "$file")
    echo "${version_line:-unknown}"
  fi
}

# Function to scan config files for version information
scan_config_file_for_version() {
  local file="$1"
  local version_line
  version_line=$(grep -Po '#\s*Created for.*' "$file" || echo "unknown")
  echo "$version_line"
}

# Function to extract version from executable files
scan_executable_for_version() {
  local file="$1"
  local version_line
  version_line=$("$file" --version 2>&1 || echo "unknown")

  if [[ "$version_line" == *"version"* ]]; then
    echo "$version_line"
    return 0
  fi

  version_line=$(strings "$file" | grep -i -E 'version|v[0-9]+(\.[0-9]+){2,}' || echo "unknown")
  echo "$version_line"
}

# Function to detect the file type
detect_file_type() {
  local file="$1"
  local file_info
  file_info=$(file "$file")

  if [[ "$file" =~ \.service$ ]]; then
    echo "systemd_unit"
  elif [[ "$file_info" == *"Python script"* ]]; then
    echo "python_script"
  elif [[ "$file_info" == *"Bash script"* || $(head -n 1 "$file" | tr -d '\0') == "#!"* ]]; then
    echo "bash_script"
  elif [[ "$file" =~ \.conf$ ]]; then
    echo "config"
  elif [[ "$file_info" == *"executable"* && "$file_info" != *"script"* ]]; then
    echo "executable"
  else
    echo "unknown"
  fi
}

# Main function
get_version() {
  check_dependencies || exit 1
  check_raspbian || exit 1
  parse_args "$@"

  if [[ -n "$1" && "$1" != "--help" && "$1" != "-h" ]]; then
    file="$1"
    file_type=$(detect_file_type "$file")
    if [[ "$file_type" == "unknown" ]]; then
      echo "unknown"
      exit 1
    fi

    case "$file_type" in
      "bash_script") version_string=$(scan_bash_script_for_version "$file") ;;
      "python_script") version_string=$(scan_python_script_for_version "$file") ;;
      "systemd_unit") version_string=$(scan_systemd_unit_for_version "$file") ;;
      "config") version_string=$(scan_config_file_for_version "$file") ;;
      "executable") version_string=$(scan_executable_for_version "$file") ;;
      *) echo "unknown"; exit 1 ;;
    esac

    extracted_version=$(extract_semantic_version "$version_string")
    echo "$extracted_version"
  else
    print_help
  fi
}

# Run the script only if it's not sourced
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  get_version "$@"
fi
