#!/usr/bin/env bash
# check_scientific_references.sh
# Verifies that every src/*/README.md contains a "## Scientific References" section.
# Exits 0 if all pass, 1 if any are missing (CI-friendly).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC_DIR="$REPO_ROOT/src"

missing=()
found=0

for readme in "$SRC_DIR"/*/README.md; do
    module="$(basename "$(dirname "$readme")")"
    if grep -q "^## Scientific References" "$readme"; then
        ((found++)) || true
    else
        missing+=("$module")
    fi
done

total=$(( ${#missing[@]} + found ))

echo "Scientific References check: $found/$total modules OK"

if [ ${#missing[@]} -gt 0 ]; then
    echo ""
    echo "MISSING ## Scientific References section in:"
    for m in "${missing[@]}"; do
        echo "  - src/$m/README.md"
    done
    exit 1
fi

echo "All $total modules have a Scientific References section."
exit 0
