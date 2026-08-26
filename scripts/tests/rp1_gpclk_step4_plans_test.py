#!/usr/bin/env python3
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
GENERATOR = ROOT / "scripts/rp1_gpclk_step4_plans.py"


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: test HARNESS_CLI HARNESS_REVISION")
    harness, revision = sys.argv[1:]
    with tempfile.TemporaryDirectory() as temporary:
        root = pathlib.Path(temporary)
        for gpio in (4, 20):
            output = root / f"gpio{gpio}"
            subprocess.run([
                sys.executable, str(GENERATOR), "--output", str(output),
                "--route", f"gpio{gpio}", "--executable", "/opt/wsprrypi/wsprrypi",
                "--wsprrypi-revision", "1" * 40, "--adapter-revision", "2" * 40,
                "--harness-cli", harness, "--harness-revision", revision,
            ], check=True)
            manifest = json.loads((output / "plan-set.json").read_text())
            assert [item["mode"] for item in manifest["plans"]] == [
                "tone", "wspr", "qrss", "fskcw", "dfcw"
            ]
            assert all(not item["qualification_claim"] for item in manifest["plans"])
            for item in manifest["plans"]:
                plan_path = output / item["plan"]
                record_path = output / item["validation_record"]
                assert hashlib.sha256(plan_path.read_bytes()).hexdigest() == item["plan_sha256"]
                assert hashlib.sha256(record_path.read_bytes()).hexdigest() == item["validation_record_sha256"]
                record = json.loads(record_path.read_text())
                assert record["terminal_result"] == "not_executed_hardware_free_plan_only"
                assert record["capture_contract"].startswith("STEP5_REQUIRED:")
                assert record["analysis_contract"].startswith("STEP5_REQUIRED:")
                assert not record["execution_authorized"] and not record["qualification_claim"]
            wrong = json.loads((output / "tone-application-plan.json").read_text())
            wrong["backend_contract"]["compatibility_id"] = (
                "v1.1.2-pi5-gpio20-6.18.34-development-candidate-r3"
                if gpio == 4 else "v1.1.2-pi5-gpio4-6.18.34-development-candidate-r3"
            )
            altered = root / f"gpio{gpio}-wrong.json"
            altered.write_text(json.dumps(wrong))
            assert subprocess.run(
                [harness, "validate-application-plan", str(altered)],
                stdin=subprocess.DEVNULL, capture_output=True,
            ).returncode != 0
        reused = root / "gpio4"
        assert subprocess.run([
            sys.executable, str(GENERATOR), "--output", str(reused),
            "--route", "gpio4", "--executable", "/opt/wsprrypi/wsprrypi",
            "--wsprrypi-revision", "1" * 40, "--adapter-revision", "2" * 40,
            "--harness-cli", harness, "--harness-revision", revision,
        ], capture_output=True).returncode != 0
    print("RP1 GPCLK Step 4 external Harness plan tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
