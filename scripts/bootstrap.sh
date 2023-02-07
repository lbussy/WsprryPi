#!/bin/bash

# Copyright (C) 2023 Lee C. Bussy (@LBussy)

############
### Global Declarations
############

# General constants
declare THISSCRIPT GITBRNCH GITPROJ PACKAGE VERBOSE OWNER COPYRIGHT
declare REPLY CMDLINE GITRAW PACKAGENAME VERSION
declare VERBOSE BRANCH
# Color/character codes
declare BOLD SMSO RMSO FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST
declare BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST DOT HHR LHR RESET

# Set branch
BRANCH="scripts"
VERSION="0.1"
# Set this script
THISSCRIPT="bootstrap.sh"
# Set Project
COPYRIGHT="Copyright (C) 2023 Lee C. Bussy (@LBussy)"
PACKAGE="WsprryPi"
PACKAGENAME="Wsprry Pi"
OWNER="lbussy"
# This should not change
if [ -z "$BRANCH" ]; then GITBRNCH="main"; else GITBRNCH="$BRANCH"; fi
GITRAW="https://raw.githubusercontent.com/$OWNER"

############
### Init
############

init() {
    # Set up some project variables we won't have running as a curled script
    CMDLINE="curl -L "$GITRAW/$PACKAGE/$GITBRNCH/scripts/$THISSCRIPT" | BRANCH=$GITBRNCH sudo bash"
    # Cobble together some strings
    GITPROJ="${PACKAGE,,}"
}

############
### Handle logging
############

timestamp() {
    # Add date in '2019-02-26 08:19:22' format to log
    [[ "$VERBOSE" == "true" ]] && length=999 || length=60 # Allow full logging
    while read -r; do
        # Clean and trim line to 60 characters to allow for timestamp on one line
        REPLY="$(clean "$REPLY" "$length")"
        # Strip blank lines
        if [ -n "$REPLY" ]; then
            # Add date in '2019-02-26 08:19:22' format to log
            printf '%(%Y-%m-%d %H:%M:%S)T %s\n' -1 "$REPLY"
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
    thisscript="bootstrap.sh"
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

# version outputs to stdout the --version message.
version() {
cat << EOF

$THISSCRIPT ($PACKAGE)

$COPYRIGHT

This is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.
<https://www.gnu.org/licenses/>

There is NO WARRANTY, to the extent permitted by law.
EOF
}

# Parse arguments and call usage or version
arguments() {
    local arg
    while [[ "$#" -gt 0 ]]; do
        arg="$1"
        case "$arg" in
            --h* )
            usage; exit 0 ;;
            --v* )
            version; exit 0 ;;
            * )
            break;;
        esac
    done
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
        sudo -n true 2> /dev/null
        retval="$?"
        if [ "$retval" -eq 0 ]; then
            echo -e "\nNot running as root, relaunching correctly."
            sleep 2
            eval "$CMDLINE"
            exit "$?"
        else
            # sudo not available, give instructions
            echo -e "\nThis script must be run with root privileges."
            echo -e "Enter the following command as one line:"
            echo -e "$CMDLINE" 1>&2
            exit 1
        fi
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
### Instructions
############

instructions() {
    local sp8 sp23
    sp8="$(printf ' %.0s' {1..8})"
    sp23="$(printf ' %.0s' {1..23})"
    clear
    # Note:  $(printf ...) hack adds spaces at beg/end to support non-black BG
  cat << EOF

$DOT$BGBLK$FGYLW$sp8 __          __                          _____ _ $sp23
$DOT$BGBLK$FGYLW$sp8 \ \        / /                         |  __ (_)$sp23
$DOT$BGBLK$FGYLW$sp8  \ \  /\  / /__ _ __  _ __ _ __ _   _  | |__) | $sp23
$DOT$BGBLK$FGYLW$sp8   \ \/  \/ / __| '_ \| '__| '__| | | | |  ___/ |$sp23
$DOT$BGBLK$FGYLW$sp8    \  /\  /\__ \ |_) | |  | |  | |_| | | |   | |$sp23
$DOT$BGBLK$FGYLW$sp8     \/  \/ |___/ .__/|_|  |_|   \__, | |_|   |_|$sp23
$DOT$BGBLK$FGYLW$sp8                | |               __/ |          $sp23
$DOT$BGBLK$FGYLW$sp8                |_|              |___/           $sp23
$DOT$BGBLK$FGGRN$HHR$RESET
You will be presented with some choices during the install. Most frequently
you will see a 'yes or no' choice, with the default choice capitalized like
so: [y/N]. Default means if you hit <enter> without typing anything, you will
make the capitalized choice, i.e. hitting <enter> when you see [Y/n] will
default to 'yes.'

