#!/bin/bash
# Export ThemisDB Compendium to Word DOCX using Pandoc
# This script converts all compendium markdown files to a single DOCX file

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
COMPENDIUM_DIR="$REPO_ROOT/docs/compendium"
OUTPUT_FILE="$REPO_ROOT/ThemisDB-Compendium.docx"

echo "=== Exporting ThemisDB Compendium to Word DOCX ==="
echo "Repository root: $REPO_ROOT"
echo "Compendium directory: $COMPENDIUM_DIR"
echo "Output file: $OUTPUT_FILE"
echo ""

# Check if pandoc is installed
if ! command -v pandoc &> /dev/null; then
    echo "❌ Error: pandoc is not installed"
    echo ""
    echo "Please install pandoc:"
    echo "  - Ubuntu/Debian: sudo apt-get install pandoc"
    echo "  - macOS: brew install pandoc"
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
    "chapter_17_llm_integration.md"
    "chapter_18_ml.md"
    "chapter_19_monitoring.md"
    "chapter_20_backup.md"
    "chapter_21_performance.md"
    "chapter_22_clients.md"
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

# Generate DOCX with pandoc
echo "🔄 Generating DOCX..."
pandoc "${EXISTING_FILES[@]}" \
    --from markdown \
    --to docx \
    --output "$OUTPUT_FILE" \
    --toc \
    --toc-depth=3 \
    --number-sections \
    --metadata title="ThemisDB Kompendium" \
    --metadata author="ThemisDB Team" \
    --metadata date="$(date +%Y-%m-%d)" \
    --highlight-style=tango \
    --verbose

# Check if DOCX was created
if [ -f "$OUTPUT_FILE" ]; then
    FILE_SIZE=$(du -h "$OUTPUT_FILE" | cut -f1)
    echo ""
    echo "✅ DOCX export complete!"
    echo ""
    echo "📁 Output file: $OUTPUT_FILE"
    echo "📊 File size: $FILE_SIZE"
    echo ""
    echo "💡 The DOCX file includes:"
    echo "   - Automatic table of contents"
    echo "   - Numbered sections"
    echo "   - Syntax-highlighted AQL code blocks"
    echo "   - All tables and lists"
    echo "   - Cross-references and links"
    echo ""
    echo "📝 You can further customize the document in Microsoft Word or LibreOffice."
    echo ""
else
    echo ""
    echo "❌ Error: DOCX generation failed"
    exit 1
fi

echo "=== Export Complete ==="
