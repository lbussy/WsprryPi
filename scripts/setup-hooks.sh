#!/usr/bin/env bash
set -euo pipefail

# Quiet prints to stderr
note() { printf "%s\n" "$*" >&2; }
die()  { printf "Error: %s\n" "$*" >&2; exit 1; }

FORCE_CONFIG="${1:-}"

# Find repo root and paths
repo_root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
[ -n "$repo_root" ] || die "Not inside a Git repository"
cd "$repo_root"

githooks_dir="$repo_root/.githooks"
hooks_dir="$(git rev-parse --git-path hooks)"

# Ensure .githooks exists
if [ ! -d "$githooks_dir" ]; then
  note "Creating $githooks_dir"
  mkdir -p "$githooks_dir"
fi

# Ensure files in .githooks are executable
# Only set +x on regular files at top level
while IFS= read -r -d '' f; do
  chmod +x "$f"
done < <(find "$githooks_dir" -maxdepth 1 -type f -print0)

# If user forced config method
if [ "$FORCE_CONFIG" = "--force-config" ]; then
  git config core.hooksPath .githooks
  note "Configured core.hooksPath to .githooks"
  note "Done"
  exit 0
fi

# Try symlink method first
# Remove existing hooks dir or link
rm -rf "$hooks_dir" 2>/dev/null || true

if ln -s "$githooks_dir" "$hooks_dir" 2>/dev/null; then
  # Verify the link points to .githooks
  if [ -L "$hooks_dir" ]; then
    note "Linked $hooks_dir -> $githooks_dir"
    # Clear any repo-level hooksPath so default resolution uses .git/hooks
    git config --unset core.hooksPath 2>/dev/null || true
    note "Done"
    exit 0
  fi
fi

# Symlink failed, fall back to core.hooksPath
git config core.hooksPath .githooks
note "Symlink not available. Configured core.hooksPath to .githooks"
note "Done"
