# To Do

- Fix shutdown-watch
- Create reloease script orchestration
- Fix LED
- Fix freq change ()
- Run lint
- Put version on web page (version.php)

## Stuff to Remember

### Branches:

- Working on update_installer
- It will go back into refactoring

### Stuff to do:

- Review the wsprrypi.service file
- See if we can include libs in compile
- If in daemon mode, do not suppress SIGINT
- Inline the apache_tool into install.
- Need to lint and unit test main
- Need to add unit tests for mailbox
- Do we need a venv for shutdown_watch.py
- Figure out what happens if I add sections or items to the INI
- Move shutdown watch to C++
- Add --debug for runtime debugging
- Log rotate skipping (newer) (dafuq did I mean with this?)
- Implement get_version in installer
- Update release.py to update versions, branches, etc.
- Add notes about gh:
    ``` bash
    $ gh auth login
    ? What account do you want to log into? GitHub.com
    ? What is your preferred protocol for Git operations? HTTPS
    ? Authenticate Git with your GitHub credentials? Yes
    ? How would you like to authenticate GitHub CLI? Login with a web browser

    ! First copy your one-time code: XXXX-XXXX
    Press Enter to open github.com in your browser...
    ✓ Authentication complete.
    - gh config set -h github.com git_protocol https
    ✓ Configured git protocol
    ✓ Logged in as foo
    ```
- See if we can implement something like daemonize instead of Singleton
- Add a .wsprrypi-alias file and source it from the .bash_aliases
- MOTD?
    ``` bash
    Linux wpsd 6.1.0-rpi7-rpi-v7 #1 SMP Raspbian 1:6.1.63-1+rpt1 (2023-11-24) armv7l
    _      _____  _______
    | | /| / / _ \/ __/ _ \
    | |/ |/ / ___/\ \/ // /
    |__/|__/_/  /___/____/

    Version Status
    ---------------
    • WPSD Digital Voice Dashboard Software:
        Ver. # 0f0841942a
    • WPSD Support Utilites and Programs:
        Ver. # 8382226eec
    • WPSD Digital Voice and Related Binaries:
        Ver. # 80c2f986cf

    [?] Your WPSD dashboard can be accesed from:
        • http://wpsd.local/
        • http://wpsd/
        • http://10.0.0.38/

    [i] WPSD command-line tools are all prefixed with "wpsd-".
        Simply type wpsd- and then the TAB key twice to see a list.

    pi-star@wpsd:~
    ```

### Fis in this iteration

- Move copying ini file before the wsprrypi daemon restarts (called "configuration file")
- logrotate.d must be called 'wsprrypi' and put it in /etc/logrotate.d/
- Fix this:  2024-11-27 22:16:15 GMT Do not use NTP sync: false

### Nice to Do

- Make a log viewing page
- No need to disable_sound() on Pi 5 and up (install and uninstall)
- Move LED to differnet code with pigpio.h
- Implement freq_test

## Previous Issues

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
