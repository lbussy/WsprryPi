# Installing Wsprry Pi

You may use this command to install Wsprry Pi (one line):

`curl -L installwspr.aa0nt.net | sudo bash`

This install command is idempotent; running it additional times will not have any negative impact. If an update is ever released, re-run the installer to take advantage of the new release.

The first screen will welcome you and give you instructions:

```
You will be presented with some choices during the install.
Most frequently you will see a 'yes or no' choice, with the
default choice capitalized like so: [y/N]. Default means if
you hit <enter> without typing anything, you will make the
capitalized choice, i.e. hitting <enter> when you see [Y/n]
will default to 'yes.'

Yes/no choices are not case sensitive. However; passwords,
system names and install paths are. Be aware of this. There
is generally no difference between 'y', 'yes', 'YES', 'Yes'.

Press any key when you are ready to proceed. 
```

Take special note about default choices, `[Y/n]` means that hitting `Enter` will default to "Yes," should you want to select no, you should choose "N."

The first choice you make will be related to the system time zone and time:

```
The time is currently set to Sun 19 Feb 12:05:48 CST 2023.
Is this correct? [Y/n]:
```

Please be sure this is correct. If you select "No," you will be presented with the Raspbian configuration screens to set this correctly.

The next choice you make will be whether or not to add support for the system shutdown button:

```
Support system shutdown button (TAPR)? [y/N]:
```

This button is included on the TAPR hat and allows one to press the button on the hat to initiate a clean system shutdown. A Pi should always be shut down correctly to avoid corrupting the SD card.

Following this choice, some system packages will be installed, including Apache2, an open-source web server.

When complete, the final screen will be displayed:

```
The WSPR daemon has started.
 - WSPR frontend URL   : http://192.168.1.24/wspr
                  -or- : http://wspr.local/wspr
 - Release version     : 0.0.1

Happy DXing!
```

At this point, Wsprry Pi is installed and running.

Note the URL listed for the configuration UI listed as a {name}.local and IP address choice. Using your system name is a Zeroconfiguration service that is enabled by default with Apple products. It allows accessing your home systems with their local names without remembering the IP Address. This is especially handy for automatically-assigned IP addresses found in most home networks. The IP address of your Raspberry Pi may change over time.

Some Windows systems can use this naming standard without additional work; some may need Apple's [Bonjour Print Services](https://support.apple.com/kb/dl999) installed to enable this helpful tool.
