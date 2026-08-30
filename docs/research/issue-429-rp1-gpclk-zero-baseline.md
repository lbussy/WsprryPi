# Issue 429: RP1 GPCLK intrinsic zero baseline

> Historical proposal, superseded by the conducted GPIO20 sweep and the shared
> `src/Chipset-Offsets` selector. The current RP1 intrinsic default is -46.245 ppm.
> Scheduler provenance reports the composed total, but requests carry only the
> additional correction: each physical backend applies the intrinsic once.
> The text below records the earlier zero-baseline design, not current behavior.

The Raspberry Pi 5 RP1 GPCLK profile uses an explicit intrinsic source-rate
baseline of `0 PPM`. This is a conservative product baseline, not a conducted
frequency-accuracy qualification or a claim that every RP1 clock has zero
physical error.

The scheduler manages RP1 with the same GPIO correction contract as the legacy
GPIO backends. It composes this intrinsic baseline exactly once with either the
selected qualified system-clock estimate plus configured conducted residual,
or the selected manual PPM value. The resulting final correction is frozen in
the execution request before the WSPR, TONE, or finite-event RP1 planner applies
it to the explicit 200 MHz compatibility parent.

A future nonzero intrinsic value requires separately authorized conducted
discovery and closure, representative-hardware evidence, and an explicit
promotion decision. It belongs in the RP1 intrinsic baseline; the existing
GPIO Frequency Residual PPM setting remains the conducted residual used only
with a qualified system-clock estimate.

This value applies only to the Raspberry Pi 5 RP1 GPCLK backend. It does not
apply to legacy BCM GPIO clocks, Si5351, another transmitter backend, receiver
calibration, or a particular GPIO route.
