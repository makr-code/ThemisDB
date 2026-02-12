#!/usr/bin/env bash
set -euo pipefail

# Initialize Legal Training Schema in ThemisDB
# Usage: ./scripts/init_legal_training_schema.sh
# Environment variables:
#   THEMISDB_URL - ThemisDB server URL (default: http://localhost:8529)

THEMISDB_URL="${THEMISDB_URL:-http://localhost:8529}"
SCHEMA_FILE="${SCHEMA_FILE:-config/schemas/legal_training_schema.sql}"

echo "=== Initializing Legal Training Schema ==="
echo "ThemisDB URL: $THEMISDB_URL"
echo "Schema file: $SCHEMA_FILE"

# Check if schema file exists
if [ ! -f "$SCHEMA_FILE" ]; then
    echo "Error: Schema file not found: $SCHEMA_FILE"
    exit 1
fi

# Check if curl and jq are available
if ! command -v curl &> /dev/null; then
    echo "Error: curl is required but not installed"
    exit 1
fi

if ! command -v jq &> /dev/null; then
    echo "Warning: jq not found, falling back to manual JSON construction"
    USE_JQ=0
else
    USE_JQ=1
fi

# Read schema file content
SCHEMA_CONTENT=$(cat "$SCHEMA_FILE")

# Execute schema
echo "Executing schema..."

# Construct JSON payload
if [ "$USE_JQ" -eq 1 ]; then
    # Use jq for proper JSON encoding
    JSON_PAYLOAD=$(jq -n --arg query "$SCHEMA_CONTENT" '{query: $query}')
    
    if echo "$JSON_PAYLOAD" | curl -X POST "$THEMISDB_URL/query" \
        -H "Content-Type: application/json" \
        -d @-; then
        echo ""
        echo "Schema initialization complete!"
    else
        echo ""
        echo "Error: Schema initialization failed"
        exit 1
    fi
else
    # Manual JSON construction (less robust, but works without jq)
    # Note: This approach has limitations with special characters
    if curl -X POST "$THEMISDB_URL/query" \
        -H "Content-Type: application/json" \
        -d "{\"query\": $(printf '%s' "$SCHEMA_CONTENT" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g' | jq -Rs .)}"; then
        echo ""
        echo "Schema initialization complete!"
    else
        echo ""
        echo "Error: Schema initialization failed"
        exit 1
    fi
fi