Yes/no choices are not case sensitive. However; passwords, system names and
install paths are. Be aware of this. There is generally no difference between
'y', 'yes', 'YES', 'Yes'; you get the idea. In some areas you are asked for a
path; the default/recommended choice is in braces like: [/home/wsprrypi].
Pressing <enter> without typing anything will take the default/recommended
choice.

EOF
    read -n 1 -s -r -p  "Press any key when you are ready to proceed. " < /dev/tty
    echo -e ""
}

############
### Set timezone
###########

settime() {
    local date tz
    date=$(date)
    while true; do
        echo -e "\nThe time is currently set to $date."
        tz="$(date +%Z)"
        if [ "$tz" == "GMT" ] || [ "$tz" == "BST" ]; then
            # Probably never been set
            read -rp "Is this correct? [y/N]: " yn  < /dev/tty
            case "$yn" in
                [Yy]* ) echo ; break ;;
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
### Daemon Functions
############

############
### Compare source vs. target
### Arguments are $source and $target
### Return eq, lt, gt based on "version" comparison
############

function compare() {
    echo -e "DEBUG: Entering compare() with src=$1 and tgt=$2"
    local src tgt
    src="$1"
    tgt="$2"
    if [ "$src" == "$tgt" ]; then
        echo "eq"
        elif [ "$(printf '%s\n' "$tgt" "$src" | sort -V | head -n1)" = "$tgt" ]; then
        echo "gt"
    else
        echo "lt";
    fi
}

############
### Call the creation of unit files
### Required:
###   unit - Name of systemd unit
###   ext - Type of controlling script (e.g. "bash" or "python3")
############

do_unit() {
    echo -e "DEBUG: Entering do_unit() with unit=$1 and ext=$2"
    local unit executable ext extension executable retval
    unit="$1"
    ext="$2"
    if [ "$ext" == "bash" ]; then
        extension="sh"
        executable="bash"
    elif [ "$ext" == "python3" ]; then
        extension="py"
        executable="python3"
    else
        echo -e "Unknown extension."&&die
    fi
    # Handle script install
    copy_file "$unit.$extension"

    # Handle Unit file install
    checkdaemon "$unit"
    retval="$?"
    if [[ "$retval" == 0 ]]; then createdaemon "$unit.$extension" "$unit" "root" "$PACKAGENAME" "$(which "$executable")"; fi
}

############
### Copy daemon scripts
### Required:
###   scriptName - Name of script to run under systemd
############

copy_file() {
    echo -e "DEBUG: Entering copy_file() with scriptName=$1"
    local scriptPath scriptName fullName curlFile
    scriptName="$1"
    scriptPath="/usr/local/bin"
    fullName="$scriptPath/$scriptName"
    curlFile="$GITRAW/$GITPROJ/$GITBRNCH/scripts/$scriptName"

    # Download file
    curl -o "$fullName" "$curlFile"

    # See if file begins with "#!"
    if grep -q "#!" "$fullName"; then
        chown root:root "$fullName"
        chmod 0744 "$fullName"
    else
        echo -e "Script install failed for $fullName"&&die
    fi
}

############
### Check existence and version of any current unit files
### Required:  daemonName - Name of Unit
### Returns:  0 to execute, 255 to skip
############

