#!/usr/bin/env bash
# Verify generated files match golden files.
set -euo pipefail

fail=0

for ext in h proto; do
  generated="examples/weave/status_register.${ext}"
  expected="examples/weave/status_register.expected.${ext}"
  if ! diff -u "${expected}" "${generated}"; then
    echo "MISMATCH: ${generated} differs from ${expected}"
    echo "To update: ./dev.sh bazel build //examples/weave:status_register_gen && cp bazel-bin/examples/weave/status_register.${ext} examples/weave/status_register.expected.${ext}"
    fail=1
  fi
done

if [[ $fail -ne 0 ]]; then
  exit 1
fi
echo "Golden files match."
