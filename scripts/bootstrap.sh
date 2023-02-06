#!/bin/bash

# Copyright (C) 2023 Lee C. Bussy (@LBussy)

# Install to /usr/local/bin

############
### Global Declarations
############

# General constants
declare THISSCRIPT GITBRNCH GITURL GITPROJ PACKAGE VERBOSE OWNER COPYRIGHT
declare REPLY SOURCE SCRIPTSOURCE SCRIPTPATH CMDLINE GITRAW GITHUB CLONE
declare SCRIPTNAME GITCMD GITTEST APTPACKAGES VERBOSE LINK BRANCH
# Color/character codes
declare BOLD SMSO RMSO FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST
declare BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST DOT HHR LHR RESET

# Set branch
BRANCH=scripts
COPYRIGHT="Copyright (C) 2023 Lee C. Bussy (@LBussy)"
PACKAGE="WsppryPi"
OWNER="lbussy"

# These should not change
if [ -z "$BRANCH" ]; then GITBRNCH="main"; else GITBRNCH="$BRANCH"; fi
THISSCRIPT="bootstrap.sh"
LINK="https://raw.githubusercontent.com/$OWNER/$PACKAGE/$GITBRNCH/scripts/bootstrap.sh"
GITRAW="https://raw.githubusercontent.com/$OWNER"
GITHUB="https://github.com/$OWNER"

############
### Init
############

init() {
    # Set up some project variables we won't have running as a curled script
    echo $LINK || exit
    CMDLINE="curl -L $LINK | BRANCH=$GITBRNCH sudo bash"
    # Cobble together some strings
    SCRIPTNAME="${THISSCRIPT%%.*}"
    GITPROJ="${PACKAGE,,}"
    GITHUB="$GITHUB/$GITPROJ.git"
    GITRAW="$GITRAW/$GITPROJ/$GITBRNCH/$THISSCRIPT"
    GITCMD="$GITHUB"
    # Website for network test
    GITTEST="$GITHUB"
    # Packages to be installed/checked via apt
    APTPACKAGES="git libraspberrypi-dev raspberrypi-kernel-headers"
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
    # And get the user home directory
    shadow="$( (getent passwd "$REALUSER") 2>&1)"
    retval="$?"
    if [ "$retval" -eq 0 ]; then
        HOMEPATH="$(echo "$shadow" | cut -d':' -f6)"
    else
        echo -e "\nUnable to retrieve $REALUSER's home directory. Manual install may be necessary."
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
### Instructions
############

instructions() {
    local any sp10 sp21
    sp10="$(printf ' %.0s' {1..10})"
    sp21="$(printf ' %.0s' {1..21})"
    clear
    # Note:  $(printf ...) hack adds spaces at beg/end to support non-black BG
  cat << EOF

$DOT$BGBLK$FGYLW$sp10 __          __                          _____ _ $sp21
$DOT$BGBLK$FGYLW$sp10 \ \        / /                         |  __ (_)$sp21
$DOT$BGBLK$FGYLW$sp10  \ \  /\  / /__ _ __  _ __ _ __ _   _  | |__) | $sp21
$DOT$BGBLK$FGYLW$sp10   \ \/  \/ / __| '_ \| '__| '__| | | | |  ___/ |$sp21
$DOT$BGBLK$FGYLW$sp10    \  /\  /\__ \ |_) | |  | |  | |_| | | |   | |$sp21
$DOT$BGBLK$FGYLW$sp10     \/  \/ |___/ .__/|_|  |_|   \__, | |_|   |_|$sp21
$DOT$BGBLK$FGYLW$sp10                | |               __/ |          $sp21
$DOT$BGBLK$FGYLW$sp10                |_|              |___/           $sp21
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
### Check for default 'pi' password and gently prompt to change it now
############

checkpass() {
    local user_exists salt extpass match badpwd yn setpass
    user_exists=$(id -u 'pi' > /dev/null 2>&1; echo $?)
    if [ "$user_exists" -eq 0 ]; then
        salt=$(getent shadow "pi" | cut -d$ -f3)
        extpass=$(getent shadow "pi" | cut -d: -f2)
        match=$(python3 -c 'import crypt; print(crypt.crypt("'"raspberry"'", "$6$'${salt}'"))')
        [ "${match}" == "${extpass}" ] && badpwd=true || badpwd=false
        if [ "$badpwd" = true ]; then
            echo -e "\nDefault password found for the 'pi' account. This should be changed."
            while true; do
                read -rp "Do you want to change the password now? [Y/n]: " yn  < /dev/tty
                case "$yn" in
                    '' ) setpass=1; break ;;
                    [Yy]* ) setpass=1; break ;;
                    [Nn]* ) break ;;
                    * ) echo "Enter [y]es or [n]o." ;;
                esac
            done
        fi
        if [ -n "$setpass" ]; then
            echo
            until passwd pi < /dev/tty; do sleep 2; echo; done
            echo -e "\nYour password has been changed, remember it or write it down now."
            sleep 5
        fi
    fi
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
### Change hostname
###########

