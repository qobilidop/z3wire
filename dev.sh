#!/usr/bin/env bash
# Run commands inside the z3wire devcontainer.
#
# Usage:
#   ./dev.sh bazel build //...
#   ./dev.sh bazel test //...
#   ./dev.sh ./tools/lint.sh
#   ./dev.sh bazel run //examples:safe_adder
#   ./dev.sh bash              # interactive shell
#
# The devcontainer image is rebuilt automatically when the Dockerfile changes.
set -euo pipefail

IMAGE=z3wire-dev

docker build -q -t "$IMAGE" .devcontainer/ >/dev/null

tty_flag=""
if [ -t 0 ]; then
  tty_flag="-it"
fi

docker run --rm $tty_flag \
  -v "$PWD":/workspace \
  -v bazel-cache:/root/.cache/bazel \
  -w /workspace \
  "$IMAGE" "$@"
