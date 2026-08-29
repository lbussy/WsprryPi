#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
collector = (root / "scripts/collect-support-bundle.sh").read_text()
runtime = (root / "src/scheduling.cpp").read_text()
websocket = (root / "src/web_socket.cpp").read_text()
admin_probe = (root / "src/WSPR-Transmitter/src/rp1_gpclk_admin_probe.cpp").read_text()

for command in ("dkms status", "modinfo", "overlay-sha256.txt",
                "rp1_gpclk_module_state", "rp1_gpclk_boot_route"):
    assert command not in collector, command
for evidence in ("/dev/rp1-gpclk",
                 "/var/lib/rp1-gpclk-dkms/route-transactions"):
    assert evidence in collector, evidence
for evidence in ("Provider provisioning and package identity: external to WsprryPi",
                 "Persisted route", "Active route and operation readiness",
                 "Qualification: not established"):
    assert evidence in collector, evidence
for identity in ("source_commit=", "module=rp1-gpclk-dkms/",
                 "package=unreleased", "development-candidate"):
    assert identity not in collector and identity not in runtime, identity
for field in ("rp1_route_requested", "rp1_route_persisted",
              "rp1_route_configured", "rp1_route_active", "rp1_eligibility",
              "rp1_cleanup_state", "rp1_journal_state"):
    assert field in runtime and field in websocket, field
assert 'snapshot.rp1_route_active = "unavailable"' in runtime
assert 'snapshot.rp1_eligibility = "unknown"' in runtime
assert "provider.passiveSnapshot" in admin_probe
for prohibited in ("provider.acquire", "provider.release", "provider.submit", "O_RDWR"):
    assert prohibited not in admin_probe, prohibited
for evidence in ("read_only=true", "lease_token_exposed=false",
                 "indeterminate safety observation", "provider route mismatch"):
    assert evidence in admin_probe, evidence
print("rp1_gpclk_diagnostics_test passed")
