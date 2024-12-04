# Source this logging functions file in another script
# source ./micro_log.sh

# Function: log_message
#
# Description:
#   Concatenates one or more strings, joining them with a single space, and
#   echoes the result to the screen.
#
# Parameters:
#   $@ - One or more strings to concatenate.
#
# Returns:
#   Outputs the concatenated string to the screen.
log_message() {
    local result="" # Holds the concatenated result
    for arg in "$@"; do
        result+="$arg "
    done
    # Trim trailing space and ensure the message ends with a newline
    echo -e "${result% }"
}

# Function: logD
#
# Description:
#   Prepends the [DEBUG] tag to the list of strings and passes them to log_message.
#
# Parameters:
#   $@ - One or more strings to concatenate.
#
# Returns:
#   Outputs the concatenated debug message to the screen.
logD() {
    : #log_message "[DEBUG]" "$@"
}

# Function: logI
#
# Description:
#   Prepends the [INFO] tag to the list of strings and passes them to log_message.
#
# Parameters:
#   $@ - One or more strings to concatenate.
#
# Returns:
#   Outputs the concatenated debug message to the screen.
logI() {
    log_message "[INFO]" "$@"
}

# Function: logW
#
# Description:
#   Prepends the [WARN] tag to the list of strings and passes them to log_message.
#
# Parameters:
#   $@ - One or more strings to concatenate.
#
# Returns:
#   Outputs the concatenated warning message to the screen.
logW() {
    log_message "[WARN]" "$@"
}

# Function: logE
#
# Description:
#   Prepends the [ERROR] tag to the list of strings and passes them to log_message.
#
# Parameters:
#   $@ - One or more strings to concatenate.
#
# Returns:
#   Outputs the concatenated error message to the screen.
logE() {
    log_message "[ERROR]" "$@"
}

# Function: logC
#
# Description:
#   Prepends the [CRIT] tag to the list of strings and passes them to log_message.
#
# Parameters:
#   $@ - One or more strings to concatenate.
#
# Returns:
#   Outputs the concatenated critical message to the screen.
logC() {
    log_message "[CRIT]" "$@"
}

# Example usage
# Uncomment the lines below to test the functions
# logD "This is a debug message."
# logW "This is a warning message."
# logE "An error has occurred."
# logC "Critical failure detected! Immediate action required."
