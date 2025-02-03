#!/bin/bash
#
# This file is part of WsprryPi.
#
# Copyright (C) 2023-2025 Lee C. Bussy (@LBussy)
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

############
### Global Declarations
############

# shellcheck disable=SC2034  # Unused variables left for reusability

LOCAL_SOURCE_DIR=""
USE_LOCAL=false  # Default to not using local files

# General constants
declare VERBOSE REBOOT
# Color/character codes
declare BOLD SMSO RMSO FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST
declare BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST DOT HHR LHR RESET

# Set branch
readonly GIT_BRCH="version_files"
readonly VERSION="1.2.1-version-files+91.3bef855-dirty"
# Set this script
readonly THISSCRIPT="install.sh"
# Set Project
readonly COPYRIGHT="Copyright (C) 2023-2025 Lee C. Bussy (@LBussy)"
readonly PACKAGE="WsprryPi"
readonly PACKAGENAME="Wsprry Pi"
readonly OWNER="lbussy"
readonly APTPACKAGES="apache2 php jq libraspberrypi-dev raspberrypi-kernel-headers"
# This should not change
readonly GIT_RAW="https://raw.githubusercontent.com/$OWNER/$PACKAGE"
readonly GIT_API="https://api.github.com/repos/$OWNER/$PACKAGE"

############
### Bitness, Architecture & OS
############

check_bitness() {
    if [ "$(getconf LONG_BIT)" == "64" ]; then
        echo -e "\nRaspbian 64-bit is not currently supported.\n"
        exit 1
    fi
}

check_release() {
    ver=$(cat /etc/os-release | grep "VERSION_ID" | awk -F "=" '{print $2}' | tr -d '"')
    if [ "$ver" -lt 11 ]; then
        echo -e "\nRaspbian older than version 11 (bullseye) not supported.\n"
        exit 1
    fi
}

check_architecture() {
    # Get the Raspberry Pi model
    model=$(tr -d '\0' < /proc/device-tree/model)

    # Check if model contains "Raspberry Pi 4" or higher
    if [[ "$model" =~ "Raspberry Pi 5" ]]; then
        echo -e "\n$model is not currently supported\.n"
        exit 1
    fi
}

############
### Handle logging
############

timestamp() {
    local reply
    # Add date in '2019-02-26 08:19:22' format to log
    [[ "$VERBOSE" == "true" ]] && length=999 || length=60 # Allow full logging
    while read -rp; do
        # Clean and trim line to 60 characters to allow for timestamp on one line
        reply="$(clean "$reply" "$length")"
        # Strip blank lines
        if [ -n "$reply" ]; then
            # Add date in '2019-02-26 08:19:22' format to log
            printf '%(%Y-%m-%d %H:%M:%S)T %s\n' -1 "$reply"
        fi
    done
}

