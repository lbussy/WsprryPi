#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "${script_dir}/.." && pwd)
output_dir=${1:-"${repo_dir}/dist/armv6-bookworm"}

if ! command -v docker >/dev/null 2>&1; then
    echo "docker is required" >&2
    exit 1
fi

if [ -e "${output_dir}" ] && [ -n "$(find "${output_dir}" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null)" ]; then
    echo "output directory is not empty: ${output_dir}" >&2
    exit 1
fi

mkdir -p "${output_dir}"

docker buildx build \
    --file "${repo_dir}/containers/armv6-bookworm/Dockerfile" \
    --platform linux/arm/v6 \
    --target build \
    --tag wsprrypi-build:armv6-bookworm \
    --load \
    "${repo_dir}"

container_id=
cleanup() {
    if [ -n "${container_id}" ]; then
        docker rm -f "${container_id}" >/dev/null 2>&1 || true
    fi
}
trap cleanup 0 1 2 15

container_id=$(docker create \
    --platform linux/arm/v6 \
    wsprrypi-build:armv6-bookworm)
docker cp "${container_id}:/artifact/." "${output_dir}"
docker rm "${container_id}" >/dev/null
container_id=

echo "ARMv6 Bookworm artifacts exported to ${output_dir}"
