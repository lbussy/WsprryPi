#!/usr/bin/python3

# Copyright (C) 2023-2024 Lee C. Bussy (@LBussy)
# Created for WsprryPi version 1.2.1-Beta.1 [devel).

"""Module providing a function to poll a shutdown button and halt the system if pressed."""

import sys
from time import sleep
from os import system, getuid, path, remove

try:
    from gpiozero import Button
except ImportError:
    print("The required library 'gpiozero' is not installed.")
    sys.exit(1)

# Debugging for local work
DEBUG = False

# Check for TAPR shutdown pin
DO_TAPR = True

# Physical pin 35 = BCM19
STOP_PIN = "BOARD35"

# Filename for web-initiated shutdown
STOP_FILE = "/var/www/html/wspr/shutdown"

def is_root():
    """Function checks for root, exits if not UID 0."""
    if getuid() != 0:
        return False
    return True


def main():
    """Function main, handles loop."""
    stop_button = Button(STOP_PIN) # defines the button as an object and chooses GPIO pin
    print(f"\nMonitoring pin {STOP_PIN} for shutdown signal.")
    print("Ctrl-C to quit.\n")

    try:
        while True:
            if (stop_button.is_pressed and DO_TAPR) or path.isfile(STOP_FILE):
                sleep(0.5)
                if stop_button.is_pressed or path.isfile(STOP_FILE):
                    if path.isfile(STOP_FILE):
                        remove(STOP_FILE)
                    if DEBUG:
                        system('wall Shutdown button pressed, system is going down in 60 seconds.')
                        system("shutdown -h")
                        print('\nShutdown initiated.')
                        sleep(30)
                        system('wall Shutdown button pressed, system is going down in 30 seconds.')
                        sleep(20)
                        system('wall Shutdown button pressed, system is going down in 10 seconds.')
                        sleep(9)
                        system('wall Shutdown button pressed, system is going down now.')
                    else:
                        print('\nShutdown initiated.')
                        system('wall Shutdown button pressed, system is going down now.')
                        system("shutdown -h now")
            sleep(0.1)

    except KeyboardInterrupt:
        print('\n\nKeyboard interrupt.')

    finally:
        pass


if __name__ == "__main__":
    if not is_root():
        print("\nScript must be run as root.")
        sys.exit(1)
    else:
        main()
        sys.exit(0)
