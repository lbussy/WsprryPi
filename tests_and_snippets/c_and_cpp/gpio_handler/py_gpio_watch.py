#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import RPi.GPIO as GPIO
import time

# GPIO pins
INPUT_PIN = 19  # GPIO19 as input with pull-up
OUTPUT_PIN = 18 # GPIO18 as output

# Setup GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

# Configure GPIO pins
GPIO.setup(INPUT_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(OUTPUT_PIN, GPIO.OUT)
GPIO.output(OUTPUT_PIN, GPIO.LOW)

try:
    print("Monitoring GPIO19. GPIO18 will toggle HIGH when GPIO19 goes LOW.")
    while True:
        if GPIO.input(INPUT_PIN) == GPIO.LOW:
            GPIO.output(OUTPUT_PIN, GPIO.HIGH)
            print("GPIO19 is LOW. GPIO18 set to HIGH.")
        else:
            GPIO.output(OUTPUT_PIN, GPIO.LOW)
        time.sleep(0.1)
except KeyboardInterrupt:
    print("\nMonitoring stopped by user.")
finally:
    GPIO.cleanup()
    print("GPIO cleaned up.")
