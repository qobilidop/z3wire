FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    clang \
    clang-format \
    clang-tidy \
    curl \
    git \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Install Bazelisk as "bazel"
ARG BAZELISK_VERSION=v1.25.0
RUN ARCH=$(uname -m | sed 's/x86_64/amd64/;s/aarch64/arm64/') && \
    curl -fSL "https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-${ARCH}" \
    -o /usr/local/bin/bazel && chmod +x /usr/local/bin/bazel

# Default to clang for Bazel's C++ toolchain auto-detection.
ENV CC=clang
