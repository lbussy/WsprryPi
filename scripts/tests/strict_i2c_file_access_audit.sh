#!/usr/bin/env bash
set -euo pipefail

binary=${1:?binary path required}
audit_bus=2147483646
expected_i2c_path="/dev/i2c-${audit_bus}"
trace_file=$(mktemp)
output_file=$(mktemp)
trap 'rm -f "$trace_file" "$output_file"' EXIT

if [ "$(id -u)" -eq 0 ]; then
    echo "strict I2C file-access audit must run as a non-root account" >&2
    exit 1
fi

if [ -e "$expected_i2c_path" ]; then
    echo "audit requires a nonexistent I2C path: $expected_i2c_path" >&2
    exit 1
fi

if strace -f -e trace=open,openat,ioctl,read,write \
    -o "$trace_file" \
    "$binary" --no-web --backend si5351 \
        --si5351-i2c-bus "$audit_bus" AA0NT EM18 20 20m \
        >"$output_file" 2>&1
then
    echo "strict I2C audit invocation unexpectedly succeeded" >&2
    exit 1
fi

if ! grep -E \
    "open(at)?\\(.*\"${expected_i2c_path}\", O_RDWR\\|O_CLOEXEC\\) = -1 ENOENT" \
    "$trace_file" >/dev/null
then
    echo "trace did not contain the expected failed I2C open: $expected_i2c_path" >&2
    cat "$trace_file" >&2
    exit 1
fi

observed_i2c_paths=$(grep -Eo '/dev/i2c-[0-9]+' "$trace_file" | sort -u)
if [ "$observed_i2c_paths" != "$expected_i2c_path" ]; then
    echo "trace contained an unexpected I2C path" >&2
    printf '%s\n' "$observed_i2c_paths" >&2
    exit 1
fi

if grep -E \
    '/dev/(mem|gpiomem|vcio|gpiochip[0-9]*|rp1-gpclk[0-9]*)|/sys/bus/platform/devices/[^ ]*/resource[0-9]*' \
    "$trace_file" >/dev/null
then
    echo "trace contained a forbidden GPIO, mailbox, MMIO, or RP1 path" >&2
    grep -E \
        '/dev/(mem|gpiomem|vcio|gpiochip[0-9]*|rp1-gpclk[0-9]*)|/sys/bus/platform/devices/[^ ]*/resource[0-9]*' \
        "$trace_file" >&2
    exit 1
fi

if grep -F "must be run as root" "$output_file" >/dev/null; then
    echo "strict I2C audit was rejected by the legacy root gate" >&2
    cat "$output_file" >&2
    exit 1
fi

grep -F "Open failed for $expected_i2c_path" "$output_file" >/dev/null || {
    echo "missing selected-I2C-path failure diagnostic" >&2
    cat "$output_file" >&2
    exit 1
}

if grep -E 'ioctl\([^,]+, (I2C_|0x070)' "$trace_file" >/dev/null; then
    echo "audit unexpectedly reached an I2C ioctl" >&2
    grep -E 'ioctl\([^,]+, (I2C_|0x070)' "$trace_file" >&2
    exit 1
fi

echo "strict I2C file-access audit passed: $expected_i2c_path only"
