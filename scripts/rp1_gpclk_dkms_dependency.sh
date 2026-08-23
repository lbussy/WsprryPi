#!/usr/bin/env bash
# Immutable optional consumer contract for RP1-GPCLK-DKMS v1.1.1.
# Installs only the product package; never qualification content, overlay
# activation, or module loading.

# shellcheck disable=SC2034
readonly RP1_GPCLK_DKMS_TAG="v1.1.1"
readonly RP1_GPCLK_DKMS_VERSION="1.1.1-1"
readonly RP1_GPCLK_DKMS_PACKAGE="rp1-gpclk-dkms_1.1.1-1_all.deb"
readonly RP1_GPCLK_DKMS_SHA256="247bd7da35e4ad812a13828668fe03673da127bad7ed2b3e970876f3f21c002d"
readonly RP1_GPCLK_DKMS_UAPI_SHA256="998ab96d7dbcc0d935c05758c46acba56bbcf92aa1b674b899bdab6932dc8384"
readonly RP1_GPCLK_ROUTE_EXECUTOR_SHA256="a1e247df88650cad0866cc37f946a2859e0594e457a03f8674f6a691901be2da"
readonly RP1_GPCLK_ROUTE_SCHEMA_SHA256="097762cf365e864162b1199bceb05d2937b719ddf426d3a90b6a7f680803251b"
readonly RP1_GPCLK_ROUTE_DOCUMENTATION_SHA256="b5936885ee3cddaeaf0b21590a0657a73f8d9b1dc1b26fd1b2fbcd2afa043f25"
readonly RP1_GPCLK_ROUTE_SOCKET_UNIT_SHA256="336f2ca703ab95b4d124d643f9b08b939ec055b11c7f4bb573207f4cb99b4068"
readonly RP1_GPCLK_ROUTE_SERVICE_UNIT_SHA256="0d14b1ba451af698d831cb4fa342ec046d513391569eacbe3c98b8fd9104e3ce"
readonly RP1_GPCLK_MEMBER_INVENTORY_SHA256="888807e4b14dffda75c20e264671d2cfe41437612ec76093618224940a698d70"
readonly RP1_GPCLK_INVENTORY_DOCUMENT_SHA256="e38d5ddebf516a313033fbdf41e01dc753fab78580a9119f08c388a69c17ac32"
readonly RP1_GPCLK_GPIO4_OVERLAY_SHA256="c3e17a685694928468bb18c24f5bb4e25454745d6989e6c9d2c2acf447b908d6"
readonly RP1_GPCLK_GPIO20_OVERLAY_SHA256="8eaa8afae7f88a665fc9bec6da1b013be049b2a32c909c729caeff9181bcf3aa"
readonly RP1_GPCLK_DKMS_URL="https://github.com/WsprryPi/RP1-GPCLK-DKMS/releases/download/v1.1.1/rp1-gpclk-dkms_1.1.1-1_all.deb"
readonly RP1_GPCLK_DKMS_UAPI_PATH="/usr/src/rp1-gpclk-dkms-1.1.1/include/uapi/linux/rp1_gpclk.h"
readonly RP1_GPCLK_ROUTE_EXECUTOR_PATH="/usr/sbin/rp1-gpclk-route-manager"
readonly RP1_GPCLK_ROUTE_SCHEMA_PATH="/usr/share/rp1-gpclk-dkms/1.1.1/rp1-gpclk-route-manager-v1.schema.json"
readonly RP1_GPCLK_ROUTE_DOCUMENTATION_PATH="/usr/share/doc/rp1-gpclk-dkms/route-manager-v1.md"
readonly RP1_GPCLK_ROUTE_SOCKET_UNIT_PATH="/usr/lib/systemd/system/rp1-gpclk-route-manager.socket"
readonly RP1_GPCLK_ROUTE_SERVICE_UNIT_PATH="/usr/lib/systemd/system/rp1-gpclk-route-manager@.service"
readonly RP1_GPCLK_GPIO4_OVERLAY="/usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio4.dtbo"
readonly RP1_GPCLK_GPIO20_OVERLAY="/usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio20.dtbo"
RP1_GPCLK_CONTRACT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly RP1_GPCLK_CONTRACT_DIR
readonly INSTALL_RP1_GPCLK_DKMS="${INSTALL_RP1_GPCLK_DKMS:-false}"
readonly REMOVE_RP1_GPCLK_DKMS="${REMOVE_RP1_GPCLK_DKMS:-false}"
readonly RP1_GPCLK_COMPATIBLE_FILE="${RP1_GPCLK_COMPATIBLE_FILE:-}"
readonly RP1_GPCLK_MODEL_FILE="${RP1_GPCLK_MODEL_FILE:-}"

