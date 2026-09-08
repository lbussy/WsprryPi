# Measurement tables

All offsets use the simultaneously captured GPSDO. PPM remains +3.470680.

| Run | Carrier offset (Hz) | Burst mean span (Hz) | Transition gap metric (ms) | Phase step metric (rad) | Transition concentration (%) |
|---|---:|---:|---:|---:|---:|
| before-r2-40m | +0.3217 | 0.0556 | 0 | 0.0136 | 95.5086 |
| before-r2-2m | +6.4754 | 0.8165 | 3 | 1.0968 | 98.8919 |
| step1-40m | +0.4485 | 0.0641 | 0 | 0.0168 | 95.2889 |
| step1-2m | +8.1960 | 1.4871 | 0 | 0.0323 | 99.9068 |
| step2-40m | +0.3990 | 0.0452 | 0 | 0.0765 | 95.8579 |
| step2-2m | +9.7046 | 1.2513 | 0 | 0.0437 | 99.9155 |
| step3-r2-40m | +0.3925 | 0.0468 | 0 | 0.0311 | 96.1203 |
| step3-r2-2m | +6.0100 | 0.6146 | 0 | 0.0219 | 99.8965 |
| step4-40m | +0.3340 | 0.0535 | 0 | 0.0506 | 96.1822 |
| step4-2m | +6.2539 | 0.5978 | 0 | 0.0272 | 99.8758 |
| step5-40m | +0.2661 | 0.0547 | 0 | 0.0195 | 97.6432 |
| step5-2m | +5.0576 | 1.3633 | 0 | 0.0320 | 99.4457 |
| step5-r2-40m | +0.1822 | 0.0702 | 0 | 0.0185 | 97.6300 |

Gap metric counts 1 ms samples below -10 dB in a transition window after 5 ms smoothing. Zero means no resolved gap by this method. Phase steps are extrapolated estimates. Concentration is power within +/-20 Hz of the indicated carrier divided by power within +/-5 kHz, including noise; it is not occupied bandwidth.

| Run | Hard-key concentration (%) | Raised-cosine duty-fade concentration (%) |
|---|---:|---:|
| before-r2-40m | 91.0678 | 89.3028 |
| before-r2-2m | 98.8633 | 96.8681 |
| step1-40m | 90.9544 | 89.1542 |
| step1-2m | 98.8756 | 96.8672 |
| step2-40m | 91.0699 | 89.2266 |
| step2-2m | 98.8846 | 96.8339 |
| step3-r2-40m | 91.7989 | 90.0228 |
| step3-r2-2m | 98.8856 | 96.8644 |
| step4-40m | 92.2770 | 90.8708 |
| step4-2m | 98.4938 | 97.4980 |
| step5-40m | 94.8792 | 93.8112 |
| step5-2m | 98.5133 | 97.5117 |
| step5-r2-40m | 94.6279 | 93.6127 |

## Programming duration

These are complete applyTone durations, including status reads and any readiness waits, not isolated I2C wire time. Instrumentation began in step 3; the initial baseline has no directly comparable duration log. Active writes remain individual. No before/after bus-speed percentage is claimed.

| Run | First programming (us) | Retune median (us) | Retune range (us) |
|---|---:|---:|---:|
| step3-r2-40m | 3758 | 3096 | 2940–3153 |
| step3-r2-2m | 3755 | 3106 | 2941–3133 |
| step4-40m | 3767 | 3130 | 2957–3152 |
| step4-2m | 3769 | 2973 | 2952–3199 |
| step5-40m | 1784 | 3097 | 2933–3123 |
| step5-2m | 3764 | 2979 | 2935–3205 |
| step5-r2-40m | 1860 | 2970 | 2938–3149 |

## Rendered comparisons

| Stage | 40 m | 2 m |
|---|---|---|
| step1 | [PDF](step1-40m/versus-before.pdf) | [PDF](step1-2m/versus-before.pdf) |
| step2 | [PDF](step2-40m/versus-before.pdf) | [PDF](step2-2m/versus-before.pdf) |
| step3-r2 | [PDF](step3-r2-40m/versus-before.pdf) | [PDF](step3-r2-2m/versus-before.pdf) |
| step4 | [PDF](step4-40m/versus-before.pdf) | [PDF](step4-2m/versus-before.pdf) |
| step5 | [PDF](step5-40m/versus-before.pdf) | [PDF](step5-2m/versus-before.pdf) |

The final-source 40 m repetition is [retained separately](step5-r2-40m/versus-before.pdf). Final-source 2 m and guarded-default 40 m failed during transitions; guarded-default 2 m was not run. These failures are recorded in [rejected attempts](rejected-attempts.json), not included in the successful comparison table.
