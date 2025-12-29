#!/bin/bash
# Build ThemisDB Compendium PDF with optimization
# Generates the compendium PDF and optimizes it to reduce file size

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=== Building ThemisDB Compendium PDF ==="
echo "Repository root: $REPO_ROOT"
echo ""

# Check if dependencies are installed
if ! python3 -c "import mkdocs_with_pdf" 2>/dev/null; then
    echo "Installing documentation dependencies..."
    pip3 install -r "$REPO_ROOT/requirements-docs.txt"
fi

# Determine which compendium directory to use
COMPENDIUM_DIR="$REPO_ROOT/compendium_site_pdf"
if [ ! -d "$COMPENDIUM_DIR" ]; then
    COMPENDIUM_DIR="$REPO_ROOT/compendium_site"
fi

if [ ! -d "$COMPENDIUM_DIR" ]; then
    echo "Error: Compendium directory not found"
    exit 1
fi

echo "Using compendium directory: $COMPENDIUM_DIR"
cd "$COMPENDIUM_DIR"

# Build PDF with mkdocs-with-pdf plugin
echo ""
echo "Building PDF with mkdocs-with-pdf..."
export ENABLE_PDF_EXPORT=1
mkdocs build --config-file mkdocs-compendium.yml

# Find the generated PDF
PDF_FILE=$(find "$REPO_ROOT" -maxdepth 1 -name "ThemisDB-Kompendium-*.pdf" -type f | head -1)

if [ -z "$PDF_FILE" ] || [ ! -f "$PDF_FILE" ]; then
    echo "Error: PDF generation failed - file not found"
    exit 1
fi

echo ""
echo "PDF generated: $PDF_FILE"
ls -lh "$PDF_FILE"

# Optimize the PDF if Ghostscript is available
echo ""
if command -v gs &> /dev/null; then
    echo "Optimizing PDF with Ghostscript..."
    OPTIMIZED_PDF="${PDF_FILE%.pdf}-optimized.pdf"
    
    "$SCRIPT_DIR/optimize_pdf.sh" "$PDF_FILE" "$OPTIMIZED_PDF"
    
    # Replace original with optimized version
    if [ -f "$OPTIMIZED_PDF" ] && [ -s "$OPTIMIZED_PDF" ]; then
        echo ""
        echo "Replacing original with optimized PDF..."
        mv "$OPTIMIZED_PDF" "$PDF_FILE"
        echo "✅ Optimization complete!"
    else
        echo "⚠️  Optimization failed, keeping original PDF"
    fi
else
    echo "⚠️  Ghostscript not found - skipping optimization"
    echo "   Install with:"
    echo "     - Ubuntu/Debian: sudo apt-get install ghostscript"
    echo "     - macOS: brew install ghostscript"
    echo ""
    echo "   The PDF has been generated with custom CSS optimizations,"
    echo "   but could be further reduced with Ghostscript compression."
fi

echo ""
echo "=== Build Complete ==="
echo "Final PDF: $PDF_FILE"
ls -lh "$PDF_FILE"

# Verify PDF has bookmarks
echo ""
echo "Verifying PDF structure..."
python3 << 'EOF'
try:
    from pypdf import PdfReader
    import sys
    import os
    
    pdf_path = sys.argv[1] if len(sys.argv) > 1 else None
    if not pdf_path or not os.path.exists(pdf_path):
        print("Error: PDF file not found for verification")
        sys.exit(1)
        
    reader = PdfReader(pdf_path)
    print(f"✓ PDF has {len(reader.pages)} pages")
    
    if reader.outline:
        bookmark_count = len(reader.outline) if isinstance(reader.outline, list) else 0
        print(f"✓ PDF has {bookmark_count} top-level bookmarks")
    else:
        print("⚠️  Warning: No bookmarks found")
        
    # Check for internal links
    links_found = False
    for i, page in enumerate(reader.pages[:5]):
        if '/Annots' in page and page['/Annots']:
            links_found = True
            break
    
    if links_found:
        print("✓ PDF has internal links")
    else:
        print("⚠️  Warning: No internal links found in first 5 pages")
        
    print("\n✅ PDF verification complete")
except ImportError:
    print("⚠️  pypdf not available for verification")
except Exception as e:
    print(f"⚠️  Verification error: {e}")
EOF
python3 -c "import sys; sys.argv.append('$PDF_FILE')" 2>/dev/null || true
