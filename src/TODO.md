# TODO

- [ ] [INFO ] WSPR loop running. <- then ->  [INFO ] INI file changed, reloading.
- [ ] Make ws messages non-blocking
- [ ] Make a multi-threaded websocket
- [ ] Need to allow -v and such without sudo check
- [ ] Invalid PPM value, defaulting to use NTP <-- shown when using NTP
- [ ] After initial startup, a second "WSPR packet payload:" pops up with a duplicate frequency
- [ ] Integrate WSPR-Transmitter
- [ ] Documentation needs updates from/related to changes to command line parsing (including INI vs option precedence)
- [ ] Note in docs that they may see: `2025-02-20 23:10:00 UTC [INFO ] Transmission started.` ... this is a floating point rounding issue.
- [ ] DMA notes at: `https://github.com/fandahao17/Raspberry-Pi-DMA-Tutorial`
- [ ] Update `make install` to call installer
- [ ] Update `make uninstall` to call uninstaller
- [ ] Make sure to skip transmission when not enabled
- [ ] Stop transmission immediately when disabling
