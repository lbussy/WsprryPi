# Wsprry Pi
*A QRP WSPR transmitter leveraging a Raspberry Pi*

Pronounced: (Whispery Pi)

[![Documentation Status](https://readthedocs.org/projects/wsprry-pi/badge/?version=latest)](https://wsprry-pi.readthedocs.io/en/latest/?badge=latest)

This was a fork of [threeme3](https://github.com/threeme3)/[WsprryPi](https://github.com/threeme3/WsprryPi) inspired by a conversation with Bruce Raymond of [TAPR](https://tapr.org/).  It has been updated, improved, and documented for ease of use.  Since this fork has diverged significantly, and I want to make it easy on myself, attribution remains, but the fork has been detached.

Please see [the documentation](https://wsprry-pi.readthedocs.io/en/latest/) for background, installation, and use.

If you really want to get started and not read anything, here is how to install it:

```
curl -L installwspr.aa0nt.net | sudo bash
```

## Developer Notes

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
