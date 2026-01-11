#!/bin/bash
# Example: Batch processing multiple documentation sources
#
# This script demonstrates how to process multiple documentation
# sources in parallel and merge them into separate databases.

set -e  # Exit on error

# Configuration
SOURCES=(
    "/opt/company/product_docs:product"
    "/opt/company/api_docs:api"
    "/opt/company/runbooks:ops"
)
OUTPUT_DIR="/var/lib/themisdb"
NAMESPACE_PREFIX="com.mycompany"

echo "============================================================"
echo "ThemisDB Documentation Builder - Batch Processing"
echo "============================================================"
echo ""

# Process each source in parallel
PIDS=()
for SOURCE in "${SOURCES[@]}"; do
    # Split source into path and name
    IFS=':' read -r INPUT_PATH DB_NAME <<< "$SOURCE"
    
    OUTPUT_DB="$OUTPUT_DIR/${DB_NAME}_docs.db"
    NAMESPACE="$NAMESPACE_PREFIX.$DB_NAME"
    
    echo "Processing: $DB_NAME"
    echo "  Input:     $INPUT_PATH"
    echo "  Output:    $OUTPUT_DB"
    echo "  Namespace: $NAMESPACE"
    
    # Start build in background
    (
        ./themis_docs_builder \
            --input "$INPUT_PATH" \
            --output "$OUTPUT_DB" \
            --format markdown \
            --format html \
            --namespace "$NAMESPACE" \
            --recursive \
            --read-only \
            --validate \
            --batch-size 200
    ) &
    
    PIDS+=($!)
    echo "  Started (PID: $!)"
    echo ""
done

# Wait for all builds to complete
echo "Waiting for all builds to complete..."
for PID in "${PIDS[@]}"; do
    wait "$PID"
    if [ $? -eq 0 ]; then
        echo "  PID $PID: SUCCESS"
    else
        echo "  PID $PID: FAILED"
    fi
done

echo ""
echo "============================================================"
echo "Batch processing complete!"
echo "============================================================"
echo ""
echo "Generated databases:"
for SOURCE in "${SOURCES[@]}"; do
    IFS=':' read -r INPUT_PATH DB_NAME <<< "$SOURCE"
    OUTPUT_DB="$OUTPUT_DIR/${DB_NAME}_docs.db"
    if [ -d "$OUTPUT_DB" ]; then
        SIZE=$(du -sh "$OUTPUT_DB" | cut -f1)
        echo "  $DB_NAME: $OUTPUT_DB ($SIZE)"
    fi
done
echo ""
echo "Add to config/docs_assistant.yaml:"
echo ""
echo "docs_assistant:"
echo "  databases:"
for SOURCE in "${SOURCES[@]}"; do
    IFS=':' read -r INPUT_PATH DB_NAME <<< "$SOURCE"
    NAMESPACE="$NAMESPACE_PREFIX.$DB_NAME"
    echo "    - name: \"${DB_NAME}_docs\""
    echo "      path: \"$OUTPUT_DIR/${DB_NAME}_docs.db\""
    echo "      namespace: \"$NAMESPACE\""
    echo "      read_only: true"
done
echo ""
