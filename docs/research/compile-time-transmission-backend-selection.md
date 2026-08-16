# Compile-time transmission backend selection

## Status

Research and implementation record. Compile-time transmission-backend source
selection, factory enforcement, and runtime capability reporting are
implemented through the `BACKENDS` Make variable and generated capability
definitions. Backend-specific privilege policy and a strict profile that
removes ancillary libgpiod support remain future work.

This investigation was prompted by an Ubuntu 24.04 x86 user who wanted to use
the Si5351 backend through a regular Linux `/dev/i2c-N` adapter. The user did not
select the hardware-free simulated backend, but simulation would not have met
the intended purpose because it deliberately performs no I2C access.

## Conclusion

An x86 Linux build that includes the Si5351 backend and excludes Raspberry Pi
clock/GPIO transmission is technically feasible. The Si5351 device layer uses
the standard Linux userspace I2C interface and does not inherently depend on a
Raspberry Pi processor, GPIO clock, mailbox, MMIO, PWM, or DMA.

Before this implementation, the executable could not be reduced to Si5351
merely by omitting one GPIO source file. Its Makefile discovered and linked
nearly all production sources, its backend factory contained unconditional
constructors for every backend, and `WsprTransmitter` initially constructed the
legacy Raspberry Pi GPIO backend before configuration selected the requested
backend. Those constraints motivated the explicit capability model below.

## Implemented transmission-backend profiles

The parent build now accepts any nonempty subset of these canonical names:

```sh
make BACKENDS=rpi-gpio,rp1-gpclk,si5351,simulated
make BACKENDS=si5351
make BACKENDS=si5351,simulated
```

The default remains the complete four-backend executable. The build validates
the selection, generates stable capability predicates, isolates objects and
retained profile executables by backend set, and conditionally links only the
requested transmission implementation groups. The established
`build/bin/wsprrypi` path receives the executable for the current invocation.

The transmitter factory compiles only enabled constructors. Selecting a valid
but omitted backend fails with a diagnostic that lists the compiled set; it
does not fall back. Initial construction selects the first compiled backend in
canonical order, preserving legacy GPIO startup for the default profile while
allowing a Si5351-only executable to avoid constructing a Raspberry Pi backend.

`BACKENDS=si5351` has been compiled on Ubuntu 24.04 x86_64 with GCC 13 and
audited to exclude legacy GPIO transmitter, Mailbox, RP1 GPCLK, and simulated
backend symbols. This is a no-GPIO-transmission profile, not yet the strict
I2C-only profile described below: ancillary LED, amplifier, shutdown-button,
band-selector, and libgpiod support remain compiled.

## Reported Ubuntu failure

The first reported error was:

```text
main.cpp:97:22: error: ignoring return value of ‘ssize_t write(int, const void*, size_t)’
declared with attribute ‘warn_unused_result’ [-Werror=unused-result]
```

The relevant common shutdown code is:

```cpp
(void)::write(g_async_shutdown_pipe[1], &wake, sizeof(wake));
```

This error is unrelated to GPIO and is not proof of an x86 incompatibility.
Ubuntu's fortified C library declaration marks `write()` with
`warn_unused_result`, while the project promotes all warnings to errors through
the common `-Werror` build flag. GCC does not reliably treat a cast to `void` as
use of a value returned by a function carrying that attribute.

The production correction should consume the return value without adding an
unsafe operation to the signal-notification path. Assigning it to a local
`ssize_t` and intentionally ignoring that local value is sufficient for this
best-effort nonblocking wakeup. The signal path must not log, allocate, throw,
or perform a blocking retry. Temporarily removing `-Werror` or using
`-Wno-error=unused-result` can expose subsequent portability errors during
diagnosis, but neither is the desired source fix.

Primary references:

- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [glibc source fortification](https://sourceware.org/glibc/manual/latest/html_node/Source-Fortification.html)

## Confirmed Linux I2C compatibility

`Si5351Device` already uses a Linux adapter that:

1. opens `/dev/i2c-<configured bus>` with `O_RDWR`;
2. selects the configured slave address with `ioctl(I2C_SLAVE, ...)`;
3. performs checked `read()` and `write()` calls; and
4. closes the device through the same adapter boundary.

That is the standard Linux `i2c-dev` userspace contract. A registered native,
USB, PCIe, or other Linux I2C adapter can provide `/dev/i2c-N`; the Si5351 layer
does not need to know the transport beneath that device. Adapter numbers are
assigned dynamically and can change, so the operator must continue to select
the correct bus rather than assuming `/dev/i2c-1`.

Primary reference:

- [Linux userspace I2C device interface](https://www.kernel.org/doc/html/latest/i2c/dev-interface.html)

This establishes software-interface compatibility only. It does not prove that
a particular adapter supports the required transactions, voltage levels,
pull-ups, bus rate, Si5351 board, or transmitter hardware.

## Pre-implementation compile and link coupling

The parent Makefile previously:

- included the `Mailbox` and complete `WSPR-Transmitter` source directories;
- recursively discovered nearly every C++ production source in those
  directories;
- always included libgpiod detection, compile flags, and link libraries; and
- linked the complete discovered object set into one application executable.

The runtime factory in `WsprTransmitter::createBackend()` contained constructors
for legacy Raspberry Pi clock/GPIO, RP1 GPCLK, Si5351, and simulated backends.
Those references require the corresponding implementations to be linked unless
the factory is also made compile-time aware.

In addition, the `WsprTransmitter` constructor selected
`RPI_CLOCK_GPIO` before CLI or INI parsing. Although this initial construction
does not by itself start RF or access hardware, it creates an unnecessary link
dependency and makes a physically unavailable backend the implicit initial
state.

## Build capability model

The implementation generates these independent build-time capabilities:

```text
WSPRRYPI_BACKEND_RPI_GPIO
WSPRRYPI_BACKEND_RP1_GPCLK
WSPRRYPI_BACKEND_SI5351
WSPRRYPI_BACKEND_SIMULATED
```

The user-facing Make variable maps a concise selection to those generated
definitions and source groups:

```sh
make BACKENDS=si5351
make BACKENDS=si5351,simulated
```

One authoritative build configuration now controls source selection, factory
availability, validation, reporting, and profile tests.

### Source selection

Backend source discovery has been replaced with explicit source groups. An
Si5351-only profile omits:

- the legacy Raspberry Pi clock/GPIO backend;
- the Raspberry Pi mailbox implementation;
- the RP1 GPCLK transmitter backend and Linux provider; and
- the simulated backend when it was not requested.

Backend-independent request, planning, scheduling, controller, logging, web,
and configuration sources remain present as needed by the application.

### Factory and initial state

Each backend factory arm is conditionally compiled. Selecting a backend that
was not compiled fails closed with an explicit diagnostic, such as:

```text
Transmission backend GPIO is unavailable in this build. Compiled backends: si5351.
```

There is no automatic fallback to Si5351, simulated, or any other backend.
Construction selects the first enabled backend in canonical order as an
explicit, generated default. The default all-backend profile therefore retains
legacy GPIO startup behavior, while reduced profiles do not acquire an omitted
backend dependency. Factory tests cover both enabled and omitted selections.

### Capability reporting

The compiled backend set is visible through:

- `--help` and `--version` output;
- the `compiled_backends` field in the `/version` JSON response; and
- the `--list-backends` command, which prints the canonical comma-separated
  set for scripts.

Command-line and persisted-configuration validation distinguish an invalid
backend name from a valid backend that is unavailable in the current build.
An omitted selection is rejected before runtime side effects, with the compiled
set in the diagnostic; it is not rewritten, normalized, or allowed to fall
back. The logical `gpio` selection maps to RP1 GPCLK capability on Raspberry Pi
5 and legacy Raspberry Pi GPIO capability on earlier Raspberry Pi generations.

### Transmission GPIO versus ancillary GPIO

Excluding Raspberry Pi clock/GPIO transmission is not identical to removing all
GPIO support. The application also uses libgpiod for TX LED, amplifier control,
shutdown-button input, and band-selection outputs.

At least two useful profiles exist:

1. **No GPIO transmission backend:** omit Raspberry Pi clock/DMA and RP1 GPCLK,
   but retain optional ancillary libgpiod controls.
2. **Strict I2C-only:** omit transmission GPIO and all ancillary libgpiod
   features, dependencies, code, and configuration acceptance.

The strict profile most directly matches an ordinary x86 host whose only
physical transmitter interface is `/dev/i2c-N`. Ancillary GPIO configuration
in that profile must fail clearly rather than being silently ignored.

## Runtime privilege boundary

The application currently requires root for every physical backend and exempts
only explicit simulation. Linux deployments commonly grant access to a
specific `/dev/i2c-N` through device ownership, groups, or udev rules, so a
future Si5351-only deployment might not technically need full root privilege.

That is a separate security and operator-workflow decision. Compile-time
backend selection should not silently remove the current privilege gate. A
later change could replace it with backend-specific access checks and clear
diagnostics while leaving physical GPIO protections intact.

## Safety and non-goals

An Si5351-only build is still a physical transmitter build. It must retain:

- explicit backend selection;
- Si5351 startup quiescence;
- output parking and cleanup;
- cancellation and shutdown handling;
- configuration and frequency validation; and
- failure inhibition when the I2C device cannot be opened or controlled.

Compile-time exclusion does not qualify I2C electrical behavior, Si5351 output,
frequency accuracy, RF output, filtering, amplifier operation, installation,
service lifecycle, or a transmitter chain. The simulated backend remains the
canonical hardware-free development path and must continue to perform no I2C
access.

Dynamic backend plugins, automatic hardware discovery, and automatic fallback
are not required to solve this problem. Static build capabilities fit the
existing `ITransmissionBackend` architecture and give the linker a verifiable
boundary.

## Validation status

The implemented profile and capability slices provide the following evidence:

1. Ubuntu 24.04 x86 compilation with GCC 13 and the normal `-Werror` policy.
2. Builds for the single-backend profiles, the Si5351-plus-simulated profile,
   and the default all-backend profile, plus intentional rejection when no
   backend is enabled.
3. Link and symbol audits proving that a Si5351-only executable contains no
   Raspberry Pi GPIO, RP1 provider, mailbox, MMIO, PWM, or DMA implementation.
4. CLI and configuration tests proving that omitted transmission-backend
   selections fail closed without fallback or persistence changes, while
   invalid names retain a distinct diagnostic.
5. Existing Si5351 fake-I2C planner, transition, startup-quiescence, controller,
   cancellation, and cleanup tests.
6. Existing simulated and Raspberry Pi backend regression suites in builds
   where those backends are enabled.

The following validation remains outside the implemented slices:

7. A file-access audit proving that an authorized Si5351-only run accesses the
   intended `/dev/i2c-N` and does not access `/dev/mem`, `/dev/vcio`, GPIO chip,
   RP1 GPCLK, mailbox, MMIO, PWM, or DMA paths.
8. Separately authorized validation of the intended adapter and Si5351 hardware.
9. Separately authorized RF and frequency qualification before making any
   physical-output claim.

## Documentation impact

This research and implementation record documents the developer/build contract,
available profiles, capability reporting, and validation boundary. Operator
documentation in the separate `Wsprry_Pi_Docs` repository will require review
if distributed packages expose different backend or ancillary-GPIO
capabilities; no package or operator workflow changes are part of this slice.
