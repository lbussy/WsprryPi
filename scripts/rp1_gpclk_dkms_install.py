#!/usr/bin/env python3
"""Resolve and install the independently owned RP1-GPCLK-DKMS provider.

This helper deliberately owns only WsprryPi's orchestration and acceptance
policy.  Package and exact-source lifecycle mechanics remain upstream.
"""

from __future__ import annotations

import argparse
import ast
import base64
import bz2
import copy
import gzip
import hashlib
import json
import lzma
import os
import pathlib
import platform
import re
import secrets
import shlex
import stat
import subprocess
import sys
import tarfile
import tempfile
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from typing import Any, Iterable, Mapping, Sequence


REPOSITORY = "WsprryPi/RP1-GPCLK-DKMS"
REPOSITORY_URL = "https://github.com/WsprryPi/RP1-GPCLK-DKMS.git"
API_BASE = f"https://api.github.com/repos/{REPOSITORY}"
MANIFEST_NAME = "rp1-gpclk-dkms-installation-manifest-v1.json"
CHECKSUMS_NAME = "SHA256SUMS"
MANIFEST_SCHEMA = "rp1-gpclk-dkms-installation-manifest-v1"
ADMIN_PROTOCOL = "rp1-gpclk-route-manager"
PACKAGE_NAME = "rp1-gpclk-dkms"
DKMS_NAME = "rp1-gpclk-dkms"
MODULE_NAME = "rp1_gpclk_dkms"
CONTROLLER_MODULE_NAME = "rp1_route_controller"
COMPATIBILITY_IDENTITY = "v0.9.0-pi5"
CORRECTED_RUNTIME_COMMIT = "8a8bf5d4184714949faffbf2e2538a1cac0526b2"
LEGACY_RECORD_SCHEMA = "wsprrypi-rp1-gpclk-dkms-installation-v1"
RECORD_SCHEMA = "wsprrypi-rp1-gpclk-dkms-ownership-v2"
RUNTIME_RECORD_SCHEMA = "wsprrypi-rp1-gpclk-dkms-ownership-v3"
STATE_SCHEMA = "wsprrypi-rp1-gpclk-dkms-plan-v1"
RUNTIME_READINESS_CONTRACT = "rp1-gpclk-runtime-readiness-v1"
RUNTIME_BINDING_CONTRACT = "rp1-gpclk-runtime-binding-v3"
RUNTIME_PROVIDER = pathlib.Path("/usr/lib/rp1-gpclk-dkms/runtime_provider.py")
RUNTIME_COMPANION = pathlib.Path("/usr/local/lib/wsprrypi/route_application.py")
SHA40 = re.compile(r"^[0-9a-f]{40}$")
SHA256 = re.compile(r"^[0-9a-f]{64}$")
SEMVER = re.compile(r"^v?(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")
SAFE_VERSION = re.compile(r"^[0-9A-Za-z.+:~-]+$")
MAX_API_BYTES = 16 * 1024 * 1024
MAX_METADATA_BYTES = 2 * 1024 * 1024
MAX_PACKAGE_BYTES = 256 * 1024 * 1024
MAX_PACKAGE_MEMBERS = 20000
MAX_PACKAGE_MEMBER_BYTES = 64 * 1024 * 1024
MAX_EXPANDED_TAR_BYTES = 512 * 1024 * 1024
MAX_ARCHIVE_PATH_BYTES = 1024
MAX_RECORD_BYTES = 4 * 1024 * 1024


class ContractError(RuntimeError):
    """A fail-closed contract refusal."""


def canonical(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def decompressed_module_sha256(path: pathlib.Path, compression: str) -> str:
    descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
    try:
        require(stat.S_ISREG(os.fstat(descriptor).st_mode),
                "runtime module prerequisite is not a regular file")
        with os.fdopen(descriptor, "rb", closefd=False) as stream:
            raw = stream.read(64 * 1024 * 1024 + 1)
    finally:
        os.close(descriptor)
    require(len(raw) <= 64 * 1024 * 1024,
            "runtime module prerequisite exceeds read bound")
    try:
        if compression == "none":
            payload = raw
        elif compression == "xz":
            payload = lzma.decompress(raw)
        elif compression == "gz":
            payload = gzip.decompress(raw)
        elif compression == "bz2":
            payload = bz2.decompress(raw)
        elif compression == "zst":
            result = subprocess.run(
                ["/usr/bin/zstd", "-q", "-d", "-c"], input=raw,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
            )
            require(result.returncode == 0, "runtime zstd module decompression failed")
            payload = result.stdout
        else:
            raise ContractError("runtime module compression is unsupported")
    except (OSError, EOFError, ValueError, lzma.LZMAError) as error:
        raise ContractError("runtime module decompression failed") from error
    require(len(payload) <= 64 * 1024 * 1024,
            "decompressed runtime module exceeds read bound")
    return sha256_bytes(payload)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def require_exact_keys(value: Mapping[str, Any], required: Iterable[str], where: str) -> None:
    required_set = set(required)
    missing = sorted(required_set - set(value))
    require(not missing, f"{where} is missing required fields: {', '.join(missing)}")
    unexpected = sorted(set(value) - required_set)
    require(not unexpected, f"{where} contains unsupported fields: {', '.join(unexpected)}")


def json_result(result: CommandResult, operation: str) -> dict[str, Any]:
    try:
        value = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        raise ContractError(f"RP1 runtime {operation} returned malformed JSON") from error
    require(isinstance(value, dict), f"RP1 runtime {operation} did not return an object")
    return value


def parse_semver(tag: str) -> tuple[int, int, int]:
    match = SEMVER.fullmatch(tag)
    if not match:
        raise ContractError(f"release tag is not an immutable semantic version: {tag!r}")
    return tuple(int(match.group(index)) for index in range(1, 4))


def normalize_repository_url(url: str) -> str:
    if url in {
        REPOSITORY_URL,
        "https://github.com/WsprryPi/RP1-GPCLK-DKMS",
        "git@github.com:WsprryPi/RP1-GPCLK-DKMS.git",
        "ssh://git@github.com/WsprryPi/RP1-GPCLK-DKMS.git",
    }:
        return REPOSITORY_URL
    return url


@dataclass(frozen=True)
class CommandResult:
    stdout: str
    stderr: str
    returncode: int


class Runner:
    def __init__(self, debug: bool = False):
        self.debug = debug

    def trace(self, argv: Sequence[str], cwd: pathlib.Path | None = None) -> None:
        if self.debug:
            location = f" (cwd={cwd})" if cwd is not None else ""
            print(f"[RP1 DEBUG] command{location}: {shlex.join(str(item) for item in argv)}", file=sys.stderr)

    def run(
        self,
        argv: Sequence[str],
        *,
        check: bool = True,
        cwd: pathlib.Path | None = None,
        passthrough: bool = False,
    ) -> CommandResult:
        self.trace(argv, cwd)
        try:
            if passthrough:
                process = subprocess.run(list(argv), cwd=cwd, text=True, check=False)
            else:
                process = subprocess.run(
                    list(argv), cwd=cwd, text=True, stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE, check=False,
                )
        except FileNotFoundError as error:
            result = CommandResult("", f"required command is unavailable: {argv[0]}", 127)
            if check:
                raise ContractError(result.stderr) from error
            return result
        result = CommandResult(process.stdout or "", process.stderr or "", process.returncode)
        if self.debug and not passthrough:
            if result.stdout:
                print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
            if result.stderr:
                print(result.stderr, end="" if result.stderr.endswith("\n") else "\n", file=sys.stderr)
        if check and process.returncode:
            if passthrough:
                detail = "see command output above"
            else:
                detail = result.stderr.strip() or result.stdout.strip() or "no diagnostic"
            raise ContractError(f"command failed ({argv[0]}): {detail}")
        return result


def read_device_tree_text(path: pathlib.Path) -> list[str]:
    try:
        return [item.decode("utf-8", "strict") for item in path.read_bytes().split(b"\0") if item]
    except (OSError, UnicodeError):
        return []


def detect_platform(model_file: pathlib.Path, compatible_file: pathlib.Path) -> dict[str, Any]:
    models = read_device_tree_text(model_file)
    compatibles = read_device_tree_text(compatible_file)
    model = models[0] if models else "unknown"
    pi5_model = bool(
        re.fullmatch(r"Raspberry Pi 5(?: Model B)?(?: Rev .+)?", model)
        or re.fullmatch(r"Raspberry Pi Compute Module 5(?: Rev .+)?", model)
    )
    pi5_compatible = any(
        item in {"raspberrypi,5-model-b", "raspberrypi,5-compute-module"}
        for item in compatibles
    )
    bcm2712 = any(item in {"brcm,bcm2712", "brcm,bcm2712d0"} for item in compatibles)
    positive = pi5_model and pi5_compatible and bcm2712
    return {
        "model": model,
        "compatible": compatibles,
        "classification": "raspberry-pi-5-family" if positive else "other-or-unknown",
        "automaticEligible": positive,
    }


def installation_decision(requested: str, detected: Mapping[str, Any]) -> dict[str, Any]:
    require(requested in {"auto", "true", "false"}, "INSTALL_RP1_GPCLK_DKMS must be auto, true, or false")
    eligible = detected.get("automaticEligible") is True
    if requested == "false":
        return {"install": False, "reason": "explicit operator opt-out", "platformOverride": False}
    if requested == "true":
        return {
            "install": True,
            "reason": "explicit operator installation request",
            "platformOverride": not eligible,
        }
    return {
        "install": eligible,
        "reason": "positively identified Raspberry Pi 5-family system" if eligible else "automatic selection skipped other or unknown system",
        "platformOverride": False,
    }


def wsprry_channel(source: str) -> str:
    require(bool(source) and source not in {"HEAD", "detached", "unknown"}, "WsprryPi source channel is ambiguous; set RP1_GPCLK_DKMS_SOURCE explicitly")
    if source in {"main", "master", "release"} or SEMVER.fullmatch(source):
        return "production"
    require(re.fullmatch(r"[A-Za-z0-9._/-]+", source) is not None, "WsprryPi source channel contains unsupported characters")
    return "development"


def source_selector(requested: str, channel: str) -> tuple[str, str | None]:
    if requested == "auto":
        return ("release", None) if channel == "production" else ("devel", None)
    if requested in {"release", "devel"}:
        return requested, None
    if requested.startswith("commit:"):
        commit = requested.removeprefix("commit:")
        require(SHA40.fullmatch(commit) is not None, "commit source requires a full lowercase 40-character SHA")
        return "commit", commit
    if requested.startswith("checkout:"):
        path = requested.removeprefix("checkout:")
        require(pathlib.Path(path).is_absolute(), "checkout source requires an absolute path")
        return "checkout", path
    raise ContractError("RP1_GPCLK_DKMS_SOURCE must be auto, release, devel, commit:FULL_SHA, or checkout:/absolute/path")


def release_assets(release: Mapping[str, Any]) -> dict[str, str]:
    assets: dict[str, str] = {}
    for asset in release.get("assets", []):
        if not isinstance(asset, dict):
            continue
        name, url = asset.get("name"), asset.get("browser_download_url")
        if isinstance(name, str) and isinstance(url, str) and asset.get("state") == "uploaded":
            require(name not in assets, f"release contains duplicate asset name: {name}")
            assets[name] = url
    return assets


def release_candidate(release: Mapping[str, Any]) -> bool:
    if release.get("draft") is not False or release.get("prerelease") is not False:
        return False
    if release.get("immutable") is not True:
        return False
    if not isinstance(release.get("published_at"), str) or not release.get("published_at"):
        return False
    tag = release.get("tag_name")
    if not isinstance(tag, str) or SEMVER.fullmatch(tag) is None:
        return False
    assets = release_assets(release)
    return MANIFEST_NAME in assets and CHECKSUMS_NAME in assets and any(
        re.fullmatch(r"rp1-gpclk-dkms_[0-9A-Za-z.+:~-]+_all\.deb", name) for name in assets
    )


def select_release(releases: Sequence[Mapping[str, Any]]) -> Mapping[str, Any]:
    candidates = [release for release in releases if release_candidate(release)]
    require(bool(candidates), "no eligible RP1-GPCLK-DKMS release is available")
    return max(candidates, key=lambda item: parse_semver(str(item["tag_name"])))


def authoritative_asset_url(tag: str, filename: str, url: str) -> None:
    parsed = urllib.parse.urlsplit(url)
    require(parsed.scheme == "https" and parsed.netloc == "github.com", f"release asset URL is not authoritative HTTPS: {url}")
    expected = f"/{REPOSITORY}/releases/download/{urllib.parse.quote(tag, safe='')}/{urllib.parse.quote(filename, safe='._+-~') }"
    require(parsed.path == expected and not parsed.query and not parsed.fragment, f"release asset URL does not match repository, tag, and filename: {url}")


def parse_checksums(data: bytes) -> dict[str, str]:
    try:
        text = data.decode("ascii")
    except UnicodeError as error:
        raise ContractError("SHA256SUMS is not ASCII") from error
    result: dict[str, str] = {}
    for line in text.splitlines():
        if not line:
            continue
        match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9][A-Za-z0-9._+:~-]*)", line)
        require(match is not None, "SHA256SUMS contains a malformed or unsafe line")
        digest, filename = match.groups()
        require(filename not in result, f"SHA256SUMS repeats {filename}")
        result[filename] = digest
    return result


def validate_manifest(manifest: Any, *, tag: str, tag_commit: str) -> dict[str, Any]:
    require(isinstance(manifest, dict), "installation manifest must be a JSON object")
    required = {
        "schema", "schemaVersion", "repository", "releaseTag", "sourceCommit",
        "productVersion", "debianVersion", "package", "dkmsModule", "kernelModule",
        "uapi", "administrationProtocol", "packageInventory", "packageInventorySha256",
        "installationBehavior", "releaseChannel",
    }
    require_exact_keys(manifest, required, "installation manifest")
    require(manifest["schema"] == MANIFEST_SCHEMA and manifest["schemaVersion"] == 1, "unsupported installation manifest schema")
    require(manifest["repository"] == REPOSITORY, "installation manifest repository substitution")
    require(manifest["releaseTag"] == tag, "manifest release tag differs from selected release")
    require(manifest["sourceCommit"] == tag_commit and SHA40.fullmatch(tag_commit) is not None, "manifest source commit differs from immutable tag commit")
    version = tag.removeprefix("v")
    require(manifest["productVersion"] == version, "manifest product version differs from release tag")
    require(isinstance(manifest["debianVersion"], str) and SAFE_VERSION.fullmatch(manifest["debianVersion"]), "manifest Debian version is invalid")
    require(manifest["dkmsModule"] == DKMS_NAME and manifest["kernelModule"] == MODULE_NAME, "manifest module identity is incompatible")
    require(manifest["administrationProtocol"] == ADMIN_PROTOCOL, "manifest administration protocol is incompatible")
    require(manifest["releaseChannel"] == "release", "manifest is not classified for the release channel")
    behavior = manifest["installationBehavior"]
    require(isinstance(behavior, dict), "manifest installation behavior must be an object")
    require_exact_keys(
        behavior,
        {"routeNeutral", "outputDisabled", "loadsModule", "appliesOverlay", "editsBootConfiguration", "operatesServices"},
        "manifest installation behavior",
    )
    require(
        behavior == {
            "routeNeutral": True,
            "outputDisabled": True,
            "loadsModule": False,
            "appliesOverlay": False,
            "editsBootConfiguration": False,
            "operatesServices": False,
        },
        "manifest does not declare the required inactive installation behavior",
    )
    package = manifest["package"]
    require(isinstance(package, dict), "manifest package must be an object")
    require_exact_keys(package, {"name", "filename", "sha256"}, "manifest package")
    require(package["name"] == PACKAGE_NAME, "manifest package name is incompatible")
    expected_filename = f"{PACKAGE_NAME}_{manifest['debianVersion']}_all.deb"
    require(package["filename"] == expected_filename, "manifest package filename and Debian version differ")
    require(isinstance(package["sha256"], str) and SHA256.fullmatch(package["sha256"]), "manifest package SHA-256 is invalid")
    uapi = manifest["uapi"]
    require(isinstance(uapi, dict), "manifest UAPI must be an object")
    require_exact_keys(uapi, {"sha256", "path"}, "manifest UAPI")
    require(isinstance(uapi["sha256"], str) and SHA256.fullmatch(uapi["sha256"]), "manifest UAPI SHA-256 is invalid")
    require(isinstance(uapi["path"], str) and safe_archive_path(uapi["path"]), "manifest UAPI path is unsafe")
    inventory = manifest["packageInventory"]
    require(isinstance(inventory, list) and bool(inventory), "manifest package inventory is empty or invalid")
    normalized = validate_inventory(inventory)
    require(isinstance(manifest["packageInventorySha256"], str) and SHA256.fullmatch(manifest["packageInventorySha256"]), "manifest package inventory SHA-256 is invalid")
    require(sha256_bytes(canonical(normalized)) == manifest["packageInventorySha256"], "manifest package inventory hash is inconsistent")
    matches = [item for item in normalized if item["path"] == uapi["path"]]
    require(len(matches) == 1 and matches[0]["type"] == "file" and matches[0]["sha256"] == uapi["sha256"], "manifest UAPI is not bound to the package inventory")
    result = dict(manifest)
    result["packageInventory"] = normalized
    return result


def safe_archive_path(value: str) -> bool:
    if not value or "\0" in value or value.startswith("/"):
        return False
    parts = pathlib.PurePosixPath(value).parts
    return all(part not in {"", ".", ".."} for part in parts)


def validate_inventory(inventory: Sequence[Any]) -> list[dict[str, Any]]:
    require(len(inventory) <= MAX_PACKAGE_MEMBERS, "package inventory exceeds the bounded member limit")
    result: list[dict[str, Any]] = []
    seen: set[str] = set()
    for raw in inventory:
        require(isinstance(raw, dict), "package inventory member must be an object")
        require_exact_keys(raw, {"path", "type", "mode", "sha256"}, "package inventory member")
        path, kind, mode, digest = raw["path"], raw["type"], raw["mode"], raw["sha256"]
        require(isinstance(path, str) and len(path.encode("utf-8")) <= MAX_ARCHIVE_PATH_BYTES, "package inventory path exceeds the bounded length")
        require(isinstance(path, str) and safe_archive_path(path), "package inventory contains an unsafe path")
        require(path not in seen, f"package inventory repeats path: {path}")
        require(kind in {"file", "directory", "symlink"}, f"package inventory contains unsupported type: {kind!r}")
        require(isinstance(mode, str) and re.fullmatch(r"0[0-7]{3}", mode) is not None, "package inventory mode is invalid")
        require(isinstance(digest, str) and SHA256.fullmatch(digest) is not None, "package inventory SHA-256 is invalid")
        seen.add(path)
        result.append({"path": path, "type": kind, "mode": mode, "sha256": digest})
    return sorted(result, key=lambda item: item["path"])


