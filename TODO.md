# TODO

## C++ Code

- [ ] ?

## Web UI

- [ ] Add "Are you sure" for shutdown and reboot
- [ ] Can we make the card title an include?

## Orchestration

- [ ] Create release script orchestration
  - `clear && make clean && make debug && make release && make install && cp ./build/bin/wsprrypi* ../executables/`
- [ ] Update install.sh on release:
  - declare REPO_BRANCH="${REPO_BRANCH:-do_not_use}"
  - declare GIT_TAG="${GIT_TAG:-2.0_Beta.3}"
  - declare SEM_VER="${SEM_VER:-2.0_Beta.3}"

## Documentation

- [ ] Add "Reboot" and "Shutdown" handler/actions docs
- [ ] Add testtone documentation
- [ ] Add Log viewer documentation
- [ ] Add spots documentation
- [ ] Add apache redirect documentation
- [ ] Add documentation about chrony running vs being synchronized
- [ ] See if I need to update docs for apache proxy @ /etc/apache2/sites-available/wsprrypi.conf

## Shit to Remember

- [ ] No need to disable_sound() on Pi 5 and up (install and uninstall)
