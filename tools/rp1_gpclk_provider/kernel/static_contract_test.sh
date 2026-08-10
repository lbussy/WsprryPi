#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
client="$root/src/WSPR-Transmitter/src/rp1_gpclk_linux_provider.cpp"
uapi="$root/src/WSPR-Transmitter/src/rp1_gpclk_uapi.h"
provider="$root/tools/rp1_gpclk_provider/kernel/rp1_gpclk_provider.c"

if grep -Eq '0x[0-9a-fA-F]{5,}|/dev/mem' "$client" "$uapi"; then
	echo "userspace exposes an RP1 address" >&2
	exit 1
fi
if grep -Eq 'divider_dma_addr|DIV_FRAC' "$client" "$uapi"; then
	echo "userspace exposes provider register representation" >&2
	exit 1
fi
if grep -Eq 'clk_prepare|clk_enable|clk_prepare_enable|pinctrl|gpio.*drive' "$provider"; then
	echo "clock-disabled provider contains output-enabling code" >&2
	exit 1
fi
if grep -Eq 'divider-dma-address|of_property_read.*divider' "$provider"; then
	echo "provider accepts a companion-supplied divider address" >&2
	exit 1
fi
grep -q 'rp1_gpclk_dma_lease_get' "$provider"
grep -q 'RP1_GPCLK_WRITES_PER_SYMBOL' "$provider"
grep -q 'RP1_GPCLK_TICK_DIVIDER' "$provider"
echo "RP1 GPCLK kernel static contract tests passed"
