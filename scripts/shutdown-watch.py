#!/usr/bin/python3
# Created for WsprryPi version 1.1.0

from gpiozero import Button
from time import sleep
from os import system, getuid, path, remove
from sys import stdout, exit

# Debugging for local work
debug = False

# Check for TAPR shutdown pin
doTAPR = True

# Physical pin 35 = BCM19
stopPin = "BOARD35"

# Filename for web-initiated shutdown
stopFile = "/var/www/html/wspr/shutdown"

def isRoot():
    if getuid() != 0:
        return False
    else:
        return True


def main():
    stopButton = Button(stopPin) # defines the button as an object and chooses GPIO pin
    print("\nMonitoring pin {} for shutdown signal.".format(stopPin))
    print("Ctrl-C to quit.\n")

    try:
        while (True):
            if (stopButton.is_pressed and doTAPR) or path.isfile(stopFile):
                sleep(0.5)
                if stopButton.is_pressed or path.isfile(stopFile):
                    if path.isfile(stopFile):
                        remove(stopFile)
                    if (debug):
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

    return


if __name__ == "__main__":
    if not (isRoot()):
        print("\nScript must be run as root.")
        exit(1)
    else:
        main()
        exit(0)
