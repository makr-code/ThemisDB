#!/usr/bin/bash
#
# PDF Generation Script for ThemisDB Compendium
# Modern Book Layout with Professional Typography
#
# Usage: ./generate_pdf.sh
#

set -e

echo "========================================"
echo "ThemisDB Compendium PDF Generator"
echo "========================================"
echo ""

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/../../pdf_output"
OUTPUT_FILE="${OUTPUT_DIR}/ThemisDB-Compendium-v1.3.4-$(date +%Y%m%d).pdf"
TEMP_DIR="/tmp/themis_pdf_build"

# Create output directory
mkdir -p "${OUTPUT_DIR}"
mkdir -p "${TEMP_DIR}"

echo "📚 Step 1: Collecting chapters..."
CHAPTERS=(
    "preface.md"
    "chapter_00_genesis.md"
    "chapter_01_introduction.md"
    "chapter_02_architecture.md"
    "chapter_03_multimodel.md"
    "chapter_05_relational.md"
    "chapter_06_graph.md"
    "chapter_08_storage_layer.md"
    "chapter_10_enterprise.md"
    "chapter_11_realtime.md"
    "chapter_15_analytics.md"
    "chapter_16_sharding.md"
    "chapter_17_llm_integration.md"
    "chapter_19_monitoring_observability.md"
    "chapter_21_performance.md"
    "chapter_24_ai_ethics.md"
    "appendix_literatur.md"
    "appendix_d_feature_status.md"
)

# Concatenate all chapters
COMBINED_MD="${TEMP_DIR}/compendium_combined.md"
> "${COMBINED_MD}"

# Add title page
cat > "${COMBINED_MD}" << 'EOF'
---
title: "ThemisDB Compendium"
subtitle: "Das vollständige technische Handbuch"
author: "ThemisDB Development Team"
date: "Version 1.3.4"
lang: de-DE
papersize: a4
geometry:
  - margin=25mm
fontsize: 11pt
mainfont: "Georgia"
sansfont: "Helvetica Neue"
monofont: "Fira Code"
linkcolor: purple
urlcolor: purple
toccolor: black
toc: true
toc-depth: 3
numbersections: true
header-includes: |
  \usepackage{fancyhdr}
  \pagestyle{fancy}
  \fancyhead[L]{ThemisDB Compendium}
  \fancyhead[R]{Version 1.3.4}
  \usepackage{listings}
  \lstset{
    basicstyle=\ttfamily\footnotesize,
    breaklines=true,
    frame=single,
    backgroundcolor=\color{gray!10}
  }
---

\newpage

# Vorwort

Willkommen zum ThemisDB Compendium - dem umfassenden technischen Handbuch für ThemisDB Version 1.3.4.

**Aktueller Stand:** ~135.500 Wörter (≈271 Seiten)

**Ziel:** 1.000 Seiten umfassendes technisches Referenzwerk

---

\newpage

EOF

# Append all chapters
for chapter in "${CHAPTERS[@]}"; do
    chapter_file="${SCRIPT_DIR}/${chapter}"
    if [ -f "${chapter_file}" ]; then
        echo "  ✓ Adding: ${chapter}"
        echo "" >> "${COMBINED_MD}"
        echo "\\newpage" >> "${COMBINED_MD}"
        echo "" >> "${COMBINED_MD}"
        cat "${chapter_file}" >> "${COMBINED_MD}"
    else
        echo "  ⚠ Skipping (not found): ${chapter}"
    fi
done

echo ""
echo "📝 Step 2: Checking Pandoc availability..."
if ! command -v pandoc &> /dev/null; then
    echo "❌ Pandoc not found. Installing..."
    
    # Try to install pandoc
    if command -v apt-get &> /dev/null; then
        sudo apt-get update && sudo apt-get install -y pandoc texlive-xetex texlive-fonts-recommended texlive-latex-extra
    elif command -v brew &> /dev/null; then
        brew install pandoc
        brew install --cask basictex
    else
        echo "❌ Cannot install Pandoc automatically. Please install manually:"
        echo "   Ubuntu/Debian: sudo apt-get install pandoc texlive-xetex"
        echo "   macOS: brew install pandoc && brew install --cask basictex"
        exit 1
    fi
fi

pandoc_version=$(pandoc --version | head -n1)
echo "  ✓ Pandoc found: ${pandoc_version}"

echo ""
echo "🎨 Step 3: Applying custom styles..."
# Copy CSS for reference (Pandoc will use SCSS via variables)
cp "${SCRIPT_DIR}/styles_modern_book.scss" "${TEMP_DIR}/"

echo ""
echo "🔨 Step 4: Generating PDF with Pandoc..."
echo "  Output: ${OUTPUT_FILE}"

pandoc "${COMBINED_MD}" \
    -o "${OUTPUT_FILE}" \
    --from=markdown+grid_tables+pipe_tables+simple_tables+multiline_tables \
    --pdf-engine=xelatex \
    --toc \
    --toc-depth=3 \
    --number-sections \
    --highlight-style=tango \
    --variable=colorlinks:true \
    --variable=linkcolor:purple \
    --variable=urlcolor:purple \
    --variable=toccolor:black \
    --variable=geometry:margin=25mm \
    --variable=fontsize:11pt \
    --variable=mainfont:"Georgia" \
    --variable=sansfont:"Helvetica Neue" \
    --variable=monofont:"Courier New" \
    --variable=linestretch:1.3 \
    --template=eisvogel \
    --listings \
    2>&1 | tee "${TEMP_DIR}/pandoc_log.txt"

# Check if PDF was created
if [ -f "${OUTPUT_FILE}" ]; then
    file_size=$(du -h "${OUTPUT_FILE}" | cut -f1)
    echo ""
    echo "✅ PDF generated successfully!"
    echo "   📄 File: ${OUTPUT_FILE}"
    echo "   📊 Size: ${file_size}"
    echo ""
    
    # Generate metadata
    echo "📋 PDF Metadata:"
    echo "   - Pages: ~271"
    echo "   - Words: ~135,500"
    echo "   - Chapters: ${#CHAPTERS[@]}"
    echo "   - Generated: $(date '+%Y-%m-%d %H:%M:%S')"
    echo ""
    
    # Save build info
    cat > "${OUTPUT_DIR}/build_info.txt" << EOF
ThemisDB Compendium PDF Build Information
==========================================

Build Date: $(date '+%Y-%m-%d %H:%M:%S')
Output File: ${OUTPUT_FILE}
File Size: ${file_size}

Chapters Included: ${#CHAPTERS[@]}
$(for chapter in "${CHAPTERS[@]}"; do echo "  - ${chapter}"; done)

Pandoc Version: ${pandoc_version}

Build Log: ${TEMP_DIR}/pandoc_log.txt
EOF
    
    echo "📝 Build info saved to: ${OUTPUT_DIR}/build_info.txt"
    echo ""
    echo "========================================"
    echo "✨ PDF Generation Complete!"
    echo "========================================"
else
    echo ""
    echo "❌ PDF generation failed!"
    echo "   Check log: ${TEMP_DIR}/pandoc_log.txt"
    exit 1
fi
