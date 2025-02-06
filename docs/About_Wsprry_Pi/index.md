# About Wsprry Pi

WsprryPi creates a very simple WSPR beacon on your Raspberry Pi by generating a Pulse-Width Modulation (PWM) square-wave signal through a General-Purpose Input/Output (GPIO) pin on a Raspberry Pi. This is connected through a [Low-Pass Filter to remove harmonics](https://www.nutsvolts.com/magazine/article/making\_waves\_) and then to an appropriate antenna. It operates on LF, MF, HF, and VHF bands from 0 to 250 MHz.

This image shows a square waveform, with an overlay showing how a waveform might look through successive low-pass filters:

![Waveforms](waveforms.png)

Image from [https://analogisnotdead.com/article25/circuit-analysis-the-boss-bd2](https://analogisnotdead.com/article25/circuit-analysis-the-boss-bd2)

You should not use Wsprry Pi without a low pass filter as it will create interference from harmonics on other bands.

This software is compatible with the original Raspberry Pi, the Raspberry Pi 2/3, the Raspberry Pi 4, and the Pi Zero.

## Attribution

The original WsprryPi was written by [Dan Ankers](https://github.com/DanAnkers/) and released on [GitHub](https://github.com/DanAnkers/WsprryPi). This work was, in turn, based on Dan's [PiFmDma](https://github.com/DanAnkers/PiBits/blob/master/PiFmDma/) where Dan further expanded (with DMA) upon previous work. That earlier work was Oliver Mattos' [FM Improved](http://www.icrobotics.co.uk/wiki/index.php/Turning\_the\_Raspberry\_Pi\_Into\_an\_FM\_Transmitter).

If I need to correct any of this history, please let me know by [opening an issue](https://github.com/lbussy/WsprryPi/issues).

[threeme3](https://github.com/threeme3/) adapted Dan's repo, and then I forked threeme3's work here. Bruce Raymond from [TAPR](https://tapr.org/) (who sell the [WPSR Without Tears](https://tapr.org/product/wsprrypi/) hat for the Raspberry Pi) incorporated a few things in some complete Pi images that TAPR makes available, and I wanted to incorporate those here as well. Of course, the Internet being what it is, the libraries Bruce used are no longer available, so I had to go in a different direction.

Where do I come in? Well, Bruce was friendly enough to answer my email. I'm a new ham in 2022 (KF0LDK originally, now AA0NT), still trying to find my way, but I have some minor success in the Raspberry Pi ecosystem, so I decided to take on the gap which might exist between a ham and a new Pi owner. I want to be VERY clear. I am not your Elmer; I cannot help with ham radio. This project stops where WSPR executes on the Pi, and you are responsible for your actions, your hardware, and your level of understanding.

My goal, and where success will be determined, is to allow you to execute one command on your Pi to install and run the Wsprry Pi software. If you are lucky and have been living right, a radio wave will hit the cosmos and be received [somewhere else](https://wsprnet.org).
