#!/bin/bash

# Package script for ThemisDB Order Request Plugin
# Creates a distributable ZIP file

set -e

PLUGIN_NAME="themisdb-order-request"
VERSION="1.0.0"
OUTPUT_DIR="dist"
ZIP_NAME="${PLUGIN_NAME}-v${VERSION}.zip"

echo "📦 Packaging ThemisDB Order Request Plugin v${VERSION}"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Clean up old packages
rm -f "$OUTPUT_DIR"/*.zip

# Create temporary directory for packaging
TEMP_DIR=$(mktemp -d)
PACKAGE_DIR="$TEMP_DIR/$PLUGIN_NAME"

echo "✅ Creating package structure..."
mkdir -p "$PACKAGE_DIR"

# Copy plugin files
cp -r includes "$PACKAGE_DIR/"
cp -r assets "$PACKAGE_DIR/"
cp themisdb-order-request.php "$PACKAGE_DIR/"
cp uninstall.php "$PACKAGE_DIR/"
cp README.md "$PACKAGE_DIR/"
cp INSTALLATION.md "$PACKAGE_DIR/"
cp CHANGELOG.md "$PACKAGE_DIR/"
cp LICENSE "$PACKAGE_DIR/"
cp update-info.json "$PACKAGE_DIR/"

# Ensure standalone packages contain the updater class.
SHARED_UPDATER="../includes/class-themisdb-plugin-updater.php"
if [ -f "$SHARED_UPDATER" ]; then
	cp "$SHARED_UPDATER" "$PACKAGE_DIR/includes/class-themisdb-plugin-updater.php"
fi

# Create empty templates directory
mkdir -p "$PACKAGE_DIR/templates"

echo "✅ Removing development files..."
# Remove development files if any
find "$PACKAGE_DIR" -name ".DS_Store" -delete
find "$PACKAGE_DIR" -name "*.swp" -delete
find "$PACKAGE_DIR" -name "*.bak" -delete
find "$PACKAGE_DIR" -name "*~" -delete

echo "✅ Creating ZIP archive..."
cd "$TEMP_DIR"
zip -r "$ZIP_NAME" "$PLUGIN_NAME" -q

# Move to output directory
mv "$ZIP_NAME" "$OLDPWD/$OUTPUT_DIR/"

# Calculate file size
FILE_SIZE=$(du -h "$OUTPUT_DIR/$ZIP_NAME" | cut -f1)

# Clean up
rm -rf "$TEMP_DIR"

echo ""
echo "🎉 Package created successfully!"
echo "📍 Location: $OUTPUT_DIR/$ZIP_NAME"
echo "📏 Size: $FILE_SIZE"
echo ""
echo "To install:"
echo "1. Upload $ZIP_NAME in WordPress Admin → Plugins → Add New"
echo "2. Or extract to wp-content/plugins/ directory"
echo ""
