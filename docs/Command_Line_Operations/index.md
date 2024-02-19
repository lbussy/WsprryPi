# Command Line Operations

The Wsprry Pi executable, wspr, is controlled by the Linux `systemd` controller. It will run in the background as soon as your Pi starts up. It is a singleton application by design, meaning only one `wspr` process may be running. You must stop the daemon if you desire to have some manual control for testing or other reasons. Here are some commands you may use:

- `sudo systemctl status wspr`: Show a status page for the running daemon.
- `sudo systemctl restart wspr`: Restart the daemon and wspr with it.
- `sudo systemctl stop wspr`: Stop the daemon. The daemon will restart again upon reboot.
- `sudo systemctl start wspr`: Start the daemon if it is not running.
- `sudo systemctl disable wspr`: Disable the daemon from restarting on reboot.
- `sudo systemctl enable wspr`: Enable the daemon to start on reboot if it is disabled.

You will control the shutdown button monitor daemon like `wspr`, substituting `shutdown-button` for `wspr` above.

To run wspr from the command line, a complete listing of command line options is available by executing `(sudo) /usr/local/bin/wspr -h`:

```text
Usage:
  wspr [options] callsign locator tx_pwr_dBm f1 <f2> <f3> ...
    OR
  wspr [options] --test-tone f

Options:
  -h --help
    Print out this help screen.
  -v --version
    Show the Wsprry Pi version.
  -p --ppm ppm
    Known PPM correction to 19.2MHz RPi nominal crystal frequency.
  -s --self-calibration
    Check NTP before every transmission to obtain the PPM error of the
    crystal (default setting.)
  -f --free-running
    Do not use NTP to correct the frequency error of RPi crystal.
  -r --repeat
    Repeatedly and in order, transmit on all the specified command line
    freqs.
  -x --terminate <n>
    Terminate after completing <n> transmissions.
  -o --offset
    Add a random frequency offset to each transmission:
      +/- 80Hz for WSPR
      +/- 8Hz for WSPR-15
  -t --test-tone freq
    Output a test tone at the specified frequency. Only used for
    debugging and verifying calibration.
  -l --led
    Use LED as a transmit indicator (TAPR board).
  -n --no-delay;
    Transmit immediately without waiting for a WSPR TX window. Used for
    testing only.
  -i --ini-file
    Load parameters from an ini file. Supply path and file name.
  -D --daemon-mode
    Run with terse messaging.
  -d --power_level
    Set actual TX power, 0-7.

Frequencies can be specified either as an absolute TX carrier frequency,
or using one of the following bands:

  LF, LF-15, MF, MF-15, 160m, 160m-15, 80m, 60m, 40m, 30m, 20m,
  17m, 15m, 12m, 10m, 6m, 4m, and 2m

If you specify a band, the transmission will happen in the middle of the
WSPR region of the selected band.

The "-15" suffix indicates the WSPR-15 region of the band.

You may create transmission gaps by specifying a TX frequency of 0.
```

### Command Line Entries for Testing

You may transmit a constant tone at a specific frequency for testing. In this example, wspr will send a tone at 780 kHz (780000 Hz):

`wspr --test-tone 780e3`

### Example Usage

Remember that anything that creates a transmission will require you to use `sudo`.

`wspr --help`

Display a brief help screen.

`wspr --test-tone 780e3`

Transmit a constant test tone at 780 kHz.

`wspr N9NNN EM10 33 20m`

Using callsign N9NNN, locator EM10, and TX power 33 dBm, transmit a single WSPR transmission on the 20m band using NTP-based frequency offset calibration.

`wspr --led N9NNN EM10 33 20m`

The same as above but using the red LED on transmit (TAPR Hat) to indicate an active transmission. The LED is connected to Pin 12 (GPIO18, BCM18).

`wspr --free-running N9NNN EM10 33 20m`

The same as above, but without NTP calibration.

`wspr --repeat --terminate 7 --ppm 43.17 N9NNN EM10 33 10140210 0 0 0 0`

Transmit a WSPR transmission slightly off-center on 30m every 10 minutes for seven transmissions, using a fixed PPM correction value.

`wspr --repeat --offset --self-calibration N9NNN EM10 33 40m`

Transmit repeatedly on 40m, use NTP-based frequency offset calibration, and add a random frequency offset to each transmission to minimize collisions with other transmitters.

