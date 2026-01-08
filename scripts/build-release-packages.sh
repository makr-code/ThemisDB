#!/bin/bash
# Build script for Linux release packages

set -e

cd /build/build

echo "=== CPack: Generating Release Packages ==="
echo "Available CPack generators: $(cpack --help | grep -A 20 "Generators")" || true

# Generate packages
echo "Generating TGZ package..."
cpack -G "TGZ" -C Release || true

echo "Generating DEB package..."
cpack -G "DEB" -C Release || true

echo "Generating RPM package..."
cpack -G "RPM" -C Release || true

# List all generated packages
echo ""
echo "=== Generated Packages ==="
find . -maxdepth 1 \( -name "*.tar.gz*" -o -name "*.deb" -o -name "*.rpm" -o -name "*.zip" \) -exec ls -lh {} \;

echo ""
echo "=== Summary ==="
echo "Total packages: $(find . -maxdepth 1 \( -name "*.tar.gz*" -o -name "*.deb" -o -name "*.rpm" -o -name "*.zip" \) | wc -l)"
