<!-- omit in toc -->
# Apache Tool Documentation

<!-- omit in toc -->
## Table of Cantents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Installation \& Execution](#installation--execution)
- [Environment Variables](#environment-variables)
- [Command‑Line Options](#commandline-options)
- [Examples](#examples)
- [Apache Configuration Changes](#apache-configuration-changes)
- [Function Reference](#function-reference)
- [License](#license)


## Overview

**`apache_tool.sh`** is a Bash script designed for the Wsprry Pi project to automate common Apache HTTP Server configuration tasks:

- **Suppress FQDN warnings** by adding or removing a `ServerName` directive.
- **Create** a catch‑all virtual host that redirects `/` to `/wsprrypi/`.
- **Install** and **uninstall** the custom site under Debian’s `sites-available`/`sites-enabled` mechanism.
- **Test** Apache configuration syntax and **restart** the service.

## Prerequisites

- Debian‑based system with Apache2 installed (`apache2ctl` available).
- `sudo` privileges to modify `/etc/apache2` and restart the server.
- Standard Unix utilities: `sed`, `grep`, `stat`, `tee`, `systemctl`.

## Installation & Execution

Currently, this is an online script, to be executed via curl:

```bash
curl -fSSL https://raw.githubusercontent.com/lbussy/WsprryPi/refs/heads/apache_tool/scripts/apache_tool.sh | sudo bash
```

## Environment Variables

| Variable               | Default                     | Description                                                    |
| ---------------------- | --------------------------- | -------------------------------------------------------------- |
| `TARGET_FILE`          | `/var/www/html/index.html`  | Path to the default landing page to detect stock Apache page.  |
| `APACHE_CONF`          | `/etc/apache2/apache2.conf` | Main Apache configuration file.                                |
| `LOG_FILE`             | `/var/log/apache_tool.log`  | File to append script logs if enabled.                         |
| `SERVERNAME_DIRECTIVE` | `ServerName localhost`      | Directive to suppress "Could not reliably determine…" warning. |
| `DRY_RUN`              | `0`                         | If `1`, commands are logged but not executed.                  |
| `VERBOSE`              | `0`                         | If `1`, prints detailed actions to stdout.                     |
| `LOGGING_ENABLED`      | `0`                         | If `1`, appends colored logs to `$LOG_FILE`.                   |
| `SELF_TEST`            | `0`                         | If `1`, runs dependency and file‑existence checks only.        |
| `REQUIRE_SUDO`         | `true`                      | Enforce running via `sudo`.                                    |

## Command‑Line Options

| Option        | Env Variable           | Description                                          |
| ------------- | ---------------------- | ---------------------------------------------------- |
| `-u`          | `DISABLE_MODE`         | Uninstall mode: remove ServerName & custom site.     |
| `-n`          | `DRY_RUN`              | Simulate actions without changing files or services. |
| `-v`          | `VERBOSE`              | Enable verbose logging.                              |
| `-l`          | `LOGGING_ENABLED`      | Enable file logging to `$LOG_FILE`.                  |
| `-t`          | `SELF_TEST`            | Validate dependencies and file existence, then exit. |
| `-f <FILE>`   | `TARGET_FILE`          | Custom path to landing page.                         |
| `-c <FILE>`   | `APACHE_CONF`          | Custom path to Apache config.                        |
| `-o <FILE>`   | `LOG_FILE`             | Custom log file path.                                |
| `-s <STRING>` | `SERVERNAME_DIRECTIVE` | Custom ServerName directive.                         |
| `-h`          | n/a                    | Display help message and exit.                       |

## Examples

- **Default install**:

  ```bash
  sudo ./apache_tool.sh
  ```

- **Uninstall**:

  ```bash
  sudo ./apache_tool.sh -u
  ```

- **Dry-run with custom ServerName and verbose**:

  ```bash
  SERVERNAME_DIRECTIVE="ServerName myhost.local" DRY_RUN=1 \
    sudo ./apache_tool.sh -n -v
  ```

- **Self-test dependencies**:

  ```bash
  sudo ./apache_tool.sh -t
  ```

## Apache Configuration Changes

This script makes the following changes to your Apache configuration to implement the `/wsprrypi/` redirect:

- **ServerName Directive**
  Inserts a line at the very top of `/etc/apache2/apache2.conf`:

  ```apache
  ServerName <hostname>
  ```

  (e.g. `ServerName localhost` or your custom hostname) to suppress the “Could not reliably determine the server's fully qualified domain name” warning.

- **Custom Site Definition**
  Creates a new file `/etc/apache2/sites-available/wsprrypi.conf` containing:

  ```apache
  <VirtualHost *:80>
      ServerName <hostname>   # matches your ServerName setting
      # Alternatively, use:
      # ServerAlias *        # to catch any Host: header
      DocumentRoot /var/www/html
      RedirectMatch 301 ^/$ /wsprrypi/
  </VirtualHost>
  ```

  This defines a catch‑all virtual host that redirects requests for `/` to `/wsprrypi/`.

- **Site Enable/Disable**

  * Disables the stock default site (`000-default.conf`) via `sudo a2dissite 000-default.conf`.
  * Enables your new `wsprrypi.conf` via `sudo a2ensite wsprrypi.conf`.

- **Uninstall Reversal**
  When run with `-u`, the script:

  1. Disables `wsprrypi.conf` (`sudo a2dissite wsprrypi.conf`).
  2. Re‑enables `000-default.conf` (`sudo a2ensite 000-default.conf`).
  3. Deletes `/etc/apache2/sites-available/wsprrypi.conf` to clean up.

## Function Reference

The script is organized into modular functions. Main highlights:

- **Debugging**: `debug_start`, `debug_print`, `debug_end`, `debug_filter` handle optional `debug` flag.
- **Logging**: `log`, `verbose_log`, `rotate_logs`, `backup_file` provide colored console and file logging.
- **Checks**: `check_apache_installed`, `check_required_commands`, `self_test`.
- **Configuration tasks**:
  - `add_servername_directive` / `remove_servername_directive`
  - `setup_wsprrypi_site` / `disable_wsprrypi_site`
- **Validation**: `test_apache_config` uses `apache2ctl configtest`.
- **Restart**: `restart_apache` calls `systemctl restart apache2`.
- **Detection**: `is_stock_apache_page` matches known Apache landing-page text.
- **Main flow**: `_main` handles option parsing, calls install/uninstall tasks, then `main` orchestrates the run.

## License

This project is released under the **MIT License**. See the header in `apache_tool.sh` for full terms.
