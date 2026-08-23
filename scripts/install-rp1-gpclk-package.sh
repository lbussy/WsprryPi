#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail

if [[ "${RP1_GPCLK_HISTORICAL_PREDECESSOR_FIXTURE:-false}" != "true" ]]; then
    printf '%s\n' \
        'RP1 GPCLK optional install: ERROR: the 1.1.1 installer is historical-only; no frozen 1.1.2 development package identity exists' >&2
    exit 1
fi

readonly EXPECTED_PACKAGE_NAME="rp1-gpclk-dkms"
readonly EXPECTED_UAPI_SHA256="998ab96d7dbcc0d935c05758c46acba56bbcf92aa1b674b899bdab6932dc8384"
readonly EXPECTED_PACKAGE_SHA256="247bd7da35e4ad812a13828668fe03673da127bad7ed2b3e970876f3f21c002d"
readonly EXPECTED_EXECUTOR_SHA256="a1e247df88650cad0866cc37f946a2859e0594e457a03f8674f6a691901be2da"
readonly EXPECTED_SCHEMA_SHA256="097762cf365e864162b1199bceb05d2937b719ddf426d3a90b6a7f680803251b"
readonly EXPECTED_DOCUMENTATION_SHA256="b5936885ee3cddaeaf0b21590a0657a73f8d9b1dc1b26fd1b2fbcd2afa043f25"
readonly EXPECTED_SOCKET_UNIT_SHA256="336f2ca703ab95b4d124d643f9b08b939ec055b11c7f4bb573207f4cb99b4068"
readonly EXPECTED_SERVICE_UNIT_SHA256="0d14b1ba451af698d831cb4fa342ec046d513391569eacbe3c98b8fd9104e3ce"
MODE="${1:-check}"
KERNEL_RELEASE="${RP1_TEST_KERNEL_RELEASE:-$(uname -r)}"
ARCHITECTURE="${RP1_TEST_ARCHITECTURE:-$(dpkg --print-architecture)}"
MODEL_PATH="${RP1_TEST_MODEL_PATH:-/proc/device-tree/model}"
HEADER_ROOT="${RP1_TEST_HEADER_ROOT:-/usr/src}"
HEADER_PACKAGE="linux-headers-${KERNEL_RELEASE}"
HEADER_PATH="${HEADER_ROOT}/linux-headers-${KERNEL_RELEASE}"
EXPECTED_VERSION="${RP1_GPCLK_DKMS_VERSION:-1.1.1-1}"
UNPACK_DIR=""

fail() {
    printf 'RP1 GPCLK optional install: ERROR: %s\n' "$*" >&2
    exit 1
}

cleanup() {
    if [[ -n "$UNPACK_DIR" && -d "$UNPACK_DIR" ]]; then
        rm -rf -- "$UNPACK_DIR"
    fi
}
trap cleanup EXIT

require_opt_in() {
    if [[ "$MODE" == "enroll" ]]; then
        [[ "${ENABLE_RP1_GPCLK_ROUTE_MANAGER:-false}" == "true" ]] ||
            fail "set ENABLE_RP1_GPCLK_ROUTE_MANAGER=true to opt in"
    else
        [[ "${INSTALL_RP1_GPCLK_DKMS:-false}" == "true" ]] ||
            fail "set INSTALL_RP1_GPCLK_DKMS=true to opt in"
    fi
}

validate_target() {
    local model
    [[ "$ARCHITECTURE" == "arm64" ]] ||
        fail "unsupported architecture: $ARCHITECTURE"
    [[ "$KERNEL_RELEASE" =~ ^[0-9]+\.[0-9]+\.[0-9]+\+rpt-rpi-2712$ ]] ||
        fail "unsupported running kernel: $KERNEL_RELEASE"
    [[ -r "$MODEL_PATH" ]] || fail "cannot read Raspberry Pi model"
    model=$(tr -d '\0' <"$MODEL_PATH")
    [[ "$model" == Raspberry\ Pi\ 5\ Model\ B* ]] ||
        fail "unsupported model: $model"
    apt-cache show "$HEADER_PACKAGE" >/dev/null 2>&1 ||
        fail "exact header package is unavailable: $HEADER_PACKAGE"
}

validate_headers() {
    [[ -f "$HEADER_PATH/Makefile" ]] ||
        fail "exact running-kernel headers are not installed at $HEADER_PATH"
}

state_value() {
    if "$@" >/dev/null 2>&1; then
        printf 'present'
    else
        printf 'absent'
    fi
}

