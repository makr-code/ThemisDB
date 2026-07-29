Name:           themisdb
Version:        1.0.0
Release:        1%{?dist}
Summary:        Multi-model database system with ACID transactions

License:        MIT
URL:            https://github.com/makr-code/ThemisDB
Source0:        https://github.com/makr-code/ThemisDB/archive/v%{version}.tar.gz

BuildRequires:  gcc-c++ >= 11
BuildRequires:  cmake >= 3.20
BuildRequires:  ninja-build
BuildRequires:  git
BuildRequires:  pkgconfig
BuildRequires:  openssl-devel
BuildRequires:  rocksdb-devel
BuildRequires:  tbb-devel
BuildRequires:  arrow-devel
BuildRequires:  boost-devel
BuildRequires:  spdlog-devel
BuildRequires:  json-devel
BuildRequires:  libcurl-devel
BuildRequires:  yaml-cpp-devel
BuildRequires:  libzstd-devel
BuildRequires:  systemd-rpm-macros

Requires:       openssl-libs
Requires:       rocksdb
Requires:       tbb
Requires:       arrow-libs
Requires:       boost-system
Requires:       spdlog
Requires:       libcurl
Requires:       yaml-cpp
Requires:       libzstd

%description
ThemisDB is a high-performance multi-model database system that supports:
 - Relational data with secondary indexes
 - Graph traversals (BFS, Dijkstra, A*)
 - Vector search with HNSW index
 - Time-series data with Gorilla compression
 - Document storage
 - Full ACID transactions with MVCC
 - Advanced Query Language (AQL)
 - OpenTelemetry tracing and Prometheus metrics

%package devel
Summary:        Development files for ThemisDB
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
Development headers and libraries for building applications that
link against ThemisDB.

%prep
%autosetup -n ThemisDB-%{version}

%build
%cmake \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DTHEMIS_BUILD_TESTS=OFF \
    -DTHEMIS_BUILD_BENCHMARKS=OFF \
    -DTHEMIS_ENABLE_GPU=OFF \
    -DTHEMIS_STRICT_BUILD=OFF \
    -DBUILD_SHARED_LIBS=OFF
%cmake_build

%install
%cmake_install

# Install systemd service file
install -D -m 644 %{_builddir}/ThemisDB-%{version}/debian/themisdb.service \
    %{buildroot}%{_unitdir}/themisdb.service

# Install binary
install -D -m 755 %{_builddir}/ThemisDB-%{version}/build/themis_server \
    %{buildroot}%{_bindir}/themis_server

# Install shared libraries from vcpkg
if [ -d %{_builddir}/ThemisDB-%{version}/vcpkg_installed/x64-linux/lib ]; then
    mkdir -p %{buildroot}%{_libdir}/themisdb
    find %{_builddir}/ThemisDB-%{version}/vcpkg_installed/x64-linux/lib \
        -name "*.so*" -exec cp -v {} %{buildroot}%{_libdir}/themisdb/ \;
fi

# Install configuration files
install -D -m 640 %{_builddir}/ThemisDB-%{version}/config/config.json \
    %{buildroot}%{_sysconfdir}/themisdb/config.json
