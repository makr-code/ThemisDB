#!/bin/bash
# Build Debian package for ThemisDB v1.0.0

set -e

PACKAGE_DIR="release/deb-package/themisdb-1.0.0"

echo "=== Building ThemisDB Debian Package ==="

# Set permissions
chmod 755 "$PACKAGE_DIR/DEBIAN/postinst"
chmod 755 "$PACKAGE_DIR/DEBIAN/prerm"
chmod 755 "$PACKAGE_DIR/usr/local/bin/themis_server"

# Calculate installed size (in KB) and add to control if not present
INSTALLED_SIZE=$(du -sk "$PACKAGE_DIR" | cut -f1)
if ! grep -q "Installed-Size:" "$PACKAGE_DIR/DEBIAN/control"; then
    echo "Installed-Size: $INSTALLED_SIZE" >> "$PACKAGE_DIR/DEBIAN/control"
fi

# Build package
echo "Building .deb package..."
dpkg-deb --build "$PACKAGE_DIR" release/themisdb_1.0.0_amd64.deb

echo "✓ Package built: release/themisdb_1.0.0_amd64.deb"

# Show package info
dpkg-deb --info release/themisdb_1.0.0_amd64.deb
dpkg-deb --contents release/themisdb_1.0.0_amd64.deb

echo ""
echo "=== Package ready for distribution ==="
echo "Install with: sudo dpkg -i themisdb_1.0.0_amd64.deb"
echo "Start service: sudo systemctl start themisdb"
