#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
copydocs.py

@brief Updates and deploys documentation for a Git repository.

@details
This script automates the process of cleaning, building, and deploying
documentation generated from a Git repository. It performs the following
steps:
- Verifies the script is executed inside a Git repository.
- Activates (or re-executes under) the `docs/.venv` if not already active,
  preserving environment.
- Builds the documentation via Sphinx using the venv’s Python module,
  avoiding reliance on Makefile shebangs.
- Deploys the generated documentation to a specified directory.
- Prints the local URL where the docs can be accessed.

@example
    sudo ./copy_docs.py

@author Lee C. Bussy <Lee@Bussy.org>
@date 2025-03-02
@license MIT License

MIT License

Copyright (c) 2023-2025 Lee C. Bussy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

import sys
import os
import shutil
import subprocess
from pathlib import Path
import pwd
import grp


def get_repo_root():
    """
    Get the root directory of the current Git repository.
    :return: Path to the repository root.
    :raises SystemExit: if not inside a Git repository.
    """
    try:
        result = subprocess.run(
            ['git', 'rev-parse', '--show-toplevel'],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            check=True
        )
        repo_root = result.stdout.strip()
        if not repo_root:
            raise subprocess.CalledProcessError(returncode=1, cmd='')
        return Path(repo_root)
    except subprocess.CalledProcessError:
        print("Error: Not inside a Git repository.", file=sys.stderr)
        sys.exit(1)


def ensure_venv(repo_root: Path):
    """
    Ensure the script is running under docs/.venv Python; if not, re-exec
    under it.
    Preserves the venv environment when running under sudo.
    """
    venv_dir = repo_root / 'docs' / '.venv'
    python_bin = venv_dir / 'bin' / 'python'

    if not venv_dir.is_dir() or not python_bin.exists():
        print(f"Error: virtualenv not found at {venv_dir}.", file=sys.stderr)
        sys.exit(1)

    # If current interpreter differs, re-launch under venv with proper env
    if Path(sys.executable) != python_bin:
        env = os.environ.copy()
        env['VIRTUAL_ENV'] = str(venv_dir)
        env['PATH'] = str(python_bin.parent) + os.pathsep + env.get('PATH', '')
        env.pop('PYTHONHOME', None)
        os.execve(str(python_bin), [str(python_bin)] + sys.argv, env)


def build_docs(repo_root: Path):
    """
    Build documentation using Sphinx via the venv’s Python module.
    :param repo_root: Path to the repository root.
    :raises SystemExit: if docs directory is missing or build fails.
    """
    docs_dir = repo_root / 'docs'
    if not docs_dir.is_dir():
        print(f"Error: Docs directory not found at {docs_dir}.", file=sys.stderr)
        sys.exit(1)

    # Clean previous build output
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
        print(f"Error: Failed to build documentation in {docs_dir}.", file=sys.stderr)
        sys.exit(1)


def deploy_docs(repo_root: Path):
    """
    Deploy the built documentation to the web server directory.
    :param repo_root: Path to the repository root.
    :raises SystemExit: if deployment fails.
    """
    src_dir = repo_root / 'docs' / '_build' / 'html'
    dest_dir = Path('/var/www/html/wsprrypi/docs')

    if not src_dir.is_dir():
        print(f"Error: Docs source not found at {src_dir}.", file=sys.stderr)
        sys.exit(1)

    # Remove existing destination
    if dest_dir.exists():
        try:
            shutil.rmtree(dest_dir)
        except Exception as e:
            print(f"Error: Failed to remove {dest_dir}: {e}", file=sys.stderr)
            sys.exit(1)

    # Create destination directory
    try:
        dest_dir.mkdir(parents=True, exist_ok=True)
    except Exception as e:
        print(f"Error: Failed to create {dest_dir}: {e}", file=sys.stderr)
        sys.exit(1)

    # Copy documentation files
    for item in src_dir.iterdir():
        target = dest_dir / item.name
        try:
            if item.is_dir():
                shutil.copytree(item, target)
            else:
                shutil.copy2(item, target)
        except Exception as e:
            print(f"Error: Failed to copy {item} to {target}: {e}", file=sys.stderr)
            sys.exit(1)

    # Set ownership and permissions
    try:
        uid = pwd.getpwnam('www-data').pw_uid
        gid = grp.getgrnam('www-data').gr_gid
        for path in dest_dir.rglob('*'):
            os.chown(path, uid, gid)
            path.chmod(0o755 if path.is_dir() else 0o644)
        os.chown(dest_dir, uid, gid)
        dest_dir.chmod(0o755)
    except Exception as e:
        print(f"Error: Failed to set ownership or permissions: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Docs successfully copied to {dest_dir}.")


def main():
    repo_root = get_repo_root()
    ensure_venv(repo_root)

    if os.geteuid() != 0:
        print("Error: This script requires root privileges. Please run with sudo.", file=sys.stderr)
        sys.exit(1)

    build_docs(repo_root)
    deploy_docs(repo_root)
    # Final message with access URL
    print("Docs copied successfully.")
    print("Documentation available at http://wspr4.local/wsprrypi/docs/")


if __name__ == '__main__':
    main()