clean() {
    # Cleanup log line
    local input length dot
    input="$1"
    length="$2"
    # Even though this is defined in term() we need it earlier
    dot="$(tput sc)$(tput setaf 0)$(tput setab 0).$(tput sgr 0)$(tput rc)"
    # If we lead the line with our semaphore, return a blank line
    if [[ "$input" == "$dot"* ]]; then echo ""; return; fi
    # Strip color codes
    # shellcheck disable=SC2001  # Unused variables left for reusability
    input="$(echo "$input" | sed 's,\x1B[[(][0-9;]*[a-zA-Z],,g')"
    # Strip beginning spaces
    input="$(printf "%s" "${input#"${input%%[![:space:]]*}"}")"
    # Strip ending spaces
    input="$(printf "%s" "${input%"${input##*[![:space:]]}"}")"
    # Squash any repeated whitespace within string
    input="$(echo "$input" | awk '{$1=$1};1')"
    # Log only first $length chars to allow for date/time stamp
    input="$(echo "$input" | cut -c-"$length")"
    echo "$input"
}

log() {
    local thisscript scriptname shadow homepath
    [[ "$*" == *"-nolog"* ]] && return # Turn off logging
    # Set up our local variables
    local thisscript scriptname realuser homepath shadow
    # Explicit scriptname (creates log name) since we start
    # before the main script
    thisscript="$THISSCRIPT"
    scriptname="${thisscript%%.*}"
    # Get home directory for logging
    if [ -n "$SUDO_USER" ]; then realuser="$SUDO_USER"; else realuser=$(whoami); fi
    shadow="$( (getent passwd "$realuser") 2>&1)"
    if [ -n "$shadow" ]; then
        homepath=$(echo "$shadow" | cut -d':' -f6)
    else
        echo -e "\nERROR: Unable to retrieve $realuser's home directory. Manual install"
        echo -e "may be necessary."
        exit 1
    fi
    # Tee all output to log file in home directory
    sudo -u "$realuser" touch "$homepath/$scriptname.log"
    exec > >(tee >(timestamp >> "$homepath/$scriptname.log")) 2>&1
}

############
### Command line arguments
############

# usage outputs to stdout the --help usage message.
usage() {
cat << EOF

$PACKAGE $THISSCRIPT

Usage: sudo ./$THISSCRIPT"
EOF
}

# Parse arguments and call usage or version
arguments() {
    local arg
    while [[ "$#" -gt 0 ]]; do
        arg="$1"
        case "$arg" in
            -h|--help )
                usage; exit 0 ;;
            -v|--version )
                show_version; exit 0 ;;
            -l|--local )
                USE_LOCAL=true ;; # Enable local file usage
            * )
                echo "Unknown option: $arg"
                usage
                exit 1 ;;
        esac
        shift
    done
}


############
### Function to display the version
############

show_version() {
  echo "$THISSCRIPT: $VERSION"
}

############
### Make sure command is running with sudo
############

checkroot() {
    local retval shadow
    if [ -n "$SUDO_USER" ]; then REALUSER="$SUDO_USER"; else REALUSER=$(whoami); fi
    if [ "$REALUSER" == "root" ]; then
        # We're not gonna run as the root user
        echo -e "\nThis script may not be run from the root account, use 'sudo' instead."
        exit 1
    fi
    ### Check if we have root privs to run
    if [[ "$EUID" -ne 0 ]]; then
        echo -e "\nNot running as root, re-run using 'sudo'."
        exit 1
    fi
}

############
### Provide terminal escape codes
############

term() {
    local retval
    # If we are colors capable, allow them
    tput colors > /dev/null 2>&1
    retval="$?"
    if [ "$retval" == "0" ]; then
        BOLD=$(tput bold)   # Start bold text
        SMSO=$(tput smso)   # Start "standout" mode
        RMSO=$(tput rmso)   # End "standout" mode
        FGBLK=$(tput setaf 0)   # FG Black
        FGRED=$(tput setaf 1)   # FG Red
        FGGRN=$(tput setaf 2)   # FG Green
        FGYLW=$(tput setaf 3)   # FG Yellow
        FGBLU=$(tput setaf 4)   # FG Blue
        FGMAG=$(tput setaf 5)   # FG Magenta
        FGCYN=$(tput setaf 6)   # FG Cyan
        FGWHT=$(tput setaf 7)   # FG White
        FGRST=$(tput setaf 9)   # FG Reset to default color
        BGBLK=$(tput setab 0)   # BG Black
        BGRED=$(tput setab 1)   # BG Red
        BGGRN=$(tput setab 2)   # BG Green$(tput setaf $fg_color)
        BGYLW=$(tput setab 3)   # BG Yellow
        BGBLU=$(tput setab 4)   # BG Blue
        BGMAG=$(tput setab 5)   # BG Magenta
        BGCYN=$(tput setab 6)   # BG Cyan
        BGWHT=$(tput setab 7)   # BG White
        BGRST=$(tput setab 9)   # BG Reset to default color
        # Some constructs
        # "Invisible" period (black FG/BG and a backspace)
        DOT="$(tput sc)$(tput setaf 0)$(tput setab 0).$(tput sgr 0)$(tput rc)"
        HHR="$(eval printf %.0s═ '{1..'"${COLUMNS:-$(tput cols)}"\}; echo)"
        LHR="$(eval printf %.0s─ '{1..'"${COLUMNS:-$(tput cols)}"\}; echo)"
        RESET=$(tput sgr0)  # FG/BG reset to default color
    fi
}

