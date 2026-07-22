FROM ubuntu:26.04

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
    python3-venv \
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

# uv for Python package management.
ARG UV_VERSION=0.7.8
RUN curl -fSL "https://astral.sh/uv/${UV_VERSION}/install.sh" | sh && \
    mv /root/.local/bin/uv /usr/local/bin/uv

# Zensical for documentation, mdformat for Markdown formatting.
RUN uv pip install --system --break-system-packages \
    mdformat-mkdocs \
    zensical

# Non-root user (Bazel discourages running as root).
RUN useradd -m -s /bin/bash dev && \
    mkdir -p /home/dev/.cache/bazel && chown -R dev:dev /home/dev/.cache && \
    git config --system --add safe.directory /workspace

# Default to clang for Bazel's C++ toolchain detection.
ENV CC=clang
