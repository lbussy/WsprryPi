#!/bin/bash

# Function to run tests and compare outputs
run_test() {
    local file="$1"
    local expected_output="$2"

    echo "Running test for $file..."

    result=$(./get_version.sh "$file")
    if [[ "$result" == "$expected_output" ]]; then
        echo "PASS: $file"
    else
        echo "FAIL: $file"
        echo "Expected: $expected_output"
        echo "Got: $result"
    fi
}

# Run tests for each file

# Test Bash Script Single Line Comments
expected_bash_output="1.2.1-8c2314a-dirty"
run_test "./test_cases/bash_script_single.sh" "$expected_bash_output"

# Test Bash Script Multi Line Comments
expected_bash_output="1.2.1-8c2314a-dirty"
run_test "./test_cases/bash_script_multi.sh" "$expected_bash_output"

# Test Python Script Single Line Comments
expected_python_output="1.2.1-8c2314a-dirty"
run_test "./test_cases/python_script_single.py" "$expected_python_output"

# Test Python Script Multi Line Comments
expected_python_output="1.2.1-8c2314a-dirty"
run_test "./test_cases/python_script_multi.py" "$expected_python_output"

# Test Systemd Unit File with Comments
expected_systemd_output="1.2.1-8c2314a-dirty"
run_test "./test_cases/systemd_unit_versioncomment.service" "$expected_systemd_output"

# Test Systemd Unit File with Version Line
expected_systemd_output="1.2.1-8c2314a-dirty"
run_test "./test_cases/systemd_unit_versionline.service" "$expected_systemd_output"

# Test Config File
expected_config_output="1.2.1-8c2314a-dirty"
run_test "./test_cases/config_file.conf" "$expected_config_output"

# Test Executable File
expected_executable_output="1.2.1-8c2314a-dirty"
run_test "./test_cases/version_executable" "$expected_executable_output"
