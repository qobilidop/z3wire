FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    clang \
    clang-format \
    clang-tidy \
    libclang-rt-dev \
    llvm \
    curl \
    git \
    lcov \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install Bazelisk as "bazel"
ARG BAZELISK_VERSION=v1.28.1
RUN ARCH=$(uname -m | sed 's/x86_64/amd64/;s/aarch64/arm64/') && \
    curl -fSL "https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-${ARCH}" \
    -o /usr/local/bin/bazel && chmod +x /usr/local/bin/bazel

# Install Buildifier for Bazel file formatting.
ARG BUILDIFIER_VERSION=v8.5.1
RUN ARCH=$(uname -m | sed 's/x86_64/amd64/;s/aarch64/arm64/') && \
    curl -fSL "https://github.com/bazelbuild/buildtools/releases/download/${BUILDIFIER_VERSION}/buildifier-linux-${ARCH}" \
    -o /usr/local/bin/buildifier && chmod +x /usr/local/bin/buildifier

# Install MkDocs with Material theme.
RUN pip install --break-system-packages mkdocs-material mkdocs-include-markdown-plugin

# Install mdformat for Markdown formatting.
RUN pip install --break-system-packages mdformat

# Non-root user (required by rules_python for bazel coverage).
RUN useradd -m -s /bin/bash dev
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
ENTRYPOINT ["entrypoint.sh"]

# Default to clang for Bazel's C++ toolchain auto-detection.
ENV CC=clang
