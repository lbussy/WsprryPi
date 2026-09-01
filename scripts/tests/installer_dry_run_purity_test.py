#!/usr/bin/env python3
"""Regression coverage for mutation-free installer dry runs."""

from __future__ import annotations

import hashlib
import os
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
INSTALLER = ROOT / "scripts" / "install.sh"


def snapshot(root: Path) -> dict[str, tuple[str, int, int, str]]:
    result: dict[str, tuple[str, int, int, str]] = {}
    for path in sorted((root, *root.rglob("*"))):
        metadata = path.lstat()
        relative = "." if path == root else str(path.relative_to(root))
        if stat.S_ISREG(metadata.st_mode):
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            kind = "file"
        elif stat.S_ISDIR(metadata.st_mode):
            digest = ""
            kind = "directory"
        elif stat.S_ISLNK(metadata.st_mode):
            digest = os.readlink(path)
            kind = "symlink"
        else:
            digest = ""
            kind = "other"
        result[relative] = (kind, stat.S_IMODE(metadata.st_mode), metadata.st_mtime_ns, digest)
    return result


class InstallerDryRunPurityTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="wsprrypi-dry-run-")
        self.root = Path(self.temporary.name)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def run_shell(self, source: str, **values: Path | str) -> subprocess.CompletedProcess[str]:
        environment = os.environ.copy()
        environment.update({"INSTALLER": str(INSTALLER)})
        environment.update({name: str(value) for name, value in values.items()})
        result = subprocess.run(
            ["bash", "-c", source],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=environment,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        return result

    def test_upgrade_ini_dry_run_creates_nothing(self) -> None:
        old_ini = self.root / "old.ini"
        new_ini = self.root / "new.ini"
        merged_ini = self.root / "merged.ini"
        old_ini.write_text("[WSPR]\nCall Sign = OLD\n", encoding="utf-8")
        new_ini.write_text("[WSPR]\nCall Sign = NEW\n", encoding="utf-8")
        before = snapshot(self.root)

        result = self.run_shell(
            r'''
source "$INSTALLER"
logD() { printf '%s\n' "$*"; }
DRY_RUN=true
upgrade_ini "$OLD_INI" "$NEW_INI" "$MERGED_INI" debug
''',
            OLD_INI=old_ini,
            NEW_INI=new_ini,
            MERGED_INI=merged_ini,
        )

        self.assertEqual(snapshot(self.root), before)
        self.assertIn("Exec: merge", result.stdout)
        self.assertFalse(merged_ini.exists())
        self.assertFalse(Path(f"{old_ini}.pre_migration.bak").exists())

    def test_upgrade_ini_preserves_preexisting_merged_file(self) -> None:
        old_ini = self.root / "old.ini"
        new_ini = self.root / "new.ini"
        merged_ini = self.root / "merged.ini"
        old_ini.write_text("old\n", encoding="utf-8")
        new_ini.write_text("new\n", encoding="utf-8")
        merged_ini.write_text("pre-existing\n", encoding="utf-8")
        merged_ini.chmod(0o640)
        before = snapshot(self.root)

        self.run_shell(
            r'''
source "$INSTALLER"
logD() { :; }
DRY_RUN=true
upgrade_ini "$OLD_INI" "$NEW_INI" "$MERGED_INI" debug
''',
            OLD_INI=old_ini,
            NEW_INI=new_ini,
            MERGED_INI=merged_ini,
        )

        self.assertEqual(snapshot(self.root), before)

    def test_placeholder_replacement_is_display_only(self) -> None:
        target = self.root / "installed.ini"
        target.write_text("version=%SEMANTIC_VERSION%\n", encoding="utf-8")
        target.chmod(0o640)
        before = snapshot(self.root)

        result = self.run_shell(
            r'''
source "$INSTALLER"
logD() { printf '%s\n' "$*"; }
DRY_RUN=true
replace_string_in_script "$TARGET" SEMANTIC_VERSION v9.9.9 debug
''',
            TARGET=target,
        )

        self.assertEqual(snapshot(self.root), before)
        self.assertIn("Exec: sed -i", result.stdout)

    def test_placeholder_replacement_allows_planned_new_target(self) -> None:
        target = self.root / "not-installed-yet.ini"
        before = snapshot(self.root)
        self.run_shell(
            r'''
source "$INSTALLER"
logD() { :; }
DRY_RUN=true
replace_string_in_script "$TARGET" SEMANTIC_VERSION v9.9.9 debug
''',
            TARGET=target,
        )
        self.assertEqual(snapshot(self.root), before)

    def test_placeholder_replacement_still_operates_normally(self) -> None:
        target = self.root / "installed.ini"
        target.write_text("version=%SEMANTIC_VERSION%\n", encoding="utf-8")
        self.run_shell(
            r'''
source "$INSTALLER"
logD() { :; }
sed() {
    if [[ "$1" == -i ]]; then
        shift
        command sed -i.bak "$@"
        rm -f "${!#}.bak"
    else
        command sed "$@"
    fi
}
DRY_RUN=false
replace_string_in_script "$TARGET" SEMANTIC_VERSION v9.9.9
''',
            TARGET=target,
        )
        self.assertEqual(target.read_text(encoding="utf-8"), "version=v9.9.9\n")

    def test_log_initialization_creates_no_dry_run_log(self) -> None:
        requested_log = self.root / "installer.log"
        before = snapshot(self.root)
        result = self.run_shell(
            r'''
source "$INSTALLER"
debug_print() { printf '%s\n' "$1"; }
DRY_RUN=true
LOG_FILE="$REQUESTED_LOG"
init_log debug
[[ "$LOG_FILE" == /dev/null ]]
''',
            REQUESTED_LOG=requested_log,
        )
        self.assertEqual(snapshot(self.root), before)
        self.assertIn("Dry run: file logging disabled.", result.stdout)
        self.assertFalse(requested_log.exists())

    def test_representative_install_and_uninstall_fixture_is_unchanged(self) -> None:
        source_dir = self.root / "config"
        destination_dir = self.root / "installed"
        source_dir.mkdir()
        destination_dir.mkdir()
        (source_dir / "wsprrypi.ini").write_text(
            "version=%SEMANTIC_VERSION%\n", encoding="utf-8"
        )
        active = destination_dir / "wsprrypi.ini"
        stock = destination_dir / "wsprrypi.ini.stock"
        active.write_text("active\n", encoding="utf-8")
        stock.write_text("stock\n", encoding="utf-8")
        before = snapshot(self.root)

        self.run_shell(
            r'''
source "$INSTALLER"
logD() { :; }
get_sem_ver() { printf 'test-version\n'; }
DRY_RUN=true
LOCAL_CONFIG_DIR="$SOURCE_DIR"
ACTION=install
manage_config wsprrypi.ini "$DESTINATION_DIR"
ACTION=uninstall
manage_config wsprrypi.ini "$DESTINATION_DIR"
''',
            SOURCE_DIR=source_dir,
            DESTINATION_DIR=destination_dir,
        )

        self.assertEqual(snapshot(self.root), before)

    def test_compile_dry_run_does_not_create_staging_directory(self) -> None:
        checkout = self.root / "checkout"
        source_dir = checkout / "src"
        source_dir.mkdir(parents=True)
        before = snapshot(self.root)

        self.run_shell(
            r'''
source "$INSTALLER"
logD() { :; }
systemctl() { return 1; }
runuser() { printf invoked >"$SENTINEL"; return 99; }
DRY_RUN=true
FGGLD= RESET= FGGRN= FGRED= MOVE_UP= CLEAR_LINE=
SUDO_USER=tester
LOCAL_REPO_DIR="$CHECKOUT"
LOCAL_SOURCE_DIR="$SOURCE_DIR"
WSPR_BUILD_TYPE=RELEASE
compile_binary wsprrypi debug
[[ ! -e "$SENTINEL" ]]
''',
            CHECKOUT=checkout,
            SOURCE_DIR=source_dir,
            SENTINEL=self.root / "command-invoked",
        )

        self.assertEqual(snapshot(self.root), before)
        self.assertFalse((checkout / "executables").exists())

    def test_utc_timezone_dry_run_neither_prompts_nor_reconfigures(self) -> None:
        sentinel = self.root / "timezone-command-invoked"
        before = snapshot(self.root)
        self.run_shell(
            r'''
source "$INSTALLER"
logI() { :; }
date() {
    if [[ "${1:-}" == +%Z ]]; then printf 'UTC\n'; else printf 'now\n'; fi
}
read() { printf read >"$SENTINEL"; return 99; }
dpkg-reconfigure() { printf dpkg-reconfigure >"$SENTINEL"; return 99; }
DRY_RUN=true
TERSE=true
set_time debug
[[ ! -e "$SENTINEL" ]]
''',
            SENTINEL=sentinel,
        )
        self.assertEqual(snapshot(self.root), before)

    def test_direct_mutation_paths_have_dry_run_guards(self) -> None:
        source = INSTALLER.read_text(encoding="utf-8")

        def function(name: str) -> str:
            start = source.index(f"{name}() {{")
            return source[start : source.index("\n}\n", start) + 3]

        sound = function("manage_sound")
        self.assertLess(sound.index('if [[ "$DRY_RUN" == "true" ]]'), sound.index('echo "$blacklist"'))
        reboot = function("flag_need_reboot")
        self.assertLess(reboot.index('if [[ "$DRY_RUN" == "true" ]]'), reboot.index("read -rp"))
        ini = function("upgrade_ini")
        self.assertLess(ini.index('if [[ "$DRY_RUN" == "true" ]]'), ini.index("rm -f /tmp/upgrade_ini.err"))


if __name__ == "__main__":
    unittest.main()
