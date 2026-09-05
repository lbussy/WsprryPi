# I2C bus selection

The Si5351 I2C bus selector lists adapters exposed by the running host through
`/sys/class/i2c-dev/i2c-N` with a corresponding character device `/dev/i2c-N`.
Entries are sorted by bus number and labeled with the adapter name when readable.
This inventory reads filesystem metadata only. It does not scan addresses,
open adapters, issue I2C transactions, or change host configuration.

The public configuration's read-only `Platform` section includes `I2C Buses`
(an array of objects with integer `Number` and string `Name`) and
`I2C Bus Discovery Error` (empty on successful discovery). These values are
transient host information, not persisted settings or client authority.

The selector refreshes when configuration is loaded. Reload after enabling,
adding, or removing adapters. A saved bus that disappears remains visible as
unavailable; it is never silently replaced. An empty inventory disables the
selector and explains that no buses are available. Discovery failure and missing
inventory information have separate messages. A replacement requires an explicit
selection. The existing integer `Si5351.I2C Bus` format remains unchanged.

New bus numbers submitted through the web configuration API, including changes
to an inactive Si5351 setting, are checked against fresh host metadata. Switching
to Si5351 also checks its bus. An unchanged unavailable bus does not prevent
recovery to a different backend. Existing startup and transmission readiness
checks still govern actual device use, including CLI and INI configuration.
A listed adapter is not proof of access permissions, external header routing,
a connected Si5351, or safe RF operation. Device detection remains separate.

## Hardware-free validation

From `src`, run `make i2c-bus-inventory-test SUDO=` for temporary filesystem
fixtures. The `i2c-bus-selection-test` target exercises rejection of a forged
inventory and nonexistent selection without changing configuration. Both targets
are included in the full and portable semantics profiles. Choose the host's
profile as documented in `AGENTS.md`.

From `WsprryPi-UI`, run `npm test` and
`node tests/conditional_transmit_gpio_integration_test.js`. The browser suite
checks available, missing, empty, failed, and absent inventory states; explicit
replacement; preservation in payloads; and backend-toggle behavior.

Operator-documentation follow-up belongs in the sibling documentation
repository's `docs/User_Interface/Setup/Transmitter/index.md`, under **I2C Bus**.
Its fixed bus-1 guidance should describe detected choices, unavailable saved
values, reload behavior, and the distinction between adapter presence and device
readiness. That repository is a separate change boundary.
