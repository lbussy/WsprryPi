# TODO

## C++ Code

- [ ] Create missing INI entries?
- [ ] Check default state of shutdown pin

## Web UI

- [ ] Implement freq_test
- [ ] Think about separating header/footer/body pages

## Orchestration

- [ ] Inline the apache_tool into install.
- [ ] Create release script orchestration
- [ ] See if we can "upgrade" the INI
- [ ] Make sure we need all libs
- [ ] Update unstall.sh on release:
    - declare REPO_BRANCH="${REPO_BRANCH:-do_not_use}"
    - declare GIT_TAG="${GIT_TAG:-2.0_Beta.3}"
    - declare SEM_VER="${SEM_VER:-2.0_Beta.3}"

## Shit to Remember

- [ ] No need to disable_sound() on Pi 5 and up (install and uninstall)

```bash
awk -F= '
  NR==FNR {                    # first, read the old INI
    overrides[$1] = $0         #  key → entire line
    next
  }
  $1 in overrides {            # then for each line in the new INI…
    $0 = overrides[$1]         #  if its key exists in the old INI, replace it
  }
  1                            # print the (possibly modified) line
' old.ini new.ini > merged.ini
```
