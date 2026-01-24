#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
copydocs.py

@brief Updates and deploys documentation for a Git repository.

@details
This script automates cleaning, building, and deploying documentation.
It ensures a Python venv under docs/.venv or docs/venv is active,
builds via Sphinx, and deploys to the web server directory.

@example
    sudo ./copydocs.py

@license MIT License
"""

import sys
import os
import shutil
import subprocess
from pathlib import Path
import pwd
import grp


def get_repo_root():
    try:
        result = subprocess.run(
            ['git', 'rev-parse', '--show-toplevel'],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            check=True
        )
        return Path(result.stdout.strip())
    except subprocess.CalledProcessError:
        print("Error: Not inside a Git repository.", file=sys.stderr)
        sys.exit(1)


def ensure_venv(repo_root: Path):
    """
    Ensure running under docs/.venv or docs/venv Python; re-exec if needed.
    Preserves environment when running under sudo.
    """
    docs_dir = repo_root / 'docs'
    candidates = [docs_dir / '.venv', docs_dir / 'venv']
    venv_dir = None
    python_bin = None

    for candidate in candidates:
        bin_py = candidate / 'bin' / 'python'
        if candidate.is_dir() and bin_py.exists():
            venv_dir = candidate
            python_bin = bin_py
            break

    if not venv_dir or not python_bin:
        print(f"Error: virtualenv not found in {candidates}.", file=sys.stderr)
        sys.exit(1)

    # Re-launch under the venv if not already the same interpreter
    if Path(sys.executable) != python_bin:
        env = os.environ.copy()
        env['VIRTUAL_ENV'] = str(venv_dir)
        env['PATH'] = str(python_bin.parent) + os.pathsep + env.get('PATH', '')
        env.pop('PYTHONHOME', None)
        os.execve(str(python_bin), [str(python_bin)] + sys.argv, env)


def build_docs(repo_root: Path):
    docs_dir = repo_root / 'docs'
    if not docs_dir.is_dir():
        print(f"Error: Docs missing at {docs_dir}.", file=sys.stderr)
        sys.exit(1)

    build_root = docs_dir / '_build'
    if build_root.exists():
        shutil.rmtree(build_root)

    html_dir = build_root / 'html'
    try:
        subprocess.run(
            [sys.executable, '-m', 'sphinx', '-b', 'html', str(docs_dir), str(html_dir)],
            check=True
        )
    except subprocess.CalledProcessError:
        print(f"Error: Sphinx build failed.", file=sys.stderr)
        sys.exit(1)


def deploy_docs(repo_root: Path):
    src_dir = repo_root / 'docs' / '_build' / 'html'
    dest_dir = Path('/var/www/html/wsprrypi/docs')

    if not src_dir.is_dir():
        print(f"Error: Docs source not found.", file=sys.stderr)
        sys.exit(1)

    if dest_dir.exists():
        shutil.rmtree(dest_dir)
    dest_dir.mkdir(parents=True, exist_ok=True)

    for item in src_dir.iterdir():
        target = dest_dir / item.name
        if item.is_dir():
            shutil.copytree(item, target)
        else:
            shutil.copy2(item, target)

    try:
        uid = pwd.getpwnam('www-data').pw_uid
        gid = grp.getgrnam('www-data').gr_gid
        for p in dest_dir.rglob('*'):
            os.chown(p, uid, gid)
            p.chmod(0o755 if p.is_dir() else 0o644)
        os.chown(dest_dir, uid, gid)
        dest_dir.chmod(0o755)
    except Exception as e:
        print(f"Error: Permission set failed: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Docs deployed to {dest_dir}.")


def main():
    repo_root = get_repo_root()
    ensure_venv(repo_root)

    if os.geteuid() != 0:
        print("Error: Requires root privileges.", file=sys.stderr)
        sys.exit(1)

    build_docs(repo_root)
    deploy_docs(repo_root)
    print("Docs copied successfully.")
    print("Available at http://wspr4.local/wsprrypi/docs/")


if __name__ == '__main__':
    main()
