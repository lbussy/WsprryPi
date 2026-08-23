#!/usr/bin/env bash
# shellcheck disable=SC2016
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
HELPER="$ROOT/scripts/rp1_gpclk_dkms_dependency.sh"
CANONICAL="$ROOT/src/WSPR-Transmitter/src/rp1_gpclk_uapi.h"

grep -Fq 'RP1_GPCLK_DKMS_TAG="v1.1.1"' "$HELPER"
grep -Fq '247bd7da35e4ad812a13828668fe03673da127bad7ed2b3e970876f3f21c002d' "$HELPER"
for superseded in \
    48d55aa9a906e83b36ed46560c81cd894024bc2d6bf375514b5e1618a43493af \
    26cdbe212898cd8a53aba6024acd05f9addb56f521af39e10f68066c8a94e7cb \
    642793e04268ddb06e35f16249d09c98e4067acef93c62620307bbea50033f5a
do
    if grep -Fq "$superseded" "$HELPER"; then
        echo "superseded package hash accepted by current dependency helper" >&2
        exit 1
    fi
done
grep -Fq 'https://github.com/WsprryPi/RP1-GPCLK-DKMS/releases/download/v1.1.1/rp1-gpclk-dkms_1.1.1-1_all.deb' "$HELPER"
grep -Fq '/usr/src/rp1-gpclk-dkms-1.1.1/include/uapi/linux/rp1_gpclk.h' "$HELPER"
grep -Fq '/usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio4.dtbo' "$HELPER"
grep -Fq '/usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio20.dtbo' "$HELPER"
grep -Fq 'c3e17a685694928468bb18c24f5bb4e25454745d6989e6c9d2c2acf447b908d6' "$HELPER"
grep -Fq '8eaa8afae7f88a665fc9bec6da1b013be049b2a32c909c729caeff9181bcf3aa' "$HELPER"
if grep -Fq 'RP1_GPCLK_ROUTE=' "$HELPER"; then
    echo "package installation must not require or consume a route" >&2
    exit 1
fi
grep -Fq 'product_inventory=$(tar -tf "$temp_dir/data.tar.xz")' "$HELPER"
grep -Fq 'grep -Fxq "$required" <<<"$product_inventory"' "$HELPER"
grep -Fq '${RP1_GPCLK_TEMP_DIR:-}' "$HELPER"
if grep -Eiq 'rp1-gpclk-dkms-qualification|c05f2f2a' "$HELPER"; then
    echo "qualification artifact leaked into product helper" >&2
    exit 1
fi
[[ "$(shasum -a 256 "$CANONICAL" | awk '{print $1}')" == "998ab96d7dbcc0d935c05758c46acba56bbcf92aa1b674b899bdab6932dc8384" ]]

fixture=$(mktemp -d)
trap 'rm -rf "$fixture"' EXIT
printf 'brcm,bcm2712\0raspberrypi,5-model-b\0' >"$fixture/compatible"
printf 'Raspberry Pi 5 Model B Rev 1.0\0' >"$fixture/model"

env INSTALL_RP1_GPCLK_DKMS=true DRY_RUN=true \
    RP1_GPCLK_COMPATIBLE_FILE="$fixture/compatible" \
    RP1_GPCLK_MODEL_FILE="$fixture/model" \
    bash -c 'source "$1"; install_optional_rp1_gpclk_dkms' _ "$HELPER" |
    grep -Fq 'without activating either route overlay'

# A legacy route variable must not influence route-neutral package installation.
env INSTALL_RP1_GPCLK_DKMS=true RP1_GPCLK_ROUTE=GPIO21 DRY_RUN=true \
    RP1_GPCLK_COMPATIBLE_FILE="$fixture/compatible" \
    RP1_GPCLK_MODEL_FILE="$fixture/model" \
    bash -c 'source "$1"; install_optional_rp1_gpclk_dkms' _ "$HELPER" |
    grep -Fq 'without activating either route overlay'

printf 'brcm,bcm2711\0raspberrypi,4-model-b\0' >"$fixture/compatible"
printf 'Raspberry Pi 4 Model B Rev 1.5\0' >"$fixture/model"
if env INSTALL_RP1_GPCLK_DKMS=true DRY_RUN=true \
    RP1_GPCLK_COMPATIBLE_FILE="$fixture/compatible" \
    RP1_GPCLK_MODEL_FILE="$fixture/model" \
    bash -c 'source "$1"; install_optional_rp1_gpclk_dkms' _ "$HELPER"; then
    echo "non-Pi-5 unexpectedly accepted" >&2; exit 1
fi

env INSTALL_RP1_GPCLK_DKMS=false bash -c 'source "$1"; install_optional_rp1_gpclk_dkms' _ "$HELPER"

# Exercise post-install verification with filesystem and command fixtures only.
verify_root="$fixture/root"
kernel_release=6.18.34+rpt-rpi-2712
mkdir -p \
    "$verify_root/usr/src/rp1-gpclk-dkms-1.1.1/include/uapi/linux" \
    "$verify_root/usr/lib/rp1-gpclk-dkms/overlays" \
    "$verify_root/usr/sbin" \
    "$verify_root/usr/share/rp1-gpclk-dkms/1.1.1" \
    "$verify_root/usr/share/doc/rp1-gpclk-dkms" \
    "$verify_root/usr/lib/systemd/system" \
    "$verify_root/lib/modules/$kernel_release/build" \
    "$verify_root/lib/modules/$kernel_release/updates/dkms" \
    "$verify_root/proc/sys/kernel"
