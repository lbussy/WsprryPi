<!-- Grammar and spelling checked -->
# About WSPR

WSPR, "Weak Signal Propagation Reporter," is a protocol for low-power, digital communication on amateur radio frequencies. WSPR is designed to be a highly efficient way of transmitting and receiving information over long distances, even under challenging radio propagation conditions.

WSPR operates by encoding information in a series of tones and transmitting it in short, controlled bursts using a highly efficient modulation scheme. The resulting signal is often too weak to be heard by human ears but can be decoded by specialized software on a receiving station, which can extract the encoded information from the noise.

WSPR is transmitted on a schedule, allowing multiple stations to share a single frequency. This will enable WSPR for various applications, including monitoring and analyzing radio propagation conditions, testing equipment and antennas, and conducting experiments in low-power, long-range communication.

One of the critical features of WSPR is its ability to operate in very low signal-to-noise ratios. This allows for reliable communication over long distances, even under poor propagation conditions. This makes it a popular choice among amateur radio enthusiasts and researchers interested in studying long-range radio propagation and related phenomena.

WSPR is the software that implements MEPT-JT, which started life in 2008 as a suggestion by Murray ZL1BPU. Joe K1JT took up the challenge, designed the software, and added many improvements. 'MEPT-JT' is derived from 'MEPT' (Manned Experimental Propagation Transmission), a generic term for a range of low-powered techniques, and 'JT' is Joe Taylor's initials. It is one of a general family of D-MEPT (digital MEPT) modes.

The idea was to take a minimal message, code it as tightly as possible, add forward error correction, and then combine this with a known pseudo-random sequence to generate a two-minute message to be sent using 4-FSK modulation and a low baud rate. Joe estimated that the system's theoretical sensitivity would be -30dB S/N.

Transmissions carry a station's callsign, [Maidenhead grid locator](https://en.wikipedia.org/wiki/Maidenhead\_Locator\_System), and transmitter power indicated in dBm. Receiving stations can decode signals with a signal-to-noise ratio [as low as −28 dB in a 2500 Hz bandwidth](https://physics.princeton.edu//pulsar/K1JT/wsprrypi.html). Stations with internet access can automatically upload their reception reports to a central database called [WSPRnet](https://wsprnet.org), which includes a mapping facility.

The basic specifications of the MEPT\_JT mode are as follows:

- Transmitted message: callsign + 4-character-locator + dBm Example: "K1JT FN20 30"
- Message length after lossless compression: 28 bits for callsign, 15 for locator, 7 for power level ==\> 50 bits total.
- Forward error correction (FEC): long-constraint convolutional code, K=32, r=1/2.
- Number of channel symbols: nsym = (50+K-1)\*2 = 162.
- Keying rate: 12000/8192 = 1.46 baud.
- Modulation: continuous phase 4-FSK. Tone separation 1.46 Hz.
- Synchronization: 162-bit pseudo-random sync vector.
- Data structure: each channel symbol conveys one sync bit and one data bit.
- Duration of transmission: 162\*8192/12000 = 110.6 s.
- Transmissions start two seconds into an even UTC minute: i.e., at hh:00:02, hh:02:02, ...
- Occupied bandwidth: about 6 Hz
- Minimum S/N for reception: around -27 dB on the WSJT scale (2500 Hz reference bandwidth).

Like JT65, MEPT\_JT includes efficient data compression and strong forward error correction. Received messages are nearly always the same as the transmitted message or are left blank.
