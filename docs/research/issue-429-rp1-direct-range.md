# Issue 429: RP1 direct-frequency range

RP1 TONE, WSPR, QRSS, FSKCW, and DFCW now use the numerical planner's shared
100 MHz direct-output ceiling instead of a separate 40 MHz application cap.
The nominal parent remains 200 MHz, source-rate PPM is applied once, and the
0.01 Hz numerical average-error tolerance is unchanged. All generated tones
must satisfy the ceiling and share one integer divider; integer-divider
boundary plans still fail closed.

This permits direct planning of 6 m as well as 12 m and the other lower bands.
It does not authorize RF output or qualify any band. RP1 6 m and 2 m remain
`untested` for every mode and require the existing experimental frequency
opt-in. Route-specific development authorization and provider lifecycle gates
are unchanged for both GPIO4 and GPIO20.

2 m remains numerically rejected by this direct-only backend. A third-harmonic
implementation would need explicit RF-to-clock frequency and modulation-spacing
conversion, correction ownership, filtering considerations, and separate
validation. No such conversion or automatic fallback is introduced here. The
absence of that implementation is not a measured failure of RP1 hardware.

The current Issue 429 conducted campaign measures intrinsic clock-frequency
accuracy against the GPSDO, with alternating reference and RP1 measurements.
It must explicitly include 12 m and retain band/route-specific results.
Spectral quality and mode qualification are outside this accuracy campaign;
they are not acceptance gates for its clock-frequency measurements. Software
tests with a fake provider do not establish measured frequency accuracy.

Operator documentation follow-up (separate repository, not modified here):
update `Wsprry_Pi_Docs/docs/Experimental/rp1_gpio.md` with the direct-range and
untested-band distinction once this implementation is integrated. Do not
replace qualification records with numerical planning results.
