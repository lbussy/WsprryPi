#!/bin/bash
#
# This file is part of WsprryPi.
#
# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
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

# Determine the root directory of the current Git repository
LOCAL_SOURCE_DIR="$(git rev-parse --show-toplevel 2>/dev/null)"
if [ -z "$LOCAL_SOURCE_DIR" ]; then
    echo "Error: This script must be run from within a Git repository."
    exit 1
fi
LOCAL_WWW_DIR="$LOCAL_SOURCE_DIR/data" # Data directory under the Git repo root
LOCAL_SCRIPTS_DIR="$LOCAL_SOURCE_DIR/scripts" # Scripts directory under the Git repo root
USE_LOCAL=false  # Default to not using local files

# General constants
declare THISSCRIPT GITBRNCH GITPROJ PACKAGE VERBOSE OWNER COPYRIGHT
declare REPLY CMDLINE GITRAW PACKAGENAME VERSION APTPACKAGES
declare VERBOSE BRANCH WWWFILES REBOOT
# Color/character codes
declare BOLD SMSO RMSO FGBLK FGRED FGGRN FGYLW FGBLU FGMAG FGCYN FGWHT FGRST
declare BGBLK BGRED BGGRN BGYLW BGBLU BGMAG BGCYN BGWHT BGRST DOT HHR LHR RESET

# Set branch
BRANCH="version_files"
VERSION="1.2.1-version-files+90.43503fc-dirty"
# Set this script
THISSCRIPT="install.sh"
# Set Project
COPYRIGHT="Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)"
PACKAGE="WsprryPi"
PACKAGENAME="Wsprry Pi"
OWNER="lbussy"
APTPACKAGES="apache2 php libraspberrypi-dev raspberrypi-kernel-headers"
WWWFILES="android-chrome-192x192.png android-chrome-512x512.png antenna.svg apple-touch-icon.png bootstrap.bundle.min.js bootstrap.css custom.css fa.js favicon-16x16.png favicon-32x32.png favicon.ico .gitignore index.php jquery-3.6.3.min.js site.webmanifest wspr_ini.php shutdown.php"
WWWREMOV="bootstrap-icons.css custom.min.css ham_white.svg README.md"
# This should not change
if [ -z "$BRANCH" ]; then GITBRNCH="main"; else GITBRNCH="$BRANCH"; fi
GITRAW="https://raw.githubusercontent.com/$OWNER"

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
        echo -e "\nRaspbian older than version 1.2.1-36ba1cd-dirty [sigterm]-dirty [sigterm]-dirty [sigterm] (bullseye) not supported.\n"
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
### Init
############

init() {
    # Set up some project variables we won't have running as a curled script
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
            -l )
            USE_LOCAL=true ;; # Enable local file usage
            * )
            break;;
        esac
        shift
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
            echo -e "\nNot running as root, re-run using 'sudo'."
            sleep 2
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
    local date tz
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
### Daemon Functions
############

############
### Compare source vs. target
### Arguments are $source and $target
### Return eq, lt, gt based on "version" comparison
############

compare() {
    local src tgt
    src="$1"
    tgt="$2"

    if dpkg --compare-versions "$src" eq "$tgt"; then
        echo "eq"
    elif dpkg --compare-versions "$src" lt "$tgt"; then
        echo "lt"
    else
        echo "gt"
    fi
}

############
### Call the creation of unit files
### Required:
###   unit - Name of systemd unit
###   ext - Type of controlling script (e.g. "bash" or "python3")
############

do_unit() {
    local unit executable ext extension executable retval
    path="/usr/local/bin"
    unit="$1"
    ext="$2"
    arg="$3"
    systemctl stop "$unit" &> /dev/null
    if [ "$ext" == "bash" ]; then
        extension=".sh"
        executable="bash"
    elif [ "$ext" == "python3" ]; then
        extension=".py"
        executable="python3"
    elif [ "$ext" == "exe" ]; then
        extension=""
        executable=""
    else
        echo -e "Unknown extension." && die "$@"
    fi
    # Handle script install
    checkscript "$unit$extension"
    retval="$?"
    if [[ "$retval" == 0 ]]; then
        copy_file "$unit" "$extension"
    fi

    # Handle Unit file install
    checkdaemon "$unit"
    retval="$?"
    if [[ "$retval" == 0 ]]; then
        createdaemon "$unit$extension" "$path" "$arg" "$unit" "root" "wspr" "$(which "$executable")"
    else
        eval "systemctl restart $unit"
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
    local srcFile localDir targetFile remoteURL
    localDir="$2"
    targetFile="$3"
    remoteURL="$4"

    if [ "$USE_LOCAL" == "true" ]; then
        # Use local file
        srcFile="$localDir/$1"
        if [ ! -f "$srcFile" ]; then
            echo "Local file $srcFile not found." && die
        fi
        cp "$srcFile" "$targetFile" || die
    else
        # Download from GitHub
        curl -s "$remoteURL/$1" > "$targetFile" || die
    fi
}

