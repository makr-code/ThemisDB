#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION_FILE="$ROOT_DIR/VERSION"
VERSION="${1:-}"

if [[ -z "$VERSION" ]]; then
  if [[ -f "$VERSION_FILE" ]]; then
    VERSION="$(tr -d '\r' < "$VERSION_FILE")"
  else
    echo "Version nicht angegeben. Nutzung: ./scripts/package-deb.sh <version>" >&2
    exit 1
  fi
fi

OUTPUT_DIR="$ROOT_DIR/release"
PKGROOT="$OUTPUT_DIR/themisdb_${VERSION}_amd64"
BIN_SRC_LINUX="$ROOT_DIR/build-linux/themis_server"

mkdir -p "$OUTPUT_DIR"
rm -rf "$PKGROOT"

mkdir -p "$PKGROOT/DEBIAN"
mkdir -p "$PKGROOT/usr/local/bin"
mkdir -p "$PKGROOT/etc/themisdb"
mkdir -p "$PKGROOT/lib/systemd/system"
mkdir -p "$PKGROOT/usr/share/doc/themisdb"

# Binary
if [[ ! -f "$BIN_SRC_LINUX" ]]; then
  echo "Binary nicht gefunden: $BIN_SRC_LINUX" >&2
  exit 1
fi
install -m 0755 "$BIN_SRC_LINUX" "$PKGROOT/usr/local/bin/themis_server"

# Configs & docs
cp -r "$ROOT_DIR/config"/* "$PKGROOT/etc/themisdb/" || true
cp "$ROOT_DIR/README.md" "$PKGROOT/usr/share/doc/themisdb/" 2>/dev/null || true
cp "$ROOT_DIR/CHANGELOG.md" "$PKGROOT/usr/share/doc/themisdb/" 2>/dev/null || true

# Control files (use templates from debian/ but patch version)
sed "s/^Version:.*/Version: ${VERSION}/" "$ROOT_DIR/debian/control" > "$PKGROOT/DEBIAN/control"
cp "$ROOT_DIR/debian/postinst" "$PKGROOT/DEBIAN/postinst" 2>/dev/null || cp "$ROOT_DIR/debian/themisdb.postinst" "$PKGROOT/DEBIAN/postinst" || true
cp "$ROOT_DIR/debian/postrm" "$PKGROOT/DEBIAN/postrm" 2>/dev/null || cp "$ROOT_DIR/debian/themisdb.postrm" "$PKGROOT/DEBIAN/postrm" || true
chmod 0755 "$PKGROOT/DEBIAN/postinst" "$PKGROOT/DEBIAN/postrm" 2>/dev/null || true

# Systemd service
cp "$ROOT_DIR/debian/themisdb.service" "$PKGROOT/lib/systemd/system/themisdb.service"

# Build package
fakeroot dpkg-deb --build "$PKGROOT"

echo "✓ Debian Paket erstellt: $OUTPUT_DIR/themisdb_${VERSION}_amd64.deb"
