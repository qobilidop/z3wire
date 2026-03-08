#!/usr/bin/env bash
# Check formatting of all C++ source files.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

files=$(find z3wire examples -name '*.h' -o -name '*.cc')

if ! echo "$files" | xargs clang-format --dry-run -Werror 2>&1; then
  echo ""
  echo "Formatting errors found. Run ./tools/format.sh to fix."
  exit 1
fi

echo "All files are properly formatted."
