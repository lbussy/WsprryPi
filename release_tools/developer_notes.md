<!-- omit in toc -->
# Developer Notes

I use VS Code installed on my working laptop (Windows or Mac) and the [Visual Studio Code Remote—SSH](https://code.visualstudio.com/docs/remote/ssh) extension to access VS Code's feature set on a Raspberry Pi from a familiar Dev UI on my laptop.

- [Set up SSH to your PI](#set-up-ssh-to-your-pi)
- [Optional Housekeeping](#optional-housekeeping)
- [VS Code](#vs-code)
- [Required Libs](#required-libs)
- [Reboot](#reboot)
- [Working with the Project](#working-with-the-project)

## Set up SSH to your PI

Any references to `{hostname}` should be replaced with the hostname of your target Pi.

1. If you are on Windows, have [Open SSH](https://windowsloop.com/install-openssh-server-windows-11/) installed.
   
2. Check that you have an SSH key generated on your system:

   * Linux or Mac (one line):

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

To handle all of these aliases and package installs, paste in this command:

``` bash
curl -fsSL https://gist.githubusercontent.com/lbussy/23c05d8dc8c24d8d8edddf1d381f1c8b/raw/install_aliases.sh | bash
```

See the [Gist](https://gist.github.com/lbussy/23c05d8dc8c24d8d8edddf1d381f1c8b) for more info if you have never used these.

## VS Code

I use VS Code to develop this environment and connect my workstation to my Pi via the [VS Code Remote Development](https://code.visualstudio.com/docs/remote/remote-overview) option. This tool makes compiling and testing on the Pi very easy as I work from my laptop.

If you are going to use VS Code from your workstation:

1. In VS Code, install the "Remote Development" extension.

2. View -> Command Pallete -> >Remote-SSH:Connect Current Window to Host

3. Select or enter your {hostname}.local

4. The local VS Code engine will install the VS Code Server on the Pi.

5. Once done and you have connected the terminal screen in VS Code to the Pi:

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

6. You should be in your git repo directory. Set up the Git global environment. Replace placeholders with your Git username and email:

   ``` bash
   git config --global user.email "you@example.com"
   git config --global user.name "Your Name"
   ```

7. These are the extensions I use. Paste these commands in the terminal window one by one. When I paste them all at once, the system seems to hang:

   ``` bash
   # Extensions installed on SSH: wspr.local:
   code --install-extension ms-python.debugpy
   code --install-extension ms-python.python
   code --install-extension ms-python.vscode-pylance
   code --install-extension ms-vscode.cpptools
   code --install-extension ms-vscode.cpptools-extension-pack
   code --install-extension ms-vscode.cpptools-themes
   code --install-extension ms-vscode.makefile-tools
   code --install-extension yzhang.markdown-all-in-one
   ```

   (List generated with `code --list-extensions | xargs -L 1 echo code --install-extension`.)

8. Set up the `venv`:

   ``` bash
   python3 -m venv ./.venv
   source ./.venv/bin/activate
   python -m pip install --upgrade pip
   pip install -r ./requirements.txt
   ```

9. For Python script and document development, always use `venv` (VS Code should do this or prompt you to select it).

10. Use the "Open Folder" button and select the root of your repo on the Pi.

11. Do great things. You are now using VS Code on your Pi; all compilation and execution happens there.

Remember that the **Wsprry Pi** and optional **Shutdown Watch** systemd daemons are running. If you are executing from your dev environment, you may receive an error that says `wspr` is already running. You can stop and deactivate these with:

``` bash
sudo systemctl stop wspr
sudo systemctl disable wspr
sudo systemctl stop shutdown_watch
sudo systemctl disable shutdown_watch
```

## Required Libs

If you did not run `install.sh` from within the Wsprry Pi repo or with the WsprryPi curl command, you would need some libs to compile the project:

* git
* gh
* jq
* apache2
* php
* colordiff
* libraspberrypi-dev
* raspberrypi-kernel-headers

Install these with:

``` bash
sudo apt-get install git gh jq apache2 php colordiff libraspberrypi-dev raspberrypi-kernel-headers -y
```

## Reboot

The installer blacklists the onboard snd_bcm2835 device, as Wsprry Pi uses this for generating the signal. You will need a reboot at some point before expecting Wsprry Pi to work correctly.

## Working with the Project

See [release.md](./scripts/release.md) for information about the project development tools.
