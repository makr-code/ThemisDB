#!/bin/bash
# Package script for ThemisDB Graph Navigation plugin

set -e

PLUGIN_NAME="themisdb-graph-navigation"
OUTPUT_DIR="dist"
ZIP_NAME="${PLUGIN_NAME}.zip"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

mkdir -p "${SCRIPT_DIR}/${OUTPUT_DIR}"
rm -f "${SCRIPT_DIR}/${OUTPUT_DIR}/${ZIP_NAME}"

# Ensure updater exists in plugin includes for standalone distribution
mkdir -p "${SCRIPT_DIR}/includes"
if [ -f "${ROOT_DIR}/includes/class-themisdb-plugin-updater.php" ]; then
    cp "${ROOT_DIR}/includes/class-themisdb-plugin-updater.php" "${SCRIPT_DIR}/includes/class-themisdb-plugin-updater.php"
fi

cd "${ROOT_DIR}"
zip -r "${SCRIPT_DIR}/${OUTPUT_DIR}/${ZIP_NAME}" "${PLUGIN_NAME}" \
  -x "${PLUGIN_NAME}/.git/*" \
  -x "${PLUGIN_NAME}/.DS_Store" \
  -x "${PLUGIN_NAME}/node_modules/*" \
  -x "${PLUGIN_NAME}/dist/*"

echo "Created: ${SCRIPT_DIR}/${OUTPUT_DIR}/${ZIP_NAME}"
