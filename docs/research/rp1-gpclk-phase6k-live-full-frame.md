# Phase 6K: minimum-drive live full-frame qualification

> Phase 6L correction: the 110.699-second value below was terminal UAPI
> observation latency, not DMA cadence. Direct tick-enable-to-DMA-callback
> instrumentation measured 110.58857 seconds, approximately -31 ppm from
> nominal and inside the subsequently enforced +/-6.75 ms timing window. See
> `rp1-gpclk-phase6l-cadence-verification.md`.

## Outcome

One scheduler-originated 162-symbol WSPR frame completed on `wspr5.local`
through the BCM2712/RP1 provider, GPIO4 radiator, and minimum 2 mA pad-drive
selection. The SDR trace contains continuous energy at all 161 symbol
boundaries and four ordered tone levels. Provider cleanup restored GPIO4 to an
input, GPCLK0 to zero prepare/enable counts, `live_output=N`, and both the
WsprryPi and SoapyRemote services to active.

This phase passes the minimum-drive live continuity and cleanup gate. It also
identifies a provider pacing error that must be corrected before decode
qualification: the measured provider frame is 110.699024 seconds, 107.024 ms
or approximately 968 ppm longer than the nominal 110.592-second WSPR frame.

## Executed path

- Parent commit: `984e66325e4f5275251b0fe83d8c76ba60334808`
- WSPR-Transmitter commit: `3d28b204e386d66fe080274fe107c23fae2f189c`
- Kernel: `6.18.44-v8-16k+ #3`, BCM2712 `rpi-2712`
- Scheduler: `wsprrypi_debug`, GPIO backend, one 20 m frame
- Identity payload: `AA0NT EM18 20`
- Provider drive request: 2 mA
- SDR: SDRplay RSP1B serial `2404058C60`
- Capture: CF32, 250 ksample/s, center 14,122,100 Hz, fixed gain 25

The installed WsprryPi daemon was stopped temporarily because it owned the
singleton port. The live-run cleanup trap restored it and the SoapyRemote
service. A first capture-only attempt is retained on `wspr5` but emitted no RF;
the scheduler rejected that attempt before submission because of the singleton
owner.

## Live cadence and relative SDR findings

The scheduler reported normal completion in 110.718523 seconds. Independent
clock-disabled provider timing from Phase 6J measured the DMA frame itself at
110.699024 seconds. The GPIO monitor observed GPCLK0 with clock
prepare/enable/protect counts `1/1/1` during output and the safe input state
with `0/0/0` afterward.

Offline analysis of the 30,000,000-sample capture found:

- all 161 symbol boundaries retained continuous amplitude;
- worst 20 ms boundary minimum: -0.285 dB relative to adjacent symbols;
- fifth-percentile boundary minimum: -0.197 dB;
- symbol amplitude spread: 0.138 dB;
- relative tone spacing estimates: approximately 1.519, 1.431, and 1.488 Hz;
- equal-window carrier/baseline contrast: 65.73 dB; and
- largest other in-band feature outside 20 Hz: -44.86 dBc relative.

The SDR cold start exhibited approximately -2.44 Hz of common-mode drift over
the frame. After separating that drift, the reference-sequence fit estimated
1.409 Hz tone spacing with 0.356 Hz RMS residual. These are relative SDR
observations, not calibrated frequency or power measurements.

## Timing finding and next gate

Nominal WSPR symbol duration is 8192/12000 seconds, producing a 110.592-second
162-symbol frame. The measured provider cadence is approximately 0.683327
seconds per symbol and 110.699024 seconds per frame. The continuous DMA chain
therefore avoids scheduler-created gaps but is paced approximately 968 ppm too
slowly.

Before attempting three independent WSPR decodes, correct or parameterize the
RP1 DMA tick cadence so the full frame meets nominal WSPR timing. Repeat the
clock-disabled exact-frame timing tests, then repeat one minimum-drive live
capture and all-boundary analysis. Decode qualification remains a later,
separately authorized gate.

## Cleanup and qualification boundary

Final verified state:

- `live_output=N`;
- GPIO4 input in the safe 2 mA pinctrl state;
- GPCLK0 prepare and enable counts zero;
- WsprryPi service active;
- SoapyRemote service active; and
- parent and transmitter source tips unchanged.

No calibrated power claim, WSPR decode claim, multi-frame reliability claim,
or product RF qualification is made.
