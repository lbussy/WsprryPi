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