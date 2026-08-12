# Phase 6J: continuous WSPR sequence and scheduler integration

## Outcome

The BCM2712/RP1 engineering provider now accepts exactly one complete
162-symbol WSPR sequence and expands it into a bounded 43,281,216-byte coherent
buffer. The provider prepares that buffer as one finite DMA-engine submission;
the RP1 DW AXI DMA driver creates the hardware descriptor chain, so no symbol
boundary depends on userspace or workqueue scheduling.

The WsprryPi scheduler selects the RP1 provider backend for GPIO transmission
on Raspberry Pi 5, Pi 500, and CM5. Pi 4 and earlier retain the existing GPIO
backend, and Si5351 selection is unchanged. The RP1 path accepts GPIO4 only and
requires `/dev/rp1-gpclk0` to be readable and writable before configuration is
accepted.

## Provider and UAPI contract

- UAPI version and exact structure-size checks remain mandatory.
- A program contains four logical divider plans plus an ordered 162-byte tone
  index array. Short, oversized, malformed, or stale-generation requests fail.
- The provider owns divider packing, the clock lease, DMA submission, GPIO4
  pinctrl and drive selection, and restoration to the safe input state.
- STOP changes RUNNING to DRAINING and does not truncate the already-linked
  finite frame.
- Closing the active owner defers lease release until the frame and terminal
  cleanup finish. A second owner remains excluded during that interval.
- `live_output` remains a read-only, load-time gate and defaults to disabled.

Testing established that `DMA_TICK_FINISH_CLEAR` stops this long hardware
descriptor chain before completion. The accepted design omits that bit and
stops the tick synchronously from the DMA completion callback before scheduling
the delayed final-divider verification. A four-entry scatter-gather design was
also rejected because execution stalled after its first entry.

## Clock-disabled timing evidence

With `live_output=N`, exact 162-symbol frames completed in approximately
110.70 seconds. Diagnostic prefix lengths of 1, 4, 32, 64, 128, and 162 symbols
completed in 0.802, 2.803, 21.921, 43.840, 87.482, and 110.699 seconds,
respectively. That monotonic DMA completion evidence is consistent with a
continuously paced finite chain; it is not RF or on-pin continuity evidence.

Exact-contract STOP requests made near 0.15, 55.08, and 109.10 seconds all
reached COMPLETE at approximately 110.70 seconds. Closing the owner at 1.03
seconds held ownership through the same finite drain; a later owner acquired
the device and received the expected `EINVAL` for a deliberately short frame.
After every terminal check GPIO4 was an input and GPCLK0 prepare/enable counts
were zero.

The clock-disabled run cannot exercise active GPCLK lease exclusion because the
provider deliberately never enables the clock in that mode. The lease contract
remains covered by the Phase 6H live-gated conflict test and the Phase 6J source,
patch-application, and KUnit checks.

## Scheduler and power selection

`WsprRp1GpclkBackend` accepts only a standard contiguous 162-event WSPR plan,
maps its four frequencies to provider tone indexes, submits once, polls terminal
state, and uses finite STOP/drain cleanup. Quiesce acquires and releases the
provider without submitting a program.

The existing persisted `GPIO.RP1 Drive mA` value is now consumed by the Pi 5
backend. Valid values are 2, 4, 8, and 12 mA; the default is 2 mA. It is also
selectable with `--rp1-gpio-drive-ma`, while generic `--power-level` maps to
that drive value on Pi 5. These are pad-drive selections, not calibrated RF
power levels.

## Compatibility and limits

This remains limited to the Raspberry Pi OS 64-bit BCM2712 `rpi-2712` kernel
used by Raspberry Pi 5, Pi 500, and CM5. It is not an upstream or portable
kernel interface. No web UI was changed. Operator UI exposure and corresponding
operator documentation remain follow-up work; CLI and persisted JSON provide
the engineering control meanwhile.

No live full WSPR frame was transmitted in Phase 6J. The next gate is one
separately authorized minimum-drive live frame with an attached SDR, followed
by verification of uninterrupted four-tone cadence, relative spectrum, STOP
drain, and safe cleanup. Decode qualification remains a later gate.
