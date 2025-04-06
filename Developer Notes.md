<!-- omit in toc -->
# Developer Notes

- [VS Code](#vs-code)
  - [Extensions installed on SSH: wspr.local](#extensions-installed-on-ssh-wsprlocal)
- [Optional Aliases](#optional-aliases)
- [Passwordless (public key) Login to Pi](#passwordless-public-key-login-to-pi)
- [Required Libs](#required-libs)
  - [Cleanup Existing Environment and Libs](#cleanup-existing-environment-and-libs)
  - [Install Required Libs](#install-required-libs)
- [Python](#python)
- [Repo Tools and Scripts](#repo-tools-and-scripts)


## VS Code

I use VS Code to develop this environment and connect my workstation to my Pi via the [VS Code Remote Development](https://code.visualstudio.com/docs/remote/remote-overview) option.  This tool makes compiling and testing on the Pi very easy as I work from my laptop.

These are the extensions I use.  I have no idea if you need all of these; I strongly suspect not.  You may copy/paste these commands to install all of them.  If you have already "moved in" to VS Code, some of these may conflict with the extensions you already have.

### Extensions installed on SSH: wspr.local

``` bash
code --install-extension
code --install-extension ms-python.debugpy
code --install-extension ms-python.python
code --install-extension ms-python.vscode-pylance
code --install-extension ms-vscode.makefile-tools
code --install-extension yzhang.markdown-all-in-one
```

(List generated with `code --list-extensions | xargs -L 1 echo code --install-extension`.)

If the command `code` throws an error, open the Command Palette (F1 or ⇧+⌘+P on Mac) and type `shell command` to find the `Shell Command: Install the 'code' command in PATH` command.

After executing the command, restart the terminal for the new `$PATH` value to take effect.

## Optional Aliases

I use these aliases on my Pi's:

``` bash
# Add to ~/.bash_aliases
#
# Random helpers
alias cd..='cd ..' # Get rid of command not found error
alias diff='colordiff' # Colorize diff output
alias ping='ping -c 3' # Stop pings after three

# Do system update/upgrade and cleanup in one shot
alias update='sudo apt update && sudo apt --fix-broken install -y && sudo apt autoremove --purge -y && sudo apt clean && sudo apt autoclean -y && sudo apt upgrade -y'
alias upgrade='update'
```

To install them quickly and activate them in the current shell:

``` bash
curl https://gist.githubusercontent.com/lbussy/23c05d8dc8c24d8d8edddf1d381f1c8b/raw/cee00ee532753d971d64a107b9ca5d58d39f81c6/.bash_aliases -o ~/.bash_aliases
. ~/.bash_aliases
```

## Passwordless (public key) Login to Pi

Remote development is greatly simplified if you exchange keys:

1. Make sure you have a key-pair (e.g. id_ed25519 and id_ed25519.pub) in `~/ssh/`.  If it is not there, generate one with `ssh-keygen`.  You can use this one-liner to check for the existence of a public key (which implies you have a private one) and generate one if not:

 ``` bash
 [ -d ~/.ssh ] && [ -f ~/.ssh/*.pub ] && echo "SSH keys already exists." || ssh-keygen
 ```

2.  Copy the key to your Pi:

  ``` bash
  ssh-copy-id pi@{hostname}.local
  ```

3.  If you want to turn off password authentication on your Pi, edit your `/etc/ssh/sshd_config`, uncomment the line starting with `PasswordAuthentication`, and set the value to `no`.

## Required Libs

### Cleanup Existing Environment and Libs

On an older Pi image, you may want to update the Kernel and Bootloader:

``` bash
sudo apt-get --reinstall install raspberrypi-kernel
sudo apt-get --reinstall install raspberrypi-bootloader
```

I use this series of `apt` commands to clean up the local library situation before installing the required libs.

``` bash
sudo apt-get update
sudo apt-get clean -y
sudo apt-get autoclean -y
sudo apt-get autoremove -y
sudo apt-get --fix-broken install -y
sudo apt-get upgrade -y
```

### Install Required Libs

These are the required libraries for a development environment (assuming you are starting with Raspbian Lite.)

* git
* apache2
* php
* libraspberrypi-dev
* raspberrypi-kernel-headers

``` bash
sudo apt-get install git apache2 php libraspberrypi-dev raspberrypi-kernel-headers -y
```

## Python

Some tools in `./scripts/` are Python, so I use a virtual environment for development.  I have created a `requirements.txt` for any dependencies.

To create the `venv`, from the root of your Pi's WsprryPi repository, execute these commands:

``` bash
python3 -m venv ./.venv
. ./.venv/bin/activate
python -m pip install --upgrade pip
pip install -r ./requirements.txt
```
Your virtual Python environment is installed and activated.  Your prompt will change similar to this:

``` bash
(.venv) pi@wspr:~/WsprryPi $
```
To activate in subsequent sessions:
``` bash
. ./.venv/bin/activate
```

If you use VSCode, it should automatically activate your `venv` when you open the project.  If it prompts you to accept it instead, do so now.

## Repo Tools and Scripts

More information about using the release scripts are in the [RELEASE.md](./scripts/RELEASE.md) document.

* `./scripts/copydocs.sh` - Compile the project Sphinx docs and place them in http://{hostname}.local/wspr/docs/
* `./scripts/copyexe.sh` - After the initial install, this will update the local system with new executable files in `./scripts/`.
* `./scripts/copysite.sh` - After the initial install, this will update the local web UI with files in `./data/`.
* `./scripts/install.sh` - This is the installer the end user will execute.  Your development Pi should have installed WsprryPi once for any executable upgrades during development to apply correctly.
* `./scripts/logrotate.d` - This is the systemd file for the logrotate function.  It copies when you install the application.
* `./scripts/release.py` - This script compiles the wspr executable.  It will also read Git properties for the project name, version (tag), commit, and branch, then use these to update specific file properties.
* `./scripts/shutdown_watch.py` - This script is optionally installed to utilize a shutdown button on physical pin 35/BCM19.  A press will halt the system.
* `./scripts/uninstall.sh` - This script uninstalls the WsprryPi environment.  It is run from the web by a user, however it will work from the repo as well.
* `./scripts/wspr` - This is the primary WsprryPi executable.  It lives here to be installed from the repo when a user performs an installation.  This file is also updated when `release.py` is executed.
* `./scripts/wspr.ini` - This is the default configuration for the WsprryPi executable.  It lives here to be installed from the repo when a user performs an installation.
