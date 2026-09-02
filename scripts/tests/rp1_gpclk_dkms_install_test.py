#!/usr/bin/env python3
"""Offline, unprivileged contract tests for RP1 provider installation."""

from __future__ import annotations

import copy
import base64
import hashlib
import importlib.util
import io
import json
import lzma
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
        super().__init__()
        self.responses = responses or {}
        self.calls: list[tuple[str, ...]] = []
        self.passthrough_calls: list[tuple[str, ...]] = []

    def run(self, argv, *, check=True, cwd=None, passthrough=False):
        key = tuple(str(item) for item in argv)
        self.calls.append(key)
        if passthrough:
            self.passthrough_calls.append(key)
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

    def test_runtime_source_requires_corrected_commit_ancestry(self):
        source = pathlib.Path("/fixture/source")
        selected = "a" * 40
        ancestor = (
            "git", "-C", str(source), "merge-base", "--is-ancestor",
            MOD.CORRECTED_RUNTIME_COMMIT, selected,
        )
        missing = FakeRunner({
            ("git", "-C", str(source), "cat-file", "-e",
             f"{MOD.CORRECTED_RUNTIME_COMMIT}^{{commit}}"): MOD.CommandResult("", "missing", 1),
        })
        with self.assertRaisesRegex(MOD.ContractError, "cannot prove the corrected"):
            MOD.require_compatible_runtime_source(source, selected, missing)
        predates = FakeRunner({ancestor: MOD.CommandResult("", "", 1)})
        with self.assertRaisesRegex(MOD.ContractError, "predates the corrected"):
            MOD.require_compatible_runtime_source(source, selected, predates)
        compatible = FakeRunner()
        MOD.require_compatible_runtime_source(source, selected, compatible)
        self.assertIn(ancestor, compatible.calls)

    def test_debug_runner_reports_argv_and_captured_output(self):
        completed = subprocess.CompletedProcess(["git", "status"], 0, "clean\n", "notice\n")
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(MOD.subprocess, "run", return_value=completed), \
             mock.patch.object(MOD.sys, "stdout", stdout), \
             mock.patch.object(MOD.sys, "stderr", stderr):
            result = MOD.Runner(debug=True).run(["git", "status"])
        self.assertEqual(result.stdout, "clean\n")
        self.assertEqual(stdout.getvalue(), "clean\n")
        self.assertIn("[RP1 DEBUG] command: git status", stderr.getvalue())
        self.assertIn("notice", stderr.getvalue())

    def test_passthrough_failure_remains_a_contract_error(self):
        completed = subprocess.CompletedProcess(["apt-get", "install"], 1, None, None)
        with mock.patch.object(MOD.subprocess, "run", return_value=completed):
            with self.assertRaisesRegex(MOD.ContractError, "see command output above"):
                MOD.Runner().run(["apt-get", "install"], passthrough=True)


