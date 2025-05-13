# TODO

## C++ Code

- [ ] PPM not initialized after reboot
    2025-05-12 11:17:16.934 UTC [INFO ] WsprryPi version 2.0_Beta.2-2-0-devel+0c60b4e (2-0-devel).
    2025-05-12 11:17:16.935 UTC [INFO ] Running on: Raspberry Pi 4 Model B Rev 1.1.
    2025-05-12 11:17:16.935 UTC [INFO ] Process PID: 708
    2025-05-12 11:17:17.093 UTC [WARN ] System time is not synchronized. Unable to measure PPM accurately.
- [ ] Logs do not restart after a reboot (or startup?)

## Web UI

- [ ] Implement freq_test?

## Orchestration

- [ ] Remove ./config/wsprrypi_merged.ini after local install
- [ ] Inline the apache_tool into install
- [ ] Create release script orchestration
- [ ] Update install.sh on release:
  - declare REPO_BRANCH="${REPO_BRANCH:-do_not_use}"
  - declare GIT_TAG="${GIT_TAG:-2.0_Beta.3}"
  - declare SEM_VER="${SEM_VER:-2.0_Beta.3}"

## Documentation

- [ ] Add "Reboot" and "Shutdown" handler/actions docs
- [ ] Add testtone documentation

## Shit to Remember

- [ ] No need to disable_sound() on Pi 5 and up (install and uninstall)
