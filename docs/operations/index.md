# Automated Operations

## Licensing Requirements

```{admonition} You must have a HAM Radio License before transmitting using this experiment.
Please do not "just test" without a license. It's illegal, and you may be interfering with those of us who are licensed and using this correctly.
```

## Normal Client Operations

All normal control operations in typical usage revolve around the Wsprry Pi's configuration file.  This file is manipulated by a web UI and accessed by the `wspr` executable.  If `wspr` is running, a change to this file will effect at the next transmission period which is the top of an even minute for normal wspr, or at the top of the quarter hour for -15 operations.  If `wspr` is not actively transmitting and the transmit selection is changed, it will begin at the next proper wspr window.

## System Files and Control

The system consists of:

* `wspr` (executable): Installed to `/user/local/bin/wspr`
* `wspr.ini` (configuration): Installed to `/user/local/etc/wspr.ini`
* `wspr.service` (service control file): Installed to `/etc/systemd/system/wspr.service`
* `wspr` Web UI: Installed to `/var/www/html/wspr`

The basic data flow is:

![alt](_static/data_flow.png)

## System Daemon Usage

### Service Controls

The `wspr` executble is controlled by a `systemd` daemon called `wspr.service`.  This daemon is started automatically after installation.

* `wspr` Start:  `sudo systemctl start wspr`
* `wspr` Stop: `sudo systemctl stop wspr`
* `wspr` Status: `sudo systemctl status wspr`

For more information about interacting with services, please see `man systemctl`.

### Log Files

The `wspr` service will log messages like any other systemd executable.

* View all log files: `journalctl -u wspr`
* View log files since last system boot: `journalctl -u wspr -b`
* View the last page of the log file: `journalctl -u wspr -e`

