# Community-Derived WSPR Frequency Validation Plan

Status: proposed; no implementation exists

Research baseline: 2026-08-06

Upstream research record:
[gist snapshot and licensing boundary](../research/wspr-community-frequency-calibration-gist-snapshot-2026-08-06.md)

## Objective

Allow a Wsprry Pi operator to evaluate transmission-frequency accuracy and
stability over an arbitrary historical window by correlating the exact
frequency Wsprry Pi intended to transmit with reports from community WSPR
receivers.

The feature should support workflows such as:

- a multi-day historical and drift analysis;
- a short warm-up study after boot or first transmission;
- a recent-window validation check;
- any operator-selected bounded time range.

Those workflows are convenient presets over one analysis engine. They are not
fixed product limits.

## Outcome language

The analysis is receiver-consensus evidence, not traceable RF metrology. The
product should use terms such as:

- `estimated frequency residual`;
- `community receiver consensus`;
- `confidence interval`;
- `receiver agreement`;
- `data quality`.

It must not describe the result as the transmitter's “true frequency,” claim
hardware qualification from network spots, or silently change calibration.

## Existing implementation seam

At the research baseline, scheduler orchestration already:

1. resolves the configured WSPR dial frequency plus WSPR audio offset into a
   base actual RF frequency;
2. optionally chooses a uniformly distributed random offset;
3. commits the resulting actual RF frequency to the transmission request;
4. retains the applied offset in that request;
5. exposes transient dial-frequency and offset information in runtime status.

This is the correct point from which to capture intended frequency. Runtime
status alone is not durable enough for historical analysis, and the random
sequence must never be reconstructed after the fact.

Current code must be reinspected before implementation. The baseline locations
were `src/scheduling.cpp`, `src/scheduling.hpp`, `src/web_socket.cpp`,
`src/frequency_semantics.hpp`, and `config/wsprrypi.ini`.

## Core invariant

For each completed or attempted WSPR transmission, Wsprry Pi must be able to
recover the exact execution intent used for that slot:

```text
intended center RF = resolved WSPR RF center + selected random offset
```

Each candidate network spot is evaluated against that transmission-specific
value:

```text
raw residual = reported frequency - intended center RF

corrected residual =
    reported frequency
    - estimated receiver bias
    - intended center RF
```

Comparing spots with a fixed band center is incorrect when random offsets are
enabled and is not the intended Wsprry Pi design.

## Proposed component boundaries

### Transmission journal

Create a parent-application component that durably records transmission intent
and outcome. It should not be placed inside an existing reusable component
under `src/`.

Minimum record fields:

- schema version and stable transmission ID;
- plan ID for related paired frames;
- scheduled slot, actual start, and actual finish in UTC;
- raw, normalized, and actually encoded callsign identity as applicable;
- encoded locator and reported power as applicable;
- band/frequency-entry identity;
- configured dial frequency;
- WSPR audio offset;
- selected random offset;
- intended WSPR center RF frequency;
- committed calibration PPM and its source;
- backend and output identity;
- single/paired plan type and frame number;
- software/configuration identity sufficient to identify change boundaries;
- outcome: planned, started, completed, interrupted, rejected, or failed;
- optional time-since-boot, Pi temperature, and available reference telemetry.

Required properties:

- append-safe and crash-tolerant;
- bounded retention with documented behavior;
- no dependency on remote service availability;
- negligible and bounded work in the scheduling/execution path;
- no sensitive values beyond those already required for public WSPR identity;
- a migration strategy for future journal schemas.

SQLite is the leading persistence candidate because the feature needs indexed
time-window lookup, matching state, retention, and resumable background work.
JSON Lines is a simpler alternative but would require more machinery for those
operations. This choice remains unresolved pending review of existing Wsprry Pi
persistence and deployment constraints.

### Spot source

Define a source-neutral interface that returns typed observations. The first
adapter may target WSPR Live, but analytics must not depend directly on its SQL,
HTTP, or JSON representation.

Responsibilities include:

- callsign, band, and time-window validation;
- service-specific query construction and escaping;
- HTTPS certificate validation;
- timeouts, cancellation, bounded retries, and exponential backoff;
- strict row, window, response-size, concurrency, and total-job limits;
- batching and cache reuse;
- partial-result and provenance reporting;
- configurable endpoint without an insecure fallback;
- compliance with the service's current acceptable-use terms.

The adapter must not run on a scheduler or transmitter-control thread.

### Spot correlator

Correlate using, in order:

1. transmitted callsign identity as encoded;
2. WSPR band;
3. two-minute UTC slot;
4. proximity to the journaled intended center RF frequency;
5. paired-frame identity and other available discriminators.

Every candidate receives an inspectable status:

- exact;
- probable;
- ambiguous;
- unmatched;
- excluded with a reason.

Do not include ambiguous observations in the headline result by default.
Expose counts and enough detail to diagnose the decision. Frequency tolerances
must account for database precision and plausible receiver/transmitter error,
but must not span the complete WSPR passband merely to force a match.

