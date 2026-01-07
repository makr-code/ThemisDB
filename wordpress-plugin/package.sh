#!/bin/bash

# ThemisDB Downloads WordPress Plugin - Packaging Script
# Creates a ZIP file ready for WordPress installation

set -e

PLUGIN_NAME="themisdb-downloads"
VERSION="1.0.0"
PLUGIN_DIR="./themisdb-downloads"
OUTPUT_DIR="./dist"
ZIP_NAME="${PLUGIN_NAME}-${VERSION}.zip"

echo "=========================================="
echo "ThemisDB WordPress Plugin Packaging Script"
echo "=========================================="
echo ""

# Check if plugin directory exists
if [ ! -d "$PLUGIN_DIR" ]; then
    echo "❌ Error: Plugin directory not found: $PLUGIN_DIR"
    exit 1
fi

# Create dist directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Remove old ZIP if exists
if [ -f "$OUTPUT_DIR/$ZIP_NAME" ]; then
    echo "🗑️  Removing old package: $ZIP_NAME"
    rm "$OUTPUT_DIR/$ZIP_NAME"
fi

echo "📦 Creating plugin package..."
echo ""

# Store original directory
ORIGINAL_DIR=$(pwd)

# Create ZIP archive
cd "$(dirname "$PLUGIN_DIR")"
zip -r "$OUTPUT_DIR/$ZIP_NAME" "$(basename "$PLUGIN_DIR")" \
    -x "*.git*" \
    -x "*__pycache__*" \
    -x "*.DS_Store" \
    -x "*.tmp" \
    -x "*.bak" \
    -x "*.swp" \
    -x "*/node_modules/*" \
    -x "*/vendor/*" \
    -x "*/.vscode/*" \
    -x "*/.idea/*"

# Return to original directory
cd "$ORIGINAL_DIR"

echo ""
echo "✅ Package created successfully!"
echo ""
echo "📋 Package Information:"
echo "   Name: $ZIP_NAME"
echo "   Size: $(du -h "$OUTPUT_DIR/$ZIP_NAME" | cut -f1)"
echo "   Location: $OUTPUT_DIR/$ZIP_NAME"
echo ""

# Calculate SHA256 checksum
if command -v sha256sum &> /dev/null; then
    echo "🔐 Calculating SHA256 checksum..."
    SHA256=$(sha256sum "$OUTPUT_DIR/$ZIP_NAME" | cut -d' ' -f1)
    echo "$SHA256  $ZIP_NAME" > "$OUTPUT_DIR/$ZIP_NAME.sha256"
    echo "   SHA256: $SHA256"
    echo "   Checksum saved to: $OUTPUT_DIR/$ZIP_NAME.sha256"
    echo ""
fi

# Verify package contents
echo "📄 Package Contents:"
unzip -l "$OUTPUT_DIR/$ZIP_NAME" | head -20
echo ""
echo "   (showing first 20 files...)"
echo ""

# Test package structure
echo "🧪 Verifying package structure..."
REQUIRED_FILES=(
    "$PLUGIN_NAME/themisdb-downloads.php"
    "$PLUGIN_NAME/README.md"
    "$PLUGIN_NAME/includes/class-github-api.php"
    "$PLUGIN_NAME/includes/class-admin.php"
    "$PLUGIN_NAME/includes/class-shortcodes.php"
    "$PLUGIN_NAME/assets/css/style.css"
    "$PLUGIN_NAME/assets/js/script.js"
)

MISSING_FILES=0
for file in "${REQUIRED_FILES[@]}"; do
    if ! unzip -l "$OUTPUT_DIR/$ZIP_NAME" | grep -q "$file"; then
        echo "   ❌ Missing: $file"
        MISSING_FILES=$((MISSING_FILES + 1))
    fi
done

if [ $MISSING_FILES -eq 0 ]; then
    echo "   ✅ All required files present"
else
    echo "   ⚠️  Warning: $MISSING_FILES required file(s) missing!"
fi

echo ""
echo "=========================================="
echo "✨ Packaging Complete!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Test the package in a WordPress installation"
echo "  2. Upload to WordPress Admin → Plugins → Add New"
echo "  3. Or upload to GitHub as a release asset"
echo ""
echo "Upload command (GitHub CLI):"
echo "  gh release upload v${VERSION} $OUTPUT_DIR/$ZIP_NAME $OUTPUT_DIR/$ZIP_NAME.sha256"
echo ""
