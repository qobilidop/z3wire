#!/usr/bin/env bash
# Verify that status_register_verification output matches expected.
set -euo pipefail

actual=$(examples/weave/status_register_verification)
expected=$(cat examples/weave/status_register_verification.expected.txt)

if [[ "$actual" != "$expected" ]]; then
  echo "MISMATCH: verification output differs from expected."
  diff -u <(echo "$expected") <(echo "$actual")
  echo ""
  echo "To update: ./dev.sh bazel run //examples/weave:status_register_verification > examples/weave/status_register_verification.expected.txt"
  exit 1
fi
echo "Verification output matches."
