# Declarations and Functions from Logging

## Declarations

``` bash
readonly THISSCRIPT=$(basename "$0")                    # Name of the script
readonly VERSION="1.2.1-version-files+91.3bef855-dirty" # Script version
readonly GIT_BRCH="version_files"                       # Git branch

declare LOG_FILE=""                       # Path to the log file (modifiable)
declare LOG_LEVEL="DEBUG"                 # Default log level
declare LOG_TO_FILE=""
# Define log properties (severity, colors, and labels)
declare -A LOG_PROPERTIES=(
    ["DEBUG"]="DEBUG|${FGCYN}|0"
    ["INFO"]="INFO|${FGGRN}|1"
    ["WARNING"]="WARN|${FGYLW}|2"
    ["ERROR"]="ERROR|${FGRED}|3"
    ["CRITICAL"]="CRIT|${FGMAG}|4"
    ["EXTENDED"]="EXTD|${FGCYN}|0"
)
```

## Functions

``` bash
default_color()
init_colors()
prepare_log_context()
clean()
timestamp()
log()
is_interactive()
print_log_entry()
log_message()
logD()
logI()
logW()
logE()
logC()
validate_log_properties()
validate_log_level()
main()
```
