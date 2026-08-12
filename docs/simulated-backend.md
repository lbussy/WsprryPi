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

## Debian CI

The `Debian non-hardware validation` workflow runs in a Debian Trixie container. It checks out the exact recorded submodules, builds the parent debug executable, runs the semantics and cleanup lifecycle suite, exercises the RP1 test-double regression, runs the simulator/controller/startup/Si5351 contract tests, compares two virtual-time traces, and audits file access with `strace`.

The workflow sets `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1`, passes an empty `SUDO` make variable, and rejects access to transmitter GPIO, MMIO, mailbox, DMA, I2C, or RP1 device paths. It does not run installation, service, test-tone, live qualification, or hardware targets.

Run the same checks locally from the repository root with the commands in `.github/workflows/debian-non-hardware.yml`. The workflow establishes software-contract evidence only; it is not physical-hardware or RF qualification.

Focused physical-backend tests remain separate and must continue to pass. Simulator results demonstrate application and backend-contract behavior only; they do not qualify GPIO timing, I2C hardware, RF output, installation, services, or any physical transmitter.
