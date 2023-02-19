# Wsprry Pi System Internals

The system consists of the following:

- `wspr` (executable): Installed to `/user/local/bin/wspr`
- `wspr.ini` (configuration): Installed to `/user/local/etc/wspr.ini`
- `wspr.service` (service control file): Installed to `/etc/systemd/system/wspr.service`
- Wsprry Pi Web UI: Installed to `/var/www/html/wspr`

The primary data flow is as follows:

![Data Flow Diagram](data_flow.png)

## Log Files

The `wspr` service will log messages like any other systemd executable.

- View all log files: `journalctl -u wspr`
- View log files since the last system boot: `journalctl -u wspr -b`
- View the last page of the log file: `journalctl -u wspr -e`
