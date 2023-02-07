#!/usr/bin/python3
# Created for Wsprry Pi version 0.1

from time import sleep
from os import system, getuid
from sys import stdout, exit

# Physical pin 35 = BCM19
stopPin = "BOARD35"

def isRoot():
    if getuid() != 0:
        return False
    else:
        return True


def main():
    print("\nMonitoring pin {} for shutdown signal.".format(stopPin))
    print("Ctrl-C to quit.\n")

    try:
        while (True):
            sleep(0.1)

    except KeyboardInterrupt:
        print('\n\nKeyboard interrupt.')

    finally:
        pass

    return


if __name__ == "__main__":
    if not (isRoot()):
        print("\nScript must be run as root.")
        exit(1)
    else:
        main()
        exit(0)
