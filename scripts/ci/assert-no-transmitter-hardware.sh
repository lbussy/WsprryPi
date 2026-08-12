#!/bin/sh
set -eu

if [ "$#" -eq 0 ]; then
    echo "usage: $0 TRACE_FILE..." >&2
    exit 2
fi

pattern='/dev/(gpiomem|mem|vcio|i2c-[^" ]*|gpiochip[^" ]*|rp1-gpclk[^" ]*)|/sys/class/gpio|/sys/kernel/debug|/dev/mailbox|/dev/dma'

if grep -En "$pattern" "$@"; then
    echo "Prohibited transmitter hardware access was observed." >&2
    exit 1
fi

echo "No prohibited transmitter hardware access observed."