touch \
    "$verify_root/usr/src/rp1-gpclk-dkms-1.1.1/include/uapi/linux/rp1_gpclk.h" \
    "$verify_root/usr/sbin/rp1-gpclk-route-manager" \
    "$verify_root/usr/share/rp1-gpclk-dkms/1.1.1/rp1-gpclk-route-manager-v1.schema.json" \
    "$verify_root/usr/share/doc/rp1-gpclk-dkms/route-manager-v1.md" \
    "$verify_root/usr/lib/systemd/system/rp1-gpclk-route-manager.socket" \
    "$verify_root/usr/lib/systemd/system/rp1-gpclk-route-manager@.service" \
    "$verify_root/usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio4.dtbo" \
    "$verify_root/usr/lib/rp1-gpclk-dkms/overlays/rp1-gpclk-gpio20.dtbo" \
    "$verify_root/lib/modules/$kernel_release/build/Makefile" \
    "$verify_root/lib/modules/$kernel_release/build/Module.symvers" \
    "$verify_root/lib/modules/$kernel_release/updates/dkms/rp1_gpclk_dkms.ko"
printf '0\n' >"$verify_root/proc/sys/kernel/module_sig_enforce"
chmod 0755 "$verify_root/usr/sbin/rp1-gpclk-route-manager"

# shellcheck source=scripts/rp1_gpclk_dkms_dependency.sh
source "$HELPER"
dpkg-query() { printf '1.1.1-1\n'; }
dkms() { printf 'rp1-gpclk-dkms/1.1.1, %s, arm64: installed\n' "$kernel_release"; }
modinfo() {
    if [[ "$*" == *" -n "* ]]; then
        printf '%s\n' "$verify_root/lib/modules/$kernel_release/updates/dkms/rp1_gpclk_dkms.ko"
    elif [[ "$*" == *" -F version "* ]]; then
        printf '1.1.1\n'
    elif [[ "$*" == *" -F signer "* ]]; then
        printf '%s\n' "${TEST_MODULE_SIGNER:-}"
    else
        return 1
    fi
}
rp1_gpclk_sha256() {
    case "$1" in
        */rp1_gpclk.h) printf '%s\n' "$RP1_GPCLK_DKMS_UAPI_SHA256" ;;
        */rp1-gpclk-route-manager) printf '%s\n' "$RP1_GPCLK_ROUTE_EXECUTOR_SHA256" ;;
        */rp1-gpclk-route-manager-v1.schema.json) printf '%s\n' "$RP1_GPCLK_ROUTE_SCHEMA_SHA256" ;;
        */route-manager-v1.md) printf '%s\n' "$RP1_GPCLK_ROUTE_DOCUMENTATION_SHA256" ;;
        */rp1-gpclk-route-manager.socket) printf '%s\n' "$RP1_GPCLK_ROUTE_SOCKET_UNIT_SHA256" ;;
        */rp1-gpclk-route-manager@.service) printf '%s\n' "$RP1_GPCLK_ROUTE_SERVICE_UNIT_SHA256" ;;
        */rp1-gpclk-gpio4.dtbo) printf '%s\n' "$RP1_GPCLK_GPIO4_OVERLAY_SHA256" ;;
        */rp1-gpclk-gpio20.dtbo) printf '%s\n' "$RP1_GPCLK_GPIO20_OVERLAY_SHA256" ;;
        *) return 1 ;;
    esac
}

rp1_gpclk_verify_installed_product "$verify_root" "$kernel_release" |
    grep -Fq 'signature enforcement is inactive'
rm "$verify_root/lib/modules/$kernel_release/build/Module.symvers"
if rp1_gpclk_require_current_kernel_headers "$verify_root" "$kernel_release"; then
    echo "incomplete current-kernel headers unexpectedly passed" >&2
    exit 1
fi
touch "$verify_root/lib/modules/$kernel_release/build/Module.symvers"
printf '1\n' >"$verify_root/proc/sys/kernel/module_sig_enforce"
if rp1_gpclk_verify_installed_product "$verify_root" "$kernel_release"; then
    echo "unsigned module unexpectedly passed enforced signing policy" >&2
    exit 1
fi
TEST_MODULE_SIGNER='RP1 GPCLK Test Signer'
rp1_gpclk_verify_installed_product "$verify_root" "$kernel_release" |
    grep -Fq 'module signer: RP1 GPCLK Test Signer'
rm "$verify_root/proc/sys/kernel/module_sig_enforce"
if rp1_gpclk_verify_installed_product "$verify_root" "$kernel_release"; then
    echo "unknown module-signing policy unexpectedly passed" >&2
    exit 1
fi
printf '# CONFIG_MODULE_SIG_FORCE is not set\n' >"$verify_root/lib/modules/$kernel_release/build/.config"
rp1_gpclk_verify_installed_product "$verify_root" "$kernel_release" |
    grep -Fq 'module signer: RP1 GPCLK Test Signer'
TEST_MODULE_SIGNER=
printf '# CONFIG_MODULE_SIG is not set\n' >"$verify_root/lib/modules/$kernel_release/build/.config"
rp1_gpclk_verify_installed_product "$verify_root" "$kernel_release" |
    grep -Fq 'signature enforcement is inactive'

echo "RP1 GPCLK release dependency tests: PASS"
