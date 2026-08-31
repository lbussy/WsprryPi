#!/usr/bin/env python3
"""Offline, unprivileged contract tests for RP1 provider installation."""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import io
import json
import os
import pathlib
import stat
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest import mock


ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "rp1_gpclk_dkms_install.py"
SPEC = importlib.util.spec_from_file_location("rp1_installer", SCRIPT)
assert SPEC and SPEC.loader
MOD = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MOD
SPEC.loader.exec_module(MOD)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def inventory_for_uapi(content: bytes = b"uapi\n") -> list[dict[str, object]]:
    return [{
        "path": "usr/src/rp1-gpclk-dkms-2.1.0/include/uapi/linux/rp1_gpclk.h",
        "type": "file", "mode": "0644", "sha256": digest(content),
    }]


def valid_manifest(version: str = "2.1.0", commit: str = "a" * 40) -> dict[str, object]:
    inventory = inventory_for_uapi()
    return {
        "schema": MOD.MANIFEST_SCHEMA,
        "schemaVersion": 1,
        "repository": MOD.REPOSITORY,
        "releaseTag": f"v{version}",
        "sourceCommit": commit,
        "productVersion": version,
        "debianVersion": f"{version}-1",
        "package": {
            "name": MOD.PACKAGE_NAME,
            "filename": f"rp1-gpclk-dkms_{version}-1_all.deb",
            "sha256": "b" * 64,
        },
        "dkmsModule": MOD.DKMS_NAME,
        "kernelModule": MOD.MODULE_NAME,
        "uapi": {
            "abiMin": 4,
            "abiMax": 4,
            "sha256": inventory[0]["sha256"],
            "path": inventory[0]["path"],
        },
        "administrationProtocol": MOD.ADMIN_PROTOCOL,
        "installationBehavior": {
            "routeNeutral": True,
            "outputDisabled": True,
            "loadsModule": False,
            "appliesOverlay": False,
            "editsBootConfiguration": False,
            "operatesServices": False,
        },
        "packageInventory": inventory,
        "packageInventorySha256": digest(MOD.canonical(inventory)),
        "releaseChannel": "release",
    }


def release(tag: str, *, draft: bool = False, prerelease: bool = False, complete: bool = True) -> dict[str, object]:
    names = []
    if complete:
        names = [MOD.MANIFEST_NAME, MOD.CHECKSUMS_NAME, f"rp1-gpclk-dkms_{tag.removeprefix('v')}-1_all.deb"]
    return {
        "tag_name": tag, "draft": draft, "prerelease": prerelease, "immutable": True,
        "published_at": "2026-08-31T00:00:00Z",
        "assets": [{"name": name, "state": "uploaded", "browser_download_url": f"https://github.com/{MOD.REPOSITORY}/releases/download/{tag}/{name}"} for name in names],
    }


class FakeRunner(MOD.Runner):
    def __init__(self, responses: dict[tuple[str, ...], MOD.CommandResult] | None = None):
        self.responses = responses or {}
        self.calls: list[tuple[str, ...]] = []

    def run(self, argv, *, check=True, cwd=None):
        key = tuple(str(item) for item in argv)
        self.calls.append(key)
        result = self.responses.get(key, MOD.CommandResult("", "", 0))
        if check and result.returncode:
            raise MOD.ContractError(result.stderr or "fake command failure")
        return result


