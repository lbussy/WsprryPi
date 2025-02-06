#!/bin/bash

# Get aliases
cd ~
curl -fsSL https://gist.githubusercontent.com/lbussy/23c05d8dc8c24d8d8edddf1d381f1c8b/raw/install_aliases.sh | bash
# Get development packages
sudo apt-get install git gh jq apache2 php colordiff libraspberrypi-dev raspberrypi-kernel-headers -y
# Set up git environment
git clone https://github.com/lbussy/WsprryPi.git
cd WsprryPi
# Setup global ID
git config --global user.email “lee@bussy.org”
git config --global user.name “lbussy”
# Setup authorization
gh auth login -w
# Get correct branch
# TODO: Branch selector?
git checkout install_update
python3 -m venv ./.venv
source ./.venv/bin/activate
python -m pip install --upgrade pip
pip install -r ./requirements.txt
# VS Code
sudo apt-get install code
# Extensions installed for WsprryPi
code --install-extension ms-python.debugpy
code --install-extension ms-python.python
code --install-extension ms-python.vscode-pylance
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cpptools-extension-pack
code --install-extension ms-vscode.cpptools-themes
code --install-extension ms-vscode.makefile-tools
code --install-extension yzhang.markdown-all-in-one