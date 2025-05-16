<!-- Grammar and spelling checked -->
# Command Line Operations

## `systemd` Service

The Wsprry Pi executable, wsprrypi, is controlled by the Linux `systemd` controller.  It will run in the background as soon as your Pi starts up.  It is a singleton application by design, meaning only one `wsprrypi` process may be running.  You must stop the daemon if you desire to have some manual control for testing or other reasons.  Here are some commands you may use:

- `sudo systemctl status wsprrypi`: Show a status page for the running daemon.
- `sudo systemctl restart wsprrypi`: Restart the daemon and wsprrypi with it.
- `sudo systemctl stop wsprrypi`: Stop the daemon.  The daemon will restart again upon reboot.
- `sudo systemctl start wsprrypi`: Start the daemon if it is not running.
- `sudo systemctl disable wsprrypi`: Disable the daemon from restarting on reboot.
- `sudo systemctl enable wsprrypi`: Enable the daemon to start on reboot if it is disabled.

## Command Line

To run wsprrypi from the command line, a terse listing of command line options is available by executing `(sudo) wsprrypi -h`:

```text
$ wsprrypi -h

WsprryPi version 2.0 (main).

Usage:
 (sudo) wsprrypi [options] callsign gridsquare transmit_power frequency <f2> <f3> ...
 OR
 (sudo) wsprrypi --test-tone {frequency}

Options:
 -h, --help
 Display this help message.
 -v, --version
 Show the WsprryPi version.
 -i, --ini-file <file>
 Load parameters from an INI file.  Provide the path and filename.

See the documentation for a complete list of available options.
```

### Common Command Line Examples

Either transmit a tone, or you must, at a minimum, supply your callsign, grid square, transmit power, and frequency.  Remember that anything that creates a transmission will require you to use `sudo`.  Arguments may use the short form with a single hyphen (e.g., `-h') or the long form with a double hyphen (e.g. --help`).  Here are some common command-line scenarios:

`wsprrypi --test-tone 780e3`

You may transmit a constant tone at a specific frequency for testing.  In this example, wsprrypi will send a tone at 780 kHz (780000 Hz):

`wsprrypi --help`

Display a brief help screen.

`wsprrypi --test-tone 780e3`

Transmit a constant test tone at 780 kHz.

`wsprrypi N9NNN EM10 33 20m`

Using callsign N9NNN, locator EM10, and TX power 33 dBm, transmit a single WSPR transmission on the 20m band using no frequency offset calibration.

`wsprrypi --use-ntp N9NNN EM10 33 20m`

The same as above, but with NTP calibration.

`wsprrypi --repeat --terminate 7 --ppm 43.17 N9NNN EM10 33 10140210 0 0 0 0`

Transmit a WSPR transmission slightly off-center on 30m every 10 minutes for seven transmissions, using a fixed PPM correction value.

`wsprrypi --repeat --offset --use-ntp N9NNN EM10 33 40m`

Transmit repeatedly on 40m, use NTP-based frequency offset calibration, and add a random frequency offset to each transmission to minimize collisions with other transmitters.

### Complete Command Line Listing

Commands are positional, commands with no arguments, and commands requiring arguments.  Additionally, the configuration may be handled via the INI file, and the path passed to the executable at runtime with the following:

`wsprrypi -i /usr/local/etc/wsprrypi.ini`

No additional command line arguments are necessary if the required positional parameters are present and correct in the INI file.  If you use an INI file and additional command line options, the command line options will apply AFTER the INI options, overwriting them.  In some scenarios, the INI file will be overwritten with your updated parameters.

#### Positional Arguments

Four positional arguments exist (with no `-x or --xxxx`).  These are required for WSPR transmissions (or must be supplied via the INI):

- **Callsign** - Your six-character or less callsign.
- **Gridsquare** - Your four-character maidenhead grid square
- **Power** - Your transmit power in dBm
- **Frequency** (list) - The frequency or list of frequencies, space separated for transmission

Example:

`wsprrypi N9NNN EM10 33 10140210 0 0 0 0`

#### No Arguments

The following commands require no arguments:

- `--help` or `-h`: This command shows the version and an abbreviated help listing, then exits.
- `--version` or `-v`: Shows the current version, then exits.
- `--use-ntp` or `-n`: Uses Network Time Protocol via `chrony` to adjust transmission frequency calibration.
- `--repeat` or `-r`: Repeats the frequency or loops through the list of frequencies indefinitely.
- `--offset` or `-o`: Uses a random offset on the transmission frequency.
- `--date-time-log` or `-D`: Applies a date/time stamp in UTC (e.g. `2025-05-06 12:17:00.561 UTC `) to the log displayed on screen.

#### Arguments Required

These commands require an argument immediately following the command:

- `--ini-file` or `-i`: Pulls initial configuration from an ini file, use the full or relative path.
- `--ppm` or` -p`: Applies a fixed PPM correnction value, -200.00 to 200.00.
- `--terminate` or `-x`: Terminates after x iterations (either a single frequency or a list of frequencies).
- `--test-tone` or `-t`: A test tone will be generated at the chosen frequency, this command overrides the need for the required positional arguments.
- `--led_pin` or `-l`: The Raspberry Pi pin number, in BCM formatting (e.g. use "18" for GPIO18 or header pin 12), to use for the transmission indicator.
- `--shutdown_button` or `-s`: The Raspberry Pi pin number, in BCM formatting (e.g., use "19" for GPIO19 or header pin 35), to use for the shutdown button.
- `--power_level` or `-d`: A value, 0-7, to set the Raspberry Pi GPIO output power:
  - `0`: 2mA or 3.0dBm
  - `1`: 4mA or 6.0dBm
  - `2`: 6mA or 7.8dBm
  - `3`: 8mA or 9.0dBm
  - `4`: 10mA or 10.0dBm
  - `5`: 12mA or 10.8dBm
  - `6`: 14mA or 11.5dBm
  - `7`: 16mA or 12.0dBm
- `--web-port `or `-w`: The socket on which the REST API for configuration runs.  This is hard-coded in the GUI JavaScript, so only change this if you know what you are doing.  The default is port 31415.
- `--socket-port` or `-k`: The socket on which the Web Socket server for UI communication runs.  This is hard-coded in the GUI JavaScript, so only change this if you know what you are doing.  The default is port 31416.
<!-- Not yet implemented - `--transmit-pin` or `-a`: The pin Raspberry Pi pin number, in BCM formatting (e.g., use "18" for GPIO4 or header pin 7) for the transmissions. -->
