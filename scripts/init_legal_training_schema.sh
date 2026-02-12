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

# Check if curl is available
if ! command -v curl &> /dev/null; then
    echo "Error: curl is required but not installed"
    exit 1
fi

# Execute schema
echo "Executing schema..."
if curl -X POST "$THEMISDB_URL/query" \
    -H "Content-Type: application/json" \
    -d @"$SCHEMA_FILE"; then
    echo ""
    echo "Schema initialization complete!"
else
    echo ""
    echo "Error: Schema initialization failed"
    exit 1
fi