############
### Functions to catch/display errors during execution
############

warn() {
    local fmt
    fmt="$1"
    command shift 2>/dev/null
    echo -e "$fmt"
    echo -e "${@}"
    echo -e "\n*** ERROR ERROR ERROR ERROR ERROR ***" > /dev/tty
    echo -e "-------------------------------------" > /dev/tty
    echo -e "\nSee above lines for error message." > /dev/tty
    echo -e "Setup NOT completed.\n" > /dev/tty
}

die() {
    local st
    st="$?"
    warn "$@"
    exit "$st"
}

############
### Adds dot to file extension if needed
############

add_dot() {
    local input="$1"
    if [[ "$input" != .* ]]; then
        input=".$input"
    fi
    echo "$input"
}

remove_dot() {
    local input="$1"
    if [[ "$input" == .* ]]; then
        input="${input#.}"
    fi
    echo "$input"
}

add_slash() {
    local input="$1"
    if [[ "$input" != */ ]]; then
        input="$input/"
    fi
    echo "$input"
}

remove_slash() {
  local input="$1"
  [[ "$input" == */ ]] && input="${input%/}"
  echo "$input"
}

############
### Instructions
############

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

############
### Set timezone
###########

settime() {
    local date tz yn
    date=$(date)
    while true; do
        echo -e "\n\nThe time is currently set to $date."
        tz="$(date +%Z)"
        if [ "$tz" == "GMT" ] || [ "$tz" == "BST" ]; then
            # Probably never been set
            read -rp "Is this correct? [y/N]: " yn  < /dev/tty
            case "$yn" in
                [Yy]* ) break ;;
                [Nn]* ) dpkg-reconfigure tzdata; break ;;
                * ) dpkg-reconfigure tzdata; break ;;
            esac
        else
            # Probably been set
            read -rp "Is this correct? [Y/n]: " yn  < /dev/tty
            case "$yn" in
                [Nn]* ) dpkg-reconfigure tzdata; break ;;
                [Yy]* ) break ;;
                * ) break ;;
            esac
        fi
    done
}

############
### Call the creation of unit files
### Required:
###   unit - Name of systemd unit
###   ext - Type of controlling script (e.g. "bash" or "python3")
############

do_service() {
    local file_name extension retval
    file_name="$1"
    extension=$(add_dot "$2")
    script_path=$(remove_slash "$3")

    # Handle script install
    check_file "$file_name" "$extension" "$script_path"
    retval="$?"
    if [[ "$retval" == 0 ]]; then
        systemctl stop "$file_name" &> /dev/null
        systemctl disable "$file_name" &> /dev/null
        copy_file "$file_name" "$extension" "$script_path"
        copy_file "$file_name" ".service" "/etc/systemd/system"
        if [[ "$file_name" == "wspr" ]]; then
            do_ini "wspr" "ini" "/usr/local/etc"
            copy_file "logrotate" "conf" "/etc/logrotate.d/"
        fi
        eval "sudo systemctl daemon-reload" &> /dev/null
        systemctl enable "$file_name" &> /dev/null
        eval "systemctl start $file_name" &> /dev/null
    fi
}

############
### Copy files (generic logic for local vs remote)
### Arguments:
###   $1 - source filename (without path)
###   $2 - local directory for local files
###   $3 - target full path
###   $4 - GitHub URL (for remote files)
############

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

############
### Copy daemon scripts
### Required:
###   file_name - Name of script to run under systemd
############

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

############
### Check existence and version of any current script files
### Required:
###     file_name - Name of script
###     extension - Script extension
### Returns:   0 to execute, 255 to skip
############

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
### Create wspr ini file
### Required:
###   none
############