rp1_gpclk_sha256() { sha256sum "$1" | awk '{print $1}'; }

rp1_gpclk_require_pi5() {
    local compatible_file="$RP1_GPCLK_COMPATIBLE_FILE" model_file="$RP1_GPCLK_MODEL_FILE"
    [[ -n "$compatible_file" ]] || compatible_file=/proc/device-tree/compatible
    [[ -n "$model_file" ]] || model_file=/proc/device-tree/model
    local compatible model
    compatible=$(tr '\0' '\n' <"$compatible_file" 2>/dev/null || true)
    model=$(tr -d '\0' <"$model_file" 2>/dev/null || true)
    if ! grep -Fxq "brcm,bcm2712" <<<"$compatible" ||
        [[ "$model" != Raspberry\ Pi\ 5* && "$model" != Raspberry\ Pi\ Compute\ Module\ 5* ]]; then
        printf 'RP1 GPCLK DKMS is restricted to a confirmed Raspberry Pi 5/CM5 (BCM2712). Detected: %s\n' "$model" >&2
        return 1
    fi
}

rp1_gpclk_validate_deb_inventory() {
    local package="$1" temp_dir="$2" required product_inventory
    [[ "$(ar t "$package")" == $'debian-binary\ncontrol.tar.xz\ndata.tar.xz' ]] || {
        printf 'Unexpected RP1 GPCLK Debian ar inventory.\n' >&2; return 1;
    }
    (cd "$temp_dir" && ar x "$package")
    if ! diff -u "$RP1_GPCLK_CONTRACT_DIR/rp1_gpclk_dkms_control_inventory.txt" <(tar -tf "$temp_dir/control.tar.xz"); then
        printf 'Pinned RP1 GPCLK control inventory mismatch.\n' >&2
        return 1
    fi
    product_inventory=$(tar -tf "$temp_dir/data.tar.xz") || return 1
    if ! diff -u "$RP1_GPCLK_CONTRACT_DIR/rp1_gpclk_dkms_product_inventory.txt" <(printf '%s\n' "$product_inventory"); then
        printf 'Pinned RP1 GPCLK product inventory mismatch.\n' >&2
        return 1
    fi
    for required in         "./usr/src/rp1-gpclk-dkms-1.1.1/include/uapi/linux/rp1_gpclk.h"         "./usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio4.dtbo"         "./usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio20.dtbo"         "./usr/sbin/rp1-gpclk-route-manager"         "./usr/share/rp1-gpclk-dkms/1.1.1/rp1-gpclk-route-manager-v1.schema.json"
    do
        grep -Fxq "$required" <<<"$product_inventory" || {
            printf 'Pinned package member missing: %s\n' "$required" >&2; return 1;
        }
    done
    if grep -Eiq 'qualification|evidence|gate[_-]d|target[_-]verification' <<<"$product_inventory"; then
        printf 'Qualification-only content leaked into the product package.\n' >&2; return 1
    fi
    tar -xJf "$temp_dir/data.tar.xz" -C "$temp_dir"         ./usr/src/rp1-gpclk-dkms-1.1.1/include/uapi/linux/rp1_gpclk.h         ./usr/sbin/rp1-gpclk-route-manager         ./usr/share/rp1-gpclk-dkms/1.1.1/rp1-gpclk-route-manager-v1.schema.json
    [[ "$(rp1_gpclk_sha256 "$temp_dir$RP1_GPCLK_DKMS_UAPI_PATH")" == "$RP1_GPCLK_DKMS_UAPI_SHA256" ]] || {
        printf 'Pinned package UAPI identity mismatch.\n' >&2; return 1;
    }
    [[ "$(rp1_gpclk_sha256 "$temp_dir/usr/sbin/rp1-gpclk-route-manager")" == "$RP1_GPCLK_ROUTE_EXECUTOR_SHA256" ]] || return 1
    [[ "$(rp1_gpclk_sha256 "$temp_dir/usr/share/rp1-gpclk-dkms/1.1.1/rp1-gpclk-route-manager-v1.schema.json")" == "$RP1_GPCLK_ROUTE_SCHEMA_SHA256" ]] || return 1
}

