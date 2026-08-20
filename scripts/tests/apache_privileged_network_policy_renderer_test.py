#!/usr/bin/env python3

import json
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RENDERER = ROOT / "scripts" / "render-apache-privileged-network-policy.py"


def run_renderer(ini_text: str, interfaces: list[dict]) -> str:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        ini = root / "wsprrypi.ini"
        addresses = root / "addresses.json"
        output = root / "policy.conf"
        ini.write_text(ini_text, encoding="utf-8")
        addresses.write_text(json.dumps(interfaces), encoding="utf-8")
        subprocess.run([
            "python3", str(RENDERER), "--ini", str(ini),
            "--ip-json", str(addresses), "--output", str(output),
        ], check=True)
        return output.read_text(encoding="utf-8")


interfaces = [
    {
        "ifname": "eth0", "flags": ["BROADCAST", "UP"], "link_type": "ether",
        "addr_info": [{"family": "inet", "local": "192.168.50.12",
                       "prefixlen": 24, "scope": "global"}],
    },
    {
        "ifname": "docker0", "flags": ["BROADCAST", "UP"], "link_type": "ether",
        "addr_info": [{"family": "inet", "local": "172.17.0.1",
                       "prefixlen": 16, "scope": "global"}],
    },
]

enforced = run_renderer("[Security]\nPrivileged Network Safety = enforced\n", interfaces)
assert "Require ip 192.168.50.0/24" in enforced
assert "172.17.0.0/16" not in enforced
assert "^/wsprrypi/socket(?:/|$)" in enforced
assert "^/wsprrypi/api/network-safety$" in enforced
assert "<LimitExcept GET>" in enforced

invalid = run_renderer("[Security]\nPrivileged Network Safety = true\n", interfaces)
assert invalid == enforced

missing = run_renderer("[Common]\nCall Sign = TEST\n", interfaces)
assert missing == enforced

disabled = run_renderer(
    "[Security]\nPrivileged Network Safety = insecure-disabled\n", [])
assert disabled == (
    "# NETWORK SAFETY OFF\n"
    "# Peer/subnet restrictions are explicitly disabled.\n")
assert "Require" not in disabled

print("apache privileged-network policy renderer tests: PASS")
