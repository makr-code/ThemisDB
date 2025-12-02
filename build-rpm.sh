#!/bin/bash
# Build RPM package for ThemisDB v1.0.0 (binary package)

set -e

echo "=== Building ThemisDB RPM Package ==="

# Create RPM build structure
mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Copy files to SOURCES
cp release/themis_server_linux ~/rpmbuild/SOURCES/themis_server
cp release/deb-package/themisdb-1.0.0/lib/systemd/system/themisdb.service ~/rpmbuild/SOURCES/
cp README.md LICENSE CHANGELOG.md ~/rpmbuild/SOURCES/

# Create binary RPM spec
cat > ~/rpmbuild/SPECS/themisdb-binary.spec << 'EOF'
Name:           themisdb
Version:        1.0.0
Release:        1%{?dist}
Summary:        Multi-model database with vector, graph, and spatial capabilities

License:        MIT
URL:            https://github.com/makr-code/ThemisDB

BuildArch:      x86_64
Requires:       glibc >= 2.34

%description
ThemisDB is a high-performance, multi-model database that combines document,
graph, vector, and spatial data models in a unified storage engine.

Features:
- Multi-model: documents, graphs, vectors, spatial data
- MVCC transactions with ACID compliance
- Vector search with HNSW indexing
- Spatial queries with R-tree indexing
- Graph traversal and pathfinding
- Field-level encryption and PII detection
- RESTful HTTP API with AQL query language

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/local/bin
mkdir -p %{buildroot}/etc/themisdb
mkdir -p %{buildroot}/usr/lib/systemd/system
mkdir -p %{buildroot}/var/lib/themisdb
mkdir -p %{buildroot}/var/log/themisdb
mkdir -p %{buildroot}/usr/share/doc/themisdb

install -m 755 %{_sourcedir}/themis_server %{buildroot}/usr/local/bin/
install -m 644 %{_sourcedir}/themisdb.service %{buildroot}/usr/lib/systemd/system/
install -m 644 %{_sourcedir}/README.md %{buildroot}/usr/share/doc/themisdb/
install -m 644 %{_sourcedir}/LICENSE %{buildroot}/usr/share/doc/themisdb/
install -m 644 %{_sourcedir}/CHANGELOG.md %{buildroot}/usr/share/doc/themisdb/

%files
/usr/local/bin/themis_server
/usr/lib/systemd/system/themisdb.service
/usr/share/doc/themisdb/README.md
/usr/share/doc/themisdb/LICENSE
/usr/share/doc/themisdb/CHANGELOG.md
%dir /var/lib/themisdb
%dir /var/log/themisdb
%dir /etc/themisdb

%pre
getent group themisdb >/dev/null || groupadd -r themisdb
getent passwd themisdb >/dev/null || \
    useradd -r -g themisdb -d /var/lib/themisdb -s /sbin/nologin \
    -c "ThemisDB Server" themisdb
exit 0

%post
%systemd_post themisdb.service

%preun
%systemd_preun themisdb.service

%postun
%systemd_postun_with_restart themisdb.service

%changelog
* Mon Dec 02 2025 ThemisDB Team <themisdb@example.com> - 1.0.0-1
- Initial release of ThemisDB v1.0.0
EOF

# Build RPM
rpmbuild -bb ~/rpmbuild/SPECS/themisdb-binary.spec

# Copy to release directory
cp ~/rpmbuild/RPMS/x86_64/themisdb-1.0.0-1.*.x86_64.rpm /mnt/c/VCC/themis/release/themisdb-1.0.0-1.x86_64.rpm

echo ""
echo "=== RPM Package built successfully ==="
rpm -qip ~/rpmbuild/RPMS/x86_64/themisdb-1.0.0-1.*.x86_64.rpm