class PlatformAndSelectionTests(unittest.TestCase):
    def detected(self, model: bytes, compatible: bytes):
        with tempfile.TemporaryDirectory() as raw:
            root = pathlib.Path(raw)
            (root / "model").write_bytes(model)
            (root / "compatible").write_bytes(compatible)
            return MOD.detect_platform(root / "model", root / "compatible")

    def test_auto_pi5_and_cm5(self):
        for model, compatible in (
            (b"Raspberry Pi 5 Model B Rev 1.0\0", b"raspberrypi,5-model-b\0brcm,bcm2712\0"),
            (b"Raspberry Pi Compute Module 5 Rev 1.0\0", b"raspberrypi,5-compute-module\0brcm,bcm2712d0\0"),
        ):
            detected = self.detected(model, compatible)
            self.assertTrue(detected["automaticEligible"])
            self.assertTrue(MOD.installation_decision("auto", detected)["install"])

    def test_auto_skips_non_pi5_and_unknown(self):
        for model, compatible in (
            (b"Raspberry Pi 4 Model B\0", b"raspberrypi,4-model-b\0brcm,bcm2711\0"),
            (b"", b""),
            (b"Raspberry Pi 5 Model B Rev 1.0\0", b"raspberrypi,5-model-b\0"),
        ):
            decision = MOD.installation_decision("auto", self.detected(model, compatible))
            self.assertFalse(decision["install"])
            self.assertFalse(decision["platformOverride"])

    def test_explicit_true_and_false(self):
        unknown = {"automaticEligible": False}
        enabled = MOD.installation_decision("true", unknown)
        self.assertTrue(enabled["install"])
        self.assertTrue(enabled["platformOverride"])
        self.assertFalse(MOD.installation_decision("false", {"automaticEligible": True})["install"])
        with self.assertRaises(MOD.ContractError):
            MOD.installation_decision("yes", unknown)

    def test_channel_and_source_selection(self):
        for source in ("main", "master", "v3.2.0"):
            self.assertEqual(MOD.wsprry_channel(source), "production")
            self.assertEqual(MOD.source_selector("auto", MOD.wsprry_channel(source))[0], "release")
        for source in ("devel", "codex/feature", "issue-412"):
            self.assertEqual(MOD.wsprry_channel(source), "development")
            self.assertEqual(MOD.source_selector("auto", MOD.wsprry_channel(source))[0], "devel")
        for ambiguous in ("", "HEAD", "detached", "unknown"):
            with self.assertRaises(MOD.ContractError):
                MOD.wsprry_channel(ambiguous)
        self.assertEqual(MOD.source_selector("release", "development"), ("release", None))
        self.assertEqual(MOD.source_selector("commit:" + "c" * 40, "development"), ("commit", "c" * 40))
        with self.assertRaises(MOD.ContractError):
            MOD.source_selector("commit:abc123", "development")
        with self.assertRaises(MOD.ContractError):
            MOD.source_selector("checkout:relative", "development")
        with self.assertRaises(MOD.ContractError):
            MOD.source_selector("commit:" + "a" * 40 + ";modprobe", "development")