class ReleaseTests(unittest.TestCase):
    def test_release_resolution_binds_exact_runtime_source_checkout(self):
        package = b"fixture package"
        manifest = valid_manifest()
        manifest["package"]["sha256"] = digest(package)
        manifest_bytes = MOD.canonical(manifest) + b"\n"
        package_name = manifest["package"]["filename"]
        checksums = (
            f"{digest(manifest_bytes)}  {MOD.MANIFEST_NAME}\n"
            f"{digest(package)}  {package_name}\n"
        ).encode()
        selected = release("v2.1.0")
        assets = {item["name"]: item["browser_download_url"] for item in selected["assets"]}

        class Client:
            def releases(self): return [selected]
            def tag_commit(self, _tag): return "a" * 40
            def bytes(self, url, **_kwargs):
                return {
                    assets[MOD.MANIFEST_NAME]: manifest_bytes,
                    assets[MOD.CHECKSUMS_NAME]: checksums,
                    assets[package_name]: package,
                }[url]

        with tempfile.TemporaryDirectory() as raw:
            state = pathlib.Path(raw).resolve()
            checkout = {"path": str(state / "source"), "commit": "a" * 40}
            with mock.patch.object(MOD, "validate_package"), \
                 mock.patch.object(MOD, "clone_exact", return_value=checkout) as clone, \
                 mock.patch.object(MOD, "runtime_source_interface") as interface, \
                 mock.patch.object(MOD, "development_identity", return_value={"version": "2.1.0"}), \
                 mock.patch.object(MOD, "runtime_source_identity") as identity:
                resolved = MOD.download_release(state, Client(), FakeRunner())
            clone.assert_called_once_with(state, "commit", "a" * 40, mock.ANY)
            interface.assert_called_once_with(state / "source")
            identity.assert_called_once_with(state / "source", "2.1.0")
            self.assertEqual(resolved["checkout"], checkout)
            self.assertEqual(resolved["runtimeVersion"], "2.1.0")

    def test_semantic_ordering_and_exclusions(self):
        selected = MOD.select_release([
            release("v1.9.9"), release("v1.10.0"), release("v9.0.0", draft=True),
            release("v8.0.0", prerelease=True), release("latest"), release("v7.0.0", complete=False),
        ])
        self.assertEqual(selected["tag_name"], "v1.10.0")

    def test_release_without_required_manifest_is_not_eligible(self):
        with self.assertRaisesRegex(MOD.ContractError, "no eligible"):
            MOD.select_release([release("v2.0.0", complete=False)])

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
        self.assertEqual(validated["uapi"]["sha256"], inventory_for_uapi()[0]["sha256"])
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
        version_source = root / "include/rp1_gpclk/version.h"
        version_source.parent.mkdir(parents=True, exist_ok=True)
        version_source.write_text('#define RP1_GPCLK_MODULE_VERSION "0.9.0"\n')
        installer = scripts / "development-install"
        installer.write_text(f"#!/bin/sh\nprintf '%s\\n' '{help_text}'\n")
        preflight = scripts / "development-preflight"
        identity = {
            "classification": "source-development", "moduleName": MOD.MODULE_NAME,
            "moduleVersion": "0.9.0", "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": digest(version_source.read_bytes()),
        }
        preflight.write_text("#!/bin/sh\nprintf '%s\\n' '" + json.dumps(identity) + "'\n")
        installer.chmod(0o755)
        preflight.chmod(0o755)
        return root

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)

    def test_current_route_required_upstream_is_explicit_blocker(self):
        source = self.lifecycle_source("usage: development-install --route {gpio4,gpio20} --runtime-controller")
        with self.assertRaisesRegex(MOD.ContractError, "no reviewed route-neutral"):
            MOD.route_neutral_interface(source, MOD.Runner())

    def test_future_reviewed_route_neutral_shapes(self):
        source = self.lifecycle_source("usage: development-install --route-neutral --runtime-controller")
        self.assertEqual(MOD.route_neutral_interface(source, MOD.Runner())[1], ["--route-neutral"])
        source = self.lifecycle_source("usage: development-install --route {gpio4,gpio20,route-neutral} --runtime-controller")
        self.assertEqual(MOD.route_neutral_interface(source, MOD.Runner())[1], ["--route", "route-neutral"])

    def test_development_identity_comes_from_upstream_preflight(self):
        source = self.lifecycle_source("usage: development-install --route-neutral --runtime-controller")
        identity = MOD.development_identity(source, MOD.Runner())
        self.assertEqual(identity["version"], "0.9.0")
        self.assertEqual(identity["versionSource"], "include/rp1_gpclk/version.h")
        (source / "include/rp1_gpclk/version.h").write_text('#define RP1_GPCLK_MODULE_VERSION "changed"\n')
        with self.assertRaisesRegex(MOD.ContractError, "differs from the selected source"):
            MOD.development_identity(source, MOD.Runner())

    def test_runtime_source_identity_binds_kernel_product_and_both_routes(self):
        source = pathlib.Path(self.temp.name) / "runtime-source"
        scripts = source / "scripts"
        scripts.mkdir(parents=True)
        (scripts / "runtime_layout.py").write_text("KERNEL = 'fixture-kernel'\n")
        (scripts / "runtime_binding.py").write_text(
            "CONTRACT = 'rp1-gpclk-runtime-binding-v3'\n"
            "PRODUCT_VERSION = '0.9.0'\n"
            "COMPATIBILITY = {'gpio4': 'v0.9.0-pi5-gpio4', 'gpio20': 'v0.9.0-pi5-gpio20'}\n"
        )
        with mock.patch.object(MOD.platform, "release", return_value="fixture-kernel"):
            MOD.runtime_source_identity(source, "0.9.0")
            (scripts / "runtime_layout.py").write_text("KERNEL = 'other-kernel'\n")
            with self.assertRaisesRegex(MOD.ContractError, "different running kernel"):
                MOD.runtime_source_identity(source, "0.9.0")

    def test_development_result_is_bound_before_recording(self):
        manifest_path = pathlib.Path(self.temp.name) / "DEVELOPMENT_MANIFEST.json"
        resolved = {"commit": "a" * 40, "version": "0.9.0", "uapiSha256": "b" * 64,
                    "versionSource": "include/rp1_gpclk/version.h", "versionSourceSha256": "e" * 64}
        manifest = {
            "schema": "rp1-gpclk-source-development-manifest",
            "classification": "source-development", "qualification": False,
            "releaseQualified": False, "sourceCommit": "a" * 40,
            "sourceState": "clean", "renderedVersion": "0.9.0",
            "versionIdentity": {"path": "include/rp1_gpclk/version.h", "sha256": "e" * 64,
                                "moduleVersion": "0.9.0"},
            "dkmsName": MOD.DKMS_NAME, "moduleName": MOD.MODULE_NAME,
            "targetKernel": MOD.platform.release(),
            "buildProfile": "runtime-controller",
            "installationMode": "route-neutral", "route": None,
            "routeNeutralSafety": {
                "before": {"loadedModule": False, "configuredRoutes": []},
                "after": {"loadedModule": False, "configuredRoutes": []},
            },
            "uapiIdentity": {"sha256": "b" * 64},
            "parameters": {"live_output": 0},
            "installedModule": {
                "moduleName": MOD.MODULE_NAME, "moduleVersion": "0.9.0",
                "kernel": MOD.platform.release(), "installedFileSha256": "c" * 64,
                "decompressedElfSha256": "d" * 64,
                "installedPath": f"/lib/modules/{MOD.platform.release()}/updates/dkms/{MOD.MODULE_NAME}.ko",
            },
        }
        manifest["installedModules"] = {
            MOD.MODULE_NAME: manifest["installedModule"],
            MOD.CONTROLLER_MODULE_NAME: {
                "moduleName": MOD.CONTROLLER_MODULE_NAME, "moduleVersion": "0.9.0",
                "kernel": MOD.platform.release(), "installedFileSha256": "f" * 64,
                "decompressedElfSha256": "1" * 64,
                "installedPath": f"/lib/modules/{MOD.platform.release()}/updates/dkms/{MOD.CONTROLLER_MODULE_NAME}.ko",
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
        source = self.lifecycle_source("usage: development-install --route-neutral --runtime-controller")
        resolved = {
            "checkout": {"path": str(source)}, "interface": "route-neutral-flag",
            "routeArguments": ["--route-neutral"], "version": "0.9.0",
        }
        existing = {
            "packageVersion": None, "dkms": "rp1-gpclk-dkms/foreign, kernel: installed",
            "activeModule": False, "configuredRoute": False, "sourceTrees": [],
            "moduleCandidates": [], "installedOverlays": [], "enrollment": False,
            "developmentManager": False,
            "runtimeResidue": [],
        }
        record = pathlib.Path(self.temp.name) / "record.json"
        with mock.patch.object(MOD, "revalidate_checkout", return_value=source), \
             mock.patch.object(MOD, "existing_inventory", return_value=existing):
            with self.assertRaisesRegex(MOD.ContractError, "not adoptable"):
                MOD.apply_development(resolved, record, MOD.Runner())
        self.assertFalse(record.exists())

    def test_exact_owned_development_state_is_an_idempotent_noop(self):
        source = self.lifecycle_source("usage: development-install --route-neutral --runtime-controller")
        resolved = {
            "checkout": {"path": str(source)}, "interface": "route-neutral-flag",
            "routeArguments": ["--route-neutral"], "commit": "a" * 40,
            "version": "0.9.0", "sourceTree": "b" * 40,
            "uapiSha256": "c" * 64,
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": "d" * 64,
        }
        existing = {
            "packageVersion": None,
            "dkms": f"{MOD.DKMS_NAME}/0.9.0, {MOD.platform.release()}, arm64: installed",
            "activeModule": False, "configuredRoute": False,
            "sourceTrees": [f"/usr/src/{MOD.PACKAGE_NAME}-0.9.0"],
            "moduleCandidates": [
                f"/lib/modules/{MOD.platform.release()}/updates/dkms/{MOD.MODULE_NAME}.ko"
            ],
            "installedOverlays": [], "enrollment": False,
            "developmentManager": False,
            "runtimeResidue": ["/var/lib/rp1-gpclk-dkms/runtime-admin"],
        }
        owned = {
            "schema": MOD.RECORD_SCHEMA,
            "channel": "development", "sourceCommit": resolved["commit"],
            "productVersion": resolved["version"], "sourceTree": resolved["sourceTree"],
            "uapiSha256": resolved["uapiSha256"],
            "versionSource": resolved["versionSource"],
            "versionSourceSha256": resolved["versionSourceSha256"],
            "targetKernel": MOD.platform.release(),
            "compatibilityIdentity": MOD.COMPATIBILITY_IDENTITY,
        }
        identity = mock.Mock(st_dev=1, st_ino=2)
        record = pathlib.Path(self.temp.name) / "record.json"
        runner = FakeRunner({
            (str(source / "scripts/development-install"), "--help"):
                MOD.CommandResult("usage: development-install --route-neutral --runtime-controller\n", "", 0),
        })
        stdout = io.StringIO()
        with mock.patch.object(MOD, "revalidate_checkout", return_value=source), \
             mock.patch.object(MOD, "existing_inventory", return_value=existing), \
             mock.patch.object(MOD, "load_ownership_record", side_effect=[
                 (owned, identity, None), (owned, identity, None),
             ]), \
             mock.patch.object(MOD, "validate_development_rollback_authority") as validate, \
             mock.patch.object(MOD.sys, "stdout", stdout):
            MOD.apply_development(resolved, record, runner)
        self.assertEqual(validate.call_args_list, [
            mock.call(owned, pathlib.Path("/")),
            mock.call(owned, pathlib.Path("/")),
        ])
        self.assertEqual(runner.passthrough_calls, [])
        self.assertIn("no provider mutation", stdout.getvalue())

    def test_owned_development_identity_drift_is_not_reused(self):
        source = self.lifecycle_source("usage: development-install --route-neutral --runtime-controller")
        resolved = {
            "checkout": {"path": str(source)}, "interface": "route-neutral-flag",
            "routeArguments": ["--route-neutral"], "commit": "a" * 40,
            "version": "0.9.0", "sourceTree": "b" * 40,
            "uapiSha256": "c" * 64,
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": "d" * 64,
        }
        existing = {
            "packageVersion": None, "dkms": "present", "activeModule": False,
            "configuredRoute": False, "sourceTrees": [], "moduleCandidates": [],
            "installedOverlays": [], "enrollment": False,
            "developmentManager": False, "runtimeResidue": [],
        }
        owned = {
            "channel": "development", "sourceCommit": "e" * 40,
            "productVersion": "0.9.0", "sourceTree": "b" * 40,
            "uapiSha256": "c" * 64,
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": "d" * 64,
            "targetKernel": MOD.platform.release(),
            "compatibilityIdentity": MOD.COMPATIBILITY_IDENTITY,
        }
        runner = FakeRunner({
            (str(source / "scripts/development-install"), "--help"):
                MOD.CommandResult("usage: development-install --route-neutral --runtime-controller\n", "", 0),
        })
        with mock.patch.object(MOD, "revalidate_checkout", return_value=source), \
             mock.patch.object(MOD, "existing_inventory", return_value=existing), \
             mock.patch.object(MOD, "load_ownership_record", return_value=(owned, mock.Mock(st_dev=1, st_ino=2), None)), \
             mock.patch.object(MOD, "validate_development_removal") as validate:
            with self.assertRaisesRegex(MOD.ContractError, "identity differs"):
                MOD.apply_development(resolved, pathlib.Path(self.temp.name) / "record.json", runner)
        validate.assert_not_called()

    def test_runtime_migration_defers_provider_inventory_until_after_runtime_removal(self):
        source = self.lifecycle_source("usage: development-install --route-neutral --runtime-controller")
        resolved = {
            "checkout": {"path": str(source)}, "interface": "route-neutral-flag",
            "routeArguments": ["--route-neutral"], "commit": "a" * 40,
            "version": "0.9.0", "sourceTree": "b" * 40,
            "uapiSha256": "c" * 64,
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": "d" * 64,
        }
        before = {
            "packageVersion": None,
            "dkms": f"{MOD.DKMS_NAME}/0.9.0, kernel, arm64: installed (Differences between built and installed modules)",
            "activeModule": False, "activeController": True,
            "configuredRoute": False,
            "sourceTrees": [f"/usr/src/{MOD.PACKAGE_NAME}-0.9.0"],
            "moduleCandidates": ["/lib/modules/kernel/updates/dkms/rp1_gpclk_dkms.ko",
                                 "/lib/modules/kernel/updates/dkms/rp1_gpclk_dkms.ko.xz"],
            "installedOverlays": [], "enrollment": False,
            "developmentManager": False,
            "runtimeResidue": ["/var/lib/rp1-gpclk-dkms/runtime-admin"],
        }
        absent = {key: value for key, value in before.items()}
        absent.update({"dkms": "", "activeController": False,
                       "sourceTrees": [], "moduleCandidates": [],
                       "runtimeResidue": []})
        owned = {
            "schema": MOD.RUNTIME_RECORD_SCHEMA, "channel": "development",
            "sourceCommit": resolved["commit"], "productVersion": resolved["version"],
            "sourceTree": resolved["sourceTree"], "uapiSha256": resolved["uapiSha256"],
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": resolved["versionSourceSha256"],
            "targetKernel": MOD.platform.release(),
            "compatibilityIdentity": MOD.COMPATIBILITY_IDENTITY,
        }
        identity = mock.Mock(st_dev=1, st_ino=2)
        record = pathlib.Path(self.temp.name) / "record.json"
        rollback = pathlib.Path("/var/lib/wsprrypi/evidence/ROLLBACK.json")
        entrypoint = pathlib.Path("/var/lib/wsprrypi/evidence/rendered-source/scripts/development-rollback")
        runner = FakeRunner({
            (str(source / "scripts/development-install"), "--help"):
                MOD.CommandResult("usage: development-install --route-neutral --runtime-controller\n", "", 0),
        })
        events = []
        def authority(*unused):
            events.append("authority")
            return entrypoint, rollback
        def recover(*unused):
            events.append("runtime-remove")
        def provider(*unused):
            events.append("provider-validate")
            return entrypoint, rollback
        with mock.patch.object(MOD, "revalidate_checkout", return_value=source), \
             mock.patch.object(MOD, "existing_inventory", side_effect=[before, absent]), \
             mock.patch.object(MOD, "load_ownership_record", side_effect=[
                 (owned, identity, None), (owned, identity, None), (None, None, "absent")]), \
             mock.patch.object(MOD, "validate_development_rollback_authority", side_effect=authority), \
             mock.patch.object(MOD, "recover_and_remove_owned_runtime", side_effect=recover), \
             mock.patch.object(MOD, "validate_development_removal", side_effect=provider), \
             mock.patch.object(MOD, "verify_provider_absent"), \
             mock.patch.object(MOD, "remove_ownership_record", side_effect=lambda *unused: events.append("record-remove")), \
             mock.patch.object(MOD, "ensure_new_ownership_record",
                               side_effect=MOD.ContractError("stop after migration")):
            with self.assertRaisesRegex(MOD.ContractError, "stop after migration"):
                MOD.apply_development(resolved, record, runner)
        self.assertEqual(events, ["authority", "runtime-remove", "provider-validate", "record-remove"])
        self.assertEqual(runner.passthrough_calls, [
            (str(entrypoint), "--record", str(rollback)),
        ])

    def test_active_controller_requires_runtime_ownership(self):
        source = self.lifecycle_source("usage: development-install --route-neutral --runtime-controller")
        resolved = {
            "checkout": {"path": str(source)}, "interface": "route-neutral-flag",
            "routeArguments": ["--route-neutral"], "commit": "a" * 40,
            "version": "0.9.0", "sourceTree": "b" * 40,
            "uapiSha256": "c" * 64,
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": "d" * 64,
        }
        before = {
            "packageVersion": None, "dkms": "owned", "activeModule": False,
            "activeController": True, "configuredRoute": False,
            "sourceTrees": ["owned"], "moduleCandidates": ["owned"],
            "controllerCandidates": ["owned"], "installedOverlays": [],
            "enrollment": False, "developmentManager": False,
            "runtimeResidue": ["/var/lib/rp1-gpclk-dkms/runtime-admin"],
        }
        owned_provider = {"schema": MOD.RECORD_SCHEMA, "channel": "development"}
        runner = FakeRunner({
            (str(source / "scripts/development-install"), "--help"):
                MOD.CommandResult("usage: development-install --route-neutral --runtime-controller\n", "", 0),
        })
        with mock.patch.object(MOD, "revalidate_checkout", return_value=source), \
             mock.patch.object(MOD, "existing_inventory", return_value=before), \
             mock.patch.object(MOD, "load_ownership_record", return_value=(
                 owned_provider, mock.Mock(st_dev=1, st_ino=2), None)):
            with self.assertRaisesRegex(
                    MOD.ContractError, "active RP1 route controller"):
                MOD.apply_development(
                    resolved, pathlib.Path(self.temp.name) / "record.json", runner)

    def test_runtime_migration_resumes_after_runtime_removal_checkpoint(self):
        source = pathlib.Path(self.temp.name) / "source"
        resolved = {
            "checkout": {"path": str(source)}, "interface": "route-neutral-flag",
            "routeArguments": ["--route-neutral"], "commit": "a" * 40,
            "version": "0.9.0", "sourceTree": "b" * 40,
            "uapiSha256": "c" * 64,
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": "d" * 64,
        }
        provider = {
            "packageVersion": None, "dkms": "owned", "activeModule": False,
            "configuredRoute": False, "sourceTrees": ["owned"],
            "moduleCandidates": ["owned"], "installedOverlays": [],
            "enrollment": False, "developmentManager": False,
            "runtimeResidue": [],
        }
        absent = dict(provider, dkms="", sourceTrees=[], moduleCandidates=[])
        owned = {
            "schema": MOD.RECORD_SCHEMA, "channel": "development",
            "sourceCommit": "e" * 40, "productVersion": "0.9.0",
            "sourceTree": "f" * 40, "uapiSha256": "1" * 64,
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": "2" * 64,
            "targetKernel": MOD.platform.release(),
            "compatibilityIdentity": MOD.COMPATIBILITY_IDENTITY,
        }
        identity = mock.Mock(st_dev=1, st_ino=2)
        record = pathlib.Path(self.temp.name) / "record.json"
        rollback = pathlib.Path("/var/lib/wsprrypi/evidence/ROLLBACK.json")
        entrypoint = pathlib.Path("/var/lib/wsprrypi/evidence/rendered-source/scripts/development-rollback")
        runner = FakeRunner()
        with mock.patch.object(MOD, "revalidate_checkout", return_value=source), \
             mock.patch.object(MOD, "route_neutral_interface",
                               return_value=("route-neutral-flag", ["--route-neutral"])), \
             mock.patch.object(MOD, "existing_inventory", side_effect=[provider, absent]), \
             mock.patch.object(MOD, "load_ownership_record", side_effect=[
                 (owned, identity, None), (owned, identity, None), (None, None, "absent")]), \
             mock.patch.object(MOD, "validate_development_removal",
                               return_value=(entrypoint, rollback)) as validate, \
             mock.patch.object(MOD, "recover_and_remove_owned_runtime") as recover, \
             mock.patch.object(MOD, "verify_provider_absent"), \
             mock.patch.object(MOD, "remove_ownership_record"), \
             mock.patch.object(MOD, "ensure_new_ownership_record",
                               side_effect=MOD.ContractError("stop after migration")):
            with self.assertRaisesRegex(MOD.ContractError, "stop after migration"):
                MOD.apply_development(resolved, record, runner)
        validate.assert_called_once_with(owned, pathlib.Path("/"), runner)
        recover.assert_not_called()
        self.assertEqual(runner.passthrough_calls, [
            (str(entrypoint), "--record", str(rollback)),
        ])

    def test_runtime_controller_residue_blocks_development_before_mutation(self):
        source = self.lifecycle_source("usage: development-install --route-neutral --runtime-controller")
        resolved = {
            "checkout": {"path": str(source)}, "interface": "route-neutral-flag",
            "routeArguments": ["--route-neutral"], "version": "0.9.0",
        }
        existing = {
            "packageVersion": None, "dkms": "", "activeModule": False,
            "configuredRoute": False, "sourceTrees": [], "moduleCandidates": [],
            "installedOverlays": [], "enrollment": False,
            "developmentManager": False,
            "runtimeResidue": [
                "/etc/systemd/system/wsprrypi.service.d/90-rp1-route-inhibit.conf"
            ],
        }
        record = pathlib.Path(self.temp.name) / "record.json"
        runner = MOD.Runner()
        with mock.patch.object(MOD, "revalidate_checkout", return_value=source), \
             mock.patch.object(MOD, "existing_inventory", return_value=existing):
            with self.assertRaisesRegex(MOD.ContractError, "runtime-controller residue.*owning runtime cleanup"):
                MOD.apply_development(resolved, record, runner)
        self.assertFalse(record.exists())

    def test_inventory_reports_runtime_files_directories_symlinks_and_controller_modules(self):
        root = pathlib.Path(self.temp.name) / "inventory-root"
        (root / "proc").mkdir(parents=True)
        (root / "proc/modules").write_text("")
        inhibitor = root / "etc/systemd/system/wsprrypi.service.d/90-rp1-route-inhibit.conf"
        inhibitor.parent.mkdir(parents=True)
        inhibitor.write_text("owned")
        runtime_library = root / "usr/lib/rp1-gpclk-dkms/runtime_provider.py"
        runtime_library.parent.mkdir(parents=True)
        runtime_library.symlink_to("missing-client")
        runtime_socket = root / "usr/lib/systemd/system/rp1-gpclk-route-manager.socket"
        runtime_socket.parent.mkdir(parents=True)
        runtime_socket.write_text("[Socket]\n")
        controller = root / "lib/modules/fixture/updates/dkms/rp1_route_controller.ko"
        controller.parent.mkdir(parents=True)
        controller.write_bytes(b"module")
        runner = FakeRunner({
            ("dpkg-query", "-W", "-f=${Status}\n${Version}\n", MOD.PACKAGE_NAME):
                MOD.CommandResult("", "not installed", 1),
        })
        inventory = MOD.existing_inventory(root, runner)
        self.assertEqual(
            inventory["runtimeResidue"],
            sorted((str(inhibitor), str(runtime_library), str(runtime_socket))),
        )
        self.assertEqual(inventory["controllerCandidates"], [str(controller)])


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

    def test_activation_failure_is_archived_exactly_before_owned_cleanup(self):
        evidence = self.root / "evidence"
        evidence.mkdir(mode=0o700)
        journal = self.root / "runtime-admin/activation.json"
        journal.parent.mkdir(mode=0o700)
        value = {"phase": "activation-failed", "requestId": "request-1234",
                 "failure": "fixture"}
        raw = json.dumps(value, sort_keys=True).encode() + b"\n"
        journal.write_bytes(raw)
        journal.chmod(0o600)
        record = {"upstreamEvidence": str(evidence)}
        MOD.preserve_owned_activation_journal(
            record, "before-recovery", journal=journal)
        self.assertTrue(journal.is_file())
        archive = evidence / "runtime-activation-archives/before-recovery-request-1234.json"
        archived = json.loads(archive.read_text())
        self.assertEqual(base64.b64decode(archived["contentBase64"]), raw)
        self.assertEqual(archived["sha256"], digest(raw))
        MOD.preserve_owned_activation_journal(
            record, "before-recovery", journal=journal)
        MOD.preserve_owned_activation_journal(
            record, "after-recovery", clear=True, journal=journal)
        self.assertFalse(journal.exists())

    def test_complete_neutral_is_terminal_owned_activation_evidence(self):
        evidence = self.root / "evidence"
        evidence.mkdir(mode=0o700)
        journal = self.root / "runtime-admin/activation.json"
        journal.parent.mkdir(mode=0o700)
        value = {"phase": "complete-neutral", "requestId": "request-neutral"}
        journal.write_text(json.dumps(value, sort_keys=True) + "\n")
        journal.chmod(0o600)
        record = {"upstreamEvidence": str(evidence)}
        MOD.preserve_owned_activation_journal(
            record, "before-recovery", journal=journal)
        archive = evidence / (
            "runtime-activation-archives/before-recovery-request-neutral.json")
        self.assertEqual(json.loads(archive.read_text())["phase"],
                         "complete-neutral")
        journal.write_text(json.dumps({
            "phase": "complete", "requestId": "request-invalid"}) + "\n")
        journal.chmod(0o600)
        with self.assertRaisesRegex(
                MOD.ContractError, "not terminal owned evidence"):
            MOD.preserve_owned_activation_journal(
                record, "invalid", journal=journal)

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
        runner = FakeRunner(self.responses("9.9.9-1"))
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
        self.assertFalse(self.record.exists(), "pre-existing exact providers must not become WsprryPi-owned")

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
        artifacts = [{
            "path": "/lib/modules/fixture/updates/dkms/rp1_gpclk_dkms.ko",
            "sha256": digest(b"module"),
        }]
        with mock.patch.object(MOD, "validate_package"), \
             mock.patch.object(MOD, "verify_installed_release"), \
             mock.patch.object(MOD, "module_artifacts", return_value=artifacts):
            MOD.apply_release(self.state, self.resolved, self.record, self.root, runner)
        apt_calls = [call for call in runner.calls if call and call[0] == "apt-get"]
        self.assertEqual(apt_calls, [("apt-get", "install", "--no-install-recommends", "-y", str(self.package))])
        self.assertEqual(runner.passthrough_calls, apt_calls)
        recorded = json.loads(self.record.read_text())
        self.assertEqual(recorded["schema"], MOD.RECORD_SCHEMA)
        self.assertEqual(recorded["owner"], "WsprryPi")
        self.assertEqual(recorded["moduleArtifacts"][0]["sha256"], digest(b"module"))

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

    def test_prepare_rejects_runtime_residue_before_source_resolution(self):
        model = self.root / "model"
        compatible = self.root / "compatible"
        model.write_bytes(b"Raspberry Pi 5 Model B Rev 1.0\0")
        compatible.write_bytes(b"raspberrypi,5-model-b\0brcm,bcm2712\0")
        args = MOD.parser().parse_args([
            "prepare", "--state-dir", str(self.state), "--install", "auto",
            "--source", "devel", "--wsprry-source", "devel",
            "--model-file", str(model), "--compatible-file", str(compatible),
        ])
        inventory = {
            "runtimeResidue": [
                "/etc/systemd/system/wsprrypi.service.d/90-rp1-route-inhibit.conf"
            ]
        }
        with mock.patch.object(MOD, "runtime_residue_inventory", return_value=inventory["runtimeResidue"]), \
             mock.patch.object(MOD, "existing_inventory") as full_inventory, \
             mock.patch.object(MOD, "prepare_development") as resolve:
            with self.assertRaisesRegex(MOD.ContractError, "residue blocks installation planning"):
                MOD.prepare(args, FakeRunner())
        full_inventory.assert_not_called()
        resolve.assert_not_called()
        self.assertFalse((self.state / "plan.json").exists())

    def test_prepare_allows_runtime_retry_for_owned_provider(self):
        model = self.root / "model"
        compatible = self.root / "compatible"
        model.write_bytes(b"Raspberry Pi 5 Model B Rev 1.0\0")
        compatible.write_bytes(b"raspberrypi,5-model-b\0brcm,bcm2712\0")
        args = MOD.parser().parse_args([
            "prepare", "--state-dir", str(self.state), "--install", "auto",
            "--source", "devel", "--wsprry-source", "devel",
            "--model-file", str(model), "--compatible-file", str(compatible),
        ])
        owned = {"schema": MOD.RECORD_SCHEMA}
        resolved = {"channel": "development", "commit": "a" * 40,
                    "version": "1.2.3", "sourceTree": "b" * 40,
                    "uapiSha256": "c" * 64}
        with mock.patch.object(
                MOD, "runtime_residue_inventory",
                return_value=["/var/lib/rp1-gpclk-dkms/runtime-admin"]), \
             mock.patch.object(MOD, "load_ownership_record",
                               return_value=(owned, mock.sentinel.identity, None)), \
             mock.patch.object(MOD, "prepare_development",
                               return_value=resolved) as resolve:
            plan = MOD.prepare(args, FakeRunner())
        resolve.assert_called_once()
        self.assertEqual(plan["resolved"], resolved)

    def test_runtime_residue_inventory_covers_complete_activation_surface(self):
        source = SCRIPT.read_text()
        for path in (
            "/usr/lib/rp1-gpclk-dkms/runtime_activation.py",
            "/dev/rp1-route-admin", "/dev/rp1-gpclk",
            "/sys/module/rp1_route_controller", "/sys/module/rp1_gpclk_dkms",
            "/run/rp1-gpclk-dkms/route-manager.sock",
            "/etc/systemd/system/wsprrypi.service.d/90-rp1-route-inhibit.conf",
            "/etc/systemd/system/wsprrypi.service.d/91-rp1-route-idle.conf",
            "/var/lib/rp1-gpclk-dkms/runtime-admin",
        ):
            self.assertIn(path, source)

    def test_owned_runtime_recovery_and_removal_reviews_both_digests(self):
        source = self.root / "source"
        provider = source / "scripts/runtime_provider.py"
        provider.parent.mkdir(parents=True)
        provider.write_text("# fixture\n")
        binding_digest = "b" * 64
        recovery_plan = {"bindingSha256": binding_digest}
        activation_digest = digest(MOD.canonical(recovery_plan))
        removal_digest = "d" * 64
        inspected = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "result": "recovery_required", "state": "recovery_required",
            "routeSelected": False,
            "identities": {"installedBinding": {
                "status": "valid", "sha256": binding_digest,
                "value": {"sourceCommit": "c" * 40,
                          "artifactSetSha256": "f" * 64, "files": {}},
            }},
            "artifacts": {"/bound": {"status": "exact"}},
        }
        recovery = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "activation-recover-plan", "planSha256": activation_digest,
            "plan": recovery_plan,
        }
        recovered = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "activation-recover", "planSha256": activation_digest,
            "response": {"status": "recovered-inhibited"},
        }
        removable = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "result": "activation_required", "state": "activation_required",
            "routeSelected": False,
            "modules": {"rp1_route_controller": {"status": "absent"},
                        "rp1_gpclk_dkms": {"status": "absent"}},
            "endpoints": {"controller": {"status": "absent"},
                          "consumer": {"status": "absent"}},
            "managerSocket": {"status": "absent"},
        }
        planned = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "remove-plan", "planSha256": removal_digest,
            "destinations": sorted(MOD.runtime_deployment_destinations()),
        }
        removed = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "remove", "planSha256": removal_digest,
            "response": {"status": "removed-exact-deployment"},
        }
        runner = FakeRunner({
            ("python3", str(provider), "inspect"):
                MOD.CommandResult(json.dumps(inspected), "", 12),
            ("python3", str(provider), "activation-recover-plan"):
                MOD.CommandResult(json.dumps(recovery), "", 0),
            ("python3", str(provider), "activation-recover", "--plan-sha256", activation_digest):
                MOD.CommandResult(json.dumps(recovered), "", 0),
            ("python3", str(provider), "remove-plan"):
                MOD.CommandResult(json.dumps(planned), "", 0),
            ("python3", str(provider), "remove", "--plan-sha256", removal_digest):
                MOD.CommandResult(json.dumps(removed), "", 0),
        })
        inspect_calls = 0
        def runtime_call(runner_arg, provider_arg, operation, arguments=(), allowed_results=()):
            nonlocal inspect_calls
            if operation == "inspect":
                inspect_calls += 1
                return inspected if inspect_calls == 1 else removable
            result = runner_arg.run(["python3", str(provider_arg), operation, *arguments], check=False)
            return json.loads(result.stdout)
        record = {"schema": MOD.RECORD_SCHEMA, "sourceCommit": "c" * 40}
        identity = mock.Mock(st_dev=1, st_ino=2)
        with mock.patch.object(MOD, "RUNTIME_PROVIDER", provider), \
             mock.patch.object(MOD, "runtime_call", side_effect=runtime_call), \
             mock.patch.object(MOD, "load_ownership_record",
                               return_value=(record, identity, None)) as ownership, \
             mock.patch.object(MOD, "runtime_residue_inventory", return_value=[]):
            MOD.recover_and_remove_owned_runtime(
                source, self.record, record, identity, runner
            )
        self.assertEqual(inspect_calls, 2)
        self.assertEqual(ownership.call_count, 2)
        self.assertIn(
            ("python3", str(provider), "activation-recover", "--plan-sha256", activation_digest),
            runner.calls,
        )

    def test_owned_post_reboot_runtime_uses_candidate_to_retire_then_remove(self):
        source = self.root / "source"
        provider = source / "scripts/runtime_provider.py"
        provider.parent.mkdir(parents=True)
        provider.write_text("# authenticated candidate fixture\n")
        binding_digest = "b" * 64
        artifact_digest = "f" * 64
        journal_digest = "a" * 64
        retirement_plan = {
            "version": 1, "operation": "retire-post-reboot-activation",
            "bindingSha256": binding_digest,
            "artifactSetSha256": artifact_digest,
            "bootId": "00000000-0000-0000-0000-000000000002",
            "lastDeploymentSha256": "1" * 64,
            "activationJournalSha256": journal_digest,
        }
        retirement_digest = digest(MOD.canonical(retirement_plan))
        removal_digest = "d" * 64
        initial = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "result": "activation_required", "state": "activation_required",
            "routeSelected": False,
            "identities": {"installedBinding": {
                "status": "valid", "sha256": binding_digest,
                "value": {"sourceCommit": "c" * 40,
                          "artifactSetSha256": artifact_digest, "files": {}},
            }},
            "artifacts": {"/bound": {"status": "exact"}},
            "journals": {"activation.json": {"status": "present",
                "value": {"phase": "complete-neutral"}}},
            "modules": {"rp1_route_controller": {"status": "absent"},
                        "rp1_gpclk_dkms": {"status": "absent"}},
            "endpoints": {"controller": {"status": "absent"},
                          "consumer": {"status": "absent"}},
            "managerSocket": {"status": "absent"},
        }
        removable = dict(initial)
        removable["journals"] = {"activation.json": {"status": "absent"}}
        retirement = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "activation-retire-plan",
            "planSha256": retirement_digest, "plan": retirement_plan,
        }
        retired = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "activation-retire", "planSha256": retirement_digest,
            "response": {"status": "retired-post-reboot-activation",
                         "activationJournalSha256": journal_digest},
        }
        planned = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "remove-plan", "planSha256": removal_digest,
            "destinations": sorted(MOD.runtime_deployment_destinations()),
        }
        removed = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "remove", "planSha256": removal_digest,
            "response": {"status": "removed-exact-deployment"},
        }
        replies = iter((initial, retirement, retired, removable, planned, removed))
        record = {"schema": MOD.RECORD_SCHEMA, "sourceCommit": "c" * 40}
        identity = mock.Mock(st_dev=1, st_ino=2)
        operations = []
        def runtime_call(unused_runner, provider_arg, operation,
                         arguments=(), allowed_results=()):
            self.assertEqual(provider_arg, provider)
            operations.append(operation)
            return next(replies)
        with mock.patch.object(MOD, "runtime_call", side_effect=runtime_call), \
             mock.patch.object(MOD, "preserve_owned_activation_journal",
                               side_effect=lambda *unused: operations.append("preserve")) as preserve, \
             mock.patch.object(MOD, "load_ownership_record",
                               return_value=(record, identity, None)), \
             mock.patch.object(MOD, "runtime_residue_inventory", return_value=[]):
            MOD.recover_and_remove_owned_runtime(
                source, self.record, record, identity, FakeRunner()
            )
        self.assertEqual(operations, ["inspect", "activation-retire-plan",
            "preserve", "activation-retire", "inspect", "remove-plan", "remove"])
        preserve.assert_called_once_with(record, "before-retirement")

    def test_unowned_runtime_never_enters_candidate_migration(self):
        source = self.root / "source"
        source.mkdir()
        resolved = {
            "checkout": {"path": str(source)}, "interface": "route-neutral-flag",
            "routeArguments": ["--route-neutral"], "commit": "a" * 40,
            "version": "0.9.0", "sourceTree": "b" * 40,
            "uapiSha256": "c" * 64,
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": "d" * 64,
        }
        inventory = {
            "packageVersion": None, "dkms": "foreign", "activeModule": False,
            "activeController": False, "configuredRoute": False,
            "sourceTrees": ["/usr/src/rp1-gpclk-dkms-0.9.0"],
            "moduleCandidates": ["/lib/modules/kernel/rp1_gpclk_dkms.ko"],
            "controllerCandidates": ["/lib/modules/kernel/rp1_route_controller.ko"],
            "installedOverlays": [], "enrollment": False,
            "developmentManager": False,
            "runtimeResidue": ["/var/lib/rp1-gpclk-dkms/runtime-admin"],
        }
        with mock.patch.object(MOD, "revalidate_checkout", return_value=source), \
             mock.patch.object(MOD, "route_neutral_interface",
                               return_value=("route-neutral-flag", ["--route-neutral"])), \
             mock.patch.object(MOD, "existing_inventory", return_value=inventory), \
             mock.patch.object(MOD, "load_ownership_record",
                               return_value=(None, None, "no WsprryPi ownership record exists")), \
             mock.patch.object(MOD, "recover_and_remove_owned_runtime") as recover:
            with self.assertRaisesRegex(MOD.ContractError, "runtime-controller residue"):
                MOD.apply_development(resolved, self.record, FakeRunner())
        recover.assert_not_called()

    def test_owned_runtime_removal_rejects_binding_mismatch_before_mutation(self):
        source = self.root / "source"
        provider = source / "scripts/runtime_provider.py"
        provider.parent.mkdir(parents=True)
        provider.write_text("# fixture\n")
        inspected = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "result": "activation_required", "state": "activation_required",
            "identities": {"installedBinding": {"status": "valid", "sha256": "b" * 64,
                "value": {"sourceCommit": "e" * 40, "files": {}}}},
            "artifacts": {"/bound": {"status": "exact"}},
        }
        runner = FakeRunner({
            ("python3", str(provider), "inspect"):
                MOD.CommandResult(json.dumps(inspected), "", 14),
        })
        with mock.patch.object(MOD, "RUNTIME_PROVIDER", provider):
            with self.assertRaisesRegex(MOD.ContractError, "binding differs"):
                MOD.recover_and_remove_owned_runtime(
                    source, self.record,
                    {"schema": MOD.RECORD_SCHEMA, "sourceCommit": "c" * 40},
                    mock.Mock(st_dev=1, st_ino=2), runner,
                )
        self.assertEqual(len(runner.calls), 1)

    def test_ordinary_removal_retains_installed_provider_entrypoint(self):
        provider = self.root / "installed/runtime_provider.py"
        provider.parent.mkdir(parents=True)
        provider.write_text("# installed provider fixture\n")
        inspected = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "result": "activation_required", "state": "activation_required",
            "routeSelected": False,
            "identities": {"installedBinding": {"status": "valid",
                "sha256": "b" * 64, "value": {"sourceCommit": "c" * 40,
                "artifactSetSha256": "f" * 64, "files": {}}}},
            "artifacts": {"/bound": {"status": "exact"}},
            "journals": {"activation.json": {"status": "absent"}},
            "modules": {"rp1_route_controller": {"status": "absent"},
                        "rp1_gpclk_dkms": {"status": "absent"}},
            "endpoints": {"controller": {"status": "absent"},
                          "consumer": {"status": "absent"}},
            "managerSocket": {"status": "absent"},
        }
        planned = {"contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "remove-plan", "planSha256": "d" * 64,
            "destinations": sorted(MOD.runtime_deployment_destinations())}
        removed = {"contract": MOD.RUNTIME_READINESS_CONTRACT,
            "operation": "remove", "planSha256": "d" * 64,
            "response": {"status": "removed-exact-deployment"}}
        replies = iter((inspected, planned, removed))
        called = []
        def runtime_call(unused_runner, provider_arg, operation,
                         arguments=(), allowed_results=()):
            called.append((provider_arg, operation))
            return next(replies)
        record = {"schema": MOD.RECORD_SCHEMA, "sourceCommit": "c" * 40}
        identity = mock.Mock(st_dev=1, st_ino=2)
        with mock.patch.object(MOD, "RUNTIME_PROVIDER", provider), \
             mock.patch.object(MOD, "runtime_call", side_effect=runtime_call), \
             mock.patch.object(MOD, "load_ownership_record",
                               return_value=(record, identity, None)), \
             mock.patch.object(MOD, "runtime_residue_inventory", return_value=[]):
            MOD.recover_and_remove_owned_runtime(
                pathlib.Path("/"), self.record, record, identity, FakeRunner())
        self.assertEqual(called, [(provider, "inspect"),
                                  (provider, "remove-plan"),
                                  (provider, "remove")])

    def test_owned_runtime_removal_rejects_incomplete_absence_evidence(self):
        source = self.root / "source"
        provider = source / "scripts/runtime_provider.py"
        provider.parent.mkdir(parents=True)
        provider.write_text("# fixture\n")
        inspected = {
            "contract": MOD.RUNTIME_READINESS_CONTRACT,
            "result": "activation_required", "state": "activation_required",
            "routeSelected": False,
            "identities": {"installedBinding": {"status": "valid", "sha256": "b" * 64,
                "value": {"sourceCommit": "c" * 40, "files": {}}}},
            "artifacts": {"/bound": {"status": "exact"}},
            "modules": {}, "endpoints": {}, "managerSocket": {"status": "absent"},
        }
        runner = FakeRunner({
            ("python3", str(provider), "inspect"):
                MOD.CommandResult(json.dumps(inspected), "", 14),
        })
        with mock.patch.object(MOD, "RUNTIME_PROVIDER", provider):
            with self.assertRaisesRegex(MOD.ContractError, "not eligible"):
                MOD.recover_and_remove_owned_runtime(
                    source, self.record,
                    {"schema": MOD.RECORD_SCHEMA, "sourceCommit": "c" * 40},
                    mock.Mock(st_dev=1, st_ino=2), runner,
                )
        self.assertEqual(len(runner.calls), 1)

    def test_runtime_bundle_requires_self_contained_bootstrap_and_bound_units(self):
        bundle = self.root / "bundle"
        bundle.mkdir(mode=0o700)
        companion = self.root / "route_application.py"
        companion.write_bytes(b"companion\n")
        kernel = "fixture-kernel"
        bootstrap = {
            "runtime_deployment.py", "runtime_controller_admin.py", "runtime_layout.py",
            "runtime_application.py", "runtime_output.py", "runtime_provider.py",
            "runtime_binding.py", "runtime_activation.py", "runtime_route_client.py",
        }
        files = {
            "/usr/lib/rp1-gpclk-dkms/runtime-uapi/rp1_gpclk.h",
            "/usr/lib/rp1-gpclk-dkms/runtime-uapi/rp1_route_admin.h",
            "/usr/lib/rp1-gpclk-dkms/runtime-overlays/gpio4.dtbo",
            "/usr/lib/rp1-gpclk-dkms/runtime-overlays/gpio20.dtbo",
            "/usr/lib/rp1-gpclk-dkms/schema/rp1-gpclk-runtime-readiness-v1.schema.json",
            "/usr/lib/systemd/system/rp1-gpclk-route-manager.socket",
            "/usr/lib/systemd/system/rp1-gpclk-route-manager@.service",
            "/etc/systemd/system/rp1-gpclk-route-manager@.service.d/95-runtime-controller.conf",
            *{"/usr/lib/rp1-gpclk-dkms/" + name for name in bootstrap},
        }
        file_digests = {}
        for destination in files:
            name = pathlib.Path(destination).name
            payload = (("# fixture: " if name in bootstrap else "payload:") +
                       destination + "\n").encode()
            file_digests[destination] = digest(payload)
            (bundle / (digest(destination.encode()) + ".bin")).write_bytes(payload)
        for name in bootstrap:
            destination = "/usr/lib/rp1-gpclk-dkms/" + name
            (bundle / name).write_bytes(("# fixture: " + destination + "\n").encode())
        binding = {
            "schemaVersion": 3, "contract": MOD.RUNTIME_BINDING_CONTRACT,
            "productVersion": "0.9.0",
            "compatibilityIdentities": {
                "gpio4": "v0.9.0-pi5-gpio4", "gpio20": "v0.9.0-pi5-gpio20"},
            "sourceCommit": "a" * 40, "kernel": kernel, "files": file_digests,
            "externalFiles": {str(companion): digest(companion.read_bytes())},
            "uapiSha256": {
                "consumer": file_digests["/usr/lib/rp1-gpclk-dkms/runtime-uapi/rp1_gpclk.h"],
                "controller": file_digests["/usr/lib/rp1-gpclk-dkms/runtime-uapi/rp1_route_admin.h"]},
            "controllerNoteSha256": "b" * 64, "consumerNoteSha256": "c" * 64,
        }
        modules = {}
        for name, note in ((MOD.CONTROLLER_MODULE_NAME, "b" * 64),
                           (MOD.MODULE_NAME, "c" * 64)):
            compression = "xz" if name == MOD.CONTROLLER_MODULE_NAME else "none"
            suffix = ".ko.xz" if compression == "xz" else ".ko"
            path = f"/lib/modules/{kernel}/updates/dkms/{name}{suffix}"
            actual = MOD.root_path(self.root, path)
            actual.parent.mkdir(parents=True, exist_ok=True)
            payload = b"\x7fELF" + name.encode()
            actual.write_bytes(lzma.compress(payload) if compression == "xz" else payload)
            modules[name] = {
                "name": name, "path": path,
                "installedFileSha256": digest(actual.read_bytes()),
                "decompressedElfSha256": digest(payload),
                "compression": compression, "buildNoteSha256": note,
                "version": "0.9.0", "kernel": kernel,
            }
        binding["modules"] = modules
        binding["artifactSetSha256"] = digest(MOD.canonical(binding))
        (bundle / "binding.json").write_text(json.dumps(binding))
        resolved = {"channel": "development", "commit": "a" * 40, "version": "0.9.0"}
        with mock.patch.object(MOD.platform, "release", return_value=kernel):
            result = MOD.validate_runtime_bundle(bundle, resolved, companion, self.root)
        self.assertEqual(result["artifactSetSha256"], binding["artifactSetSha256"])

        (bundle / "runtime_route_client.py").unlink()
        with mock.patch.object(MOD.platform, "release", return_value=kernel):
            with self.assertRaisesRegex(MOD.ContractError, "unsupported or missing member"):
                MOD.validate_runtime_bundle(bundle, resolved, companion, self.root)

    def test_runtime_bundle_import_closure_rejects_missing_local_module(self):
        bundle = self.root / "bootstrap"
        bundle.mkdir()
        (bundle / "runtime_provider.py").write_text("import runtime_missing\n")
        with self.assertRaisesRegex(MOD.ContractError, "import closure is incomplete"):
            MOD.validate_bootstrap_import_closure(
                bundle, {"runtime_provider.py"})

    def test_neutral_runtime_activation_records_reviewed_identity(self):
        resolved = {"channel": "development", "commit": "a" * 40, "version": "0.9.0"}
        MOD.atomic_json(self.state / "plan.json", {
            "schema": MOD.STATE_SCHEMA, "dryRun": False,
            "decision": {"install": True}, "resolved": resolved,
        })
        owned = {
            "schema": MOD.RECORD_SCHEMA, "channel": "development",
            "sourceCommit": "a" * 40, "productVersion": "0.9.0",
        }
        identity = mock.Mock(st_dev=1, st_ino=2)
        binding = {
            "artifactSetSha256": "b" * 64,
            "compatibilityIdentities": {
                "gpio4": "v0.9.0-pi5-gpio4", "gpio20": "v0.9.0-pi5-gpio20",
            },
            "files": {"/bound/runtime": "c" * 64},
        }
        readiness = lambda state: {
            "contract": MOD.RUNTIME_READINESS_CONTRACT, "result": state, "state": state,
        }
        deployment = readiness("deployment_required")
        deployment["deployment"] = {"plan": {
            "planSha256": "d" * 64,
            "destinations": {path: {"before": None, "after": "c" * 64} for path in (
                "/bound/runtime", "/etc/rp1-gpclk-dkms/runtime-controller.json",
                "/var/lib/rp1-gpclk-dkms/runtime-admin/transaction.json",
                "/var/lib/rp1-gpclk-dkms/runtime-admin/manager.json",
                "/var/lib/rp1-gpclk-dkms/runtime-admin/application.json",
                "/var/lib/rp1-gpclk-dkms/runtime-admin/activation.json",
            )},
        }}
        activation = readiness("activation_required")
        activation["activationPlan"] = {
            "planSha256": "e" * 64, "bindingSha256": "f" * 64,
            "artifactSetSha256": "b" * 64,
        }
        final = readiness("neutral_ready")
        final.update({
            "administrationEligible": True, "transmissionEligible": False,
            "routeSelected": False,
            "routes": {name: None for name in ("requested", "configured", "persisted", "active")},
            "safety": {"liveOutput": False, "owner": False, "lease": False,
                       "authorization": False},
            "identities": {"installedBinding": {"sha256": "f" * 64,
                "value": {"artifactSetSha256": "b" * 64}}},
            "activation": {"value": {"controllerState": {"session": 42, "generation": 0}}},
        })
        replies = [
            readiness("absent"), deployment, {"operation": "ensure"},
            readiness("activation_required"), activation,
            {"contract": MOD.RUNTIME_READINESS_CONTRACT,
             "operation": "activation-ensure", "planSha256": "e" * 64,
             "response": {"journal": {"requestId": "00000000-0000-0000-0000-000000000001"}}},
            final,
        ]
        args = MOD.parser().parse_args([
            "activate-runtime", "--state-dir", str(self.state), "--record", str(self.record),
        ])
        captured = {}
        def replace(path, expected, expected_identity, replacement):
            captured.update(replacement)
        with mock.patch.object(MOD, "load_ownership_record", return_value=(owned, identity, None)), \
             mock.patch.object(MOD, "build_runtime_bundle", return_value=(self.state / "bundle", {
                 "binding": binding, "bindingSha256": "f" * 64,
                 "artifactSetSha256": "b" * 64,
             })), \
             mock.patch.object(MOD, "runtime_call", side_effect=replies) as calls, \
             mock.patch.object(MOD, "replace_owned_record", side_effect=replace):
            MOD.activate_runtime(args, FakeRunner())
        self.assertEqual(calls.call_count, 7)
        self.assertEqual(captured["schema"], MOD.RUNTIME_RECORD_SCHEMA)
        runtime = captured["runtime"]
        self.assertEqual(runtime["deploymentPlanSha256"], "d" * 64)
        self.assertEqual(runtime["activationPlanSha256"], "e" * 64)
        self.assertEqual(runtime["controllerSession"], 42)
        self.assertEqual(runtime["controllerGeneration"], 0)
        self.assertIsNone(runtime["route"])
        self.assertEqual(runtime["output"], "disabled")

    def test_selected_shell_dry_run_never_invokes_python_or_resolution_tools(self):
        install_script = ROOT / "scripts" / "install.sh"
        shell = r'''
source "$INSTALL_SCRIPT"
python3() { printf python3 >"$SENTINEL"; return 99; }
logI() { printf '[INFO ] %s\n' "$1"; }
curl() { printf curl >"$SENTINEL"; return 99; }
mktemp() { printf mktemp >"$SENTINEL"; return 99; }
git() { printf git >"$SENTINEL"; return 99; }
apt-get() { printf apt-get >"$SENTINEL"; return 99; }
dpkg-deb() { printf dpkg-deb >"$SENTINEL"; return 99; }
DRY_RUN=true
ACTION=install
INSTALL_RP1_GPCLK_DKMS=true
RP1_GPCLK_DKMS_SOURCE="$SOURCE_SELECTOR"
REPO_BRANCH="$WSPRRYPi_BRANCH"
FGGLD= RESET= FGGRN= FGRED= MOVE_UP= CLEAR_LINE=
prepare_rp1_gpclk_dkms_installation debug
apply_rp1_gpclk_dkms_installation debug
[[ ! -e "$SENTINEL" ]]
cleanup_rp1_gpclk_dkms_state
[[ -z "$RP1_GPCLK_DKMS_STATE_DIR" && -z "$RP1_GPCLK_DKMS_HELPER" ]]
'''
        for selector, branch in (("release", "main"), ("devel", "codex/issue-412-test")):
            sentinel = self.root / f"{selector}.invoked"
            environment = os.environ.copy()
            environment.update({
                "INSTALL_SCRIPT": str(install_script),
                "SENTINEL": str(sentinel),
                "SOURCE_SELECTOR": selector,
                "WSPRRYPi_BRANCH": branch,
            })
            result = subprocess.run(
                ["bash", "-c", shell], text=True, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, env=environment, check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertFalse(sentinel.exists())
            self.assertEqual(result.stdout.count("Complete: (dry)"), 1)
            self.assertIn(
                "[INFO ] Resolve RP1-GPCLK-DKMS installation plan.",
                result.stdout,
            )
            self.assertNotIn(
                "Complete: (dry) Resolve RP1-GPCLK-DKMS installation plan",
                result.stdout,
            )
            self.assertIn(
                "Complete: (dry) Apply RP1-GPCLK-DKMS installation plan",
                result.stdout,
            )
            self.assertIn("Name:    Resolve RP1-GPCLK-DKMS installation plan", result.stderr)
            self.assertIn("Name:    Apply RP1-GPCLK-DKMS installation plan", result.stderr)
            helper = install_script.parent / "rp1_gpclk_dkms_install.py"
            prepare_command = (
                f"Command: python3 {helper} prepare --state-dir dry-run:no-state-created "
                f"--install true --source {selector} --wsprry-source {branch} --dry-run --debug"
            )
            apply_command = (
                f"Command: python3 {helper} apply --state-dir dry-run:no-state-created --debug"
            )
            self.assertIn(prepare_command, result.stderr)
            self.assertIn(apply_command, result.stderr)

    def test_exec_command_preserves_internal_debug_argv(self):
        install_script = ROOT / "scripts" / "install.sh"
        capture = self.root / "argv.txt"
        shell = r'''
source "$INSTALL_SCRIPT"
probe() { printf '%s\n' "$@" >"$CAPTURE"; }
logD() { :; }
DRY_RUN=false
FGGLD= RESET= FGGRN= FGRED= MOVE_UP= CLEAR_LINE=
exec_command "argv fidelity" probe before debug after debug
'''
        environment = os.environ.copy()
        environment.update({"INSTALL_SCRIPT": str(install_script), "CAPTURE": str(capture)})
        result = subprocess.run(
            ["bash", "-c", shell], text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, env=environment, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(capture.read_text().splitlines(), ["before", "debug", "after"])

    def test_shell_propagates_debug_to_both_helper_invocations(self):
        install_script = ROOT / "scripts" / "install.sh"
        capture = self.root / "helper-argv.txt"
        shell = r'''
source "$INSTALL_SCRIPT"
python3() {
    {
        printf 'CALL\n'
        printf 'ARG=%s\n' "$@"
    } >>"$CAPTURE"
}
logI() { printf '[INFO ] %s\n' "$1"; }
logD() { :; }
DRY_RUN=false
ACTION=install
INSTALL_RP1_GPCLK_DKMS=true
RP1_GPCLK_DKMS_SOURCE=release
REPO_BRANCH=main
FGGLD= RESET= FGGRN= FGRED= MOVE_UP= CLEAR_LINE=
prepare_rp1_gpclk_dkms_installation debug
apply_rp1_gpclk_dkms_installation debug
cleanup_rp1_gpclk_dkms_state
'''
        environment = os.environ.copy()
        environment.update({"INSTALL_SCRIPT": str(install_script), "CAPTURE": str(capture)})
        result = subprocess.run(
            ["bash", "-c", shell], text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, env=environment, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        arguments = capture.read_text().splitlines()
        self.assertEqual(arguments.count("CALL"), 2)
        self.assertEqual(arguments.count("ARG=--debug"), 2)

    def test_selected_uninstall_dry_run_reports_check_without_invoking_python(self):
        install_script = ROOT / "scripts" / "install.sh"
        sentinel = self.root / "uninstall-python-invoked"
        shell = r'''
source "$INSTALL_SCRIPT"
python3() { printf python3 >"$SENTINEL"; return 99; }
curl() { printf curl >"$SENTINEL"; return 99; }
mktemp() { printf mktemp >"$SENTINEL"; return 99; }
DRY_RUN=true
ACTION=uninstall
REPO_ORG=WsprryPi
REPO_NAME=WsprryPi
REPO_BRANCH=devel
FGGLD= RESET= FGGRN= FGRED= MOVE_UP= CLEAR_LINE=
remove_owned_rp1_gpclk_dkms_provider debug
[[ ! -e "$SENTINEL" ]]
cleanup_rp1_gpclk_dkms_state
'''
        environment = os.environ.copy()
        environment.update({"INSTALL_SCRIPT": str(install_script), "SENTINEL": str(sentinel)})
        result = subprocess.run(
            ["bash", "-c", shell], text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, env=environment, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(sentinel.exists())
        self.assertEqual(result.stdout.count("Complete: (dry)"), 1)
        self.assertIn("Complete: (dry) Check owned RP1 provider.", result.stdout)
        self.assertLessEqual(max(map(len, result.stdout.splitlines())), 80)
        helper = install_script.parent / "rp1_gpclk_dkms_install.py"
        self.assertIn(
            f"Command: python3 {helper} remove --remove auto --debug",
            result.stderr,
        )

    def test_uninstall_opt_out_never_prepares_or_invokes_helper(self):
        install_script = ROOT / "scripts" / "install.sh"
        sentinel = self.root / "opt-out-helper-activity"
        shell = r'''
source "$INSTALL_SCRIPT"
python3() { printf python3 >"$SENTINEL"; return 99; }
curl() { printf curl >"$SENTINEL"; return 99; }
mktemp() { printf mktemp >"$SENTINEL"; return 99; }
logI() { :; }
DRY_RUN=false
ACTION=uninstall
REMOVE_RP1_GPCLK_DKMS=false
remove_owned_rp1_gpclk_dkms_provider debug
[[ ! -e "$SENTINEL" ]]
[[ -z "$RP1_GPCLK_DKMS_STATE_DIR" && -z "$RP1_GPCLK_DKMS_HELPER" ]]
'''
        environment = os.environ.copy()
        environment.update({"INSTALL_SCRIPT": str(install_script), "SENTINEL": str(sentinel)})
        result = subprocess.run(
            ["bash", "-c", shell], text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, env=environment, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(sentinel.exists())

    def test_forbidden_runtime_operations_absent_from_installer_paths(self):
        install_source = (ROOT / "scripts/install.sh").read_text()
        helper_source = SCRIPT.read_text()
        self.assertIn('[[ "$ACTION" == "install" ]] || return 0', install_source)
        self.assertNotRegex(helper_source, r'\["(?:dtoverlay|modprobe|insmod|rmmod|systemctl|service|reboot|shutdown)"')
        self.assertNotRegex(helper_source, r"config\.txt[^\n]*(?:write|replace|unlink)")

    def test_prepare_precedes_package_mutation_and_apply_follows_dependencies(self):
        source = (ROOT / "scripts/install.sh").read_text()
        prepare = source.index('prepare_rp1_gpclk_dkms_installation "$debug" || return 1')
        packages = source.index('handle_apt_packages "$debug" || return 1')
        apply = source.index('apply_rp1_gpclk_dkms_installation "$debug" || return 1')
        self.assertLess(prepare, packages)
        self.assertLess(packages, apply)
        group = source[source.index("local install_group=("):]
        companion = group.index('"manage_route_application"')
        service = group.index('"manage_service')
        activation = group.index('"activate_rp1_gpclk_runtime_administration"')
        website = group.index('"manage_web"')
        readiness = group.index('"validate_wsprrypi_runtime"')
        self.assertLess(companion, activation)
        self.assertLess(service, activation)
        self.assertLess(activation, website)
        self.assertLess(website, readiness)

    def test_runtime_readiness_summary_follows_execution_loop(self):
        source = (ROOT / "scripts/install.sh").read_text()
        validator_start = source.index("validate_wsprrypi_runtime() {")
        validator_end = source.index("\n}\n", validator_start)
        validator = source[validator_start:validator_end]
        self.assertNotIn('logI "WsprryPi runtime readiness verified."', validator)

        manager_start = source.index("manage_wsprry_pi() {")
        manager_end = source.index("\n}\n", manager_start)
        manager = source[manager_start:manager_end]
        execution_loop = manager.index('for func in "${group_to_execute[@]}"; do')
        loop_end = manager.index("\n    done", execution_loop)
        summary = manager.index('logI "WsprryPi runtime readiness verified."')
        reboot_notice = manager.index('flag_need_reboot "$debug"')
        self.assertIn('"$DRY_RUN" != "true"', manager[loop_end:summary])
        self.assertLess(loop_end, summary)
        self.assertLess(summary, reboot_notice)

    def test_helper_invocations_use_exec_command_and_propagate_debug(self):
        source = (ROOT / "scripts/install.sh").read_text()
        self.assertNotRegex(source, r'if ! python3 "\$RP1_GPCLK_DKMS_HELPER"')
        self.assertEqual(source.count('exec_command "Resolve RP1-GPCLK-DKMS installation plan"'), 1)
        self.assertEqual(source.count('exec_command "Apply RP1-GPCLK-DKMS installation plan"'), 1)
        self.assertEqual(source.count('exec_command "Activate neutral RP1-GPCLK-DKMS administration"'), 1)
        self.assertIn('EXEC_COMMAND_STATUS_MODE=info', source)
        self.assertGreaterEqual(source.count('args+=(--debug)'), 3)

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

    def test_temporary_cleanup_is_scoped_and_uninstall_uses_owned_provider_step(self):
        source = (ROOT / "scripts/install.sh").read_text()
        cleanup_start = source.index("cleanup_rp1_gpclk_dkms_state() {")
        cleanup = source[cleanup_start:source.index("\n}\n", cleanup_start) + 3]
        self.assertIn("/tmp/wsprrypi-rp1-gpclk-dkms.*", cleanup)
        self.assertIn("Refusing to remove unexpected", cleanup)
        self.assertIn('group_to_execute+=("remove_owned_rp1_gpclk_dkms_provider")', source)

    def run_uninstall_orchestration(self, failing_step=""):
        install_script = ROOT / "scripts" / "install.sh"
        capture = self.root / f"uninstall-{failing_step or 'success'}.txt"
        shell = r'''
source "$INSTALL_SCRIPT"
ACTION=uninstall
DRY_RUN=false
NO_WEB=false

record_step() {
    printf '%s\n' "$1" >>"$CAPTURE"
    [[ "$1" != "$FAILING_STEP" ]]
}
manage_apache() { record_step manage_apache; }
manage_web() { record_step manage_web; }
manage_service() { record_step manage_service; }
manage_i2c() { record_step manage_i2c; }
manage_config() { record_step manage_config; }
manage_route_application() { record_step manage_route_application; }
manage_support_bundle_runtime() { record_step manage_support_bundle_runtime; }
manage_exe() { record_step manage_exe; }
manage_sound() { record_step manage_sound; }
remove_owned_rp1_gpclk_dkms_provider() { record_step remove_owned_rp1_gpclk_dkms_provider; }
flag_need_reboot() { record_step flag_need_reboot; }
eval "$(declare -f finish_script | sed '1s/finish_script/original_finish_script/')"
finish_script() {
    printf 'finish_status=%s\n' "$1" >>"$CAPTURE"
    original_finish_script "$@"
}
logE() { :; }
warn() { printf 'warning=%s\n' "$*" >>"$CAPTURE"; }

status=0
manage_wsprry_pi || status=$?
printf 'return_status=%s\n' "$status" >>"$CAPTURE"
'''
        environment = os.environ.copy()
        environment.update({
            "INSTALL_SCRIPT": str(install_script),
            "CAPTURE": str(capture),
            "FAILING_STEP": failing_step,
        })
        result = subprocess.run(
            ["bash", "-c", shell], text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, env=environment, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        return capture.read_text().splitlines(), result.stdout

    def test_teardown_failures_skip_owned_provider_removal_and_fail_uninstall(self):
        expected_teardown = {
            "manage_apache", "manage_web", "manage_service", "manage_i2c",
            "manage_config", "manage_route_application",
            "manage_support_bundle_runtime", "manage_exe", "manage_sound",
            "flag_need_reboot",
        }
        for failing_step in ("manage_service", "manage_route_application", "manage_exe"):
            with self.subTest(failing_step=failing_step):
                lines, stdout = self.run_uninstall_orchestration(failing_step)
                self.assertTrue(expected_teardown.issubset(lines))
                self.assertNotIn("remove_owned_rp1_gpclk_dkms_provider", lines)
                self.assertIn("finish_status=1", lines)
                self.assertIn("return_status=1", lines)
                self.assertTrue(any("provider and ownership record were preserved" in line for line in lines))
                self.assertNotIn("Uninstallation successful", stdout)
                self.assertNotIn("has been uninstalled", stdout)

    def test_successful_teardown_runs_owned_provider_removal_last(self):
        lines, stdout = self.run_uninstall_orchestration()
        provider = lines.index("remove_owned_rp1_gpclk_dkms_provider")
        self.assertGreater(provider, lines.index("manage_sound"))
        self.assertIn("finish_status=0", lines)
        self.assertIn("return_status=0", lines)
        self.assertIn("Uninstallation successful", stdout)

    def test_owned_provider_removal_failure_fails_uninstall(self):
        lines, stdout = self.run_uninstall_orchestration("remove_owned_rp1_gpclk_dkms_provider")
        self.assertIn("remove_owned_rp1_gpclk_dkms_provider", lines)
        self.assertIn("finish_status=1", lines)
        self.assertIn("return_status=1", lines)
        self.assertNotIn("Uninstallation successful", stdout)
        self.assertNotIn("has been uninstalled", stdout)


class RemovalPolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name).resolve()
        self.record = self.root / "ownership.json"

    def release_record(self) -> dict[str, object]:
        return {
            "schema": MOD.RECORD_SCHEMA, "repository": MOD.REPOSITORY,
            "owner": "WsprryPi", "channel": "release",
            "installationMethod": "debian-package", "tag": "v2.1.0",
            "sourceCommit": "a" * 40, "manifest": valid_manifest(),
            "moduleArtifacts": [{
                "path": "/lib/modules/fixture/updates/dkms/rp1_gpclk_dkms.ko",
                "sha256": "c" * 64,
            }],
            "routeActivation": "disabled", "output": "disabled",
            "qualificationClaim": False,
        }

    def development_record(self) -> dict[str, object]:
        return {
            "schema": MOD.RECORD_SCHEMA, "repository": MOD.REPOSITORY,
            "owner": "WsprryPi", "channel": "development",
            "installationMethod": "upstream-development-rollback",
            "sourceCommit": "a" * 40, "productVersion": "0.9.0",
            "sourceTree": "b" * 40, "uapiSha256": "c" * 64,
            "versionSource": "include/rp1_gpclk/version.h",
            "versionSourceSha256": "d" * 64,
            "targetKernel": MOD.platform.release(),
            "compatibilityIdentity": MOD.COMPATIBILITY_IDENTITY,
            "upstreamEvidence": "/var/lib/wsprrypi/rp1-gpclk-dkms-development-evidence",
            "rollbackRecord": "/var/lib/wsprrypi/rp1-gpclk-dkms-development-evidence/ROLLBACK.json",
            "rollbackRecordSha256": "e" * 64,
            "rollbackEntrypoint": "/var/lib/wsprrypi/rp1-gpclk-dkms-development-evidence/rendered-source/scripts/development-rollback",
            "rollbackEntrypointSha256": "f" * 64,
            "installedModulePath": f"/lib/modules/{MOD.platform.release()}/updates/dkms/rp1_gpclk_dkms.ko",
            "installedModuleSha256": "1" * 64,
            "decompressedModuleSha256": "2" * 64,
            "routeActivation": "disabled", "output": "disabled",
            "qualificationClaim": False,
        }

    def write_record(self, value: dict[str, object]) -> None:
        self.record.write_text(json.dumps(value))
        self.record.chmod(0o600)

    def args(self):
        return MOD.parser().parse_args(["remove", "--record", str(self.record), "--root", str(self.root)])

    def test_missing_legacy_malformed_and_symlink_records_preserve_provider(self):
        runner = FakeRunner()
        stdout = io.StringIO()
        with mock.patch.object(MOD.sys, "stdout", stdout):
            MOD.remove_owned_provider(self.args(), runner)
        self.assertIn("preserved", stdout.getvalue())
        self.assertEqual(runner.calls, [])

        for value in (
            {"schema": MOD.LEGACY_RECORD_SCHEMA},
            {"schema": MOD.RECORD_SCHEMA, "owner": "attacker"},
            {**self.release_record(), "tag": 210},
        ):
            self.write_record(value)
            stdout = io.StringIO()
            with mock.patch.object(MOD.sys, "stdout", stdout):
                MOD.remove_owned_provider(self.args(), runner)
            self.assertIn("preserved", stdout.getvalue())
            self.assertTrue(self.record.exists())
            self.record.unlink()

        target = self.root / "target.json"
        target.write_text(json.dumps(self.release_record()))
        self.record.symlink_to(target)
        stdout = io.StringIO()
        with mock.patch.object(MOD.sys, "stdout", stdout):
            MOD.remove_owned_provider(self.args(), runner)
        self.assertIn("preserved", stdout.getvalue())
        self.assertTrue(target.exists())

    def test_explicit_opt_out_preserves_even_a_valid_owned_provider(self):
        self.write_record(self.release_record())
        args = MOD.parser().parse_args([
            "remove", "--remove", "false", "--record", str(self.record),
            "--root", str(self.root),
        ])
        runner = FakeRunner()
        stdout = io.StringIO()
        with mock.patch.object(MOD.sys, "stdout", stdout):
            MOD.remove_owned_provider(args, runner)
        self.assertIn("explicit operator opt-out", stdout.getvalue())
        self.assertTrue(self.record.exists())
        self.assertEqual(runner.calls, [])

    def test_owned_release_uses_package_removal_then_deletes_record(self):
        self.write_record(self.release_record())
        runner = FakeRunner()
        with mock.patch.object(MOD, "validate_release_removal"), \
             mock.patch.object(MOD, "verify_provider_absent"):
            MOD.remove_owned_provider(self.args(), runner)
        self.assertEqual(runner.passthrough_calls, [("apt-get", "remove", "-y", MOD.PACKAGE_NAME)])
        self.assertFalse(self.record.exists())

    def test_identity_drift_preserves_record_without_package_mutation(self):
        self.write_record(self.release_record())
        runner = FakeRunner()
        stdout = io.StringIO()
        with mock.patch.object(MOD, "validate_release_removal", side_effect=MOD.ContractError("artifact drift")), \
             mock.patch.object(MOD.sys, "stdout", stdout):
            MOD.remove_owned_provider(self.args(), runner)
        self.assertIn("artifact drift", stdout.getvalue())
        self.assertEqual(runner.calls, [])
        self.assertTrue(self.record.exists())

    def test_release_allows_only_same_version_additional_kernel_artifacts(self):
        record = self.release_record()
        artifacts = [
            *record["moduleArtifacts"],
            {
                "path": "/lib/modules/new-kernel/updates/dkms/rp1_gpclk_dkms.ko.xz",
                "sha256": "d" * 64,
            },
        ]
        responses = {
            ("modinfo", "-k", "fixture", "-F", "version", MOD.MODULE_NAME): MOD.CommandResult("2.1.0\n", "", 0),
            ("modinfo", "-k", "new-kernel", "-F", "version", MOD.MODULE_NAME): MOD.CommandResult("2.1.0\n", "", 0),
        }
        runner = FakeRunner(responses)
        with mock.patch.object(MOD, "verify_installed_release"), \
             mock.patch.object(MOD, "module_artifacts", return_value=artifacts):
            MOD.validate_release_removal(record, self.root, runner)
        runner.responses[("modinfo", "-k", "new-kernel", "-F", "version", MOD.MODULE_NAME)] = MOD.CommandResult("9.9.9\n", "", 0)
        with mock.patch.object(MOD, "verify_installed_release"), \
             mock.patch.object(MOD, "module_artifacts", return_value=artifacts):
            with self.assertRaisesRegex(MOD.ContractError, "additional-kernel"):
                MOD.validate_release_removal(record, self.root, runner)

    def test_package_removal_failure_retains_record_and_fails(self):
        self.write_record(self.release_record())
        failed = MOD.CommandResult("", "package busy", 1)
        runner = FakeRunner({("apt-get", "remove", "-y", MOD.PACKAGE_NAME): failed})
        with mock.patch.object(MOD, "validate_release_removal"):
            with self.assertRaises(MOD.ContractError):
                MOD.remove_owned_provider(self.args(), runner)
        self.assertTrue(self.record.exists())

    def test_record_change_after_preflight_blocks_package_mutation(self):
        self.write_record(self.release_record())
        runner = FakeRunner()

        def change_record(*_):
            changed = self.release_record()
            changed["sourceCommit"] = "b" * 40
            self.write_record(changed)

        with mock.patch.object(MOD, "validate_release_removal", side_effect=change_record):
            with self.assertRaisesRegex(MOD.ContractError, "ownership changed"):
                MOD.remove_owned_provider(self.args(), runner)
        self.assertEqual(runner.calls, [])
        self.assertTrue(self.record.exists())

    def test_owned_development_uses_captured_upstream_rollback(self):
        self.write_record(self.development_record())
        evidence = self.root / "evidence/rendered-source/scripts"
        evidence.mkdir(parents=True)
        entrypoint = evidence / "development-rollback"
        entrypoint.write_text("#!/bin/sh\n")
        entrypoint.chmod(0o700)
        rollback = self.root / "evidence/ROLLBACK.json"
        rollback.write_text("{}")
        rollback.chmod(0o600)
        runner = FakeRunner()
        with mock.patch.object(MOD, "validate_development_removal", return_value=(entrypoint, rollback)), \
             mock.patch.object(MOD, "verify_provider_absent"):
            MOD.remove_owned_provider(self.args(), runner)
        self.assertEqual(runner.passthrough_calls, [(str(entrypoint), "--record", str(rollback))])
        self.assertFalse(self.record.exists())

    def test_owned_runtime_uninstall_recovers_before_dkms_rollback(self):
        record = self.development_record()
        record["schema"] = MOD.RUNTIME_RECORD_SCHEMA
        record["runtime"] = {
            "readinessContract": MOD.RUNTIME_READINESS_CONTRACT,
            "bindingSha256": "3" * 64, "artifactSetSha256": "4" * 64,
            "sourceCommit": record["sourceCommit"],
            "productVersion": record["productVersion"],
            "targetKernel": MOD.platform.release(),
            "compatibilityIdentities": {"gpio4": "gpio4-id", "gpio20": "gpio20-id"},
            "deploymentPlanSha256": "5" * 64, "activationPlanSha256": "6" * 64,
            "activationRequestId": "request-1234", "controllerSession": 1,
            "controllerGeneration": 0, "state": "neutral_ready", "route": None,
            "output": "disabled",
        }
        self.write_record(record)
        args = MOD.parser().parse_args(["remove", "--record", str(self.record)])
        entrypoint = self.root / "evidence/rendered-source/scripts/development-rollback"
        entrypoint.parent.mkdir(parents=True)
        entrypoint.write_text("#!/bin/sh\n")
        entrypoint.chmod(0o700)
        rollback = self.root / "evidence/ROLLBACK.json"
        rollback.parent.mkdir(parents=True, exist_ok=True)
        rollback.write_text("{}")
        rollback.chmod(0o600)
        events = []
        runner = FakeRunner()
        with mock.patch.object(MOD, "existing_inventory",
                               return_value={"runtimeResidue": ["owned"]}), \
             mock.patch.object(MOD, "validate_development_rollback_authority",
                               side_effect=lambda *_: events.append("authority")), \
             mock.patch.object(MOD, "recover_and_remove_owned_runtime",
                               side_effect=lambda *_: events.append("runtime-remove")), \
             mock.patch.object(MOD, "validate_development_removal",
                               side_effect=lambda *_: (events.append("provider-validate")
                                                       or (entrypoint, rollback))), \
             mock.patch.object(MOD, "verify_provider_absent"):
            MOD.remove_owned_provider(args, runner)
        self.assertEqual(events, ["authority", "runtime-remove", "provider-validate"])
        self.assertEqual(runner.passthrough_calls,
                         [(str(entrypoint), "--record", str(rollback))])
        self.assertFalse(self.record.exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
