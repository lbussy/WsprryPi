# To Do

## Stuff to Remember

- Make a log viewing page
- Do we need a venv for shutdown_watch.py
- Suggest to clone git repo after required libs (devel notes)
- No need to disable_sound() on Pi 5 and up (install and uninstall)
- Put in a redirect to /wspr if nothing is in web root.
- Documentation says `wspr [options] --test-tone f` but it should be more
clear that `f` is a frequency:  `wspr --test-tone 780e3

# Previous Issues

*These are from the original distro and I am not sure they are still valid.*

1. Two users were reporting that the program never stops transmitting, even
when intervals for disabled tx are programmed. The problem was in both
cases fixed by flashing a new image on the SD card with a freshly downloaded
image: 2013-02-09-wheezy-raspbian.zip. No apt-get upgrade or firmware
upgrade was performed. After this WsprryPi TX was running successfully.

1. One user reported his RPi died while in WsprryPi service caused by excessive
RF voltage (90V) on GPIO4 created by a 100 watts AM transmitter 50ft away
from the antenna. After the damage exessive current was consumed by RPi (1.1A
from 5V supply), caused by short-circuiting in the 3.3V logic of the BCM2835
SOC. On his replacement RPi, he is planning to add galvanic isolation and
buffering.

1. If running from the console, recent versions of Jessie cause WsprryPi to
crash when the console screen blanks. The symptom is that WsppryPi works
for several transmissions and then crashes. The fix is to add "consoleblank=0"
to /boot/cmdline.txt.
https://github.com/JamesP6000/WsprryPi/issues/10
