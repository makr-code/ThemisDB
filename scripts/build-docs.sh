#!/bin/bash
# Build ThemisDB Documentation with MkDocs
# Generates static site and PDF export

set -e

echo "=== Build ThemisDB Documentation ==="

# Check if requirements are met
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is required"
    exit 1
fi

# Install/update dependencies
echo "Installing documentation dependencies..."
pip3 install -r requirements-docs.txt --upgrade

# Clean previous build
echo "Cleaning previous build..."
rm -rf site/

# Build MkDocs site
echo "Building MkDocs site..."
mkdocs build --clean

# Generate PDF if wkhtmltopdf is available
if command -v wkhtmltopdf &> /dev/null; then
    echo "Generating PDF..."
    ./scripts/export_pdf_wkhtml.sh
else
    echo "⚠️  wkhtmltopdf not found. Skipping PDF generation."
    echo "   Install with: sudo apt-get install wkhtmltopdf (Debian/Ubuntu)"
    echo "             or: brew install wkhtmltopdf (macOS)"
fi

echo "✅ Documentation build complete!"
echo "   Static site: ./site/"
if [ -f "docs/ThemisDB-Documentation.pdf" ]; then
    echo "   PDF: ./docs/ThemisDB-Documentation.pdf"
    ls -lh docs/ThemisDB-Documentation.pdf
fi
