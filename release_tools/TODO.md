# To Do

- Create release script orchestration
- Show logs on web page (#27)
- Show transission indicator (#64)
- Unable to install when lib versions differ (#92)

## Stuff to Remember

### Stuff to do:

- Inline the apache_tool into install.
- Add notes about gh:
    ``` bash
    $ gh auth login
    ? What account do you want to log into? GitHub.com
    ? What is your preferred protocol for Git operations? HTTPS
    ? Authenticate Git with your GitHub credentials? Yes
    ? How would you like to authenticate GitHub CLI? Login with a web browser

    ! First copy your one-time code: XXXX-XXXX
    Press Enter to open github.com in your browser...
    ✓ Authentication complete.
    - gh config set -h github.com git_protocol https
    ✓ Configured git protocol
    ✓ Logged in as foo
    ```
- Add a .wsprrypi-alias file and source it from the .bash_aliases

### Nice to Do

- Make a log viewing page
- No need to disable_sound() on Pi 5 and up (install and uninstall)
- Implement freq_test