do_ini() {
    local file_name extension path file_path remote_url yn
    file_name="$1"
    extension=$(remove_dot "$2")
    path=$(remove_slash "$3")
    file_path="$path/$file_name.$extension"

    if [ -f "$file_path" ]; then
        echo -e "\nYour configuration can be reset to stock. "
        read -rp "Overwrite? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]* )
                echo "Overwriting configuration file."
                ;;
            * )
                echo "Skipping configuration file setup."
                return
                ;;  # Skip this step
        esac
    else
        echo "Creating configuration file."
    fi
    copy_file "$file_name" "$extension" "$path" # Create ini file
}

############
### Install apt packages
############

apt_packages() {
    local last_update now_time pkg_ok upgrades_avail pkg

    echo -e "\nUpdating any expired apt keys."
    for K in $(apt-key list 2> /dev/null | grep expired | cut -d'/' -f2 | cut -d' ' -f1); do
	    sudo apt-key adv --recv-keys --keyserver keys.gnupg.net "$K";
    done

    echo -e "\nFixing any broken installations."
    sudo apt-get --fix-broken install -y||die
    # Run 'apt update' if last run was > 1 week ago
    last_update=$(stat -c %Y /var/lib/apt/lists)
    now_time=$(date +%s)
    if [ $((now_time - last_update)) -gt 604800 ]; then
        echo -e "\nLast apt update was over a week ago. Running apt update before updating"
        echo -e "dependencies."
        apt-get update -yq||die
    fi

    # Now install any necessary packages if they are not installed
    echo -e "\nChecking and installing required dependencies via apt."
    for pkg in $APTPACKAGES; do
        pkg_ok=$(dpkg-query -W --showformat='${Status}\n' "$pkg" | \
        grep "install ok installed")
        if [ -z "$pkg_ok" ]; then
            echo -e "\nInstalling '$pkg'."
            apt-get install "$pkg" -y -q=2||die
        fi
    done

    # Get list of installed packages with updates available
    upgrades_avail=$(dpkg --get-selections | xargs apt-cache policy {} | \
        grep -1 Installed | sed -r 's/(:|Installed: |Candidate: )//' | \
    uniq -u | tac | sed '/--/I,+1 d' | tac | sed '$d' | sed -n 1~2p)
    # Loop through the required packages and see if they need an upgrade
    for pkg in $APTPACKAGES; do
        if [[ "$upgrades_avail" == *"$pkg"* ]]; then
            echo -e "\nUpgrading '$pkg'."
            apt-get install "$pkg" -y -q=2||die
        fi
    done

    # Restart Apache just in case
    systemctl restart apache2
}

############
### Install website
############

do_www() {
    local www_dir local_dir directory git_raw git_repo api_url local_files file
    # Arguments
    www_dir=$(remove_slash "$1")
    local_dir=$(remove_slash "$2")
    # Configuration
    directory="data"
    git_raw=$(remove_slash "$GIT_RAW")
    git_raw="$git_raw/$BRANCH"
    git_repo=$(remove_slash "$GIT_BRCH")

    # Delete old files
    echo -e "\nDeleting old files."
    rm -f "$www_dir/"

    # Copy down web pages
    echo -e "\nChecking and installing website."
    if [ ! -d "$www_dir" ]; then
        mkdir "$www_dir"
    fi

    # Get the list of files
    api_url=$(remove_slash "$GIT_API")
    api_url="$api_url/contents/$directory?ref=$BRANCH"

    if [ "$USE_LOCAL" == "true" ]; then
        local_files=$("find $local_dir -type f -exec realpath {} \;")
        for file in $local_files; do
            echo "Copying $file..."
            cp "$file" "$www_dir/$file"
        done
    else
        github_files=$(curl -s "$api_url" | jq -r '.[] | select(.type == "file") | .name')
        for file in $github_files; do
            echo "Downloading $file..."
            curl "$git_raw/$directory/$file" -o "$www_dir/$file"
        done
    fi

    echo "Website copy complete."

    # Set the permissions
    echo -e "\nFixing file permissions for $www_dir."
    chown -R www-data:www-data "$www_dir" || die
    find "$www_dir" -type d -exec chmod 2770 {} \; || die
    find "$www_dir" -type f -exec chmod 660 {} \; || die
}

