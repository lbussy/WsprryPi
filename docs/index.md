# Welcome to Wsprry Pi's documentation!

This is a project for amateur (ham) radio enthusiasts, using an inexpensive single board computer (Raspberry Pi) to experiment with signal propagation.

## Description

[WSPR](https://en.wikipedia.org/wiki/WSPR_(amateur_radio_software)) (pronounced "whisper") is an acronym for Weak Signal Propagation Reporter. It is a protocol, implemented in a computer program, used for weak-signal radio communication between amateur radio operators. The protocol was designed, and a program written initially, by Joe Taylor, K1JT. The software code is now open source and is developed by a small team. The program is designed for sending and receiving low-power transmissions to test propagation paths on the MF and HF bands.

WSPR implements a protocol designed for probing potential propagation paths with low-power transmissions. Transmissions carry a station's callsign, [Maidenhead grid locator](https://en.wikipedia.org/wiki/Maidenhead_Locator_System), and transmitter power indicated in in dBm. The program can decode signals with a signal-to-noise ratio [as low as −28 dB in a 2500 Hz bandwidth](https://physics.princeton.edu//pulsar/K1JT/wspr.html). Stations with internet access can automatically upload their reception reports to a central database called [WSPRnet](https://wsprnet.org), which includes a mapping facility. 

WsspryPi (Whisperry Pi) creates a very simple WSPR beacon on your Rasberry Pi by generating a Pulse-Witgh Modulation (PWM) square-wave signal through a General Purpose Input/Output (GPIO) pin on a Raspberry Pi.  This is connected through a [Low-Pass Filter to remove harmonics](https://www.nutsvolts.com/magazine/article/making_waves_) and then to an appropriate antenna.  It operates on LF, MF, HF, and VHF bands from 0 to 250 MHz.

This software is compatible with the original Raspberry Pi, the Raspberry Pi 2/3, the Raspberry Pi 4, and the Pi Zero.

## Foreword and Attribution

The original WsprryPi was written by [Dan Ankers](https://github.com/DanAnkers/) and released on [GitHub](https://github.com/DanAnkers/WsprryPi). This work was, in turn, based on Dan's [PiFmDma](https://github.com/DanAnkers/PiBits/blob/master/PiFmDma/) where Dan further expanded (with DMA) upon previous work. That previous work was Oliver Mattos' [FM Improved](http://www.icrobotics.co.uk/wiki/index.php/Turning_the_Raspberry_Pi_Into_an_FM_Transmitter).

If I need to correct any of this history, please let me know by [opening an issue](https://github.com/lbussy/WsprryPi/issues).

[threeme3](https://github.com/threeme3/) adapted Dan's repo, and then I forked threeme3's work here.  Bruce Raymond from [TAPR](https://tapr.org/) (who sell the [WPSR Without Tears](https://tapr.org/product/wspr/) hat for the Raspberry Pi) incorporated a few things in some complete Pi images that TAPR makes available, and I wanted to incorporate those here as well. Of course, the Internet being what it is, the libraries Bruce used are no longer available, so I had to go in a different direction.

Where do I come in? Well, Bruce was friendly enough to answer my email. I'm a new ham in 2022 (KF0LDK) still trying to find my way, but I have some minor success in the Raspberry Pi ecosystem, so I decided to take on the gap which might exist between a ham and a new Pi owner. I want to be VERY clear. I am not your Elmer; I cannot help with Ham radio. This project stops where WSPR executes on the Pi, and you are responsible for your actions, your hardware, and your level of understanding.

My goal, and where success will be determined, is to allow you to execute one command on your Pi to install and run the Wsprry Pi software. If you are lucky and have been living right, a radio wave will hit the cosmos and be received [somewhere else](https://wsprnet.org).

## Table of Contents
```{toctree}
:glob: true
:maxdepth: 2
:titlesonly: true

installation/index
operations/index
developer/index
```

## External References

* [BCM2835 ARM Peripherals](http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf)
* [BCM2835 Audio Clocks](http://www.scribd.com/doc/127599939/BCM2835-Audio-clocks)
* [GPIO Pads Control2](http://www.scribd.com/doc/101830961/GPIO-Pads-Control2)
* [Clock Management](https://github.com/mgottschlag/vctools/blob/master/vcdb/cm.yaml)
* [pagemap, from the userspace perspective](https://www.kernel.org/doc/Documentation/vm/pagemap.txt)
