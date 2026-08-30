# Issue 429 closeout: comparative clock stability

Operator requirement, 2026-08-30: after the intrinsic-frequency measurements,
document how much more stable RP1 is compared with the previously tested Pi4
and PiZero2W. Preserve this as an explicit closeout requirement, not a claim
that the current sweep alone has established a stability improvement.

## Separate the measurements

- **Accuracy:** GPSDO-referenced mean carrier offset, in Hz and ppm.
- **Across-band consistency:** spread of band-by-band ppm offsets, accounting
  for divider planning and elapsed-time differences. This can be useful for
  comparing how well a single clock correction describes each board, but
  must be labeled separately from temporal stability.
- **Short-term stability:** variation of frequency estimates over equal-length
  windows within a fixed-frequency observation, with the window length stated.
- **Longer-term stability:** drift and repeatability at the same frequency over
  matched elapsed times and thermal/workload conditions.

The restarted RP1 sweep uses two-second tones at different frequencies. Its
across-band ppm spread is not a substitute for a same-frequency time-series
stability measurement. Longer GPSDO reference intervals do not lengthen the
RP1 observation. Do not promote a stability ratio, Allan deviation, or thermal
drift claim without enough appropriate data.

## Evidence needed for a numerical comparison

1. Locate the retained Pi4 and PiZero2W captures, clock/source identities,
   correction settings, receiver settings, reference observations, and timing.
2. Compare matching frequency, usable observation duration, estimator window,
   reference correction, and sample-rate treatment. Keep GPIO route and module
   build provenance explicit. Account for divider quantization/dithering and
   receiver/reference drift rather than attributing all variation to a board.
3. Report per-board mean offset separately from frequency variation: identical
   window-length standard deviation and a robust or peak-to-peak range; report
   drift rate only when the time span and data support it. State sample counts,
   elapsed duration, warm-up/load conditions, and measurement limitations.
4. Quantify any supported RP1 improvement using the same metric and conditions,
   with absolute values alongside ratios. If older observations are not
   comparable, identify the gap and plan a separately authorized matched test.
5. Include reproducible calculations and source links in the Issue 429 closeout.
   Preserve the distinction between measured clock stability and spectral or
   modulation-mode qualification.

Status: required follow-up. No extra RF test, new calibration constant, or
numerical stability-improvement claim is authorized or established by this note.
