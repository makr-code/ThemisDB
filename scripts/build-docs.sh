#!/bin/bash
# Build ThemisDB Documentation with MkDocs
# Generates static site and optional PDF export
#
# Usage:
#   ./scripts/build-docs.sh                       # site only (no PDF)
#   ENABLE_PDF_EXPORT=1 ./scripts/build-docs.sh   # site + PDF
#
# Output:
#   Static site: ./site/
#   PDF (optional): ./artifacts/docs/ThemisDB-Documentation-v1.3.5.pdf

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

if [ -n "${ENABLE_PDF_EXPORT}" ]; then
    # Build MkDocs site with PDF export (output goes to artifacts/docs/)
    echo "Building MkDocs site (with PDF export)..."
    mkdir -p artifacts/docs
    mkdocs build --clean
    echo "Documentation build complete!"
    echo "   Static site: ./site/"
    if [ -f "artifacts/docs/ThemisDB-Documentation-v1.3.5.pdf" ]; then
        echo "   PDF: ./artifacts/docs/ThemisDB-Documentation-v1.3.5.pdf"
        ls -lh artifacts/docs/ThemisDB-Documentation-v1.3.5.pdf
    fi
else
    # Build MkDocs site without PDF
    echo "Building MkDocs site (no PDF)..."
    mkdocs build --config-file mkdocs-nopdf.yml --clean
    echo "Documentation build complete!"
    echo "   Static site: ./site/"
    echo "   Tip: set ENABLE_PDF_EXPORT=1 to also generate a PDF"
fi