class ReleaseTests(unittest.TestCase):
    def test_semantic_ordering_and_exclusions(self):
        selected = MOD.select_release([
            release("v1.9.9"), release("v1.10.0"), release("v9.0.0", draft=True),
            release("v8.0.0", prerelease=True), release("latest"), release("v7.0.0", complete=False),
        ])
        self.assertEqual(selected["tag_name"], "v1.10.0")

    def test_historical_release_without_new_manifest_is_not_eligible(self):
        with self.assertRaisesRegex(MOD.ContractError, "no eligible"):
            MOD.select_release([release("v1.0.0", complete=False)])

    def test_selected_highest_corruption_does_not_fallback(self):
        selected = MOD.select_release([release("v2.0.0"), release("v1.9.0")])
        self.assertEqual(selected["tag_name"], "v2.0.0")
        broken = valid_manifest("2.0.0", "d" * 40)
        broken["repository"] = "attacker/repository"
        with self.assertRaisesRegex(MOD.ContractError, "repository substitution"):
            MOD.validate_manifest(broken, tag="v2.0.0", tag_commit="d" * 40)

    def test_manifest_contract(self):
        manifest = valid_manifest()
        validated = MOD.validate_manifest(manifest, tag="v2.1.0", tag_commit="a" * 40)
        self.assertEqual(validated["uapi"]["abiMin"], 4)
        mutations = [
            ("schema", "bad"), ("repository", "other/repo"), ("sourceCommit", "c" * 40),
            ("releaseChannel", "development"), ("administrationProtocol", "shell-v0"),
            ("dkmsModule", "other"),
        ]
        for field, value in mutations:
            candidate = copy.deepcopy(manifest)
            candidate[field] = value
            with self.assertRaises(MOD.ContractError, msg=field):
                MOD.validate_manifest(candidate, tag="v2.1.0", tag_commit="a" * 40)

        candidate = copy.deepcopy(manifest)
        candidate["installationBehavior"]["loadsModule"] = True
        with self.assertRaisesRegex(MOD.ContractError, "inactive installation behavior"):
            MOD.validate_manifest(candidate, tag="v2.1.0", tag_commit="a" * 40)

    def test_missing_manifest_uapi_and_inventory_hash_refused(self):
        for mutation in ("missing", "unexpected", "uapi", "inventory"):
            candidate = copy.deepcopy(valid_manifest())
            if mutation == "missing":
                del candidate["package"]
            elif mutation == "unexpected":
                candidate["alternateRepository"] = "attacker/repository"
            elif mutation == "uapi":
                candidate["uapi"]["sha256"] = "e" * 64
            else:
                candidate["packageInventorySha256"] = "f" * 64
            with self.assertRaises(MOD.ContractError, msg=mutation):
                MOD.validate_manifest(candidate, tag="v2.1.0", tag_commit="a" * 40)

    def test_checksum_parser_and_authoritative_urls(self):
        parsed = MOD.parse_checksums(("a" * 64 + "  file.deb\n").encode())
        self.assertEqual(parsed["file.deb"], "a" * 64)
        for unsafe in (
            "A" * 64 + "  file.deb\n",
            "a" * 64 + " *file.deb\n",
            "a" * 64 + "  ../file.deb\n",
            "a" * 64 + "  file.deb\n" + "b" * 64 + "  file.deb\n",
        ):
            with self.assertRaises(MOD.ContractError):
                MOD.parse_checksums(unsafe.encode())
        good = f"https://github.com/{MOD.REPOSITORY}/releases/download/v2.1.0/file.deb"
        MOD.authoritative_asset_url("v2.1.0", "file.deb", good)
        for bad in (good.replace("github.com", "evil.example"), good + "?token=1", good.replace("v2.1.0", "v2.0.0")):
            with self.assertRaises(MOD.ContractError):
                MOD.authoritative_asset_url("v2.1.0", "file.deb", bad)

    def test_asset_download_does_not_forward_api_token(self):
        class Response:
            headers = {"Content-Length": "2"}
            def __enter__(self): return self
            def __exit__(self, *_): return False
            def geturl(self): return "https://release-assets.githubusercontent.com/asset"
            def read(self, _): return b"ok"

        observed = {}
        def fake_open(request, timeout):
            observed["authorization"] = request.get_header("Authorization")
            observed["timeout"] = timeout
            return Response()

        with mock.patch.object(MOD.urllib.request, "urlopen", side_effect=fake_open):
            value = MOD.GitHubClient("secret-token").bytes(
                "https://github.com/WsprryPi/RP1-GPCLK-DKMS/releases/download/v2.1.0/file.deb",
                allow_asset_redirect=True,
            )
        self.assertEqual(value, b"ok")
        self.assertIsNone(observed["authorization"])


class ArchiveTests(unittest.TestCase):
    def make_tar(self, members):
        temporary = tempfile.NamedTemporaryFile(suffix=".tar", delete=False)
        temporary.close()
        path = pathlib.Path(temporary.name)
        with tarfile.open(path, "w") as archive:
            for name, kind, content in members:
                info = tarfile.TarInfo(name)
                info.mode = 0o644
                if kind == "file":
                    info.size = len(content)
                    archive.addfile(info, io.BytesIO(content))
                elif kind == "symlink":
                    info.type = tarfile.SYMTYPE
                    info.linkname = content.decode()
                    archive.addfile(info)
                elif kind == "hardlink":
                    info.type = tarfile.LNKTYPE
                    info.linkname = content.decode()
                    archive.addfile(info)
        self.addCleanup(path.unlink, missing_ok=True)
        return path

    def test_safe_inventory(self):
        path = self.make_tar([("./usr/include/uapi.h", "file", b"header")])
        inventory = MOD.tar_inventory(path)
        self.assertEqual(inventory[0]["path"], "usr/include/uapi.h")
        self.assertEqual(inventory[0]["sha256"], digest(b"header"))

    def test_member_size_and_inventory_count_are_bounded(self):
        path = self.make_tar([("usr/large", "file", b"xx")])
        with mock.patch.object(MOD, "MAX_PACKAGE_MEMBER_BYTES", 1):
            with self.assertRaisesRegex(MOD.ContractError, "bounded size"):
                MOD.tar_inventory(path)
        with mock.patch.object(MOD, "MAX_PACKAGE_MEMBERS", 0):
            with self.assertRaisesRegex(MOD.ContractError, "bounded member"):
                MOD.validate_inventory(inventory_for_uapi())

    def test_path_traversal_symlink_escape_and_special_refused(self):
        for members in (
            [("../escape", "file", b"x")],
            [("usr/link", "symlink", b"../../escape")],
            [("usr/hard", "hardlink", b"usr/target")],
        ):
            with self.assertRaises(MOD.ContractError):
                MOD.tar_inventory(self.make_tar(members))


class CheckoutTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = pathlib.Path(self.temp.name).resolve() / "repo"
        self.repo.mkdir()
        subprocess.run(["git", "init", "-q", str(self.repo)], check=True)
        subprocess.run(["git", "-C", str(self.repo), "config", "user.email", "test@example.invalid"], check=True)
        subprocess.run(["git", "-C", str(self.repo), "config", "user.name", "Test"], check=True)
        (self.repo / "tracked").write_text("clean\n")
        subprocess.run(["git", "-C", str(self.repo), "add", "tracked"], check=True)
        subprocess.run(["git", "-C", str(self.repo), "commit", "-q", "-m", "fixture"], check=True)
        subprocess.run(["git", "-C", str(self.repo), "remote", "add", "origin", MOD.REPOSITORY_URL], check=True)

    def test_clean_checkout_acceptance(self):
        identity = MOD.validate_checkout(str(self.repo), MOD.Runner())
        self.assertRegex(identity["commit"], r"^[0-9a-f]{40}$")
        self.assertEqual(MOD.revalidate_checkout(identity, MOD.Runner()), self.repo)

    def test_dirty_untracked_relative_symlink_and_unrelated_refused(self):
        (self.repo / "untracked").write_text("x")
        with self.assertRaisesRegex(MOD.ContractError, "tracked or untracked"):
            MOD.validate_checkout(str(self.repo), MOD.Runner())
        (self.repo / "untracked").unlink()
        with self.assertRaises(MOD.ContractError):
            MOD.validate_checkout("relative", MOD.Runner())
        link = pathlib.Path(self.temp.name) / "link"
        link.symlink_to(self.repo, target_is_directory=True)
        with self.assertRaisesRegex(MOD.ContractError, "symlink"):
            MOD.validate_checkout(str(link), MOD.Runner())
        subprocess.run(["git", "-C", str(self.repo), "remote", "set-url", "origin", "https://example.invalid/other.git"], check=True)
        with self.assertRaisesRegex(MOD.ContractError, "authoritative"):
            MOD.validate_checkout(str(self.repo), MOD.Runner())

    def test_checkout_toctou_change_refused(self):
        identity = MOD.validate_checkout(str(self.repo), MOD.Runner())
        (self.repo / "tracked").write_text("replaced\n")
        with self.assertRaisesRegex(MOD.ContractError, "dirty or replaced"):
            MOD.revalidate_checkout(identity, MOD.Runner())

    def test_devel_and_exact_commit_resolve_once_to_detached_sha(self):
        bare = pathlib.Path(self.temp.name).resolve() / "authoritative.git"
        subprocess.run(["git", "clone", "-q", "--bare", str(self.repo), str(bare)], check=True)
        subprocess.run(["git", "-C", str(self.repo), "push", "-q", str(bare), "HEAD:refs/heads/devel"], check=True)
        commit = subprocess.run(
            ["git", "-C", str(self.repo), "rev-parse", "HEAD"],
            check=True, text=True, stdout=subprocess.PIPE,
        ).stdout.strip()
        with mock.patch.object(MOD, "REPOSITORY_URL", str(bare)):
            devel_state = pathlib.Path(self.temp.name).resolve() / "devel-state"
            devel_state.mkdir()
            devel = MOD.clone_exact(devel_state, "devel", None, MOD.Runner())
            self.assertEqual(devel["commit"], commit)
            head_name = subprocess.run(
                ["git", "-C", devel["path"], "symbolic-ref", "-q", "HEAD"],
                text=True, stdout=subprocess.PIPE,
            )
            self.assertNotEqual(head_name.returncode, 0)
            commit_state = pathlib.Path(self.temp.name).resolve() / "commit-state"
            commit_state.mkdir()
            exact = MOD.clone_exact(commit_state, "commit", commit, MOD.Runner())
            self.assertEqual(exact["commit"], commit)
            missing_state = pathlib.Path(self.temp.name).resolve() / "missing-state"
            missing_state.mkdir()
            with self.assertRaisesRegex(MOD.ContractError, "does not exist"):
                MOD.clone_exact(missing_state, "commit", "f" * 40, MOD.Runner())