############
### Copy daemon scripts
### Required:
###   scriptName - Name of script to run under systemd
############

copy_file() {
    local scriptName extension scriptPath fullName remoteURL
    scriptName="$1"
    extension="$2"
    scriptPath="/usr/local/bin"
    fullName="$scriptPath/$scriptName$extension"
    remoteURL="$GITRAW/$GITPROJ/$GITBRNCH/scripts"

    copy_file_generic "$scriptName$extension" "$LOCAL_SCRIPTS_DIR" "$fullName" "$remoteURL"

    # Set permissions
    if file "$fullName" | grep -q executable; then
        chown root:root "$fullName"
        chmod 0755 "$fullName"
    else
        echo "Script install failed for $fullName." && die
    fi
}

############
### Copy Log Rotate Config
############

copy_logd() {
    local fullName remoteURL
    fullName="/etc/logrotate.d/wspr"
    remoteURL="$GITRAW/$GITPROJ/$GITBRNCH/scripts"

    copy_file_generic "logrotate.conf" "$LOCAL_SCRIPTS_DIR" "$fullName" "$remoteURL"

    chown root:root "$fullName"
    chmod 0644 "$fullName"
}

############
### Check existence and version of any current script files
### Required:  scriptName - Name of script
### Returns:   0 to execute, 255 to skip
############

# TODO: Simplify script version checking

checkscript() {
    local scriptName scriptFile src verchk
    scriptName="${1,,}"
    scriptFile="/usr/local/bin/$scriptName"

    if [ -f "$scriptFile" ]; then
        # Handle version extraction based on script name
        if [[ "$scriptName" == "shutdown_watch" ]]; then
            # Extract version for shutdown_watch.py from the comment line
            src=$(grep -Pzo "Created for the WsprryPi project, version [\d\.]+[-\w]*\s*(\[[^\]]*\])?" "$scriptFile" | \
                  sed -E 's/.*version ([^ ]+).*/\1/')
            if [ -z "$src" ]; then
                # Fallback: Try to extract the version by running the script with a flag if no comment is found
                src=$(python3 "$scriptFile" -v 2>&1 | grep -oP 'version \K[^\s]+')
            fi
        elif [[ "$scriptName" == "wspr" ]]; then
            # Extract version for wspr using the -v command output
            src=$(wspr -v | grep -oP 'version \K[^\s]+')
        else
            # General case for other scripts with version info in a comment line
            src=$(grep -Eo "^# Created for the WsprryPi project, version [^ ]+" "$scriptFile" | \
                  sed -E 's/^.*version //')
        fi

        # If no version is found, skip further checks and default to overwrite
        if [ -z "$src" ]; then
            echo -e "\nVersion info not found in $scriptName. Skipping version check and proceeding with overwrite."
            return 0  # Default to overwrite
        fi

        # Compare versions
        verchk="$(compare "$src" "$VERSION")"
        case "$verchk" in
            lt)
                echo -e "\nFile: $scriptName exists but is an older version ($src vs. $VERSION)." > /dev/tty
                read -rp "Upgrade to newest? [Y/n]: " yn < /dev/tty
                case "$yn" in
                    [Nn]* ) return 255 ;;  # Skip update
                    * ) return 0 ;;        # Do overwrite
                esac
                ;;
            eq)
                echo -e "\nFile: $scriptName exists and is the same version ($src)."
                read -rp "Overwrite anyway? [y/N]: " yn < /dev/tty
                case "$yn" in
                    [Yy]* ) return 0 ;;  # Overwrite
                    * ) return 255 ;;    # Skip overwrite (default to No)
                esac
                ;;
            gt)
                echo -e "\nFile: $scriptName is newer than the version being installed ($src vs. $VERSION)." > /dev/tty
                return 255 ;;  # Skip update
        esac
    else
        echo "File: $scriptName does not exist. Proceeding with installation." > /dev/tty
        return 0  # File does not exist, proceed with installation
    fi
}

############
### Check existence and version of any current unit files
### Required:  daemonName - Name of Unit
### Returns:  0 to execute, 255 to skip
############

