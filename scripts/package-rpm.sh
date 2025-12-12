#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION_FILE="$ROOT_DIR/VERSION"
VERSION="${1:-}"

if [[ -z "$VERSION" ]]; then
  if [[ -f "$VERSION_FILE" ]]; then
    VERSION="$(tr -d '\r' < "$VERSION_FILE")"
  else
    echo "Version nicht angegeben. Nutzung: ./scripts/package-rpm.sh <version>" >&2
    exit 1
  fi
fi

OUTPUT_DIR="$ROOT_DIR/release"
BIN_SRC_LINUX="$ROOT_DIR/build-linux/themis_server"
SPEC_SRC="$ROOT_DIR/themisdb.spec"

mkdir -p "$OUTPUT_DIR"

RPMROOT="$OUTPUT_DIR/rpm"
rm -rf "$RPMROOT"
mkdir -p "$RPMROOT/BUILD" "$RPMROOT/RPMS" "$RPMROOT/SOURCES" "$RPMROOT/SPECS" "$RPMROOT/SRPMS"

TARBALL="$RPMROOT/SOURCES/ThemisDB-$VERSION.tar.gz"

# Create source tarball minimal (binary packaging)
TMP_SRC="$OUTPUT_DIR/tmp-src-$VERSION"
rm -rf "$TMP_SRC" && mkdir -p "$TMP_SRC"
mkdir -p "$TMP_SRC/build" "$TMP_SRC/config" "$TMP_SRC/debian" "$TMP_SRC/docs"

if [[ ! -f "$BIN_SRC_LINUX" ]]; then
  echo "Linux Binary nicht gefunden: $BIN_SRC_LINUX" >&2
  exit 1
fi
cp "$BIN_SRC_LINUX" "$TMP_SRC/build/themis_server"
cp -r "$ROOT_DIR/config" "$TMP_SRC/" || true
cp "$ROOT_DIR/README.md" "$TMP_SRC/" 2>/dev/null || true
cp "$ROOT_DIR/CHANGELOG.md" "$TMP_SRC/" 2>/dev/null || true
cp "$ROOT_DIR/debian/themisdb.service" "$TMP_SRC/debian/" 2>/dev/null || true

tar -C "$TMP_SRC/.." -czf "$TARBALL" "$(basename "$TMP_SRC")"
rm -rf "$TMP_SRC"

# Prepare spec
SPEC_DST="$RPMROOT/SPECS/themisdb.spec"
if [[ -f "$SPEC_SRC" ]]; then
  sed -e "s/^Version:.*/Version:        $VERSION/" "$SPEC_SRC" > "$SPEC_DST"
else
  cat > "$SPEC_DST" <<'EOF'
Name:           themisdb
Version:        1.0.0
Release:        1%{?dist}
Summary:        ThemisDB server
License:        MIT
URL:            https://github.com/makr-code/ThemisDB
Source0:        ThemisDB-%{version}.tar.gz

%description
ThemisDB packaged binary.

%prep
%setup -q -n ThemisDB-%{version}

%build
# no-op

%install
mkdir -p %{buildroot}%{_bindir}
install -m 0755 build/themis_server %{buildroot}%{_bindir}/themis_server
mkdir -p %{buildroot}%{_unitdir}
install -m 0644 debian/themisdb.service %{buildroot}%{_unitdir}/themisdb.service
mkdir -p %{buildroot}%{_sysconfdir}/themisdb
cp -r config/* %{buildroot}%{_sysconfdir}/themisdb/ 2>/dev/null || true

%files
%{_bindir}/themis_server
%{_unitdir}/themisdb.service
%config(noreplace) %{_sysconfdir}/themisdb/*

%changelog
* Thu Dec 12 2025 ThemisDB <support@themisdb.org> - %{version}-1
- Initial RPM packaging
EOF
fi

rpmbuild \
  --define "_topdir $RPMROOT" \
  -ba "$SPEC_DST"

echo "✓ RPM Paket erstellt unter: $RPMROOT/RPMS"
