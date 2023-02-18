#!/usr/bin/python3
# Created for Wsprry Pi version 0.0.1

from gpiozero import Button
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
    stopButton = Button(stopPin) # defines the button as an object and chooses GPIO pin
    print("\nMonitoring pin {} for shutdown signal.".format(stopPin))
    print("Ctrl-C to quit.\n")

    try:
        while (True):
            if stopButton.is_pressed:
                sleep(0.5)
                if stopButton.is_pressed:
                    system('wall Shutdown button pressed, system is going down in 60 seconds.')
                    system("shutdown -h")
                    print('\nShutdown initiated.')
                    sleep(30)
                    system('wall Shutdown button pressed, system is going down in 30 seconds.')
                    sleep(20)
                    system('wall Shutdown button pressed, system is going down in 10 seconds.')
                    sleep(9)
                    system('wall Shutdown button pressed, system is going down now.')

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
