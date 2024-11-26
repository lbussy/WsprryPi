#!/usr/bin/python3

"""
Shutdown Watcher for Raspberry Pi.

This script monitors a shutdown button (via GPIO pin) and initiates a system shutdown
if the button is pressed or if a specific file (`STOP_FILE`) is created in the system.
It also handles graceful shutdowns, logging, and debugging options.

Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)

Created for WsprryPi project, version 1.2.1-7f4c707 [refactoring].
"""

import sys
import argparse
import logging
from time import sleep
from os import system, getuid, path, remove
import signal
from gpiozero import Button


def parse_args():
    """
    Parse command-line arguments.

    This function processes the command-line arguments, specifically enabling
    debug mode through the `--debug` flag. If no flags are provided, normal mode
    will be used.

    Returns:
        argparse.Namespace: Parsed arguments with flags like `debug`.
    """
    parser = argparse.ArgumentParser(description="Shutdown Watcher for Raspberry Pi")
    parser.add_argument('--debug', action='store_true', help="Enable debug mode")
    return parser.parse_args()


def is_root():
    """
    Check if the script is run as the root user.

    This function ensures that the script is executed with root privileges (UID 0).
    If the user is not root, an error is logged and the script exits.

    Returns:
        bool: True if the script is run as root, False otherwise.
    """
    if getuid() != 0:
        logger.error("Script must be run as root.")
        return False
    return True


def cleanup(signal, frame):
    """
    Gracefully terminate the script on shutdown signal.

    Handles cleanup when the script receives a termination signal (SIGINT, SIGTERM).
    Logs the shutdown event and exits the program.

    Parameters:
        signal (int): The signal number.
        frame (frame): The current stack frame.
    """
    logger.info("Shutdown watch script terminating.")
    sys.exit(0)


def main():
    """
    Main function to monitor the shutdown button and trigger system shutdown.

    The function monitors the GPIO pin and the `STOP_FILE`. If the shutdown button is pressed
    or the `STOP_FILE` is created, the system will initiate a shutdown with a countdown, logging
    the event along the way.

    The loop runs indefinitely until the script is terminated or interrupted.
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
                        system("shutdown -h")
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
        pass


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
