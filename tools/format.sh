#!/usr/bin/env bash
# Auto-format all source files, or check formatting with --check.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

cpp_files=$(find z3wire examples -name '*.h' -o -name '*.cc' |
  grep -v '\.expected\.')
bzl_files=$(find . -name '*.bazel' -o -name '*.bzl' -o -name 'BUILD' |
  grep -v '.git/')
sh_files=$(find . -name '*.sh' -not -path './.git/*')
md_files=$(find . -name '*.md' -not -path './.git/*')
py_files=$(find z3wire examples -name '*.py' 2>/dev/null || true)

if [[ "${1:-}" == "--check" ]]; then
  fail=0

  if ! echo "$cpp_files" | xargs clang-format --dry-run -Werror 2>&1; then
    fail=1
  fi

  if ! echo "$bzl_files" | xargs buildifier -mode=check 2>&1; then
    fail=1
  fi

  if ! echo "$sh_files" | xargs shfmt -d 2>&1; then
    fail=1
  fi

  if ! echo "$md_files" | xargs mdformat --check 2>&1; then
    fail=1
  fi

  if [[ -n "$py_files" ]]; then
    if ! echo "$py_files" | xargs black --check 2>&1; then
      fail=1
    fi
  fi

  if [[ $fail -ne 0 ]]; then
    echo ""
    echo "Formatting errors found. Run ./tools/format.sh to fix."
    exit 1
  fi
  echo "All files are properly formatted."
else
  echo "$cpp_files" | xargs clang-format -i
  echo "$bzl_files" | xargs buildifier
  echo "$sh_files" | xargs shfmt -w
  echo "$md_files" | xargs mdformat
  if [[ -n "$py_files" ]]; then
    echo "$py_files" | xargs black
  fi
  echo "Formatted all files."
fi