def tar_inventory(tar_path: pathlib.Path) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    seen: set[str] = set()
    try:
        archive = tarfile.open(tar_path, "r:*")
    except (tarfile.TarError, OSError) as error:
        raise ContractError("Debian data archive is unreadable or uses unsupported compression") from error
    with archive:
        for member_index, member in enumerate(archive, start=1):
            require(member_index <= MAX_PACKAGE_MEMBERS, "package inventory exceeds the bounded member limit")
            path = member.name.removeprefix("./")
            if not path or path == ".":
                continue
            require(safe_archive_path(path), f"package contains unsafe archive path: {member.name!r}")
            require(len(path.encode("utf-8")) <= MAX_ARCHIVE_PATH_BYTES, f"package path exceeds the bounded length: {path!r}")
            require(path not in seen, f"package repeats archive path: {path}")
            seen.add(path)
            mode = f"0{stat.S_IMODE(member.mode):03o}"
            if member.isfile():
                require(0 <= member.size <= MAX_PACKAGE_MEMBER_BYTES, f"package member exceeds the bounded size limit: {path}")
                stream = archive.extractfile(member)
                require(stream is not None, f"package member cannot be read: {path}")
                digest = hashlib.sha256(stream.read()).hexdigest()
                kind = "file"
            elif member.isdir():
                digest = sha256_bytes(b"")
                kind = "directory"
            elif member.issym():
                require(not member.linkname.startswith("/"), f"package symlink has an absolute target: {path}")
                target_parts = pathlib.PurePosixPath(path).parent.joinpath(member.linkname).parts
                depth = 0
                for part in target_parts:
                    if part == "..":
                        depth -= 1
                    elif part not in {"", "."}:
                        depth += 1
                    require(depth >= 0, f"package symlink escapes package root: {path}")
                digest = sha256_bytes(member.linkname.encode())
                kind = "symlink"
            else:
                raise ContractError(f"package contains unsupported special or hard-linked member: {path}")
            result.append({"path": path, "type": kind, "mode": mode, "sha256": digest})
    return sorted(result, key=lambda item: item["path"])