rp1_gpclk_rooted_path() {
    local root="$1" path="$2"
    if [[ "$root" == "/" ]]; then
        printf '%s\n' "$path"
    else
        printf '%s%s\n' "${root%/}" "$path"
    fi
}

rp1_gpclk_module_signing_policy() {
    local root="$1" kernel_release="$2" value config cmdline
    cmdline=$(rp1_gpclk_rooted_path "$root" /proc/cmdline)
    if [[ -r "$cmdline" ]] && grep -Eq '(^|[[:space:]])module\.sig_enforce=1($|[[:space:]])' "$cmdline"; then
        printf 'required\n'
        return 0
    fi
    value=$(rp1_gpclk_rooted_path "$root" /proc/sys/kernel/module_sig_enforce)
    if [[ -r "$value" ]]; then
        case "$(tr -d '[:space:]' <"$value")" in
            1) printf 'required\n'; return 0 ;;
            0) printf 'not-required\n'; return 0 ;;
        esac
    fi
    for config in \
        "$(rp1_gpclk_rooted_path "$root" "/boot/config-${kernel_release}")" \
        "$(rp1_gpclk_rooted_path "$root" "/lib/modules/${kernel_release}/build/.config")"
    do
        if [[ -r "$config" ]] && grep -Fxq 'CONFIG_MODULE_SIG_FORCE=y' "$config"; then
            printf 'required\n'
            return 0
        fi
        if [[ -r "$config" ]] && grep -Eq '^(# CONFIG_MODULE_SIG is not set|# CONFIG_MODULE_SIG_FORCE is not set|CONFIG_MODULE_SIG_FORCE=n)$' "$config"; then
            printf 'not-required\n'
            return 0
        fi
    done
    printf 'unknown\n'
}

rp1_gpclk_require_current_kernel_headers() {
    local root="${1:-/}" kernel_release="${2:-$(uname -r)}" headers
    headers=$(rp1_gpclk_rooted_path "$root" "/lib/modules/${kernel_release}/build")
    [[ -f "$headers/Makefile" && -f "$headers/Module.symvers" ]] || {
        printf 'Installed kernel headers are incomplete for %s.\n' "$kernel_release" >&2
        return 1
    }
}

