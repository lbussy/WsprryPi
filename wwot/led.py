#!/usr/bin/python3

from gpiozero import LED
from time import sleep
from os import system, getuid
from sys import stdout, exit

# Physical pin 12 = BCM18
ledPin = "BOARD12"


def main():
    led = LED(ledPin)
    print("\nBlinking pin {}.".format(ledPin))
    print("Ctrl-C to quit.\n")

    try:
        while True:
            led.on()
            sleep(1)
            led.off()
            sleep(1)

    except KeyboardInterrupt:
        print('\n\nKeyboard interrupt.')

    finally:
        pass

    return


if __name__ == "__main__":
    main()
    exit(0)
