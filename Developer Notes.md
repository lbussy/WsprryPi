<!-- omit in toc -->
# Developer Notes

I use VS Code installed on my working laptop (Windows or Mac) and the [Visual Studio Code Remote—SSH](https://code.visualstudio.com/docs/remote/ssh) extension to access VS Code's feature set on a Raspberry Pi from a familiar Dev UI on my laptop.

- [Set up SSH to your PI](#set-up-ssh-to-your-pi)
- [Optional Housekeeping](#optional-housekeeping)
  - [Aliases Deployed](#aliases-deployed)
  - [Install Helpful Packages](#install-helpful-packages)
  - [Do It All](#do-it-all)
- [VS Code](#vs-code)
- [Required Libs](#required-libs)
- [Reboot](#reboot)
- [Working with the Project](#working-with-the-project)

## Set up SSH to your PI

Any references to `{hostname}` should be replaced with the hostname of your target Pi.

1. If you are on Windows, have [Open SSH](https://windowsloop.com/install-openssh-server-windows-11/) installed.
   
2. Check that you have an SSH key generated on your system:

   * Linux or Mac:

        ``` bash
        [ -d ~/.ssh ] && [ -f ~/.ssh/*.pub ] && echo "SSH keys already exists." || ssh-keygen
        ```

   * Windows PowerShell:

        ``` PowerShell
        if (Test-Path "$env:USERPROFILE\.ssh" -and (Test-Path "$env:USERPROFILE\.ssh\*.pub")) {
            Write-Host "SSH keys already exist."
        } else {
            ssh-keygen
        }
        ```

   * Windows Command Line:

        ``` cmd
        @echo off
        if exist "%USERPROFILE%\\.ssh" (
            if exist "%USERPROFILE%\.ssh\*.pub" (
                echo SSH keys already exist.
            ) else (
                ssh-keygen
            )
        ) else (
            ssh-keygen
        )
        ```

3. `ssh` to your `pi@{hostname}.local` with the target host password to ensure your `ssh` client and name resolution via zeroconf or mDNS. If you see:

    ```
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    @    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    ```

   * Edit `~/.ssh/known_hosts` and remove any lines beginning with your target hostname
   * "Yes" to a prompt to continue connecting
   * Exit back out

4. Copy keys to host with (enter target host password when asked):
   
    ``` bash
    ssh-copy-id pi@{hostname}.local
    ```

5. Edit `~/.ssh/config` (or `$HOME\.ssh` on Windows) and add a stanza like this - be sure to mind the indentation:

    ``` bash
        Host {hostname}.local
            HostName {hostname}.local
            User pi
            Port 22
            PreferredAuthentications publickey
    ```

6. `ssh` to pi@{hostname}.local to ensure your changes allow key exchange (passwordless) logins.

## Optional Housekeeping

I share some `bash` aliases [here](https://gist.github.com/lbussy/23c05d8dc8c24d8d8edddf1d381f1c8b) that help me when I work on a *nix system..

Scroll down to the [Do It All](#do-it-all) section to find the command to set all this up.

### Aliases Deployed

1. **`alias cd..='cd ..'`**:
   - This alias creates a shorthand for the `cd ..` command, which moves up one directory in the filesystem. Typing `cd..` normally results in a "command not found" error, but this alias resolves that by making `cd..` function as `cd ..`. I typo that often.

2. **`alias diff='colordiff'`**:
   - This alias replaces the `diff` command with `colordiff`. The `colordiff` command is a colorized version of `diff`, which makes it easier to compare files, highlighting differences using color visually.

3. **`alias ping='ping -c 3'`**:
   - This alias modifies the `ping` command to send only three packets by default (`ping -c 3`), stopping after three pings instead of running indefinitely. I find this useful for quick network checks without stopping the command manually.

4. **`alias update='sudo apt update && sudo apt --fix-broken install -y && sudo apt autoremove --purge -y && sudo apt autoclean -y && sudo apt full-upgrade -y && sudo deborphan | xargs sudo apt-get -y remove --purge'`**:
   - This alias combines multiple package management tasks into one command, making it easier to keep the system updated and cleaned up:
     - `sudo apt update`: Updates the list of available packages.
     - `sudo apt --fix-broken install -y`: Fixes any broken package dependencies.
     - `sudo apt autoremove --purge -y`: Removes unnecessary packages and their configuration files.
     - `sudo apt autoclean -y': Removes outdated package files from the cache.
     - `sudo apt full-upgrade -y': Upgrades all installed packages, possibly installing or removing packages to satisfy dependencies.
     - `sudo deborphan | xargs sudo apt-get -y remove --purge`: Removes orphaned packages (unused libraries and dependencies) that are no longer needed.

5. **`alias upgrade='update'`**:
   - This alias makes `upgrade` a shortcut for the `update` alias. Essentially, when you type `upgrade`, it will run the same commands as `update`. It allows users to perform the full update and cleanup process using a more intuitive command (`upgrade` instead of `update`).

### Install Helpful Packages

* `colordiff`: A colorized version of the diff command, making it easier to compare file differences with color highlighting visually.

* `deborphan`: A tool to find orphaned packages (i.e., libraries or dependencies no longer needed by any other installed package) and allows you to remove them.

### Do It All

To handle all of the above aliasing and package installs, paste in this command:

``` bash
curl https://gist.githubusercontent.com/lbussy/23c05d8dc8c24d8d8edddf1d381f1c8b/raw/2fb062a9ab7b374b72674775971ca699da901afb/install_aliases.sh -o ~/.bash_aliases
. ~/.bash_aliases
```

It will:

* Add the above described aliases to your `~/.bash_aliases`

* Source (add to the environment) the new aliases.

* Issue the aliased command `update` to update all installed software and clean the local repository.
  
* It will install `colordiff` and `deborphan` that are in the aliases.

## VS Code

I use VS Code to develop this environment and connect my workstation to my Pi via the [VS Code Remote Development](https://code.visualstudio.com/docs/remote/remote-overview) option. This tool makes compiling and testing on the Pi very easy as I work from my laptop.

If you are going to use VS Code from your workstation:

1. In VS Code, install the "Remote Development" extension.

2. View -> Command Pallete -> >Remote-SSH:Connect Current Window to Host

3. Select or enter your {hostname}.local

4. The local VS Code engine will install the VS Code Server on the Pi.

5. Once done and the terminal screen in VS Code is connected to the Pi:

Either:

        ``` bash
        sudo apt install git -y
        git clone https://github.com/lbussy/WsprryPi.git
        cd ~/WsprryPi/
        sudo ./scripts/install.sh -l
        ```

(This will allow cloning git first, which you need anyway, then installing, which gets the rest of the libs.)

Or:

        ``` bash
        curl -L installwspr.aa0nt.net | sudo bash
        git clone https://github.com/lbussy/WsprryPi.git
        cd ~/WsprryPi/
        ```
  
(This lets the installer install everything, but then you clone the repo after.)

1. You should be in your git repo directory.  Set up the Git global environment.  Replace placeholders with your Git username and email:

    ``` bash
    git config --global user.email "you@example.com"
    git config --global user.name "Your Name"
    ```

2. These are the extensions I use. Paste these commands in the terminal window one by one. When I paste them all at once, the system seems to hang:

    ``` bash
    code --install-extension ms-python.debugpy
    code --install-extension ms-python.python
    code --install-extension ms-python.vscode-pylance
    code --install-extension ms-vscode.cmake-tools
    code --install-extension ms-vscode.cpptools
    code --install-extension ms-vscode.cpptools-extension-pack
    code --install-extension ms-vscode.cpptools-themes
    code --install-extension ms-vscode.makefile-tools
    code --install-extension twxs.cmake
    code --install-extension yzhang.markdown-all-in-one
    ```

(List generated with `code --list-extensions | xargs -L 1 echo code --install-extension`.)

8. Set up the `venv`:

    ``` bash
    python3 -m venv ./.venv
    . ./.venv/bin/activate
    python -m pip install --upgrade pip
    pip install -r ./requirements.txt
    ```

9. For Python script and document development, always use `venv` (VS Code should do this or prompt you select it).

10. Use the "Open Folder" button and select the root of your your repo on the Pi.

11. Do great things. You are now using VS Code on your Pi; all compilation and execution happens there.

Keep in mind that the **Wsprry Pi** and optional **Shutdown Watch** systemd daemons are running at this point.  If oyu will be executing from your dev environment you may receive an error that `wspr` is already running.  You can stop and disable these with:

``` bash
sudo systemctl stop wspr
sudo systemctl disable wspr
sudo systemctl stop shutdown_watch
sudo systemctl disable shutdown_watch
```

## Required Libs

If you did not run `install.sh` from within the Wsprry Pi repo or with the WsprryPi curl command, you will need some libs to compile the project:

* git
* apache2
* php
* libraspberrypi-dev
* raspberrypi-kernel-headers

Install these with:

``` bash
sudo apt-get install git apache2 php libraspberrypi-dev raspberrypi-kernel-headers -y
```

## Reboot

The installer blacklists the onboard snd_bcm2835 device as this is used for generating the signal.  You will need a reboot at some point before expecting Wsprry Pi to work corectly.

## Working with the Project

See [RELEASE.md](./scripts/RELEASE.md) for information about the project development tools.
