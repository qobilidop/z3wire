FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    clang \
    curl \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install Bazelisk as "bazel"
ARG BAZELISK_VERSION=v1.25.0
ARG TARGETARCH
RUN curl -fSL "https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-${TARGETARCH}" \
    -o /usr/local/bin/bazel && chmod +x /usr/local/bin/bazel
