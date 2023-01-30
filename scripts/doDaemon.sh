#!/bin/bash

############
### Compare source vs. target
### Arguments are $source and $target
### Return eq, lt, gt based on "version" comparison
############

function compare() {
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
### Check existence and version of any current unit files
### Required:  daemonName - Name of Unit
### Returns:  0 to execute, 255 to skip
############

checkdaemon() {
    local daemonName unitFile src verchk
    daemonName="${1,,}"
    unitFile="/etc/systemd/system/$daemonName.service"
    if [ -f "$unitFile" ]; then
        src=$(grep "^# Created for BrewPi version" "$unitFile")
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
############

createdaemon () {
    local scriptName daemonName userName unitFile productName
    scriptName="$1 -d"
    daemonName="${2,,}"
    userName="$3"
    productName="$4"
    unitFile="/etc/systemd/system/$daemonName.service"
    if [ -f "$unitFile" ]; then
        echo -e "\nStopping $daemonName daemon.";
        systemctl stop "$daemonName";
        echo -e "Disabling $daemonName daemon.";
        systemctl disable "$daemonName";
        echo -e "Removing unit file $unitFile";
        rm "$unitFile"
    fi
    echo -e "\nCreating unit file for $daemonName."
    {
        echo -e "# Created for BrewPi version $VERSION

[Unit]
Description=$productName daemon for: $daemonName
Documentation=https://docs.brewpiremix.com/
After=multi-user.target

[Service]
Type=simple
Restart=on-failure
RestartSec=1
User=$userName
Group=$userName
ExecStart=/bin/bash $scriptName
SyslogIdentifier=$daemonName

[Install]
WantedBy=multi-user.target"
    } > "$unitFile"
    chown root:root "$unitFile"
    chmod 0644 "$unitFile"
    echo -e "Reloading systemd config."
    systemctl daemon-reload
    echo -e "Enabling $daemonName daemon."
    eval "systemctl enable $daemonName"
    echo -e "Starting $daemonName daemon."
    eval "systemctl restart $daemonName"
}

############
### Call the creation of unit files
############

project_unit() {
    local brewpicheck retval
    # Handle WSPR Unit file setup
    checkdaemon "wspr"
    retval="$?"
    if [[ "$retval" == 0 ]]; then createdaemon "wspr.sh" "wspr" "root" "Wsprry Pi"
}

shutdown_unit() {
    local retval
    # Handle Shutdown Watch Unit file setup
    checkdaemon "shutdown"
    retval="$?"
    if [[ "$retval" == 0 ]]; then createdaemon "shutdown.py" "shutdown-button" "root" "Wsprry Pi"
}
