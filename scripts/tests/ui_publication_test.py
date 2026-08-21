#!/usr/bin/env python3
"""Hardware-free regression tests for coherent UI artifact publication."""

from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import stat
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


PUBLISHER = load_module("copy_ui", ROOT / "scripts" / "copy_ui.py")
MANIFEST = load_module(
    "generate_ui_manifest",
    ROOT / "WsprryPi-UI" / "scripts" / "generate_ui_manifest.py",
)


class UiPublicationTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.source = self.root / "source"
        self.target = self.root / "live" / "wsprrypi"
        (self.source / "views").mkdir(parents=True)
        (self.source / "cache").mkdir()
        (self.source / "backups").mkdir()
        (self.source / "index.php").write_text("<?php echo 'new';\n", encoding="utf-8")
        (self.source / "site.css").write_text("body { color: black; }\n", encoding="utf-8")
        (self.source / "views" / "operation.php").write_text("operation\n", encoding="utf-8")
        (self.source / "cache" / "runtime.json").write_text("runtime\n", encoding="utf-8")
        (self.source / "backups" / "old.php").write_text("backup\n", encoding="utf-8")

    def tearDown(self):
        self.temporary.cleanup()

    def publish(self, **kwargs):
        return PUBLISHER.publish_ui(
            self.source,
            self.target,
            source_commit="a" * 40,
            application_version="3.2.0-test",
            manifest_module=MANIFEST,
            **kwargs,
        )

    def test_published_tree_exactly_matches_manifest(self):
        status = self.publish()
        manifest_path = self.target / "ui-manifest.json"
        validated = MANIFEST.load_manifest(manifest_path)
        installed = MANIFEST.classify_installed_ui(self.target, manifest_path)

        self.assertEqual(status["installed_state"], "packaged")
        self.assertEqual(installed["installed_state"], "packaged")
        self.assertEqual(
            installed["installed_ui_build_id"],
            validated["packaged_ui_build_id"],
        )
        self.assertEqual(
            validated["files"],
            MANIFEST.collect_file_records(self.target),
        )
        self.assertEqual(
            stat.S_IMODE(manifest_path.stat().st_mode),
            0o444,
            "the installed manifest must not be runtime-writable",
        )

    def test_repository_ui_artifact_converges(self):
        repository_target = self.root / "repository-live" / "wsprrypi"
        status = PUBLISHER.publish_ui(
            ROOT / "WsprryPi-UI" / "data",
            repository_target,
            source_commit="b" * 40,
            application_version="3.2.0-test",
            manifest_module=MANIFEST,
        )
        installed = MANIFEST.classify_installed_ui(
            repository_target, repository_target / "ui-manifest.json"
        )
        self.assertEqual(installed["installed_state"], "packaged")
        self.assertEqual(
            installed["packaged_ui_build_id"], installed["installed_ui_build_id"]
        )
        self.assertEqual(status["installed_ui_build_id"], installed["installed_ui_build_id"])

    def test_runtime_and_backup_paths_do_not_change_identity(self):
        status = self.publish()
        (self.target / "cache" / "new-runtime.json").write_text("changed\n")
        (self.target / "backups" / "new-backup.php").write_text("changed\n")
        installed = MANIFEST.classify_installed_ui(
            self.target, self.target / "ui-manifest.json"
        )
        self.assertEqual(installed["installed_state"], "packaged")
        self.assertEqual(
            installed["installed_ui_build_id"], status["installed_ui_build_id"]
        )

    def test_validation_failure_leaves_live_tree_untouched(self):
        self.target.mkdir(parents=True)
        old_file = self.target / "index.php"
        old_file.write_text("old live UI\n", encoding="utf-8")
        with mock.patch.object(
            MANIFEST, "load_manifest", side_effect=ValueError("invalid staged manifest")
        ):
            with self.assertRaises(PUBLISHER.UiPublicationError):
                self.publish()
        self.assertEqual(old_file.read_text(encoding="utf-8"), "old live UI\n")
        self.assertFalse((self.target / "ui-manifest.json").exists())

    def test_staging_failure_leaves_live_tree_untouched(self):
        self.target.mkdir(parents=True)
        old_file = self.target / "index.php"
        old_file.write_text("old live UI\n", encoding="utf-8")
        os.symlink(self.source / "index.php", self.source / "unsafe-link.php")
        with self.assertRaises(PUBLISHER.UiPublicationError):
            self.publish()
        self.assertEqual(old_file.read_text(encoding="utf-8"), "old live UI\n")

    def test_symlink_publication_target_is_rejected(self):
        actual = self.root / "actual-live"
        actual.mkdir()
        marker = actual / "marker"
        marker.write_text("untouched\n")
        self.target.parent.mkdir(parents=True)
        os.symlink(actual, self.target)
        with self.assertRaisesRegex(PUBLISHER.UiPublicationError, "must not be a symlink"):
            self.publish()
        self.assertEqual(marker.read_text(), "untouched\n")

    def test_runtime_ownership_requires_root(self):
        with mock.patch.object(PUBLISHER.os, "geteuid", return_value=501):
            with self.assertRaisesRegex(PUBLISHER.UiPublicationError, "root privileges"):
                self.publish(owner="www-data", group="www-data")
        self.assertFalse(self.target.exists())

    def test_static_tree_is_runtime_immutable_but_cache_is_runtime_owned(self):
        staged = self.root / "permission-stage"
        (staged / "cache").mkdir(parents=True)
        (staged / "index.php").write_text("static\n")
        (staged / "ui-manifest.json").write_text("{}\n")
        (staged / "cache" / "runtime.json").write_text("runtime\n")
        ownership = {}

        def record_chown(path, uid, gid):
            ownership[Path(path)] = (uid, gid)

        with (
            mock.patch.object(PUBLISHER.os, "geteuid", return_value=0),
            mock.patch.object(PUBLISHER.pwd, "getpwnam", return_value=mock.Mock(pw_uid=33)),
            mock.patch.object(PUBLISHER.grp, "getgrnam", return_value=mock.Mock(gr_gid=44)),
            mock.patch.object(PUBLISHER.os, "chown", side_effect=record_chown),
        ):
            PUBLISHER._set_tree_permissions(staged, "www-data", "www-data")

        self.assertEqual(ownership[staged], (0, 0))
        self.assertEqual(ownership[staged / "index.php"], (0, 0))
        self.assertEqual(ownership[staged / "ui-manifest.json"], (0, 0))
        self.assertEqual(ownership[staged / "cache"], (33, 44))
        self.assertEqual(ownership[staged / "cache" / "runtime.json"], (33, 44))
        self.assertEqual(stat.S_IMODE((staged / "ui-manifest.json").stat().st_mode), 0o444)

    def test_prepublication_failure_leaves_live_tree_untouched(self):
        self.target.mkdir(parents=True)
        old_file = self.target / "index.php"
        old_file.write_text("old live UI\n", encoding="utf-8")

        def fail_before_publish(stage: Path):
            self.assertTrue((stage / "ui-manifest.json").is_file())
            raise RuntimeError("injected prepublication failure")

        with self.assertRaises(PUBLISHER.UiPublicationError):
            self.publish(before_publish=fail_before_publish)
        self.assertEqual(old_file.read_text(encoding="utf-8"), "old live UI\n")

    def test_publish_rename_failure_restores_live_tree(self):
        self.target.mkdir(parents=True)
        old_file = self.target / "index.php"
        old_file.write_text("old live UI\n", encoding="utf-8")
        real_replace = os.replace
        rejected = False

        def fail_new_tree_once(source, destination):
            nonlocal rejected
            source_path = Path(source)
            if source_path.name.startswith(".wsprrypi.stage.") and not rejected:
                rejected = True
                raise OSError("injected publication rename failure")
            return real_replace(source, destination)

        with mock.patch.object(PUBLISHER.os, "replace", side_effect=fail_new_tree_once):
            with self.assertRaises(PUBLISHER.UiPublicationError):
                self.publish()
        self.assertEqual(old_file.read_text(encoding="utf-8"), "old live UI\n")

    def test_postvalidation_change_is_not_published(self):
        self.target.mkdir(parents=True)
        old_file = self.target / "index.php"
        old_file.write_text("old live UI\n", encoding="utf-8")

        def change_staged_file(stage: Path):
            os.chmod(stage / "site.css", 0o644)
            (stage / "site.css").write_text("changed after validation\n")

        with self.assertRaisesRegex(
            PUBLISHER.UiPublicationError, "changed after manifest validation"
        ):
            self.publish(before_publish=change_staged_file)
        self.assertEqual(old_file.read_text(encoding="utf-8"), "old live UI\n")


if __name__ == "__main__":
    unittest.main()
