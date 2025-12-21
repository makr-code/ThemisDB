#!/bin/bash
# Exports the MkDocs print page to PDF with table of contents/bookmarks via wkhtmltopdf

set -e

SOURCE_HTML="${1:-./site/print_page/index.html}"
OUTPUT_PDF="${2:-./docs/ThemisDB-Documentation.pdf}"

echo "=== Export PDF (wkhtmltopdf) ==="

if [ ! -f "$SOURCE_HTML" ]; then
    echo "Error: Source not found: $SOURCE_HTML"
    echo "Please run mkdocs build with print-site plugin first."
    exit 1
fi

# Check if wkhtmltopdf is installed
if ! command -v wkhtmltopdf &> /dev/null; then
    echo "Error: wkhtmltopdf not found"
    echo "Install with: sudo apt-get install wkhtmltopdf (Debian/Ubuntu)"
    echo "           or: brew install wkhtmltopdf (macOS)"
    exit 1
fi

echo "Building PDF from: $SOURCE_HTML"
echo "Output to: $OUTPUT_PDF"

# Create PDF with outline (bookmarks) and local file access
# Note: --outline may be ignored if using unpatched Qt
# Network errors (missing fonts, badges) are ignored as they don't prevent PDF generation
wkhtmltopdf \
    --enable-local-file-access \
    --outline \
    --zoom 1.0 \
    --page-size A4 \
    --margin-top 20mm \
    --margin-bottom 20mm \
    --margin-left 15mm \
    --margin-right 15mm \
    "$SOURCE_HTML" \
    "$OUTPUT_PDF" || true

# Check if PDF was created (even with warnings/errors)
if [ -f "$OUTPUT_PDF" ]; then
    echo "✅ PDF created: $OUTPUT_PDF"
    ls -lh "$OUTPUT_PDF"
else
    echo "❌ PDF creation failed"
    exit 1
fi
