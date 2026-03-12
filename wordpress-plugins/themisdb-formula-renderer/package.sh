#!/bin/bash
#
# Package ThemisDB Formula Renderer Plugin for WordPress
#

PLUGIN_NAME="themisdb-formula-renderer"
PLUGIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="${PLUGIN_DIR}/.."
OUTPUT_FILE="${PACKAGE_DIR}/${PLUGIN_NAME}.zip"

echo "Packaging ${PLUGIN_NAME}..."

# Remove old package if exists
if [ -f "${OUTPUT_FILE}" ]; then
    rm "${OUTPUT_FILE}"
    echo "Removed old package"
fi

# Create package
cd "${PACKAGE_DIR}" || exit 1

# Ensure standalone packages contain the updater class.
SHARED_UPDATER="${PACKAGE_DIR}/includes/class-themisdb-plugin-updater.php"
PLUGIN_UPDATER_DIR="${PACKAGE_DIR}/${PLUGIN_NAME}/includes"
if [ -f "${SHARED_UPDATER}" ]; then
    mkdir -p "${PLUGIN_UPDATER_DIR}"
    cp "${SHARED_UPDATER}" "${PLUGIN_UPDATER_DIR}/class-themisdb-plugin-updater.php"
fi

zip -r "${OUTPUT_FILE}" "${PLUGIN_NAME}" \
    -x "${PLUGIN_NAME}/.git/*" \
    -x "${PLUGIN_NAME}/.gitignore" \
    -x "${PLUGIN_NAME}/package.sh" \
    -x "${PLUGIN_NAME}/*.zip" \
    -x "${PLUGIN_NAME}/.DS_Store" \
    -x "${PLUGIN_NAME}/.vscode/*" \
    -x "${PLUGIN_NAME}/.idea/*" \
    -x "${PLUGIN_NAME}/node_modules/*"

if [ $? -eq 0 ]; then
    echo "✓ Package created: ${OUTPUT_FILE}"
    echo "✓ Size: $(du -h "${OUTPUT_FILE}" | cut -f1)"
else
    echo "✗ Error creating package"
    exit 1
fi

# List contents
echo ""
echo "Package contents:"
unzip -l "${OUTPUT_FILE}" | tail -n +4 | head -n -2

exit 0
