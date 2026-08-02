#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
INSTALLER="${SCRIPT_DIR}/../install.sh"

if ! awk '
    /^readonly APT_PACKAGES=\(/ { in_packages = 1; next }
    in_packages && /^\)/ { exit }
    in_packages && /^[[:space:]]*"libssl-dev"[[:space:]]*$/ { found = 1 }
    END { exit(found ? 0 : 1) }
' "$INSTALLER"; then
    echo "libssl-dev must remain in install.sh APT_PACKAGES" >&2
    exit 1
fi

echo "installer dependency tests: PASS"
