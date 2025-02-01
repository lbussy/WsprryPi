#!/usr/bin/env bash

instructions() {
    local sp12 sp19
    sp12="$(printf ' %.0s' {1..12})"
    sp19="$(printf ' %.0s' {1..19})"
    # DEBUG TODO clear
    # Note:  $(printf ...) hack adds spaces at beg/end to support non-black BG
  cat << EOF

$DOT$BGBLK$FGYLW$sp12 __          __                          _____ _ $sp19
$DOT$BGBLK$FGYLW$sp12 \ \        / /                         |  __ (_)$sp19
$DOT$BGBLK$FGYLW$sp12  \ \  /\  / /__ _ __  _ __ _ __ _   _  | |__) | $sp19
$DOT$BGBLK$FGYLW$sp12   \ \/  \/ / __| '_ \| '__| '__| | | | |  ___/ |$sp19
$DOT$BGBLK$FGYLW$sp12    \  /\  /\__ \ |_) | |  | |  | |_| | | |   | |$sp19
$DOT$BGBLK$FGYLW$sp12     \/  \/ |___/ .__/|_|  |_|   \__, | |_|   |_|$sp19
$DOT$BGBLK$FGYLW$sp12                | |               __/ |          $sp19
$DOT$BGBLK$FGYLW$sp12                |_|              |___/           $sp19
$DOT$BGBLK$FGGRN$HHR$RESET
You will be presented with some choices during the install. Most frequently
you will see a 'yes or no' choice, with the default choice capitalized like
so: [y/N]. Default means if you hit <enter> without typing anything, you will
make the capitalized choice, i.e. hitting <enter> when you see [Y/n] will
default to 'yes.'

Yes/no choices are not case sensitive. However; passwords, system names and
install paths are. Be aware of this. There is generally no difference between
'y', 'yes', 'YES', 'Yes'.

EOF
    read -n 1 -s -r -p  "Press any key when you are ready to proceed. " < /dev/tty
    #DEBUG TODO clear
}

complete() {
    local sp7 sp11 sp18 sp28 sp49 rebootmessage
    if [ "$REBOOT" == "true" ]; then
        rebootmessage=$(echo -e "\nRemember to reboot: (sudo reboot).\n")
    else
        rebootmessage=""
    fi
    sp7="$(printf ' %.0s' {1..7})" sp11="$(printf ' %.0s' {1..11})"
    sp18="$(printf ' %.0s' {1..18})" sp28="$(printf ' %.0s' {1..28})"
    sp49="$(printf ' %.0s' {1..49})"
    # Note:  $(printf ...) hack adds spaces at beg/end to support non-black BG
    # DEBUG TODO clear
  cat << EOF

$DOT$BGBLK$FGYLW$sp7 ___         _        _ _    ___                _     _$sp18
$DOT$BGBLK$FGYLW$sp7|_ _|_ _  __| |_ __ _| | |  / __|___ _ __  _ __| |___| |_ ___ $sp11
$DOT$BGBLK$FGYLW$sp7 | || ' \(_-<  _/ _\` | | | | (__/ _ \ '  \| '_ \ / -_)  _/ -_)$sp11
$DOT$BGBLK$FGYLW$sp7|___|_|\_/__/\__\__,_|_|_|  \___\___/_|_|_| .__/_\___|\__\___|$sp11
$DOT$BGBLK$FGYLW$sp49|_|$sp28
$DOT$BGBLK$FGGRN$HHR$RESET

The WSPR daemon has started.
 - WSPR frontend URL   : http://$(hostname -I | awk '{print $1}')/wspr
                  -or- : http://$(hostname).local/wspr
 - Release version     : $VERSION
$rebootmessage
Happy DXing!
EOF
}

copy_file_generic() {
    local file_name extension local_script_path file_path remote_url
    local source_file target_file
    file_name="$1"
    extension=$(remove_dot "$2")
    local_script_path=$(remove_slash "$3")
    file_path=$(remove_slash "$4")
    remote_url=$(remove_slash "$5")

    target_file="$file_path/$file_name.$extension"

    if [ "$USE_LOCAL" == "true" ]; then
        # Use local file
        source_file="$local_script_path/$file_name.$extension"
        if [ ! -f "$source_file" ]; then
            echo "Local file $source_file not found." && die
        fi
        cp "$source_file" "$target_file" || die
    else
        # Download from GitHub
        source_file="$remote_url/$file_name.$extension"
        curl -s "$source_file" > "$target_file" || die
    fi
}

copy_file() {
    local file_name extension script_path file_path remote_url local_scripts_dir
    file_name="$1"
    extension=$(remove_dot "$2")
    script_path=$(remove_slash "$3")
    local_scripts_dir=$(remove_slash "$LOCAL_SCRIPTS_DIR")

    git_raw=$(remove_slash "$GIT_RAW")
    git_repo=$(remove_slash "$GIT_BRCH")
    
    file_path="$script_path/$file_name.$extension"
    remote_url="$git_raw/$git_repo/scripts"

    copy_file_generic "$file_name" "$extension" "$local_scripts_dir" "$file_path" "$remote_url"

    # Set permissions
    chown root:root "$file_path"
    if file "$file_path" | grep -q executable; then
        chmod 0755 "$file_path"
    else
        chmod 0644 "$file_path"
    fi
}

