#!/usr/bin/env python3
"""Build hardware-free RP1-GPCLK Step 4 application plans.

This external consumer emits JSON and invokes the published Qualification
Harness CLI.  It never imports Harness implementation modules or authorizes
execution.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import sys
from typing import Any

HARNESS_REVISION = "18246c76d2918dbbf2358ba01df0872839856d53"
SHA1 = re.compile(r"^[0-9a-f]{40}$")
CONTRACT_HEADER = pathlib.Path(__file__).resolve().parents[1] / "src/WSPR-Transmitter/src/rp1_gpclk_development_policy.hpp"


def contract_value(name: str) -> str:
    source = CONTRACT_HEADER.read_text(encoding="utf-8")
    match = re.search(rf"{re.escape(name)}\s*=\s*\n?\s*\"([^\"]+)\"", source)
    if not match:
        raise RuntimeError(f"canonical development contract is missing {name}")
    return match.group(1)


DKMS_REVISION = contract_value("kRp1GpclkDevelopmentSourceRevision")
MODULE_ID = contract_value("kRp1GpclkDevelopmentModuleId")
MODULE_VERSION = contract_value("kRp1GpclkDevelopmentModuleVersion")
UAPI_SHA256 = contract_value("kRp1GpclkDevelopmentUapiSha256")
COMPATIBILITY = {
    4: contract_value("kRp1GpclkDevelopmentGpio4Compatibility"),
    20: contract_value("kRp1GpclkDevelopmentGpio20Compatibility"),
}


def number(value: float) -> str:
    return format(value, ".15g")


def backend_contract(gpio: int) -> dict[str, Any]:
    return {
        "output": f"GPIO{gpio}",
        "ppm": 0,
        "drive_or_power_level": 2,
        "gpio_pin": gpio,
        "module_id": MODULE_ID,
        "module_version": MODULE_VERSION,
        "uapi_abi": 3,
        "uapi_sha256": UAPI_SHA256,
        "compatibility_id": COMPATIBILITY[gpio],
        "compatibility_state": "Experimental",
        "dkms_revision": DKMS_REVISION,
    }


def plan(
    *, mode: str, gpio: int, executable: str, source_revision: str,
    adapter_revision: str,
) -> dict[str, Any]:
    frequency = 14_097_100.0
    common = [
        executable, "--backend", "rp1-gpclk", "--transmit-gpio", str(gpio),
        "--gpio-power-level", "2", "--gpio-manual-ppm", "0", "--no-offset",
    ]
    protocol: dict[str, Any]
    arguments: list[str]
    if mode == "tone":
        protocol = {"requested_rf_frequency_hz": frequency, "duration_seconds": 1.0}
        arguments = [*common, "--test-tone", number(frequency)]
    elif mode == "wspr":
        protocol = {
            "callsign": "Q0QQQ", "grid": "JJ00", "power_dbm": 0,
            "frame_count": 3, "dial_frequency_hz": frequency - 1500,
            "audio_offset_hz": 1500.0, "requested_rf_frequency_hz": frequency,
        }
        arguments = [
            *common, "--terminate", "3", "Q0QQQ", "JJ00", "0",
            number(frequency - 1500),
        ]
    else:
        secondary = None if mode == "qrss" else frequency - 1
        protocol = {
            "message": "TEST", "dot_seconds": 3.0,
            "primary_frequency_hz": frequency,
            "secondary_frequency_hz": secondary,
        }
        flags = {
            "qrss": ["--qrss-message", "TEST", "--qrss-frequency", number(frequency)],
            "fskcw": ["--fskcw-message", "TEST", "--fskcw-mark-frequency",
                      number(frequency), "--fskcw-space-frequency", number(frequency - 1)],
            "dfcw": ["--dfcw-message", "TEST", "--dfcw-dot-frequency",
                     number(frequency), "--dfcw-dash-frequency", number(frequency - 1)],
        }[mode]
        arguments = [*common, *flags, f"--{mode}-dot-seconds", "3"]
    return {
        "schema_version": 1,
        "evidence_type": "application_plan",
        "plan_id": f"issue-412-step4-gpio{gpio}-{mode}",
        "identity": {
            "application": "wsprrypi", "executable": executable,
            "source_revision": source_revision,
            "submodule_revision": adapter_revision,
        },
        "backend": "rp1-gpclk", "backend_contract": backend_contract(gpio),
        "protocol": mode, "protocol_contract": protocol, "arguments": arguments,
        "self_terminating_request": True, "supervisor_required": True,
        "random_offset_enabled": False, "execution_authorized": False,
        "stopping_contract": "supervisor deadline and generation-specific bounded drain",
        "cleanup_contract": "STOP, stable terminal state, lease release, endpoint closure, and quiescence proof",
    }


def file_sha(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--route", choices=("gpio4", "gpio20"), required=True)
    parser.add_argument("--executable", required=True)
    parser.add_argument("--wsprrypi-revision", required=True)
    parser.add_argument("--adapter-revision", required=True)
    parser.add_argument("--harness-cli", required=True)
    parser.add_argument("--harness-revision", required=True)
    args = parser.parse_args(argv)
    if args.output.exists():
        parser.error("output destination already exists")
    if not SHA1.fullmatch(args.wsprrypi_revision) or not args.adapter_revision.strip():
        parser.error("exact WsprryPi and adapter revisions are required")
    if not args.harness_revision.startswith(HARNESS_REVISION):
        parser.error("Harness revision does not contain the reviewed RP1 contract")
    gpio = 4 if args.route == "gpio4" else 20
    args.output.mkdir(parents=True)
    records = []
    for mode in ("tone", "wspr", "qrss", "fskcw", "dfcw"):
        document = plan(
            mode=mode, gpio=gpio, executable=args.executable,
            source_revision=args.wsprrypi_revision,
            adapter_revision=args.adapter_revision,
        )
        path = args.output / f"{mode}-application-plan.json"
        path.write_text(json.dumps(document, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        subprocess.run(
            [args.harness_cli, "validate-application-plan", str(path)], check=True,
            stdin=subprocess.DEVNULL,
        )
        validation_record = {
            "schema_version": 1,
            "evidence_type": "hardware_free_application_plan_validation",
            "plan_id": document["plan_id"],
            "mode": mode,
            "route": args.route,
            "application_plan": path.name,
            "application_plan_sha256": file_sha(path),
            "semantic_validator": "wsprrypi-qualification validate-application-plan",
            "semantic_validation": "passed",
            "launch_contract": "exact structured argv in the application plan; execution unauthorized",
            "capture_contract": "STEP5_REQUIRED: receiver, calibration, pre-roll, post-roll, and immutable capture path",
            "analysis_contract": "STEP5_REQUIRED: mode-specific attributable bounds and rejection criteria",
            "cancellation_contract": document["stopping_contract"],
            "lifecycle_contract": "lease, generation, stable terminal reason, endpoint closure, and no successor",
            "cleanup_contract": document["cleanup_contract"],
            "terminal_result": "not_executed_hardware_free_plan_only",
            "artifact_destination": "STEP5_REQUIRED: new immutable destination",
            "operator_window": "STEP5_REQUIRED",
            "physical_path": "STEP5_REQUIRED",
            "authorization_digest": "STEP5_REQUIRED",
            "execution_authorized": False,
            "qualification_claim": False,
        }
        validation_path = args.output / f"{mode}-validation-record.json"
        validation_path.write_text(
            json.dumps(validation_record, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        records.append({
            "mode": mode, "route": args.route, "plan": path.name,
            "plan_sha256": file_sha(path),
            "validation_record": validation_path.name,
            "validation_record_sha256": file_sha(validation_path),
            "semantic_validation": "passed",
            "qualification_claim": False, "execution_authorized": False,
        })
    manifest = {
        "schema_version": 1, "evidence_type": "wsprrypi_step4_plan_set",
        "harness_revision": args.harness_revision, "dkms_revision": DKMS_REVISION,
        "route": args.route, "compatibility_id": COMPATIBILITY[gpio],
        "state": "Experimental", "hardware_access": False,
        "qualification_claim": False, "plans": records,
        "step5_required_values": [
            "operator_window", "target_identity", "receiver_identity",
            "physical_path", "safe_level_basis", "calibration_binding",
            "finite_tone_duration", "capture_bounds", "artifact_destination",
            "operator_authorized_plan_digest",
        ],
    }
    (args.output / "plan-set.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
