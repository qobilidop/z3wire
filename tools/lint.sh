#!/usr/bin/env bash
# Run linters on all source files.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

# --- Shell ---
sh_files=$(find . -name '*.sh' -not -path './.git/*' -not -path './build/*' -not -path './site/*')
echo "$sh_files" | xargs shellcheck
echo "shellcheck: all checks passed."

# --- Bazel ---
bzl_files=$(find . -name '*.bazel' -o -name '*.bzl' -o -name 'BUILD' |
  grep -v -e '.git/' -e './build/' -e './site/')
echo "$bzl_files" | xargs buildifier -lint=warn
echo "buildifier: all checks passed."

# --- C++ ---

# Build a test target to ensure external dependencies (z3, googletest) are fetched.
bazel build //z3wire:sym_bit_vec_test

output_base=$(bazel info output_base)

# Locate z3 include path from the cmake build output.
z3_header=$(find -L "$output_base" \
  -path "*/z3_static/include/z3++.h" -print -quit 2>/dev/null)
if [[ -z "$z3_header" ]]; then
  echo "Error: could not find z3 include directory in Bazel build output."
  exit 1
fi
z3_include_dir=$(dirname "$z3_header")

# Locate googletest include path.
gtest_header=$(find -L "$output_base/external" \
  -path "*/include/gtest/gtest.h" -print -quit 2>/dev/null)
if [[ -z "$gtest_header" ]]; then
  echo "Error: could not find googletest include directory in Bazel externals."
  exit 1
fi
gtest_include_dir=$(dirname "$(dirname "$gtest_header")")

# Generate a compile_commands.json so clang-tidy knows the compilation flags.
files=$(find z3wire examples -name '*.h' -o -name '*.cc' |
  grep -v '\.expected\.' |
  grep -v 'examples/weave/status_register_test\.cc' |
  grep -v 'examples/weave/status_register_verification\.cc')
entries=""
for f in $files; do
  abs_path="$(pwd)/$f"
  if [[ -n "$entries" ]]; then entries+=","; fi
  entries+="
  {
    \"directory\": \"$(pwd)\",
    \"command\": \"clang++ -std=c++20 -xc++ -I. -isystem $z3_include_dir -isystem $gtest_include_dir -c $abs_path\",
    \"file\": \"$abs_path\"
  }"
done
echo "[$entries
]" >compile_commands.json

echo "$files" | xargs clang-tidy -p .
status=$?

if [[ $status -ne 0 ]]; then
  rm -f compile_commands.json
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

rm -f compile_commands.json

if [[ $ic_fail -ne 0 ]]; then
  echo ""
  echo "include-cleaner: issues found."
  exit 1
fi

echo "include-cleaner: all checks passed."
