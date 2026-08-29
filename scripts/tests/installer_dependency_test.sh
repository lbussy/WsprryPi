#!/usr/bin/env bash
# shellcheck disable=SC2016
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
INSTALLER="${SCRIPT_DIR}/../install.sh"

if grep -Eq '^[[:space:]]*22m([[:space:]]|Active High[[:space:]])*=' "${SCRIPT_DIR}/../../config/wsprrypi.ini"; then
    echo "the canonical INI must not contain retired 22m Band GPIO keys" >&2
    exit 1
fi

stock_capture_line="$(grep -nF 'stock_source_path="$source_path"' "$INSTALLER" | head -1 | cut -d: -f1)"
merge_switch_line="$(grep -nF 'source_path="$merged_ini"' "$INSTALLER" | head -1 | cut -d: -f1)"

if [[ -z "$stock_capture_line" || -z "$merge_switch_line" || "$stock_capture_line" -ge "$merge_switch_line" ]]; then
    echo "install.sh must retain the current canonical INI before selecting merged upgrade output" >&2
    exit 1
fi

if ! grep -Fq 'cp -f "${stock_source_path}" "${stock_config_path}"' "$INSTALLER"; then
    echo "install.sh must install .stock from the untouched current canonical INI" >&2
    exit 1
fi

if ! grep -Fq 'upgrade_ini "$old_path" "$source_path" "$merged_ini"' "$INSTALLER"; then
    echo "install.sh upgrades must merge old values into the current canonical INI schema" >&2
    exit 1
fi

python3 "${SCRIPT_DIR}/ini_upgrade_schema_test.py"

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

if grep -Eiq 'live_output|kernel_2712_phase|dtoverlay=rp1' "$INSTALLER"; then
    echo "standard installer must not deploy historical experimental RP1 paths" >&2
    exit 1
fi
if grep -Eiq 'qualification.*(tar|download|install)|dtoverlay=rp1|modprobe[[:space:]]+rp1' "$INSTALLER"; then
    echo "standard installer must not consume qualification content or activate the RP1 path" >&2
    exit 1
fi

if ! grep -Fq 'exec_command "Publish validated UI artifact"' "$INSTALLER"; then
    echo "install.sh must publish the UI through the validated artifact publisher" >&2
    exit 1
fi

if ! grep -Fq -- '--source-commit "$source_commit"' "$INSTALLER" ||
    ! grep -Fq -- '--application-version "$SEM_VER"' "$INSTALLER"; then
    echo "install.sh must bind the UI manifest to the exact source commit and application version" >&2
    exit 1
fi

for required in \
    '"--fail-on-ui-modifications 0 set_fail_on_ui_modifications' \
    '--result-file "$UI_PUBLICATION_RESULT_FILE"' \
    'publisher_args+=(--fail-on-ui-modifications)' \
    'report_ui_publication_result' \
    'trap egress EXIT'
do
    if ! grep -Fq -- "$required" "$INSTALLER"; then
        echo "install.sh is missing UI modification/reporting contract: $required" >&2
        exit 1
    fi
done

if ! awk '/egress\(\)/,/^}/' "$INSTALLER" | grep -Fq 'report_ui_publication_result'; then
    echo "the UI replacement report must be emitted from the final EXIT trap" >&2
    exit 1
fi

if awk '/manage_web\(\)/,/^}/' "$INSTALLER" | grep -Eq 'cp -r .*source_path|chown -R .*target_path'; then
    echo "manage_web must not copy or mutate the live UI tree incrementally" >&2
    exit 1
fi

echo "installer dependency tests: PASS"
