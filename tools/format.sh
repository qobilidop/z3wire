#!/usr/bin/env bash
# Auto-format all source files, or check formatting with --check.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

cpp_files=$(find z3wire examples -name '*.h' -o -name '*.cc')
md_files=$(find . -name '*.md' -not -path './.git/*')

if [[ "${1:-}" == "--check" ]]; then
  fail=0

  if ! echo "$cpp_files" | xargs clang-format --dry-run -Werror 2>&1; then
    fail=1
  fi

  if ! echo "$md_files" | xargs mdformat --check 2>&1; then
    fail=1
  fi

  if [[ $fail -ne 0 ]]; then
    echo ""
    echo "Formatting errors found. Run ./tools/format.sh to fix."
    exit 1
  fi
  echo "All files are properly formatted."
else
  echo "$cpp_files" | xargs clang-format -i
  echo "$md_files" | xargs mdformat
  echo "Formatted all files."
fi
