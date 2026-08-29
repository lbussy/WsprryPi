<!-- omit in toc -->
# Wsprry Pi

*A QRP WSPR transmitter leveraging a Raspberry Pi*, (pronounced: (Whispery Pi))

[![Documentation Status](https://readthedocs.org/projects/wsprry-pi/badge/?version=stable)](https://wsprry-pi.readthedocs.io/en/stable/?badge=stable)

<!-- omit in toc -->
## Table of Contents

- [License](#license)
- [Origins](#origins)
- [Development and testing](#development-and-testing)
- [Installation](#installation)

## License

Versions 2.x+ of this project are distributed the [MIT License](LICENSE.MIT.md).

## Origins

This idea likely traces its origins to an idea Oliver Mattos and Oskar Weigl presented at the PiFM project. While the website is no longer online, the Wayback Machine has [the last known good version]( http://web.archive.org/web/20131016184311/http://www.icrobotics.co.uk/wiki/index.php/Turning_the_Raspberry_Pi_Into_an_FM_Transmitter).

The original PiFM code is still hosted by the icrobotics.co.uk website. However, I suspect the domain has fallen into disrepair and may be unsafe, and no direct links are provided here. You can use the link above to see the site; should the code disappear, I have [saved it here](./historical/pifm.tar.gz).

After a conversation with Bruce Raymond of TAPR; I forked @threeme3's repo and provided some rudimentary installation capabilities and associated orchestration.  Version's 1.x of this project were a fork of threeme3/WsprryPi, licensed under the GNU General Public License v3 (GPLv3). The original project is no longer maintained.

In late 2024, George [K9TRV] of TAPR reached out to me about some questions related to using WsprryPi on the Pi 5.  While I have not yet made that jump, the conversation spurred me to discard the original code in favor of a more modern, extensible, and maintainable base.

Version 2.x+ is re-written from scratch, is no longer a derivative work, and is released under the MIT license.

## Development and testing

Developers can exercise WsprryPi's application planning, scheduling,
cancellation, status, failure, and cleanup paths on an ordinary Debian machine
without GPIO, MMIO, mailbox, DMA, I2C, transmitter device nodes, or RF hardware.
See the [hardware-free simulated backend guide](docs/simulated-backend.md) for
building, explicit selection, deterministic virtual-time traces, bounded
real-time tests, fault injection, repeated execution, Debian CI, and the limits
of simulation evidence.

RP1 GPCLK support is compiled into the application as a consumer of an
externally provisioned `/dev/rp1-gpclk` provider. WsprryPi does not install,
remove, package, attest, or validate the provenance of that provider. Runtime
use still validates the provider protocol, required capabilities, selected
route, exclusive operation ownership, bounded requests, and cleanup state.
Those checks establish application compatibility only; they do not qualify
installation, GPIO behavior, timing, frequency accuracy, spectral performance,
or RF output.

## Installation

Please see [the documentation](https://wsprry-pi.readthedocs.io/en/stable/) for background, installation, and use.

If you want to get started and not read anything, here is how to install it:

``` bash
curl -L installwspr.aa0nt.net | sudo bash
```
