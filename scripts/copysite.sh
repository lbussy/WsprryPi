#!/bin/bash

# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi version 1.2.1-Beta.1 [bookworm-32).

# Get repo root
repo_root=$(git rev-parse --show-toplevel 2>/dev/null)
if [ -z "$repo_root" ]; then
    echo "Not in a Git repository."
    exit
fi

sudo rm -fr /var/www/html/wspr
sudo mkdir /var/www/html/wspr
sudo cp -R "$repo_root"/WsprryPi/data/* /var/www/html/wspr/
sudo ln -sf /usr/local/etc/wspr.ini /var/www/html/wspr/wspr.ini
sudo chown -R www-data:www-data /var/www/html/wspr/
