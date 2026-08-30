# Issue 429: GPIO20 clock-accuracy brackets, 2026-08-30

The restarted sweep completed **all fourteen planned direct-output points**
from 2200 m through 4 m, including 12 m and 6 m. It ran from 21:36:57 UTC
through the final bracket started at 22:08:48 UTC, after the USB-cable reset
and addition of conservative GPSDO control pacing. No USB warnings occurred.
Observed output-frequency error ranges from -46.2674 to -46.1747 ppm.
This is diagnostic evidence, not a calibration constant, uncertainty budget,
mode qualification, or spectral qualification.

Subsequent operator decision: the rounded fourteen-band mean, **-46.245 ppm**,
is now adopted as the universal RP1 intrinsic default in the shared
`src/Chipset-Offsets` selector. This policy decision does not establish
board-to-board equivalence or corrected live-RF closure. The retained results
below are the original zero-transmitter-correction measurements.

| Band | Requested Hz | GPSDO-bracketed error, ppm |
| --- | ---: | ---: |
| 2200 m | 137500 | -46.2674 |
| 630 m | 475700 | -46.2578 |
| 160 m | 1838100 | -46.2591 |
| 80 m | 3570100 | -46.2637 |
| 60 m | 5288700 | -46.2599 |
| 40 m | 7040100 | -46.2604 |
| 30 m | 10140200 | -46.2535 |
| 20 m | 14097100 | -46.2502 |
| 17 m | 18106100 | -46.2515 |
| 15 m | 21096100 | -46.2465 |
| 12 m | 24926100 | -46.2511 |
| 10 m | 28126100 | -46.2462 |
| 6 m | 50294500 | -46.1905 |
| 4 m | 70092500 | -46.1747 |

2 m remains untested without a harmonic implementation; no band is disqualified
by this sweep. These TONE observations do not change mode-qualification labels.
The earlier six accepted brackets remain in the JSON, identified separately
by campaign and module hash; they are not pooled with the restarted sweep.

The across-band span is 0.0927 ppm, or 0.0212 ppm from 2200 m through 10 m.
The slightly different VHF observations remain visible rather than being
forced into one correction constant. This is not a temporal-stability metric.
The [comparative stability closeout plan](stability-comparison-plan.md) records
the operator's requirement to quantify RP1's advantage using comparable Pi4
and PiZero2W evidence, with accuracy, consistency, and stability distinguished.

## Identity and method

- Host and route: wspr5, Pi 5, GPIO20, 2 mA, attenuated conducted combiner path.
- SDR: same-host SDRplay serial `2404058C60`, 250000 complex samples/second,
  center 25000 Hz below the requested carrier, manual receiver correction zero.
- Reference: local Mac Leo Bodnar `0673ED0FA107`, locked PLL/SAT, output 1 LOW;
  output 2 disabled. Control uses `/Users/lbussy/GitHub/lbgpsdo/lbe142x.py`.
- Sequence: GPSDO 8-second hold plus measured command latency; both sources off;
  RP1 finite 2-second TONE; both off; GPSDO 8-second hold plus measured latency.
  Never intentionally enable the two sources together.
- Application: isolated binary SHA-256
  `8c0e73b9429905b3424a3242c92adc0a720c155f36b03d3fb727c1ffebd19336`;
  source-rate estimate disabled, manual transmitter PPM zero, scheduling disabled.
  The installed application was not replaced.
- Current DKMS source: `c6d4da8fca36484df4f87079d35f52cc8e3fcdb5`, kernel
  `6.18.34+rpt-rpi-2712`, module `1.1.2`, GPIO20 development candidate r4.
- Current compressed module SHA-256:
  `895f2d74a2143541775451b1fdbc0c7ec61eacd311e5b838d8273654430d0c0f`.
  Decompressed ELF:
  `4b63870abcf50d679a52beab30d441854effe333599d8a2053c89beced342584`.

The estimator identifies carrier intervals with a 100 ms Hann-window peak
within +/-5 kHz, checks them against the measured GPSDO command-duration bounds,
and trims transitions. It estimates each carrier using FFT interpolation and
weighted phase slope. Reference drift is interpolated at the transmitter
interval midpoint and subtracted once. A linear reference-drift assumption
and one bracket per frequency limit the interpretation.

Each restarted capture contains 30,000,000 complex samples (120 seconds);
the earlier accepted captures contain 11,000,000 (44 seconds). All have no reported
overflow, timeout, or clipping and verified SDR cleanup. Before/after Chrony
checks have Normal leap status, the same selected source `69.197.177.234`, and
absolute residual/skew at most 0.5 ppm. These gates do not turn the separate RP1
oscillator into the host's system clock; host frequency is recorded, not
subtracted from the RF result.

Snapshots confirm PLL_SYS during each accepted RP1 tone and restoration to the
previous disabled XOSC/50 MHz state afterward. Final gate/refcount readbacks
are `N`/`0`; GPSDO output readbacks are both disabled.
The audit explicitly checks the active parent rate is 200000000 Hz. All fourteen
restarted raw captures have independently verified SHA-256 hashes and byte counts.