class DevelopmentInterfaceTests(unittest.TestCase):
    def lifecycle_source(self, help_text: str) -> pathlib.Path:
        root = pathlib.Path(self.temp.name)
        scripts = root / "scripts"
        scripts.mkdir(exist_ok=True)
        installer = scripts / "development-install"
        installer.write_text(f"#!/bin/sh\nprintf '%s\\n' '{help_text}'\n")
        preflight = scripts / "development-preflight"
        preflight.write_text("#!/bin/sh\nexit 0\n")
        installer.chmod(0o755)
        preflight.chmod(0o755)
        return root

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)

    def test_current_route_required_upstream_is_explicit_blocker(self):
        source = self.lifecycle_source("usage: development-install --route {gpio4,gpio20}")
        with self.assertRaisesRegex(MOD.ContractError, "no reviewed route-neutral"):
            MOD.route_neutral_interface(source, MOD.Runner())

    def test_future_reviewed_route_neutral_shapes(self):
        source = self.lifecycle_source("usage: development-install --route-neutral")
        self.assertEqual(MOD.route_neutral_interface(source, MOD.Runner())[1], ["--route-neutral"])
        source = self.lifecycle_source("usage: development-install --route {gpio4,gpio20,route-neutral}")
        self.assertEqual(MOD.route_neutral_interface(source, MOD.Runner())[1], ["--route", "route-neutral"])

    def test_development_result_is_bound_before_recording(self):
        manifest_path = pathlib.Path(self.temp.name) / "DEVELOPMENT_MANIFEST.json"
        resolved = {"commit": "a" * 40, "version": "0.9.0", "uapiSha256": "b" * 64}
        manifest = {
            "schema": "rp1-gpclk-source-development-manifest-v1",
            "classification": "source-development", "qualification": False,
            "releaseQualified": False, "sourceCommit": "a" * 40,
            "sourceState": "clean", "renderedVersion": "0.9.0",
            "dkmsName": MOD.DKMS_NAME, "moduleName": MOD.MODULE_NAME,
            "targetKernel": MOD.platform.release(),
            "uapiIdentity": {"sha256": "b" * 64},
            "parameters": {"live_output": 0},
            "installedModule": {
                "moduleName": MOD.MODULE_NAME, "moduleVersion": "0.9.0",
                "kernel": MOD.platform.release(), "installedFileSha256": "c" * 64,
                "decompressedElfSha256": "d" * 64,
            },
        }
        manifest_path.write_text(json.dumps(manifest))
        self.assertEqual(MOD.verify_development_result(manifest_path, resolved)["sourceState"], "clean")
        for field, value in (("route", "gpio4"), ("sourceState", "dirty-explicitly-allowed")):
            candidate = copy.deepcopy(manifest)
            candidate[field] = value
            manifest_path.write_text(json.dumps(candidate))
            with self.assertRaises(MOD.ContractError):
                MOD.verify_development_result(manifest_path, resolved)

    def test_existing_foreign_state_blocks_development_before_mutation(self):
        source = self.lifecycle_source("usage: development-install --route-neutral")
        resolved = {
            "checkout": {"path": str(source)}, "interface": "route-neutral-flag",
            "routeArguments": ["--route-neutral"], "version": "0.9.0",
        }
        existing = {
            "packageVersion": None, "dkms": "rp1-gpclk-dkms/foreign, kernel: installed",
            "activeModule": False, "configuredRoute": False, "sourceTrees": [],
            "moduleCandidates": [], "installedOverlays": [], "enrollment": False,
            "developmentManager": False,
        }
        record = pathlib.Path(self.temp.name) / "record.json"
        with mock.patch.object(MOD, "revalidate_checkout", return_value=source), \
             mock.patch.object(MOD, "existing_inventory", return_value=existing):
            with self.assertRaisesRegex(MOD.ContractError, "foreign.*mixed"):
                MOD.apply_development(resolved, record, MOD.Runner())
        self.assertFalse(record.exists())


class ApplyPolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name).resolve()
        (self.root / "proc").mkdir()
        (self.root / "proc/modules").write_text("")
        self.state = self.root / "state"
        self.state.mkdir(mode=0o700)
        self.package = self.state / "rp1-gpclk-dkms_2.1.0-1_all.deb"
        self.package.write_bytes(b"fixture")
        self.manifest = valid_manifest()
        self.resolved = {"tag": "v2.1.0", "commit": "a" * 40, "manifest": self.manifest, "packagePath": str(self.package)}
        self.record = self.root / "record.json"

    def responses(self, installed_version=None, dkms=""):
        dpkg = MOD.CommandResult("", "missing", 1)
        if installed_version:
            dpkg = MOD.CommandResult(f"install ok installed\n{installed_version}\n", "", 0)
        return {
            ("dpkg-query", "-W", "-f=${Status}\n${Version}\n", MOD.PACKAGE_NAME): dpkg,
            ("dkms", "status", "-m", MOD.DKMS_NAME): MOD.CommandResult(dkms, "", 0),
        }

    def test_foreign_and_mixed_state_refused_before_apt(self):
        source = self.root / "usr/src/rp1-gpclk-dkms-foreign"
        source.mkdir(parents=True)
        runner = FakeRunner(self.responses())
        with mock.patch.object(MOD, "validate_package"):
            with self.assertRaisesRegex(MOD.ContractError, "foreign or mixed"):
                MOD.apply_release(self.state, self.resolved, self.record, self.root, runner)
        self.assertFalse(any(call and call[0] == "apt-get" for call in runner.calls))

    def test_different_installed_version_requires_owner_migration(self):
        runner = FakeRunner(self.responses("1.0.1-1"))
        with mock.patch.object(MOD, "validate_package"):
            with self.assertRaisesRegex(MOD.ContractError, "owning package migration"):
                MOD.apply_release(self.state, self.resolved, self.record, self.root, runner)
        self.assertFalse(any(call and call[0] == "apt-get" for call in runner.calls))

    def test_active_module_and_development_manager_refused(self):
        (self.root / "proc/modules").write_text(f"{MOD.MODULE_NAME} 1 0 - Live 0x0\n")
        runner = FakeRunner(self.responses())
        with mock.patch.object(MOD, "validate_package"):
            with self.assertRaisesRegex(MOD.ContractError, "active"):
                MOD.apply_release(self.state, self.resolved, self.record, self.root, runner)
        self.assertFalse(any(call and call[0] == "apt-get" for call in runner.calls))

    def test_exact_installation_is_idempotent(self):
        member = self.manifest["packageInventory"][0]
        installed = self.root / member["path"]
        installed.parent.mkdir(parents=True)
        installed.write_bytes(b"uapi\n")
        responses = self.responses(
            "2.1.0-1",
            "rp1-gpclk-dkms/2.1.0, fixture-kernel, arm64: installed\n",
        )
        responses[("modinfo", "-k", MOD.platform.release(), "-F", "version", MOD.MODULE_NAME)] = MOD.CommandResult("2.1.0\n", "", 0)
        runner = FakeRunner(responses)
        with mock.patch.object(MOD, "validate_package"):
            MOD.apply_release(self.state, self.resolved, self.record, self.root, runner)
        self.assertFalse(any(call and call[0] == "apt-get" for call in runner.calls))
        recorded = json.loads(self.record.read_text())
        self.assertEqual(recorded["sourceCommit"], "a" * 40)
        self.assertFalse(recorded["qualificationClaim"])

    def test_exact_installation_rejects_foreign_same_version_state(self):
        member = self.manifest["packageInventory"][0]
        installed = self.root / member["path"]
        installed.parent.mkdir(parents=True)
        installed.write_bytes(b"uapi\n")
        (self.root / "usr/src/rp1-gpclk-dkms-foreign").mkdir()
        runner = FakeRunner(self.responses(
            "2.1.0-1",
            "rp1-gpclk-dkms/2.1.0, fixture-kernel, arm64: installed\n",
        ))
        with mock.patch.object(MOD, "validate_package"):
            with self.assertRaisesRegex(MOD.ContractError, "source destinations"):
                MOD.apply_release(self.state, self.resolved, self.record, self.root, runner)

    def test_fresh_release_uses_normal_debian_path(self):
        runner = FakeRunner(self.responses())
        with mock.patch.object(MOD, "validate_package"), mock.patch.object(MOD, "verify_installed_release"):
            MOD.apply_release(self.state, self.resolved, self.record, self.root, runner)
        apt_calls = [call for call in runner.calls if call and call[0] == "apt-get"]
        self.assertEqual(apt_calls, [("apt-get", "install", "--no-install-recommends", "-y", str(self.package))])

    def test_dry_run_plan_cannot_be_applied(self):
        model = self.root / "model"
        compatible = self.root / "compatible"
        model.write_bytes(b"Unknown\0")
        compatible.write_bytes(b"")
        args = MOD.parser().parse_args([
            "prepare", "--state-dir", str(self.state), "--install", "false",
            "--source", "auto", "--wsprry-source", "main",
            "--model-file", str(model), "--compatible-file", str(compatible), "--dry-run",
        ])
        plan = MOD.prepare(args, FakeRunner())
        self.assertTrue(plan["dryRun"])
        apply_args = MOD.parser().parse_args([
            "apply", "--state-dir", str(self.state), "--record", str(self.record), "--root", str(self.root),
        ])
        with self.assertRaisesRegex(MOD.ContractError, "dry-run"):
            MOD.apply(apply_args, FakeRunner())

    def test_uninstall_and_forbidden_operations_absent_from_installer_paths(self):
        install_source = (ROOT / "scripts/install.sh").read_text()
        helper_source = SCRIPT.read_text()
        self.assertIn('[[ "$ACTION" == "install" ]] || return 0', install_source)
        self.assertNotRegex(helper_source, r'\["(?:dtoverlay|modprobe|insmod|rmmod|systemctl|service|reboot|shutdown)"')
        self.assertNotIn("apt-get remove", helper_source)
        self.assertNotRegex(helper_source, r"config\.txt[^\n]*(?:write|replace|unlink)")

    def test_prepare_precedes_package_mutation_and_apply_follows_dependencies(self):
        source = (ROOT / "scripts/install.sh").read_text()
        prepare = source.index('prepare_rp1_gpclk_dkms_installation "$debug" || return 1')
        packages = source.index('handle_apt_packages "$debug" || return 1')
        apply = source.index('apply_rp1_gpclk_dkms_installation "$debug" || return 1')
        self.assertLess(prepare, packages)
        self.assertLess(packages, apply)

    def test_explicit_true_reaches_planner_without_raspberry_pi_identity(self):
        source = (ROOT / "scripts/install.sh").read_text()

        def function_body(name):
            start = source.index(f"{name}() {{")
            return source[start:source.index("\n}\n", start) + 3]

        for name in ("validate_sys_accs", "print_model_name", "check_arch"):
            body = function_body(name)
            self.assertIn('INSTALL_RP1_GPCLK_DKMS" == "true"', body)
            self.assertIn("explicitly requested", body)
        self.assertIn('"$file" == "/proc/device-tree/compatible"', function_body("validate_sys_accs"))

    def test_temporary_cleanup_is_scoped_and_uninstall_preserves_provider(self):
        source = (ROOT / "scripts/install.sh").read_text()
        cleanup_start = source.index("cleanup_rp1_gpclk_dkms_state() {")
        cleanup = source[cleanup_start:source.index("\n}\n", cleanup_start) + 3]
        self.assertIn("/tmp/wsprrypi-rp1-gpclk-dkms.*", cleanup)
        self.assertIn("Refusing to remove unexpected", cleanup)
        self.assertIn('[[ "$ACTION" == "install" ]] || return 0', source)


if __name__ == "__main__":
    unittest.main(verbosity=2)