############
### TAPR Shutdown Button Support
############

do_shutdown_button() {
    local yn file_name extension script_path
    file_name="$1"
    extension=$(add_dot "$2")
    script_path=$(remove_slash "$3")
    echo
    # Check if the shutdown_watch service is already enabled
    if systemctl is-enabled shutdown_watch.service &>/dev/null; then
        echo "TAPR shutdown button support is already enabled."
        read -rp "Do you want to disable TAPR shutdown button support? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]* )
                echo "Disabling TAPR shutdown button support."
                systemctl stop shutdown_watch.service || true
                systemctl disable shutdown_watch.service || true
                rm -f /etc/systemd/system/shutdown_watch.service
                echo "Reloading systemd daemon configuration."
                systemctl daemon-reload
                return
                ;;
            * )
                echo "Keeping TAPR shutdown button support enabled."
                ;;
        esac
    else
        # Prompt user to enable support
        read -rp "Support system shutdown button (TAPR)? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]* )
                echo "Enabling TAPR shutdown button support."
                ;;
            * )
                echo "TAPR shutdown button support remains disabled."
                return
                ;;
        esac
    fi
    do_service "shutdown_watch" ".py" "/usr/local/bin" # Upgrade the service
}

############
### Disable sound
############

disable_sound() {
    local blacklist file retval
    blacklist="blacklist snd_bcm2835"
    file="/etc/modprobe.d/alsa-blacklist.conf"
    if grep -Fxq "$blacklist" "$file" 2>/dev/null; then
        REBOOT="false"
        return
    fi

    REBOOT="true"
    echo "$blacklist" > "$file"
    cat << EOF

*Important Note:*

Wsprry Pi uses the same hardware as the sound system to generate
radio frequencies. This soundcard has been disabled. You must
reboot the Pi with the following command after install for this
to take effect:

'sudo reboot'

EOF
    read -rp "Press any key to continue." < /dev/tty
    echo
}

############
### Print final banner
############

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

############
### Main function
############

main() {
    VERBOSE=true  # Do not trim logs
    local sysver
    check_bitness # Make sure we are not 64-bit
    check_release # Make sure we are not using some dusty old version
    check_architecture # Make sure we are not on a Pi 5
    command -v dpkg >/dev/null 2>&1 || { echo "dpkg not found. Exiting." >&2; return 1; }
    log "$@" # Start logging
    arguments "$@" # Check command line arguments

    if [ "$USE_LOCAL" == "true" ]; then
        # Determine the root directory of the current Git repository
        LOCAL_SOURCE_DIR="$(git rev-parse --show-toplevel 2>/dev/null)"
        if [ -z "$LOCAL_SOURCE_DIR" ]; then
            echo "Error: This script must be run from within a Git repository."
            exit 1
        fi
        LOCAL_WWW_DIR="$LOCAL_SOURCE_DIR/data" # Data directory under the Git repo root
        LOCAL_SCRIPTS_DIR="$LOCAL_SOURCE_DIR/scripts" # Scripts directory under the Git repo root
    fi

    echo -e "\n***Script $THISSCRIPT starting.***"
    sysver=$(grep 'PRETTY_NAME' /etc/os-release | cut -d '=' -f2 | tr -d '"')
    echo -e "\nRunning on: $sysver.\n"
    checkroot # Make sure we are su into root
    term # Add term command constants
    instructions # Show instructions
    settime # Set timezone
    apt_packages # Install any apt packages needed
    do_service "wspr" "" "/usr/local/bin" # Install/upgrade wspr daemon
    do_shutdown_button "shutdown_watch" "py" "/usr/local/bin" # Handle TAPR shutdown button
    do_www "/var/www/html/wspr" "$LOCAL_WWW_DIR" # Download website
    disable_sound
    echo -e "\n***Script $THISSCRIPT complete.***\n"
    complete
}

############
### Start the script
############

main "$@" && exit 0
