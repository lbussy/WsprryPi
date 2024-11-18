#!/bin/bash

# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.1-69995bb [new_release_proc].

############
### Global Declarations
############

# shellcheck disable=SC2034  # Unused variables left for reusability

# General constants
declare THISSCRIPT GITBRNCH GITPROJ PACKAGE VERBOSE OWNER COPYRIGHT
declare REPLY CMDLINE GITRAW PACKAGENAME VERSION
declare VERBOSE BRANCH
# Color/character codes
declare BOLD SMSO RMSO FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST
declare BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST DOT HHR LHR RESET

# Set branch
BRANCH=main
VERSION=1.2.1
# Set this script
THISSCRIPT="uninstall.sh"
# Set Project
COPYRIGHT="Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)"
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
### Uninstall function
############

uninstall() {
    systemctl stop wspr.service 2>/dev/null
    systemctl disable wspr.service 2>/dev/null
    systemctl stop shutdown-button.service 2>/dev/null
    systemctl stop shutdown-watch.service 2>/dev/null
    systemctl disable shutdown-button.service 2>/dev/null
    systemctl disable shutdown-watch.service 2>/dev/null
    rm -f /etc/systemd/system/wspr.service 2>/dev/null
    rm -f /usr/local/bin/wspr 2>/dev/null
    rm -f /usr/local/etc/wspr.ini 2>/dev/null
    rm -f /etc/systemd/system/shutdown-button.service 2>/dev/null
    rm -f /etc/systemd/system/shutdown_button.service 2>/dev/null
    rm -f /etc/systemd/system/shutdown-watch.service 2>/dev/null
    rm -f /usr/local/bin/shutdown-button.py 2>/dev/null
    rm -f /usr/local/bin/shutdown-watch.py 2>/dev/null
    rm -fr /var/www/html/wspr/ 2>/dev/null
    sed -i '/blacklist snd_bcm2835/d' /etc/modprobe.d/alsa-blacklist.conf
    rm -fr /var/log/wsprrypi 2>/dev/null
    rm -f /etc/logrotate.d/wsprrypi 2>/dev/null
    rm -fr /var/log/wspr 2>/dev/null
    rm -f /etc/logrotate.d/wspr 2>/dev/null
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
    echo -e "Running on: $sysver\n"
    echo -e "Uninstalling Wsprry Pi.\n"
    checkroot # Make sure we are su into root
    uninstall # Uninstall services
    echo -e "***Script $THISSCRIPT complete.***\n"
}

############
### Start the script
############

main "$@" && exit 0
