# Advanced Operations

Everyone wants to know hat is under the covers. These topics cover some of the underlying controls and topics related to WSPR and `wspr` operations.

## Transmission Timing

This software uses system time to determine the start of WSPR transmissions, so keep the system time synchronized within 1-second precision, i.e., use NTP network time synchronization or set the time manually with the `date` command. A WSPR broadcast starts on an even minute and takes 2 minutes for WSPR-2. Broadcasts will begin at minutes 0, 15, 30, and 45 and take 15 minutes for WSPR-15.

## Calibration

The system will use NTP calibration by default and produces a frequency error of about 0.1 PPM after the Pi has temperature stabilized and the NTP loop has converged.

Frequency calibration is required to ensure the WSPR-2 transmission occurs within the narrow 200 Hz band. The reference crystal on your RPi might have a frequency error (which, in addition, is temperature dependent -1.3Hz/deg C @10MHz). You may manually correct the frequency or add a PPM correction on the command line to calibrate the transmission.

### NTP Calibration

NTP automatically tracks and calculates a PPM frequency correction. If running NTP on your Pi, use the `--self-calibration` option to have this program query NTP for the latest frequency correction before each WSPR transmission. Some residual frequency errors may still be present due to delays in the NTP measurement loop, and this method works best if your Pi has been on for a long time, the crystal's temperature has stabilized, and the NTP control loop has converged.

### AM Calibration

A practical way to calibrate is to tune the transmitter on the same frequency as a medium wave AM broadcast station, keep tuning until zero-beat (the constant audio tone disappears when the transmitter is precisely on the same frequency as the broadcast station), and determine the frequency difference with the broadcast station. This difference is the frequency error that can be applied for correction while tuning on a WSPR frequency.

Suppose your local AM radio station is at 780kHz. Use the `--test-tone` option to produce different tones around 780kHz (e.g., 780100 Hz) until you can successfully zero-beat the AM station. If the zero-beat tone specified on the command line is F, calculate the PPM correction required as `ppm=(F/780000-1)*1e6`. In the future, specify this value as the argument to the `--ppm` option on the command line. You can verify that the ppm value has been set correction by specifying `--test-tone 780000 --ppm \<ppm\>` on the command line and confirming that the Pi is still zero beating the AM station.

## INI File

System daemon operations will read the `wspr.ini` file to supply execution parameters. During everyday use, there should be no reason to edit the file directly. The Wsprry Pi installer stores the file in the user data directory:

``` bash
$ ls -al /usr/local/etc
total 12
drwxr-xr-x  2 root root 4096 Feb 18 14:51 .
drwxr-xr-x 10 root root 4096 Sep 21 19:02 ..
-rw-rw-rw-  1 root root  171 Mar  6 19:47 wspr.ini
```

The INI file is a standard INI file with which you may already be familiar. It has three sections, Control, Common, and Extended. The application will ignore blank lines and other whitespaces. The settings are in a key/value pair separated by an equals sign. Comments are allowed and delineated by a semicolon.

The web page configuration will overwrite any comments added to the file.

Here is an example file:

``` ini
[Control]
Transmit = False ; Bool

[Common]
Call Sign = NXXX ; String of 7 or less
Grid Square = ZZ99 ; Four characters
TX Power = 20 ; Power in dBm is always a relatively low integer
Frequency = 20m ; This can be an integer with "m" on end, an integer as in "450000000", or an exponential format like "780e3"

[Extended]
PPM = 0.0 ; Double
Self Cal = True ; Bool
Offset = False; Bool
Use LED = False ; Bool
Power Level = 7 ; 0-7, 7 is max
```

- **Control**
   - *Transmit**: A true or false Boolean, indicating whether wspr will transmit. Even when not transmitting, wspr will continue to run and react to changes in the INI.
