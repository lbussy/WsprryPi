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

if ! awk '
    /^readonly APT_PACKAGES=\(/ { in_packages = 1; next }
    in_packages && /^\)/ { exit }
    in_packages && /^[[:space:]]*"age"[[:space:]]*$/ { found = 1 }
    END { exit(found ? 0 : 1) }
' "$INSTALLER"; then
    echo "age must remain in install.sh APT_PACKAGES" >&2
    exit 1
fi

# shellcheck disable=SC2016
if ! grep -Fq 'validate_support_bundle_age_dependency "$debug" || return 1' "$INSTALLER"; then
    echo "install.sh must fail closed after validating age executables" >&2
    exit 1
fi

if ! awk '
    /handle_apt_packages "\$debug" \|\| return 1/ { packages = NR }
    /validate_support_bundle_age_dependency "\$debug" \|\| return 1/ { validation = NR }
    END { exit(packages > 0 && validation == packages + 1 ? 0 : 1) }
' "$INSTALLER"; then
    echo "age validation must immediately follow successful package handling" >&2
    exit 1
fi

if ! grep -Fq 'Dry run: would validate the support-bundle encryption tools.' "$INSTALLER"; then
    echo "installer dry-run must report age validation without executing it" >&2
    exit 1
fi

if awk '/manage_wsprry_pi\(\)/,/^}/' "$INSTALLER" | grep -Fq 'apt-get remove age'; then
    echo "uninstall must not remove the distribution age package" >&2
    exit 1
fi

for model in \
    '["Raspberry Pi Compute Module 4S|4s-compute-module|bcm2711"]="Supported"' \
    '["Raspberry Pi Compute Module 3+|3-plus-compute-module|bcm2837"]="Supported"' \
    '["Raspberry Pi Compute Module Zero|0-compute-module|bcm2837"]="Supported"'
do
    if ! grep -Fq "$model" "$INSTALLER"; then
        echo "install.sh must recognize: $model" >&2
        exit 1
    fi
done

if grep -Eiq 'rp1[-_]gpclk|live_output|kernel_2712_phase|dtoverlay=rp1' "$INSTALLER"; then
    echo "standard installer must not deploy the experimental RP1 GPCLK kernel/provider path" >&2
    exit 1
fi

echo "installer dependency tests: PASS"
