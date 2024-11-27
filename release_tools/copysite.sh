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
repo_root=$(git rev-parse --show-toplevel 2>/dev/null)
if [ -z "$repo_root" ]; then
    echo "Not in a Git repository."
    exit
fi

sudo rm -fr /var/www/html/wspr
sudo mkdir /var/www/html/wspr
sudo cp -R "$repo_root"/data/* /var/www/html/wspr/
sudo ln -sf /usr/local/etc/wspr.ini /var/www/html/wspr/wspr.ini
sudo chown -R www-data:www-data /var/www/html/wspr/
echo "Website copied."