Duplicate-report behavior and identity changes from compound or extended WSPR
messages require explicit fixtures. A paired plan produces separate
transmission records joined by one plan ID.

### Receiver-bias estimator

Use comparison transmitters heard by the same receivers at the same time to
estimate receiver error. The captured gist's median-based calculation is a
candidate starting point, not a frozen requirement.

Initial requirements:

- require multiple independent receivers for a comparison-transmitter
  reference;
- use robust location and spread estimators;
- do not call a consensus median “true frequency”;
- regularize variance estimates from small samples;
- cap the influence of a single receiver;
- detect receivers whose bias or variance changes materially during the
  window;
- retain input counts and rejection reasons for audit.

Whether bias should be estimated per slot, over adjacent slots, or with a
time-varying model remains an empirical decision. Sparse recent windows may
need a carefully bounded nearby-time estimate and must disclose that fact.

### Frequency analyzer

Input should be plain typed C++ records so tests require neither RF hardware nor
network access.

Candidate outputs:

- robust estimated residual in hertz and ppm;
- uncertainty/confidence interval;
- raw spot count, matched transmission count, and receiver count;
- effective receiver count distinct from effective spot count;
- robust spread and receiver-to-receiver agreement;
- residual slope in Hz/hour and ppm/hour when supported;
- per-hour or per-day aggregates as appropriate to window length;
- warm-up relation to time-since-boot or first transmission when journal data
  supports it;
- configuration, backend, restart, and calibration change boundaries;
- unmatched, ambiguous, and rejected counts and reasons;
- a data-quality decision, including `insufficient evidence`.

Repeated observations from one receiver are correlated. Aggregate within a
receiver before combining receivers, use a receiver-clustered uncertainty
method, or bootstrap whole receivers. Do not present a spot-level Kish value as
the sole measure of independent evidence.

The captured gist's empirical-pi calculation and fixed 1 Hz/3 Hz qualitative
thresholds are explicit non-requirements.

### Validation job service

Own background-job lifecycle independently of RF execution:

- create a bounded analysis request;
- report queued, retrieving, correlating, analyzing, complete, cancelled, and
  failed states;
- provide granular progress and actionable errors;
- cancel without affecting transmission;
- persist or cache enough state to avoid unnecessary repeated public queries;
- clean partial temporary data according to a documented policy;
- expose source timestamp, analysis version, and completeness in results.

Network outage, malformed remote data, database overload, or analysis failure
must never inhibit, delay, enable, or otherwise alter transmission behavior.

## Operator workflow proposal

This section records workflow requirements only. Any future UI design or
implementation must follow the repository's required Impeccable workflow.

### Starting an analysis

Provide one Frequency Validation surface with:

- historical preset, initially five days;
- warm-up preset, initially four hours;
- recent validation preset, initially 45 minutes;
- custom bounded start/end or lookback;
- band and callsign derived from journal records when possible;
- an explanation that public community data will be queried;
- estimated query scope before starting;
- a start action that does not change configuration.

### Progress and failures

Keep feedback beside the initiating control. Show retrieval batches, partial
availability, cancellation, rate limiting, timeout, no matching transmissions,
no received spots, insufficient comparison traffic, and insufficient evidence
as distinct states.

### Results

Lead with:

- estimated residual in Hz and ppm;
- confidence interval or explicit absence of one;
- data-quality result;
- matched transmissions versus journaled transmissions;
- receiver and effective-receiver counts;
- observation window and source freshness;
- backend and calibration context;
- confirmation that randomized offsets were matched from journal records.

Candidate visualizations:

1. corrected residual versus time, with configuration/restart boundaries;
2. residual distribution;
3. receiver agreement and influence;
4. intended versus reported center frequency per transmission;
5. temperature/time-since-boot overlay where recorded;
6. daily/hourly aggregate trend appropriate to the selected window.

An evidence table or export should identify matches, corrections, exclusions,
and analysis-version provenance without exposing secrets.

## Calibration-setting boundary

The first implementation is analysis-only. It must not write configuration.

A later separately approved feature may propose a calibration adjustment only
after it can:

1. show current and proposed PPM;
2. explain the sign convention;
3. identify whether GPIO/NTP or Si5351 calibration is affected;
4. preview the resulting configuration change;
5. require explicit confirmation;
6. preserve the prior value;
7. run a new validation window;
8. offer rollback.

GPIO NTP-derived correction and Si5351 configured PPM are distinct mechanisms.
A single community estimate must not be applied indiscriminately to both.

## Delivery slices

Each slice requires separate approval and current-code reinspection.

### Slice 1: durable transmission observability

- finalize the journal schema and retention policy;
- capture exact committed random offset and intended WSPR center RF;
- record lifecycle outcomes without changing RF behavior;
- add deterministic persistence, migration, crash, and paired-frame tests.

Exit evidence: journal fixtures prove one authoritative record per attempted
frame and preserve random offsets across restart. This is not RF qualification.

### Slice 2: offline matching and analysis

