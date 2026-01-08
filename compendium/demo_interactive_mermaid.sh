#!/bin/bash
# Demo Script für Interactive Mermaid PDF Generation
# Zeigt alle Features
# 
# HINWEIS: Dieses Script installiert Python-Pakete mit pip.
# Wenn Sie das nicht möchten, brechen Sie ab und installieren Sie manuell:
#   pip install --user reportlab pypdf qrcode[pil] pillow markdown

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Interactive Mermaid PDF - Demo                                ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Determine cross-platform temp directory
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    TEMP_OUTPUT="$TEMP/themis_interactive_pdf"
else
    TEMP_OUTPUT="${TMPDIR:-/tmp}/themis_interactive_pdf"
fi

# Check dependencies
echo "📦 Checking dependencies..."
command -v python3 >/dev/null 2>&1 || { echo "❌ Python 3 required"; exit 1; }
echo "   ✓ Python 3 found"

python3 -c "import reportlab" 2>/dev/null || {
    echo "   ⚠ reportlab not found"
    read -p "   Install Python packages now? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        pip install --user reportlab pypdf qrcode[pil] pillow markdown
    else
        echo "   Please install manually: pip install reportlab pypdf qrcode[pil] pillow markdown"
        exit 1
    fi
}
echo "   ✓ Python packages OK"

# Generate interactive content
echo ""
echo "🎨 Generating interactive diagrams..."
cd "$SCRIPT_DIR"
python3 generate_interactive_pdf_with_embeds.py

# Check output
echo ""
echo "📂 Generated files:"
echo ""

if [ -d "$TEMP_OUTPUT/mermaid_htmls" ]; then
    HTML_COUNT=$(ls -1 "$TEMP_OUTPUT/mermaid_htmls/"*.html 2>/dev/null | wc -l)
    echo "   📄 HTML Files: $HTML_COUNT"
    echo "      Location: $TEMP_OUTPUT/mermaid_htmls/"
fi

if [ -d "$TEMP_OUTPUT/qr_codes" ]; then
    QR_COUNT=$(ls -1 "$TEMP_OUTPUT/qr_codes/"*.png 2>/dev/null | wc -l)
    echo "   📱 QR Codes: $QR_COUNT"
    echo "      Location: $TEMP_OUTPUT/qr_codes/"
fi

if [ -f "$TEMP_OUTPUT/diagrams_metadata.json" ]; then
    echo "   📋 Metadata: diagrams_metadata.json"
fi

# Demo: Open first HTML
echo ""
echo "🌐 Demo: Opening first interactive diagram..."
FIRST_HTML=$(ls -1 "$TEMP_OUTPUT/mermaid_htmls/"*.html 2>/dev/null | head -1)

if [ -n "$FIRST_HTML" ]; then
    echo "   File: $(basename "$FIRST_HTML")"
    
    # Try to open in browser
    if command -v xdg-open >/dev/null 2>&1; then
        xdg-open "$FIRST_HTML" &
        echo "   ✓ Opened in browser"
    elif command -v open >/dev/null 2>&1; then
        open "$FIRST_HTML" &
        echo "   ✓ Opened in browser"
    else
        echo "   ℹ️  Manual open: file://$FIRST_HTML"
    fi
fi

# Setup local server for viewer
echo ""
echo "🖥️  Interactive PDF Viewer Setup:"
echo ""
echo "   1. Copy viewer files:"
echo "      cp $SCRIPT_DIR/interactive_pdf_viewer.html $TEMP_OUTPUT/"
echo "      cp $TEMP_OUTPUT/diagrams_metadata.json $TEMP_OUTPUT/"
echo ""
echo "   2. Start local server:"
echo "      cd $TEMP_OUTPUT"
echo "      python3 -m http.server 8000"
echo ""
echo "   3. Open in browser:"
echo "      http://localhost:8000/interactive_pdf_viewer.html"
echo ""

# Summary
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Summary                                                       ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "✅ Interactive HTML diagrams generated"
echo "✅ QR codes for mobile access created"
echo "✅ Metadata file for viewer ready"
echo ""
echo "📖 Next steps:"
echo "   1. Test HTML files (opened in browser)"
echo "   2. Upload to GitHub Pages for QR functionality"
echo "   3. Use custom PDF.js viewer for full interactivity"
echo ""
echo "📚 Documentation:"
echo "   - README_INTERACTIVE.md - Quick Start"
echo "   - INTERACTIVE_PDF_MERMAID_GUIDE.md - Technical Details"
echo ""
echo "✨ Demo complete!"
