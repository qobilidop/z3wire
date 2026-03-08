#!/usr/bin/env bash
# Auto-format all C++ source files, or check formatting with --check.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

files=$(find z3wire examples -name '*.h' -o -name '*.cc')

if [[ "${1:-}" == "--check" ]]; then
  if ! echo "$files" | xargs clang-format --dry-run -Werror 2>&1; then
    echo ""
    echo "Formatting errors found. Run ./tools/format.sh to fix."
    exit 1
  fi
  echo "All files are properly formatted."
else
  echo "$files" | xargs clang-format -i
  echo "Formatted all C++ files."
fi