- implement typed spot, match, bias, and result models;
- correlate against journal fixtures;
- implement robust statistics and insufficient-evidence rules;
- cover random offsets, duplicates, paired frames, interruptions, callsign
  variants, sparse receivers, outliers, and ambiguous candidates.

Exit evidence: deterministic fixtures reproduce independently calculated
expected results without network or hardware access.

### Slice 3: bounded live retrieval

- implement the source adapter, caching, limits, cancellation, provenance, and
  failure handling;
- validate against recorded service responses first;
- perform separately authorized, respectful live-service checks.

Exit evidence: remote failure cannot affect scheduling or execution, and query
limits are demonstrated.

### Slice 4: textual application report

- expose job control and structured/text results through parent-application
  interfaces;
- include evidence counts, uncertainty, source, and limitations;
- retain analysis-only behavior.

### Slice 5: user interface and charts

- use the mandatory Impeccable workflow;
- implement presets plus custom windows, local progress, errors, plots, and
  evidence inspection;
- render and inspect desktop and mobile behavior;
- update the separate operator-documentation repository only under explicit
  cross-repository authorization.

### Slice 6: optional calibration proposal

- separately specify backend-specific correction semantics;
- implement preview, confirmation, validation, and rollback only after
  qualification and explicit approval.

## Validation strategy

### Unit and property tests

- UTC slot boundaries, leap/day transitions, and clock skew;
- exact random-offset persistence and decimal-frequency behavior;
- frequency matching near tolerance boundaries;
- duplicate, ambiguous, and unmatched classification;
- paired-frame identity;
- robust statistics, weight caps, cluster uncertainty, and outliers;
- sparse and empty inputs;
- Hz/ppm conversions and sign convention;
- journal migration, interrupted writes, retention, and concurrent readers;
- cancellation, response-size limits, malformed data, and partial batches.

### Golden fixtures

Maintain small synthetic and sanitized service-response fixtures with explicit
provenance and expected intermediate tables. Independently calculate expected
receiver biases and residuals rather than treating output from the captured
Python gist as an oracle.

### Non-hardware integration

Prove that journal recording and background analysis do not alter request
planning, random-offset selection, transmission state, cleanup, or backend
routing. Inspect every test target before running it because generic test names
may invoke privileged transmitter paths.

### Qualification boundaries

Source tests, saved public spots, fake clocks, or dry-run execution do not
qualify RF frequency. Community reports provide operational evidence but do not
replace a calibrated counter, GPS-disciplined reference, conducted measurement,
or backend-specific hardware qualification.

## Privacy, security, and operations

- disclose callsign/band/time data sent to the external source;
- send no local configuration, logs, credentials, IP addresses, or host
  identity beyond what the request strictly requires;
- do not accept arbitrary SQL or an arbitrary URL from the browser client;
- keep endpoint changes administrator-controlled and validated;
- escape service queries defensively and constrain all enumerated values;
- bound local storage and provide intentional retention/export/delete behavior;
- include cached/live source status and query time in every result;
- do not imply availability or correctness guarantees from a volunteer service.

## Explicit non-goals

- vendoring or translating the unlicensed upstream Python source;
- embedding Python, pandas, NumPy, or Matplotlib in Wsprry Pi;
- reproducing random offsets after transmission instead of recording them;
- asserting absolute or traceable frequency truth from network consensus;
- automatic calibration changes in the initial feature;
- changing transmitter, scheduling, randomization, or backend behavior;
- RF, GPIO, service, installation, or hardware work under this plan alone;
- changes to the sibling `Wsprry_Pi_Docs` repository without separate approval.

## Unresolved decisions

1. Journal storage format, location, ownership, retention, and backup behavior.
2. Exact event at which planned, started, and completed records become durable.
3. Service query limits and caching policy acceptable to WSPR Live.
4. Frequency and time matching tolerances based on observed database precision.
5. Receiver-bias time model and minimum independent-receiver requirements.
6. Robust estimator, influence cap, and receiver-clustered confidence method.
7. Minimum evidence rules for each time-window scale.
8. Temperature and boot-identity telemetry availability and privacy.
9. API, WebSocket, export, and UI ownership boundaries.
10. Whether any future result may inform GPIO/NTP and Si5351 calibration, and
    the independently qualified sign/units behavior for each.

## Restart checklist

To resume after loss of the original chat or gist:

1. Read the linked dated research snapshot, including its license boundary.
2. Recheck the gist revision/checksum and license status if it still exists.
3. Recheck WSPR Live schema, terms, and limits.
4. Inspect current `devel`, the complete parent working tree, repository
   instructions, `docs/components/provenance.md`, all ten retained component
   paths, frequency semantics, the scheduler request boundary, runtime status,
   and tests. Review any component changes through path-scoped parent diffs.
5. Resolve the journal schema and persistence decision before implementation.
6. Obtain approval for exactly one delivery slice.
7. Keep implementation evidence, later plans, non-goals, and RF qualification
   explicitly separate.
