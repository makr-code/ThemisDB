#!/bin/bash
# Initialize Legal Training Schema in ThemisDB

THEMISDB_URL="${THEMISDB_URL:-http://localhost:8529}"
SCHEMA_FILE="config/schemas/legal_training_schema.sql"

echo "Initializing Legal Training Schema..."
echo "ThemisDB URL: $THEMISDB_URL"

# Execute schema
curl -X POST "$THEMISDB_URL/query" \
    -H "Content-Type: application/json" \
    -d @"$SCHEMA_FILE"

echo "Schema initialization complete!"
