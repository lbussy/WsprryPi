<!-- SPDX-License-Identifier: MIT -->

# Clock-disabled route restoration on wspr5

On 2026-08-31, the authorized exact-target validation passed on wspr5,
Raspberry Pi 5, kernel `6.18.34+rpt-rpi-2712`, without reboot or transmission.
[Results](results.json) retain route, controller, application PID/readiness and
passive snapshot observations. This is bounded functional evidence, not RF,
waveform, reliability, release or general hardware qualification.

The installed application executable is from `fbe62e5`, SHA-256
`d62c26e0248fef31daf7aa4a0aed97602a786c6e16bc22a40f2695e8eb4c794a`.
DKMS runtime deployment binding SHA-256 is
`0f2f3ebe42cdc1e9454382d9fa75430a57e5d905241fe697ad07a18e4cf4d0cd`.
The installed companion includes this change's repeated-section repair,
SHA-256 `6a8e6df9e1a0f768d86f139a3de53a9a4f56b2180031bbb03a74c5d59cc25180`.
All observations used boot `f69985c3-bbd9-46cd-8bb1-04aac1e5af7e` and controller
session `14421307271028578308`.

## Findings and repair

The application INI writer appends section headers when saving newly introduced
keys. The installed configuration had repeated GPIO, WSPR and Band GPIO
sections with distinct keys. The route companion's strict ConfigParser rejected
this application-generated file before route mutation.

The companion now coalesces repeated sections in its parsing view while keeping
the original file bytes for editing. Duplicate keys remain rejected, including
duplicates across separate occurrences of a section. Only the requested pin and
transmit value are edited. Eight offline companion tests pass, including
repeated-section preservation and duplicate rejection. The 15 DKMS offline
restoration tests also pass.

## Target checks

- Running application: GPIO20 to GPIO4 to GPIO20 restored a new idle application
  process after each switch, with matching route/readiness and no clock/DMA work.
- Restart failure: a temporary `ExecStartPre=/usr/bin/false`, `Restart=no`
  drop-in forced startup failure after successful GPIO4 installation. The manager
  reported `application-restoration-failed`, retained inhibition and the original
  successful controller state. After removing the drop-in, `restore --execute`
  restored idle readiness without changing controller session, generation, route
  or overlay ID. The subsequent GPIO20 switch passed.
- Stopped application: GPIO4 and GPIO20 switches kept it stopped. A later
  explicit start acknowledged idle readiness and removed the startup override.
- Application HTTP API: preflight/switch through `/api/rp1-gpclk-route` completed
  GPIO20 to GPIO4 to GPIO20. Stopping the requester disconnected each HTTP
  request; the independent manager completed restoration. Fresh HTTP queries
  reported `runtime_ready` and matching active/configured routes afterward.
- No route: `recover --execute` produced route 0, overlay ID 0, flags 0 and
  error 0, with no consumer module or `/dev/rp1-gpclk` endpoint, application
  stopped/inhibited, and clock enable count 0. Switching back to GPIO20 restored
  the previously running application automatically.
- Final: GPIO20, application PID 39982 acknowledged, controller generation 19,
  no owner/lease/fault, GPIO/clock/DMA quiescent, clock enable count 0. The service
  unit remained byte-identical, no temporary drop-ins remained, and `/dev/null`
  retained mode 0666. PID and runtime observations describe this check only.

## Review and limits

Adversarial review checked configuration-byte preservation, duplicate rejection,
requester death, stopped-service intent, recovery without repeated overlay
effects, no-route absence, stable boot/controller ownership, service preservation
and final cleanup. The parser mismatch was repaired before repeating affected
checks; no unresolved finding remains in these exercised paths.

The service unit and its old mask had already been repaired before this run.
The full installer was not rerun and issue #434 remains separately tracked.
Browser rendering/clicks, kernel removal fault injection, reboot recovery and RF
operation were not tested here. HTTP API coverage is not browser visual coverage.
Target backups and intermediate records remain under
`/var/lib/rp1-gpclk-dkms/validation-restoration-20260831`.

## Documentation impact

This assessment records the tested behavior and limitation. Operator guidance in
the separate Wsprry_Pi_Docs repository was not changed; it should explain the
temporary connection loss during switching, no-route recovery stopping the
application, and application-only `restore --execute` recovery.
