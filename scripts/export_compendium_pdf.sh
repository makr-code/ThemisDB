#!/bin/bash
# Export ThemisDB Compendium to PDF using Pandoc
# This script converts all compendium markdown files to a single PDF

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
COMPENDIUM_DIR="$REPO_ROOT/docs/compendium"
OUTPUT_FILE="$REPO_ROOT/ThemisDB-Compendium.pdf"

echo "=== Exporting ThemisDB Compendium to PDF ==="
echo "Repository root: $REPO_ROOT"
echo "Compendium directory: $COMPENDIUM_DIR"
echo "Output file: $OUTPUT_FILE"
echo ""

# Check if pandoc is installed
if ! command -v pandoc &> /dev/null; then
    echo "❌ Error: pandoc is not installed"
    echo ""
    echo "Please install pandoc:"
    echo "  - Ubuntu/Debian: sudo apt-get install pandoc texlive-latex-base texlive-latex-extra"
    echo "  - macOS: brew install pandoc basictex"
    echo "  - Windows: Download from https://pandoc.org/installing.html"
    exit 1
fi

echo "✅ Found pandoc: $(pandoc --version | head -1)"
echo ""

# Change to compendium directory
cd "$COMPENDIUM_DIR"

# Define chapter order
CHAPTERS=(
    "preface.md"
    "index.md"
    "chapter_01_introduction.md"
    "chapter_02_architecture.md"
    "chapter_02_5_mvcc_timeline.md"
    "chapter_03_multimodel.md"
    "chapter_04_installation.md"
    "chapter_05_relational.md"
    "chapter_06_graph.md"
    "chapter_07_document.md"
    "chapter_08_vector.md"
    "chapter_09_timeseries.md"
    "chapter_10_enterprise.md"
    "chapter_11_realtime.md"
    "chapter_12_computervision.md"
    "chapter_13_fulltext.md"
    "chapter_14_geospatial.md"
    "chapter_15_analytics.md"
    "chapter_16_ml.md"
    "chapter_18_monitoring.md"
    "chapter_19_backup.md"
    "chapter_20_performance.md"
    "chapter_21_clients.md"
)

# Check which files exist
EXISTING_FILES=()
for chapter in "${CHAPTERS[@]}"; do
    if [ -f "$chapter" ]; then
        EXISTING_FILES+=("$chapter")
    else
        echo "⚠️  Warning: $chapter not found, skipping"
    fi
done

echo ""
echo "📄 Processing ${#EXISTING_FILES[@]} chapters..."
echo ""

# Generate PDF with pandoc
echo "🔄 Generating PDF..."
pandoc "${EXISTING_FILES[@]}" \
    --from markdown \
    --to pdf \
    --output "$OUTPUT_FILE" \
    --toc \
    --toc-depth=3 \
    --number-sections \
    --pdf-engine=pdflatex \
    --variable documentclass=report \
    --variable papersize=a4 \
    --variable geometry:margin=2.5cm \
    --variable fontsize=11pt \
    --variable linkcolor=blue \
    --variable urlcolor=blue \
    --metadata title="ThemisDB Kompendium" \
    --metadata author="ThemisDB Team" \
    --metadata date="$(date +%Y-%m-%d)" \
    --highlight-style=tango \
    --verbose 2>&1 | grep -v "Missing character"

# Check if PDF was created
if [ -f "$OUTPUT_FILE" ]; then
    FILE_SIZE=$(du -h "$OUTPUT_FILE" | cut -f1)
    echo ""
    echo "✅ PDF export complete!"
    echo ""
    echo "📁 Output file: $OUTPUT_FILE"
    echo "📊 File size: $FILE_SIZE"
    echo ""
else
    echo ""
    echo "❌ Error: PDF generation failed"
    exit 1
fi

# Optional: Optimize with Ghostscript if available
if command -v gs &> /dev/null; then
    echo "🔄 Optimizing PDF with Ghostscript..."
    TEMP_FILE="${OUTPUT_FILE}.tmp"
    
    gs -sDEVICE=pdfwrite \
       -dCompatibilityLevel=1.4 \
       -dPDFSETTINGS=/prepress \
       -dNOPAUSE \
       -dQUIET \
       -dBATCH \
       -sOutputFile="$TEMP_FILE" \
       "$OUTPUT_FILE" 2>/dev/null
    
    if [ -f "$TEMP_FILE" ] && [ -s "$TEMP_FILE" ]; then
        mv "$TEMP_FILE" "$OUTPUT_FILE"
        OPTIMIZED_SIZE=$(du -h "$OUTPUT_FILE" | cut -f1)
        echo "✅ Optimization complete! New size: $OPTIMIZED_SIZE"
        echo ""
    else
        rm -f "$TEMP_FILE"
        echo "⚠️  Optimization failed, keeping original"
        echo ""
    fi
fi

echo "=== Export Complete ==="