def parse_debian_control(text: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    current: str | None = None
    for line in text.splitlines():
        if line.startswith((" ", "\t")):
            require(current is not None, "Debian control continuation has no field")
            fields[current] += "\n" + line[1:]
            continue
        if not line:
            continue
        require(":" in line, "Debian control contains malformed field")
        key, value = line.split(":", 1)
        require(re.fullmatch(r"[A-Za-z0-9-]+", key) is not None and key not in fields, "Debian control contains invalid or duplicate field")
        fields[key] = value.strip()
        current = key
    return fields


def validate_package(path: pathlib.Path, manifest: Mapping[str, Any], runner: Runner) -> None:
    package = manifest["package"]
    require(path.name == package["filename"], "downloaded package filename differs from manifest")
    require(sha256_file(path) == package["sha256"], "downloaded package SHA-256 differs from manifest")
    control = [
        runner.run(["dpkg-deb", "-f", str(path), field]).stdout.strip()
        for field in ("Package", "Version", "Architecture")
    ]
    require(control == [PACKAGE_NAME, manifest["debianVersion"], "all"], "Debian control identity differs from manifest")
    tar_path = path.parent / "data.tar"
    fsys_command = ["dpkg-deb", "--fsys-tarfile", str(path)]
    runner.trace(fsys_command)
    process = subprocess.Popen(
        fsys_command,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    expanded = 0
    with tar_path.open("wb") as stream:
        assert process.stdout is not None
        for chunk in iter(lambda: process.stdout.read(1024 * 1024), b""):
            expanded += len(chunk)
            if expanded > MAX_EXPANDED_TAR_BYTES:
                process.kill()
                process.wait()
                tar_path.unlink(missing_ok=True)
                raise ContractError("Debian data archive exceeds the bounded expanded size limit")
            stream.write(chunk)
    assert process.stderr is not None
    stderr = process.stderr.read().decode(errors="replace").strip()
    returncode = process.wait()
    if runner.debug and stderr:
        print(stderr, file=sys.stderr)
    require(returncode == 0, f"dpkg-deb could not expose package inventory: {stderr}")
    try:
        actual = tar_inventory(tar_path)
    finally:
        tar_path.unlink(missing_ok=True)
    require(actual == manifest["packageInventory"], "package contents differ from checksummed manifest inventory")


class GitHubClient:
    def __init__(self, token: str | None = None):
        self.headers = {"Accept": "application/vnd.github+json", "User-Agent": "WsprryPi-installer"}
        if token:
            self.headers["Authorization"] = f"Bearer {token}"

    def bytes(self, url: str, *, limit: int = MAX_API_BYTES, allow_asset_redirect: bool = False) -> bytes:
        headers = self.headers if not allow_asset_redirect else {
            "Accept": "application/octet-stream", "User-Agent": "WsprryPi-installer"
        }
        request = urllib.request.Request(url, headers=headers, method="GET")
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                final = response.geturl()
                if final != url:
                    parsed = urllib.parse.urlsplit(final)
                    require(
                        allow_asset_redirect
                        and parsed.scheme == "https"
                        and parsed.netloc in {"release-assets.githubusercontent.com", "objects.githubusercontent.com"},
                        f"unexpected redirect while retrieving {url}",
                    )
                length = response.headers.get("Content-Length")
                if length is not None:
                    require(length.isdigit() and int(length) <= limit, f"remote content exceeds the bounded size limit: {url}")
                data = response.read(limit + 1)
                require(len(data) <= limit, f"remote content exceeds the bounded size limit: {url}")
                return data
        except (urllib.error.URLError, TimeoutError) as error:
            raise ContractError(f"failed to retrieve {url}: {error}") from error

    def json(self, url: str) -> Any:
        try:
            return json.loads(self.bytes(url, limit=MAX_API_BYTES))
        except json.JSONDecodeError as error:
            raise ContractError(f"remote JSON is malformed: {url}") from error

    def releases(self) -> list[Mapping[str, Any]]:
        result: list[Mapping[str, Any]] = []
        for page in range(1, 21):
            payload = self.json(f"{API_BASE}/releases?per_page=100&page={page}")
            require(isinstance(payload, list), "GitHub releases response is not an array")
            result.extend(item for item in payload if isinstance(item, dict))
            if len(payload) < 100:
                return result
        raise ContractError("GitHub release enumeration exceeded the bounded page limit")

    def tag_commit(self, tag: str) -> str:
        payload = self.json(f"{API_BASE}/git/ref/tags/{urllib.parse.quote(tag, safe='')}")
        require(isinstance(payload, dict) and isinstance(payload.get("object"), dict), "Git tag reference response is malformed")
        obj = payload["object"]
        for _ in range(8):
            kind, sha = obj.get("type"), obj.get("sha")
            require(isinstance(sha, str) and SHA40.fullmatch(sha), "Git tag object SHA is malformed")
            if kind == "commit":
                return sha
            require(kind == "tag", "Git tag does not resolve to a commit")
            annotated = self.json(f"{API_BASE}/git/tags/{sha}")
            require(isinstance(annotated, dict) and isinstance(annotated.get("object"), dict), "annotated Git tag response is malformed")
            obj = annotated["object"]
        raise ContractError("Git tag indirection exceeds the bounded resolution limit")


def download_release(state_dir: pathlib.Path, client: GitHubClient, runner: Runner) -> dict[str, Any]:
    release = select_release(client.releases())
    tag = str(release["tag_name"])
    assets = release_assets(release)
    package_names = sorted(name for name in assets if re.fullmatch(r"rp1-gpclk-dkms_[0-9A-Za-z.+:~-]+_all\.deb", name))
    require(len(package_names) == 1, "selected release must contain exactly one RP1-GPCLK-DKMS package")
    package_name = package_names[0]
    for name in (MANIFEST_NAME, CHECKSUMS_NAME, package_name):
        authoritative_asset_url(tag, name, assets[name])
    manifest_bytes = client.bytes(assets[MANIFEST_NAME], limit=MAX_METADATA_BYTES, allow_asset_redirect=True)
    checksums_bytes = client.bytes(assets[CHECKSUMS_NAME], limit=MAX_METADATA_BYTES, allow_asset_redirect=True)
    asset_objects = {
        item["name"]: item for item in release.get("assets", [])
        if isinstance(item, dict) and isinstance(item.get("name"), str)
    }
    for name, data in ((MANIFEST_NAME, manifest_bytes), (CHECKSUMS_NAME, checksums_bytes)):
        api_digest = asset_objects[name].get("digest")
        if api_digest is not None:
            require(api_digest == f"sha256:{sha256_bytes(data)}", f"GitHub asset digest differs for {name}")
    checksums = parse_checksums(checksums_bytes)
    require(checksums.get(MANIFEST_NAME) == sha256_bytes(manifest_bytes), "SHA256SUMS does not bind the installation manifest")
    try:
        manifest_raw = json.loads(manifest_bytes)
    except json.JSONDecodeError as error:
        raise ContractError("selected release installation manifest is malformed") from error
    tag_commit = client.tag_commit(tag)
    manifest = validate_manifest(manifest_raw, tag=tag, tag_commit=tag_commit)
    require(manifest["package"]["filename"] == package_name, "selected package asset differs from manifest")
    require(checksums.get(package_name) == manifest["package"]["sha256"], "SHA256SUMS package identity differs from manifest")
    package_path = state_dir / package_name
    package_bytes = client.bytes(assets[package_name], limit=MAX_PACKAGE_BYTES, allow_asset_redirect=True)
    api_package_digest = asset_objects[package_name].get("digest")
    if api_package_digest is not None:
        require(api_package_digest == f"sha256:{sha256_bytes(package_bytes)}", "GitHub asset digest differs for the package")
    package_path.write_bytes(package_bytes)
    os.chmod(package_path, 0o600)
    validate_package(package_path, manifest, runner)
    (state_dir / MANIFEST_NAME).write_bytes(canonical(manifest) + b"\n")
    os.chmod(state_dir / MANIFEST_NAME, 0o600)
    checkout = clone_exact(state_dir, "commit", tag_commit, runner)
    source = pathlib.Path(checkout["path"])
    require_compatible_runtime_source(source, tag_commit, runner)
    runtime_source_interface(source)
    upstream_identity = development_identity(source, runner)
    require(
        upstream_identity["version"] == manifest["productVersion"],
        "release package and exact runtime source product versions differ",
    )
    runtime_source_identity(source, upstream_identity["version"])
    return {
        "channel": "release", "tag": tag, "commit": tag_commit,
        "manifest": manifest, "packagePath": str(package_path),
        "checkout": checkout, "runtimeVersion": upstream_identity["version"],
    }


def git_output(runner: Runner, source: pathlib.Path, *args: str) -> str:
    return runner.run(["git", "-C", str(source), *args]).stdout.strip()


def require_compatible_runtime_source(
    source: pathlib.Path, commit: str, runner: Runner
) -> None:
    require(SHA40.fullmatch(commit) is not None, "selected runtime source commit is invalid")
    available = runner.run(
        ["git", "-C", str(source), "cat-file", "-e", f"{CORRECTED_RUNTIME_COMMIT}^{{commit}}"],
        check=False,
    )
    require(
        available.returncode == 0,
        "selected source cannot prove the corrected RP1 runtime ancestry",
    )
    compatible = runner.run(
        ["git", "-C", str(source), "merge-base", "--is-ancestor", CORRECTED_RUNTIME_COMMIT, commit],
        check=False,
    )
    require(
        compatible.returncode == 0,
        "selected RP1-GPCLK-DKMS source predates the corrected runtime controller UAPI",
    )


def validate_checkout(source_text: str, runner: Runner) -> dict[str, Any]:
    requested = pathlib.Path(source_text)
    require(requested.is_absolute(), "checkout source requires an absolute path")
    require(requested.exists() and requested.is_dir(), "checkout source is not an existing directory")
    absolute = pathlib.Path(os.path.abspath(requested))
    resolved = requested.resolve(strict=True)
    require(absolute == resolved, "checkout source contains a symlink or path substitution")
    top = pathlib.Path(git_output(runner, resolved, "rev-parse", "--show-toplevel"))
    require(top.resolve(strict=True) == resolved, "checkout path is not the repository root")
    remote = git_output(runner, resolved, "remote", "get-url", "origin")
    require(normalize_repository_url(remote) == REPOSITORY_URL, "checkout is not the authoritative RP1-GPCLK-DKMS repository")
    commit = git_output(runner, resolved, "rev-parse", "HEAD")
    require(SHA40.fullmatch(commit) is not None, "checkout HEAD is not a full commit SHA")
    status_text = git_output(runner, resolved, "status", "--porcelain=v1", "--untracked-files=all")
    require(not status_text, "checkout has tracked or untracked changes")
    flags = git_output(runner, resolved, "ls-files", "-v").splitlines()
    require(all(line.startswith("H ") for line in flags), "checkout uses assume-unchanged, skip-worktree, or unsupported tracked-file state")
    tree = git_output(runner, resolved, "rev-parse", "HEAD^{tree}")
    require(SHA40.fullmatch(tree) is not None, "checkout tree identity is invalid")
    root_stat, git_stat = resolved.stat(), pathlib.Path(git_output(runner, resolved, "rev-parse", "--absolute-git-dir")).resolve(strict=True).stat()
    return {
        "path": str(resolved), "commit": commit, "tree": tree,
        "rootDevice": root_stat.st_dev, "rootInode": root_stat.st_ino,
        "gitDevice": git_stat.st_dev, "gitInode": git_stat.st_ino,
    }


def clone_exact(state_dir: pathlib.Path, selector: str, value: str | None, runner: Runner) -> dict[str, Any]:
    source = state_dir / "source"
    runner.run(["git", "clone", "--no-checkout", "--origin", "origin", REPOSITORY_URL, str(source)])
    if selector == "devel":
        result = runner.run(["git", "-C", str(source), "rev-parse", "--verify", "origin/devel^{commit}"], check=False)
        require(result.returncode == 0, "authoritative RP1-GPCLK-DKMS origin/devel is unavailable")
        commit = result.stdout.strip()
    else:
        require(value is not None and SHA40.fullmatch(value) is not None, "internal exact-commit resolution error")
        result = runner.run(["git", "-C", str(source), "cat-file", "-e", f"{value}^{{commit}}"], check=False)
        require(result.returncode == 0, "requested RP1-GPCLK-DKMS commit does not exist in the authoritative repository")
        commit = value
    require(SHA40.fullmatch(commit) is not None, "resolved development commit is not immutable")
    runner.run(["git", "-C", str(source), "checkout", "--detach", commit])
    identity = validate_checkout(str(source), runner)
    require(identity["commit"] == commit, "development checkout changed after immutable resolution")
    return identity


def development_identity(source: pathlib.Path, runner: Runner) -> dict[str, str]:
    preflight = source / "scripts" / "development-preflight"
    require(preflight.is_file() and os.access(preflight, os.X_OK), "development source lacks the maintained preflight")
    result = runner.run(
        [str(preflight), "--source", str(source), "--kernel", platform.release()],
        cwd=source,
    )
    try:
        payload = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        raise ContractError("upstream development preflight returned malformed identity") from error
    require(isinstance(payload, dict), "upstream development preflight identity is not an object")
    version = payload.get("moduleVersion")
    version_source = payload.get("versionSource")
    version_sha256 = payload.get("versionSourceSha256")
    require(payload.get("classification") == "source-development", "upstream development preflight classification differs")
    require(payload.get("moduleName") == MODULE_NAME, "upstream development preflight module identity differs")
    require(isinstance(version, str) and SAFE_VERSION.fullmatch(version), "upstream development preflight has an invalid version")
    require(version_source == "include/rp1_gpclk/version.h", "upstream development preflight version source differs")
    path = source / version_source
    require(path.is_file() and not path.is_symlink(), "upstream canonical development version source is missing or substituted")
    require(version_sha256 == sha256_file(path), "upstream development version identity differs from the selected source")
    return {"version": version, "versionSource": version_source, "versionSourceSha256": version_sha256}


def route_neutral_interface(source: pathlib.Path, runner: Runner) -> tuple[str, list[str]]:
    installer = source / "scripts" / "development-install"
    preflight = source / "scripts" / "development-preflight"
    require(installer.is_file() and os.access(installer, os.X_OK) and preflight.is_file() and os.access(preflight, os.X_OK), "development source lacks maintained executable lifecycle tools")
    help_result = runner.run([str(installer), "--help"], cwd=source)
    help_text = help_result.stdout + help_result.stderr
    require("--runtime-controller" in help_text,
            "upstream development-install lacks the single-authority runtime-controller profile")
    if "--route-neutral" in help_text:
        return "route-neutral-flag", ["--route-neutral"]
    if re.search(r"--route[^\n]*route-neutral", help_text):
        return "route-choice", ["--route", "route-neutral"]
    raise ContractError("upstream development-install has no reviewed route-neutral installation interface; no development mutation was attempted")


def runtime_source_interface(source: pathlib.Path) -> None:
    required = (
        "Makefile",
        "scripts/build_runtime_bundle.py",
        "scripts/runtime_provider.py",
        "scripts/runtime_activation.py",
        "scripts/runtime_deployment.py",
        "scripts/runtime_binding.py",
        "scripts/runtime_layout.py",
        "schema/rp1-gpclk-runtime-readiness-v1.schema.json",
        "include/uapi/linux/rp1_gpclk.h",
        "include/uapi/linux/rp1_route_admin.h",
        "systemd/rp1-gpclk-route-manager.socket",
        "systemd/rp1-gpclk-route-manager@.service",
        "systemd/95-runtime-controller.conf",
    )
    for relative in required:
        path = source / relative
        require(path.is_file() and not path.is_symlink(), f"selected source lacks runtime administration input: {relative}")


def runtime_source_identity(source: pathlib.Path, product: str) -> None:
    try:
        layout = (source / "scripts/runtime_layout.py").read_text(encoding="utf-8")
        binding = (source / "scripts/runtime_binding.py").read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise ContractError("selected runtime source identity is unreadable") from error
    product_match = re.search(r"(?m)^PRODUCT_VERSION\s*=\s*(['\"])([^'\"]+)\1\s*$", binding)
    contract_match = re.search(r"(?m)^CONTRACT\s*=\s*(['\"])([^'\"]+)\1\s*$", binding)
    compatibility_match = re.search(r"(?m)^COMPATIBILITY\s*=\s*(\{[^\n]+\})\s*$", binding)
    require(re.search(r"(?m)^KERNEL\s*=", layout) is None,
            "selected runtime source hard-codes a kernel instead of using the build binding")
    require(product_match is not None and product_match.group(2) == product, "selected runtime source product version differs")
    require(contract_match is not None and contract_match.group(2) == RUNTIME_BINDING_CONTRACT, "selected runtime binding contract differs")
    try:
        compatibility = ast.literal_eval(compatibility_match.group(1)) if compatibility_match else None
    except (SyntaxError, ValueError) as error:
        raise ContractError("selected runtime compatibility identity is malformed") from error
    require(
        compatibility == {
            "gpio4": f"v{product}-rp1-gpio4",
            "gpio20": f"v{product}-rp1-gpio20",
        },
        "selected runtime route compatibility identities differ",
    )


def revalidate_checkout(identity: Mapping[str, Any], runner: Runner) -> pathlib.Path:
    source = pathlib.Path(str(identity["path"]))
    try:
        current = validate_checkout(str(source), runner)
    except ContractError as error:
        raise ContractError("development checkout was dirty or replaced after preflight") from error
    for key in ("commit", "tree", "rootDevice", "rootInode", "gitDevice", "gitInode"):
        require(current[key] == identity[key], "development checkout was dirty or replaced after preflight")
    return source


def prepare_development(state_dir: pathlib.Path, selector: str, value: str | None, runner: Runner) -> dict[str, Any]:
    if selector == "checkout":
        original = validate_checkout(str(value), runner)
        snapshot = state_dir / "source"
        runner.run(["git", "clone", "--no-local", "--no-checkout", original["path"], str(snapshot)])
        runner.run(["git", "-C", str(snapshot), "remote", "set-url", "origin", REPOSITORY_URL])
        runner.run(["git", "-C", str(snapshot), "checkout", "--detach", original["commit"]])
        identity = validate_checkout(str(snapshot), runner)
        require(
            identity["commit"] == original["commit"] and identity["tree"] == original["tree"],
            "local checkout snapshot differs from the selected commit",
        )
    else:
        identity = clone_exact(state_dir, selector, value, runner)
    source = pathlib.Path(identity["path"])
    require_compatible_runtime_source(source, identity["commit"], runner)
    uapi = source / "include" / "uapi" / "linux" / "rp1_gpclk.h"
    require(uapi.is_file() and not uapi.is_symlink(), "development source lacks the canonical regular-file UAPI")
    interface, route_args = route_neutral_interface(source, runner)
    runtime_source_interface(source)
    upstream_identity = development_identity(source, runner)
    runtime_source_identity(source, upstream_identity["version"])
    return {
        "channel": "development", "commit": identity["commit"], "version": upstream_identity["version"],
        "checkout": identity, "interface": interface, "routeArguments": route_args,
        "sourceTree": identity["tree"], "uapiSha256": sha256_file(uapi),
        "versionSource": upstream_identity["versionSource"],
        "versionSourceSha256": upstream_identity["versionSourceSha256"],
        "compatibilityIdentity": COMPATIBILITY_IDENTITY,
    }


def secure_state_dir(path: pathlib.Path) -> pathlib.Path:
    require(path.is_absolute() and path.exists() and path.is_dir(), "state directory must be an existing absolute directory")
    resolved = path.resolve(strict=True)
    require(resolved == pathlib.Path(os.path.abspath(path)), "state directory cannot contain symlinks")
    info = resolved.stat()
    require(info.st_uid == os.geteuid(), "state directory is not owned by the invoking user")
    require(stat.S_IMODE(info.st_mode) & 0o077 == 0, "state directory must not be accessible by group or others")
    return resolved


def secure_record_parent(path: pathlib.Path) -> None:
    parent = path.parent
    parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    absolute = pathlib.Path(os.path.abspath(parent))
    resolved = parent.resolve(strict=True)
    require(absolute == resolved, "installation record parent contains a symlink or path substitution")
    info = resolved.stat()
    require(info.st_uid == os.geteuid(), "installation record parent is not owned by the invoking user")
    require(stat.S_IMODE(info.st_mode) & 0o022 == 0, "installation record parent is group- or world-writable")


def atomic_json(path: pathlib.Path, value: Any, mode: int = 0o600) -> None:
    payload = canonical(value) + b"\n"
    descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        os.fchmod(descriptor, mode)
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
        parent_descriptor = os.open(path.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(parent_descriptor)
        finally:
            os.close(parent_descriptor)
    finally:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass


def atomic_json_new(path: pathlib.Path, value: Any, mode: int = 0o600) -> None:
    """Publish a new ownership record without replacing any concurrent state."""
    payload = canonical(value) + b"\n"
    descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        os.fchmod(descriptor, mode)
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        os.link(temporary, path, follow_symlinks=False)
        parent_descriptor = os.open(path.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(parent_descriptor)
        finally:
            os.close(parent_descriptor)
    except FileExistsError as error:
        raise ContractError("installation ownership record appeared during provider installation") from error
    finally:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass


def record_file_identity(path: pathlib.Path) -> os.stat_result:
    require(path.is_absolute(), "installation ownership record path must be absolute")
    secure_record_parent(path)
    try:
        info = path.lstat()
    except FileNotFoundError as error:
        raise ContractError("installation ownership record is absent") from error
    require(stat.S_ISREG(info.st_mode), "installation ownership record is not a regular file")
    require(info.st_uid == os.geteuid(), "installation ownership record is not owned by the invoking user")
    require(stat.S_IMODE(info.st_mode) & 0o077 == 0, "installation ownership record is accessible by group or others")
    require(info.st_size <= MAX_RECORD_BYTES, "installation ownership record exceeds the bounded size")
    return info


def load_ownership_record(path: pathlib.Path) -> tuple[dict[str, Any] | None, os.stat_result | None, str | None]:
    """Return a validated v2 record, or a preservation reason without following unsafe state."""
    if not path.is_absolute():
        return None, None, "ownership record path is not absolute"
    if not path.exists() and not path.is_symlink():
        return None, None, "no WsprryPi ownership record exists"
    try:
        info = record_file_identity(path)
        descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
        try:
            opened = os.fstat(descriptor)
            require(
                (opened.st_dev, opened.st_ino) == (info.st_dev, info.st_ino),
                "installation ownership record changed while opening",
            )
            with os.fdopen(descriptor, "rb", closefd=False) as stream:
                payload = stream.read(MAX_RECORD_BYTES + 1)
        finally:
            os.close(descriptor)
        require(len(payload) <= MAX_RECORD_BYTES, "installation ownership record exceeds the bounded size")
        value = json.loads(payload.decode("utf-8"))
        require(isinstance(value, dict), "installation ownership record is not an object")
        if value.get("schema") == LEGACY_RECORD_SCHEMA:
            return None, info, "legacy v1 installation record cannot prove WsprryPi ownership"
        schema = value.get("schema")
        require(schema in {RECORD_SCHEMA, RUNTIME_RECORD_SCHEMA}, "installation ownership record schema is unsupported")
        common = {
            "schema", "repository", "owner", "channel", "installationMethod",
            "routeActivation", "output", "qualificationClaim",
        }
        channel = value.get("channel")
        if channel == "release":
            required = common | {"tag", "sourceCommit", "manifest", "moduleArtifacts"}
        elif channel == "development":
            required = common | {
                "sourceCommit", "productVersion", "sourceTree", "uapiSha256",
                "versionSource", "versionSourceSha256", "targetKernel",
                "compatibilityIdentity", "upstreamEvidence", "rollbackRecord",
                "rollbackRecordSha256", "rollbackEntrypoint",
                "rollbackEntrypointSha256", "installedModulePath",
                "installedModuleSha256", "decompressedModuleSha256",
            }
        else:
            raise ContractError("installation ownership record channel is unsupported")
        if schema == RUNTIME_RECORD_SCHEMA:
            required.add("runtime")
        require_exact_keys(value, required, "installation ownership record")
        require(value["repository"] == REPOSITORY, "installation ownership repository differs")
        require(value["owner"] == "WsprryPi", "installation ownership owner differs")
        require(value["routeActivation"] == "disabled" and value["output"] == "disabled", "installation ownership record is not route-neutral")
        require(value["qualificationClaim"] is False, "installation ownership record contains an unsupported qualification claim")
        if channel == "release":
            require(isinstance(value["tag"], str) and isinstance(value["sourceCommit"], str), "release ownership identity fields are invalid")
            require(isinstance(value["manifest"], dict) and isinstance(value["moduleArtifacts"], list), "release ownership evidence fields are invalid")
        else:
            string_fields = required - common - {"runtime"}
            require(all(isinstance(value[field], str) for field in string_fields), "development ownership evidence fields are invalid")
        if schema == RUNTIME_RECORD_SCHEMA:
            validate_runtime_ownership(value["runtime"], value)
        return value, info, None
    except (ContractError, OSError, UnicodeError, json.JSONDecodeError) as error:
        return None, None, str(error)


def validate_runtime_ownership(runtime: Any, record: Mapping[str, Any]) -> dict[str, Any]:
    required = {
        "readinessContract", "bindingSha256", "artifactSetSha256",
        "sourceCommit", "productVersion", "targetKernel",
        "compatibilityIdentities", "deploymentPlanSha256",
        "activationPlanSha256", "activationRequestId", "controllerSession",
        "controllerGeneration", "state", "route", "output",
    }
    require(isinstance(runtime, dict), "installation runtime ownership is not an object")
    require_exact_keys(runtime, required, "installation runtime ownership")
    require(runtime["readinessContract"] == RUNTIME_READINESS_CONTRACT, "runtime readiness contract differs")
    for field in (
        "bindingSha256", "artifactSetSha256", "deploymentPlanSha256",
        "activationPlanSha256",
    ):
        require(isinstance(runtime[field], str) and SHA256.fullmatch(runtime[field]), f"runtime ownership has invalid {field}")
    require(isinstance(runtime["sourceCommit"], str) and SHA40.fullmatch(runtime["sourceCommit"]), "runtime source commit is invalid")
    require(runtime["sourceCommit"] == record.get("sourceCommit"), "runtime source commit differs from provider ownership")
    product = record.get("productVersion")
    if record.get("channel") == "release":
        product = record.get("manifest", {}).get("productVersion")
    require(runtime["productVersion"] == product, "runtime product version differs from provider ownership")
    require(runtime["targetKernel"] == platform.release(), "runtime ownership targets a different running kernel")
    compatibility = runtime["compatibilityIdentities"]
    require(
        isinstance(compatibility, dict)
        and set(compatibility) == {"gpio4", "gpio20"}
        and all(isinstance(value, str) and value for value in compatibility.values()),
        "runtime compatibility identities are invalid",
    )
    require(isinstance(runtime["activationRequestId"], str) and len(runtime["activationRequestId"]) >= 8, "runtime activation request ID is invalid")
    require(type(runtime["controllerSession"]) is int and runtime["controllerSession"] > 0, "runtime controller session is invalid")
    require(type(runtime["controllerGeneration"]) is int and runtime["controllerGeneration"] == 0, "runtime neutral controller generation differs")
    require(runtime["state"] == "neutral_ready", "runtime ownership is not neutral-ready")
    require(runtime["route"] is None, "runtime ownership selected a GPIO route")
    require(runtime["output"] == "disabled", "runtime ownership does not retain disabled output")
    return runtime


def runtime_deployment_destinations(files: Iterable[str] = ()) -> set[str]:
    library = "/usr/lib/rp1-gpclk-dkms"
    state = "/var/lib/rp1-gpclk-dkms/runtime-admin"
    return {
        *files,
        "/etc/rp1-gpclk-dkms/runtime-controller.json",
        "/etc/systemd/system/rp1-gpclk-route-manager@.service.d/95-runtime-controller.conf",
        "/usr/lib/systemd/system/rp1-gpclk-route-manager.socket",
        "/usr/lib/systemd/system/rp1-gpclk-route-manager@.service",
        *(f"{library}/{name}" for name in (
            "runtime_controller_admin.py", "runtime_manager.py",
            "runtime_route_client.py", "runtime_layout.py",
            "runtime_deployment.py", "runtime_output.py",
            "runtime_application.py", "runtime_activation.py",
            "runtime_provider.py", "runtime_binding.py")),
        *(f"{library}/runtime-uapi/{name}" for name in
          ("rp1_gpclk.h", "rp1_route_admin.h")),
        *(f"{library}/runtime-overlays/{route}.dtbo" for route in
          ("gpio4", "gpio20")),
        f"{library}/schema/rp1-gpclk-runtime-readiness-v1.schema.json",
        *(f"{state}/{name}" for name in
          ("transaction.json", "manager.json", "application.json",
           "activation.json")),
    }


def preserve_owned_activation_journal(record: Mapping[str, Any], label: str,
                                      clear: bool = False,
                                      journal: pathlib.Path = pathlib.Path(
                                          "/var/lib/rp1-gpclk-dkms/runtime-admin/activation.json")) -> None:
    """Archive exact activation evidence, then clear it for owned deployment inversion."""
    if not journal.exists() and not journal.is_symlink():
        return
    try:
        info = journal.lstat()
        require(stat.S_ISREG(info.st_mode),
                "runtime activation journal is missing or substituted")
        descriptor = os.open(journal, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
        try:
            opened = os.fstat(descriptor)
            require((opened.st_dev, opened.st_ino) == (info.st_dev, info.st_ino)
                    and opened.st_uid == os.geteuid()
                    and stat.S_IMODE(opened.st_mode) & 0o077 == 0,
                    "runtime activation journal ownership or mode differs")
            with os.fdopen(descriptor, "rb", closefd=False) as stream:
                raw = stream.read(MAX_METADATA_BYTES + 1)
        finally:
            os.close(descriptor)
    except OSError as error:
        raise ContractError("runtime activation journal is unavailable") from error
    require(len(raw) <= MAX_METADATA_BYTES,
            "runtime activation journal exceeds read bound")
    try:
        value = json.loads(raw)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ContractError("runtime activation journal is unreadable") from error
    phase = value.get("phase") if isinstance(value, dict) else None
    require(phase in {"activation-failed", "recovered-inhibited", "complete-neutral"},
            "runtime activation journal is not terminal owned evidence")
    request_id = value.get("requestId")
    require(isinstance(request_id, str)
            and re.fullmatch(r"[A-Za-z0-9._-]{8,128}", request_id) is not None,
            "runtime activation journal request identity is invalid")
    evidence_value = record.get("upstreamEvidence")
    evidence = (pathlib.Path(evidence_value) if isinstance(evidence_value, str)
                else pathlib.Path("/var/lib/wsprrypi/rp1-gpclk-dkms-runtime-evidence"))
    if not evidence.exists() and record.get("channel") == "release":
        evidence.mkdir(mode=0o700)
    require(evidence.is_absolute() and evidence.is_dir() and not evidence.is_symlink(),
            "runtime ownership evidence is unavailable")
    archive = evidence / "runtime-activation-archives"
    archive.mkdir(mode=0o700, exist_ok=True)
    require(archive.is_dir() and not archive.is_symlink()
            and archive.stat().st_uid == os.geteuid()
            and stat.S_IMODE(archive.stat().st_mode) & 0o077 == 0,
            "runtime activation archive directory is unsafe")
    destination = archive / f"{label}-{request_id}.json"
    archived = {
        "schema": "wsprrypi-rp1-gpclk-activation-archive-v1",
        "phase": phase,
        "requestId": request_id,
        "sha256": sha256_bytes(raw),
        "contentBase64": base64.b64encode(raw).decode("ascii"),
    }
    if destination.exists() or destination.is_symlink():
        require(destination.is_file() and not destination.is_symlink(),
                "runtime activation archive is substituted")
        try:
            existing = json.loads(destination.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            raise ContractError("runtime activation archive is unreadable") from error
        require(existing == archived, "runtime activation archive identity differs")
    else:
        atomic_json_new(destination, archived)
    current = journal.lstat()
    require(stat.S_ISREG(current.st_mode)
            and (current.st_dev, current.st_ino) == (info.st_dev, info.st_ino),
            "runtime activation journal changed during preservation")
    descriptor = os.open(journal, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
    try:
        with os.fdopen(descriptor, "rb", closefd=False) as stream:
            current_raw = stream.read(MAX_METADATA_BYTES + 1)
    finally:
        os.close(descriptor)
    require(current_raw == raw, "runtime activation journal changed during preservation")
    if clear:
        journal.unlink()
        parent_descriptor = os.open(journal.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(parent_descriptor)
        finally:
            os.close(parent_descriptor)


def remove_ownership_record(path: pathlib.Path, expected: os.stat_result) -> None:
    current = record_file_identity(path)
    require(
        (current.st_dev, current.st_ino) == (expected.st_dev, expected.st_ino),
        "installation ownership record changed during provider removal",
    )
    path.unlink()
    parent_descriptor = os.open(path.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
    try:
        os.fsync(parent_descriptor)
    finally:
        os.close(parent_descriptor)


def module_artifacts(root: pathlib.Path) -> list[dict[str, str]]:
    base = root_path(root, "/lib/modules")
    artifacts: list[dict[str, str]] = []
    if not base.exists():
        return artifacts
    for path in sorted(base.glob("*/updates/dkms/rp1_gpclk_dkms.ko*")):
        relative = pathlib.PurePosixPath("/" + str(path.relative_to(root)))
        installed = installed_member(root, str(relative).removeprefix("/"), "file")
        artifacts.append({"path": str(relative), "sha256": sha256_file(installed)})
    return artifacts


def ensure_new_ownership_record(path: pathlib.Path) -> None:
    require(path.is_absolute(), "installation ownership record path must be absolute")
    secure_record_parent(path)
    require(not path.exists() and not path.is_symlink(), "stale installation ownership state blocks a new provider installation")


def render_plan(plan: Mapping[str, Any]) -> None:
    print("RP1-GPCLK-DKMS installation plan")
    print(f"  WsprryPi source channel: {plan['wsprryChannel']}")
    print(f"  Installation decision: {'install' if plan['decision']['install'] else 'skip'} ({plan['decision']['reason']})")
    print(f"  Detected platform: {plan['platform']['model']} [{plan['platform']['classification']}]")
    print(f"  Explicit platform override: {'yes' if plan['decision']['platformOverride'] else 'no'}")
    print(f"  Requested source selector: {plan['requestedSource']}")
    if plan.get("resolved"):
        resolved = plan["resolved"]
        print(f"  Resolved channel: {resolved['channel']}")
        print(f"  Resolved tag or commit: {resolved.get('tag') or resolved.get('commit')}")
        if resolved["channel"] == "release":
            manifest = resolved["manifest"]
            print(f"  Product/Debian version: {manifest['productVersion']} / {manifest['debianVersion']}")
            print(f"  Package SHA-256: {manifest['package']['sha256']}")
            print(f"  UAPI SHA-256: {manifest['uapi']['sha256']}")
            print("  Planned lifecycle: apt-get install of the validated local Debian package")
        else:
            print(f"  Source version: {resolved['version']}")
            print(f"  Source tree: {resolved['sourceTree']}")
            print(f"  UAPI SHA-256: {resolved['uapiSha256']}")
            print("  Module SHA-256: resolved and verified by the upstream exact-source build")
            print("  Planned lifecycle: upstream development-preflight and exact-source development-install")
    print("  Safety boundary: no route, overlay, module load, service, GPIO, transmission, or RF action is authorized.")


def prepare(args: argparse.Namespace, runner: Runner) -> dict[str, Any]:
    state_dir = secure_state_dir(args.state_dir)
    detected = detect_platform(args.model_file, args.compatible_file)
    decision = installation_decision(args.install, detected)
    explicit_source = args.source != "auto"
    try:
        channel = wsprry_channel(args.wsprry_source)
    except ContractError:
        if not explicit_source:
            raise
        channel = "explicit-source"
    plan: dict[str, Any] = {
        "schema": STATE_SCHEMA, "wsprrySource": args.wsprry_source, "wsprryChannel": channel,
        "platform": detected, "decision": decision, "requestedSource": args.source,
        "resolved": None, "dryRun": args.dry_run,
    }
    if decision["install"]:
        residue = runtime_residue_inventory(pathlib.Path("/"))
        if residue:
            owned, _, reason = load_ownership_record(args.record)
            require(
                owned is not None
                and owned.get("schema") in {RECORD_SCHEMA, RUNTIME_RECORD_SCHEMA},
                "RP1 runtime-controller residue blocks installation planning; use the owning runtime cleanup workflow: "
                + ", ".join(residue) + (f" ({reason})" if reason else ""),
            )
        selector, value = source_selector(args.source, channel)
        if selector == "release":
            plan["resolved"] = download_release(
                state_dir, GitHubClient(os.environ.get("GITHUB_TOKEN")), runner
            )
        else:
            plan["resolved"] = prepare_development(state_dir, selector, value, runner)
    atomic_json(state_dir / "plan.json", plan)
    render_plan(plan)
    return plan


def root_path(root: pathlib.Path, absolute: str) -> pathlib.Path:
    require(absolute.startswith("/"), "internal installed path must be absolute")
    return root / absolute.removeprefix("/")


def runtime_residue_inventory(root: pathlib.Path) -> list[str]:
    runtime_paths = (
        "/etc/rp1-gpclk-dkms/runtime-controller.json",
        "/etc/systemd/system/rp1-gpclk-route-manager@.service.d/95-runtime-controller.conf",
        "/etc/systemd/system/wsprrypi.service.d/90-rp1-route-inhibit.conf",
        "/etc/systemd/system/wsprrypi.service.d/91-rp1-route-idle.conf",
        "/usr/lib/systemd/system/rp1-gpclk-route-manager.socket",
        "/usr/lib/systemd/system/rp1-gpclk-route-manager@.service",
        "/var/lib/rp1-gpclk-dkms/runtime-admin",
        "/run/rp1-gpclk-dkms/route-manager.sock",
        "/usr/lib/rp1-gpclk-dkms/runtime-uapi",
        "/usr/lib/rp1-gpclk-dkms/runtime-overlays",
        "/usr/lib/rp1-gpclk-dkms/runtime_application.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_activation.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_binding.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_controller_admin.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_deployment.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_layout.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_manager.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_output.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_provider.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_route_client.py",
        "/usr/lib/rp1-gpclk-dkms/schema/rp1-gpclk-runtime-readiness-v1.schema.json",
        "/dev/rp1-route-admin",
        "/dev/rp1-gpclk",
        "/sys/module/rp1_route_controller",
        "/sys/module/rp1_gpclk_dkms",
    )
    residue = []
    for path_text in runtime_paths:
        path = root_path(root, path_text)
        if path.exists() or path.is_symlink():
            residue.append(str(path))
    return sorted(set(residue))


def existing_inventory(root: pathlib.Path, runner: Runner) -> dict[str, Any]:
    package = runner.run(["dpkg-query", "-W", "-f=${Status}\n${Version}\n", PACKAGE_NAME], check=False)
    package_version = None
    if package.returncode == 0:
        lines = package.stdout.splitlines()
        require(len(lines) >= 2 and lines[0] == "install ok installed", "existing package state is ambiguous")
        package_version = lines[1]
    dkms = runner.run(["dkms", "status", "-m", DKMS_NAME], check=False)
    modules_file = root_path(root, "/proc/modules")
    active = False
    active_controller = False
    if modules_file.exists():
        active = any(line.split(maxsplit=1)[0] == MODULE_NAME for line in modules_file.read_text(errors="replace").splitlines() if line)
        active_controller = any(line.split(maxsplit=1)[0] == CONTROLLER_MODULE_NAME
            for line in modules_file.read_text(errors="replace").splitlines() if line)
    source_base = root_path(root, "/usr/src")
    sources = sorted(str(path) for path in source_base.glob(f"{PACKAGE_NAME}-*") if source_base.exists())
    module_candidates: list[str] = []
    controller_candidates: list[str] = []
    modules_base = root_path(root, "/lib/modules")
    if modules_base.exists():
        module_candidates = sorted(str(path) for path in modules_base.glob("*/updates/dkms/rp1_gpclk_dkms.ko*"))
        controller_candidates = sorted(str(path) for path in modules_base.glob("*/updates/dkms/rp1_route_controller.ko*"))
    enrollment = root_path(root, "/etc/rp1-gpclk-dkms/enrollment.json").exists()
    manager = root_path(root, "/etc/systemd/system/rp1-gpclk-route-manager@.service.d/90-source-development.conf").exists()
    overlay_paths = [
        root_path(root, "/boot/firmware/overlays/rp1-gpclk-gpio4.dtbo"),
        root_path(root, "/boot/firmware/overlays/rp1-gpclk-gpio20.dtbo"),
        root_path(root, "/usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio4.dtbo"),
        root_path(root, "/usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio20.dtbo"),
    ]
    overlays = sorted(str(path) for path in overlay_paths if path.exists() or path.is_symlink())
    configured = False
    for config in (root_path(root, "/boot/firmware/config.txt"), root_path(root, "/boot/config.txt")):
        if config.is_file() and not config.is_symlink():
            configured = configured or "dtoverlay=rp1-gpclk-" in config.read_text(errors="replace")
    return {
        "packageVersion": package_version, "dkms": dkms.stdout.strip(),
        "activeModule": active, "activeController": active_controller,
        "sourceTrees": sources, "moduleCandidates": module_candidates,
        "controllerCandidates": controller_candidates,
        "installedOverlays": overlays, "configuredRoute": configured,
        "enrollment": enrollment, "developmentManager": manager,
        "runtimeResidue": runtime_residue_inventory(root),
    }


def require_no_runtime_residue(inventory: Mapping[str, Any], operation: str) -> None:
    residue = inventory.get("runtimeResidue")
    require(isinstance(residue, list), "RP1 runtime-controller inventory is unavailable")
    require(
        not residue,
        f"RP1 runtime-controller residue blocks {operation}; use the owning runtime cleanup workflow: "
        + ", ".join(str(path) for path in residue),
    )


def installed_member(root: pathlib.Path, relative: str, expected_type: str) -> pathlib.Path:
    current = root
    parts = pathlib.PurePosixPath(relative).parts
    for index, part in enumerate(parts):
        current = current / part
        try:
            info = current.lstat()
        except FileNotFoundError as error:
            raise ContractError(f"installed package member is missing: /{relative}") from error
        final = index == len(parts) - 1
        if not final:
            require(stat.S_ISDIR(info.st_mode) and not stat.S_ISLNK(info.st_mode), f"installed package path has a substituted ancestor: /{relative}")
        elif expected_type == "file":
            require(stat.S_ISREG(info.st_mode), f"installed package file is substituted: /{relative}")
        elif expected_type == "directory":
            require(stat.S_ISDIR(info.st_mode) and not stat.S_ISLNK(info.st_mode), f"installed package directory is substituted: /{relative}")
        else:
            require(stat.S_ISLNK(info.st_mode), f"installed package symlink is substituted: /{relative}")
    return current


def verify_installed_release(
    root: pathlib.Path,
    manifest: Mapping[str, Any],
    runner: Runner,
    *,
    allow_owned_runtime: bool = False,
) -> None:
    inventory = existing_inventory(root, runner)
    if not allow_owned_runtime:
        require_no_runtime_residue(inventory, "release verification")
    require(inventory["packageVersion"] == manifest["debianVersion"], "installed Debian package version differs from plan")
    require(not inventory["activeModule"], "RP1 GPCLK module became active during route-neutral installation")
    require(not inventory["configuredRoute"], "RP1 GPCLK route became configured during route-neutral installation")
    require(not inventory["enrollment"] and not inventory["developmentManager"], "installed provider has unexpected development enrollment or manager binding")
    dkms_lines = [line for line in inventory["dkms"].splitlines() if line.strip()]
    expected_dkms = re.compile(rf"^{re.escape(DKMS_NAME)}/{re.escape(manifest['productVersion'])}[^\n]*installed$")
    require(bool(dkms_lines) and all(expected_dkms.fullmatch(line) for line in dkms_lines), "DKMS registration contains a missing, foreign, or non-installed version")

    expected_sources: set[str] = set()
    for member in manifest["packageInventory"]:
        parts = pathlib.PurePosixPath(member["path"]).parts
        if len(parts) >= 3 and parts[:2] == ("usr", "src") and parts[2].startswith(f"{PACKAGE_NAME}-"):
            expected_sources.add(str(root_path(root, "/" + "/".join(parts[:3]))))
    require(bool(expected_sources), "manifest package inventory does not contain the DKMS source destination")
    require(set(inventory["sourceTrees"]) == expected_sources, "installed DKMS source destinations contain missing or foreign state")

    expected_overlays = {
        str(root_path(root, "/" + member["path"]))
        for member in manifest["packageInventory"]
        if member["path"].endswith(("/rp1-gpclk-gpio4.dtbo", "/rp1-gpclk-gpio20.dtbo"))
    }
    require(set(inventory["installedOverlays"]) == expected_overlays, "installed overlay inventory contains missing or foreign state")
    for member in manifest["packageInventory"]:
        installed = installed_member(root, member["path"], member["type"])
        kind = member["type"]
        if kind == "file":
            require(sha256_file(installed) == member["sha256"], f"installed package file hash differs: /{member['path']}")
        elif kind == "directory":
            pass
        else:
            require(sha256_bytes(os.readlink(installed).encode()) == member["sha256"], f"installed package symlink differs: /{member['path']}")
        if kind != "symlink":
            actual_mode = f"0{stat.S_IMODE(installed.stat().st_mode):03o}"
            require(actual_mode == member["mode"], f"installed package member mode differs: /{member['path']}")
    version = runner.run(["modinfo", "-k", platform.release(), "-F", "version", MODULE_NAME]).stdout.strip()
    require(version == manifest["productVersion"], "installed module metadata version differs from manifest")


def apply_release(state_dir: pathlib.Path, resolved: Mapping[str, Any], record: pathlib.Path, root: pathlib.Path, runner: Runner) -> None:
    manifest = resolved["manifest"]
    package_path = pathlib.Path(str(resolved["packagePath"]))
    require(package_path.parent == state_dir and package_path.is_file() and not package_path.is_symlink(), "prepared package was replaced before installation")
    validate_package(package_path, manifest, runner)
    inventory = existing_inventory(root, runner)
    owned, owned_identity, _ = load_ownership_record(record)
    owned_runtime = bool(
        owned is not None and owned_identity is not None
        and owned.get("schema") == RUNTIME_RECORD_SCHEMA
        and owned.get("channel") == "release"
        and owned.get("sourceCommit") == resolved.get("commit")
    )
    if owned_runtime:
        require(owned.get("tag") == resolved.get("tag") and owned.get("manifest") == manifest,
                "existing WsprryPi-owned runtime release identity differs from the selected source")
    if not owned_runtime:
        require_no_runtime_residue(inventory, "release installation")
    require(not inventory["activeModule"], "an active RP1 GPCLK module blocks ordinary installation")
    require(not inventory["configuredRoute"], "a configured RP1 GPCLK route blocks ordinary installation")
    require(not inventory["enrollment"] and not inventory["developmentManager"], "development enrollment or manager binding blocks release installation")
    expected = manifest["debianVersion"]
    installed_by_wsprrypi = False
    if inventory["packageVersion"] is None:
        require(
            not inventory["dkms"] and not inventory["sourceTrees"]
            and not inventory["moduleCandidates"] and not inventory.get("controllerCandidates", [])
            and not inventory["installedOverlays"],
            "foreign or mixed RP1 GPCLK installation blocks package installation",
        )
        ensure_new_ownership_record(record)
        runner.run(
            ["apt-get", "install", "--no-install-recommends", "-y", str(package_path)],
            passthrough=True,
        )
        installed_by_wsprrypi = True
    elif inventory["packageVersion"] == expected:
        pass
    else:
        raise ContractError(f"existing RP1-GPCLK-DKMS {inventory['packageVersion']} requires its owning package migration workflow; automatic replacement refused")
    verify_installed_release(root, manifest, runner, allow_owned_runtime=owned_runtime)
    if installed_by_wsprrypi:
        artifacts = module_artifacts(root)
        require(bool(artifacts), "installed release lacks attributable DKMS module artifacts")
        atomic_json_new(record, {
            "schema": RECORD_SCHEMA, "repository": REPOSITORY, "owner": "WsprryPi",
            "channel": "release", "installationMethod": "debian-package",
            "tag": resolved["tag"], "sourceCommit": resolved["commit"],
            "manifest": manifest, "moduleArtifacts": artifacts,
            "routeActivation": "disabled", "output": "disabled",
            "qualificationClaim": False,
        })


def verify_development_result(
    manifest_path: pathlib.Path,
    resolved: Mapping[str, Any],
) -> dict[str, Any]:
    require(manifest_path.is_file() and not manifest_path.is_symlink(), "upstream development result manifest is missing or substituted")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ContractError("upstream development result manifest is unreadable") from error
    require(isinstance(manifest, dict), "upstream development result manifest is not an object")
    expected = {
        "schema": "rp1-gpclk-source-development-manifest",
        "classification": "source-development",
        "qualification": False,
        "releaseQualified": False,
        "sourceCommit": resolved["commit"],
        "sourceState": "clean",
        "renderedVersion": resolved["version"],
        "dkmsName": DKMS_NAME,
        "moduleName": MODULE_NAME,
        "targetKernel": platform.release(),
    }
    for key, value in expected.items():
        require(manifest.get(key) == value, f"upstream development result has incompatible {key}")
    require(manifest.get("installationMode") == "route-neutral", "upstream development result is not route-neutral")
    version_identity = manifest.get("versionIdentity")
    require(
        isinstance(version_identity, dict)
        and version_identity.get("path") == resolved["versionSource"]
        and version_identity.get("sha256") == resolved["versionSourceSha256"]
        and version_identity.get("moduleVersion") == resolved["version"],
        "upstream development result canonical version identity differs from the selected source",
    )
    require(manifest.get("route") is None, "upstream development result selected a GPIO route")
    safety = manifest.get("routeNeutralSafety")
    require(isinstance(safety, dict) and set(safety) == {"before", "after"}, "upstream development result lacks route-neutral safety observations")
    for stage in ("before", "after"):
        observation = safety.get(stage)
        require(isinstance(observation, dict) and observation, f"upstream development result has invalid {stage} route-neutral observation")
        require(not any(bool(value) for value in observation.values()), f"upstream development result reports a {stage} route-neutral blocker")
    uapi = manifest.get("uapiIdentity")
    require(isinstance(uapi, dict) and uapi.get("sha256") == resolved["uapiSha256"], "upstream development result UAPI differs from the selected source")
    parameters = manifest.get("parameters")
    require(isinstance(parameters, dict) and parameters.get("output_inhibit") == 1,
            "upstream route-neutral development result did not retain installation output inhibition")
    installed = manifest.get("installedModule")
    require(isinstance(installed, dict), "upstream development result lacks installed module identity")
    require(installed.get("moduleName") == MODULE_NAME and installed.get("moduleVersion") == resolved["version"], "upstream installed module identity differs from the selected source")
    require(installed.get("kernel") == platform.release(), "upstream installed module targets a different kernel")
    for field in ("installedFileSha256", "decompressedElfSha256"):
        require(isinstance(installed.get(field), str) and SHA256.fullmatch(installed[field]), f"upstream installed module lacks valid {field}")
    require(manifest.get("buildProfile") == "runtime-controller",
            "upstream development result is not the DKMS runtime-controller profile")
    installed_modules = manifest.get("installedModules")
    require(isinstance(installed_modules, dict)
            and set(installed_modules) == {MODULE_NAME, CONTROLLER_MODULE_NAME},
            "upstream development result lacks the exact runtime module set")
    for name, module in installed_modules.items():
        require(isinstance(module, dict) and module.get("moduleName") == name
                and module.get("moduleVersion") == resolved["version"]
                and module.get("kernel") == platform.release(),
                f"upstream installed {name} identity differs from selected source")
        require(isinstance(module.get("installedPath"), str)
                and isinstance(module.get("installedFileSha256"), str)
                and SHA256.fullmatch(module["installedFileSha256"])
                and isinstance(module.get("decompressedElfSha256"), str)
                and SHA256.fullmatch(module["decompressedElfSha256"]),
                f"upstream installed {name} artifact identity is invalid")
    require(installed == installed_modules[MODULE_NAME],
            "upstream canonical consumer identity differs from runtime module set")
    return manifest


def validate_runtime_development_provider(
    record: Mapping[str, Any], inventory: Mapping[str, Any], runner: Runner
) -> None:
    version = record["productVersion"]
    require(inventory["packageVersion"] is None, "a Debian-owned RP1 provider blocks runtime development reuse")
    require(not inventory["activeModule"], "the transmission consumer is active during neutral runtime reuse")
    # A WsprryPi-owned neutral runtime deliberately keeps the administration
    # controller loaded while the transmission consumer remains absent.  The
    # caller admits that state only for a v3 runtime ownership record; runtime
    # activation subsequently revalidates the bound controller and artifacts.
    require(not inventory["configuredRoute"], "a configured RP1 GPCLK route blocks neutral runtime reuse")
    require(not inventory["enrollment"] and not inventory["developmentManager"], "legacy development administration blocks neutral runtime reuse")
    lines = [line for line in inventory["dkms"].splitlines() if line.strip()]
    expected_dkms = re.compile(rf"^{re.escape(DKMS_NAME)}/{re.escape(version)}[^\n]*installed$")
    require(bool(lines) and all(expected_dkms.fullmatch(line) for line in lines), "runtime development DKMS registration differs from ownership")
    require(inventory["sourceTrees"] == [f"/usr/src/{PACKAGE_NAME}-{version}"], "runtime development source destination differs from ownership")
    require(not inventory["installedOverlays"], "packaged route overlays block neutral runtime reuse")
    require(len(inventory["moduleCandidates"]) == 1,
            "runtime consumer module destination differs from ownership")
    require(len(inventory.get("controllerCandidates", [])) == 1,
            "runtime controller module destination differs from ownership")
    require("Differences between built and installed modules" not in inventory["dkms"],
            "DKMS reports installed module drift")
    installed_version = runner.run(["modinfo", "-k", platform.release(), "-F", "version", MODULE_NAME]).stdout.strip()
    require(installed_version == version, "runtime consumer module version differs from ownership")


def validate_resumable_neutral_activation(
    record: Mapping[str, Any], inventory: Mapping[str, Any], runner: Runner
) -> dict[str, Any]:
    """Admit only the exact fail-closed checkpoint before v3 record promotion."""
    validate_development_rollback_authority(record, pathlib.Path("/"))
    require(inventory.get("activeController") is True,
            "interrupted runtime activation lacks an active controller")
    inspected = validate_readiness(
        runtime_call(runner, RUNTIME_PROVIDER, "inspect", (),
                     {"neutral_ready", "conflict"})
    )
    binding = inspected.get("identities", {}).get("installedBinding", {})
    value = binding.get("value", {}) if isinstance(binding, dict) else {}
    expected_compatibility = {
        "gpio4": f"v{record['productVersion']}-rp1-gpio4",
        "gpio20": f"v{record['productVersion']}-rp1-gpio20",
    }
    require(
        binding.get("status") == "valid"
        and isinstance(binding.get("sha256"), str)
        and SHA256.fullmatch(binding["sha256"])
        and value.get("sourceCommit") == record["sourceCommit"]
        and value.get("productVersion") == record["productVersion"]
        and value.get("kernel") == platform.release()
        and value.get("compatibilityIdentities") == expected_compatibility,
        "interrupted runtime activation binding differs from provider ownership",
    )
    artifacts = inspected.get("artifacts")
    routes = inspected.get("routes")
    modules = inspected.get("modules", {})
    endpoints = inspected.get("endpoints", {})
    activation = inspected.get("activation", {}).get("value", {})
    controller = activation.get("controllerState", {})
    manager = inspected.get("manager", {}).get("query", {}).get("state", {})
    journal = inspected.get("journals", {}).get("activation.json", {})
    safety = inspected.get("safety", {})
    result = inspected.get("result")
    require(
        ((result == "neutral_ready"
          and inspected.get("administrationEligible") is True)
         or (result == "conflict"
             and inspected.get("administrationEligible") is False))
        and inspected.get("routeSelected") is False
        and inspected.get("executionReady") is False
        and isinstance(artifacts, dict) and artifacts
        and all(isinstance(item, dict) and item.get("status") == "exact"
                for item in artifacts.values())
        and isinstance(routes, dict)
        and set(routes) == {"requested", "configured", "persisted", "active"}
        and all(routes[name] is None for name in routes)
        and modules.get(CONTROLLER_MODULE_NAME, {}).get("status") == "loaded"
        and modules.get(MODULE_NAME, {}).get("status") == "absent"
        and endpoints.get("controller", {}).get("status") == "owned"
        and endpoints.get("controller", {}).get("open") is False
        and endpoints.get("consumer", {}).get("status") == "absent"
        and endpoints.get("consumer", {}).get("open") is False
        and isinstance(controller, dict)
        and type(controller.get("session")) is int
        and controller["session"] > 0
        and not any(controller.get(name) for name in
                    ("generation", "id", "error", "route", "flags"))
        and isinstance(manager, dict)
        and manager.get("activeRoute") is None
        and manager.get("configuredRoute") is None
        and manager.get("outputEnabled") is False
        and journal.get("status") == "present"
        and journal.get("value", {}).get("phase") == "complete-neutral"
        and isinstance(journal.get("value", {}).get("requestId"), str)
        and len(journal["value"]["requestId"]) >= 8
        and isinstance(journal.get("value", {}).get("planSha256"), str)
        and SHA256.fullmatch(journal["value"]["planSha256"])
        and isinstance(activation.get("artifactSetSha256"), str)
        and SHA256.fullmatch(activation["artifactSetSha256"])
        and activation.get("artifactSetSha256") == value.get("artifactSetSha256")
        and isinstance(activation.get("lastDeploymentSha256"), str)
        and SHA256.fullmatch(activation["lastDeploymentSha256"]),
        "interrupted runtime activation is not exact neutral state",
    )
    if result == "conflict":
        require(
            inspected.get("conflicts") ==
                ["loaded-controller-without-completed-activation"],
            "interrupted runtime activation has a non-resumable conflict",
        )
        validate_runtime_update_retry_conflict(inspected, journal["value"])
    else:
        require(
            not inspected.get("conflicts")
            and safety.get("clock") == "quiescent"
            and safety.get("dma") == "quiescent"
            and safety.get("gpio") == "quiescent"
            and safety.get("endpointOpen") is False
            and safety.get("owner") is False
            and safety.get("lease") is False,
            "interrupted runtime activation lacks quiescent safety evidence",
        )
    return {
        "readinessContract": RUNTIME_READINESS_CONTRACT,
        "bindingSha256": binding["sha256"],
        "artifactSetSha256": activation["artifactSetSha256"],
        "sourceCommit": record["sourceCommit"],
        "productVersion": record["productVersion"],
        "targetKernel": platform.release(),
        "compatibilityIdentities": expected_compatibility,
        "deploymentPlanSha256": activation["lastDeploymentSha256"],
        "activationPlanSha256": journal["value"]["planSha256"],
        "activationRequestId": journal["value"]["requestId"],
        "controllerSession": controller["session"],
        "controllerGeneration": controller["generation"],
        "state": "neutral_ready", "route": None, "output": "disabled",
    }


def recover_and_remove_owned_runtime(
    record_path: pathlib.Path,
    record: Mapping[str, Any],
    record_identity: os.stat_result,
    runner: Runner,
    migration_provider: pathlib.Path | None = None,
) -> None:
    # The installed runtime first interprets its own binding. A reviewed
    # candidate may become authority only for the exact cross-boot retirement
    # case that an older installed provider cannot represent.
    provider = RUNTIME_PROVIDER
    require(provider.is_file() and not provider.is_symlink(),
            "runtime provider is missing or substituted")
    inspected = validate_readiness(runtime_call(
        runner, provider, "inspect", (),
        {"activation_required", "recovery_required", "neutral_ready"},
    ))
    binding = inspected.get("identities", {}).get("installedBinding", {})
    binding_value = binding.get("value", {}) if isinstance(binding, dict) else {}
    require(
        binding.get("status") == "valid"
        and binding_value.get("sourceCommit") == record.get("sourceCommit"),
        "installed runtime binding differs from WsprryPi provider ownership",
    )
    if record.get("schema") == RUNTIME_RECORD_SCHEMA:
        runtime = validate_runtime_ownership(record.get("runtime"), record)
        require(
            binding.get("sha256") == runtime["bindingSha256"]
            and binding_value.get("artifactSetSha256") == runtime["artifactSetSha256"],
            "installed runtime binding differs from recorded runtime ownership",
        )
    artifacts = inspected.get("artifacts")
    require(
        isinstance(artifacts, dict) and artifacts
        and all(value.get("status") == "exact" for value in artifacts.values()),
        "installed runtime artifacts differ from the owned binding",
    )
    activation_journal = inspected.get("journals", {}).get("activation.json", {})
    if (inspected.get("result") in {"activation_required", "recovery_required"}
            and activation_journal.get("status") == "present"):
        activation_value = activation_journal.get("value", {})
        if activation_value.get("phase") == "recovered-inhibited":
            validate_inactive_runtime_update_state(inspected, recovered=True)
        else:
            require(
                activation_value.get("phase") == "complete-neutral"
                and inspected.get("reboot", {}).get("occurred") is True,
                "inactive runtime activation journal is neither recovered nor attributable to a prior boot",
            )
            if inspected.get("result") == "recovery_required":
                require(
                    migration_provider is not None
                    and migration_provider.is_file()
                    and not migration_provider.is_symlink(),
                    "installed runtime requires a reviewed cross-boot migration provider",
                )
                provider = migration_provider
                inspected = validate_readiness(runtime_call(
                    runner, provider, "inspect", (), {"recovery_required"},
                ), "recovery_required")
                candidate_binding = inspected.get("identities", {}).get(
                    "installedBinding", {})
                require(
                    candidate_binding.get("status") == "valid"
                    and candidate_binding.get("sha256") == binding.get("sha256")
                    and candidate_binding.get("value", {}).get("sourceCommit")
                        == record.get("sourceCommit"),
                    "migration provider interpreted a different installed runtime binding",
                )
            retirement = runtime_call(runner, provider, "activation-retire-plan")
            require(
                retirement.get("contract") == RUNTIME_READINESS_CONTRACT
                and retirement.get("operation") == "activation-retire-plan",
                "post-reboot activation retirement plan identity differs",
            )
            retirement_digest = retirement.get("planSha256")
            retirement_plan = retirement.get("plan", {})
            require(
                isinstance(retirement_digest, str)
                and SHA256.fullmatch(retirement_digest)
                and retirement_digest == sha256_bytes(canonical(retirement_plan))
                and retirement_plan.get("bindingSha256") == binding.get("sha256")
                and retirement_plan.get("artifactSetSha256")
                    == binding_value.get("artifactSetSha256"),
                "post-reboot activation retirement plan lacks owned identities",
            )
            transaction_digests = retirement_plan.get("transactionJournalSha256")
            if retirement_plan.get("version") == 2:
                idle_digest = retirement_plan.get("applicationIdleSha256")
                require(
                    isinstance(transaction_digests, dict)
                    and set(transaction_digests)
                        == {"transaction.json", "manager.json", "application.json"}
                    and all(value is None or
                            (isinstance(value, str) and SHA256.fullmatch(value))
                            for value in transaction_digests.values()),
                    "post-reboot activation retirement plan lacks fixed transaction identities",
                )
                require(
                    idle_digest is None or
                    (isinstance(idle_digest, str) and SHA256.fullmatch(idle_digest)),
                    "post-reboot activation retirement plan has an invalid idle-override identity",
                )
            preserve_owned_activation_journal(record, "before-retirement")
            require_unchanged_ownership(
                record_path, record, record_identity,
                "post-reboot activation retirement",
            )
            retired = runtime_call(
                runner, provider, "activation-retire",
                ["--plan-sha256", retirement_digest],
            )
            require(
                retired.get("contract") == RUNTIME_READINESS_CONTRACT
                and retired.get("operation") == "activation-retire"
                and retired.get("planSha256") == retirement_digest
                and retired.get("response", {}).get("status")
                    == "retired-post-reboot-activation"
                and retired.get("response", {}).get("activationJournalSha256")
                    == retirement_plan.get("activationJournalSha256"),
                "post-reboot activation retirement response differs from the reviewed plan",
            )
            inspected = validate_readiness(runtime_call(
                runner, provider, "inspect", (), {"activation_required"},
            ), "activation_required")
            require(
                inspected.get("journals", {}).get("activation.json", {}).get("status")
                    == "absent",
                "post-reboot activation retirement left its journal present",
            )
    if inspected.get("result") in {"recovery_required", "neutral_ready"}:
        preserve_owned_activation_journal(record, "before-recovery")
        recovery = runtime_call(runner, provider, "activation-recover-plan")
        require(
            recovery.get("contract") == RUNTIME_READINESS_CONTRACT
            and recovery.get("operation") == "activation-recover-plan",
            "runtime activation recovery plan identity differs",
        )
        recovery_digest = recovery.get("planSha256")
        recovery_plan = recovery.get("plan", {})
        require(
            isinstance(recovery_digest, str) and SHA256.fullmatch(recovery_digest)
            and recovery_digest == sha256_bytes(canonical(recovery_plan))
            and recovery_plan.get("bindingSha256") == binding.get("sha256"),
            "runtime activation recovery plan lacks the owned binding digest",
        )
        require_unchanged_ownership(
            record_path, record, record_identity, "runtime activation recovery"
        )
        recovered = runtime_call(
            runner, provider, "activation-recover",
            ["--plan-sha256", recovery_digest],
        )
        require(
            recovered.get("contract") == RUNTIME_READINESS_CONTRACT
            and recovered.get("operation") == "activation-recover"
            and recovered.get("planSha256") == recovery_digest
            and recovered.get("response", {}).get("status")
                in {"recovered-inhibited", "idempotent-no-change"},
            "runtime activation recovery response differs from the reviewed plan",
        )
        preserve_owned_activation_journal(record, "after-recovery", clear=True)
        inspected = validate_readiness(runtime_call(
            runner, provider, "inspect", (), {"activation_required"},
        ), "activation_required")
    modules = inspected.get("modules")
    endpoints = inspected.get("endpoints")
    require(
        inspected.get("result") == "activation_required"
        and not inspected.get("routeSelected")
        and isinstance(modules, dict)
        and set(modules) == {"rp1_route_controller", "rp1_gpclk_dkms"}
        and all(isinstance(value, dict) and value.get("status") == "absent"
                for value in modules.values())
        and isinstance(endpoints, dict)
        and set(endpoints) == {"controller", "consumer"}
        and all(isinstance(value, dict) and value.get("status") == "absent"
                for value in endpoints.values())
        and inspected.get("managerSocket", {}).get("status") == "absent",
        "recovered runtime is not eligible for reviewed deployment removal",
    )
    planned = runtime_call(runner, provider, "remove-plan")
    require(
        planned.get("contract") == RUNTIME_READINESS_CONTRACT
        and planned.get("operation") == "remove-plan",
        "runtime deployment removal plan identity differs",
    )
    digest = planned.get("planSha256")
    require(isinstance(digest, str) and SHA256.fullmatch(digest),
            "runtime deployment removal plan lacks a reviewed digest")
    destinations = planned.get("destinations")
    binding_files = binding_value.get("files")
    require(isinstance(binding_files, dict),
            "installed runtime binding file inventory is invalid")
    require(isinstance(destinations, list)
            and set(destinations) == runtime_deployment_destinations(binding_files),
            "runtime deployment removal inventory differs")
    require_unchanged_ownership(
        record_path, record, record_identity, "runtime deployment removal"
    )
    removed = runtime_call(
        runner, provider, "remove", ["--plan-sha256", digest]
    )
    require(
        removed.get("contract") == RUNTIME_READINESS_CONTRACT
        and removed.get("operation") == "remove"
        and removed.get("planSha256") == digest
        and removed.get("response", {}).get("status") == "removed-exact-deployment",
        "runtime deployment removal response differs from the reviewed plan",
    )
    require(not runtime_residue_inventory(pathlib.Path("/")),
            "reviewed runtime deployment removal left residue")


def apply_development(resolved: Mapping[str, Any], record: pathlib.Path, runner: Runner) -> None:
    source = revalidate_checkout(resolved["checkout"], runner)
    interface, route_args = route_neutral_interface(source, runner)
    require(interface == resolved["interface"] and route_args == resolved["routeArguments"], "upstream development interface changed after preflight")
    before = existing_inventory(pathlib.Path("/"), runner)
    require(not before["configuredRoute"], "a configured RP1 GPCLK route blocks exact-source development installation")
    existing_record, record_identity, record_reason = load_ownership_record(record)
    owned_runtime = bool(
        existing_record is not None and record_identity is not None
        and existing_record.get("schema") == RUNTIME_RECORD_SCHEMA
    )
    owned_provider = bool(
        existing_record is not None and record_identity is not None
        and existing_record.get("schema") == RECORD_SCHEMA
    )
    require(
        not before.get("activeController", False) or owned_runtime or owned_provider,
        "an active RP1 route controller blocks exact-source development installation",
    )
    if not (owned_runtime or owned_provider):
        require_no_runtime_residue(before, "source-development installation")
    provider_present = bool(
        before["packageVersion"] is not None
        or before["dkms"]
        or before["sourceTrees"]
        or before["moduleCandidates"]
        or before.get("controllerCandidates", [])
        or before["installedOverlays"]
        or before["enrollment"]
        or before["developmentManager"]
    )
    if provider_present and existing_record is not None and record_identity is not None:
        require(existing_record["channel"] == "development", "an existing WsprryPi-owned release provider requires its owning migration workflow")
        expected_identity = {
            "sourceCommit": resolved["commit"],
            "productVersion": resolved["version"],
            "sourceTree": resolved["sourceTree"],
            "uapiSha256": resolved["uapiSha256"],
            "versionSource": resolved["versionSource"],
            "versionSourceSha256": resolved["versionSourceSha256"],
            "targetKernel": platform.release(),
            "compatibilityIdentity": COMPATIBILITY_IDENTITY,
        }
        identity_matches = all(
            existing_record.get(name) == value
            for name, value in expected_identity.items()
        )
        profile_migration_required = bool(
            owned_runtime and (
                len(before["moduleCandidates"]) != 1
                or len(before.get("controllerCandidates", [])) != 1
                or "Differences between built and installed modules" in before["dkms"]
            )
        )
        migration_required = not identity_matches or profile_migration_required
        recoverable_runtime = bool(
            migration_required
            and before["runtimeResidue"]
            and existing_record.get("schema")
                in {RECORD_SCHEMA, RUNTIME_RECORD_SCHEMA}
        )
        require(
            not before["activeModule"] or recoverable_runtime,
            "an active RP1 GPCLK module blocks exact-source development installation",
        )
        require(
            not before.get("activeController", False)
            or owned_runtime
            or identity_matches,
            "an active RP1 route controller blocks exact-source development installation",
        )
        if (migration_required
                and existing_record.get("schema") in {RECORD_SCHEMA, RUNTIME_RECORD_SCHEMA}):
            if before["runtimeResidue"]:
                entrypoint, rollback = validate_development_rollback_authority(
                    existing_record, pathlib.Path("/")
                )
                recover_and_remove_owned_runtime(
                    record, existing_record, record_identity, runner,
                    source / "scripts/runtime_provider.py",
                )
                verified_entrypoint, verified_rollback = validate_development_removal(
                    existing_record, pathlib.Path("/"), runner
                )
                require(
                    (verified_entrypoint, verified_rollback) == (entrypoint, rollback),
                    "development rollback authority changed after runtime removal",
                )
            else:
                entrypoint, rollback = validate_development_removal(
                    existing_record, pathlib.Path("/"), runner
                )
            current_record, current_identity, current_reason = load_ownership_record(record)
            require(
                current_record == existing_record
                and current_identity is not None
                and (current_identity.st_dev, current_identity.st_ino)
                == (record_identity.st_dev, record_identity.st_ino),
                f"installation ownership changed before partial-provider migration: {current_reason or 'identity mismatch'}",
            )
            runner.run([str(entrypoint), "--record", str(rollback)],
                       cwd=entrypoint.parents[1], passthrough=True)
            verify_provider_absent(pathlib.Path("/"), runner)
            remove_ownership_record(record, record_identity)
            return apply_development(resolved, record, runner)
        require(identity_matches,
                "existing WsprryPi-owned development provider identity differs from the selected source")
        checkpoint_runtime = None
        if owned_runtime:
            validate_runtime_development_provider(existing_record, before, runner)
        elif before.get("activeController", False):
            checkpoint_runtime = validate_resumable_neutral_activation(
                existing_record, before, runner
            )
        elif before["runtimeResidue"]:
            validate_development_rollback_authority(
                existing_record, pathlib.Path("/")
            )
        else:
            validate_development_removal(existing_record, pathlib.Path("/"), runner)
        current_record, current_identity, current_reason = load_ownership_record(record)
        require(
            current_record == existing_record
            and current_identity is not None
            and (current_identity.st_dev, current_identity.st_ino)
            == (record_identity.st_dev, record_identity.st_ino),
            f"installation ownership changed during exact-provider verification: {current_reason or 'identity mismatch'}",
        )
        # Recheck the provider after the record observation so a change during
        # the first verification cannot be reported as an exact no-op.
        if owned_runtime:
            validate_runtime_development_provider(current_record, existing_inventory(pathlib.Path("/"), runner), runner)
        elif before.get("activeController", False):
            rechecked_runtime = validate_resumable_neutral_activation(
                current_record, existing_inventory(pathlib.Path("/"), runner), runner
            )
            require(
                rechecked_runtime == checkpoint_runtime,
                "interrupted runtime activation changed during verification",
            )
            replacement = copy.deepcopy(current_record)
            replacement["schema"] = RUNTIME_RECORD_SCHEMA
            replacement["runtime"] = rechecked_runtime
            validate_runtime_ownership(rechecked_runtime, replacement)
            replace_owned_record(record, current_record, current_identity, replacement)
        elif before["runtimeResidue"]:
            validate_development_rollback_authority(
                current_record, pathlib.Path("/")
            )
        else:
            validate_development_removal(current_record, pathlib.Path("/"), runner)
        print("Exact WsprryPi-owned development provider already installed; no provider mutation performed.")
        return
    require(not before["activeModule"], "an active RP1 GPCLK module blocks exact-source development installation")
    if provider_present and (existing_record is None or record_identity is None):
        require(False, f"an existing provider is not adoptable: {record_reason or 'WsprryPi ownership is unproven'}")
    if (not provider_present and existing_record is not None
            and record_identity is not None):
        # Resume an interrupted owned migration that completed provider
        # rollback but stopped before retiring the ownership record.
        verify_provider_absent(pathlib.Path("/"), runner)
        remove_ownership_record(record, record_identity)
        return apply_development(resolved, record, runner)
    require(
        before["packageVersion"] is None
        and not before["dkms"]
        and not before["sourceTrees"]
        and not before["moduleCandidates"]
        and not before.get("controllerCandidates", [])
        and not before["installedOverlays"]
        and not before["enrollment"]
        and not before["developmentManager"],
        "an existing packaged, development, foreign, or mixed RP1 GPCLK installation requires its owning migration workflow",
    )
    ensure_new_ownership_record(record)
    evidence = record.parent / f"rp1-gpclk-dkms-development-evidence.{secrets.token_hex(12)}"
    require(not evidence.exists() and not evidence.is_symlink(), "development evidence destination already exists")
    command = [
        str(source / "scripts" / "development-install"), "--source", str(source),
        "--kernel", platform.release(),
        *route_args, "--output-inhibit", "1", "--install", "--evidence-directory", str(evidence),
        "--runtime-controller",
    ]
    runner.run(command, cwd=source, passthrough=True)
    result = verify_development_result(evidence / "rendered-source" / "DEVELOPMENT_MANIFEST.json", resolved)
    after = existing_inventory(pathlib.Path("/"), runner)
    require(not after["activeModule"], "RP1 GPCLK module became active during development installation")
    require(not after.get("activeController", False), "RP1 route controller became active during development installation")
    require(not after["configuredRoute"], "RP1 GPCLK route became configured during development installation")
    installed = result["installedModule"]
    installed_modules = result["installedModules"]
    rollback_record = pathlib.Path(str(result.get("rollbackRecord", "")))
    expected_rollback = evidence / "ROLLBACK.json"
    require(rollback_record == expected_rollback and rollback_record.is_file() and not rollback_record.is_symlink(), "upstream development rollback record is missing or out of scope")
    rollback_entrypoint = evidence / "rendered-source" / "scripts" / "development-rollback"
    require(rollback_entrypoint.is_file() and not rollback_entrypoint.is_symlink(), "upstream development rollback entrypoint is missing or substituted")
    installed_path = pathlib.Path(str(installed.get("installedPath", "")))
    expected_prefix = pathlib.Path(f"/lib/modules/{platform.release()}/updates/dkms")
    require(installed_path.is_absolute() and installed_path.parent == expected_prefix, "upstream installed module path is outside the exact-kernel DKMS destination")
    controller_path = pathlib.Path(str(installed_modules[CONTROLLER_MODULE_NAME].get("installedPath", "")))
    require(controller_path.is_absolute() and controller_path.parent == expected_prefix,
            "upstream installed controller path is outside the exact-kernel DKMS destination")
    require(existing_inventory(pathlib.Path("/"), runner).get("controllerCandidates", []) == [str(controller_path)],
            "upstream runtime controller installation is not unique")
    atomic_json_new(record, {
        "schema": RECORD_SCHEMA, "repository": REPOSITORY, "owner": "WsprryPi",
        "channel": "development", "installationMethod": "upstream-development-rollback",
        "sourceCommit": resolved["commit"], "productVersion": resolved["version"],
        "sourceTree": resolved["sourceTree"], "uapiSha256": resolved["uapiSha256"],
        "versionSource": resolved["versionSource"],
        "versionSourceSha256": resolved["versionSourceSha256"],
        "installedModuleSha256": installed["installedFileSha256"],
        "decompressedModuleSha256": installed["decompressedElfSha256"],
        "installedModulePath": str(installed_path),
        "targetKernel": platform.release(),
        "compatibilityIdentity": COMPATIBILITY_IDENTITY, "routeActivation": "disabled",
        "output": "disabled", "qualificationClaim": False, "upstreamEvidence": str(evidence),
        "rollbackRecord": str(rollback_record), "rollbackRecordSha256": sha256_file(rollback_record),
        "rollbackEntrypoint": str(rollback_entrypoint),
        "rollbackEntrypointSha256": sha256_file(rollback_entrypoint),
    })


def require_hash(value: Any, field: str) -> str:
    require(isinstance(value, str) and SHA256.fullmatch(value) is not None, f"installation ownership record has invalid {field}")
    return value


def validate_recorded_module_artifacts(value: Any) -> list[dict[str, str]]:
    require(isinstance(value, list) and value, "installation ownership record lacks module artifacts")
    result: list[dict[str, str]] = []
    for item in value:
        require(isinstance(item, dict), "installation ownership module artifact is not an object")
        require_exact_keys(item, {"path", "sha256"}, "installation ownership module artifact")
        path = item["path"]
        require(
            isinstance(path, str)
            and re.fullmatch(r"/lib/modules/[^/]+/updates/dkms/rp1_gpclk_dkms\.ko(?:\.(?:xz|gz|zst|bz2))?", path) is not None,
            "installation ownership module artifact path is out of scope",
        )
        result.append({"path": path, "sha256": require_hash(item["sha256"], "module artifact SHA-256")})
    require(len({item["path"] for item in result}) == len(result), "installation ownership module artifacts contain duplicate paths")
    return sorted(result, key=lambda item: item["path"])


def validate_release_removal(record: Mapping[str, Any], root: pathlib.Path, runner: Runner) -> None:
    require(record["installationMethod"] == "debian-package", "release ownership installation method differs")
    tag = record["tag"]
    commit = record["sourceCommit"]
    require(isinstance(tag, str) and SEMVER.fullmatch(tag) is not None, "release ownership tag is invalid")
    require(isinstance(commit, str) and SHA40.fullmatch(commit) is not None, "release ownership commit is invalid")
    manifest = record["manifest"]
    require(isinstance(manifest, dict), "release ownership manifest is invalid")
    validated = validate_manifest(manifest, tag=tag, tag_commit=commit)
    verify_installed_release(root, validated, runner)
    expected = {item["path"]: item["sha256"] for item in validate_recorded_module_artifacts(record["moduleArtifacts"])}
    current = {item["path"]: item["sha256"] for item in module_artifacts(root)}
    require(all(current.get(path) == digest for path, digest in expected.items()), "recorded DKMS module artifacts differ from WsprryPi ownership")
    for path in current:
        match = re.fullmatch(r"/lib/modules/([^/]+)/updates/dkms/rp1_gpclk_dkms\.ko(?:\.(?:xz|gz|zst|bz2))?", path)
        require(match is not None, "installed DKMS module artifact path is out of scope")
        version = runner.run(["modinfo", "-k", match.group(1), "-F", "version", MODULE_NAME]).stdout.strip()
        require(version == validated["productVersion"], "additional-kernel DKMS module identity differs from WsprryPi ownership")


def secure_owned_path(path: pathlib.Path, parent: pathlib.Path, description: str) -> pathlib.Path:
    require(path.is_absolute(), f"{description} path is not absolute")
    absolute = pathlib.Path(os.path.abspath(path))
    parent_absolute = pathlib.Path(os.path.abspath(parent))
    require(absolute.is_relative_to(parent_absolute), f"{description} path is outside WsprryPi-owned evidence")
    try:
        resolved = absolute.resolve(strict=True)
    except OSError as error:
        raise ContractError(f"{description} is unavailable") from error
    require(resolved == absolute and resolved.is_file() and not resolved.is_symlink(), f"{description} is missing or substituted")
    info = resolved.stat()
    require(info.st_uid == os.geteuid(), f"{description} is not owned by the invoking user")
    require(stat.S_IMODE(info.st_mode) & 0o022 == 0, f"{description} is group- or world-writable")
    return resolved


def validate_development_rollback_authority(
    record: Mapping[str, Any], root: pathlib.Path
) -> tuple[pathlib.Path, pathlib.Path]:
    require(record["installationMethod"] == "upstream-development-rollback", "development ownership installation method differs")
    require(isinstance(record["sourceCommit"], str) and SHA40.fullmatch(record["sourceCommit"]) is not None, "development ownership commit is invalid")
    require(isinstance(record["productVersion"], str) and SAFE_VERSION.fullmatch(record["productVersion"]) is not None, "development ownership version is invalid")
    require(SHA40.fullmatch(record["sourceTree"]) is not None, "development ownership source tree is invalid")
    require(record["versionSource"] == "include/rp1_gpclk/version.h", "development ownership version source differs")
    require(record["compatibilityIdentity"] == COMPATIBILITY_IDENTITY, "development ownership compatibility identity differs")
    require(record["targetKernel"] == platform.release(), "development ownership targets a different running kernel")
    for field in (
        "uapiSha256", "versionSourceSha256", "rollbackRecordSha256",
        "rollbackEntrypointSha256", "installedModuleSha256", "decompressedModuleSha256",
    ):
        require_hash(record[field], field)
    evidence = pathlib.Path(record["upstreamEvidence"])
    expected_parent = pathlib.Path("/var/lib/wsprrypi")
    if root != pathlib.Path("/"):
        expected_parent = root_path(root, str(expected_parent))
    require(
        evidence.parent == expected_parent
        and re.fullmatch(r"rp1-gpclk-dkms-development-evidence\.[0-9a-f]{24}", evidence.name) is not None,
        "development ownership evidence path differs",
    )
    rollback = secure_owned_path(pathlib.Path(record["rollbackRecord"]), evidence, "development rollback record")
    entrypoint = secure_owned_path(pathlib.Path(record["rollbackEntrypoint"]), evidence, "development rollback entrypoint")
    require(rollback == evidence / "ROLLBACK.json", "development rollback record path differs")
    require(entrypoint == evidence / "rendered-source/scripts/development-rollback", "development rollback entrypoint path differs")
    require(sha256_file(rollback) == record["rollbackRecordSha256"], "development rollback record identity differs")
    require(sha256_file(entrypoint) == record["rollbackEntrypointSha256"], "development rollback entrypoint identity differs")
    return entrypoint, rollback


def validate_development_removal(
    record: Mapping[str, Any], root: pathlib.Path, runner: Runner
) -> tuple[pathlib.Path, pathlib.Path]:
    entrypoint, rollback = validate_development_rollback_authority(record, root)
    inventory = existing_inventory(root, runner)
    require_no_runtime_residue(inventory, "source-development removal")
    require(inventory["packageVersion"] is None, "a Debian-owned RP1 provider blocks development rollback")
    require(not inventory["activeModule"], "an active RP1 GPCLK module blocks owned provider removal")
    require(not inventory.get("activeController", False), "an active RP1 route controller blocks owned provider removal")
    require(not inventory["configuredRoute"], "a configured RP1 GPCLK route blocks owned provider removal")
    require(not inventory["enrollment"] and not inventory["developmentManager"], "development enrollment or manager binding blocks owned provider removal")
    version = record["productVersion"]
    dkms_lines = [line for line in inventory["dkms"].splitlines() if line.strip()]
    expected_dkms = re.compile(rf"^{re.escape(DKMS_NAME)}/{re.escape(version)}[^\n]*installed$")
    require(bool(dkms_lines) and all(expected_dkms.fullmatch(line) for line in dkms_lines), "development DKMS registration differs from WsprryPi ownership")
    require("Differences between built and installed modules" not in inventory["dkms"],
            "DKMS reports installed module drift")
    expected_source = str(root_path(root, f"/usr/src/{PACKAGE_NAME}-{version}"))
    require(inventory["sourceTrees"] == [expected_source], "development source destination differs from WsprryPi ownership")
    require(not inventory["installedOverlays"], "installed RP1 route overlays block development rollback")
    installed_path = pathlib.Path(record["installedModulePath"])
    expected_prefix = pathlib.Path(f"/lib/modules/{record['targetKernel']}/updates/dkms")
    require(installed_path.is_absolute() and installed_path.parent == expected_prefix, "development installed module path is out of scope")
    actual_installed = root_path(root, str(installed_path))
    require(inventory["moduleCandidates"] == [str(actual_installed)], "development module destination differs from WsprryPi ownership")
    require(actual_installed.is_file() and not actual_installed.is_symlink(), "development installed module is missing or substituted")
    require(sha256_file(actual_installed) == record["installedModuleSha256"], "development installed module identity differs")
    evidence = pathlib.Path(record["upstreamEvidence"])
    manifest_path = secure_owned_path(
        evidence / "rendered-source/DEVELOPMENT_MANIFEST.json", evidence,
        "development source manifest")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        rollback_value = json.loads(rollback.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ContractError("development module-set ownership evidence is unreadable") from error
    profile = rollback_value.get("buildProfile", "ordinary")
    require(profile in {"ordinary", "runtime-controller"},
            "development rollback build profile is invalid")
    if profile == "runtime-controller":
        installed_modules = rollback_value.get("installedModules")
        require(manifest.get("buildProfile") == "runtime-controller"
                and isinstance(installed_modules, dict)
                and set(installed_modules) == {MODULE_NAME, CONTROLLER_MODULE_NAME}
                and installed_modules == manifest.get("installedModules")
                and rollback_value.get("installedManifestSha256") == sha256_file(manifest_path),
                "development rollback does not own the runtime module pair")
        controller = installed_modules[CONTROLLER_MODULE_NAME]
        controller_path = pathlib.Path(controller.get("installedPath", ""))
        require(controller_path.is_absolute() and controller_path.parent == expected_prefix,
                "development controller path is out of scope")
        actual_controller = root_path(root, str(controller_path))
        require(inventory.get("controllerCandidates", []) == [str(actual_controller)]
                and actual_controller.is_file() and not actual_controller.is_symlink()
                and sha256_file(actual_controller) == controller.get("installedFileSha256"),
                "development controller identity differs from WsprryPi ownership")
    else:
        require(not inventory.get("controllerCandidates", []),
                "legacy development rollback does not own a controller module")
    return entrypoint, rollback


def verify_provider_absent(root: pathlib.Path, runner: Runner) -> None:
    inventory = existing_inventory(root, runner)
    require_no_runtime_residue(inventory, "provider absence verification")
    require(
        inventory["packageVersion"] is None
        and not inventory["dkms"]
        and not inventory["activeModule"]
        and not inventory.get("activeController", False)
        and not inventory["sourceTrees"]
        and not inventory["moduleCandidates"]
        and not inventory.get("controllerCandidates", [])
        and not inventory["installedOverlays"]
        and not inventory["configuredRoute"]
        and not inventory["enrollment"]
        and not inventory["developmentManager"],
        "provider removal completed with residual or active RP1 state",
    )


def remove_owned_provider(args: argparse.Namespace, runner: Runner) -> None:
    if args.remove == "false":
        print("RP1-GPCLK-DKMS preserved: explicit operator opt-out.")
        return
    record, identity, reason = load_ownership_record(args.record)
    if record is None or identity is None:
        try:
            root = args.root.resolve(strict=True)
            verify_provider_absent(root, runner)
        except (OSError, ContractError):
            print(f"RP1-GPCLK-DKMS preserved: {reason or 'WsprryPi ownership is unproven'}.")
        else:
            print("RP1-GPCLK-DKMS absent: no provider removal was required.")
        return
    try:
        root = args.root.resolve(strict=True)
    except OSError as error:
        raise ContractError("provider inventory root is unavailable") from error
    try:
        if record["schema"] == RUNTIME_RECORD_SCHEMA:
            inventory = existing_inventory(root, runner)
        else:
            inventory = {"runtimeResidue": []}
        if inventory["runtimeResidue"]:
            require(root == pathlib.Path("/"),
                    "runtime removal requires the production filesystem root")
            if record["channel"] == "development":
                validate_development_rollback_authority(record, root)
            recover_and_remove_owned_runtime(
                args.record, record, identity, runner)
        if record["channel"] == "release":
            validate_release_removal(record, root, runner)
            command = ["apt-get", "remove", "-y", PACKAGE_NAME]
            working_directory = None
        else:
            entrypoint, rollback = validate_development_removal(record, root, runner)
            command = [str(entrypoint), "--record", str(rollback)]
            working_directory = entrypoint.parents[1]
    except ContractError as error:
        print(f"RP1-GPCLK-DKMS preserved: installed state does not match WsprryPi ownership ({error}).")
        if args.remove == "true":
            raise ContractError(
                "explicitly requested owned-provider removal did not complete"
            ) from error
        return
    current_record, current_identity, current_reason = load_ownership_record(args.record)
    require(
        current_record == record
        and current_identity is not None
        and (current_identity.st_dev, current_identity.st_ino) == (identity.st_dev, identity.st_ino),
        f"installation ownership changed before provider removal: {current_reason or 'identity mismatch'}",
    )
    runner.run(command, cwd=working_directory, passthrough=True)
    verify_provider_absent(root, runner)
    remove_ownership_record(args.record, identity)
    print("WsprryPi-owned RP1-GPCLK-DKMS provider removed and absence verified.")


def load_plan(state_dir: pathlib.Path) -> dict[str, Any]:
    plan_path = state_dir / "plan.json"
    require(plan_path.is_file() and not plan_path.is_symlink(), "prepared plan is missing or substituted")
    try:
        plan = json.loads(plan_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ContractError("prepared plan is unreadable") from error
    require(isinstance(plan, dict) and plan.get("schema") == STATE_SCHEMA, "prepared plan schema is invalid")
    return plan


def runtime_product_version(resolved: Mapping[str, Any]) -> str:
    if resolved.get("channel") == "release":
        value = resolved.get("manifest", {}).get("productVersion")
    else:
        value = resolved.get("version")
    require(isinstance(value, str) and SAFE_VERSION.fullmatch(value), "resolved runtime product version is invalid")
    return value


def validate_runtime_bundle(
    bundle: pathlib.Path,
    resolved: Mapping[str, Any],
    companion: pathlib.Path = RUNTIME_COMPANION,
    module_root: pathlib.Path = pathlib.Path("/"),
) -> dict[str, Any]:
    require(bundle.is_absolute() and bundle.is_dir() and not bundle.is_symlink(), "runtime bundle directory is missing or substituted")
    info = bundle.stat()
    require(info.st_uid == os.geteuid() and stat.S_IMODE(info.st_mode) & 0o077 == 0, "runtime bundle directory is not private to the invoking user")
    binding_path = bundle / "binding.json"
    require(binding_path.is_file() and not binding_path.is_symlink(), "runtime bundle binding is missing or substituted")
    try:
        binding = json.loads(binding_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ContractError("runtime bundle binding is unreadable") from error
    required = {
        "schemaVersion", "contract", "productVersion", "compatibilityIdentities",
        "sourceCommit", "kernel", "files", "externalFiles", "modules", "uapiSha256",
        "controllerNoteSha256", "consumerNoteSha256", "artifactSetSha256",
    }
    require(isinstance(binding, dict), "runtime bundle binding is not an object")
    require_exact_keys(binding, required, "runtime bundle binding")
    product = runtime_product_version(resolved)
    compatibility = {
        "gpio4": f"v{product}-rp1-gpio4",
        "gpio20": f"v{product}-rp1-gpio20",
    }
    require(binding["schemaVersion"] == 3 and binding["contract"] == RUNTIME_BINDING_CONTRACT, "runtime bundle binding contract differs")
    require(binding["sourceCommit"] == resolved.get("commit"), "runtime bundle source commit differs from selected provider")
    require(binding["productVersion"] == product, "runtime bundle product version differs from selected provider")
    require(binding["kernel"] == platform.release(), "runtime bundle targets a different running kernel")
    require(binding["compatibilityIdentities"] == compatibility, "runtime bundle compatibility identities differ")
    files = binding["files"]
    require(isinstance(files, dict) and files, "runtime bundle file inventory is invalid")
    critical = {
        "/usr/lib/rp1-gpclk-dkms/runtime_activation.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_provider.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_deployment.py",
        "/usr/lib/rp1-gpclk-dkms/runtime_binding.py",
        "/usr/lib/rp1-gpclk-dkms/runtime-uapi/rp1_gpclk.h",
        "/usr/lib/rp1-gpclk-dkms/runtime-uapi/rp1_route_admin.h",
        "/usr/lib/rp1-gpclk-dkms/runtime-overlays/gpio4.dtbo",
        "/usr/lib/rp1-gpclk-dkms/runtime-overlays/gpio20.dtbo",
        "/usr/lib/rp1-gpclk-dkms/schema/rp1-gpclk-runtime-readiness-v1.schema.json",
        "/usr/lib/systemd/system/rp1-gpclk-route-manager.socket",
        "/usr/lib/systemd/system/rp1-gpclk-route-manager@.service",
        "/etc/systemd/system/rp1-gpclk-route-manager@.service.d/95-runtime-controller.conf",
    }
    require(critical <= set(files), "runtime bundle omits a required bound artifact")
    require(not any(path.startswith("/lib/modules/") for path in files),
            "runtime bundle attempts to deploy a DKMS-owned module")
    require(all(isinstance(path, str) and isinstance(value, str) and SHA256.fullmatch(value) for path, value in files.items()), "runtime bundle file digests are invalid")
    modules = binding["modules"]
    require(isinstance(modules, dict)
            and set(modules) == {CONTROLLER_MODULE_NAME, MODULE_NAME},
            "runtime bundle module prerequisite inventory differs")
    for name, note_field in ((CONTROLLER_MODULE_NAME, "controllerNoteSha256"),
                             (MODULE_NAME, "consumerNoteSha256")):
        module = modules[name]
        require(isinstance(module, dict)
                and set(module) == {"name", "path", "installedFileSha256",
                                    "decompressedElfSha256", "compression",
                                    "buildNoteSha256", "version", "kernel"},
                f"runtime bundle {name} identity is invalid")
        path = pathlib.Path(module["path"])
        require(module["name"] == name and path.is_absolute()
                and path.parent == pathlib.Path(
                    f"/lib/modules/{platform.release()}/updates/dkms")
                and re.fullmatch(rf"{re.escape(name)}\.ko(?:\.(?:xz|gz|zst|bz2))?",
                                 path.name) is not None,
                f"runtime bundle {name} path is invalid")
        actual_path = root_path(module_root, str(path))
        expected_compression = ("none" if path.name.endswith(".ko")
                                else path.suffix.removeprefix("."))
        require(actual_path.is_file() and not actual_path.is_symlink()
                and module["compression"] == expected_compression
                and sha256_file(actual_path) == module["installedFileSha256"]
                and decompressed_module_sha256(actual_path, module["compression"])
                    == module["decompressedElfSha256"],
                f"runtime bundle {name} installed artifact differs")
        require(module["version"] == product
                and module["kernel"] == platform.release()
                and module["buildNoteSha256"] == binding[note_field]
                and all(isinstance(module[field], str) and SHA256.fullmatch(module[field])
                        for field in ("installedFileSha256", "decompressedElfSha256",
                                      "buildNoteSha256")),
                f"runtime bundle {name} identity differs")
    external = binding["externalFiles"]
    expected_external = {str(companion)}
    require(isinstance(external, dict) and set(external) == expected_external, "runtime bundle external prerequisite inventory differs")
    require(companion.is_file() and not companion.is_symlink(), "installed WsprryPi route companion is missing or substituted")
    require(external[str(companion)] == sha256_file(companion), "runtime bundle WsprryPi companion identity differs")
    uapi = binding["uapiSha256"]
    require(isinstance(uapi, dict) and set(uapi) == {"consumer", "controller"}, "runtime bundle UAPI identity is invalid")
    require(
        uapi["consumer"] == files["/usr/lib/rp1-gpclk-dkms/runtime-uapi/rp1_gpclk.h"]
        and uapi["controller"] == files["/usr/lib/rp1-gpclk-dkms/runtime-uapi/rp1_route_admin.h"],
        "runtime bundle UAPI digests differ from bound artifacts",
    )
    for value in (
        *external.values(), *uapi.values(),
        binding["controllerNoteSha256"], binding["consumerNoteSha256"],
        binding["artifactSetSha256"],
    ):
        require(isinstance(value, str) and SHA256.fullmatch(value), "runtime bundle contains an invalid identity digest")
    identity = dict(binding)
    identity.pop("artifactSetSha256")
    require(sha256_bytes(canonical(identity)) == binding["artifactSetSha256"], "runtime bundle artifact-set digest differs")
    bootstrap = {
        "runtime_deployment.py", "runtime_controller_admin.py", "runtime_layout.py",
        "runtime_application.py", "runtime_output.py", "runtime_provider.py",
        "runtime_binding.py", "runtime_activation.py", "runtime_route_client.py",
    }
    expected_members = {"binding.json", *bootstrap}
    for destination, expected in files.items():
        member = bundle / (sha256_bytes(destination.encode()) + ".bin")
        require(member.is_file() and not member.is_symlink(), f"runtime bundle payload is missing: {destination}")
        require(sha256_file(member) == expected, f"runtime bundle payload digest differs: {destination}")
        expected_members.add(member.name)
    actual_members = {path.name for path in bundle.iterdir()}
    require(actual_members == expected_members, "runtime bundle contains an unsupported or missing member")
    for name in bootstrap:
        member = bundle / name
        require(member.is_file() and not member.is_symlink(), f"runtime bundle bootstrap is missing: {name}")
        destination = "/usr/lib/rp1-gpclk-dkms/" + name
        require(destination in files and sha256_file(member) == files[destination], f"runtime bundle bootstrap digest differs: {name}")
    validate_bootstrap_import_closure(bundle, bootstrap)
    return {
        "binding": binding,
        "bindingSha256": sha256_file(binding_path),
        "artifactSetSha256": binding["artifactSetSha256"],
    }


def validate_bootstrap_import_closure(bundle: pathlib.Path, bootstrap: set[str]) -> None:
    """Compile bootstrap sources and require every local runtime import."""
    available = {pathlib.Path(name).stem for name in bootstrap}
    for name in sorted(bootstrap):
        member = bundle / name
        try:
            source = member.read_text(encoding="utf-8")
            tree = ast.parse(source, filename=str(member))
        except (OSError, UnicodeDecodeError, SyntaxError) as error:
            raise ContractError(f"runtime bundle bootstrap is not valid Python: {name}") from error
        imports = set()
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                imports.update(alias.name.split(".", 1)[0] for alias in node.names)
            elif isinstance(node, ast.ImportFrom) and node.level == 0 and node.module:
                imports.add(node.module.split(".", 1)[0])
        missing = sorted(module for module in imports
                         if module.startswith("runtime_") and module not in available)
        require(not missing,
                f"runtime bundle bootstrap import closure is incomplete: {name} imports {', '.join(missing)}")


def build_runtime_bundle(
    state_dir: pathlib.Path,
    resolved: Mapping[str, Any],
    runner: Runner,
) -> tuple[pathlib.Path, dict[str, Any]]:
    checkout = resolved.get("checkout")
    require(isinstance(checkout, dict), "resolved runtime source checkout is missing")
    source = revalidate_checkout(checkout, runner)
    runtime_source_interface(source)
    bundle = state_dir / "runtime-bundle"
    require(not bundle.exists() and not bundle.is_symlink(), "runtime bundle destination already exists")
    module_directory = pathlib.Path(f"/lib/modules/{platform.release()}/updates/dkms")
    require(module_directory.is_dir() and not module_directory.is_symlink(),
            "DKMS runtime-profile module directory is unavailable")
    runner.run(
        ["python3", str(source / "scripts/build_runtime_bundle.py"),
         str(module_directory), str(bundle),
         "--application-companion", str(RUNTIME_COMPANION)],
        cwd=source,
        passthrough=True,
    )
    return bundle, validate_runtime_bundle(bundle, resolved)


def runtime_call(
    runner: Runner,
    provider: pathlib.Path,
    operation: str,
    arguments: Sequence[str] = (),
    allowed_results: Iterable[str] = (),
) -> dict[str, Any]:
    result = runner.run(["python3", str(provider), operation, *arguments], check=False)
    value = json_result(result, operation)
    if result.returncode != 0:
        classification = value.get("result")
        require(classification in set(allowed_results), f"RP1 runtime {operation} failed with {classification or 'unknown state'}")
    return value


def validate_readiness(value: Mapping[str, Any], expected_result: str | None = None) -> dict[str, Any]:
    require(value.get("contract") == RUNTIME_READINESS_CONTRACT, "runtime readiness contract differs")
    if expected_result is not None:
        require(value.get("result") == expected_result and value.get("state") == expected_result, f"runtime readiness did not reach {expected_result}")
    return dict(value)


def replace_owned_record(
    path: pathlib.Path,
    expected_value: Mapping[str, Any],
    expected_identity: os.stat_result,
    replacement: Mapping[str, Any],
) -> None:
    current, identity, reason = load_ownership_record(path)
    require(
        current == expected_value and identity is not None
        and (identity.st_dev, identity.st_ino) == (expected_identity.st_dev, expected_identity.st_ino),
        f"installation ownership changed before runtime evidence publication: {reason or 'identity mismatch'}",
    )
    atomic_json(path, replacement)


def require_unchanged_ownership(
    path: pathlib.Path,
    expected_value: Mapping[str, Any],
    expected_identity: os.stat_result,
    operation: str,
) -> None:
    current, identity, reason = load_ownership_record(path)
    require(
        current == expected_value and identity is not None
        and (identity.st_dev, identity.st_ino)
            == (expected_identity.st_dev, expected_identity.st_ino),
        f"installation ownership changed before {operation}: {reason or 'identity mismatch'}",
    )


def validate_runtime_update_identity(
    record: Mapping[str, Any], inspected: Mapping[str, Any]
) -> dict[str, Any]:
    """Bind a pre-update transition to the exact owned neutral runtime."""
    runtime = validate_runtime_ownership(record.get("runtime"), record)
    binding = inspected.get("identities", {}).get("installedBinding", {})
    binding_value = binding.get("value", {}) if isinstance(binding, dict) else {}
    require(
        binding.get("status") == "valid"
        and binding.get("sha256") == runtime["bindingSha256"]
        and binding_value.get("artifactSetSha256") == runtime["artifactSetSha256"]
        and binding_value.get("sourceCommit") == record.get("sourceCommit"),
        "installed runtime binding differs from WsprryPi ownership",
    )
    artifacts = inspected.get("artifacts")
    require(
        isinstance(artifacts, dict) and artifacts
        and all(value.get("status") == "exact" for value in artifacts.values()),
        "installed runtime artifacts differ from WsprryPi ownership",
    )
    routes = inspected.get("routes")
    require(
        isinstance(routes, dict)
        and set(routes) == {"requested", "configured", "persisted", "active"}
        and all(routes[name] is None for name in routes),
        "runtime update preparation found a selected GPIO route",
    )
    consumer = inspected.get("modules", {}).get(MODULE_NAME, {})
    consumer_endpoint = inspected.get("endpoints", {}).get("consumer", {})
    require(
        consumer.get("status") == "absent"
        and consumer_endpoint.get("status") == "absent"
        and consumer_endpoint.get("open") is False
        and inspected.get("routeSelected") is False
        and inspected.get("executionReady") is False,
        "runtime update preparation found transmission-capable state",
    )
    return runtime


def validate_inactive_runtime_update_state(
    inspected: Mapping[str, Any], *, recovered: bool
) -> None:
    require(
        inspected.get("result") == "activation_required"
        and inspected.get("state") == "activation_required"
        and not inspected.get("conflicts"),
        "runtime update preparation did not reach inactive activation-required state",
    )
    modules = inspected.get("modules")
    endpoints = inspected.get("endpoints")
    routes = inspected.get("routes")
    require(
        isinstance(modules, dict)
        and set(modules) == {MODULE_NAME, CONTROLLER_MODULE_NAME}
        and all(value.get("status") == "absent" for value in modules.values())
        and isinstance(endpoints, dict)
        and set(endpoints) == {"consumer", "controller"}
        and all(value.get("status") == "absent" for value in endpoints.values())
        and all(value.get("open") is False for value in endpoints.values()),
        "runtime update preparation left a module or endpoint active",
    )
    require(
        isinstance(routes, dict)
        and set(routes) == {"requested", "configured", "persisted", "active"}
        and all(routes[name] is None for name in routes)
        and inspected.get("routeSelected") is False
        and inspected.get("executionReady") is False,
        "runtime update preparation left a route or transmission eligibility",
    )
    if recovered:
        activation_observation = inspected.get("activation", {})
        activation = activation_observation.get("value", {})
        journal = inspected.get("journals", {}).get("activation.json", {})
        service = activation.get("applicationService", {})
        require(
            activation_observation.get("status") == "observed"
            and activation.get("inhibited") is True
            and journal.get("status") == "present"
            and journal.get("value", {}).get("phase") == "recovered-inhibited"
            and service.get("active") in {"inactive", "failed"}
            and service.get("MainPID") == "0",
            "runtime update recovery did not retain application inhibition",
        )


def validate_runtime_update_retry_conflict(
    inspected: Mapping[str, Any], journal_value: Mapping[str, Any]
) -> None:
    """Admit only same-boot application PID drift after neutral activation."""
    activation_observation = inspected.get("activation", {})
    activation = activation_observation.get("value", {})
    controller_state = activation.get("controllerState")
    plan = journal_value.get("plan", {})
    outcome = journal_value.get("application", {})
    prior_service = outcome.get("service", {}) if isinstance(outcome, dict) else {}
    current_service = activation.get("applicationService", {})
    socket = activation.get("socket", {})
    manager_socket = activation.get("managerSocket", {})
    manager_service = activation.get("managerService", {})
    manager = inspected.get("manager", {})
    query = manager.get("query", {}) if isinstance(manager, dict) else {}
    manager_state = query.get("state", {}) if isinstance(query, dict) else {}
    require(
        activation_observation.get("status") == "observed"
        and journal_value.get("controller") == controller_state
        and plan.get("bootId") == activation.get("bootId")
        and plan.get("bindingSha256") == activation.get("bindingSha256")
        and plan.get("artifactSetSha256") == activation.get("artifactSetSha256"),
        "runtime update retry conflict lacks matching activation provenance",
    )
    require(
        isinstance(outcome, dict)
        and outcome.get("phase") == "restored"
        and plan.get("application", {}).get("wasActive") is True
        and set(prior_service) == {"LoadState", "ActiveState", "UnitFileState", "MainPID"}
        and set(current_service) == {"load", "active", "enabled", "fragment", "MainPID"}
        and prior_service.get("LoadState") == "loaded"
        and prior_service.get("ActiveState") == "active"
        and prior_service.get("MainPID", "0") != "0"
        and current_service.get("load") == prior_service.get("LoadState")
        and current_service.get("active") == prior_service.get("ActiveState")
        and current_service.get("enabled") == prior_service.get("UnitFileState")
        and current_service.get("fragment") == "/etc/systemd/system/wsprrypi.service"
        and current_service.get("MainPID", "0") != "0"
        and current_service.get("MainPID") != prior_service.get("MainPID"),
        "runtime update preparation permits only application PID drift",
    )
    require(
        socket.get("active") == "active"
        and socket.get("fragment") ==
            "/usr/lib/systemd/system/rp1-gpclk-route-manager.socket"
        and manager_socket.get("status") == "owned"
        and manager_service.get("load") == "loaded"
        and manager_service.get("fragment") ==
            "/usr/lib/systemd/system/rp1-gpclk-route-manager@.service"
        and activation.get("inhibited") is False
        and manager.get("status") == "observed"
        and query.get("operation") == "query"
        and query.get("status") == "ok"
        and manager_state.get("bindingSha256") == activation.get("bindingSha256")
        and manager_state.get("bootId") == activation.get("bootId")
        and manager_state.get("controller") == controller_state
        and manager_state.get("activeRoute") is None
        and manager_state.get("configuredRoute") is None
        and manager_state.get("qualification") is False
        and manager_state.get("outputEnabled") is False
        and manager_state.get("applicationInhibited") is False
        and manager_state.get("pendingTransaction") is None
        and manager_state.get("application") is None
        and manager_state.get("applicationRestoration") is True,
        "runtime update retry conflict differs beyond application PID drift",
    )


def prepare_runtime_update(args: argparse.Namespace, runner: Runner) -> None:
    """Recover exact neutral administration before application replacement."""
    if args.command == "prepare-runtime-update":
        state_dir = secure_state_dir(args.state_dir)
        plan = load_plan(state_dir)
        require(not plan.get("dryRun"), "dry-run plan cannot prepare a runtime update")
        if not plan["decision"]["install"]:
            print("RP1-GPCLK-DKMS runtime update preparation skipped by resolved plan.")
            return
        resolved = plan.get("resolved")
        require(isinstance(resolved, dict), "prepared runtime source identity is missing")
    record, record_identity, reason = load_ownership_record(args.record)
    if args.command == "prepare-runtime-removal" and (
        record is None or record_identity is None
    ):
        print(f"No owned RP1 runtime requires pre-uninstall recovery: {reason or 'ownership is unproven'}.")
        return
    require(
        record is not None and record_identity is not None,
        f"WsprryPi provider ownership is required before runtime update preparation: {reason or 'unknown record'}",
    )
    if args.command == "prepare-runtime-update":
        require(
            record.get("sourceCommit") == resolved.get("commit"),
            "owned provider and runtime source commits differ",
        )
    if record.get("schema") != RUNTIME_RECORD_SCHEMA:
        print("No existing neutral runtime administration requires update preparation.")
        return

    inspected = validate_readiness(runtime_call(
        runner, RUNTIME_PROVIDER, "inspect", (),
        {"neutral_ready", "activation_required", "conflict"},
    ))
    runtime = validate_runtime_update_identity(record, inspected)
    result = inspected.get("result")
    journal = inspected.get("journals", {}).get("activation.json", {})
    journal_value = journal.get("value", {}) if isinstance(journal, dict) else {}

    if result == "activation_required":
        recovered = journal.get("status") == "present" and journal_value.get("phase") == "recovered-inhibited"
        post_reboot = (
            journal.get("status") == "present"
            and journal_value.get("phase") == "complete-neutral"
            and inspected.get("reboot", {}).get("occurred") is True
        )
        require(recovered or post_reboot,
                "owned runtime is inactive without recoverable update or post-reboot evidence")
        validate_inactive_runtime_update_state(inspected, recovered=recovered)
        print(
            "RP1-GPCLK-DKMS runtime already recovered for application update."
            if recovered else
            "RP1-GPCLK-DKMS post-reboot runtime remains eligible for fresh neutral activation."
        )
        return

    if result == "conflict":
        require(
            inspected.get("conflicts") == ["loaded-controller-without-completed-activation"],
            "runtime update preparation refuses non-retry conflicts",
        )
        validate_runtime_update_retry_conflict(inspected, journal_value)
    else:
        require(result == "neutral_ready", "runtime update preparation requires neutral readiness")

    activation_observation = inspected.get("activation", {})
    activation = activation_observation.get("value", {})
    controller = inspected.get("modules", {}).get(CONTROLLER_MODULE_NAME, {})
    activation_controller = activation.get("controller", {})
    controller_endpoint = inspected.get("endpoints", {}).get("controller", {})
    controller_state = activation.get("controllerState")
    require(
        activation_observation.get("status") == "observed"
        and controller.get("status") == "loaded"
        and activation_controller.get("status") == "loaded"
        and activation_controller.get("exact") is True
        and controller_endpoint.get("status") == "owned"
        and controller_endpoint.get("open") is False
        and isinstance(controller_state, dict)
        and controller_state.get("session") == runtime["controllerSession"]
        and controller_state.get("generation") == runtime["controllerGeneration"]
        and not any(controller_state.get(name) for name in
                    ("generation", "id", "error", "route", "flags"))
        and journal.get("status") == "present"
        and journal_value.get("phase") == "complete-neutral"
        and journal_value.get("requestId") == runtime["activationRequestId"]
        and journal_value.get("planSha256") == runtime["activationPlanSha256"],
        "runtime update preparation lacks exact neutral controller evidence",
    )

    recovery = runtime_call(runner, RUNTIME_PROVIDER, "activation-recover-plan")
    require(
        recovery.get("contract") == RUNTIME_READINESS_CONTRACT
        and recovery.get("operation") == "activation-recover-plan",
        "runtime update recovery plan identity differs",
    )
    recovery_digest = recovery.get("planSha256")
    recovery_plan = recovery.get("plan", {})
    recovery_plan_fields = {
        "version", "operation", "activationJournalSha256", "bindingSha256",
        "bootId", "controllerLoaded", "socketWasActive", "alreadyRecovered",
    }
    require(
        isinstance(recovery_digest, str) and SHA256.fullmatch(recovery_digest)
        and isinstance(recovery_plan, dict)
        and set(recovery_plan) == recovery_plan_fields
        and recovery_digest == sha256_bytes(canonical(recovery_plan))
        and recovery_plan.get("version") == 1
        and recovery_plan.get("operation") == "neutral-activation-recovery"
        and recovery_plan.get("bindingSha256") == runtime["bindingSha256"]
        and recovery_plan.get("bootId") == activation.get("bootId")
        and recovery_plan.get("activationJournalSha256")
            == sha256_bytes(canonical(journal_value))
        and recovery_plan.get("controllerLoaded") is True
        and recovery_plan.get("socketWasActive")
            is journal_value.get("plan", {}).get("socketWasActive")
        and recovery_plan.get("alreadyRecovered") is False,
        "runtime update recovery plan lacks exact owned activation identities",
    )
    require_unchanged_ownership(
        args.record, record, record_identity, "runtime update evidence preservation"
    )
    preserve_owned_activation_journal(record, "before-application-update")
    require_unchanged_ownership(
        args.record, record, record_identity, "runtime update recovery"
    )
    recovered = runtime_call(
        runner, RUNTIME_PROVIDER, "activation-recover",
        ["--plan-sha256", recovery_digest],
    )
    require(
        recovered.get("contract") == RUNTIME_READINESS_CONTRACT
        and recovered.get("operation") == "activation-recover"
        and recovered.get("planSha256") == recovery_digest
        and recovered.get("response", {}).get("status")
            in {"recovered-inhibited", "idempotent-no-change"},
        "runtime update recovery response differs from the reviewed plan",
    )
    final = validate_readiness(runtime_call(
        runner, RUNTIME_PROVIDER, "inspect", (), {"activation_required"}
    ))
    validate_runtime_update_identity(record, final)
    validate_inactive_runtime_update_state(final, recovered=True)
    print("RP1-GPCLK-DKMS neutral runtime recovered and inhibited for application update.")


def activate_runtime(args: argparse.Namespace, runner: Runner) -> None:
    state_dir = secure_state_dir(args.state_dir)
    plan = load_plan(state_dir)
    require(not plan.get("dryRun"), "dry-run plan cannot activate runtime administration")
    if not plan["decision"]["install"]:
        print("RP1-GPCLK-DKMS runtime administration skipped by resolved plan.")
        return
    resolved = plan.get("resolved")
    require(isinstance(resolved, dict), "prepared runtime source identity is missing")
    record, record_identity, reason = load_ownership_record(args.record)
    require(record is not None and record_identity is not None, f"WsprryPi provider ownership is required before runtime deployment: {reason or 'unknown record'}")
    require(record["schema"] in {RECORD_SCHEMA, RUNTIME_RECORD_SCHEMA}, "runtime deployment ownership schema is unsupported")
    require(record["sourceCommit"] == resolved.get("commit"), "owned provider and runtime source commits differ")
    if record["schema"] == RUNTIME_RECORD_SCHEMA:
        print("Exact WsprryPi-owned neutral runtime administration already recorded; verifying idempotent readiness.")
    bundle, reviewed = build_runtime_bundle(state_dir, resolved, runner)
    provider = bundle / "runtime_provider.py"
    initial = runtime_call(runner, provider, "inspect", ["--bundle", str(bundle)],
                           {"absent", "deployment_required", "activation_required", "neutral_ready"})
    validate_readiness(initial)
    require(initial.get("result") not in {"conflict", "recovery_required", "exact_ready"}, "runtime inspection is not eligible for neutral installation")
    deployment = runtime_call(runner, provider, "plan", ["--bundle", str(bundle)],
                              {"absent", "deployment_required", "activation_required", "neutral_ready"})
    validate_readiness(deployment)
    deployment_plan = deployment.get("deployment", {}).get("plan", {})
    deployment_digest = deployment_plan.get("planSha256")
    require(isinstance(deployment_digest, str) and SHA256.fullmatch(deployment_digest), "runtime deployment plan lacks a reviewed digest")
    destinations = deployment_plan.get("destinations")
    runtime_state = "/var/lib/rp1-gpclk-dkms/runtime-admin"
    expected_destinations = set(reviewed["binding"]["files"]) | {
        "/etc/rp1-gpclk-dkms/runtime-controller.json",
        *(f"{runtime_state}/{name}" for name in
          ("transaction.json", "manager.json", "application.json", "activation.json")),
    }
    require(isinstance(destinations, dict) and set(destinations) == expected_destinations, "runtime deployment plan destination inventory differs")
    runtime_call(runner, provider, "ensure",
                 ["--bundle", str(bundle), "--plan-sha256", deployment_digest])
    installed = runtime_call(runner, RUNTIME_PROVIDER, "inspect", (), {"activation_required", "neutral_ready"})
    validate_readiness(installed)
    require(installed.get("result") in {"activation_required", "neutral_ready"}, "runtime deployment did not reach neutral activation admission")
    activation = runtime_call(runner, RUNTIME_PROVIDER, "activation-plan", (), {"activation_required", "neutral_ready"})
    validate_readiness(activation)
    activation_plan = activation.get("activationPlan", {})
    activation_digest = activation_plan.get("planSha256")
    require(isinstance(activation_digest, str) and SHA256.fullmatch(activation_digest), "neutral activation plan lacks a reviewed digest")
    require(activation_plan.get("bindingSha256") == reviewed["bindingSha256"], "neutral activation plan binding differs from reviewed bundle")
    require(activation_plan.get("artifactSetSha256") == reviewed["artifactSetSha256"], "neutral activation plan artifact set differs from reviewed bundle")
    activation_result = runtime_call(
        runner, RUNTIME_PROVIDER, "activation-ensure", ["--plan-sha256", activation_digest]
    )
    require(
        activation_result.get("contract") == RUNTIME_READINESS_CONTRACT
        and activation_result.get("operation") == "activation-ensure"
        and activation_result.get("planSha256") == activation_digest,
        "neutral activation response identity differs from the reviewed plan",
    )
    response = activation_result.get("response", {})
    journal = response.get("journal", {}) if isinstance(response, dict) else {}
    request_id = journal.get("requestId")
    require(isinstance(request_id, str) and len(request_id) >= 8, "neutral activation response lacks an attributable request ID")
    final = validate_readiness(runtime_call(runner, RUNTIME_PROVIDER, "inspect"), "neutral_ready")
    require(final.get("administrationEligible") is True, "neutral runtime administration is not eligible")
    require(final.get("executionReady") is False and final.get("routeSelected") is False, "neutral runtime unexpectedly selected or enabled transmission")
    routes = final.get("routes")
    require(
        isinstance(routes, dict)
        and set(routes) == {"requested", "configured", "persisted", "active"}
        and all(routes[name] is None for name in routes),
        "neutral runtime route evidence is missing or selected",
    )
    safety = final.get("safety", {})
    require(safety.get("outputInhibited") is False and
            safety.get("operationalReady") is False and
            safety.get("owner") is False and safety.get("lease") is False,
            "neutral runtime output state is not unowned and administratively quiescent")
    installed_binding = final.get("identities", {}).get("installedBinding", {})
    require(installed_binding.get("sha256") == reviewed["bindingSha256"], "installed runtime binding differs from reviewed bundle")
    require(installed_binding.get("value", {}).get("artifactSetSha256") == reviewed["artifactSetSha256"], "installed runtime artifact set differs from reviewed bundle")
    activation_observation = final.get("activation", {}).get("value", {})
    controller = activation_observation.get("controllerState", {})
    require(isinstance(controller, dict), "neutral readiness lacks controller identity")
    runtime = {
        "readinessContract": RUNTIME_READINESS_CONTRACT,
        "bindingSha256": reviewed["bindingSha256"],
        "artifactSetSha256": reviewed["artifactSetSha256"],
        "sourceCommit": resolved["commit"],
        "productVersion": runtime_product_version(resolved),
        "targetKernel": platform.release(),
        "compatibilityIdentities": reviewed["binding"]["compatibilityIdentities"],
        "deploymentPlanSha256": deployment_digest,
        "activationPlanSha256": activation_digest,
        "activationRequestId": request_id,
        "controllerSession": controller.get("session"),
        "controllerGeneration": controller.get("generation"),
        "state": "neutral_ready", "route": None, "output": "disabled",
    }
    replacement = copy.deepcopy(record)
    replacement["schema"] = RUNTIME_RECORD_SCHEMA
    replacement["runtime"] = runtime
    validate_runtime_ownership(runtime, replacement)
    replace_owned_record(args.record, record, record_identity, replacement)
    print("RP1-GPCLK-DKMS neutral runtime administration verified; GPIO route and transmission remain unselected and disabled.")


def apply(args: argparse.Namespace, runner: Runner) -> None:
    state_dir = secure_state_dir(args.state_dir)
    plan = load_plan(state_dir)
    require(not plan.get("dryRun"), "dry-run plan cannot be applied")
    if not plan["decision"]["install"]:
        print("RP1-GPCLK-DKMS installation skipped by resolved plan.")
        return
    resolved = plan.get("resolved")
    require(isinstance(resolved, dict), "prepared installation identity is missing")
    record = args.record
    require(record.is_absolute(), "installation record path must be absolute")
    secure_record_parent(record)
    root = args.root.resolve(strict=True)
    if resolved["channel"] == "release":
        apply_release(state_dir, resolved, record, root, runner)
    elif resolved["channel"] == "development":
        apply_development(resolved, record, runner)
    else:
        raise ContractError("prepared source channel is invalid")
    print("RP1-GPCLK-DKMS package/source and DKMS installation verified; no route or transmission was activated.")


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    sub = result.add_subparsers(dest="command", required=True)
    prepare_parser = sub.add_parser("prepare")
    prepare_parser.add_argument("--state-dir", type=pathlib.Path, required=True)
    prepare_parser.add_argument("--install", choices=("auto", "true", "false"), default="auto")
    prepare_parser.add_argument("--source", default="auto")
    prepare_parser.add_argument("--wsprry-source", required=True)
    prepare_parser.add_argument("--record", type=pathlib.Path, default=pathlib.Path("/var/lib/wsprrypi/rp1-gpclk-dkms-installation.json"))
    prepare_parser.add_argument("--model-file", type=pathlib.Path, default=pathlib.Path("/proc/device-tree/model"))
    prepare_parser.add_argument("--compatible-file", type=pathlib.Path, default=pathlib.Path("/proc/device-tree/compatible"))
    prepare_parser.add_argument("--dry-run", action="store_true")
    prepare_parser.add_argument("--debug", action="store_true")
    apply_parser = sub.add_parser("apply")
    apply_parser.add_argument("--state-dir", type=pathlib.Path, required=True)
    apply_parser.add_argument("--record", type=pathlib.Path, default=pathlib.Path("/var/lib/wsprrypi/rp1-gpclk-dkms-installation.json"))
    apply_parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("/"))
    apply_parser.add_argument("--debug", action="store_true")
    update_parser = sub.add_parser("prepare-runtime-update")
    update_parser.add_argument("--state-dir", type=pathlib.Path, required=True)
    update_parser.add_argument("--record", type=pathlib.Path, default=pathlib.Path("/var/lib/wsprrypi/rp1-gpclk-dkms-installation.json"))
    update_parser.add_argument("--debug", action="store_true")
    removal_update_parser = sub.add_parser("prepare-runtime-removal")
    removal_update_parser.add_argument("--record", type=pathlib.Path, default=pathlib.Path("/var/lib/wsprrypi/rp1-gpclk-dkms-installation.json"))
    removal_update_parser.add_argument("--debug", action="store_true")
    runtime_parser = sub.add_parser("activate-runtime")
    runtime_parser.add_argument("--state-dir", type=pathlib.Path, required=True)
    runtime_parser.add_argument("--record", type=pathlib.Path, default=pathlib.Path("/var/lib/wsprrypi/rp1-gpclk-dkms-installation.json"))
    runtime_parser.add_argument("--debug", action="store_true")
    remove_parser = sub.add_parser("remove")
    remove_parser.add_argument("--remove", choices=("auto", "true", "false"), default="auto")
    remove_parser.add_argument("--record", type=pathlib.Path, default=pathlib.Path("/var/lib/wsprrypi/rp1-gpclk-dkms-installation.json"))
    remove_parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("/"))
    remove_parser.add_argument("--debug", action="store_true")
    return result


def main(argv: Sequence[str] | None = None) -> int:
    args = parser().parse_args(argv)
    try:
        if args.command == "prepare":
            prepare(args, Runner(args.debug))
        elif args.command == "apply":
            apply(args, Runner(args.debug))
        elif args.command in {"prepare-runtime-update", "prepare-runtime-removal"}:
            prepare_runtime_update(args, Runner(args.debug))
        elif args.command == "activate-runtime":
            activate_runtime(args, Runner(args.debug))
        else:
            remove_owned_provider(args, Runner(args.debug))
    except ContractError as error:
        print(f"RP1-GPCLK-DKMS operation refused: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
