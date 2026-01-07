#!/bin/bash
# Build-Script: HTML + SVG → PDF

cd /mnt/c/VCC/themis/compendium

echo "📦 Kopiere SVG-Dateien in PDF-Verzeichnis für Build..."
cp -v temp/mermaid_*.svg pdf/ 2>&1 | head -20

echo ""
echo "🔨 Konvertiere HTML zu PDF..."
weasyprint ./pdf/ThemisDB-Kompendium-v1.3.4-print.html ./pdf/ThemisDB-Kompendium-v1.3.4-print.pdf 2>&1 | grep -v 'WARNING.*notdef' | tail -10

echo ""
echo "✅ PDF erstellt!"
ls -lh ./pdf/ThemisDB-Kompendium-v1.3.4-print.pdf

echo ""
echo "🧹 Bereinige temporäre SVGs aus PDF-Verzeichnis..."
rm -v pdf/mermaid_*.svg
