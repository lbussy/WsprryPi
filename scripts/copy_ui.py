#!/usr/bin/env python3
"""Build, validate, and transactionally publish the installed WsprryPi UI."""

from __future__ import annotations

import argparse
import importlib.util
import os
from pathlib import Path
import pwd
import grp
import shutil
import stat
import subprocess
import sys
import tempfile
from typing import Callable

MANIFEST_FILENAME = "ui-manifest.json"


class UiPublicationError(RuntimeError):
    """Raised when a UI artifact cannot be staged, validated, or published."""


def get_git_root() -> Path | None:
    script_dir = Path(__file__).resolve().parent
    try:
        return Path(subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"], cwd=script_dir, text=True
        ).strip())
    except subprocess.CalledProcessError:
        return None


def get_source_commit(git_root: Path) -> str:
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=git_root, text=True
        ).strip()
    except subprocess.CalledProcessError as error:
        raise UiPublicationError("unable to determine the UI source commit") from error


def get_application_version(git_root: Path) -> str:
    version_script = git_root / "scripts" / "get_semantic_version.py"
    try:
        version = subprocess.check_output(
            [sys.executable, str(version_script)], cwd=git_root, text=True
        ).strip()
    except subprocess.CalledProcessError as error:
        raise UiPublicationError("unable to determine the application version") from error
    if not version:
        raise UiPublicationError("application version is empty")
    return version


def load_manifest_module(git_root: Path):
    module_path = git_root / "WsprryPi-UI" / "scripts" / "generate_ui_manifest.py"
    if not module_path.is_file():
        raise UiPublicationError(f"manifest generator not found: {module_path}")
    spec = importlib.util.spec_from_file_location("wsprrypi_ui_manifest", module_path)
    if spec is None or spec.loader is None:
        raise UiPublicationError(f"unable to load manifest generator: {module_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _safe_publication_paths(source: Path, target: Path) -> tuple[Path, Path]:
    if target.is_symlink():
        raise UiPublicationError(f"UI publication target must not be a symlink: {target}")
    source = source.resolve(strict=True)
    target = target.resolve(strict=False)
    if not source.is_dir():
        raise UiPublicationError(f"UI source directory not found: {source}")
    if target == Path(target.anchor) or target.parent == target:
        raise UiPublicationError(f"unsafe UI publication target: {target}")
    if target == source or source in target.parents or target in source.parents:
        raise UiPublicationError("UI source and publication target must not overlap")
    if target.exists() and not target.is_dir():
        raise UiPublicationError(f"UI publication target is not a directory: {target}")
    return source, target


def _set_tree_permissions(root: Path, owner: str | None, group: str | None) -> None:
    if (owner or group) and os.geteuid() != 0:
        raise UiPublicationError(
            "root privileges are required to make the UI immutable to the runtime account"
        )
    uid = pwd.getpwnam(owner).pw_uid if owner else -1
    gid = grp.getgrnam(group).gr_gid if group else -1
    static_uid = 0 if os.geteuid() == 0 else uid
    static_gid = 0 if os.geteuid() == 0 else gid
    for directory, child_directories, files in os.walk(root):
        directory_path = Path(directory)
        os.chmod(directory_path, 0o755)
        if owner or group:
            os.chown(directory_path, static_uid, static_gid)
        for name in child_directories:
            child = directory_path / name
            if child.is_symlink():
                raise UiPublicationError(f"symbolic links are not permitted: {child}")
        for name in files:
            child = directory_path / name
            if child.is_symlink():
                raise UiPublicationError(f"symbolic links are not permitted: {child}")
            os.chmod(child, 0o644)
            if owner or group:
                os.chown(child, static_uid, static_gid)

    for mutable_name in ("cache", "backups"):
        mutable_root = root / mutable_name
        if not mutable_root.is_dir() or not (owner or group):
            continue
        for directory, _, files in os.walk(mutable_root):
            directory_path = Path(directory)
            os.chown(directory_path, uid, gid)
            for name in files:
                os.chown(directory_path / name, uid, gid)

    manifest = root / MANIFEST_FILENAME
    os.chmod(manifest, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
    if os.geteuid() == 0:
        os.chown(manifest, 0, 0)


def publish_ui(
    source: Path,
    target: Path,
    *,
    source_commit: str,
    application_version: str,
    manifest_module,
    owner: str | None = None,
    group: str | None = None,
    before_publish: Callable[[Path], None] | None = None,
) -> dict:
    """Publish one validated UI directory tree without exposing partial staging."""
    source, target = _safe_publication_paths(source, target)
    target.parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix=f".{target.name}.stage.", dir=target.parent))
    previous = target.parent / f".{target.name}.previous.{os.getpid()}"
    previous_created = False

    try:
        shutil.copytree(source, stage, dirs_exist_ok=True, symlinks=True)
        manifest_path = stage / MANIFEST_FILENAME
        manifest = manifest_module.build_manifest(
            stage,
            source_commit,
            application_version,
        )
        manifest_module.write_atomic(
            manifest_path, manifest_module.serialize_manifest(manifest)
        )
        validated = manifest_module.load_manifest(manifest_path)
        installed = manifest_module.classify_installed_ui(stage, manifest_path)
        if (
            installed.get("installed_state") != "packaged"
            or installed.get("packaged_ui_build_id") != installed.get("installed_ui_build_id")
            or installed.get("installed_ui_build_id") != validated["packaged_ui_build_id"]
        ):
            raise UiPublicationError("staged UI identity does not match its manifest")

        _set_tree_permissions(stage, owner, group)
        if before_publish:
            before_publish(stage)
        final_status = manifest_module.classify_installed_ui(stage, manifest_path)
        if (
            final_status.get("installed_state") != "packaged"
            or final_status.get("installed_ui_build_id")
            != installed.get("installed_ui_build_id")
        ):
            raise UiPublicationError("staged UI changed after manifest validation")

        if previous.exists():
            raise UiPublicationError(f"stale publication rollback path exists: {previous}")
        if target.exists():
            os.replace(target, previous)
            previous_created = True
        try:
            os.replace(stage, target)
        except Exception:
            if previous_created:
                os.replace(previous, target)
                previous_created = False
            raise
        if previous_created:
            shutil.rmtree(previous)
            previous_created = False
        return installed
    except UiPublicationError:
        raise
    except Exception as error:
        raise UiPublicationError(str(error)) from error
    finally:
        if stage.exists():
            shutil.rmtree(stage)
        if previous_created and previous.exists() and not target.exists():
            os.replace(previous, target)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--target", type=Path, default=Path("/var/www/html/wsprrypi"))
    parser.add_argument("--source-commit")
    parser.add_argument("--application-version")
    parser.add_argument("--owner", default="www-data")
    parser.add_argument("--group", default="www-data")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    git_root = get_git_root()
    if git_root is None:
        print("Error: Not inside a Git repository.", file=sys.stderr)
        return 1
    source = args.source or git_root / "WsprryPi-UI" / "data"
    try:
        source_commit = args.source_commit or get_source_commit(git_root)
        application_version = args.application_version or get_application_version(git_root)
        status = publish_ui(
            source,
            args.target,
            source_commit=source_commit,
            application_version=application_version,
            manifest_module=load_manifest_module(git_root),
            owner=args.owner or None,
            group=args.group or None,
        )
    except (OSError, KeyError, UiPublicationError) as error:
        print(f"Error: UI publication failed: {error}", file=sys.stderr)
        return 1
    print(
        "Deployment successful: "
        f"{status['installed_ui_build_id']} ({status['installed_state']})."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
