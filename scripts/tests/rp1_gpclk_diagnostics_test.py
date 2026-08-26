#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
collector = (root / "scripts/collect-support-bundle.sh").read_text()
runtime = (root / "src/scheduling.cpp").read_text()
websocket = (root / "src/web_socket.cpp").read_text()
admin_probe = (root / "src/WSPR-Transmitter/src/rp1_gpclk_admin_probe.cpp").read_text()

for command in ("dpkg-query", "dkms status", "modinfo", "overlay-sha256.txt"):
    assert command in collector, command
for evidence in ("rp1-gpclk-gpio4.dtbo", "rp1-gpclk-gpio20.dtbo",
                 "/var/lib/rp1-gpclk-dkms/route-transactions",
                 "RP1-GPCLK-DKMS OWNED ROUTE"):
    assert evidence in collector, evidence
for evidence in ("Persisted route", "Active route and live eligibility",
                 "Cleanup evidence", "Journal evidence", "rp1_gpclk_boot_route"):
    assert evidence in collector, evidence
assert "eb384beefcb1a0253062cffdfc3f6364594faa56" in collector
assert "eb384beefcb1a0253062cffdfc3f6364594faa56" in runtime
for identity in ("rp1-gpclk-dkms 1.1.2, ABI v3 passive snapshot",
                 "no package hash is accepted",
                 "Qualification: unvalidated"):
    assert identity in collector, identity
for identity in ("module=rp1-gpclk-dkms/1.1.2", "uapi_abi=3",
                 "package=unreleased", "qualification=unvalidated"):
    assert identity in runtime, identity
assert "dkms status -m rp1-gpclk-dkms -v 1.1.2" in collector
assert "package_sha256=247bd7da" not in runtime
for field in ("rp1_package_expected", "rp1_route_requested", "rp1_route_persisted",
              "rp1_route_configured", "rp1_route_active", "rp1_eligibility",
              "rp1_cleanup_state", "rp1_journal_state"):
    assert field in runtime and field in websocket, field
assert 'snapshot.rp1_route_active = "unavailable"' in runtime
assert 'snapshot.rp1_eligibility = "unknown"' in runtime
assert "provider.passiveSnapshot" in admin_probe
for prohibited in ("provider.acquire", "provider.release", "provider.submit", "O_RDWR"):
    assert prohibited not in admin_probe, prohibited
for evidence in ("read_only=true", "lease_token_exposed=false",
                 "indeterminate safety observation", "exact r3 development identity mismatch"):
    assert evidence in admin_probe, evidence
print("rp1_gpclk_diagnostics_test passed")
