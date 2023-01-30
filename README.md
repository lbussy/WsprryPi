# WsprryPi (Whispery Pi)
*A bareback LF/MF/HF/VHF WSPR transmitter for the Raspberry Pi*

## Foreword and Attribution

The original WsprryPi was written by [Dan Ankers](https://github.com/DanAnkers/) and released on [GitHub](https://github.com/DanAnkers/WsprryPi). This work was, in turn, based on Dan's [PiFmDma](https://github.com/DanAnkers/PiBits/blob/master/PiFmDma/) where Dan further expanded (with DMA) upon previous work. That previous work was Oliver Mattos' [FM Improved](http://www.icrobotics.co.uk/wiki/index.php/Turning_the_Raspberry_Pi_Into_an_FM_Transmitter).

If I need to correct any of this, please let me know by opening an issue.

[threeme3](https://github.com/threeme3/) adapted Dan's repo, and then I forked threeme3's work here.  Bruce Raymond from [TAPR](https://tapr.org/) (who sell the [WPSR Without Tears](https://tapr.org/product/wspr/) hat for the Raspberry Pi) incorporated a few things in some Pi images TAPR makes available, and I wanted to incorporate those here as well. Of course, the Internet being what it is, the libraries Bruce used are no longer available, so I had to go in a different direction.

Where do I come in? Well, Bruce was friendly enough to answer my email. I'm a new ham (KF0LDK) still trying to find my way, but I have some minor success in the Raspberry Pi ecosystem, so I decided to take on the gap which might exist between a ham and a new Pi owner. I want to be VERY clear. I am not your Elmer; I cannot help with Ham radio. This project stops where WSPR executes on the Pi, and you are responsible for your actions, your hardware, and your level of understanding.

My goal, and where success will be determined, is to allow you to execute one command on your Pi to install and run the Wsprry Pi software. If you are lucky and have been living right, a radio wave will hit the cosmos and be received [somewhere else](https://wsprnet.org).

## Description

WsspryPi creates a very simple [WSPR](https://en.wikipedia.org/wiki/WSPR_(amateur_radio_software)) beacon on your Rasberry Pi by connecting a GPIO port on a Raspberry Pi to an antenna (through a Low-Pass Filter, please!)  It operates on LF, MF, HF, and VHF bands from 0 to 250 MHz.

This software is compatible with the original Raspberry Pi, the Raspberry Pi 2/3, the Raspberry Pi 4, and the Pi Zero.

## Usage

```
******
Usage: (WSPR --help output):
******

  Usage:
    wspr [options] callsign locator tx_pwr_dBm f1 <f2> <f3> ...
      OR
    wspr [options] --test-tone f

  Options:
    -h --help
      Print out this help screen.
    -p --ppm ppm
      Known PPM correction to 19.2MHz RPi nominal crystal frequency.
    -s --self-calibration
      Check NTP before every transmission to obtain the PPM error of the crystal (default setting!).
    -f --free-running
      Do not use NTP to correct frequency error of RPi crystal.
    -r --repeat
      Repeatedly, and in order, transmit on all the specified command line
      freqs.
    -x --terminate <n>
      Terminate after n transmissions have been completed.
    -o --offset
      Add a random frequency offset to each transmission:
        +/- 80 Hz for WSPR
        +/- 8 Hz for WSPR-15
    -t --test-tone freq
      Simply output a test tone at the specified frequency. Only used for debugging and to verify calibration.
    -n --no-delay
      Transmit immediately, do not wait for a WSPR TX window. Used for testing only.
```

Frequencies can be specified either as an absolute TX carrier frequency or using one of the following strings. If you use a string, the transmission will happen in the middle of the WSPR region of the selected band.

Available strings are:

* LF
* LF-15
* MF
* MF-15
* 160m
* 160m-15
* 80m
* 60m
* 40m
* 30m
* 20m
* 17m
* 15m
* 12m
* 10m
* 6m
* 4m
* 2m

`-15` indicates the WSPR-15 region of the band.

Transmission gaps can be created by specifying a TX frequency of 0

Note that `callsign`, `locator`, and `tx_power_dBm` are used to fill in the appropriate fields of the WSPR message; `tx_power_dBm` *has no bearing on the actual power used to transmit the message*. Typically, `tx_power_dBm` will be 10, representing the signal power coming out of the Pi (or 20 with the TAPR hat.) Set this value appropriately if you are using an external amplifier.

## Licensing Requirements

To transmit legally, you must have a HAM Radio License before running this experiment.

## RF Considerations

The output is a square wave, so a low-pass filter is REQUIRED. Connect a low-pass filter (via decoupling C) to GPIO4 (GPCLK0) and the Ground pin of your Raspberry Pi, and connect an antenna to the LPF. The GPIO4 and GND pins are found on the main header on pins 7 and 9, respectively; the pin closest to the P1 label is pin 1, and its 3rd and 4th neighbor is pin 7 and 9, respectively. See [this link](http://elinux.org/RPi_Low-level_peripherals) for pin layout.
  
Examples of low-pass filters can be [found here](http://www.gqrp.com/harmonic_filters.pdf). [TAPR makes a very nice shield for the Raspberry Pi](https://www.tapr.org/kits_20M-wspr-pi.html) that is pre-assembled performs the appropriate filtering for the selected band, and also increases the power output to 20dBm! Just connect your antenna, and you're good to go.
  
The expected power output is 10mW (+10dBm) in a 50 Ohm load without the TAPR hat. This power looks negligible, but when connected to a simple dipole antenna, this may result in reception reports ranging up to several thousands of kilometers.

As the Raspberry Pi does not attenuate ripple and noise components from the 5V USB power supply, it is RECOMMENDED to use a regulated supply with sufficient ripple suppression. Supply ripple might be seen as mixing products centered around the transmit carrier, typically at 100/120Hz.

DO NOT expose GPIO4 to voltages or currents above the specified Absolute Maximum limits. GPIO4 outputs a digital clock in 3V3 logic, with a maximum current of 16mA. As there is no current protection available and a DC component of 1.6V, DO NOT short-circuit or place a resistive (dummy) load straight on the GPIO4 pin, as it may draw too much current. Instead, use a decoupling capacitor to remove the DC component when connecting the output dummy loads, transformers, antennas, etc. DO NOT expose GPIO4 to electrostatic voltages or voltages exceeding the 0 to 3.3V logic range; connecting an antenna directly to GPIO4 may damage your RPi due to transient voltages such as lightning or static buildup as well as RF from other transmitters operating into nearby antennas. Therefore it is RECOMMENDED to add some form of isolation, e.g., by using an RF transformer, a simple buffer/driver/PA stage, or two Schottky small signal diodes back to back.

## Transmission Timing

This software uses system time to determine the start of WSPR transmissions, so keep the system time synchronized within 1-second precision, i.e., use NTP network time synchronization or set time manually with the `date` command. A WSPR broadcast starts on an even minute and takes 2 minutes for WSPR-2 or begins at minutes :00, :15, :30, :45 and takes 15 minutes for WSPR-15. It contains a callsign, a 4-digit Maidenhead square locator, and transmission power. You may view reception reports on the [Weak Signal Propagation Reporter Network Spot Database](http://wsprnet.org/drupal/wsprnet/spots). 

## Calibration

As of 2017-02, NTP calibration is enabled by default and produces a frequency error of about 0.1 PPM after the Pi has temperature stabilized and the NTP loop has converged.

Frequency calibration is REQUIRED to ensure that the WSPR-2 transmission occurs within the narrow 200 Hz band. The reference crystal on your RPi might have a frequency error (which, in addition, is temp. dependent -1.3Hz/degC @10MHz). You may manually correct the frequency or add a PPM correction on the command line to calibrate the transmission.

### NTP Calibration

NTP automatically tracks and calculates a PPM frequency correction. If you are running NTP on your Pi, you can use the `--self-calibration` option to have this program query NTP for the latest frequency correction before each WSPR transmission. Some residual frequency error may still be present due to delays in the NTP measurement loop, and this method works best if your Pi has been on for a long time, the crystal's temperature has stabilized, and the NTP control loop has converged.

### AM Calibration

A practical way to calibrate is to tune the transmitter on the same frequency as a medium wave AM broadcast station; keep tuning until zero beat (the constant audio tone disappears when the transmitter is exactly on the same frequency as the broadcast station), and determine the frequency difference with the broadcast station. This difference is the frequency error that can be applied for correction while tuning on a WSPR frequency.

Suppose your local AM radio station is at 780kHz. Use the `--test-tone` option to produce different tones around 780kHz (e.g., 780100 Hz) until you can successfully zero-beat the AM station. If the zero beat tone specified on the command line is F, calculate the PPM correction required as `ppm=(F/780000-1)*1e6`. In the future, specify this value as the argument to the `--ppm` option on the command line. You can verify that the ppm value has been set correction by specifying `--test-tone 780000 --ppm <ppm>` on the command line and confirming that the Pi is still zero beating the AM station.

## PWM Peripheral

The code uses the RPi PWM peripheral to time the frequency transitions of the output clock. The RPi sound system also uses this peripheral; hence, any sound events that occur during a WSPR transmission will interfere with WSPR transmissions. Sound can be permanently disabled by editing `/etc/modules` and commenting out the `snd-bcm2835` device.

## Example Usage

Remember that anything that creates a transmission will require you to use `sudo`.

Brief help screen:

`./wspr --help`

Transmit a constant test tone at 780 kHz:

`sudo ./wspr --test-tone 780e3`

Using callsign N9NNN, locator EM10, and TX power 33 dBm, transmit a single WSPR transmission on the 20m band using NTP-based frequency offset calibration.

`sudo ./wspr N9NNN EM10 33 20m`

The same as above, but without NTP calibration:

`sudo ./wspr --free-running N9NNN EM10 33 20m`

Transmit a WSPR transmission slightly off-center on 30m every 10 minutes for a total of 7 transmissions, and using a fixed PPM correction value:

`sudo ./wspr --repeat --terminate 7 --ppm 43.17 N9NNN EM10 33 10140210 0 0 0 0`

Transmit repeatedly on 40m, use NTP-based frequency offset calibration, and add a random frequency offset to each transmission to minimize collisions with other transmitters:

`sudo ./wspr --repeat --offset --self-calibration N9NNN EM10 33 40m`

## References

* [BCM2835 ARM Peripherals](http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf)
* [BCM2835 Audio Clocks](http://www.scribd.com/doc/127599939/BCM2835-Audio-clocks)
* [GPIO Pads Control2](http://www.scribd.com/doc/101830961/GPIO-Pads-Control2)
* [Clock Management](https://github.com/mgottschlag/vctools/blob/master/vcdb/cm.yaml)
* [pagemap, from the userspace perspective](https://www.kernel.org/doc/Documentation/vm/pagemap.txt)
