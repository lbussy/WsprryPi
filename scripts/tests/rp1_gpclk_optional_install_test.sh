#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# shellcheck disable=SC2016
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
INSTALLER="${SCRIPT_DIR}/../install-rp1-gpclk-package.sh"
TEMP_DIR=$(mktemp -d)
trap 'rm -rf "$TEMP_DIR"' EXIT

MODEL_PATH="$TEMP_DIR/model"
HEADER_ROOT="$TEMP_DIR/usr-src"
KERNEL="6.18.34+rpt-rpi-2712"
mkdir -p "$HEADER_ROOT/linux-headers-$KERNEL" "$TEMP_DIR/bin"
printf 'Raspberry Pi 5 Model B Rev 1.0\0' >"$MODEL_PATH"
touch "$HEADER_ROOT/linux-headers-$KERNEL/Makefile"

cat >"$TEMP_DIR/bin/apt-cache" <<'EOF'
#!/usr/bin/env bash
[[ "$1" == show && "$2" == "linux-headers-6.18.34+rpt-rpi-2712" ]]
EOF
chmod +x "$TEMP_DIR/bin/apt-cache"

run_check() {
    env \
        PATH="$TEMP_DIR/bin:$PATH" \
        RP1_GPCLK_HISTORICAL_PREDECESSOR_FIXTURE=true \
        INSTALL_RP1_GPCLK_DKMS=true \
        RP1_TEST_KERNEL_RELEASE="${RP1_TEST_KERNEL_RELEASE:-$KERNEL}" \
        RP1_TEST_ARCHITECTURE="${RP1_TEST_ARCHITECTURE:-arm64}" \
        RP1_TEST_MODEL_PATH="$MODEL_PATH" \
        RP1_TEST_HEADER_ROOT="$HEADER_ROOT" \
        "$INSTALLER" check
}

if INSTALL_RP1_GPCLK_DKMS=true "$INSTALLER" check >/dev/null 2>&1; then
    echo "historical 1.1.1 installer must reject current consumer use" >&2
    exit 1
fi

if env PATH="$TEMP_DIR/bin:$PATH" \
    RP1_GPCLK_HISTORICAL_PREDECESSOR_FIXTURE=true \
    RP1_TEST_KERNEL_RELEASE="$KERNEL" \
    RP1_TEST_ARCHITECTURE=arm64 \
    RP1_TEST_MODEL_PATH="$MODEL_PATH" \
    RP1_TEST_HEADER_ROOT="$HEADER_ROOT" \
    "$INSTALLER" check >/dev/null 2>&1; then
    echo "optional installer must require explicit opt-in" >&2
    exit 1
fi

run_check | grep -Fq "RP1 GPCLK prerequisite check: PASS"

for bad_arch in amd64 armhf; do
    if RP1_TEST_ARCHITECTURE="$bad_arch" run_check >/dev/null 2>&1; then
        echo "optional installer accepted unsupported architecture: $bad_arch" >&2
        exit 1
    fi
done

if RP1_TEST_KERNEL_RELEASE="6.18.34-generic" run_check >/dev/null 2>&1; then
    echo "optional installer accepted a non-Raspberry-Pi kernel" >&2
    exit 1
fi

printf 'Raspberry Pi 4 Model B Rev 1.5\0' >"$MODEL_PATH"
if run_check >/dev/null 2>&1; then
    echo "optional installer accepted a non-Pi-5 model" >&2
    exit 1
fi
printf 'Raspberry Pi 5 Model B Rev 1.0\0' >"$MODEL_PATH"

rm "$HEADER_ROOT/linux-headers-$KERNEL/Makefile"
if run_check >/dev/null 2>&1; then
    echo "optional installer accepted missing exact headers" >&2
    exit 1
fi
touch "$HEADER_ROOT/linux-headers-$KERNEL/Makefile"

if grep -Eq 'apt-get install.*(debhelper|dh-dkms|device-tree-compiler)' "$INSTALLER"; then
    echo "consumer installer must not install package-build dependencies" >&2
    exit 1
fi
if grep -Eq '(^|[;&|[:space:]])dkms[[:space:]]+(add|build|install)' "$INSTALLER"; then
    echo "consumer installer must delegate lifecycle to the Debian package" >&2
    exit 1
fi
grep -Fq 'apt-get install -y "$HEADER_PACKAGE"' "$INSTALLER"
grep -Fq 'apt-get install -y "$package_path"' "$INSTALLER"
grep -Fq 'EXPECTED_UAPI_SHA256=' "$INSTALLER"
grep -Fq 'boot_hash_after' "$INSTALLER"
grep -Fq 'module_after' "$INSTALLER"
grep -Fq 'endpoint_after' "$INSTALLER"
grep -Fq '$1 == "rp1_gpclk_dkms"' "$INSTALLER"
grep -Fq '247bd7da35e4ad812a13828668fe03673da127bad7ed2b3e970876f3f21c002d' "$INSTALLER"
install_body=$(sed -n '/^install_package()/,/^enroll_route_manager()/p' "$INSTALLER")
if grep -Eq 'systemctl[[:space:]]+enable|systemctl[[:space:]]+start|systemctl[[:space:]]+enable[[:space:]]+--now' <<<"$install_body"; then
    echo "package installation must leave the route-manager socket disabled" >&2
    exit 1
fi
grep -Fq 'ENABLE_RP1_GPCLK_ROUTE_MANAGER' "$INSTALLER"
grep -Fq 'systemctl show wsprrypi.service --property=User --value' "$INSTALLER"
grep -Fq 'root:rp1-gpclk-route' "$INSTALLER"
grep -Fq '"$socket_mode" == "660"' "$INSTALLER"

grep -Fq 'refusing package installation while rp1_gpclk_dkms is loaded' "$INSTALLER"
grep -Fq 'refusing package installation while /dev/rp1-gpclk exists' "$INSTALLER"
echo "RP1 GPCLK optional install tests: PASS"
