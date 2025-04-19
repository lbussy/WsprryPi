#!/bin/bash

# Copyright (C) 2023-2025 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.3-c95920f [main].

# Begin
#
# Get repo root
repo_root=$(git rev-parse --show-toplevel 2>/dev/null)
if [ -z "$repo_root" ]; then
    echo "Not in a Git repository."
    exit
fi

# Check if wspr is already installed
if ! systemctl list-unit-files | grep -q "^wspr"; then
    echo "WsprryPi is not installed."
    echo "Please use install.sh to set up environment for the first time."
    exit
fi

# Stop wspr daemons if they exist
sudo systemctl stop wspr 2>/dev/null || true
sudo systemctl stop shutdown-button 2>/dev/null || true
sudo systemctl stop shutdown_button 2>/dev/null || true

# Copy new files
sudo cp -f "$repo_root"/scripts/wspr /usr/local/bin
sudo cp -f "$repo_root"/scripts/wspr.ini /usr/local/etc
sudo cp -f "$repo_root"/scripts/logrotate.d /etc/logrotate.d/wspr

# Refresh shutdown button if it was installed
sudo rm -f "/usr/local/bin/shutdown-watch.py"
if systemctl list-unit-files | grep -q "^shutdown-button"; then
    sudo cp -f "$repo_root"/scripts/shutdown_button.py /usr/local/bin
fi

# Make sure log location exists
[[ -d "/var/log/wspr" ]] || mkdir "/var/log/wspr"

# Reload systemctl daemon to pick up changes and start wspr
sudo systemctl daemon-reload
sudo systemctl start wspr

# If shutdown button is installed, start it
if systemctl list-unit-files | grep -q "^shutdown-button"; then
    sudo rm -fr /etc/systemd/system/shutdown-button.service
fi
if systemctl list-unit-files | grep -q "^shutdown-watch"; then
    sudo rm -fr /etc/systemd/system/shutdown-watch.service
fi
if systemctl list-unit-files | grep -q "^shutdown_watch"; then
    systemctl start shutdown_watch
else
    echo "shutdown_watch.service is not installed, re-run installer."
fi
echo "Executables copied."
