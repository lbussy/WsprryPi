# Installing Wsprry Pi

## Gather Hardware

You will need the following:

- A Raspberry Pi:
  - Raspberry Pi 1 B
  - Raspberry Pi 1 B+
  - Raspberry Pi 1 A+
  - Raspberry Pi 2
  - Raspberry Pi Zero
  - Raspberry Pi 3
  - Raspberry Pi Zero W
  - Raspberry Pi 3 B+
  - Raspberry Pi 3 A+
  - Raspberry Pi 4
- An SD card for the OS image
- A power supply for the Pi. Pay attention here to potentially noisy power supplies. You will benefit from a well-regulated supply with sufficient ripple suppression. You may see supply ripple as mixing products centered around the transmit carrier, typically at 100/120Hz.

## Prerequisites

This section may be the most challenging part of the whole installation.  *You must have a working Raspberry Pi with Internet access.*  It can be hard-wired or on Wi-Fi. There is no better place to learn how to set up your new Pi than the people who make it themselves. [Go here](https://www.raspberrypi.com/documentation/computers/getting-started.html), and learn how to install the operating system with the [Raspberry Pi Imager](https://www.raspberrypi.com/software/). To enable SSH access, you can pre-configure your image with your local/time zone, Wi-Fi credentials, and a different hostname.

![Raspberry Pi Imager](rpi_imager.png)

**You MUST use a 32-bit version**, and I am only testing with the current version: Bookworm.

You can use a full-featured desktop version with all the bells and whistles, or wspr will run just fine on the Lite version on an SD card as small as 2 GB (although a minimum of 8 GB seems more comfortable these days.)  You can even run it headless without a keyboard, mouse, or monitor. If you enable SSH, you can use your command line from Windows 10/11, MacOS, or another Pi.

Whatever you do, you will need command line access to your Pi to proceed. Once you are up and running and connected to the Internet, you may proceed with Wsprry Pi installation. Here is a recommended process:

**Open the Raspberry Pi Imager:**

* Choose your Raspberry Pi Device
* Choose your Operating system from one of these:
  * Raspberry Pi OS (32-bit)
  * Raspberry Pi OS (other)
    * Raspberry Pi OS Lite (32-bit)
    * Raspberry Pi OS Full (32-bit)
* Choose Storage (there should be only one SD card inserted)
* Next
* At "Use OS Customizations," select "Edit Settings"
  * On the General Tab
    * Set the hostname to something unique on your network (like `wspr`)
    * Set username and password (you must do this, or SSH will not work) 
      * Only the username `pi` has been tested
      * Choose a password you can remember
    * Configure wireless LAN
      * SSID is your network name
      * The password is whatever you use
      * Select the proper Wireless LAN country
    * Set Locale settings to your location and keyboard type
  * On the Services Tab
    * Check "Enable SSH"
    * Select "Use Password authentication"
  * Save
  * "Yes" to apply customizations
* "Yes" to erase media
* Wait for the writer to tell you it is okay to remove the SD card

## System Changes

Aside from the obvious, installing Wsprry Pi, the install script will do the following:

- **Install Apache2**, a popular open-source, cross-platform web server that is the most popular web server by the numbers. The [Apache Software Foundation](https://www.apache.org/) maintains Apache. Apache is used to control wspr from an easy-to-use web page.
- **Install PHP**, a popular general-purpose scripting language especially suited to web development. The [PHO Group](https://www.php.net/) maintains PHP. I wrote the web pages in PHP.
- **Install Raspberry Pi development libraries**, `libraspberrypi-dev` `raspberrypi-kernel-headers`.
- **Git for Debian**, for no other reason than it is easier for me to develop on a system with Git. Since this is free, you get to put up with me being lazy about this part. :)
- Optionally install support for TAPR's shutdown button.
- Disable the Raspberry Pi's built-in sound card. Wsprry Pi uses the RPi PWM peripheral to time the frequency transitions of the output clock. The Pi's sound system also uses this peripheral; any sound events during a WSPR transmission will interfere with WSPR transmissions.

## Install WSPR

You may use this command to install Wsprry Pi (one line):

`curl -L installwspr.aa0nt.net | sudo bash`

This install command is idempotent; running it additional times will not have any negative impact. If an update is released, re-run the installer to take advantage of the new release.

The first screen will welcome you and give you instructions:

```text
The script will present you with some choices during the installation.
You will most frequently see a 'yes or no' choice, with the
default choice capitalized as so: [y/N]. Default means if
you hit <enter> without typing anything, you will make the
capitalized choice, i.e., hitting <enter> when you see [Y/n]
will default to 'yes.'

Yes/no choices are not case-sensitive. However, passwords,
system names, and install paths are. Be aware of this. There
is generally no difference between 'y', 'yes,' 'YES,' and 'Yes.'

Press any key when you are ready to proceed. 
```

Take special note of default choices. `[Y/n] means that hitting `Enter` will default to "Yes. " If you want to select no, you should choose "N."

The first choice you make will be related to the system time zone and time if you did not set it before flashing the OS:

```text
The time is currently set to Sun 19 Feb 12:05:48 CST 2023.
Is this correct? [Y/n]:
```

Please be sure this is correct. If you select "No," the script will provide you with the Raspbian configuration screens to set this correctly.

The next choice you make will be whether or not to add support for the system shutdown button:

```text
Support system shutdown button (TAPR)? y/N]:
```

This button is included on the TAPR hat and allows one to press it to initiate a clean system shutdown. t is best to shut your Pi down correctly to avoid corrupting the SD card; the button allows that without logging in, typically shutting the Pi down in a few seconds.

Following this choice, the script will install some packages.

When complete, the script displays the final screen:

```text
The WSPR daemon has started.
 - WSPR frontend URL   : http://192.168.1.24/wspr
 -or- : http://wspr.local/wspr
 - Release version     : 0.0.1

Happy DXing!
```

At this point, Wsprry Pi is installed and running.

Note the URL for the configuration UI listed as a `{name}.local` and IP address choice. You can access your system with the `{name}.local` names without remembering the IP address. The `{name}.local` is convenient for automatically assigned IP addresses in most home networks. The IP address of your Raspberry Pi may change over time, but the name will not.

Some Windows systems can use this naming standard without additional work, but others may need Apple's [Bonjour Print Services](https://support.apple.com/kb/dl999) installed to enable this helpful tool. If you get a host hot found error, try installing Bonjour.

Connect to your new web page from your favorite computer or cell phone with the IP address or the `{name}.local` name, and you can begin to set things up. See Operations for more information.

## Additional Hardware

While the TAPR Hat is optional, an antenna is not. Choosing an antenna is beyond the scope of this documentation, but you can use something as simple as a random wire connected to the GPIO4 (GPCLK0) pin, which is numbered 7 on the header.

![Raspberry Pi Pinout](pinout.png)
