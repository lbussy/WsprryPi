# WsprTransmitter

A C++17 class for transmitting WSPR messages or continuous test tones using DMA and PWM on Raspberry Pi. Runs in a dedicated real-time thread with controllable scheduling policy and priority.

## Features

- **WSPR & Test-Tone**: Supports both timed WSPR symbol sequences and indefinite test-tone transmission.
- **Real-Time Threading**: Runs transmission in a separate `std::thread` with configurable POSIX scheduling policy (`SCHED_FIFO`, `SCHED_RR`, `SCHED_OTHER`) and priority.
- **Clean Shutdown**: Provides `stop_transmission()`, `join_transmission()`, and `shutdown_transmitter()` for safe, coordinated thread termination.
- **DMA-Based**: Uses DMA control blocks and PWM clocks for precise timing.
- **Callback Support**: Optional `on_transmission_complete` callback for event-driven programs.

## Repository Structure

```text
├── src/
│   └── wspr_transmit.hpp   # Class declaration and public API
│   └── wspr_transmit.cpp   # Class implementation
│   └── main.cpp            # Sample usage in a standalone application
│   └── config_handler.cpp  # Supports sample usage in a standalone application
│   └── utils.cpp           # Supports sample usage in a standalone application
│   └── utils.hpp           # Supports sample usage in a standalone application
│   └── Makefile            # Compile the sample usage application
└── README.md               # This file
```

## Requirements

- C++17 compiler (e.g. `g++ 9+`)
- POSIX-compliant OS (Raspberry Pi OS recommended)
- Root or `CAP_SYS_NICE` privileges for real-time scheduling
- [Broadcom-Mailbox](https://github.com/lbussy/Broadcom-Mailbox) as a submodule for Mailbox methods
- [WSPR-Message](https://github.com/lbussy/WSPR-Message) as a submodule for WSPR Symbol generations

## Building

```bash
cd src
make debug
make test
```

## Usage Example

```cpp
#include "wspr_transmit.hpp"
#include <iostream>

int main() {
    WsprTransmitter tx;

    // Configure for test-tone:
    tx.setup_transmission(7.040100e6, 0, 12, "", "", 0, false);

    // Start in real-time thread (FIFO, priority 30)
    tx.start_threaded_transmission(SCHED_FIFO, 30);

    // Wait for user to stop
    std::cout << "Press <space> to stop...";
    pause_for_space();  // interlocks with tx.isStopping()

    // Clean shutdown
    tx.shutdown_transmitter();
    tx.dma_cleanup();

    std::cout << "Transmission stopped." << std::endl;
    return 0;
}
```

## API Reference

### `void setup_transmission(double freq, int tone, int ppm, const std::string &call = "", const std::string &grid = "", int power_dBm = 0, bool isWSPR = false)`

Configure frequency, PPM adjustment, power, callsign/grid (for WSPR), and mode.

### `void start_threaded_transmission(int policy, int priority)`

Launches transmission in a background thread with the given scheduling policy and priority.

### `void stop_transmission()`

Signals the transmit thread to exit at the next safe point.

### `void join_transmission()`

Blocks until the background transmission thread has terminated.

### `void shutdown_transmitter()`

Convenience: calls `stop_transmission()` then `join_transmission()`.

### `bool isStopping() const`

Returns `true` if a stop request has been issued.

### `std::function<void()> on_transmission_complete`

Optional callback invoked in the transmit thread when `transmit()` finishes.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE.md) for details.
