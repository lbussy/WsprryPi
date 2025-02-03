#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# -----------------------------------------------------------------------------
# @file shutdown_watch.py
# @brief Poll a GPIO pin and initiate shutdown when puled low.
#
# @author Lee C. Bussy <Lee@Bussy.org>
# @version 1.0.0
# @date 2025-02-03
# @copyright MIT License
#
# @license
# MIT License
#
# Copyright (c) 2023-2025 Lee C. Bussy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# @usage
# ./get_semantic_version.sh
#
# @returns
# Returns a correctly formatted sematic version string.  Defaults to
# declarations in header.
#
# -----------------------------------------------------------------------------

"""
@file shutdown_watch.py
@brief Monitor GPIO and file signals for a shutdown request on a Raspberry Pi.

This script monitors a GPIO pin and a STOP_FILE to detect shutdown requests.
If a shutdown signal is detected via a button press or the presence of the
STOP_FILE, it initiates a system shutdown. It also supports a debug mode
to test behavior without actually shutting down the system.

@note This script must be run with root privileges to access GPIO and 
shutdown functionalities.
"""

import sys
import argparse
import logging
import os
import time
import signal
import termios
import subprocess

try:
    import gpiozero
except ImportError:
    sys.exit("Failed to import gpiozero. Ensure it is installed and available.")

# Version
__version__ = "1.2.3"

# Global Constants
STOP_PIN = 19           # GPIO pin for the shutdown button
STOP_FILE = "/tmp/stop" # Path to the STOP_FILE

# Global Variables
logger = None           # Logger instance
stop_button = None      # GPIO resource
original_termios = None # Handle terminal I/O


def get_script_name():
    """
    Get the name of the script being executed.

    @return str: The name of the script.
    """
    return os.path.basename(__file__)


def is_root():
    """
    Check if the script is running with root privileges.

    @return bool: True if the script is running as root, False otherwise.
    """
    if os.getuid() != 0:
        logger.error("Script must be run as root.")
        return False
    return True


def initiate_shutdown():
    """
    Forks a shutdown command into a detached child process.
    """
    try:
        # Use Popen to fork and detach the shutdown command
        subprocess.Popen(
            ["sh", "-c", "sleep 1 && shutdown -h now"], # Shell command with delay
            stdout=subprocess.DEVNULL,                  # Suppress standard output
            stderr=subprocess.DEVNULL,                  # Suppress error output
            preexec_fn=os.setsid,                       # Detach from parent process
        )
    except Exception as e:
        logger.error(f"Failed to initiate shutdown: {e}")


def cleanup(signal_received=None, frame=None):
    """
    Gracefully terminate the script and clean up resources.

    - Closes GPIO resources.
    - Logs the termination event.
    - Exits the script cleanly.

    @param signal_received (int): The signal number received.
    @param frame (frame): The current stack frame.
    """
    global stop_button
    logger.info("Cleaning up resources...")

    if stop_button is not None:
        stop_button.close()  #: Release GPIO resources
        logger.info("GPIO resources released.")

    logger.info("Shutdown watch script terminating.")
    sys.exit(0)


def watch(debug, daemon):
    """
    Monitor the GPIO pin and STOP_FILE for shutdown requests.

    Continuously checks for a button press or the presence of the STOP_FILE.
    If a shutdown signal is detected, it initiates a system shutdown.

    @param debug (bool): Flag indicating whether to run in debug mode.
    """
    global stop_button

    try:
        stop_button = gpiozero.Button(STOP_PIN)
    except Exception as e:
        logger.error(f"Failed to initialize GPIO pin {STOP_PIN}: {e}")
        cleanup()

    logger.info(f"Monitoring pin {STOP_PIN} for shutdown signal.")
    if not daemon: logger.info("Press Ctrl-C to quit.")

    try:
        while True:
            # Check for shutdown signals
            if stop_button.is_pressed or os.path.isfile(STOP_FILE):
                if os.path.isfile(STOP_FILE):
                    os.remove(STOP_FILE)

                logger.info(f"Shutdown initiated by button press on pin {STOP_PIN} or presence of {STOP_FILE}.")

                if debug: logger.debug("Debug mode enabled. No action will be taken.")
                else:
                    initiate_shutdown() # Fork a process to shutdown in 1 second
                cleanup()               # Hurry and cleanup during that one second

            time.sleep(0.1)  #: Poll interval

    except KeyboardInterrupt:
        logger.info("Shutdown watch script interrupted by keyboard.")