- **Common**
  - **Call Sign**: Your registered call sign.  Your callsign will be alphanumeric, contain no spaces, and be that which is assigned to you by your authorizing entity.
  - **Grid Square**: Your four-digit Maidenhead grid square. It will be two letters followed by two numbers.
  - **TX Power**:
  - **Frequency**: A string representing the frequency or a list of frequencies through which wspr will rotate. The notation can be an integer with "m" as a suffix indicating a standard band plan, an integer as in '450000000' representing the frequency in Hz, or an exponential format like '780e3'. The '-15' indicates it should use a 15-minute window in LF, MF, or 160m.
- **Extended**
  - **PPM*: A double for the PPM offset to use as a specific calibration. The compensation is not honored when in self-calibration mode.
  - **Self Cal**: A true/false Boolean indicating whether wspr should self-calibrate with the NTP clock.
  - **Offset**: = A true/false Boolean indicating whether to add a slight offset to each transmission.
  - **Use LED**: A true/false Boolean to use the LED attached to the TAPR board as a transmission indicator.
  - **Power Level**: A power level indicator, 0-7, the default is 7:
    - *0*: 2mA or -3.4dBm
    - *1*: 4mA or 2.1dBm
    - *2*: 6mA or 4.9dBm
    - *3*: 8mA or 6.6dBm
    - *4*: 10mA or 8.2dBm
    - *5*: 12mA or 9.2dBm
    - *6*: 14mA or 10.0dBm
    - *7*: 16mA or 10.6dBm

## PWM Peripheral

The code uses the RPi PWM peripheral to time the frequency transitions of the output clock. The RPi sound system also uses this peripheral; hence, any system sound events during a WSPR transmission will interfere with WSPR transmissions. Sound can be permanently disabled by editing `/etc/modules` and commenting out the `snd-bcm2835` device.

## RF And Electrical Considerations

The output of the Pi's PWM is a square wave, so a low-pass filter is REQUIRED. Knowing why this is required is part of learning to be a Ham. Connect a low-pass filter (via a decoupling capacitor) to GPIO4 (GPCLK0) and the Ground pin of your Raspberry Pi and connect an antenna to the LPF. The GPIO4 and GND pins are found on the main header on pins 7 and 9, respectively; the pin closest to the P1 label is pin 1, and its 3rd and 4th neighbor is pin 7 and 9, respectively. See [this link](http://elinux.org/RPi\_Low-level\_peripherals) for pin layout.

Examples of low-pass filters can be [found here](http://www.gqrp.com/harmonic\_filters.pdf) (PDF link). TAPR makes a [very nice shield for the Raspberry Pi](https://www.tapr.org/kits\_20M-wspr-pi.html) that is pre-assembled, performs the appropriate filtering for the selected band, and also amplifies the power output to 20dBm. Just connect your antenna, and you're good to go.

The expected power output is 10mW (+10dBm) in a 50 Ohm load without the TAPR hat. This power looks negligible, but when connected to a simple dipole antenna, this may result in reception reports ranging up to several thousand kilometers.

As the Raspberry Pi does not attenuate ripple and noise components from the 5V USB power supply, you should use a regulated supply with sufficient ripple suppression. You may see a supply ripple as mixing products centered around the transmit carrier, typically at 100/120Hz.

Do not expose GPIO4 to voltages or currents above the Absolute Maximum limits. GPIO4 outputs a digital clock in 3V3 logic, with a maximum current of 16mA. As no current protection is available and the system uses a DC component of 1.6V, do not short-circuit or place a resistive (dummy) load straight on the GPIO4 pin, as it may draw too much current. Instead, use a decoupling capacitor to remove the DC component when connecting the output dummy loads, transformers, antennas, etc. Do not expose GPIO4 to electrostatic voltages or voltages exceeding the 0 to 3.3V logic range; connecting an antenna directly to GPIO4 may damage your RPi due to transient voltages such as lightning or static buildup as well as RF from other transmitters operating into nearby antennas. Therefore, adding some form of isolation is recommended, e.g., using an RF transformer, a simple buffer/driver/PA stage, or two Schottky small signal diodes back-to-back.
