#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
client="$root/src/WSPR-Transmitter/src/rp1_gpclk_linux_provider.cpp"
uapi="$root/src/WSPR-Transmitter/src/rp1_gpclk_uapi.h"
provider="$root/tools/rp1_gpclk_provider/kernel/rp1_gpclk_provider.c"
patch4="$root/tools/rp1_gpclk_provider/kernel/0004-rp1-gpclk-enable-live-finite-events.patch"

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
grep -q 'provider->generation = 0' "$provider"
grep -q 'rp1_gpclk_dma_lease_enable' "$provider"
grep -q 'rp1_gpclk_dma_lease_disable' "$provider"
grep -q 'pinctrl_select_state' "$provider"
grep -q 'RP1_GPCLK_WRITES_PER_SYMBOL' "$provider"
grep -q 'RP1_GPCLK_TICK_DIVIDER' "$provider"
grep -q 'RP1_GPCLK_WSPR_SYMBOL_COUNT' "$provider"
grep -q 'rp1_gpclk_valid_frame_elapsed' "$provider"
grep -q 'dmaengine_prep_slave_single' "$provider"
grep -q 'RP1_GPCLK_IOC_SUBMIT_EVENTS' "$provider"
grep -q 'RP1_GPCLK_IOC_EVENT_STATE' "$provider"
grep -q 'hrtimer_start' "$provider"
grep -q 'HRTIMER_MODE_ABS' "$provider"
grep -q 'HRTIMER_MODE_ABS_SOFT' "$provider"
grep -q 'rp1_gpclk_dma_lease_set_output' "$provider"
grep -q 'prepare_event_words' "$provider"
grep -q 'event_cleanup_work' "$provider"
grep -q 'RP1_GPCLK_TERMINAL_DEADLINE_MISSED' "$provider"
if grep -Eq 'event_deadline.*(pinctrl_select_state|clk_prepare|clk_enable)' "$provider"; then
	echo "event deadline callback invokes a sleeping output API" >&2
	exit 1
fi
if grep -q 'dmaengine_prep_slave_sg' "$provider"; then
	echo "provider splits the WSPR frame into scheduler-dependent submissions" >&2
	exit 1
fi
remove_body=$(sed -n '/^static void rp1_gpclk_provider_remove(/,/^}/p' "$provider")
stop_line=$(printf '%s\n' "$remove_body" | grep -n 'stop_event_program' | cut -d: -f1)
cancel_line=$(printf '%s\n' "$remove_body" | grep -n 'cancel_work_sync.*event_cleanup_work' | cut -d: -f1)
if [ -z "$stop_line" ] || [ -z "$cancel_line" ] || [ "$stop_line" -ge "$cancel_line" ]; then
	echo "provider removal does not quiesce the event timer before draining cleanup work" >&2
	exit 1
fi
submit_body=$(sed -n '/^static int submit_event_program(/,/^}/p' "$provider")
safe_line=$(printf '%s\n' "$submit_body" | grep -n 'pinctrl_select_state(provider->pinctrl, provider->safe_state)' | head -1 | cut -d: -f1)
enable_line=$(printf '%s\n' "$submit_body" | grep -n 'rp1_gpclk_dma_lease_enable' | head -1 | cut -d: -f1)
gate_off_line=$(printf '%s\n' "$submit_body" | grep -n 'rp1_gpclk_dma_lease_set_output(&provider->lease, false)' | head -1 | cut -d: -f1)
drive_line=$(printf '%s\n' "$submit_body" | grep -n 'provider->drive_states' | head -1 | cut -d: -f1)
armed_line=$(printf '%s\n' "$submit_body" | grep -n 'provider->event_armed = true' | head -1 | cut -d: -f1)
dma_line=$(printf '%s\n' "$submit_body" | grep -n 'dma_async_issue_pending' | head -1 | cut -d: -f1)
timer_line=$(printf '%s\n' "$submit_body" | grep -n 'hrtimer_start' | head -1 | cut -d: -f1)
if [ "$safe_line" -ge "$enable_line" ] ||
	[ "$enable_line" -ge "$gate_off_line" ] ||
	[ "$gate_off_line" -ge "$drive_line" ] ||
	[ "$drive_line" -ge "$armed_line" ] ||
	[ "$armed_line" -ge "$dma_line" ] ||
	[ "$dma_line" -ge "$timer_line" ]; then
	echo "live event startup can expose output before its defined start boundary" >&2
	exit 1
fi
if printf '%s\n' "$submit_body" | grep -q 'provider->dma_tick + DMA_TICK0_EN'; then
	echo "submission enables tick requests before the armed start callback" >&2
	exit 1
fi
deadline_body=$(sed -n '/^static enum hrtimer_restart event_deadline(/,/^}/p' "$provider")
grep -q 'provider->event_armed' <<EOF
$deadline_body
EOF
grep -q 'provider->dma_tick + DMA_TICK0_EN' <<EOF
$deadline_body
EOF
grep -q 'gate_event_output' <<EOF
$deadline_body
EOF
grep -q 'ktime_add_ns(provider->event_deadline' <<EOF
$deadline_body
EOF
start_gate_line=$(printf '%s\n' "$deadline_body" | grep -n 'ret = gate_event_output' | head -1 | cut -d: -f1)
barrier_line=$(printf '%s\n' "$deadline_body" | grep -n 'wmb()' | head -1 | cut -d: -f1)
tick_line=$(printf '%s\n' "$deadline_body" | grep -n 'provider->dma_tick + DMA_TICK0_EN' | head -1 | cut -d: -f1)
if [ -z "$start_gate_line" ] || [ -z "$barrier_line" ] || [ -z "$tick_line" ] ||
	[ "$start_gate_line" -ge "$barrier_line" ] || [ "$barrier_line" -ge "$tick_line" ]; then
	echo "armed start does not publish the first gate before tick requests" >&2
	exit 1
fi
grep -q 'if (gate_event_output(provider,' <<EOF
$deadline_body
EOF
grep -q 'return fail_event_boundary(provider, flags,' <<EOF
$deadline_body
EOF
if printf '%s\n' "$deadline_body" | grep -Eq 'pinctrl_select_state|dmaengine_terminate_sync|mutex_(lock|unlock)|clk_(prepare|enable|disable)|k[mz]?alloc'; then
	echo "event hrtimer callback invokes a sleeping operation" >&2
	exit 1
fi
gate_patch=$(sed -n '/^+int rp1_gpclk_dma_lease_set_output(/,/^+}/p' "$patch4")
grep -q 'clockman->regs + CLK_GP0_CTRL' <<EOF
$gate_patch
EOF
grep -q 'readl(ctrl_reg)' <<EOF
$gate_patch
EOF
grep -q 'writel(ctrl, ctrl_reg)' <<EOF
$gate_patch
EOF
if printf '%s\n' "$gate_patch" | grep -Eq 'regs_lock|regmap|GPCLK_OE_CTRL|spin_lock|mutex'; then
	echo "deadline gate enters a shared or sleeping RP1 clock domain" >&2
	exit 1
fi
echo "RP1 GPCLK kernel static contract tests passed"
