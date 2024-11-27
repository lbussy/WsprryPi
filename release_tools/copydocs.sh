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

(cd "$repo_root"/docs || exit; make clean)
(cd "$repo_root"/docs || exit; make html)
sudo rm -fr /var/www/html/wspr/docs
sudo mkdir -p /var/www/html/wspr/docs
sudo cp -R "$repo_root"/docs/_build/html/* /var/www/html/wspr/docs/
sudo chown -R www-data:www-data /var/www/html/wspr/docs
echo "Docs copied."
