#!/bin/bash
# Example: Incremental documentation database update
#
# This script demonstrates how to update an existing database
# with new documents without rebuilding everything.

set -e  # Exit on error

# Configuration
INPUT_DIR="/opt/company/new_docs"
OUTPUT_DB="/var/lib/themisdb/company_docs.db"
NAMESPACE="com.mycompany"

echo "============================================================"
echo "ThemisDB Documentation Builder - Incremental Update"
echo "============================================================"
echo ""
echo "Input:     $INPUT_DIR"
echo "Database:  $OUTPUT_DB"
echo "Namespace: $NAMESPACE"
echo ""

# Check if database exists
if [ ! -d "$OUTPUT_DB" ]; then
    echo "Error: Database does not exist: $OUTPUT_DB"
    echo "Run simple_build.sh first to create the initial database."
    exit 1
fi

# Check if input directory exists
if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: Input directory does not exist: $INPUT_DIR"
    exit 1
fi

# Backup existing database
BACKUP_DB="${OUTPUT_DB}.backup.$(date +%Y%m%d_%H%M%S)"
echo "Creating backup: $BACKUP_DB"
cp -r "$OUTPUT_DB" "$BACKUP_DB"

# Perform incremental update
echo "Updating documentation database..."
./themis_docs_builder \
    --input "$INPUT_DIR" \
    --output "$OUTPUT_DB" \
    --format markdown \
    --namespace "$NAMESPACE" \
    --mode incremental \
    --recursive \
    --validate \
    --verbose

echo ""
echo "============================================================"
echo "Update complete!"
echo "============================================================"
echo "Database: $OUTPUT_DB"
echo "Backup:   $BACKUP_DB"
echo ""
echo "If something went wrong, restore from backup:"
echo "  rm -rf $OUTPUT_DB"
echo "  mv $BACKUP_DB $OUTPUT_DB"
echo ""