The LBE-1421 control path now waits two seconds after opening and at least one
second after every HID transaction; the controller also leaves two seconds
between CLI commands. Actual reference-carrier intervals are approximately
30 seconds despite the eight-second explicit hold, because command latency
is included. Each run retains its acquisition plan and measured reference bounds.
All but the first restarted bracket additionally pin the GPSDO CLI source hash
`0195e7576ac9908e2d5d7d18a4c2a876d6692af227f642a1424c79408994ed16`;
the first retains the CLI path and device/status evidence, but no source digest.

## Findings and disposition

1. **80 m parent-selection cycle: repaired in DKMS.** The preceding `d9acd18`
   module alternated 50/200 MHz choices and exhausted its bounded setup loop
   with `-ERANGE`. The corrected loop retains its small seed bias across
   parent changes. A nearest-rate/double-rounding regression fails against
   the preceding source and passes against `c6d4da8`. The full module offline
   suite, target kernel build, module-identity check, and fresh 80 m bracket
   passed. No private provider API or direct clock-register write was added.
2. **Application failure containment: unresolved.** The rejected 80 m provider
   frame caused an uncaught `std::runtime_error` and disconnected the bounded
   control client. The module restored safe state. Application exception
   containment needs a separately reviewed repair; no application runtime code
   was changed during this testing turn.
3. **Interval detector: corrected offline.** Broadband energy after the first
   630 m reference extended a whole-band RMS interval. Carrier-specific
   detection places the reference end at 11.6 seconds, within its measured
   control bounds. The raw capture was retained and reanalyzed, not replaced.
4. **GPSDO USB control: operational retry passed.** `IOHIDDeviceSetReport` failed with
   `0xE00002ED` (device not responding) on an enable command, then recurred on
   a disable command despite increasing inter-command spacing from 0.2 to
   0.5 seconds. The controller did not proceed to RP1 after either failed
   reference operation. Subsequent readback confirmed both GPSDO outputs off.
   Aborted captures are excluded. The CLI frequency frame length and unnecessary
   enable writes were repaired, with readback verification and bounded safe
   retries. After the operator reset the USB cable, both outputs were found
   enabled at 10 MHz and were explicitly disabled while RP1 remained inhibited.
   Twenty-four OFF-state cycles and the fourteen-point paced sweep then passed
   without USB warnings. Reset and pacing changed together; the underlying USB
   fault cause is not isolated and future reliability is not guaranteed.
   The isolated application is stopped and GPIO20 is input/low. Both GPSDO
   outputs/PPS are off, with the starting 10 MHz temporary frequency restored.
5. The earlier 8-second finite-TONE failure remains unresolved. These successful
   2-second brackets do not close it.
6. **VHF analysis-window sensitivity: checked offline.** Using the reference
   portions nearest the RP1 tone and then a shorter central tone interval changes
   the 6 m and 4 m estimates by less than 0.004 ppm. This does not explain away
   their shift from the lower-band cluster and is not a formal uncertainty budget.

## Reproduce the offline checks

- [results.json](results.json) retains per-bracket estimates, source paths,
  capture hashes, clock snapshots summarized as fields, and Chrony values.
  Earlier/current module rows are explicitly separated by module hash, and
  pre/post-USB-reset observations by campaign. The notebook's main results
  select only the restarted paced sweep.
- [review.ipynb](review.ipynb) independently recomputes interpolation and ppm.
  Its code cells were executed sequentially with Python and outputs retained;
  Jupyter/nbformat were unavailable, so Jupyter-kernel execution was not run.
- [estimate.py](estimate.py) reads one retained raw capture and metadata:
  `python3 estimate.py /path/to/capture-directory FREQUENCY_HZ` (NumPy required).
- [estimator_selftest.py](estimator_selftest.py) checks a known offset, extra
  burst isolation, extended reference-command latency, broadband tails, and
  the longer paced acquisition plan:
  `python3 estimator_selftest.py`. All passed on wspr5. The fixtures deliberately
  create temporary raw IQ files; they do not access hardware.
- [audit.py](audit.py) audits complete local control records:
  `python3 audit.py /path/to/parent-of-run-directories` (standard library only).
  It rechecks metadata, identities, numeric gates, cleanup, clock restoration,
  interpolation, and agreement with the FFT cross-check.
- [window-sensitivity.json](window-sensitivity.json) retains the VHF alternative
  window results. Its read-only source is [window_sensitivity.py](window_sensitivity.py).

Final adversarial review found no additional blocker to reporting these bounded
frequency observations. The older application-containment and long-tone issues,
2 m implementation gap, and comparative temporal-stability work remain open.

Raw captures and complete control records are retained on wspr5 under
`/home/pi/wsprrypi-qualification-runs/issue429-rp1-accuracy-<UTC>-<Hz>`.
Portable scripts are also retained in `issue429-accuracy-audit-20260830` under
that parent. The aborted attempts are `20260830T204341Z-3570100`,
`20260830T205154Z-137500`, and `20260830T205528Z-1838100`.
Installation evidence is in `issue429-parent-c6d4da8-install`; predecessor
module/source/route-manager backups remain in `/tmp/issue429-parent-build.vGP7Zw`.
