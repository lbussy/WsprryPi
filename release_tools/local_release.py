#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Copy project executables to installed location."""

import os
import subprocess
import sys


def get_git_root():
    """Determine the Git repository root directory."""
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"], text=True
        ).strip()
    except subprocess.CalledProcessError:
        return None  # Indicate failure to find the git root


def service_exists(service_name):
    """Check if a systemd service exists."""
    try:
        result = subprocess.check_output(
            ["systemctl", "list-unit-files", "--type=service", "--no-pager"], text=True
        )
        return service_name in result
    except subprocess.CalledProcessError:
        return False  # If the command fails, assume the service does not exist


def stop_and_disable_service(service_name):
    """Stop and disable a systemd service if it exists."""
    if service_exists(service_name):
        subprocess.call(["sudo", "systemctl", "stop", service_name])
        subprocess.call(["sudo", "systemctl", "disable", service_name])
    else:
        print(f"Error: Service '{service_name}' is not installed. Exiting.")
        sys.exit(1)


def copy_file(src, dest):
    """Copy a file to a destination with correct permissions."""
    if os.path.exists(src):
        subprocess.call(["sudo", "cp", src, dest])
        subprocess.call(["sudo", "chown", "root:root", dest])
        subprocess.call(["sudo", "chmod", "755", dest])
    else:
        print(f"Error: Required executable '{src}' not found. Exiting.")
        sys.exit(1)


def clear_logs(log_dir):
    """Delete all logs in a specified directory."""
    if os.path.exists(log_dir):
        log_pattern = os.path.join(log_dir, "*")
        try:
            # raises CalledProcessError on non-zero exit
            subprocess.check_call(["sudo", "rm", "-rf", log_pattern])
        except subprocess.CalledProcessError as e:
            print(f"Error clearing logs in {log_dir}: return code {e.returncode}")
        except OSError as e:
            # e.g. if 'sudo' binary isn’t found, or permission error
            print(f"OS error clearing logs in {log_dir}: {e}")


def enable_and_start_service(service_name):
    """Enable and start a systemd service if it exists."""
    if service_exists(service_name):
        subprocess.call(["sudo", "systemctl", "enable", service_name])
        subprocess.call(["sudo", "systemctl", "start", service_name])
    else:
        print(f"Error: Service '{service_name}' does not exist. Exiting.")
        sys.exit(1)


def main():
    """
    Entry point.
    """
    git_root = get_git_root()

    if not git_root:
        print("Error: Not inside a Git repository.")
        sys.exit(1)  # Exit with an error code

    wsprrypi_service = "wsprrypi.service"

    # Paths
    executables_dir = os.path.join(git_root, "executables")
    wsprrypi_bin = os.path.join(executables_dir, "wsprrypi")
    wsprrypi_dest = "/usr/local/bin/wsprrypi"
    log_dir = "/var/log/wsprrypi"

    # Check if required executables exist before proceeding
    if not os.path.exists(wsprrypi_bin):
        print(
            f"Error: '{wsprrypi_bin}' not found. Build the project before running this script."
        )
        sys.exit(1)

    # Check if required services exist
    if not service_exists(wsprrypi_service):
        print(
            f"Error: Service '{wsprrypi_service}' does not exist. Ensure it is installed."
        )
        sys.exit(1)

    # Handle wsprrypi service
    stop_and_disable_service(wsprrypi_service)
    copy_file(wsprrypi_bin, wsprrypi_dest)

    # Clear logs
    clear_logs(log_dir)

    # Enable and start services
    enable_and_start_service(wsprrypi_service)


if __name__ == "__main__":
    main()
