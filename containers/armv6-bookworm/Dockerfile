# syntax=docker/dockerfile:1.7

# This digest pins balena's Bookworm ARMv6 build image. It contains a Raspbian
# GNU/Linux 12 userland and runs under Docker's ARM emulation on non-ARMv6
# hosts.
FROM balenalib/raspberry-pi:bookworm-build@sha256:5e44b1697cd34bac1eaf43fbadab44c54d25246ff0b57144b054b8039d85141a AS build

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        file \
        git \
        libgpiod-dev \
        libssl-dev \
        libsystemd-dev \
        pkg-config \
        python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . .

RUN git config --global --add safe.directory /workspace \
    && make -C src release SUDO= JOBS=4

RUN mkdir -p /artifact \
    && cp src/build/bin/wsprrypi /artifact/wsprrypi \
    && file /artifact/wsprrypi > /artifact/file.txt \
    && readelf --file-header /artifact/wsprrypi > /artifact/elf-header.txt \
    && readelf --arch-specific /artifact/wsprrypi > /artifact/elf-attributes.txt \
    && ldd /artifact/wsprrypi > /artifact/shared-libraries.txt \
    && dpkg-query -W \
        -f='${binary:Package}\t${Version}\n' \
        gcc g++ libc6 libstdc++6 libgpiod2 libgpiod-dev \
        libssl3 libssl-dev libsystemd0 libsystemd-dev \
        > /artifact/build-packages.txt \
    && cd /artifact \
    && sha256sum wsprrypi > SHA256SUMS

FROM scratch AS artifact
COPY --from=build /artifact/ /
