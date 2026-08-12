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

Focused physical-backend tests remain separate and must continue to pass. Simulator results demonstrate application and backend-contract behavior only; they do not qualify GPIO timing, I2C hardware, RF output, installation, services, or any physical transmitter.
