#!/usr/bin/env bash
# Run linters on all source files.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

# --- C++ ---

# Generate compile_commands.json.
cmake -B build >/dev/null 2>&1
cp build/compile_commands.json compile_commands.json

files=$(find z3wire examples -name '*.h' -o -name '*.cc')

echo "$files" | xargs --no-run-if-empty clang-tidy -p .
status=$?

if [[ $status -ne 0 ]]; then
  exit $status
fi

echo "clang-tidy: all checks passed."

ic_fail=0
for f in $files; do
  output=$(clang-include-cleaner-18 --print=changes -p . "$f" 2>&1)
  if [[ -n "$output" ]]; then
    echo "$f:"
    echo "$output"
    ic_fail=1
  fi
done

if [[ $ic_fail -ne 0 ]]; then
  echo ""
  echo "include-cleaner: issues found."
  exit 1
fi

echo "include-cleaner: all checks passed."

# --- Bazel ---
bzl_files=$(find . -name '*.bazel' -o -name '*.bzl' -o -name 'BUILD' |
  grep -v -e '.git/' -e './build/' -e './site/')
echo "$bzl_files" | xargs --no-run-if-empty buildifier -lint=warn
echo "buildifier: all checks passed."

# --- Shell ---
sh_files=$(find . -name '*.sh' -not -path './.git/*' -not -path './build/*' -not -path './site/*')
echo "$sh_files" | xargs --no-run-if-empty shellcheck
echo "shellcheck: all checks passed."
