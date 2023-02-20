# Installing Wsprry Pi

## Gather Hardware

You will need the following:

- A Raspberry Pi of any type
- An SD card for the OS image
- A power supply for the Pi. Pay attention here to potentially noisy power supplies. You will benefit from a well-regulated supply with sufficient ripple suppression. You may see supply ripple as mixing products centered around the transmit carrier, typically at 100/120Hz.

## Prerequisites

This may be the most challenging part of the whole installation. You must have a working Raspberry Pi with Internet access. It can be hard-wired or on Wi-Fi. There is no better place to learn how to set up your new Pi than the people who make it themselves. [Go here](https://www.raspberrypi.com/documentation/computers/getting-started.html), and learn how to install the operating system with the [Raspberry Pi Imager](https://www.raspberrypi.com/software/). You can pre-config your image with your local/time zone, Wi-Fi credentials, and a different hostname and enable SSH access from the little cog wheel on the Imager's control page.

![Raspberry Pi Imager](rpi_imager.png)

Select any Raspbian image you like. You can use a full-featured desktop version with all the bells and whistles, or wspr will run just fine on the lite version on an SD card as small as 2 GB (although a minimum of 8 GB seems to be more comfortable these days.)  If you use the cog wheel to configure your Pi first, as described above, you can even run it headless without a keyboard, mouse, or monitor. Provided you enabled SSH, you can use your command line from Windows 10/11, MacOS, or another Pi.

Whatever you do, you will need command line access to your Pi to proceed. Once you are up and running and connected to the Internet, you may proceed with Wsprry Pi installation.

## System Changes

Aside from the obvious installation of Wsprry Pi, the install script will do the following:

- Install Apache2, a popular open-source, cross-platform web server that is the most popular web server by the numbers. The [Apache Software Foundation](https://www.apache.org/) maintains Apache. Apache is used to control wspr from an easy-to-use web page.
- Optionally install support for TAPR's shutdown button.
- Disable the Raspberry Pi's built-in sound card. Wsprry Pi uses the RPi PWM peripheral to time the frequency transitions of the output clock. The Pi's sound system also uses this peripheral; any sound events during a WSPR transmission will interfere with WSPR transmissions.

## Install WSPR

You may use this command to install Wsprry Pi (one line):

`curl -L installwspr.aa0nt.net | sudo bash`

This install command is idempotent; running it additional times will not have any negative impact. If an update is released, re-run the installer to take advantage of the new release.

The first screen will welcome you and give you instructions:

```text
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

```text
The time is currently set to Sun 19 Feb 12:05:48 CST 2023.
Is this correct? [Y/n]:
```

Please be sure this is correct. If you select "No," the script will provide you with the Raspbian configuration screens to set this correctly.

The next choice you make will be whether or not to add support for the system shutdown button:

```text
Support system shutdown button (TAPR)? [y/N]:
```

This button is included on the TAPR hat and allows one to press the button on the hat to initiate a clean system shutdown. It is best to shut your Pi down correctly to avoid corrupting the SD card; the button allows that without logging in, typically shutting the Pi down in a few seconds.

Following this choice, some system packages, including Apache2, an open-source web server, will be installed.

When complete, the script displays the final screen:

```text
The WSPR daemon has started.
 - WSPR frontend URL   : http://192.168.1.24/wspr
                  -or- : http://wspr.local/wspr
 - Release version     : 0.0.1

Happy DXing!
```

At this point, Wsprry Pi is installed and running.

Note the URL for the configuration UI listed as a {name}.local and IP address choice. Using your system name is a Zeroconfiguration service capability enabled by default with Apple products. It allows accessing your home systems with their local names without remembering the IP Address. The .local name is convenient for automatically-assigned IP addresses in most home networks. The IP address of your Raspberry Pi may change over time, but the name will not.

Some Windows systems can use this naming standard without additional work; some may need Apple's [Bonjour Print Services](https://support.apple.com/kb/dl999) installed to enable this helpful tool.

Connect to your new web page from your favorite computer or cell phone with the IP address or the {hostname}.local name, and you can begin to set things up. See Operations for more information.

## Additional Hardware

While the TAPR Hat is optional, an antenna is not. Choosing an antenna is beyond the scope of this documentation, but you can use something as simple as a random wire connected to the GPIO4 (GPCLK0) pin, which is numbered 7 on the header.

![Raspberry Pi Pinout](pinout.png)
