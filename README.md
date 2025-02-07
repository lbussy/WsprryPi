<!-- omit in toc -->
# Wsprry Pi
*A QRP WSPR transmitter leveraging a Raspberry Pi*, (pronounced: (Whispery Pi))

[![Documentation Status](https://readthedocs.org/projects/wsprry-pi/badge/?version=latest)](https://wsprry-pi.readthedocs.io/en/latest/?badge=latest)

<!-- omit in toc -->
## Table of Contents
- [License](#license)
- [Origins](#origins)
- [Contributors](#contributors)
- [Installation](#installation)

## License

This project is distributed under multiple licenses:

- **Compiled Code:** The original code and codebase (C and C++) within the `/src` directory are licensed under the GNU General Public License (GPL) v3. See the [LICENSE.GPL](./LICENSE.GPL.md) file for details.
- **ALl Other Works:** All new works in this repository, including those within the `/src` directory are licensed under the MIT License. See the [LICENSE.MIT](LICENSE.MIT.md) file for details.

## Origins

This work likely traces its origins to an idea Oliver Mattos and Oskar Weigl presented at the PiFM project. While the website is no longer online, the Wayback Machine has [the last known good version]( http://web.archive.org/web/20131016184311/http://www.icrobotics.co.uk/wiki/index.php/Turning_the_Raspberry_Pi_Into_an_FM_Transmitter).

The original PiFM code is still hosted by the icrobotics.co.uk website. However, I suspect the domain has fallen into disrepair and may be unsafe, and no direct links are provided here. You can use the link above to see the site; should the code disappear, I have [saved it here](./historical/pifm.tar.gz).

This project is a fork of threeme3/WsprryPi, licensed under the GNU General Public License v3 (GPLv3). The original project is no longer maintained, and this fork aims to continue developing its capabilities.

My work was inspired by a conversation with Bruce Raymond of TAPR; this fork has diverged substantially from its origins and operates as an independent project.

## Contributors
- threeme3 (Original Author)
- Bruce Raymond (Inspiration and Guidance)
- Lee Bussy, aa0nt@arrl.net

## Installation

Please see [the documentation](https://wsprry-pi.readthedocs.io/en/latest/) for background, installation, and use.

If you want to get started and not read anything, here is how to install it:

``` bash
curl -L installwspr.aa0nt.net | sudo bash
```
