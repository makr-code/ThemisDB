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
# wkhtmltopdf may exit with code 1 due to network errors (missing fonts, badges)
# but still produces a valid PDF. We check if the PDF file was created instead of exit code.
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
    "$OUTPUT_PDF" 2>&1 | grep -v "Failed to load https://" | grep -v "ContentNotFoundError" || true

# Check if PDF was created (even with warnings/errors)
if [ -f "$OUTPUT_PDF" ] && [ -s "$OUTPUT_PDF" ]; then
    echo "âœ… PDF created: $OUTPUT_PDF"
    ls -lh "$OUTPUT_PDF"
else
    echo "âŒ PDF creation failed - file not created or empty"
    exit 1
fi
