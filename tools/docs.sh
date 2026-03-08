#!/usr/bin/env bash
# Build or serve the MkDocs documentation site.
#
# Usage:
#   ./tools/docs.sh          # build the site to site/
#   ./tools/docs.sh serve    # serve locally with live reload
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

if [[ "${1:-}" == "serve" ]]; then
  mkdocs serve -a 0.0.0.0:8000
else
  mkdocs build --strict
  echo "Site built in site/"
fi
