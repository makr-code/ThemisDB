#!/usr/bin/bash
set -e

echo "=========================================="
echo "ThemisDB Compendium PDF Generator (Basic)"
echo "=========================================="
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/../../pdf_output"
OUTPUT_FILE="${OUTPUT_DIR}/ThemisDB-Compendium-v1.4.0-alpha-$(date +%Y%m%d).pdf"
TEMP_DIR="/tmp/themis_pdf_build"

mkdir -p "${OUTPUT_DIR}"
mkdir -p "${TEMP_DIR}"

echo "�� Collecting chapters..."
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
    "appendix_h_glossary.md"
)

COMBINED_MD="${TEMP_DIR}/compendium_combined.md"
> "${COMBINED_MD}"

cat > "${COMBINED_MD}" << 'EOF'
---
title: "ThemisDB Compendium"
subtitle: "Das vollständige technische Handbuch"
author: "ThemisDB Development Team"
date: "Version 1.4.0-alpha"
lang: de-DE
papersize: a4
geometry:
  - margin=25mm
fontsize: 11pt
toc: true
toc-depth: 3
numbersections: true
---

\newpage

# Vorwort

Willkommen zum ThemisDB Compendium - dem umfassenden technischen Handbuch für ThemisDB Version 1.4.0-alpha.

---

\newpage

EOF

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
echo "🔨 Generating PDF with Pandoc..."
echo "  Output: ${OUTPUT_FILE}"

pandoc "${COMBINED_MD}" \
    -o "${OUTPUT_FILE}" \
    --from=markdown+grid_tables+pipe_tables+simple_tables+multiline_tables \
    --pdf-engine=xelatex \
    --toc \
    --toc-depth=3 \
    --number-sections \
    2>&1

if [ -f "${OUTPUT_FILE}" ]; then
    file_size=$(du -h "${OUTPUT_FILE}" | cut -f1)
    echo ""
    echo "✅ PDF generated successfully!"
    echo "   📄 File: ${OUTPUT_FILE}"
    echo "   📊 Size: ${file_size}"
    echo ""
    echo "=========================================="
    echo "✨ PDF Generation Complete!"
    echo "=========================================="
else
    echo ""
    echo "❌ PDF generation failed!"
    exit 1
fi
