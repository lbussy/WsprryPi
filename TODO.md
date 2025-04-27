# TODO

## C++ Code

- [ ] Integrate WSPR-Transmitter

## Web UI

- [ ] Show logs on web page (#27)
- [ ] Implement freq_test
- [ ] Reload config when we receive `{"state":"reload","timestamp":"2025-04-27T22:26:48Z","type":"configuration"}`

## Orchestration

- [ ] Inline the apache_tool into install.
- [ ] Update `make install` to call installer
- [ ] Update `make uninstall` to call uninstaller
- [ ] Create release script orchestration
- [ ] Remove INI symlink from installer

## Documentation

- [ ] Documentation needs updates from/related to changes to command line parsing (including INI vs option precedence)
- [ ] Note in docs that they may see: `2025-02-20 23:10:00 UTC [INFO ] Transmission started.` ... this is a floating point rounding issue.
- [ ] DMA notes at: `https://github.com/fandahao17/Raspberry-Pi-DMA-Tutorial`

## Shit to Remember

- [ ] No need to disable_sound() on Pi 5 and up (install and uninstall)
