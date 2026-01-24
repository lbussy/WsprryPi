#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Update installed executables from Git repo."""

import os
import subprocess

def get_git_root():
    """Determine the Git repository root directory."""
    try:
        return subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], text=True).strip()
    except subprocess.CalledProcessError:
        print("Error: Not inside a Git repository.")
        exit(1)

def service_exists(service_name):
    """Check if a systemd service exists."""
    subprocess.call(['systemctl', 'list-units', '--type=service', '--all', '--no-pager'],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return service_name in subprocess.check_output(
        ['systemctl', 'list-units', '--type=service', '--all', '--no-pager'],
        text=True
    )

def stop_and_disable_service(service_name):
    """Stop and disable a systemd service if it exists."""
    if service_exists(service_name):
        subprocess.call(['sudo', 'systemctl', 'stop', service_name])
        subprocess.call(['sudo', 'systemctl', 'disable', service_name])

def copy_file(src, dest):
    """Copy a file to a destination with correct permissions."""
    if os.path.exists(src):
        subprocess.call(['sudo', 'cp', src, dest])
        subprocess.call(['sudo', 'chown', 'root:root', dest])
        subprocess.call(['sudo', 'chmod', '755', dest])
    else:
        print(f"Warning: {src} does not exist, skipping.")

def clear_logs(log_dir):
    """Delete all logs in a specified directory."""
    if os.path.exists(log_dir):
        subprocess.call(['sudo', 'rm', '-rf', os.path.join(log_dir, '*')])

def enable_and_start_service(service_name):
    """Enable and start a systemd service."""
    subprocess.call(['sudo', 'systemctl', 'enable', service_name])
    subprocess.call(['sudo', 'systemctl', 'start', service_name])

def main():
    """Check for and repoace executables."""
    git_root = get_git_root()

    wsprrypi_service = "wsprrypi.service"

    # Paths
    executables_dir = os.path.join(git_root, "executables")
    wsprrypi_bin = os.path.join(executables_dir, "wsprrypi")
    wsprrypi_dest = "/usr/local/bin/wsprrypi"
    log_dir = "/var/log/wsprrypi"

    # Handle wsprrypi service
    stop_and_disable_service(wsprrypi_service)
    copy_file(wsprrypi_bin, wsprrypi_dest)

    # Clear logs
    clear_logs(log_dir)

    # Enable and start services
    enable_and_start_service(wsprrypi_service)

if __name__ == "__main__":
    main()