checkdaemon() {
    local daemonName unitFile src verchk
    daemonName="${1,,}"
    unitFile="/etc/systemd/system/$daemonName.service"

    if systemctl list-unit-files | grep -q "^$daemonName.service"; then
        # Extract the version
        src=$(grep "^# Created for $PACKAGE version" "$unitFile" | awk '{print $NF}')
        if [ -z "$src" ]; then
            src="unknown"
        fi

        # Compare versions
        verchk="$(compare "$src" "$VERSION")"
        case "$verchk" in
            lt)
                echo -e "\nThe unit file for the $daemonName.service exists but is an older version." > /dev/tty
                read -rp "($src vs. $VERSION). Upgrade to newest? [Y/n]: " yn < /dev/tty
                case "$yn" in
                    [Nn]* ) return 255 ;;
                    * ) return 0 ;;
                esac
                ;;
            eq)
                echo -e "\nThe unit file for the $daemonName.service exists and is the same version ($src)."

                read -rp "($src vs. $VERSION). Overwrite anyway? [y/N]: " yn < /dev/tty
                case "$yn" in
                    [Yy]* ) return 0 ;;
                    * ) return 255 ;;
                esac
                ;;
            gt)
                echo -e "\nUnit file for $daemonName.service is newer than the version being installed." > /dev/tty
                return 255 ;;
        esac
    else
        return 0
    fi
}

############
### Create systemd unit file
### Required:
###   scriptName - Name of script to run under Bash
###   scriptPath - Path to scriptName
###   daemonName - Name to be used for Unit
###   userName - Context under which daemon shall be run
###   productName - Common name for the daemon
###   processShell - Executable under which the script shall run
############

createdaemon () {
    local scriptName scriptPath daemonName userName unitFile unitFileLocation productName processShell execStart
    unitFileLocation="/etc/systemd/system"
    logFileLocation="/var/log"
    scriptName="$1"
    scriptPath="$2"
    arguments="$3"
    daemonName="${4,,}"
    userName="$5"
    productName="$6"
    processShell="$7"
    execStart=""
    dirName="${productName// /}"
    dirName="${dirName,,}"
    unitFile="$unitFileLocation/$daemonName.service"
    logFileLocation="$logFileLocation/$dirName"
    stdLog="$logFileLocation/$daemonName.transmit.log"
    errLog="$logFileLocation/$daemonName.error.log"

    # Remove old version
    rm -fr /var/log/wsprrypi 2>/dev/null

    # ExecStart=$processShell $envSet $scriptPath/$scriptName $arguments
    if [ -n "$processShell" ]; then
        execStart="$processShell"
        if [ -n "$scriptPath" ]; then execStart="${execStart} $scriptPath/"; fi
    else
        if [ -n "$scriptPath" ]; then execStart="$scriptPath/"; fi
    fi
    if [ -n "$scriptName" ]; then execStart="${execStart}$scriptName"; fi
    if [ -n "$arguments" ]; then execStart="${execStart} $arguments"; fi

    if [ -f "$unitFile" ]; then
        echo -e "\nStopping $daemonName daemon.";
        systemctl stop "$daemonName";
        echo -e "Disabling $daemonName daemon.";
        systemctl disable "$daemonName";
        echo -e "Removing unit file $unitFile.";
        rm "$unitFile"
    fi
    echo -e "\nCreating unit file for $daemonName ($unitFile)."
    {
        echo -e "# Created for $PACKAGE version $VERSION

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
ExecStart=$execStart
SyslogIdentifier=$daemonName
StandardOutput=append:$stdLog
StandardError=append:$errLog

[Install]
WantedBy=multi-user.target"
    } > "$unitFile"

    [[ -d "$logFileLocation" ]] || mkdir "$logFileLocation"
    chown root:root "$unitFile"
    chmod 0644 "$unitFile"
    echo -e "Reloading systemd config."
    systemctl daemon-reload
    echo -e "Enabling $daemonName daemon."
    eval "systemctl enable $daemonName"
    echo -e "Starting $daemonName daemon."
    eval "systemctl restart $daemonName"
    echo
}

############
### Create wspr ini file
### Required:
###   none
############

createini() {
    local fullName
    fullName="/usr/local/etc/wspr.ini"

    if [ -f "$fullName" ]; then
        echo
        read -rp "Configuration file exists, overwrite? [y/N/s (skip)]: " yn < /dev/tty
        case "$yn" in
            [Yy]* )
                echo "Overwriting configuration file." ;;
            [Ss]* )
                echo "Skipping configuration file setup."
                return ;;  # Skip this step
            * )
                echo "Keeping existing configuration file."
                return ;;
        esac
    fi

    echo "Creating configuration file for $PACKAGENAME."
    # Rest of the function logic here
}

