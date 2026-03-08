#!/usr/bin/env bash
# Auto-format all C++ source files.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

find z3wire examples -name '*.h' -o -name '*.cc' | xargs clang-format -i

echo "Formatted all C++ files."
