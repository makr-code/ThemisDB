#!/bin/bash
# PDF Optimization Script
# Compresses and optimizes PDF files using Ghostscript
# Significantly reduces file size while maintaining quality

set -e

INPUT_PDF="${1}"
OUTPUT_PDF="${2:-${INPUT_PDF%.pdf}-optimized.pdf}"

if [ -z "$INPUT_PDF" ]; then
    echo "Usage: $0 <input.pdf> [output.pdf]"
    echo ""
    echo "Optimizes PDF file to reduce size while maintaining quality."
    echo "Requires Ghostscript (gs) to be installed."
    exit 1
fi

if [ ! -f "$INPUT_PDF" ]; then
    echo "Error: Input file not found: $INPUT_PDF"
    exit 1
fi

# Check if ghostscript is installed
if ! command -v gs &> /dev/null; then
    echo "Warning: Ghostscript (gs) not found."
    echo "Install with:"
    echo "  - Ubuntu/Debian: sudo apt-get install ghostscript"
    echo "  - macOS: brew install ghostscript"
    echo ""
    echo "Skipping optimization - copying file instead"
    cp "$INPUT_PDF" "$OUTPUT_PDF"
    exit 0
fi

echo "=== PDF Optimization ==="
echo "Input:  $INPUT_PDF ($(du -h "$INPUT_PDF" | cut -f1))"
echo "Output: $OUTPUT_PDF"
echo ""

# Optimize PDF with Ghostscript
# Settings explained:
# - PDFSETTINGS=/ebook: Good balance between quality and file size (150 dpi)
# - ColorImageDownsampleType=/Bicubic: High-quality downsampling
# - EmbedAllFonts=true: Ensure fonts are embedded (but subset)
# - SubsetFonts=true: Only embed used glyphs to reduce font overhead
# - CompressPages=true: Compress page content streams
# - OptimizeImages=true: Optimize embedded images
# - PassThroughJPEGImages=false: Recompress JPEGs
# - AutoRotatePages=/None: Don't auto-rotate pages

echo "Running Ghostscript optimization..."
gs -sDEVICE=pdfwrite \
   -dCompatibilityLevel=1.7 \
   -dPDFSETTINGS=/ebook \
   -dNOPAUSE \
   -dQUIET \
   -dBATCH \
   -dDetectDuplicateImages=true \
   -dCompressFonts=true \
   -dEmbedAllFonts=true \
   -dSubsetFonts=true \
   -dCompressPages=true \
   -dColorImageDownsampleType=/Bicubic \
   -dColorImageResolution=150 \
   -dGrayImageDownsampleType=/Bicubic \
   -dGrayImageResolution=150 \
   -dMonoImageDownsampleType=/Bicubic \
   -dMonoImageResolution=150 \
   -dOptimizeImages=true \
   -dPassThroughJPEGImages=false \
   -dAutoRotatePages=/None \
   -dPrinted=false \
   -sOutputFile="$OUTPUT_PDF" \
   "$INPUT_PDF"

if [ ! -f "$OUTPUT_PDF" ] || [ ! -s "$OUTPUT_PDF" ]; then
    echo "Error: Optimization failed - output file not created or empty"
    exit 1
fi

echo ""
echo "✅ Optimization complete!"
echo ""
echo "File sizes:"
echo "  Before: $(du -h "$INPUT_PDF" | cut -f1)"
echo "  After:  $(du -h "$OUTPUT_PDF" | cut -f1)"

# Calculate size reduction
BEFORE=$(stat -c%s "$INPUT_PDF" 2>/dev/null || stat -f%z "$INPUT_PDF" 2>/dev/null)
AFTER=$(stat -c%s "$OUTPUT_PDF" 2>/dev/null || stat -f%z "$OUTPUT_PDF" 2>/dev/null)

if [ -n "$BEFORE" ] && [ -n "$AFTER" ] && [ "$BEFORE" -gt 0 ]; then
    REDUCTION=$((100 - (AFTER * 100 / BEFORE)))
    echo "  Reduction: ${REDUCTION}%"
else
    echo "  Reduction: Unable to calculate"
fi