host_name() {
    local oldHostName yn sethost host1 host2 newHostName
    oldHostName=$(hostname)
    if [ "$oldHostName" = "raspberrypi" ]; then
        while true; do
            echo -e "\nYour hostname is set to '$oldHostName'. Each machine on your network should"
            echo -e "have a unique name to prevent issues. Do you want to change it now, maybe"
            read -rp "to 'wsprrypi'? [Y/n]: " yn < /dev/tty
            case "$yn" in
                '' ) sethost=1; break ;;
                [Yy]* ) sethost=1; break ;;
                [Nn]* ) break ;;
                * ) echo "Enter [y]es or [n]o." ; sleep 1 ; echo ;;
            esac
        done
        echo
        if [ -n "$sethost" ]; then
            echo -e "You will now be asked to enter a new hostname."
            while
            read -rp "Enter new hostname: " host1  < /dev/tty
            read -rp "Enter new hostname again: " host2 < /dev/tty
            [[ -z "$host1" || "$host1" != "$host2" ]]
            do
                echo -e "\nHost names blank or do not match.\n";
                sleep 1
            done
            echo
            newHostName=$(echo "$host1" | awk '{print tolower($0)}')
            eval "sed -i 's/$oldHostName/$newHostName/g' /etc/hosts"||die
            eval "sed -i 's/$oldHostName/$newHostName/g' /etc/hostname"||die
            hostnamectl set-hostname "$newHostName"
            /etc/init.d/avahi-daemon restart
            echo -e "\nYour hostname has been changed to '$newHostName'."
            echo -e "\n(If your hostname is part of your prompt, your prompt will not change until"
            echo -e "you log out and in again.  This will have no effect on anything but the way"
            echo -e "the prompt looks.)"
            sleep 5
        fi
    fi
}

############
### Install or update required packages
############

packages() {
    local lastUpdate nowTime pkgOk upgradesAvail pkg
    echo -e "\nUpdating any expired apt keys."
    for K in $(apt-key list 2> /dev/null | grep expired | cut -d'/' -f2 | cut -d' ' -f1); do
	    sudo apt-key adv --recv-keys --keyserver keys.gnupg.net $K;
    done
    echo -e "\nFixing any broken installations."
    sudo apt-get --fix-broken install -y||die
    # Run 'apt update' if last run was > 1 week ago
    lastUpdate=$(stat -c %Y /var/lib/apt/lists)
    nowTime=$(date +%s)
    if [ $((nowTime - lastUpdate)) -gt 604800 ]; then
        echo -e "\nLast apt update was over a week ago. Running apt update before updating"
        echo -e "dependencies."
        apt-get update -yq||die
    fi
    
    # Now install any necessary packages if they are not installed
    echo -e "\nChecking and installing required dependencies via apt."
    for pkg in $APTPACKAGES; do
        pkgOk=$(dpkg-query -W --showformat='${Status}\n' "$pkg" | \
        grep "install ok installed")
        if [ -z "$pkgOk" ]; then
            echo -e "\nInstalling '$pkg'."
            apt-get install "$pkg" -y -q=2||die
        fi
    done
    
    # Get list of installed packages with updates available
    upgradesAvail=$(dpkg --get-selections | xargs apt-cache policy {} | \
        grep -1 Installed | sed -r 's/(:|Installed: |Candidate: )//' | \
    uniq -u | tac | sed '/--/I,+1 d' | tac | sed '$d' | sed -n 1~2p)
    # Loop through the required packages and see if they need an upgrade
    for pkg in $APTPACKAGES; do
        if [[ "$upgradesAvail" == *"$pkg"* ]]; then
            echo -e "\nUpgrading '$pkg'."
            apt-get install "$pkg" -y -q=2||die
        fi
    done
}

############
### Check for an existing WsprryPi installation
############

check_wsprrypi() {
    # TODO:  Figure out how we want to do this
    if [ -d "$HOMEPATH/$GITPROJ" ]; then
        if [ -n "$(ls -A "$HOMEPATH/$GITPROJ")" ]; then
            echo -e "\nWarning: $HOMEPATH/$GITPROJ exists and is not empty."
        else
            echo -e "\nWarning: $HOMEPATH/$GITPROJ exists."
        fi
        echo -e "\nIf you are sure you do not need it, or you are starting over completely, we can"
        echo -e "delete the old repo by accepting the below prompt."
        read -rp "Remove $HOMEPATH/$GITPROJ? [y/N] " < /dev/tty
        if [[ "$REPLY" =~ ^[Yy]$ ]]; then
            rm -fr "${HOMEPATH:?}/$GITPROJ"
        else
            echo -e "\nLeaving $HOMEPATH/$GITPROJ in place and exiting."
            exit 1
        fi
    fi
}

############
### Clone WsprryPi repo
############

clone_repo() {
    # TODO: Clone repo instead of install
    echo -e "\nCloning $GITPROJ repo."
    eval "sudo -u $REALUSER git clone $GITCMD $HOMEPATH/$GITPROJ"||die
    cd "$HOMEPATH/$GITPROJ"||die
    eval "sudo -u $REALUSER git checkout $GITBRNCH"||die
    cd "$HOMEPATH"||die
}

############
### Install Wsprry Pi function
############

install_program() {
    true
    # TODO: Install vs clone
}

############
### Main function
############

main() {
    [[ "$*" == *"-clone"* ]] && CLONE=true # Clone repo only
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
    if [[ "$VERBOSE" == "true" ]]; then
        true
    else
        true
    fi
    # check_wsprrypi # See if WsprryPi is installed
    # checkpass # Check for default password
    # settime # Set timezone
    # host_name # Change hostname
    # packages # Install and update required packages
    # clone_repo # Clone tools repo
    # TODO:  Are we done?  Any follow-up?
}

############
### Start the script
############

main "$@" && exit 0
