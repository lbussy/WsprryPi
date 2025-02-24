#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# TODO:  Get parameters from INI file
# TODO:  Move to wsprrypi exe

"""
@file wspr_watch.py
@brief Monitors GPIO and file-based triggers to initiate a system shutdown or reboot.

This script is designed for **Raspberry Pi and similar systems**, monitoring both:
1. **GPIO Pin (`-p GPIOx`)**:
   - A designated button press can trigger a system shutdown.
2. **File-Based Triggers**:
   - The presence of `STOP_FILE_PATH` initiates a **system shutdown**.
   - The presence of `REBOOT_FILE_PATH` initiates a **system reboot**.

### How It Works:
- **Continuously Monitors GPIO and Files**:
  - Uses `gpiozero.Button` to detect GPIO presses.
  - Checks for the existence of `STOP_FILE_PATH` and `REBOOT_FILE_PATH` every second.
- **Logs Events**:
  - Uses a structured logging system (`shutdown_watch` logger).
- **Executes a Delayed Safe Shutdown/Reboot**:
  - Executes `"sleep 1 && shutdown -h now"` or `"sleep 1 && reboot"` in a **detached subprocess**.
- **Handles System Signals**:
  - Captures `SIGINT`, `SIGTERM`, and `SIGHUP` for graceful termination.
- **Cleans Up Resources**:
  - Disables GPIO callbacks and restores terminal settings upon exit.

---

## Features:
- **GPIO Monitoring:** Listens for a button press to trigger a shutdown.
- **File-Based Shutdown/Reboot:** Detects specific files to trigger system actions.
- **Logging System:** Logs events at different levels (INFO, WARNING, ERROR).
- **Debug Mode (`-d`):** Allows testing without executing shutdown/reboot.
- **Signal Handling:** Gracefully exits when receiving `SIGINT`, `SIGTERM`, or `SIGHUP`.
- **Daemon Mode (`-D`):** Supports logging with timestamps for background execution.
- **Ensures Safe Shutdowns:** Uses `sleep 1` to prevent accidental multiple triggers.

---

## Requirements:
- **Must be run as root** for GPIO and shutdown privileges.
- Requires `gpiozero` for GPIO monitoring.
- Optionally supports `lgpio` for enhanced error handling.

---

## Command-Line Usage:

### **Start Monitoring:**
```bash
./wspr_watch.py -w
```

### **Enable Debug Mode (No Actual Shutdown/Reboot):**
```bash
./wspr_watch.py -w -d
```

### **Specify GPIO Pin for Monitoring:**
```bash
./wspr_watch.py -w -p 17
```

### **Display Help:**
```bash
./wspr_watch.py --help
```

### **Display Version:**
```bash
./wspr_watch.py --version
```

---

## Notes:
- Ensure the script has the necessary **root permissions** for GPIO and shutdown execution.
- Running as a **non-root user** will result in an error.
- **Debug mode (`-d`) prevents actual shutdown execution** but logs the event.
- The script runs **indefinitely** unless a shutdown/reboot request is detected o
  manually terminated.

---
@version 1.2.1-config_lib+50.0985f26-dirty
@date 2025-02-16
@author Lee C. Bussy <Lee@Bussy.org>
@copyright MIT License

@license
MIT License

Copyright (c) 2023-2025 Lee C. Bussy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""


import sys
import argparse
import logging
import os
import time
import signal
import termios
import subprocess
import atexit
import functools
from pathlib import Path


# Import pgiozero for GPIO function support
try:
    import gpiozero  # type: ignore
except ImportError:
    sys.exit("Failed to import gpiozero. Ensure it is installed and available.")

# Import lgpio to handle GPIO errors properly
try:
    import lgpio  # type: ignore
except ImportError:
    lgpio = None  # If unavailable, set to None so we can check later


# Version information
__version__ = "1.2.1-timing_loop+67.477644f-dirty"

# Filenames for shutdown and reboot triggers
STOP_FILE_NAME = "shutdown"
REBOOT_FILE_NAME = "reboot"

# Paths to shutdown and reboot semaphore files
STOP_FILE_PATH = Path(f"/var/www/html/wsprrypi/{STOP_FILE_NAME}.semaphore")
REBOOT_FILE_PATH = Path(f"/var/www/html/wsprrypi/{REBOOT_FILE_NAME}.semaphore")


def setup_logger(debug, date_time):
    """
    Configure and return a logger instance for shutdown monitoring.

    @param debug
        If True, sets the log level to DEBUG. Otherwise, defaults to INFO.
    @param date_time
        If True, includes timestamps in the log format. Otherwise, timestamps are omitted.

    @return
        Configured logger instance.

    @details
        - Creates a logger named `"shutdown_watch"`.
        - Sets the log level based on `debug`:
            - DEBUG level if `debug=True`.
            - INFO level otherwise.
        - Prevents duplicate log messages by setting `logger.propagate = False`.
        - Removes any existing handlers before adding new ones.
        - Configures two logging handlers:
            - `stdout_handler`: Logs INFO and DEBUG messages to stdout.
            - `stderr_handler`: Logs ERROR and higher messages to stderr.
        - Applies a log format:
            - Includes timestamps if `date_time=True`.
            - Uses a simpler format if `date_time=False`.
        - Returns the configured logger instance.

    @note
        - This function ensures that logs are properly separated between stdout and stderr.
        - The logger's handlers are cleared before adding new ones to avoid duplication.
    """
    log_level = logging.DEBUG if debug else logging.INFO
    log_format = (
        "%(asctime)s - %(levelname)s - %(message)s"
        if date_time
        else "%(levelname)s - %(message)s"
    )

    logger = logging.getLogger("shutdown_watch")
    logger.setLevel(log_level)

    # Prevent duplicate logs by disabling propagation
    logger.propagate = False  # This prevents logs from appearing in multiple handlers

    # Remove all handlers before adding new ones
    while logger.hasHandlers():
        logger.handlers.clear()

    # Create separate handlers for stdout and stderr
    stdout_handler = logging.StreamHandler(sys.stdout)
    stdout_handler.setLevel(logging.DEBUG if debug else logging.INFO)

    stderr_handler = logging.StreamHandler(sys.stderr)
    stderr_handler.setLevel(logging.ERROR)  # Only log ERROR and above to stderr

    # Set formatter
    formatter = logging.Formatter(log_format)
    stdout_handler.setFormatter(formatter)
    stderr_handler.setFormatter(formatter)

    # Add fresh handlers
    logger.addHandler(stdout_handler)
    logger.addHandler(stderr_handler)

    return logger


def disable_ctrlc_echo(logger):
    """
    Disable the terminal's default echo of `^C` when SIGINT (Ctrl+C) is received.

    @param logger
        Logger instance used to log warnings in case of errors.

    @return
        - The original terminal settings (to allow restoration later), or
        - None if the function is executed in a non-interactive environment or an error occurs.

    @details
        - Checks if the script is running in an interactive terminal using `sys.stdin.isatty()`.
        - If not running in a terminal, returns `None` without making any changes.
        - If running in a terminal:
            1. Retrieves the current terminal settings using `termios.tcgetattr(sys.stdin)`.
            2. Disables the `ECHOCTL` flag to prevent Ctrl+C from being echoed.
            3. Applies the modified settings using `termios.tcsetattr()`.
        - Returns the original terminal settings so they can be restored later.
        - If an error occurs (`termios.error`), logs a warning and returns `None`.

    @note
        - This function is useful for preventing unwanted terminal output when handling SIGINT.
        - The original terminal settings should be restored before the script exits.
        - Logs a warning instead of raising an exception if modifying the terminal settings fails.
    """
    if not sys.stdin.isatty():
        return None  # Not a terminal, do nothing

    try:
        original_settings = termios.tcgetattr(sys.stdin)  # Save original settings
        new_termios = original_settings[:]  # Copy original settings
        new_termios[3] &= ~termios.ECHOCTL  # Disable echoing of Ctrl+C
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, new_termios)
        return original_settings  # Return a reference to original settings
    except termios.error as e:
        logger.warning("Failed to disable Ctrl+C echo: %s", e, exc_info=True)
        return None  # Return None if an error occurs


def restore_terminal(local_termios=None):
    """
    Restore the terminal settings to their original state.

    :param local_termios:
        The original terminal settings before modification, typically returned
        by `disable_ctrlc_echo()`.

    :details
        - If `local_termios` is provided, restores the terminal settings using
          `termios.tcsetattr()`.
        - Uses `TCSADRAIN` to apply changes after all pending output has been written.
        - If `local_termios` is `None`, no action is taken.

    :note:
        - This function should be called before exiting if `disable_ctrlc_echo()` was used.
        - Failing to restore terminal settings may result in unexpected terminal behavior.
    """
    if local_termios:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, local_termios)


def handle_signal(signal_received, _frame, logger):
    """
    Custom signal handler to capture and gracefully handle termination signals.

    :param signal_received:
        The signal number received (e.g., SIGINT, SIGTERM).
    :param _frame:
        (Unused) The current stack frame, included as per the signal handler signature.
    :param logger:
        Logger instance used to log the received signal and termination process.

    :details
        - Converts the received signal number into a human-readable signal name.
        - Logs the received signal along with its numeric value.
        - Calls `restore_terminal()` to ensure terminal settings are reset before exiting.
        - Exits the program with `sys.exit(0)`, allowing registered cleanup handlers
          (`atexit`) to execute.

    :note:
        - This function is intended to be used with `signal.signal()` to handle OS signals.
        - Ensuring terminal settings are restored prevents unwanted terminal behavior after exit.
    """
    signal_name = signal.Signals(signal_received).name
    logger.info(
        "Received signal %s (%d). Exiting gracefully.",
        signal_name,
        signal_received,
    )

    restore_terminal()  # Ensure terminal settings are restored
    sys.exit(0)  # Let exit handlers (atexit) handle cleanup


def handle_arguments(logger, args):
    """
    Process and execute command-line arguments for the shutdown watcher.

    @param logger
        Logger instance used to log messages and errors.
    @param args
        Parsed command-line arguments from `argparse.Namespace`.

    @return
        - 0 if execution is successful.
        - 1 if the script must exit due to missing arguments or permission errors.

    @details
        - Handles command-line arguments and determines the appropriate actions.
        - If `-w` (`--watch`) is specified:
            1. Checks if the script is running as root.
                - If not, logs an error (once per session) and exits with code 1.
            2. If root, enables debug logging if `-d` (`--debug`) is set.
            3. Starts monitoring:
                - If `-p` (`--pin`) is specified, initializes GPIO monitoring via
                  `start_gpio_watch()`.
                - Calls `file_watch()` to watch for shutdown triggers.
        - If no valid arguments are provided:
            - Prints usage instructions.
            - Returns exit code 1.

    @note
        - The script must be run as root for the watcher (`-w`) to function properly.
        - Debug mode (`-d`) logs actions without executing shutdown.
        - Prevents duplicate root permission errors using `logger.root_logged`.
        - The GPIO pin (`-p`) is optional but enables additional monitoring.
    """
    if args.watch:
        if not is_root():
            if not getattr(logger, "root_logged", False):  # Safe attribute check
                logger.error("This script must be run as root. Exiting.")
                setattr(logger, "root_logged", True)  # Set attribute safely
            return 1  # Exit code for failure

        if args.debug:
            logger.info("Debug mode enabled. No actions will be taken.")

        logger.info("Starting shutdown watcher.")
        if args.pin:
            start_gpio_watch(args.pin, logger, args.debug)
        file_watch(logger, args.debug)
        return 0

    print("At least '-w' is required to enter loop.")
    print("Use -h or --help to see available options.")
    return 1  # Return error exit code


def process_arguments():
    """
    Parse and return command-line arguments for the shutdown watcher.

    @return
        argparse.Namespace: An object containing the parsed command-line arguments.

    @details
        - Uses `CustomArgumentParser` to parse command-line arguments.
        - Supports the following options:
            - `-h, --help`      → Displays usage information and exits.
            - `-d, --debug`     → Enables debug mode (logs actions but prevents shutdown).
            - `-D, --date_time` → Enables daemon mode (adds timestamps to logs).
            - `-w, --watch`     → Starts monitoring for shutdown signals.
            - `-v, --version`   → Displays the script version and exits.
            - `-p, --pin GPIOx` → Specifies the GPIO pin (BCM numbering) for monitoring.
        - Uses `argparse.RawTextHelpFormatter` for improved help message formatting.
        - The script name is dynamically included in the `usage` message via `get_script_name()`.
        - Ensures that help is shown if no arguments are provided (`TODO` item).

    @note
        - Debug mode (`-d`) allows logging without executing shutdown actions.
        - Date/Time mode (`-D`) adds timestamps to logs for background execution.
        - The `--watch` mode (`-w`) must be used to activate shutdown monitoring.
        - The GPIO pin (`-p`) must be specified as an integer (e.g., `-p 17`).
        - `-h` and `-v` cause immediate termination after displaying help/version info.
        - The script should ideally show help if executed without arguments (`TODO`).
    """
    parser = CustomArgumentParser(
        description="Shutdown Watcher for Raspberry Pi.",
        formatter_class=argparse.RawTextHelpFormatter,
        usage=f"{get_script_name()} [-h] [-d] [-D] [-w] [-v] [-p GPIOx]",
        add_help=False,
    )
    parser.add_argument(
        "-h", "--help", action="help", help="Display this help message and exit."
    )
    parser.add_argument(
        "-d",
        "--debug",
        action="store_true",
        help="Enable debug mode. Logs actions but takes no shutdown actions.",
    )
    parser.add_argument(
        "-D",
        "--date_time",
        action="store_true",
        help="Enable date_time mode. Output/logs will have date/time stamp.",
    )
    parser.add_argument(
        "-w",
        "--watch",
        action="store_true",
        help="Start monitoring for shutdown signals and take action.",
    )
    parser.add_argument(
        "-v",
        "--version",
        action="version",
        version=f"%(prog)s: {__version__}",
        help="Show the script version and exit.",
    )
    parser.add_argument(
        "-p",
        "--pin",
        metavar="GPIOx",
        type=int,
        help=(
            "GPIO pin using (BCM numbering) to monitor for a shutdown signal.\n"
            "Provide an integer only representing the GPIO pin number."
        ),
    )
    return parser.parse_args()


def is_gpio_available(pin):
    """
    Check if a specified GPIO pin is available for use.

    :param pin:
        The GPIO pin number to check.

    :return:
        - True if the pin is available.
        - False if the pin is busy or an expected error occurs.

    :details
        - Attempts to create a `gpiozero.Button` instance on the specified pin.
        - If successful, immediately closes the instance and returns `True`.
        - If an `OSError`, `RuntimeError`, or `KeyError` occurs:
            - If the error message contains "GPIO busy", returns `False`.
            - Otherwise, returns `False` without raising an exception.
        - If `lgpio` is imported and the error is an `lgpio.error`, returns `False`.
        - Unexpected exceptions are re-raised to prevent silent failures.

    :note:
        - This function ensures the pin is **immediately released** after checking.
        - Errors related to a busy GPIO pin are handled gracefully.
        - Other unexpected exceptions are **not suppressed** to avoid silent failures.
    """
    try:
        test_button = gpiozero.Button(pin)
        test_button.close()  # Release immediately
        return True
    except (OSError, RuntimeError, KeyError) as e:
        if "GPIO busy" in str(e):
            return False  # Return False without traceback
        return False  # Catch other errors without traceback
    except Exception as e:  # Catch lgpio.error only if lgpio is imported
        if lgpio and isinstance(e, lgpio.error):
            return False
        raise  # Re-raise unexpected exceptions


def start_gpio_watch(stop_pin, logger, debug):
    """
    Set up and monitor a GPIO pin for a shutdown trigger.

    @param stop_pin
        The GPIO pin (BCM numbering) to monitor for a shutdown signal.
    @param logger
        Logger instance used for logging status, warnings, and errors.
    @param debug
        If True, logs debug messages but prevents actual shutdown execution.

    @return
        - 0 if GPIO monitoring is successfully initialized.
        - 1 if GPIO initialization fails or the pin is unavailable.

    @details
        - Checks if the specified GPIO pin is available before attempting to use it.
        - If unavailable, logs a warning and exits cleanly with code 1.
        - If available:
            1. Initializes a `gpiozero.Button` instance with a pull-up resistor.
            2. Registers a callback (`when_pressed`) that calls `initiate_action()`
               to trigger the shutdown process.
        - Logs a success message when GPIO monitoring is successfully initialized.
        - Handles exceptions to ensure graceful failure:
            - `OSError`, `RuntimeError`, `KeyError`: GPIO cannot be initialized.
            - `lgpio.error`: GPIO is locked by another process.

    @note
        - The GPIO pin must be free and available for monitoring to start.
        - Debug mode (`debug=True`) logs the shutdown event but prevents actual execution.
        - Proper logging ensures visibility into both successful and failed GPIO setups.
    """
    stop_button = None  # Declare variable to avoid issues in `finally`

    # Check GPIO availability BEFORE using it
    if not is_gpio_available(stop_pin):
        logger.warning("GPIO %d is busy. Skipping GPIO monitoring.", stop_pin)
        return 1

    try:
        # Setup stop button with a pull-up
        stop_button = gpiozero.Button(stop_pin, pull_up=True)
        # Set callback for when pressed
        stop_button.when_pressed = lambda: initiate_action(logger, debug, stop_button, None)
        logger.info("Monitoring GPIO%d for shutdown signal.", stop_button.pin.number)
        return 0
    except (OSError, RuntimeError, KeyError) as e:
        logger.warning("Could not initialize GPIO %d: %s", stop_pin, e)
        return 1
    except lgpio.error as e:  # Handle `lgpio.error` separately if `lgpio` is available
        logger.warning("GPIO %d is locked by another process: %s", stop_pin, e)
        return 1


def file_watch(logger, debug):
    """
    Monitor file-based shutdown and reboot triggers.

    @param logger
        Logger instance used for logging messages.
    @param debug
        If True, logs actions without executing shutdown or reboot.

    @details
        - This function continuously monitors the presence of two special files:
            1. `STOP_FILE_PATH`: If detected, the system shuts down.
            2. `REBOOT_FILE_PATH`: If detected, the system reboots.
        - When a trigger file is found:
            - The file is deleted immediately to prevent multiple executions.
            - A log message is recorded indicating the requested action.
            - `initiate_action()` is called to process the request.
        - If `debug=True`, actions are logged but **not executed**.
        - The function runs indefinitely, polling every 1 second.
        - Exits only when a shutdown or reboot request is detected.

    @return
        This function runs indefinitely unless interrupted.

    @raises KeyboardInterrupt
        - If the function is manually interrupted (e.g., via Ctrl+C), logs an informational
          message.

    @note
        - This function relies on an external process to create `STOP_FILE_PATH` or
          `REBOOT_FILE_PATH`.
        - Debug mode (`debug=True`) prevents actual shutdown/reboot execution.
        - The script should ensure that this function is running in an appropriate execution
          environment.
        - If this function is running as a separate thread or service, ensure proper exit
          handling.
    """
    try:
        while True:
            if Path(STOP_FILE_PATH).exists():
                Path(STOP_FILE_PATH).unlink()
                logger.info("Shutdown requested by presence of %s.", STOP_FILE_PATH)
                initiate_action(logger, debug, None, STOP_FILE_PATH)

            if Path(REBOOT_FILE_PATH).exists():
                Path(REBOOT_FILE_PATH).unlink()
                logger.info("Reboot requested by presence of %s.", REBOOT_FILE_PATH)
                initiate_action(logger, debug, None, REBOOT_FILE_PATH)

            time.sleep(1)  # Poll interval

    except KeyboardInterrupt:
        logger.info("Shutdown watch script interrupted by keyboard.")


def get_script_name():
    """
    Retrieves the name of the currently executing script.

    This function returns the filename of the script, excluding its path.

    @return str: The name of the script file.
    """
    return Path(__file__).name


def is_root():
    """
    Determines whether the script is running with root (superuser) privileges.

    This function checks the user ID (UID) of the process to verify if it is
    being executed by the root user.

    @return bool: True if running as root, False otherwise.
    """
    return os.getuid() == 0  # Just return True/False


def initiate_action(logger, debug, stop_button=None, file_path=None):
    """
    Initiates a system shutdown or reboot using a detached background process.

    @param logger
        Logger instance used for logging messages.
    @param debug
        If True, logs the action but prevents actual shutdown or reboot execution.
    @param stop_button
        (Optional) `gpiozero.Button` instance that triggered the shutdown event.
    @param file_path
        (Optional) Path to the file that triggered the action
        (e.g., `STOP_FILE_PATH` or `REBOOT_FILE_PATH`). If not provided,
        the action is assumed to be triggered by a GPIO event.

    @return
        - 0 if the action is successfully logged or initiated.
        - 1 if an error occurs during execution.

    @details
        - If `debug=True`:
            - Logs the intended shutdown or reboot action but does **not** execute it.
            - If a `stop_button` is provided, logs that a shutdown was prevented.
            - If triggered by a file, logs which file prevented execution.
            - Exits early to avoid executing the system command.
        - If `debug=False`:
            1. If `file_path` and `stop_button` are both `None`:
                - Logs a warning and **exits early** without executing any action.
            2. If `stop_button` is provided:
                - Logs the shutdown initiation event.
                - Disables the button callback to prevent repeated triggers.
            3. Determines the appropriate system command:
                - `shutdown -h now` if `STOP_FILE_PATH` triggered the action.
                - `reboot` if `REBOOT_FILE_PATH` triggered the action.
                - If `file_path` is `None`, assumes shutdown is triggered via GPIO.
            4. Executes the system command **asynchronously** using `subprocess.Popen()`:
                - Uses `"sleep 1 && command"` to introduce a **1-second delay** before execution.
                - Runs the command in a **detached background process** to prevent blocking.
                - Suppresses both standard output and error output.
            5. Logs the initiation of the shutdown or reboot process.

    @raises OSError
        If an issue occurs while executing the shutdown command.
    @raises subprocess.SubprocessError
        If the shutdown process fails to start.

    @note
        - Debug mode (`debug=True`) prevents actual shutdown execution.
        - Ensures `stop_button.when_pressed` is disabled upon initiating shutdown.
        - Uses `sys.exit(1)` if an error occurs, ensuring proper exit handling.
        - The delay (`sleep 1`) helps avoid race conditions with ongoing processes.
        - If `file_path` and `stop_button` are both `None`, the function logs a
          warning and **does nothing**.
    """
    if debug:
        logger.debug(
            "Debug mode active: %s prevented by %s.",
            "Shutdown"
                if file_path == STOP_FILE_PATH
                else "Reboot"
                    if file_path == REBOOT_FILE_PATH
                else "Unknown action",
            file_path or "GPIO event"
        )
        return 0  # Exit early in debug mode

    try:
        if file_path is None and stop_button is None:
            logger.warning("No shutdown trigger detected: Both file_path and stop_button are None.")
            return 0  # Prevent execution if no valid trigger exists
        if stop_button:
            logger.info(
                "Shutdown initiated by pin GPIO%s. Disabling callback.",
                stop_button.pin.number,
            )
            stop_button.when_pressed = None  # Disable the callback

        command_string = None
        action_string = None
        if file_path in (STOP_FILE_PATH, REBOOT_FILE_PATH):
            command_string = "shutdown -h now" if file_path == STOP_FILE_PATH else "reboot"
            action_string = "Shutdown" if file_path == STOP_FILE_PATH else "Reboot"
        else:
            logger.debug("Actions undefined: Unknown file trigger.")
            return 0

        # Fork and detach the shutdown command
        if not command_string:
            logger.error("Invalid command string. No action taken.")
            return 1  # Exit without executing an invalid command
        # pylint: disable-next=consider-using-with
        subprocess.Popen(
            ["sh", "-c", "sleep 1 && " + command_string],  # Shell command with delay
            stdout=subprocess.DEVNULL,  # Suppress standard output
            stderr=subprocess.DEVNULL,  # Suppress error output
            start_new_session=True,  # Detaches the child process
            close_fds=True,  # Ensure no file descriptors are inherited
        )

        logger.info("%s process initiated by '%s'.", action_string, file_path)
        return 0

    except (OSError, subprocess.SubprocessError) as e:
        logger.error("Failed to initiate action: %s", e, exc_info=True)
        sys.exit(1)


class CustomArgumentParser(argparse.ArgumentParser):
    """
    Custom argument parser with enhanced usage and help message formatting.

    This subclass of `argparse.ArgumentParser` customizes:
    - The `Usage:` section by capitalizing the heading.
    - The help message to ensure consistent formatting.
    - The options section for a more structured and readable output.
    """

    def format_usage(self):
        """
        Overrides `format_usage` to capitalize the "Usage:" heading.

        @return str: The formatted usage string.
        """
        return f"Usage: {self.usage}\n"

    def format_help(self):
        """
        Overrides `format_help` to customize the structure of the help message.

        This method:
        - Includes the capitalized "Usage:" section.
        - Appends the parser description.
        - Formats and appends the options section.

        @return str: The fully formatted help message.
        """
        help_text = self.format_usage()
        help_text += f"\n{self.description}\n\n"
        help_text += "Options:\n"
        help_text += self.format_options()
        return help_text

    def format_options(self):
        """
        Formats the options section of the help message.

        This method iterates through all registered argument actions and
        structures them into a readable format.

        @return str: The formatted options section.
        """
        formatter = self._get_formatter()
        for action in self._actions:
            formatter.add_argument(action)
        return formatter.format_help()


def run():
    """
    Main entry point for the shutdown watcher script.

    @details
        - Initializes the logging system with debugging and timestamp options.
        - Deletes any existing shutdown/reboot trigger files at startup.
        - Configures the terminal to suppress Ctrl+C echo for improved UX.
        - Registers cleanup functions to restore terminal settings on exit.
        - Installs signal handlers for SIGINT, SIGTERM, and SIGHUP.
        - Parses and processes command-line arguments to determine script behavior.
        - Manages exception handling and ensures proper script termination.

    @return
        This function does not return; it calls `sys.exit()` to terminate the script.

    @raises RuntimeError
        If a runtime issue occurs during execution.
    @raises OSError
        If a system-related error occurs (e.g., file access issues).
    @raises ValueError
        If invalid arguments or configurations are detected.
    @raises subprocess.SubprocessError
        If a subprocess operation fails.
    @raises KeyboardInterrupt
        If the script is manually interrupted (Ctrl+C).
    @raises SystemExit
        Ensures proper exit handling with the expected exit code.

    @note
        - This function ensures that the shutdown watcher operates correctly
          by handling errors gracefully and cleaning up resources before exit.
        - The terminal settings are restored on script termination.
        - The logger remains active throughout the script execution, even during exceptions.
    """
    try:
        args = process_arguments()
        logger = setup_logger(args.debug, args.date_time)  # Initialize logger

        # Delete semaphores on startup if they exist
        for file_path in (STOP_FILE_PATH, REBOOT_FILE_PATH):
            file = Path(file_path)
            try:
                file.unlink()
                logger.info("Startup: Deleted '%s'.", file)
            except FileNotFoundError:
                pass  # File already deleted, no need to log

        # Store the original terminal settings in a local variable
        original_termios = disable_ctrlc_echo(logger)

        # Register cleanup with the returned original settings
        atexit.register(restore_terminal, original_termios)

        # Register signal handlers
        signal.signal(signal.SIGINT, functools.partial(handle_signal, logger=logger))
        signal.signal(signal.SIGTERM, functools.partial(handle_signal, logger=logger))
        signal.signal(signal.SIGHUP, functools.partial(handle_signal, logger=logger))

        exit_code = handle_arguments(logger, args)
        sys.exit(exit_code)  # Exit gracefully with the returned exit code

    except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as e:
        if logger:
            logger.critical("Unhandled application error: %s", e, exc_info=True)
        else:
            print(f"Critical application error: {e}", file=sys.stderr)
        sys.exit(1)

    except KeyboardInterrupt:
        if logger:
            logger.info("Script interrupted by user (Ctrl+C). Exiting gracefully.")
        sys.exit(130)  # Standard exit code for SIGINT

    except SystemExit as e:
        sys.exit(e.code)  # Maintain exit code consistency


if __name__ == "__main__":
    # Script execution entry point.

    # This block ensures that the script runs `run()` only when executed directly,
    # rather than when imported as a module. It initializes the shutdown watcher
    # process and manages system interactions.
    run()
