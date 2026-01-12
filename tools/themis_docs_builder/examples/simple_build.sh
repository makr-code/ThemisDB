#!/bin/bash
# Example: Simple documentation database build
#
# This script demonstrates basic usage of themis_docs_builder
# for creating a documentation database from Markdown files.

set -e  # Exit on error

# Configuration
INPUT_DIR="/opt/company/docs"
OUTPUT_DB="/var/lib/themisdb/company_docs.db"
NAMESPACE="com.mycompany"

echo "============================================================"
echo "ThemisDB Documentation Builder - Simple Build Example"
echo "============================================================"
echo ""
echo "Input:     $INPUT_DIR"
echo "Output:    $OUTPUT_DB"
echo "Namespace: $NAMESPACE"
echo ""

# Check if input directory exists
if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: Input directory does not exist: $INPUT_DIR"
    exit 1
fi

# Build the documentation database
echo "Building documentation database..."
./themis_docs_builder \
    --input "$INPUT_DIR" \
    --output "$OUTPUT_DB" \
    --format markdown \
    --namespace "$NAMESPACE" \
    --recursive \
    --read-only \
    --validate \
    --verbose

echo ""
echo "============================================================"
echo "Build complete!"
echo "============================================================"
echo "Database: $OUTPUT_DB"
echo ""
echo "To use with ThemisDB, add to config/docs_assistant.yaml:"
echo ""
echo "docs_assistant:"
echo "  databases:"
echo "    - name: \"company_docs\""
echo "      path: \"$OUTPUT_DB\""
echo "      namespace: \"$NAMESPACE\""
echo "      read_only: true"
echo ""
