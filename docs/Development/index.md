<!-- Grammar and spelling checked -->
# Wsprry Pi Development

All current development has been done with g++ version 10.2.1 20210110 (Raspbian 10.2.1-6+rpi1).  The compilation is done on Bullseye to allow the compiled executable to be run on Bullseye (`old stable) and Bookworm (`stable`).

Because of modularity, re-use, and licensing considerations, the central WsprryPi repository references the following Git Repositories as submodules:

- [WsprryPi-UI](https://github.com/lbussy/WsprryPi-UI): The Bootstrap-based Wsprry Pi Web UI.
- [Broadcom-Mailbox](https://github.com/lbussy/Broadcom-Mailbox): Boradcom's interface for Mailbox communication.
- [INI-Handler](https://github.com/lbussy/INI-Handler): A class that allows reading and wriging formatted INI cfiles.
- [LCBLog](https://github.com/lbussy/LCBLog): A class to handle log formatting, writing, and timestamps.
- [MonitorFile](https://github.com/lbussy/MonitorFile): A class to watch a file for changes.
- [PPM-Manager](https://github.com/lbussy/PPM-Manager): A class to maintain an eye on the current PPM value for the OS/clock.
- [Signal-Handler](https://github.com/lbussy/Signal-Handler): A class to intercept various signals (e.g. SIGINT), and allow proper handling.
- [Singleton](https://github.com/lbussy/Singleton): A class that enforces running only a single instance.
- [WSPR-Message](https://github.com/lbussy/WSPR-Message): A class that creates WSPR symbols from the callsign, gridsquare, and power in dBm.
- [WSPR-Transmitter](https://github.com/lbussy/WSPR-Transmitter): A class that manages DMA work and scheduling for transmistting WSPR symbols.

In theory, this organization will allow more modular changes (like maybe adding Raspberry Pi 5 support?) in the future.
