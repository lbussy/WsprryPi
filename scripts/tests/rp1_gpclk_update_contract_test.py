#!/usr/bin/env python3
"""Exercise the actual provider/installer JSON boundary without hardware.

Pass an explicit RP1-GPCLK-DKMS checkout. Only its offline test adapters run.
"""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

APP = Path(__file__).resolve().parents[2]
if len(sys.argv) != 2:
    raise SystemExit("usage: rp1_gpclk_update_contract_test.py /path/to/RP1-GPCLK-DKMS")
PROVIDER = Path(sys.argv.pop()).resolve(strict=True)
sys.path[:0] = [str(PROVIDER / "scripts"), str(PROVIDER / "tests")]
import runtime_provider as provider
from check_runtime_provider import UpdateHost

spec = importlib.util.spec_from_file_location("wsprrypi_installer", APP / "scripts/rp1_gpclk_dkms_install.py")
installer = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = installer
spec.loader.exec_module(installer)


class PublicRunner:
    def __init__(self, host):
        self.host = host
        self.calls = []

    def run(self, command, **unused):
        self.calls.append(command[2])
        if command[:2] != ["python3", str(PROVIDER / "scripts/runtime_provider.py")]:
            raise AssertionError("unexpected operational command")
        output = io.StringIO()
        with mock.patch.object(sys, "argv", command[1:]), contextlib.redirect_stdout(output):
            result = provider.main(self.host)
        return installer.CommandResult(output.getvalue(), "", result)


class ContractTests(unittest.TestCase):
    def test_real_json_contract_and_lifecycle_planners(self):
        for route in (None, 4, 20, "removed"):
            for active, masked in ((True, False), (False, False), (False, True)):
                if route in (4, 20) and not active:
                    continue
                with self.subTest(route=route, active=active, masked=masked):
                    host = UpdateHost(active, masked, removed=route == "removed")
                    receipt = provider.installation_status(host)["receipt"]
                    if route in (4, 20):
                        host.select_route(route)
                    record = {"schema": installer.RUNTIME_RECORD_SCHEMA, "channel": "development",
                              "sourceCommit": host.value["sourceCommit"], "productVersion": "0.9.0",
                              "runtime": receipt}
                    runner = PublicRunner(host)
                    with tempfile.TemporaryDirectory() as tmp, \
                         mock.patch.object(installer, "RUNTIME_PROVIDER", PROVIDER / "scripts/runtime_provider.py"), \
                         mock.patch.object(installer.platform, "release", return_value=host.running_kernel()), \
                         mock.patch.object(installer, "require_unchanged_ownership"):
                        installer.recover_and_remove_owned_runtime(Path(tmp) / "record.json", record, object(), runner)
                    self.assertTrue(host.absent)
                    self.assertIn("update-execute", runner.calls)
                    self.assertTrue(set(runner.calls) <= {"capabilities", "update-plan", "update-execute"})

    def test_installer_cannot_override_provider_identity_or_unsafe_state(self):
        for bad in ("identity", "owner", "artifact"):
            host = UpdateHost()
            receipt = provider.installation_status(host)["receipt"]
            if bad == "identity": receipt["bindingSha256"] = "0" * 64
            elif bad == "owner": host.system.controller_open = True
            else: host._artifacts[next(iter(host._artifacts))]["status"] = "changed"
            record = {"schema": installer.RUNTIME_RECORD_SCHEMA, "channel": "development",
                      "sourceCommit": host.value["sourceCommit"], "productVersion": "0.9.0", "runtime": receipt}
            runner = PublicRunner(host)
            with mock.patch.object(installer, "RUNTIME_PROVIDER", PROVIDER / "scripts/runtime_provider.py"), \
                 mock.patch.object(installer.platform, "release", return_value=host.running_kernel()), \
                 mock.patch.object(installer, "require_unchanged_ownership"):
                with self.assertRaises(ValueError):
                    installer.recover_and_remove_owned_runtime(Path("/offline"), record, object(), runner)
            self.assertNotIn("update-execute", runner.calls)
            self.assertEqual(host.effects, [])


if __name__ == "__main__":
    unittest.main()