extract_semantic_version() {
    # Function to extract semantic version from a string
  local line="$1"
  echo "$line" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?(\+[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?' || echo "unknown"
}

strip_pre_release() {
    # Function to strip unknown pre-release info
  local input_version="$1"

  # Match the base version and allowed pre-release types
  if [[ "$input_version" =~ ^([0-9]+\.[0-9]+\.[0-9]+)(-(alpha|beta|rc[0-9]*|final))?$ ]]; then
    # If it matches, return the base version with allowed pre-release types
    echo "${BASH_REMATCH[1]}${BASH_REMATCH[2]}"
    return 0
  else
    # Otherwise, strip pre-release metadata and append "-development"
    echo "${input_version%%-*}-development"
    return 0
  fi
}

check_file() {
    local file_name script_path verchk yn file_path
    local installed_version stripped_installed_version
    local available_version stripped_available_version
    file_name="$1"
    extension=$(remove_dot "$2")
    file_name="$file_name.$extension"
    script_path=$(remove_slash "$3")
    file_path="$script_path/$file_name"

    if [ -f "$file_path" ]; then
        # Get semantic versions
        installed_version=$("$script_path" -v)
        installed_version=$(extract_semantic_version "$installed_version")
        stripped_installed_version=$(strip_pre_release "$installed_version")
        available_version="$VERSION"
        available_version=$(extract_semantic_version "$available_version")
        stripped_available_version=$(strip_pre_release "$available_version")

        # Compare versions
        if [[ -z "$stripped_installed_version" || -z "$stripped_available_version" ]]; then
            # One or both versions vailed to return a proper semantic version
            echo -e "Error: Failed to parse semantic version for:"
            [[ -z "$stripped_installed_version" ]] && echo "- Installed version: $installed_version"
            [[ -z "$stripped_available_version" ]] && echo "- Available version: $available_version"
            read -rp "Proceed with overwrite? [Y/n]: " yn < /dev/tty
            case "$yn" in
                [Nn]* ) return 255 ;;  # Skip Overwrite
                * ) return 0 ;;    # Overwrite (default to YEs)
            esac
        elif [[ "$stripped_installed_version" == *"development"* && "$stripped_available_version" == *"development"* ]]; then
            # Both of the versions shows as development
            echo -e "\nFile: $file_name exists and both it and the available file are a development version."
            read -rp "Overwrite anyway? [y/N]: " yn < /dev/tty
            case "$yn" in
                [Yy]* ) return 0 ;;  # Overwrite
                * ) return 255 ;;    # Skip overwrite (default to No)
            esac
        elif [[ "$stripped_installed_version" == *"development"* && "$stripped_available_version" != *"development"* ]]; then
                echo -e "\nFile: $file_name exists but is an older version ($installed_version vs. $available_version)." > /dev/tty
                read -rp "Upgrade to newest? [Y/n]: " yn < /dev/tty
                case "$yn" in
                    [Nn]* ) return 255 ;;  # Skip Overwwrite
                    * ) return 0 ;;        # Do overwrite (default to Yes)
                esac
        elif [[ "$stripped_installed_version" != *"development"* && "$stripped_available_version" == *"development"* ]]; then
            echo -e "\nFile: $file_name is newer than the version being installed ($installed_version vs. $available_version)." > /dev/tty
            return 255  # Skip update
        else
            if dpkg --compare-versions "$stripped_installed_version" lt "$stripped_available_version"; then
                echo -e "\nFile: $file_name exists but is an older version ($installed_version vs. $available_version)." > /dev/tty
                read -rp "Upgrade to newest? [Y/n]: " yn < /dev/tty
                case "$yn" in
                    [Nn]* ) return 255 ;;  # Skip update
                    * ) return 0 ;;        # Do overwrite (default to Yes)
                esac
            elif dpkg --compare-versions "$stripped_installed_version" gt "$stripped_available_version"; then
                echo -e "\nFile: $file_name is newer than the version being installed ($installed_version vs. $available_version)." > /dev/tty
                return 255 # Skip update
            else
                echo -e "\nFile: $file_name exists and is the same version as the available version ($installed_version)."
                read -rp "Overwrite anyway? [y/N]: " yn < /dev/tty
                case "$yn" in
                    [Yy]* ) return 0 ;;  # Overwrite
                    * ) return 255 ;;    # Skip overwrite (default to No)
                esac
            fi
        fi
    else
        echo "File: $file_name does not exist. Proceeding with installation." > /dev/tty
        return 0  # File does not exist, proceed with installation
    fi
}

############
### Main function
############

main() {
    arguments "$@" # Check command line arguments

    do_service "wspr" "" "/usr/local/bin" # Install/upgrade wspr daemon
    do_shutdown_button "shutdown_watch" "py" "/usr/local/bin" # Handle TAPR shutdown button
    do_www "/var/www/html/wspr" "$LOCAL_WWW_DIR" # Download website
    disable_sound

    echo -e "\n***Script $THISSCRIPT complete.***\n"
    complete
}
