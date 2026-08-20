#!/usr/bin/env python3
"""Render the managed Apache browser-peer policy without changing Apache."""

import argparse
import configparser
import ipaddress
import json
import os
import subprocess
import tempfile
from pathlib import Path


EXCLUDED_PREFIXES = (
    "br-", "docker", "veth", "virbr", "podman", "cni", "flannel",
    "tun", "tap", "wg", "tailscale", "zt",
)


def configured_mode(ini_path: Path) -> str:
    parser = configparser.ConfigParser(interpolation=None)
    try:
        with ini_path.open(encoding="utf-8") as source:
            parser.read_file(source)
        value = parser.get("Security", "Privileged Network Safety").strip()
    except (OSError, configparser.Error, KeyError):
        return "enforced"
    return value if value in {"enforced", "insecure-disabled"} else "enforced"


def eligible_networks(addresses: list[dict]) -> list[str]:
    networks: set[str] = set()
    for interface in addresses:
        name = str(interface.get("ifname", "")).lower()
        flags = {str(flag).upper() for flag in interface.get("flags", [])}
        if not name or name == "lo" or name.startswith(EXCLUDED_PREFIXES):
            continue
        if "UP" not in flags or "LOOPBACK" in flags or "POINTOPOINT" in flags:
            continue
        if str(interface.get("link_type", "")).lower() not in {"ether", ""}:
            continue
        for info in interface.get("addr_info", []):
            if info.get("scope") != "global" or info.get("family") not in {"inet", "inet6"}:
                continue
            try:
                address = ipaddress.ip_address(info["local"])
                prefix = int(info["prefixlen"])
                network = ipaddress.ip_network(f"{address}/{prefix}", strict=False)
            except (KeyError, TypeError, ValueError):
                continue
            if address.is_loopback or address.is_multicast or address.is_unspecified:
                continue
            networks.add(str(network))
    return sorted(networks)


def render(mode: str, networks: list[str]) -> str:
    if mode == "insecure-disabled":
        return "# NETWORK SAFETY OFF\n# Peer/subnet restrictions are explicitly disabled.\n"
    if not networks:
        raise RuntimeError("no eligible directly connected LAN was discovered")

    def requirement() -> str:
        lines = ["    <RequireAny>", "        Require local"]
        lines.extend(f"        Require ip {network}" for network in networks)
        lines.append("    </RequireAny>")
        return "\n".join(lines)

    blocks = [
        '<LocationMatch "^/wsprrypi/config$">\n    <LimitExcept GET>\n'
        + requirement() + '\n    </LimitExcept>\n</LocationMatch>',
        '<LocationMatch "^/wsprrypi/config/">\n' + requirement() + '\n</LocationMatch>',
        '<LocationMatch "^/wsprrypi/control/stop(?:/|$)">\n'
        + requirement() + '\n</LocationMatch>',
        '<LocationMatch "^/wsprrypi/api/support-bundles(?:/|$)">\n'
        + requirement() + '\n</LocationMatch>',
        '<LocationMatch "^/wsprrypi/socket(?:/|$)">\n' + requirement() + '\n</LocationMatch>',
    ]
    header = (
        "# Managed by WsprryPi. Manual edits will be replaced.\n"
        "# Browser-peer policy uses only Apache's connection address.\n\n"
    )
    return header + "\n\n".join(blocks) + "\n"


def atomic_write(path: Path, contents: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=path.name + ".", dir=path.parent)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as output:
            output.write(contents)
            output.flush()
            os.fsync(output.fileno())
        os.chmod(temporary, 0o644)
        os.replace(temporary, path)
    except Exception:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass
        raise


def main() -> int:
    arguments = argparse.ArgumentParser()
    arguments.add_argument("--ini", required=True, type=Path)
    arguments.add_argument("--output", required=True, type=Path)
    arguments.add_argument("--ip-json", type=Path)
    args = arguments.parse_args()

    mode = configured_mode(args.ini)
    if mode == "insecure-disabled":
        networks: list[str] = []
    else:
        if args.ip_json:
            addresses = json.loads(args.ip_json.read_text(encoding="utf-8"))
        else:
            completed = subprocess.run(
                ["ip", "-j", "address", "show", "up", "scope", "global"],
                check=True, capture_output=True, text=True)
            addresses = json.loads(completed.stdout)
        networks = eligible_networks(addresses)
    atomic_write(args.output, render(mode, networks))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
