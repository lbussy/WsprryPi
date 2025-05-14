# TODO

## C++ Code

- [ ] PPM not initialized after reboot
  - 2025-05-12 11:17:16.934 UTC [INFO ] WsprryPi version 2.0_Beta.2-2-0-devel+0c60b4e (2-0-devel).
  - 2025-05-12 11:17:16.935 UTC [INFO ] Running on: Raspberry Pi 4 Model B Rev 1.1.
  - 2025-05-12 11:17:16.935 UTC [INFO ] Process PID: 708
  - 2025-05-12 11:17:17.093 UTC [WARN ] System time is not synchronized. Unable to measure PPM accurately.
- [ ] Logs do not restart after a reboot (or startup?)
- [ ] For Test Tone
  - 2025-05-14 18:03:17.792 UTC [WARN ] Unknown command received: tone_start
  - 2025-05-14 18:03:19.433 UTC [WARN ] Unknown command received: tone_end

## Web UI

- [ ] Implement freq_test?

## Orchestration

- [ ] Create release script orchestration
- [ ] Update install.sh on release:
  - declare REPO_BRANCH="${REPO_BRANCH:-do_not_use}"
  - declare GIT_TAG="${GIT_TAG:-2.0_Beta.3}"
  - declare SEM_VER="${SEM_VER:-2.0_Beta.3}"

## Documentation

- [ ] Add "Reboot" and "Shutdown" handler/actions docs
- [ ] Add testtone documentation
- [ ] Add Log viewer documentation
- [ ] Add spots documentation

## Shit to Remember

- [ ] No need to disable_sound() on Pi 5 and up (install and uninstall)
