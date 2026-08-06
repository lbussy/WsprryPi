# WSPR Community Frequency Calibration Gist Research Snapshot

Status: research evidence, not an implementation contract

Captured: 2026-08-06

Upstream author/maintainer attribution: LB7UG (`riyas-org` on GitHub)

Related Wsprry Pi plan: [Community frequency validation](../plans/wspr-community-frequency-validation.md)

## Purpose

This document preserves enough provenance and independently written behavioral
detail to reconstruct the Wsprry Pi research if the upstream gist changes or
disappears. It does not contain or relicense the upstream Python source.

## Upstream identity

- Gist: [WSPR Frequency Calibration Using Community Data](https://gist.github.com/riyas-org/64c18993b8a80f9a42450c9caf787d23)
- Gist ID: `64c18993b8a80f9a42450c9caf787d23`
- File: `wspr_frequency_cal.py`
- Title in the file: `WSPR TRANSCEIVER NETWORK CALIBRATION ENGINE (Stability-Weighted) - v2.3`
- GitHub owner: `riyas-org`
- Attribution in the file: “Compiled, refined, and maintained by LB7UG.”
- Acknowledgments in the file: Stephen (VK3SPM), Ryan (W7RLF), and Hans Summers
  (QRP Labs).
- Created: `2026-07-15T10:41:52Z`
- Last modified at capture: `2026-07-16T07:23:59Z`
- Latest gist revision at capture:
  `49e4e65a0c960ec13573537cc15a9b000216d321`
- Raw file URL returned by the GitHub Gist API at capture:
  `https://gist.githubusercontent.com/riyas-org/64c18993b8a80f9a42450c9caf787d23/raw/ba450f9c6f75a80c792e1c53ceb9d17519f57eb0/wspr_frequency_cal.py`
- GitHub API size: `20,991` bytes
- Exact content returned in the GitHub API JSON string: 436 newline characters
  and 20,991 bytes
- SHA-256 of that exact decoded content, without adding a retrieval newline:
  `31b38dace6620a6ff8fc286976c66a133b432adc23cf652f6409b40636b3f47f`

The revision history reported at capture, newest first, was:

| Committed UTC | Gist revision |
|---|---|
| 2026-07-16 07:23:58 | `49e4e65a0c960ec13573537cc15a9b000216d321` |
| 2026-07-16 06:58:39 | `b30b4f7c70d36ee04f53b873b5da44c235810350` |
| 2026-07-16 06:25:51 | `59ba4e6d754977d7dfb622cda9bdd091d6a25d98` |
| 2026-07-16 03:29:26 | `ad5a5238fde85686790ca309246023b248dfcccd` |
| 2026-07-15 11:01:39 | `61493add83300b3939d7df229995660521886c4c` |
| 2026-07-15 10:42:46 | `8880ef4cc3298b88e8dbadee31b008b70a4b4049` |
| 2026-07-15 10:41:52 | `e0df74f69aecaa20a986ef167c18a62fdf9e80f1` |

## License and preservation boundary

No license, SPDX identifier, copyright grant, or reuse permission was present
in the target gist or its Python file on 2026-08-06.

A GitHub API search of every non-fork/source repository then owned by
`riyas-org` returned `NOASSERTION` for `licenseInfo` on all 18 repositories:
`rp2040-sound-card`, `sdr`, `atu-100`, `tiny5351`, `simpleblinky`, `TFT_R61503`,
`ir_translator`, `ledclock`, `swranalyser`, `RVK_attiny_sensor`, `dgf`,
`si570test`, `Arduino_codehopping`, `ili9341`, `nrf24scannerPi`, `max7219`,
`ntpclock`, and `HC7Segment`.

An unrelated older gist displayed a Beerware notice inside a different source
file. A license attached to another work is not authority to copy this work.
Consequently:

- the Python source is not vendored, quoted at length, or represented as Wsprry
  Pi code;
- ownership and attribution remain with LB7UG/`riyas-org` and the people
  acknowledged upstream;
- this record captures facts, interfaces, equations, observations, and an
  independently written design analysis;
- permission from the rights holder, or a license added specifically to the
  target work, is required before distributing the original or a derivative
  translation of its protected expression.

If a future license is found, record the exact source, revision, scope, and
date before changing this boundary. Do not infer a license from public access,
GitHub hosting, or licensing used by another repository or gist.

## Dependencies and external service

The script uses Python standard-library HTTP and time facilities plus NumPy and
pandas. Matplotlib is an optional, locally imported dependency for plots.

It sends HTTP GET requests to `https://db1.wspr.live/` with ClickHouse SQL in a
`query` parameter and requests `FORMAT JSONEachRow`. It identifies itself with
the user agent `WSPR-Network-Calibration-Engine/2.1` and uses a 30-second HTTP
timeout. The target service and its usage terms are external dependencies and
must be revalidated before implementation.

The script validates callsigns with a deliberately loose uppercase
alphanumeric/slash expression, validates a fixed band catalog, minimally
escapes interpolated SQL strings, and accepts lookbacks expressed as an integer
followed by `d`, `h`, or `m`. Invalid lookbacks fall back to one day.

## Band catalog observed at capture

The gist uses intentionally broad receive windows and a nominal center:

| Band | Minimum Hz | Maximum Hz | Nominal center Hz |
|---|---:|---:|---:|
| 160m | 1,836,500 | 1,836,700 | 1,836,600 |
| 80m | 3,568,500 | 3,568,700 | 3,568,600 |
| 40m | 7,038,500 | 7,041,700 | 7,038,600 |
| 30m | 10,138,600 | 10,138,800 | 10,138,700 |
| 20m | 14,097,000 | 14,097,200 | 14,097,100 |
| 17m | 18,106,000 | 18,106,200 | 18,106,100 |
| 15m | 21,096,000 | 21,096,200 | 21,096,100 |
| 12m | 24,926,000 | 24,926,200 | 24,926,100 |
| 10m | 28,126,000 | 28,126,200 | 28,126,100 |
| 6m | 50,293,000 | 50,294,500 | 50,293,100 |

The unusually wide 40m and 6m maximums are part of the captured behavior, not
recommendations for Wsprry Pi. A future implementation must use Wsprry Pi's
authoritative band/frequency semantics and independently validate service-side
coverage.

## Captured algorithm, expressed independently

The public entry point defaults to callsign `LB7UG`, band `20m`, lookback
`10d`, minimum receiver sample count 2, MAD outlier rejection enabled with a
factor of 6, and plotting disabled.

### First retrieval: target transmitter

The script queries `wspr.rx` for the requested transmitter, time range, and
broad band window. It obtains:

- observation time;
- the containing two-minute slot;
- receiver callsign;
- reported frequency.

It then forms the unique set of slots and receivers that heard the target.

### Second retrieval: comparison traffic

Slots are divided into batches of 30. For every batch, the script fetches all
other transmitters heard by the target's receivers during those slots and in
the same broad band window. It waits 0.25 seconds between requests. Failed
batches are reported but do not discard successful batches.

### Receiver-bias estimate

For every `(slot, comparison transmitter)` group, at least three receiver
reports are required. The group median is treated by the script as that
transmitter's reference frequency. For an individual report:

```text
receiver slot error = reported frequency - group median frequency
```

The median receiver error over all qualifying comparison transmitters in the
same `(slot, receiver)` is then joined to target-transmitter spots. A target
spot is corrected as:

```text
corrected target frequency = target reported frequency - receiver slot error
```

“Reference frequency” and “corrected target frequency” are more defensible
terms than the script's “true” frequency: the computation is tied to receiver
consensus and is not traceable absolute metrology.

### Receiver weighting and outliers

For each receiver, the script calculates the count, mean, and sample standard
deviation of its slot-error observations. Missing standard deviation is
replaced by `5 + 10 / sample_count` Hz. Exactly zero is replaced with 0.1 Hz.

Sample confidence is capped at one:

```text
sample confidence = min(sample count / configured minimum, 1)
trust weight = sample confidence / receiver error variance
normalized weight = trust weight / median trust weight
```

When at least eight corrected spots exist and outlier rejection is enabled,
the script rejects corrected values farther than six median absolute
deviations from their median (or the configured factor replacing six).

### Headline calculations

The script computes a weighted mean corrected frequency and subtracts the
fixed band center to obtain the displayed transmitter offset. It also reports:

- raw spot comparisons and effective sample size;
- audited receiver count and a table of the lowest-variance receivers;
- weighted variance and standard deviation;
- standard error estimated as weighted standard deviation divided by the
  square root of Kish effective sample size;
- unweighted standard deviation;
- weighted mean absolute deviation;
- per-day weighted means and offsets when multiple dates occur.

The script additionally derives values resembling square-root-of-pi and pi
from the ratio of weighted standard deviation to weighted mean absolute
deviation. It calls this a Gaussian-noise sanity check rather than metrology,
then assigns qualitative stability labels at weighted standard deviations of
1 Hz and 3 Hz. Neither the pi derivation nor those thresholds are accepted as
requirements for Wsprry Pi.

### Optional plots

When enabled, the script draws:

1. corrected spot frequency versus time, with weighted mean and nominal center;
2. a corrected-frequency histogram with a Gaussian curve;
3. a daily weighted-frequency trend when more than one day is represented.

### Examples observed at capture

The file ends with these three intended use cases:

- five-day historical deep dive with plots;
- four-hour warm-up stress test;
- 45-minute recent-window validation.

They all invoke the same flexible function. They are presentation presets, not
separate algorithms or limits.

## Research conclusions preserved from the 2026-08-06 review

1. The receiver-consensus correction is useful but not an absolute frequency
   standard. Results must be labeled estimates and include data-quality
   evidence.
2. Randomized Wsprry Pi offsets make comparison against a fixed band center
   invalid. Every received spot must instead be correlated with the exact
   intended RF center recorded for that transmission.
3. Repeating observations from one receiver are correlated. Spot-level Kish
   effective sample size alone can overstate independent evidence. Aggregate
   within receiver or estimate uncertainty by receiver clusters.
4. Inverse-variance weights need regularization and a per-receiver influence
   cap so an implausibly small variance cannot dominate the estimate.
5. Hertz and ppm should both be reported. Band-independent qualitative labels
   should not be adopted without validation.
6. The empirical-pi calculation is not relevant to the product feature.
7. Remote querying and analysis must be asynchronous, cancellable, bounded,
   cached, and isolated from scheduling and RF execution.
8. Analysis should initially be read-only. Any future calibration-setting
   proposal needs explicit preview, backend-specific sign semantics,
   confirmation, post-change validation, and rollback.

## Revalidation checklist

Before relying on this record for implementation:

1. Check the gist URL and compare its latest revision and content checksum with
   this snapshot.
2. Recheck the target file for a license or explicit reuse permission.
3. Recheck the author's repositories only as supporting evidence; do not treat
   an unrelated repository license as applying to the gist.
4. Revalidate WSPR Live schema, acceptable use, availability, query limits,
   and frequency precision.
5. Reinspect Wsprry Pi's current scheduler, frequency semantics, backends,
   status surface, persistence facilities, and submodule boundaries.
6. Treat this document as historical research where current code or external
   service behavior differs.

## External references used in the review

- [WSPR Live](https://wspr.live/) — public ClickHouse endpoint, `wspr.rx`
  schema, raw-data disclaimer, and service-use conditions as displayed on
  2026-08-06.
- [WSPRdaemon field definitions](https://www.wsprdaemon.org/fields) — describes
  reported frequency as the receiver's dial frequency plus measured audio
  frequency and documents the precision of its separate spot store.
- [WSJT-X user guide](https://wsjt.sourceforge.io/wsjtx-main_en.html) — provides
  the contrast with calibration against carrier-based signals of reliably known
  frequency.
