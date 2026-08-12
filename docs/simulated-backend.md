# Hardware-free simulated backend

Issue 400 adds an explicitly selected, non-RF backend for Debian development and CI. It runs the normal request planning, execution-plan scheduling, cancellation, status, failure, and cleanup lifecycle through `ITransmissionBackend`, but it does not initialize GPIO, MMIO, mailbox, DMA, I2C, or transmitter device nodes.

## Build and run

Build the debug executable from `src`:

```sh
make debug
```

Run a short virtual-time QRSS execution as an unprivileged user:

```sh
./build/bin/wsprrypi_debug \
  --backend simulated \
  --no-web \
  --qrss-message E \
  --qrss-frequency 14097100 \
  --qrss-dot-seconds 0.01
```

Simulation is CLI-only and transient. It is never selected as a fallback and is not written to the application configuration. Physical GPIO and Si5351 selections retain their existing requirements and RF safety policy.

## Trace contract

The simulator writes `/tmp/wsprrypi-simulated-trace.json`. The stable JSON document identifies the schema version, backend, plan, ordered events, logical nanosecond timestamps, frequencies, RF state, completion, cancellation, injected failures, and cleanup. Virtual time is the default integration mode, so identical requests produce byte-identical traces without waiting for RF-duration wall-clock time.

The reusable submodule tests exercise normal completion, cancellation before and during execution, configure and execute failure, cleanup failure propagation, repeated execution, deterministic time, startup quiescence, identity, capabilities, and mode rejection:

```sh
cd WSPR-Transmitter/src
make simulated-backend-test transmission-controller-contract-test
```

The parent regression suite also exercises configuration failure, backend replacement, explicit shutdown, execution, cancellation, repeated cleanup, and destructor-safe cleanup handling:

```sh
cd ../..
make build/bin/cleanup_lifecycle_test
./build/bin/cleanup_lifecycle_test
```

## Complete WSPR integration check

The hardware-free CI suite also runs a complete WSPR request through the normal application CLI, WSPR reference preparation, execution-plan compiler, scheduler, simulated backend, completion, and cleanup paths. Run the same check from `src` after building the debug executable:

```sh
../scripts/ci/verify-simulated-wspr.sh
```

The script runs this unprivileged request twice with virtual time and compares the JSON traces byte for byte:

```sh
./build/bin/wsprrypi_debug --backend simulated --no-web --no-offset \
  --no-system-clock-frequency-estimate --terminate 1 K1ABC FN42 30 20m
```

It verifies the WSPR mode and plan identity, 162 ordered symbol events, monotonic logical timestamps, four evenly spaced tones, RF state, one completion, one cleanup, and the absence of cancellation or injected failure. A complete frame represents approximately 110.592 logical seconds but finishes without waiting for that wall-clock duration. Independent processes must produce byte-identical traces, demonstrating that simulator state does not leak between runs.

The final JSON trace is `/tmp/wsprrypi-wspr-trace.json`. The second run's file-access trace is `/tmp/wsprrypi-wspr-simulator.strace`; the CI hardware audit checks it together with the other non-hardware traces.

This check validates application and backend software contracts. It does not qualify WSPR timing on physical hardware, GPIO, DMA, mailbox, MMIO, I2C, Si5351 output, RF output or accuracy, installation, services, a Raspberry Pi model, a band, or a transmitter chain.

## Debian CI

The `Debian non-hardware validation` workflow runs in a Debian Trixie container. It checks out the exact recorded submodules, builds the parent debug executable, runs the semantics and cleanup lifecycle suite, exercises the RP1 test-double regression, runs the simulator/controller/startup/Si5351 contract tests, compares deterministic QRSS and complete WSPR virtual-time traces, and audits file access with `strace`.

The workflow sets `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1`, passes an empty `SUDO` make variable, and rejects access to transmitter GPIO, MMIO, mailbox, DMA, I2C, or RP1 device paths. It does not run installation, service, test-tone, live qualification, or hardware targets.

Run the same checks locally from the repository root with the commands in `.github/workflows/debian-non-hardware.yml`. The workflow establishes software-contract evidence only; it is not physical-hardware or RF qualification.

Focused physical-backend tests remain separate and must continue to pass. Simulator results demonstrate application and backend-contract behavior only; they do not qualify GPIO timing, I2C hardware, RF output, installation, services, or any physical transmitter.
