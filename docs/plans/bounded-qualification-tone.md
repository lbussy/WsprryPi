# Bounded Qualification Tone Control

Status: implemented product-side primitive; hardware qualification pending

## Purpose

The `bounded_tone` WebSocket command provides a product-owned, self-terminating
Test Tone transaction for an authenticated qualification helper running on the
same host. It prevents loss or delay of the helper connection from extending RF
past the requested interval.

This software contract does not qualify GPIO timing, RF output, frequency,
power, spectral purity, cabling, attenuation, or receiver behavior.

## Containment

Start WsprryPi with `--no-http --socket-loopback-only` for a qualification
session. The HTTP listener stays disabled while the WebSocket listener binds to
IPv6 loopback rather than the wildcard address. Because this combination does
not expose a network listener, it does not require privileged external-network
policy reconciliation.
The `bounded_tone` command fails closed unless that mode was selected. Existing
`tone_start`, `tone_end`, and default browser-facing binding behavior are
unchanged.

The flag is command-line-only. It is not persisted by configuration reads or
writes and intentionally makes the WebSocket unavailable to remote browser
clients for that process lifetime.

## Request and lifecycle

Example request:

```json
{
  "command": "bounded_tone",
  "request_id": "phase7-001",
  "duration_ms": 2000,
  "frequency_source": "custom_rf",
  "frequency_hz": 14097100
}
```

`request_id` must contain 1 through 128 ASCII letters, digits, dots, hyphens,
or underscores. `duration_ms` must be an integer from 1 through 60000. Frequency
selection uses the existing Test Tone validation contract.

WsprryPi rejects overlapping bounded transactions. After the normal Test Tone
start succeeds, it arms a monotonic process-owned watchdog before acknowledging
success. At the deadline it runs the normal Test Tone stop and scheduler restore
path, then broadcasts a terminal `bounded_tone` event carrying the request ID,
stop result, scheduler-restore result, and status.

An explicit `tone_end` cancels the bounded transaction and uses the same cleanup
path. WebSocket-server shutdown requests watchdog stop, joins it, and quiesces
an active bounded tone before completing shutdown. Failure to arm the watchdog
immediately invokes Test Tone cleanup and reports an error.

## Remaining integration boundary

The Qualification Harness does not consume this command yet. A later reviewed
slice must add the SSH-authenticated helper, launch or select an explicitly
loopback-contained WsprryPi process, correlate both response events, enforce its
own outer deadline, and preserve cleanup evidence. That helper remains defense
in depth; the WsprryPi watchdog is the RF-duration authority.