install -m 640 %{_builddir}/ThemisDB-%{version}/config/*.yaml \
    %{buildroot}%{_sysconfdir}/themisdb/
install -m 640 %{_builddir}/ThemisDB-%{version}/config/policies.json \
    %{buildroot}%{_sysconfdir}/themisdb/

# Install processors and schemas if they exist
if [ -d %{_builddir}/ThemisDB-%{version}/config/processors ]; then
    mkdir -p %{buildroot}%{_sysconfdir}/themisdb/processors
    cp -r %{_builddir}/ThemisDB-%{version}/config/processors/* \
        %{buildroot}%{_sysconfdir}/themisdb/processors/
fi
if [ -d %{_builddir}/ThemisDB-%{version}/config/schemas ]; then
    mkdir -p %{buildroot}%{_sysconfdir}/themisdb/schemas
    cp -r %{_builddir}/ThemisDB-%{version}/config/schemas/* \
        %{buildroot}%{_sysconfdir}/themisdb/schemas/
fi

# Install documentation
install -D -m 644 %{_builddir}/ThemisDB-%{version}/README.md \
    %{buildroot}%{_docdir}/themisdb/README.md
install -m 644 %{_builddir}/ThemisDB-%{version}/CHANGELOG.md \
    %{buildroot}%{_docdir}/themisdb/CHANGELOG.md
if [ -f %{_builddir}/ThemisDB-%{version}/docs/ThemisDB-Documentation.pdf ]; then
    install -m 644 %{_builddir}/ThemisDB-%{version}/docs/ThemisDB-Documentation.pdf \
        %{buildroot}%{_docdir}/themisdb/
fi

# Install OpenAPI specification
if [ -d %{_builddir}/ThemisDB-%{version}/openapi ]; then
    mkdir -p %{buildroot}%{_datadir}/themisdb/openapi
    cp -r %{_builddir}/ThemisDB-%{version}/openapi/* \
        %{buildroot}%{_datadir}/themisdb/openapi/
fi

# Install client libraries (SDKs)
if [ -d %{_builddir}/ThemisDB-%{version}/clients ]; then
    mkdir -p %{buildroot}%{_datadir}/themisdb/clients
    cp -r %{_builddir}/ThemisDB-%{version}/clients/* \
        %{buildroot}%{_datadir}/themisdb/clients/
fi

# Install examples
if [ -d %{_builddir}/ThemisDB-%{version}/examples ]; then
    mkdir -p %{buildroot}%{_docdir}/themisdb/examples
    cp -r %{_builddir}/ThemisDB-%{version}/examples/* \
        %{buildroot}%{_docdir}/themisdb/examples/
fi

# Install tools
if [ -d %{_builddir}/ThemisDB-%{version}/plugins/private/themisdb_plugin_signer ]; then
    mkdir -p %{buildroot}%{_datadir}/themisdb/tools
    cp -r %{_builddir}/ThemisDB-%{version}/plugins/private/themisdb_plugin_signer \
        %{buildroot}%{_datadir}/themisdb/tools/
    install -m 755 %{_builddir}/ThemisDB-%{version}/tools/sign_*.py \
        %{buildroot}%{_datadir}/themisdb/tools/ || true
fi

# Create data directory
install -d -m 750 %{buildroot}%{_sharedstatedir}/themisdb

%pre
getent group themisdb >/dev/null || groupadd -r themisdb
getent passwd themisdb >/dev/null || \
    useradd -r -g themisdb -d %{_sharedstatedir}/themisdb -s /sbin/nologin \
    -c "ThemisDB Database Server" themisdb
exit 0

%post
%systemd_post themisdb.service
# Set ownership of data directory
chown -R themisdb:themisdb %{_sharedstatedir}/themisdb
chmod 750 %{_sharedstatedir}/themisdb
# Set ownership of config directory
chown -R root:themisdb %{_sysconfdir}/themisdb
chmod 750 %{_sysconfdir}/themisdb
chmod 640 %{_sysconfdir}/themisdb/config.json
# Configure library path
echo "/usr/lib64/themisdb" > /etc/ld.so.conf.d/themisdb.conf
ldconfig

%preun
%systemd_preun themisdb.service

%postun
%systemd_postun_with_restart themisdb.service
if [ $1 -eq 0 ] ; then
    # Package removal, not upgrade
    getent passwd themisdb >/dev/null && userdel themisdb
    getent group themisdb >/dev/null && groupdel themisdb
    rm -rf %{_sharedstatedir}/themisdb
    rm -f /etc/ld.so.conf.d/themisdb.conf
    ldconfig
fi

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/themis_server
%{_libdir}/themisdb/*.so*
%{_unitdir}/themisdb.service
%dir %attr(750,root,themisdb) %{_sysconfdir}/themisdb
%config(noreplace) %attr(640,root,themisdb) %{_sysconfdir}/themisdb/config.json
%config(noreplace) %attr(640,root,themisdb) %{_sysconfdir}/themisdb/*.yaml
%config(noreplace) %attr(640,root,themisdb) %{_sysconfdir}/themisdb/policies.json
%config(noreplace) %attr(640,root,themisdb) %{_sysconfdir}/themisdb/processors/*
%config(noreplace) %attr(640,root,themisdb) %{_sysconfdir}/themisdb/schemas/*
%dir %attr(750,themisdb,themisdb) %{_sharedstatedir}/themisdb
%{_docdir}/themisdb/*
%{_datadir}/themisdb/tools/*
%{_datadir}/themisdb/openapi/*
%{_datadir}/themisdb/clients/*

%files devel
%{_includedir}/*
%{_libdir}/libthemis_core.a

%changelog
* Fri Nov 22 2024 ThemisDB Team <info@themisdb.org> - 1.0.0-1
- Initial RPM release
- Multi-model database with relational, graph, vector, and time-series support
- Full ACID transactions with MVCC
- Advanced Query Language (AQL) with graph traversals
- HNSW vector index with persistence
- Enterprise security features (TLS 1.3, RBAC, audit logging)
- OpenTelemetry tracing and Prometheus metrics
- Change Data Capture (CDC) support
