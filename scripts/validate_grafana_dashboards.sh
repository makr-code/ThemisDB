#!/bin/bash
# Validate Grafana dashboard JSON files
#
# This script validates all Grafana dashboard JSON files in the repository.
# It checks for valid JSON syntax, required fields, and dashboard structure.
#
# Usage: ./scripts/validate_grafana_dashboards.sh
# Note: Requires jq to be installed (apt-get install jq)

set -e

DASHBOARD_DIR="config/grafana/dashboards"
FAILED=0

echo "Validating Grafana dashboards..."
echo "================================"

# Check if jq is available
if ! command -v jq &> /dev/null; then
    echo "Error: jq is not installed. Please install jq to validate JSON files."
    exit 1
fi

# Validate each JSON file
for file in "$DASHBOARD_DIR"/*.json; do
    if [ -f "$file" ]; then
        echo -n "Validating $(basename "$file")... "
        
        # Check JSON syntax
        if ! jq empty "$file" 2>/dev/null; then
            echo "❌ FAILED (Invalid JSON syntax)"
            FAILED=$((FAILED + 1))
            continue
        fi
        
        # Check required fields
        TITLE=$(jq -r '.title // empty' "$file")
        DASHBOARD_UID=$(jq -r '.uid // empty' "$file")
        PANELS=$(jq '.panels | length' "$file")
        
        if [ -z "$TITLE" ]; then
            echo "❌ FAILED (Missing title)"
            FAILED=$((FAILED + 1))
            continue
        fi
        
        if [ -z "$DASHBOARD_UID" ]; then
            echo "❌ FAILED (Missing UID)"
            FAILED=$((FAILED + 1))
            continue
        fi
        
        if [ "$PANELS" -eq 0 ]; then
            echo "⚠️  WARNING (No panels defined)"
        fi
        
        echo "✓ PASSED ($PANELS panels, title: '$TITLE', uid: '$DASHBOARD_UID')"
    fi
done

echo ""
echo "================================"
if [ $FAILED -eq 0 ]; then
    echo "✓ All dashboards validated successfully"
    exit 0
else
    echo "❌ $FAILED dashboard(s) failed validation"
    exit 1
fi
