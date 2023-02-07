# Installing Wsprry Pi

To install Wsprry Pi, please execute this command (one line):

`curl -L https://raw.githubusercontent.com/lbussy/WsprryPi/{$BRANCH}/scripts/bootstrap.sh | sudo bash`

Where:

`{$BRANCH}` = The branch to install.  In most cases this will be `main` as in:

`curl -L https://raw.githubusercontent.com/lbussy/WsprryPi/main/scripts/bootstrap.sh | sudo bash`

If you do not use `sudo bash` at the end of this install command, the script will automatically re-launch itself using `sudo`.  If you find this offensive you likely know enough to perform these tasks without the script.

## Install Process

The script will perform the following in order:

1. Begin logging the installation in `~/bootstrap.log`
1. Make sure the script is running as root; relaunch if not
1. Show instructions
1. Check if Wsprry Pi is already installed
1. Set the timezone if not already set
1. Install necessary packages
1. Install service entries
1. Start Services
1. Provide final instructions
