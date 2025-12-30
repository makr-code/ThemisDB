#!/bin/bash
# Export ThemisDB Compendium to both PDF and DOCX formats
# This is a convenience script that calls both export scripts

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================"
echo "  ThemisDB Compendium Export Suite"
echo "========================================"
echo ""
echo "This script will export the compendium to:"
echo "  1. PDF format (ThemisDB-Compendium.pdf)"
echo "  2. DOCX format (ThemisDB-Compendium.docx)"
echo ""

# Export to PDF
echo "========================================="
echo "1/2: Exporting to PDF..."
echo "========================================="
echo ""
"$SCRIPT_DIR/export_compendium_pdf.sh"

echo ""
echo ""

# Export to DOCX
echo "========================================="
echo "2/2: Exporting to DOCX..."
echo "========================================="
echo ""
"$SCRIPT_DIR/export_compendium_docx.sh"

echo ""
echo ""
echo "========================================="
echo "  ✅ All Exports Complete!"
echo "========================================="
echo ""
echo "Generated files in repository root:"
ls -lh "$(cd "$SCRIPT_DIR/.." && pwd)"/ThemisDB-Compendium.* 2>/dev/null || echo "  (Check individual export logs for errors)"
echo ""
