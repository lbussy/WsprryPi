#!/usr/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
COLLECTOR="${SCRIPT_DIR}/../collect-support-bundle.sh"
TEST_ROOT="$(mktemp -d)"
trap 'rm -rf "$TEST_ROOT"' EXIT

fail() { echo "FAIL: $*" >&2; exit 1; }
assert_contains() { grep -Fq -- "$2" "$1" || fail "expected '$2' in $1"; }
assert_not_contains() { ! grep -Fq -- "$2" "$1" || fail "did not expect '$2' in $1"; }
command -v python3 >/dev/null 2>&1 || fail "python3 is required for JSON result validation"

assert_valid_json() {
  python3 - "$1" <<'PY'
import json
import sys
with open(sys.argv[1], encoding="utf-8") as source:
    json.load(source)
PY
}

assert_failure_result() {
  python3 - "$1" <<'PY'
import json
import sys
with open(sys.argv[1], encoding="utf-8") as source:
    result = json.load(source)
assert result["status"] == "failure"
assert result["archive_filename"] is None
assert result["sha256_filename"] is None
assert result["sha256"] is None
PY
}

assert_no_workdirs() {
  local leaked
  leaked="$(find "$TEST_ROOT/tmp" -mindepth 1 -maxdepth 1 -type d -name 'WsprryPi-support-20260102T030405Z.*' -print -quit)"
  [[ -z "$leaked" ]] || fail "collector work directory was retained: $leaked"
}

