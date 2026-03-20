#!/usr/bin/env bash
# Run linters on all source files.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

# --- C++ ---

# Build a test target to ensure external dependencies (z3, googletest) are fetched.
bazel build //z3wire:sym_bit_vec_test

output_base=$(bazel info output_base)

# Locate z3 include paths.
# z3++.h includes <z3.h>, so we need both directories on the include path.
z3_cpp_header=$(find -L "$output_base/external" \
  -path "*/z3++.h" -print -quit 2>/dev/null || true)
if [[ -z "$z3_cpp_header" ]]; then
  echo "Error: could not find z3++.h in Bazel build output."
  exit 1
fi
z3_cpp_include_dir=$(dirname "$z3_cpp_header")

z3_c_header=$(find -L "$output_base/external" \
  -path "*/z3.h" -not -path "*/c++/*" -print -quit 2>/dev/null || true)
if [[ -z "$z3_c_header" ]]; then
  echo "Error: could not find z3.h in Bazel build output."
  exit 1
fi
z3_c_include_dir=$(dirname "$z3_c_header")

# Locate googletest include path.
gtest_header=$(find -L "$output_base/external" \
  -path "*/include/gtest/gtest.h" -print -quit 2>/dev/null || true)
if [[ -z "$gtest_header" ]]; then
  echo "Error: could not find googletest include directory in Bazel externals."
  exit 1
fi
gtest_include_dir=$(dirname "$(dirname "$gtest_header")")

# Generate compile_commands.json for clang-tidy, include-cleaner, and IDE use.
files=$(find z3wire examples -name '*.h' -o -name '*.cc' |
  grep -v '\.expected\.' |
  grep -v 'examples/weave/status_register_test\.cc' |
  grep -v 'examples/weave/status_register_verification\.cc' |
  grep -v 'z3wire/weave/')
entries=""
for f in $files; do
  abs_path="$(pwd)/$f"
  if [[ -n "$entries" ]]; then entries+=","; fi
  entries+="
  {
    \"directory\": \"$(pwd)\",
    \"command\": \"clang++ -std=c++20 -xc++ -I. -isystem $z3_cpp_include_dir -isystem $z3_c_include_dir -isystem $gtest_include_dir -c $abs_path\",
    \"file\": \"$abs_path\"
  }"
done
echo "[$entries
]" >compile_commands.json

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