install_package() {
    local package_path package_hash package_name package_version package_arch
    local actual_hash packaged_uapi boot_config boot_hash_before boot_hash_after
    local module_before module_after endpoint_before endpoint_after

    [[ $EUID -eq 0 ]] || fail "install mode must run as root"
    package_path="${RP1_GPCLK_DKMS_PACKAGE:-}"
    package_hash="${RP1_GPCLK_DKMS_SHA256:-}"
    [[ "$package_path" == /* && -f "$package_path" ]] ||
        fail "RP1_GPCLK_DKMS_PACKAGE must name an absolute local .deb"
    [[ "$package_hash" =~ ^[0-9a-f]{64}$ ]] ||
        fail "RP1_GPCLK_DKMS_SHA256 must be a lowercase SHA-256 digest"
    actual_hash=$(sha256sum "$package_path" | awk '{print $1}')
    [[ "$package_hash" == "$EXPECTED_PACKAGE_SHA256" ]] ||
        fail "only the exact authorized RP1 GPCLK package is accepted"
    [[ "$actual_hash" == "$package_hash" ]] ||
        fail "package SHA-256 mismatch"

    package_name=$(dpkg-deb -f "$package_path" Package)
    package_version=$(dpkg-deb -f "$package_path" Version)
    package_arch=$(dpkg-deb -f "$package_path" Architecture)
    [[ "$package_name" == "$EXPECTED_PACKAGE_NAME" ]] ||
        fail "unexpected package name: $package_name"
    [[ "$package_version" == "$EXPECTED_VERSION" ]] ||
        fail "unexpected package version: $package_version"
    [[ "$package_arch" == "all" ]] ||
        fail "unexpected package architecture: $package_arch"

    UNPACK_DIR=$(mktemp -d)
    dpkg-deb -x "$package_path" "$UNPACK_DIR"
    packaged_uapi="$UNPACK_DIR/usr/src/rp1-gpclk-dkms-${EXPECTED_VERSION%-*}/include/uapi/linux/rp1_gpclk.h"
    [[ -f "$packaged_uapi" ]] || fail "canonical UAPI is missing from package"
    printf '%s  %s\n' "$EXPECTED_UAPI_SHA256" "$packaged_uapi" |
        sha256sum --check --status || fail "canonical UAPI identity mismatch"
    printf '%s  %s\n' "$EXPECTED_EXECUTOR_SHA256" "$UNPACK_DIR/usr/sbin/rp1-gpclk-route-manager" |
        sha256sum --check --status || fail "route-manager executor identity mismatch"
    printf '%s  %s\n' "$EXPECTED_EXECUTOR_SHA256" "$UNPACK_DIR/usr/libexec/rp1-gpclk-dkms/rp1-gpclk-route-manager" |
        sha256sum --check --status || fail "canonical route-manager executor identity mismatch"
    printf '%s  %s\n' "$EXPECTED_SCHEMA_SHA256" "$UNPACK_DIR/usr/share/rp1-gpclk-dkms/1.1.1/rp1-gpclk-route-manager-v1.schema.json" |
        sha256sum --check --status || fail "route-manager schema identity mismatch"
    printf '%s  %s\n' "$EXPECTED_SOCKET_UNIT_SHA256" "$UNPACK_DIR/usr/lib/systemd/system/rp1-gpclk-route-manager.socket" |
        sha256sum --check --status || fail "route-manager socket unit identity mismatch"
    printf '%s  %s\n' "$EXPECTED_SERVICE_UNIT_SHA256" "$UNPACK_DIR/usr/lib/systemd/system/rp1-gpclk-route-manager@.service" |
        sha256sum --check --status || fail "route-manager service unit identity mismatch"
    printf '%s  %s\n' "$EXPECTED_DOCUMENTATION_SHA256" "$UNPACK_DIR/usr/share/doc/rp1-gpclk-dkms/route-manager-v1.md" |
        sha256sum --check --status || fail "route-manager documentation identity mismatch"

    boot_config="/boot/firmware/config.txt"
    [[ -f "$boot_config" ]] || fail "boot configuration is unavailable"
    boot_hash_before=$(sha256sum "$boot_config" | awk '{print $1}')
    # shellcheck disable=SC2016
    module_before=$(state_value awk '$1 == "rp1_gpclk_dkms" { found = 1 } END { exit !found }' /proc/modules)
    endpoint_before=$(state_value test -e /dev/rp1-gpclk)
    [[ "$module_before" == "absent" ]] ||
        fail "refusing package installation while rp1_gpclk_dkms is loaded"
    [[ "$endpoint_before" == "absent" ]] ||
        fail "refusing package installation while /dev/rp1-gpclk exists"

    apt-get install -y "$HEADER_PACKAGE"
    validate_headers
    apt-get install -y "$package_path"

    [[ "$(dpkg-query -W -f='${Version}' "$EXPECTED_PACKAGE_NAME")" == "$EXPECTED_VERSION" ]] ||
        fail "installed package version does not match"
    boot_hash_after=$(sha256sum "$boot_config" | awk '{print $1}')
    # shellcheck disable=SC2016
    module_after=$(state_value awk '$1 == "rp1_gpclk_dkms" { found = 1 } END { exit !found }' /proc/modules)
    endpoint_after=$(state_value test -e /dev/rp1-gpclk)
    [[ "$boot_hash_after" == "$boot_hash_before" ]] ||
        fail "package installation changed config.txt"
    [[ "$module_after" == "$module_before" ]] ||
        fail "package installation changed module load state"
    [[ "$endpoint_after" == "$endpoint_before" ]] ||
        fail "package installation changed endpoint state"
    [[ -f /boot/firmware/overlays/rp1-gpclk-gpio4.dtbo ]] ||
        fail "GPIO4 overlay was not installed"
    [[ -f /boot/firmware/overlays/rp1-gpclk-gpio20.dtbo ]] ||
        fail "GPIO20 overlay was not installed"

    printf 'RP1 GPCLK optional package install: PASS\n'
    printf 'kernel=%s package=%s version=%s uapi_sha256=%s\n' \
        "$KERNEL_RELEASE" "$package_hash" "$EXPECTED_VERSION" "$EXPECTED_UAPI_SHA256"
}

enroll_route_manager() {
    local service_account socket_identity socket_mode
    [[ $EUID -eq 0 ]] || fail "enroll mode must run as root"
    [[ "$(dpkg-query -W -f='${Version}' "$EXPECTED_PACKAGE_NAME" 2>/dev/null)" == "$EXPECTED_VERSION" ]] ||
        fail "the exact RP1 GPCLK package is not installed"
    printf '%s  %s\n' "$EXPECTED_SOCKET_UNIT_SHA256" /usr/lib/systemd/system/rp1-gpclk-route-manager.socket |
        sha256sum --check --status || fail "installed route-manager socket unit identity mismatch"
    printf '%s  %s\n' "$EXPECTED_SERVICE_UNIT_SHA256" /usr/lib/systemd/system/rp1-gpclk-route-manager@.service |
        sha256sum --check --status || fail "installed route-manager service unit identity mismatch"

    service_account=$(systemctl show wsprrypi.service --property=User --value)
    [[ -n "$service_account" ]] || service_account=root
    [[ "$service_account" =~ ^[a-z_][a-z0-9_-]*$ ]] ||
        fail "the fixed WsprryPi service account is invalid"
    if [[ "$service_account" != root ]] &&
        ! id -nG "$service_account" | tr ' ' '\n' | grep -Fxq rp1-gpclk-route; then
        usermod -a -G rp1-gpclk-route "$service_account"
    fi

    systemctl enable --now rp1-gpclk-route-manager.socket
    systemctl is-enabled --quiet rp1-gpclk-route-manager.socket ||
        fail "route-manager socket was not enabled"
    systemctl is-active --quiet rp1-gpclk-route-manager.socket ||
        fail "route-manager socket is not active"
    socket_identity=$(stat -c '%U:%G' /run/rp1-gpclk-dkms/route-manager.sock)
    socket_mode=$(stat -c '%a' /run/rp1-gpclk-dkms/route-manager.sock)
    [[ "$socket_identity" == "root:rp1-gpclk-route" && "$socket_mode" == "660" ]] ||
        fail "route-manager socket ownership or mode differs from root:rp1-gpclk-route 0660"
    printf 'RP1 GPCLK route-manager enrollment: PASS account=%s\n' "$service_account"
}

require_opt_in
validate_target

case "$MODE" in
    check)
        validate_headers
        printf 'RP1 GPCLK prerequisite check: PASS\n'
        printf 'kernel=%s architecture=%s headers=%s\n' \
            "$KERNEL_RELEASE" "$ARCHITECTURE" "$HEADER_PACKAGE"
        ;;
    install)
        install_package
        ;;
    enroll)
        enroll_route_manager
        ;;
    *)
        fail "usage: $0 {check|install|enroll}"
        ;;
esac
