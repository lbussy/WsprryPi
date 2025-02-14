# To Do

- Clean up config - it's rredundant now in main
- Save config INI on setter in cconfig - allow saving when we change via the server.  MAke sure it is threadsafe.
- See if we can thread the transmit, or if it's threaded.  Check threadsave things.
- Merge Singleton port with server port
- Fix shutdown-watch
- Create release script orchestration
- Fix LED
- Fix freq change (#57)
- Show current time on web page (#63)
- Show logs on web page (#27)
- Allow reboots (#62)
- Show transission indicator (#64)
- Run lint
- Put version on web page (#97)
- Unable to instal when lib versions differ (#92)
- Put spots on web page: https://www.wsprnet.org/olddb?mode=html&band=all&limit=50&findcall=AA0NT&sort=date

## Stuff to Remember

### Branches:

- Working on update_installer
- It will go back into refactoring

### Stuff to do:

- See if we can include libs in compile
- If in daemon mode, do not suppress SIGINT
- Inline the apache_tool into install.
- Need to lint and unit test main
- Need to add unit tests for mailbox
- Do we need a venv for shutdown_watch.py
- Move shutdown watch to C++
- Add --debug for runtime debugging
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


### Nice to Do

- Make a log viewing page
- No need to disable_sound() on Pi 5 and up (install and uninstall)
- Move LED to differnet code with pigpio.h
- Implement freq_test

## Previous Issues

1. If running from the console, recent versions of Jessie cause WsprryPi to
crash when the console screen blanks. The symptom is that WsppryPi works
for several transmissions and then crashes. The fix is to add "consoleblank=0"
to /boot/cmdline.txt.
https://github.com/JamesP6000/WsprryPi/issues/10
