# Developer Notes

I use VS Code to develop this environment, and connect my workstation to my Pi via the [VS Code Remote Development](https://code.visualstudio.com/docs/remote/remote-overview) option.

## Required Libs

### Cleanup Existing Environment and Libs

On an older Pi, you may want to cleanup/update the Kernel and Bootloader:

``` bash
sudo apt-get --reinstall install raspberrypi-kernel
sudo apt-get --reinstall install raspberrypi-bootloader
```

I use this series of `apt` commands to clean up the local library situation before installing required libs.

``` bash
sudo apt-get update
sudo apt-get clean -y
sudo apt-get autoclean -y
sudo apt-get autoremove -y
sudo apt-get --fix-broken install -y
sudo apt-get upgrade -y
```

### Install Required Libs

These are the required libraries for a development environment (assuming you are starting with )

* apache2
* php
* libraspberrypi-bin
* libraspberrypi-doc
* libraspberrypi-dev
* raspberrypi-kernel-headers

sudo apt-get install --reinstall apache2 php libraspberrypi-bin libraspberrypi-doc libraspberrypi-dev raspberrypi-kernel-headers -y

## Python

Some of the tools in `./scripts/` are Python, so I have taken to using a venv for development.  While I have created a `requirements.txt` for the venv, so far I have not had to change the envirironment.  Shoudl this change, the requirements file will be the source of truth.

To create the venv, from the root of your local repo:
``` bash
python3 -m venv ./venv
. ./.venv/bin/activate
pip install -r ./requirements.txt
```
You should now have a venv installed, and activated.  Your prompt will change to:
``` bash
(.venv) pi@wspr:~/WsprryPi $
```
To activate in subsequent sessions:
``` bash
. ./.venv/bin/activate
```

If you use VSCode, it should automatically activate your venv when you open the project.
