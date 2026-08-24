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


def main() -> None:
    awk = shutil.which("mawk") or shutil.which("awk")
    require(awk is not None, "no awk implementation is available")

    current = CURRENT_INI.read_text(encoding="utf-8")
    require("\n22m =" not in current, "current INI still contains 22m")
    require(
        "\n22m Active High =" not in current,
        "current INI still contains 22m Active High",
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

        with merged_ini.open("w", encoding="utf-8") as output:
            subprocess.run(
                [awk, program, str(old_ini), str(CURRENT_INI)],
                check=True,
                stdout=output,
                text=True,
            )

        shutil.copyfile(CURRENT_INI, stock_ini)
        merged = merged_ini.read_text(encoding="utf-8")

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
            stock_ini.read_bytes() == CURRENT_INI.read_bytes(),
            "installed stock basis differs from the current canonical INI",
        )
        require(
            stock_ini.read_bytes() != merged_ini.read_bytes(),
            "installed stock incorrectly uses merged active configuration",
        )

    print("INI upgrade schema lifecycle test: PASS")


if __name__ == "__main__":
    main()