rp1_gpclk_verify_installed_product() {
    local root="${1:-/}" kernel_release="${2:-$(uname -r)}"
    local uapi executor schema documentation socket_unit service_unit gpio4 gpio20 dkms_status module_path module_version signer signing_policy
    uapi=$(rp1_gpclk_rooted_path "$root" "$RP1_GPCLK_DKMS_UAPI_PATH")
    executor=$(rp1_gpclk_rooted_path "$root" "$RP1_GPCLK_ROUTE_EXECUTOR_PATH")
    schema=$(rp1_gpclk_rooted_path "$root" "$RP1_GPCLK_ROUTE_SCHEMA_PATH")
    documentation=$(rp1_gpclk_rooted_path "$root" "$RP1_GPCLK_ROUTE_DOCUMENTATION_PATH")
    socket_unit=$(rp1_gpclk_rooted_path "$root" "$RP1_GPCLK_ROUTE_SOCKET_UNIT_PATH")
    service_unit=$(rp1_gpclk_rooted_path "$root" "$RP1_GPCLK_ROUTE_SERVICE_UNIT_PATH")
    gpio4=$(rp1_gpclk_rooted_path "$root" "$RP1_GPCLK_GPIO4_OVERLAY")
    gpio20=$(rp1_gpclk_rooted_path "$root" "$RP1_GPCLK_GPIO20_OVERLAY")

    [[ "$(dpkg-query -W -f='$''{Version}' rp1-gpclk-dkms 2>/dev/null)" == "$RP1_GPCLK_DKMS_VERSION" ]] || {
        printf 'Installed RP1 GPCLK package version mismatch.\n' >&2; return 1;
    }
    [[ -f "$uapi" ]] && [[ "$(rp1_gpclk_sha256 "$uapi")" == "$RP1_GPCLK_DKMS_UAPI_SHA256" ]] || {
        printf 'Installed RP1 GPCLK UAPI identity mismatch.\n' >&2; return 1;
    }
    [[ -x "$executor" ]] && [[ "$(rp1_gpclk_sha256 "$executor")" == "$RP1_GPCLK_ROUTE_EXECUTOR_SHA256" ]] || {
        printf 'Installed RP1 GPCLK route executor identity mismatch.\n' >&2; return 1;
    }
    [[ -f "$schema" ]] && [[ "$(rp1_gpclk_sha256 "$schema")" == "$RP1_GPCLK_ROUTE_SCHEMA_SHA256" ]] || {
        printf 'Installed RP1 GPCLK route schema identity mismatch.\n' >&2; return 1;
    }
    [[ -f "$documentation" ]] && [[ "$(rp1_gpclk_sha256 "$documentation")" == "$RP1_GPCLK_ROUTE_DOCUMENTATION_SHA256" ]] || {
        printf 'Installed RP1 GPCLK route documentation identity mismatch.\n' >&2; return 1;
    }
    [[ -f "$socket_unit" ]] && [[ "$(rp1_gpclk_sha256 "$socket_unit")" == "$RP1_GPCLK_ROUTE_SOCKET_UNIT_SHA256" ]] || {
        printf 'Installed RP1 GPCLK route socket unit identity mismatch.\n' >&2; return 1;
    }
    [[ -f "$service_unit" ]] && [[ "$(rp1_gpclk_sha256 "$service_unit")" == "$RP1_GPCLK_ROUTE_SERVICE_UNIT_SHA256" ]] || {
        printf 'Installed RP1 GPCLK route service unit identity mismatch.\n' >&2; return 1;
    }
    [[ -f "$gpio4" ]] && [[ "$(rp1_gpclk_sha256 "$gpio4")" == "$RP1_GPCLK_GPIO4_OVERLAY_SHA256" ]] || {
        printf 'Installed RP1 GPCLK GPIO4 overlay identity mismatch.\n' >&2; return 1;
    }
    [[ -f "$gpio20" ]] && [[ "$(rp1_gpclk_sha256 "$gpio20")" == "$RP1_GPCLK_GPIO20_OVERLAY_SHA256" ]] || {
        printf 'Installed RP1 GPCLK GPIO20 overlay identity mismatch.\n' >&2; return 1;
    }
    rp1_gpclk_require_current_kernel_headers "$root" "$kernel_release" || return 1

    dkms_status=$(dkms status -m rp1-gpclk-dkms -v 1.1.1 -k "$kernel_release" 2>/dev/null) || {
        printf 'Unable to query RP1 GPCLK DKMS status for %s.\n' "$kernel_release" >&2; return 1;
    }
    grep -Eq '^rp1-gpclk-dkms/1\.1\.1, [^,]+(, [^:]+)?: installed$' <<<"$dkms_status" || {
        printf 'RP1 GPCLK DKMS is not installed for %s: %s\n' "$kernel_release" "$dkms_status" >&2; return 1;
    }

    module_path=$(modinfo -k "$kernel_release" -n rp1_gpclk_dkms 2>/dev/null) || {
        printf 'Unable to locate the installed RP1 GPCLK module for %s.\n' "$kernel_release" >&2; return 1;
    }
    [[ -f "$module_path" && "$module_path" == */lib/modules/"$kernel_release"/updates/dkms/rp1_gpclk_dkms.ko* ]] || {
        printf 'Unexpected RP1 GPCLK installed module path: %s\n' "$module_path" >&2; return 1;
    }
    module_version=$(modinfo -k "$kernel_release" -F version rp1_gpclk_dkms 2>/dev/null) || return 1
    [[ "$module_version" == "1.1.1" ]] || {
        printf 'Installed RP1 GPCLK module version mismatch: %s\n' "$module_version" >&2; return 1;
    }
    signer=$(modinfo -k "$kernel_release" -F signer rp1_gpclk_dkms 2>/dev/null) || return 1
    signing_policy=$(rp1_gpclk_module_signing_policy "$root" "$kernel_release")
    case "$signing_policy" in
        required)
            if [[ -z "$signer" ]]; then
                printf 'RP1 GPCLK module is unsigned while signature enforcement is active.\n' >&2
                return 1
            fi
            ;;
        not-required) ;;
        *)
            printf 'Unable to determine the kernel module-signing enforcement policy.\n' >&2
            return 1
            ;;
    esac
    if [[ -n "$signer" ]]; then
        printf 'Verified RP1 GPCLK DKMS %s for %s; module signer: %s. This installation did not activate either route overlay.\n' \
            "$RP1_GPCLK_DKMS_VERSION" "$kernel_release" "$signer"
    else
        printf 'Verified RP1 GPCLK DKMS %s for %s; the module is unsigned and signature enforcement is inactive. This installation did not activate either route overlay.\n' \
            "$RP1_GPCLK_DKMS_VERSION" "$kernel_release"
    fi
}