checkdaemon() {
    echo -e "DEBUG: Entering checkdaemon() with daemonName=$1"
    local daemonName unitFile src verchk
    daemonName="${1,,}"
    unitFile="/etc/systemd/system/$daemonName.service"
    if [ -f "$unitFile" ]; then
        src=$(grep "^# Created for $PACKAGENAME version" "$unitFile")
        src=${src##* }
        verchk="$(compare "$src" "$VERSION")"
        if [ "$verchk" == "lt" ]; then
            echo -e "\nUnit file for $daemonName.service exists but is an older version" > /dev/tty
            read -rp "($src vs. $VERSION). Upgrade to newest? [Y/n]: " yn < /dev/tty
            case "$yn" in
                [Nn]* )
                return 255;;
                * )
                return 0 ;; # Do overwrite
            esac
            elif [ "$verchk" == "eq" ]; then
            echo -e "\nUnit file for $daemonName.service exists and is the same version" > /dev/tty
            read -rp "($src vs. $VERSION). Overwrite anyway? [y/N]: " yn < /dev/tty
            case "$yn" in
                [Yy]* ) return 0;; # Do overwrite
                * ) return 255;;
            esac
            elif [ "$verchk" == "gt" ]; then
            echo -e "\nVersion of $daemonName.service file is newer than the version being installed."
            echo -e "Skipping."
            return 255
        fi
    else
        return 0
    fi
}

############
### Create systemd unit file
### Required:
###   scriptName - Name of script to run under Bash
###   daemonName - Name to be used for Unit
###   userName - Context under which daemon shall be run
###   productName - Common name for the daemon
###   processShell - Executable under which the script shall run
############

createdaemon () {
    echo -e "DEBUG: Entering createdaemon() with scriptName=$1 daemonName=$2 userName=$3 productName=$4 processShell=$5"
    local scriptName daemonName userName unitFile unitFileLocation productName processShell
    unitFileLocation="/etc/systemd/system"
    scriptName="$1 -d"
    daemonName="${2,,}"
    userName="$3"
    productName="$4"
    processShell="$5"
    unitFile="$unitFileLocation/$daemonName.service"
    
    if [ -f "$unitFile" ]; then
        echo -e "\nStopping $daemonName daemon.";
        systemctl stop "$daemonName";
        echo -e "Disabling $daemonName daemon.";
        systemctl disable "$daemonName";
        echo -e "Removing unit file $unitFile";
        rm "$unitFile"
    fi
    echo -e "\nCreating $productName unit file for $daemonName ($unitFile)."
    {
        echo -e "# Created for $PACKAGENAME version $VERSION

[Unit]
Description=$productName daemon for: $daemonName
Documentation=https://github.com/lbussy/WsprryPi/discussions
After=multi-user.target

[Service]
Type=simple
Restart=on-failure
RestartSec=5
User=$userName
Group=$userName
ExecStart=$processShell $scriptName
SyslogIdentifier=$daemonName

[Install]
WantedBy=multi-user.target"
    } > "$unitFile"

    chown root:root "$unitFile"
    chmod 0644 "$unitFile"
    echo -e "Reloading systemd config."
    systemctl daemon-reload
    echo -e "Enabling $daemonName daemon."
    # TODO: eval "systemctl enable $daemonName"
    echo -e "Starting $daemonName daemon."
    # TODO: eval "systemctl restart $daemonName"
}

############
### Main function
############

main() {
    VERBOSE=true  # Do not trim logs
    log "$@" # Start logging
    init "$@" # Get constants
    arguments "$@" # Check command line arguments
    echo -e "\n***Script $THISSCRIPT starting.***\n"
    sysver="$(cat "/etc/os-release" | grep 'PRETTY_NAME' | cut -d '=' -f2)"
    sysver="$(sed -e 's/^"//' -e 's/"$//' <<<"$sysver")"
    echo -e "\nRunning on: $sysver\n"
    checkroot # Make sure we are su into root
    term # Add term command constants
    instructions # Show instructions
    settime # Set timezone
    do_unit "wspr" "bash" # Install/upgrade wspr daemon
    # TODO: Make shutdown button install a choice
    do_unit "shutdown-button" "python3" # Install/upgrade shutdown-button daemon
}

############
### Start the script
############

main "$@" && exit 0
