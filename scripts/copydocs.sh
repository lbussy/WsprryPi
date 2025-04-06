#!/bin/bash

# Copyright (C) 2023-2025 Lee C. Bussy (@LBussy)
# Created for WsprryPi project, version 1.2.2-e375721 [devel].

# Begin
#
# Get repo root
repo_root=$(git rev-parse --show-toplevel 2>/dev/null)
if [ -z "$repo_root" ]; then
    echo "Not in a Git repository."
    exit
fi

(cd "$repo_root"/docs || exit; make clean)
(cd "$repo_root"/docs || exit; make html)
sudo rm -fr /var/www/html/wspr/docs
sudo mkdir -p /var/www/html/wspr/docs
sudo cp -R "$repo_root"/docs/_build/html/* /var/www/html/wspr/docs/
sudo chown -R www-data:www-data /var/www/html/wspr/docs
echo "Docs copied."
