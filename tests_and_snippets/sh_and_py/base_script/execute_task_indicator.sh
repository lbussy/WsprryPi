#!/bin/bash

##
# @brief Determine if the script is running in an interactive shell.
#
# This function checks if the script is connected to a terminal by testing
# whether standard input and output (file descriptor 1 & 0) is a terminal.
#
# @return 0 (true) if the script is running interactively; non-zero otherwise.
##
is_interactive() {
    [[ -t 1 ]] && [[ -t 0 ]]
}

##
# @brief Retrieve the terminal color code or attribute.
#
# This function uses `tput` to retrieve a terminal color code or attribute
# (e.g., sgr0 for reset, bold for bold text). If the attribute is unsupported
# by the terminal, it returns an empty string.
#
# @param $1 The terminal color code or attribute to retrieve.
# @return The corresponding terminal value or an empty string if unsupported.
##
default_color() {
    tput "$@" 2>/dev/null || echo ""  # Fallback to an empty string on error
}

##
# @brief Initialize terminal colors and text formatting.
#
# This function sets up variables for foreground colors, background colors,
# and text formatting styles. It checks terminal capabilities and provides
# fallback values for unsupported or non-interactive environments.
##
init_colors() {
    # Local variable to check terminal color support
    local tput_colors_available
    # shellcheck disable=SC2034  # Intentional use for clarity
    tput_colors_available=$(tput colors 2>/dev/null || echo "0")

    # Initialize colors and formatting if interactive and the terminal supports at least 8 colors
    if is_interactive && [ "$tput_colors_available" -ge 8 ]; then
        # General text attributes
        RESET=$(default_color sgr0)
        MOVE_UP=$(default_color cuu 1)
        CLEAR_LINE=$(default_color el)

        # Foreground colors
        FGRED=$(default_color setaf 1)
        FGGRN=$(default_color setaf 2)
        FGGLD=$(default_color setaf 214)
    else
        # Fallback for unsupported or non-interactive terminals
        RESET=""
        MOVE_UP=""
        CLEAR_LINE=""
        FGRED=""
        FGGRN=""
        FGGLD=""
    fi

    # Export variables globally
    export RESET MOVE_UP CLEAR_LINE FGRED FGGRN FGGLD
}

##
# @brief Print the task status with start and end messages.
#
# This function prints the "[ ]" start message, then simulates a task, and finally
# moves the cursor up and rewrites the line with the "[✔]" end message.
#
# @param message The task message to display.
##
execute_task_indicator() {
    local start_indicator="${FGGLD}[-]${RESET}"
    local end_indicator="${FGGRN}[✔]${RESET}"
    local fail_indicator="${FGRED}[✘]${RESET}"
    local status_message
    local command_text="$1"
    local command="$2"
    local running_pre="Executing:"
    local running_post="running."
    local pass_pre="Executing:"
    local pass_post="completed."
    local fail_pre="Executing:"
    local fail_post="failed."

    # Put consistent single quotes around the comamnd text
    command_text="'$command_text'"

    # Set the intial message with $running_pre $command_text $running_post
    status_message="$running_pre $command_text $running_post"

    # Print the initial status
    printf "%s %s\n" "$start_indicator" "$status_message"

    # Execute the command and capture both stdout and stderr
    output=$(eval "$command 2>&1")  # Capture both stdout and stderr

    # Capture the exit status of the command
    local exit_status=$?

    # Move the cursor up and clear the line before printing the final status
    printf "%s%s" "${MOVE_UP}" "${CLEAR_LINE}"

    # Check the exit status of the command
    if [ $exit_status -eq 0 ]; then
        # If the command was successful, print $pass_pre $command_text $pass_post
        status_message="$pass_pre $command_text $pass_post"
        printf "%s %s\n" "$end_indicator" "$status_message"
    else
        # If the command failed, print $fail_pre $command_text $fail_post
        status_message="$fail_pre $command_text $fail_post"
        printf "%s %s\n" "$fail_indicator" "$status_message"
    fi
}

# Initialize terminal colors
init_colors

# Call the function with the message and the command
command="sleep 2 && foo"  # This will fail
command_text="data processing fail"
execute_task_indicator "$command_text" "$command"

# Call the function with the message and the command
command="sleep 2"  # This will fail
command_text="data processing success"
execute_task_indicator "$command_text" "$command"
