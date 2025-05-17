#!/usr/bin/env bash
set -euo pipefail

# -----------------------------------------------------------------------------
# @brief Detects the git repository root, navigates to src, builds,
#        and stages new executables.
#
# @details Uses git to find the project root, then executes a series of
#          build commands (clean, debug, release, install), and finally
#          copies the resulting wsprrypi executables into the
#          executables directory adjacent to the src folder.
#
# @global THIS_SCRIPT Name of the current script.
#
# @throws Exits with an error if the git root cannot be detected,
#         if cd fails, or if any build/copy command fails.
#
# @return None.
# -----------------------------------------------------------------------------
main() {
    # Detect the repository root directory
    local repo_root
    repo_root=$(git rev-parse --show-toplevel 2>/dev/null) \
        || die "Unable to detect git repository root directory."
    echo "Repository root detected at '$repo_root'."

    # Navigate to the src directory under the repo root
    cd "$repo_root/src" \
        || die "Failed to change directory to '$repo_root/src'."
    echo "Changed directory to '$repo_root/src'."

    # Run the build pipeline
    clear
    make clean
    make debug
    make release
    make install

    # Copy the generated wsprrypi executables
    cp ./build/bin/wsprrypi* ../executables/ \
        || die "Failed to copy executables to '../executables/'."
    echo "Executables staged in '$repo_root/executables/'."
}

# Execute main with all passed arguments
main "$@"
