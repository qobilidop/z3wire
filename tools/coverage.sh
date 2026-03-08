#!/usr/bin/env bash
# Generate an LCOV coverage report for the z3wire library.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

bazel coverage //... --combined_report=lcov --nocache_test_results

# The combined report path.
report=$(bazel info output_path)/_coverage/_coverage_report.dat
if [[ ! -f "$report" ]]; then
  echo "Error: coverage report not found at $report"
  exit 1
fi

# Print a summary.
lcov --summary "$report" 2>&1

echo ""
echo "Full report: $report"
