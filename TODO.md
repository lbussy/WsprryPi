# TODO

## C++ Code

- [ ] Check default state of shutdown pin

## Web UI

- [ ] Implement freq_test
- [ ] Think about separating header/footer/body pages

## Orchestration

- [ ] Inline the apache_tool into install.
- [ ] Create release script orchestration
- [ ] Update unstall.sh on release:
    - declare REPO_BRANCH="${REPO_BRANCH:-do_not_use}"
    - declare GIT_TAG="${GIT_TAG:-2.0_Beta.3}"
    - declare SEM_VER="${SEM_VER:-2.0_Beta.3}"

## Shit to Remember

- [ ] No need to disable_sound() on Pi 5 and up (install and uninstall)
