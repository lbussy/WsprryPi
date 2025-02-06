#!/usr/bin/env python3

import os
import sys
import re
from pathlib import Path

# ANSI escape codes for formatting
GREEN = "\033[32m"
GOLD = "\033[33m"
BOLD = "\033[1m"
RESET = "\033[0m"

def normalize_path(file_path):
    """Normalize file paths for consistent processing."""
    return str(Path(file_path).resolve())

def check_shebang(file_path):
    """Check for known or unknown shebang."""
    try:
        with open(file_path, "r") as file:
            first_line = file.readline().strip()
    except Exception:
        return "Unknown"

    if first_line.startswith("#!"):
        shebang_path = first_line[2:].strip()
        if shebang_path.startswith("/usr/bin/env"):
            interpreter = shebang_path.split()[-1]
            env_prefix = "(via env)"
        else:
            interpreter = os.path.basename(shebang_path.split()[0])
            env_prefix = ""

        return {
            "bash": f"Bash script {env_prefix}",
            "sh": f"POSIX shell script {env_prefix}",
            "zsh": f"Zsh script {env_prefix}",
            "ksh": f"Korn shell script {env_prefix}",
            "python": f"Python script {env_prefix}",
            "python2": f"Python 2 script {env_prefix}",
            "python3": f"Python 3 script {env_prefix}",
            "perl": f"Perl script {env_prefix}",
            "ruby": f"Ruby script {env_prefix}",
            "node": f"Node.js script {env_prefix}",
            "php": f"PHP script {env_prefix}",
            "groovy": f"Groovy script {env_prefix}",
            "Rscript": f"R script {env_prefix}",
            "lua": f"Lua script {env_prefix}",
            "tclsh": f"Tcl script {env_prefix}",
            "awk": f"Awk script {env_prefix}",
            "sed": f"Sed script {env_prefix}",
        }.get(interpreter, f"Unknown script ({interpreter}) {env_prefix}")

    return "Unknown"

def is_empty_file(file_path):
    """Check if a file is empty based on size, content, and shebang rules."""
    try:
        file_size = os.path.getsize(file_path)
        if file_size == 0:
            return True  # Case 1: File size is 0 bytes

        with open(file_path, "r") as file:
            content = file.read().strip()

            # Case 2: File contains only whitespace
            if not content:
                return True

            # Case 3: File contains only a shebang with or without trailing whitespace
            if content.startswith("#!") and len(content.splitlines()) == 1:
                return True

    except Exception:
        pass  # If the file cannot be read, assume it is not empty

    return False

def determine_file_type(file_path):
    """Determine file type based on content and extension."""
    basename = os.path.basename(file_path)
    file_size = os.path.getsize(file_path)
    empty = "empty" if is_empty_file(file_path) else ""  # Use is_empty_file

    # Special case for files named "conf"
    if basename == "conf":
        return re.sub(r'\s+', ' ', f"Configuration file{f' ({empty})' if empty else ''} [{file_size} bytes]").strip()

    # Check if file has a shebang
    shebang_type = check_shebang(file_path)
    if shebang_type != "Unknown":
        return re.sub(r'\s+', ' ', f"{shebang_type}{f' ({empty})' if empty else ''} [{file_size} bytes]").strip()

    # Check for configuration file patterns in content
    try:
        with open(file_path, "r") as file:
            content = file.read()
            if re.search(r"^\[[^\]]+\]$", content, re.MULTILINE) or re.search(r"^[^#;]+=[^#;]+$", content, re.MULTILINE):
                return re.sub(r'\s+', ' ', f"Configuration file{f' ({empty})' if empty else ''} [{file_size} bytes]").strip()
    except Exception:
        pass  # If the file cannot be read, skip this check

    # Classify based on file extension
    extension_mapping = {
        ".py": "Python script (no shebang)",
        ".sh": "Bash script (no shebang)",
        ".conf": "Configuration file",
        ".ini": "Configuration file",
        ".txt": "ASCII text file",
        ".text": "ASCII text file",
    }
    extension = Path(file_path).suffix

    # Detect "ASCII" content for unknown files
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as file:
            content = file.read()
            if re.match(r"^[\x00-\x7F]*$", content):  # ASCII-only content
                if extension == "":
                    return re.sub(r'\s+', ' ', f"Unknown ASCII file{f' ({empty})' if empty else ''} [{file_size} bytes]").strip()
    except Exception:
        pass

    # Default to Unknown if no match
    return re.sub(r'\s+', ' ', f"{extension_mapping.get(extension, 'Unknown')}{f' ({empty})' if empty else ''} [{file_size} bytes]").strip()

def process_files(target):
    """Process files or directories."""
    target_path = Path(target).resolve()
    if not target_path.exists():
        print(f"{GOLD}{target}{RESET}: Not found.")
        return []

    if target_path.is_dir():
        # Process all files in a directory, sorted alphabetically
        return [
            (file.name, determine_file_type(file))
            for file in sorted(target_path.iterdir())
            if file.is_file()
        ]

    if target_path.is_file():
        # Process a single file
        return [(target_path.name, determine_file_type(target_path))]

    return []

def main():
    """Main function."""
    if len(sys.argv) < 2:
        print("Usage: python3 get_filetype.py <files, directories, or patterns>")
        sys.exit(1)

    print(f"{BOLD}Processing file(s):{RESET}")

    # Normalize and process all targets
    targets = [normalize_path(arg) for arg in sys.argv[1:]]
    results = []
    for target in targets:
        results.extend(process_files(target))

    # Print results
    for filename, file_type in results:
        print(f"{GOLD}{filename}{RESET}: {GREEN}{file_type}{RESET}")

if __name__ == "__main__":
    main()
