# TODO

## Web UI

- [ ] Implement freq_test
- [ ] Think about separating header/footer/body pages

## Orchestration

- [ ] Inline the apache_tool into install.
- [ ] Inline the old version cleanup into install.
- [ ] Update `make install` to call installer
- [ ] Update `make uninstall` to call uninstaller
- [ ] Create release script orchestration
- [ ] Uninstall message weirdness:
        [INFO ] Ensuring destination directory does not exist: '/home/pi/WsprryPi'
        [INFO ] Destination directory already exists: '/home/pi/WsprryPi'
- [ ] Allow copy_docs to work from a submodule

## Shit to Remember

- [ ] No need to disable_sound() on Pi 5 and up (install and uninstall)
