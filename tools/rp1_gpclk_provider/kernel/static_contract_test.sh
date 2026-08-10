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
if grep -Eq '(^|[^_])clk_(prepare|enable|prepare_enable)[[:space:]]*\(' "$provider"; then
	echo "provider bypasses the clk-rp1 lease for clock activation" >&2
	exit 1
fi
if grep -Eq 'divider-dma-address|of_property_read.*divider' "$provider"; then
	echo "provider accepts a companion-supplied divider address" >&2
	exit 1
fi
grep -q 'rp1_gpclk_dma_lease_get' "$provider"
grep -q 'rp1_gpclk_dma_lease_enable' "$provider"
grep -q 'rp1_gpclk_dma_lease_disable' "$provider"
grep -q 'pinctrl_select_state' "$provider"
grep -q 'RP1_GPCLK_WRITES_PER_SYMBOL' "$provider"
grep -q 'RP1_GPCLK_TICK_DIVIDER' "$provider"
grep -q 'RP1_GPCLK_WSPR_SYMBOL_COUNT' "$provider"
grep -q 'dmaengine_prep_slave_single' "$provider"
if grep -q 'dmaengine_prep_slave_sg' "$provider"; then
	echo "provider splits the WSPR frame into scheduler-dependent submissions" >&2
	exit 1
fi
echo "RP1 GPCLK kernel static contract tests passed"
