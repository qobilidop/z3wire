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

docker buildx build -q -t "$IMAGE" .devcontainer/ >/dev/null

docker_flags=()
if [ -t 0 ]; then
  docker_flags+=(-it)
fi

# Expose port 8000 for mkdocs serve.
if [[ "${*}" == *"docs.sh serve"* ]]; then
  docker_flags+=(-p 8000:8000)
fi

docker run --rm ${docker_flags[@]+"${docker_flags[@]}"} \
  -v "$PWD":/workspace \
  -v bazel-cache:/home/dev/.cache/bazel \
  -w /workspace \
  "$IMAGE" "$@"
