#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# This file is part of WsprryPi.
#
# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
#
# WsprryPi is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

"""
@file shutdown_watch.py
@brief Script to monitor GPIO for shutdown signals on a Raspberry Pi.

This script monitors a GPIO pin and a STOP_FILE to detect shutdown requests. If a shutdown
signal is detected via a button press or the presence of the STOP_FILE, it initiates a system
shutdown. It also supports debug mode for detailed logging.

@note This script must be run with root privileges to access GPIO and shutdown functionalities.
"""

import sys
import argparse
import logging
from time import sleep
from os import system, getuid, path, remove
import signal
from gpiozero import Button

# Constants
STOP_PIN = 17  # GPIO pin for the shutdown button
STOP_FILE = "/tmp/stop"  # Path to the STOP_FILE
DO_TAPR = True  # Enable shutdown on button press

# Logger setup
logger = None
DEBUG = False


def parse_args():
    """
    @brief Parse command-line arguments.

    Processes the `--debug` flag from the command line to enable debug logging.

    @return argparse.Namespace: Parsed arguments with the `debug` flag.
    """
    parser = argparse.ArgumentParser(description="Shutdown Watcher for Raspberry Pi")
    parser.add_argument('--debug', action='store_true', help="Enable debug mode")
    return parser.parse_args()


def is_root():
    """
    @brief Check if the script is run as the root user.

    Verifies that the script is executed with root privileges (UID 0). Logs an error if the
    user is not root and exits the script.

    @return bool: True if the script is run as root, False otherwise.
    """
    if getuid() != 0:
        logger.error("Script must be run as root.")
        return False
    return True


def cleanup(signal, frame):
    """
    @brief Gracefully terminate the script on receiving a termination signal.

    Handles cleanup tasks when the script receives a termination signal (e.g., SIGINT or SIGTERM).
    Logs the termination event and exits the program.

    @param signal (int): The signal number.
    @param frame (frame): The current stack frame.
    """
    logger.info("Shutdown watch script terminating.")
    sys.exit(0)


def main():
    """
    @brief Main function to monitor the shutdown button and the STOP_FILE.

    Monitors the specified GPIO pin (`STOP_PIN`) for button presses and the `STOP_FILE` for shutdown requests.
    If a shutdown signal is detected, initiates a system shutdown with appropriate logging.

    The loop runs indefinitely until terminated via a signal (e.g., Ctrl-C) or another interruption.
    """
    stop_button = Button(STOP_PIN)  # Initialize GPIO button
    logger.info(f"Monitoring pin {STOP_PIN} for shutdown signal.")
    logger.info("Press Ctrl-C to quit.\n")

    try:
        while True:
            # Check for shutdown button press or STOP_FILE existence
            if (stop_button.is_pressed and DO_TAPR) or path.isfile(STOP_FILE):
                sleep(0.5)
                if stop_button.is_pressed or path.isfile(STOP_FILE):
                    if path.isfile(STOP_FILE):
                        remove(STOP_FILE)  # Remove the STOP_FILE after detection
                    shutdown_msg = "Shutdown initiated by button press or web request."
                    if DEBUG:
                        system(f'wall {shutdown_msg} The system will shut down in 60 seconds.')
                        logger.info(f'{shutdown_msg} The system is going down in 60 seconds.')
                        sleep(30)
                        system('wall Shutdown button pressed, system is going down in 30 seconds.')
                        sleep(20)
                        system('wall Shutdown button pressed, system is going down in 10 seconds.')
                        sleep(9)
                        system('wall Shutdown button pressed, system is going down now.')
                    else:
                        logger.info(shutdown_msg)
                        system(f'wall {shutdown_msg}')
                        system("shutdown -h now")
            sleep(0.1)

    except KeyboardInterrupt:
        logger.info('Shutdown watch script interrupted by keyboard.')

    finally:
        logger.info("Shutdown watch script exiting.")


if __name__ == "__main__":
    # Parse arguments and set debug flag
    args = parse_args()
    DEBUG = args.debug

    # Configure logging
    logging.basicConfig(level=logging.DEBUG if DEBUG else logging.INFO,
                        format='%(asctime)s - %(levelname)s - %(message)s')
    logger = logging.getLogger()

    # Register termination signals
    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    # Run the main function if the user is root
    if not is_root():
        sys.exit(1)
    else:
        main()
        sys.exit(0)
