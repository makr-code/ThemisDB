#!/bin/bash

# ThemisDB Compendium Downloads - Package Script
# Creates a distributable ZIP file of the plugin

PLUGIN_NAME="themisdb-compendium-downloads"
VERSION="1.0.0"
OUTPUT_DIR="dist"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=========================================="
echo "ThemisDB Compendium Downloads Packager"
echo "=========================================="
echo ""
echo "Plugin: $PLUGIN_NAME"
echo "Version: $VERSION"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Create temporary directory for packaging
TEMP_DIR=$(mktemp -d) || { echo "❌ Failed to create temp directory"; exit 1; }
PACKAGE_DIR="$TEMP_DIR/$PLUGIN_NAME"
mkdir -p "$PACKAGE_DIR"

echo "📦 Copying plugin files..."

# Copy plugin files
cp -r "$SCRIPT_DIR/includes" "$PACKAGE_DIR/"
cp -r "$SCRIPT_DIR/assets" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/themisdb-compendium-downloads.php" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/README.md" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/CHANGELOG.md" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/INSTALLATION.md" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/QUICKSTART.md" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/PREVIEW.html" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/LICENSE" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/update-info.json" "$PACKAGE_DIR/"

# Ensure standalone packages contain the updater class.
SHARED_UPDATER="$SCRIPT_DIR/../includes/class-themisdb-plugin-updater.php"
if [ -f "$SHARED_UPDATER" ]; then
    cp "$SHARED_UPDATER" "$PACKAGE_DIR/includes/class-themisdb-plugin-updater.php"
fi

# Create languages directory (even if empty for now)
mkdir -p "$PACKAGE_DIR/languages"
echo "# Translation files go here" > "$PACKAGE_DIR/languages/.gitkeep"

echo "✅ Files copied"

# Create ZIP file
echo "🗜️  Creating ZIP archive..."
cd "$TEMP_DIR"
ZIP_FILE="$SCRIPT_DIR/$OUTPUT_DIR/$PLUGIN_NAME-v$VERSION.zip"

if command -v zip &> /dev/null; then
    zip -r "$ZIP_FILE" "$PLUGIN_NAME" -q
    echo "✅ ZIP created: $ZIP_FILE"
else
    echo "⚠️  Warning: 'zip' command not found. Using tar instead..."
    TAR_FILE="$SCRIPT_DIR/$OUTPUT_DIR/$PLUGIN_NAME-v$VERSION.tar.gz"
    tar -czf "$TAR_FILE" "$PLUGIN_NAME"
    echo "✅ TAR.GZ created: $TAR_FILE"
fi

# Cleanup
cd "$SCRIPT_DIR"
rm -rf "$TEMP_DIR"

echo ""
echo "=========================================="
echo "✅ Packaging complete!"
echo "=========================================="
echo ""
echo "Output: $OUTPUT_DIR/$PLUGIN_NAME-v$VERSION.zip"
echo ""
echo "Next steps:"
echo "  1. Test the plugin in a WordPress installation"
echo "  2. Upload to WordPress.org (if approved)"
echo "  3. Or distribute manually to users"
echo ""
