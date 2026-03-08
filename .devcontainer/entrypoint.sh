#!/usr/bin/env bash
# Ensure the cache directory is writable, then run the command as dev.
set -euo pipefail

mkdir -p /home/dev/.cache/bazel
chown dev:dev /home/dev/.cache /home/dev/.cache/bazel
runuser -u dev -- git config --global --add safe.directory /workspace 2>/dev/null || true
exec runuser -u dev -- "$@"
