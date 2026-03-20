#!/usr/bin/env bash
# Auto-format all source files, or check formatting with --check.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

cpp_files=$(find z3wire examples -name '*.h' -o -name '*.cc' -o -name '*.proto' |
  grep -v '\.expected\.')
bzl_files=$(find . -name '*.bazel' -o -name '*.bzl' -o -name 'BUILD' |
  grep -v -e '.git/' -e './build/' -e './site/')
sh_files=$(find . -name '*.sh' -not -path './.git/*' -not -path './build/*' -not -path './site/*')

if [[ "${1:-}" == "--check" ]]; then
  fail=0

  # --- C++ ---
  if ! echo "$cpp_files" | xargs --no-run-if-empty clang-format --dry-run -Werror 2>&1; then
    fail=1
  fi

  # --- Bazel ---
  if ! echo "$bzl_files" | xargs --no-run-if-empty buildifier -mode=check 2>&1; then
    fail=1
  fi

  # --- Shell ---
  if ! echo "$sh_files" | xargs --no-run-if-empty shfmt -d 2>&1; then
    fail=1
  fi

  # --- Markdown, JSON, YAML, TOML ---
  if ! dprint check 2>&1; then
    fail=1
  fi

  if [[ $fail -ne 0 ]]; then
    echo ""
    echo "Formatting errors found. Run ./tools/format.sh to fix."
    exit 1
  fi
  echo "All files are properly formatted."
else
  echo "$cpp_files" | xargs --no-run-if-empty clang-format -i
  echo "$bzl_files" | xargs --no-run-if-empty buildifier
  echo "$sh_files" | xargs --no-run-if-empty shfmt -w
  dprint fmt
  echo "Formatted all files."
fi