############
### Install apt packages
############

aptPackages() {
    local lastUpdate nowTime pkgOk upgradesAvail pkg

    echo -e "\nUpdating any expired apt keys."
    for K in $(apt-key list 2> /dev/null | grep expired | cut -d'/' -f2 | cut -d' ' -f1); do
	    sudo apt-key adv --recv-keys --keyserver keys.gnupg.net "$K";
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

    # Restart Apache just in case
    systemctl restart apache2
}

############
### Install website
############

doWWW() {
    local file dir fullName remoteURL
    dir="/var/www/html/wspr"
    remoteURL="$GITRAW/$GITPROJ/$GITBRNCH/data"

    # Delete old files
    echo -e "\nDeleting any deprecated files."
    for file in $WWWREMOV; do
        if [ -f "$dir/$file" ]; then
            rm -f "$dir/$file"
        fi
    done

    # Copy down web pages
    echo -e "\nChecking and installing web pages."
    if [ ! -d "$dir" ]; then
        mkdir "$dir"
    fi

    for file in $WWWFILES; do
        fullName="$dir/$file"
        copy_file_generic "$file" "$LOCAL_WWW_DIR" "$fullName" "$remoteURL"
    done

    # Set the permissions
    echo -e "\nFixing file permissions for $dir."
    chown -R www-data:www-data "$dir" || die
    find "$dir" -type d -exec chmod 2770 {} \; || die
    find "$dir" -type f -exec chmod 660 {} \; || die
}

############
### TAPR Shutdown Button Support
############

support_shutdown_button() {
    local yn
    echo
    # Check if the shutdown_watch service is already enabled
    if systemctl is-enabled shutdown_watch.service &>/dev/null; then
        echo "TAPR shutdown button support is already enabled."
        read -p "Do you want to disable TAPR shutdown button support? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]* )
                echo "Disabling TAPR shutdown button support."
                systemctl stop shutdown_watch.service || true
                systemctl disable shutdown_watch.service || true
                rm -f /etc/systemd/system/shutdown_watch.service
                echo "Reloading systemd daemon configuration."
                systemctl daemon-reload
                ;;
            * )
                echo "Keeping TAPR shutdown button support enabled."
                # Check version of the shutdown_watch service
                verchk="$(compare "$(systemctl show -p Version shutdown_watch.service)" "$VERSION")"
                case "$verchk" in
                    lt)
                        echo -e "\nThe current version of shutdown_watch is older than the version being installed. Upgrading to the latest version."
                        do_unit "shutdown_watch" "python3"  # Upgrade the service
                        ;;
                    eq)
                        echo -e "\nThe version of shutdown_watch is already up to date."
                        ;;
                    gt)
                        echo -e "\nThe version of shutdown_watch is newer than the version being installed. Skipping upgrade."
                        ;;
                esac
                ;;
        esac
    else
        # Prompt user to enable support
        read -p "Support system shutdown button (TAPR)? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]* )
                echo "Enabling TAPR shutdown button support."
                do_unit "shutdown_watch" "python3"
                ;;
            * )
                echo "TAPR shutdown button support remains disabled."
                ;;
        esac
    fi
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
    check_bitness # Make sure we are not 64-bit
    check_release # Make sure we are not using some dusty old version
    check_architecture # Make sure we are not on a Pi 5
    log "$@" # Start logging
    init "$@" # Get constants
    arguments "$@" # Check command line arguments
    echo -e "\n***Script $THISSCRIPT starting.***"
    sysver="$(cat "/etc/os-release" | grep 'PRETTY_NAME' | cut -d '=' -f2)"
    sysver="$(sed -e 's/^"//' -e 's/"$//' <<<"$sysver")"
    echo -e "\nRunning on: $sysver.\n"
    checkroot # Make sure we are su into root
    term # Add term command constants
    instructions # Show instructions
    settime # Set timezone
    # DEBUG TODO aptPackages # Install any apt packages needed
    do_unit "wspr" "exe" "-D -i /usr/local/etc/wspr.ini" # Install/upgrade wspr daemon
    createini # Create ini file
    support_shutdown_button # Handle TAPR shutdown button
    copy_logd "$@" # Enable log rotation
    doWWW # Download website
    disable_sound
    echo -e "\n***Script $THISSCRIPT complete.***\n"
    complete
}

############
### Start the script
############

main "$@" && exit 0