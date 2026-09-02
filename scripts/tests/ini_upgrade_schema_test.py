#!/usr/bin/env python3
"""Exercise the installer's INI migration against the current schema."""

from __future__ import annotations

import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
INSTALLER = ROOT / "scripts" / "install.sh"
CURRENT_INI = ROOT / "config" / "wsprrypi.ini"


def extract_upgrade_program(installer: str) -> str:
    function_start = installer.index("upgrade_ini() {")
    program_start = installer.index("    mawk '\n", function_start) + len("    mawk '\n")
    program_end = installer.index("\n    ' \"$old_ini\" \"$new_ini\"", program_start)
    return installer[program_start:program_end]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def upgrade(
    awk: str,
    program: str,
    old_ini: Path,
    current_ini: Path,
    output: Path,
) -> str:
    with output.open("w", encoding="utf-8") as stream:
        subprocess.run(
            [awk, program, str(old_ini), str(current_ini)],
            check=True,
            stdout=stream,
            text=True,
        )
    return output.read_text(encoding="utf-8")


def main() -> None:
    awk = shutil.which("mawk") or shutil.which("awk")
    require(awk is not None, "no awk implementation is available")

    current = CURRENT_INI.read_text(encoding="utf-8")
    require("\n22m =" not in current, "current INI still contains 22m")
    require(
        "\n22m Active High =" not in current,
        "current INI still contains 22m Active High",
    )
    require(
        current.count("RP1 Drive mA = 2") == 1,
        "current INI must declare the canonical RP1 drive default exactly once",
    )

    program = extract_upgrade_program(INSTALLER.read_text(encoding="utf-8"))

    with tempfile.TemporaryDirectory(prefix="wsprrypi-ini-upgrade-") as directory:
        root = Path(directory)
        old_ini = root / "old.ini"
        merged_ini = root / "merged.ini"
        stock_ini = root / "installed.ini.stock"

        old_ini.write_text(
            "[Band GPIO]\n"
            "22m = 17\n"
            "22m Active High = true\n"
            "20m = 23\n"
            "20m Active High = true\n"
            "\n"
            "[WSPR]\n"
            "Call Sign = UPGRADE\n",
            encoding="utf-8",
        )

        merged = upgrade(awk, program, old_ini, CURRENT_INI, merged_ini)

        shutil.copyfile(CURRENT_INI, stock_ini)

        require("22m =" not in merged, "upgrade retained retired 22m key")
        require(
            "22m Active High =" not in merged,
            "upgrade retained retired 22m Active High key",
        )
        require("20m = 23" in merged, "upgrade did not preserve current-schema value")
        require(
            "20m Active High = true" in merged,
            "upgrade did not preserve current-schema polarity",
        )
        require("Call Sign = UPGRADE" in merged, "upgrade did not preserve operator value")
        require(
            merged.count("RP1 Drive mA = 2") == 1,
            "legacy upgrade did not add exactly one canonical RP1 drive default",
        )
        require(
            stock_ini.read_bytes() == CURRENT_INI.read_bytes(),
            "installed stock basis differs from the current canonical INI",
        )
        require(
            stock_ini.read_bytes() != merged_ini.read_bytes(),
            "installed stock incorrectly uses merged active configuration",
        )

        repeated_ini = root / "repeated.ini"
        repeated = upgrade(awk, program, merged_ini, CURRENT_INI, repeated_ini)
        require(repeated == merged, "repeat upgrade changed the merged configuration")

        duplicate_old_ini = root / "duplicate-old.ini"
        duplicate_merged_ini = root / "duplicate-merged.ini"
        duplicate_old_ini.write_text(
            "[GPIO]\n"
            "Transmit Pin = 4\n"
            "\n"
            "[Band GPIO]\n"
            "20m = 23\n"
            "\n"
            "[GPIO]\n"
            "RP1 Drive mA = 8\n",
            encoding="utf-8",
        )
        duplicate_merged = upgrade(
            awk,
            program,
            duplicate_old_ini,
            CURRENT_INI,
            duplicate_merged_ini,
        )
        require(
            duplicate_merged.count("[GPIO]") == 1,
            "upgrade retained a duplicate legacy GPIO section",
        )
        require(
            duplicate_merged.count("RP1 Drive mA = 8") == 1,
            "upgrade did not canonicalize the retained RP1 drive value",
        )

        for name, old_value, expected in (
            ("valid", "8", "RP1 Drive mA = 8"),
            ("invalid", "6", "RP1 Drive mA = 6"),
            ("blank", "", "RP1 Drive mA = "),
        ):
            value_old_ini = root / f"{name}-old.ini"
            value_merged_ini = root / f"{name}-merged.ini"
            value_old_ini.write_text(
                "[GPIO]\n" f"RP1 Drive mA = {old_value}\n",
                encoding="utf-8",
            )
            value_merged = upgrade(
                awk,
                program,
                value_old_ini,
                CURRENT_INI,
                value_merged_ini,
            )
            require(
                expected in value_merged,
                f"upgrade did not preserve {name} RP1 drive value",
            )
            require(
                value_merged.count("RP1 Drive mA =") == 1,
                f"upgrade emitted duplicate RP1 drive keys for {name} value",
            )

    print("INI upgrade schema lifecycle test: PASS")


if __name__ == "__main__":
    main()