def setup_logger(debug, daemon):
    """
    Configure the logger for the script.

    The logger can operate in debug or normal mode based on the debug parameter.
    If daemon mode is enabled, logs will include a date/time stamp.

    @param debug (bool): Flag indicating whether to run in debug mode.
    @param daemon (bool): Flag indicating whether to include date/time stamps in logs.
    """
    global logger
    log_level = logging.DEBUG if debug else logging.INFO
    log_format = (
        "%(asctime)s - %(levelname)s - %(message)s" if daemon
        else "%(levelname)s - %(message)s"
    )
    logging.basicConfig(level=log_level, format=log_format)
    logger = logging.getLogger()


def register_signals():
    """
    Register handlers for relevant signals to suppress unwanted terminal outputs.
    """
    signals_to_handle = [
        signal.SIGINT,   # Ctrl+C
        signal.SIGTERM,  # Termination signal
        signal.SIGHUP,   # Terminal hang-up
        signal.SIGUSR1,  # User-defined signal 1
        signal.SIGUSR2   # User-defined signal 2
    ]
    for sig in signals_to_handle:
        signal.signal(sig, handle_signal)


def disable_ctrlc_echo():
    """
    Disable the terminal's default echo of `^C` when SIGINT is received.
    """
    global original_termios
    original_termios = termios.tcgetattr(sys.stdin)
    new_termios = termios.tcgetattr(sys.stdin)
    new_termios[3] = new_termios[3] & ~termios.ECHO  # Disable ECHO
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, new_termios)


def restore_terminal():
    """
    Restore the terminal settings to their original state.
    """
    if original_termios is not None:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, original_termios)


def handle_signal(signal_received, frame):
    """
    Custom signal handler to capture and suppress terminal outputs.

    @param signal_received (int): The signal number received.
    @param frame (frame): The current stack frame.
    """
    signal_names = {
        signal.SIGINT: "SIGINT (Ctrl+C)",
        signal.SIGTERM: "SIGTERM",
        signal.SIGHUP: "SIGHUP"
    }
    signal_name = signal_names.get(signal_received, f"Signal {signal_received}")
    logger.info(f"Received {signal_name}. Exiting gracefully.")
    restore_terminal()  # Ensure terminal settings are restored
    cleanup()


class CustomArgumentParser(argparse.ArgumentParser):
    """
    Custom argument parser with enhanced usage and help formatting.
    """

    def format_usage(self):
        """
        Override the format_usage method to capitalize the "Usage:" wording.
        """
        return f"Usage: {self.usage}\n"

    def format_help(self):
        """
        Override the format_help method to ensure the usage line in help output
        also uses the capitalized "Usage:".
        """
        help_text = self.format_usage()
        help_text += f"\n{self.description}\n\n"
        help_text += "Options:\n"
        help_text += self.format_options()
        return help_text

    def format_options(self):
        """
        Format the options section of the help message.
        """
        formatter = self._get_formatter()
        for action in self._actions:
            formatter.add_argument(action)
        return formatter.format_help()


def process_arguments():
    """
    Handle command-line arguments.

    @return argparse.Namespace: Parsed arguments.
    """
    parser = CustomArgumentParser(
        description="Shutdown Watcher for Raspberry Pi.",
        formatter_class=argparse.RawTextHelpFormatter,
        usage=f"{get_script_name()} [-h] [-d] [-D] [-w] [-v]",
        add_help=False,
    )
    parser.add_argument(
        "-h", "--help",
        action="help",
        help="Display this help message and exit."
    )
    parser.add_argument(
        "-d", "--debug",
        action="store_true",
        help="Enable debug mode. Logs actions but takes no shutdown actions."
    )
    parser.add_argument(
        "-D", "--daemon", 
        action="store_true",
        help="Enable daemon mode. Output/logs will have date/time stamp."
    )
    parser.add_argument(
        "-w", "--watch",
        action="store_true",
        help="Start monitoring for shutdown signals and take action."
    )
    parser.add_argument(
        "-v", "--version",
        action="version",
        version=f"%(prog)s: {__version__}",
        help="Show the script version and exit."
    )
    return parser.parse_args()


def handle_arguments():
    """
    Process the parsed arguments and execute the appropriate functionality.
    """
    # Get arguments
    args = process_arguments()
    setup_logger(args.debug, args.daemon)  # Pass the daemon flag

    if args.watch:
        if not is_root(): sys.exit(-1)
        if args.debug: logger.info("Debug mode enabled. No actions will be taken.")
        logger.info("Starting shutdown watcher...")
        watch(args.debug, args.daemon)
    else:
        print("At least '-w' is required to enter loop.")
        print("Use -h or --help to see available options.")


def main():
    """
    Main entry point of the script.

    Parses command-line arguments, sets up logging, and initiates monitoring.
    """
    handle_arguments()


if __name__ == "__main__":
    # Register signal handlers
    import atexit
    atexit.register(restore_terminal)
    register_signals()
    disable_ctrlc_echo()
    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)
    signal.signal(signal.SIGHUP, handle_signal)

    # Run the main function
    main()