install_optional_rp1_gpclk_dkms() {
    [[ "$INSTALL_RP1_GPCLK_DKMS" == "true" ]] || return 0
    rp1_gpclk_require_pi5 || return 1
    if [[ "${DRY_RUN:-false}" == "true" ]]; then
        printf 'Dry run: would verify current-kernel headers, download, verify, install, and validate pinned RP1 GPCLK DKMS %s without activating either route overlay.\n' "$RP1_GPCLK_DKMS_VERSION"
        return 0
    fi
    local temp_dir package
    rp1_gpclk_require_current_kernel_headers || return 1
    temp_dir=$(mktemp -d "/tmp/wsprrypi-rp1-gpclk.XXXXXX")
    RP1_GPCLK_TEMP_DIR="$temp_dir"
    trap 'rm -rf -- "${RP1_GPCLK_TEMP_DIR:-}"' EXIT HUP INT TERM
    package="$temp_dir/$RP1_GPCLK_DKMS_PACKAGE"
    curl -fL --proto '=https' --tlsv1.2 -o "$package" "$RP1_GPCLK_DKMS_URL"
    [[ "$(rp1_gpclk_sha256 "$package")" == "$RP1_GPCLK_DKMS_SHA256" ]] || {
        printf 'RP1 GPCLK package checksum mismatch.\n' >&2; return 1;
    }
    rp1_gpclk_validate_deb_inventory "$package" "$temp_dir" || return 1
    apt-get install -y "$package" || return 1
    rp1_gpclk_verify_installed_product || return 1
    rm -rf "$temp_dir"
    RP1_GPCLK_TEMP_DIR=
    trap - EXIT HUP INT TERM
}

remove_optional_rp1_gpclk_dkms() {
    [[ "$REMOVE_RP1_GPCLK_DKMS" == "true" ]] || return 0
    rp1_gpclk_require_pi5 || return 1
    if [[ "${DRY_RUN:-false}" == "true" ]]; then
        printf 'Dry run: would remove only pinned RP1 GPCLK DKMS %s.\n' "$RP1_GPCLK_DKMS_VERSION"
        return 0
    fi
    local version
    version=$(dpkg-query -W -f='$''{Version}' rp1-gpclk-dkms 2>/dev/null || true)
    [[ "$version" == "$RP1_GPCLK_DKMS_VERSION" ]] || {
        printf 'Refusing to remove unrecognized RP1 GPCLK package version: %s\n' "$version" >&2; return 1;
    }
    apt-get remove -y rp1-gpclk-dkms
}