make_mocks() {
  local directory="$1" probe_exit="$2"
  mkdir -p "$directory"
  cat > "$directory/date" <<'EOF'
#!/usr/bin/bash
if [[ "$*" == *'+%Y%m%dT%H%M%SZ'* ]]; then printf '20260102T030405Z\n'; else /usr/bin/date "$@"; fi
EOF
  cat > "$directory/hostname" <<'EOF'
#!/usr/bin/bash
printf 'test-host\n'
EOF
  cat > "$directory/i2cdetect" <<EOF
#!/usr/bin/bash
printf '%s\\n' "\$*" >> "\${I2C_CALL_LOG:?}"
if [[ "\$1" == '-l' ]]; then exit 0; fi
exit ${probe_exit}
EOF
  for command in raspi-config vcgencmd gpioinfo gpiodetect gpiofind pinout gpio curl systemctl journalctl dmesg sh; do
    cat > "$directory/$command" <<'EOF'
#!/usr/bin/env bash
exit 0
EOF
  done
  chmod 700 "$directory"/*
}

make_no_i2c_path() {
  local directory="$1" command
  mkdir -p "$directory"
  for command in awk basename cat chmod cp cut df dirname file find free getconf getent grep gzip id ldd ln ls mkdir mktemp mount perl readelf rm sha256sum sh sort stat tail tar uname uptime; do
    ln -s "$(command -v "$command")" "$directory/$command"
  done
  rm "$directory/sh"
  cp "$TEST_ROOT/mocks/date" "$directory/date"
  cp "$TEST_ROOT/mocks/hostname" "$directory/hostname"
  for command in raspi-config vcgencmd gpioinfo gpiodetect gpiofind pinout gpio curl systemctl journalctl dmesg sh; do
    cp "$TEST_ROOT/mocks/$command" "$directory/$command"
  done
}

run_collector() {
  local output="$1" mocks="$2" i2c_log="$3"
  shift 3
  ( cd / && I2C_CALL_LOG="$i2c_log" TMPDIR="$TEST_ROOT/tmp" PATH="$mocks:$PATH" bash "$COLLECTOR" --output-dir "$output" "$@" )
}

run_collector_isolated() {
  local output="$1" mocks="$2" i2c_log="$3"
  shift 3
  ( cd / && I2C_CALL_LOG="$i2c_log" TMPDIR="$TEST_ROOT/tmp" PATH="$mocks" /usr/bin/bash "$COLLECTOR" --output-dir "$output" "$@" )
}

mkdir -p "$TEST_ROOT/tmp" "$TEST_ROOT/out" "$TEST_ROOT/mocks"
chmod 700 "$TEST_ROOT/out"
make_mocks "$TEST_ROOT/mocks" 0

# Default: no active bus scan, stable artifacts, and no work tree remains.
: > "$TEST_ROOT/default-i2c.log"
run_collector "$TEST_ROOT/out" "$TEST_ROOT/mocks" "$TEST_ROOT/default-i2c.log" >"$TEST_ROOT/default.stdout"
archive="$TEST_ROOT/out/WsprryPi-support-test-host-20260102T030405Z.tar.gz"
result="${archive}.result.json"
[[ -f "$archive" && -f "${archive}.sha256" && -f "$result" ]] || fail "expected completed artifacts"
assert_valid_json "$result"
assert_contains "$result" '"status": "success"'
assert_contains "$result" '"configuration_files_included": true'
assert_contains "$result" '"full_logs_included": false'
assert_contains "$result" '"i2c_probe_status": "skipped_by_user"'
assert_not_contains "$TEST_ROOT/default-i2c.log" '-y 1'
assert_no_workdirs
[[ "$(stat -c %a "$archive")" == 600 ]] || fail "archive permissions are not restrictive"

# The fixed opt-in probe executes exactly one allowed command and records success.
rm -f "$archive" "${archive}.sha256" "$result"
: > "$TEST_ROOT/probe-i2c.log"
run_collector "$TEST_ROOT/out" "$TEST_ROOT/mocks" "$TEST_ROOT/probe-i2c.log" --probe-i2c >"$TEST_ROOT/probe.stdout"
[[ "$(cat "$TEST_ROOT/probe-i2c.log")" == $'-l\n-y 1' ]] || fail "unexpected I2C invocation"
assert_valid_json "$result"
assert_contains "$result" '"i2c_probe_status": "succeeded"'

# Failure and unavailable probe states remain machine-readable without a real bus.
rm -f "$archive" "${archive}.sha256" "$result"
make_mocks "$TEST_ROOT/mocks" 9
: > "$TEST_ROOT/failed-i2c.log"
run_collector "$TEST_ROOT/out" "$TEST_ROOT/mocks" "$TEST_ROOT/failed-i2c.log" --probe-i2c >"$TEST_ROOT/failed.stdout"
assert_valid_json "$result"
assert_contains "$result" '"i2c_probe_status": "failed"'

rm -f "$archive" "${archive}.sha256" "$result"
make_no_i2c_path "$TEST_ROOT/no-i2c-path"
: > "$TEST_ROOT/unavailable-i2c.log"
run_collector_isolated "$TEST_ROOT/out" "$TEST_ROOT/no-i2c-path" "$TEST_ROOT/unavailable-i2c.log" --probe-i2c >"$TEST_ROOT/unavailable.stdout"
assert_valid_json "$result"
assert_contains "$result" '"i2c_probe_status": "unavailable"'

# Unsafe/missing output paths and existing artifact collisions fail without overwriting output.
if run_collector "$TEST_ROOT/missing" "$TEST_ROOT/mocks" "$TEST_ROOT/missing.log" >/dev/null 2>&1; then fail "missing output directory succeeded"; fi
mkdir "$TEST_ROOT/world-writable" && chmod 777 "$TEST_ROOT/world-writable"
if run_collector "$TEST_ROOT/world-writable" "$TEST_ROOT/mocks" "$TEST_ROOT/unsafe.log" >/dev/null 2>&1; then fail "unsafe output directory succeeded"; fi
mkdir -p "$TEST_ROOT/collision" && chmod 700 "$TEST_ROOT/collision"
printf 'preserve me\n' > "$TEST_ROOT/collision/$(basename "$archive")"
if run_collector "$TEST_ROOT/collision" "$TEST_ROOT/mocks" "$TEST_ROOT/collision.log" >/dev/null 2>&1; then fail "archive collision succeeded"; fi
assert_contains "$TEST_ROOT/collision/$(basename "$archive")" 'preserve me'

# A failed archive build leaves only an explicit failure result, never partial success output.
mkdir "$TEST_ROOT/failure-out" "$TEST_ROOT/failure-mocks"
chmod 700 "$TEST_ROOT/failure-out"
make_mocks "$TEST_ROOT/failure-mocks" 0
cat > "$TEST_ROOT/failure-mocks/tar" <<'EOF'
#!/usr/bin/bash
exit 1
EOF
chmod 700 "$TEST_ROOT/failure-mocks/tar"
if run_collector "$TEST_ROOT/failure-out" "$TEST_ROOT/failure-mocks" "$TEST_ROOT/failure-i2c.log" >/dev/null 2>&1; then fail "archive failure succeeded"; fi
failure_archive="$TEST_ROOT/failure-out/$(basename "$archive")"
[[ ! -e "$failure_archive" && ! -e "${failure_archive}.sha256" ]] || fail "partial archive was retained"
assert_failure_result "${failure_archive}.result.json"
assert_no_workdirs

# A checksum failure occurs after temporary workspace creation and also cleans it up.
mkdir "$TEST_ROOT/checksum-failure-out" "$TEST_ROOT/checksum-failure-mocks"
chmod 700 "$TEST_ROOT/checksum-failure-out"
make_mocks "$TEST_ROOT/checksum-failure-mocks" 0
cat > "$TEST_ROOT/checksum-failure-mocks/sha256sum" <<'EOF'
#!/usr/bin/bash
printf 'not-a-sha256\n'
EOF
chmod 700 "$TEST_ROOT/checksum-failure-mocks/sha256sum"
if run_collector "$TEST_ROOT/checksum-failure-out" "$TEST_ROOT/checksum-failure-mocks" "$TEST_ROOT/checksum-failure-i2c.log" >/dev/null 2>&1; then fail "checksum failure succeeded"; fi
checksum_failure_archive="$TEST_ROOT/checksum-failure-out/$(basename "$archive")"
[[ ! -e "$checksum_failure_archive" && ! -e "${checksum_failure_archive}.sha256" ]] || fail "checksum failure retained partial artifacts"
assert_failure_result "${checksum_failure_archive}.result.json"
assert_no_workdirs

# Explicitly reject caller-controlled probe/bus arguments and retain legacy CWD output.
if run_collector "$TEST_ROOT/out" "$TEST_ROOT/mocks" "$TEST_ROOT/arguments.log" --probe-i2c=2 >/dev/null 2>&1; then fail "arbitrary probe argument accepted"; fi
legacy="$TEST_ROOT/legacy"
mkdir "$legacy"
( cd "$legacy" && I2C_CALL_LOG="$TEST_ROOT/legacy-i2c.log" TMPDIR="$TEST_ROOT/tmp" PATH="$TEST_ROOT/mocks:$PATH" bash "$COLLECTOR" >"$TEST_ROOT/legacy.stdout" )
[[ -f "$legacy/$(basename "$archive")" ]] || fail "legacy current-directory archive missing"
assert_valid_json "$legacy/$(basename "$archive").result.json"

echo "collect-support-bundle tests: PASS"
