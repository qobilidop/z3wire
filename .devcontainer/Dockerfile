FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    ccache \
    clang \
    clang-format \
    clang-tidy \
    cmake \
    curl \
    git \
    libclang-rt-dev \
    make \
    python3 \
    python3-pip \
    shellcheck \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Pre-built Z3 for CMake. Keep version in sync with MODULE.bazel.
ARG Z3_VERSION=4.15.2
RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "x86_64" ]; then \
      Z3_ARCHIVE="z3-${Z3_VERSION}-x64-glibc-2.39.zip"; \
    elif [ "$ARCH" = "aarch64" ]; then \
      Z3_ARCHIVE="z3-${Z3_VERSION}-arm64-glibc-2.34.zip"; \
    else \
      echo "Unsupported architecture: $ARCH" >&2; exit 1; \
    fi && \
    curl -fSL "https://github.com/Z3Prover/z3/releases/download/z3-${Z3_VERSION}/${Z3_ARCHIVE}" \
      -o /tmp/z3.zip && \
    unzip -q /tmp/z3.zip -d /tmp/z3 && \
    cp /tmp/z3/z3-*/include/*.h /usr/local/include/ && \
    cp /tmp/z3/z3-*/bin/libz3.so /usr/local/lib/ && \
    cp /tmp/z3/z3-*/bin/libz3.a /usr/local/lib/ && \
    ldconfig && \
    rm -rf /tmp/z3.zip /tmp/z3

# GoogleTest for CMake. Keep version in sync with MODULE.bazel.
ARG GOOGLETEST_VERSION=v1.17.0
RUN git clone --depth 1 --branch ${GOOGLETEST_VERSION} \
      https://github.com/google/googletest.git /tmp/googletest && \
    cmake -S /tmp/googletest -B /tmp/googletest/build \
      -DCMAKE_BUILD_TYPE=Release -DINSTALL_GTEST=ON && \
    cmake --build /tmp/googletest/build -j$(nproc) && \
    cmake --install /tmp/googletest/build && \
    rm -rf /tmp/googletest

# Abseil for CMake. Must be installed before Protobuf. Keep version in sync with MODULE.bazel.
ARG ABSEIL_VERSION=20260107.1
RUN git clone --depth 1 --branch ${ABSEIL_VERSION} \
      https://github.com/abseil/abseil-cpp.git /tmp/abseil && \
    cmake -S /tmp/abseil -B /tmp/abseil/build \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20 \
      -DABSL_PROPAGATE_CXX_STD=ON -DABSL_BUILD_TESTING=OFF && \
    cmake --build /tmp/abseil/build -j$(nproc) && \
    cmake --install /tmp/abseil/build && \
    rm -rf /tmp/abseil

# Protobuf for CMake. Keep version in sync with MODULE.bazel.
ARG PROTOBUF_VERSION=v34.0
RUN git clone --depth 1 --branch ${PROTOBUF_VERSION} \
      https://github.com/protocolbuffers/protobuf.git /tmp/protobuf && \
    cmake -S /tmp/protobuf -B /tmp/protobuf/build \
      -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF \
      -Dprotobuf_ABSL_PROVIDER=package \
      -DCMAKE_CXX_STANDARD=20 && \
    cmake --build /tmp/protobuf/build -j$(nproc) && \
    cmake --install /tmp/protobuf/build && \
    rm -rf /tmp/protobuf

# Bazelisk as "bazel".
ARG BAZELISK_VERSION=v1.28.1
RUN ARCH=$(uname -m | sed 's/x86_64/amd64/;s/aarch64/arm64/') && \
    curl -fSL "https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-${ARCH}" \
    -o /usr/local/bin/bazel && chmod +x /usr/local/bin/bazel

# Buildifier for Bazel file formatting.
ARG BUILDIFIER_VERSION=v8.5.1
RUN ARCH=$(uname -m | sed 's/x86_64/amd64/;s/aarch64/arm64/') && \
    curl -fSL "https://github.com/bazelbuild/buildtools/releases/download/${BUILDIFIER_VERSION}/buildifier-linux-${ARCH}" \
    -o /usr/local/bin/buildifier && chmod +x /usr/local/bin/buildifier

# shfmt for shell script formatting.
ARG SHFMT_VERSION=v3.13.0
RUN ARCH=$(uname -m | sed 's/x86_64/amd64/;s/aarch64/arm64/') && \
    curl -fSL "https://github.com/mvdan/sh/releases/download/${SHFMT_VERSION}/shfmt_${SHFMT_VERSION}_linux_${ARCH}" \
    -o /usr/local/bin/shfmt && chmod +x /usr/local/bin/shfmt

# dprint for Markdown, JSON, YAML, and TOML formatting.
ARG DPRINT_VERSION=0.53.0
RUN ARCH=$(uname -m) && \
    curl -fSL "https://github.com/dprint/dprint/releases/download/${DPRINT_VERSION}/dprint-${ARCH}-unknown-linux-gnu.zip" \
    -o /tmp/dprint.zip && \
    unzip -q /tmp/dprint.zip -d /tmp/dprint && \
    cp /tmp/dprint/dprint /usr/local/bin/dprint && chmod +x /usr/local/bin/dprint && \
    rm -rf /tmp/dprint.zip /tmp/dprint

# MkDocs and plugins for documentation.
RUN pip3 install --break-system-packages \
    mkdocs-include-markdown-plugin \
    mkdocs-material

# Non-root user (Bazel discourages running as root).
RUN useradd -m -s /bin/bash dev && \
    mkdir -p /home/dev/.cache/bazel && chown -R dev:dev /home/dev/.cache && \
    git config --system --add safe.directory /workspace

# Default to clang for Bazel's C++ toolchain detection.
ENV CC=clang
