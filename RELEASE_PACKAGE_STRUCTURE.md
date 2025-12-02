# ThemisDB 1.0.0 Release Package Structure

## Verzeichnisstruktur

```
themisdb-1.0.0/
├── bin/
│   ├── themis_server              # Haupt-Binary (Linux)
│   └── themis_server.exe          # Haupt-Binary (Windows)
│
├── config/                        # Konfigurationsdateien
│   ├── config.json                # Standard-Config
│   ├── config.qnap.json           # QNAP-optimiert
│   ├── config.rpi3.json           # Raspberry Pi 3
│   ├── config.rpi4.json           # Raspberry Pi 4  
│   ├── config.rpi5.json           # Raspberry Pi 5
│   ├── acceleration.yaml          # GPU/SIMD-Config
│   ├── cep_rules.yaml             # CEP Streaming Rules
│   ├── content_processors.yaml    # Content Processing
│   ├── governance.yaml            # Data Governance
│   ├── mime_types.yaml            # MIME Type Detection
│   ├── pii_patterns.yaml          # PII Pattern Rules
│   ├── policies.json              # Access Control Policies
│   ├── prometheus-arm.yml         # Prometheus Metrics (ARM)
│   ├── retention_policies.yaml    # Data Retention
│   ├── storage_redundancy.yaml    # Storage Redundancy
│   └── updates.yaml               # Update Server Config
│
├── docs/                          # Dokumentation
│   ├── README.md                  # Schnellstart
│   ├── INSTALLATION.md            # Installations-Guide
│   ├── CONFIGURATION.md           # Config-Referenz
│   ├── API_REFERENCE.md           # API Dokumentation
│   ├── AQL_REFERENCE.md           # AQL Query Language
│   ├── DEPLOYMENT.md              # Deployment Guide
│   ├── SECURITY.md                # Security Best Practices
│   ├── PERFORMANCE_TUNING.md      # Performance Tuning
│   ├── BACKUP_RECOVERY.md         # Backup & Recovery
│   ├── TROUBLESHOOTING.md         # Troubleshooting Guide
│   └── CHANGELOG.md               # Version History
│
├── examples/                      # Code-Beispiele
│   ├── python/                    # Python Client Examples
│   │   ├── basic_crud.py
│   │   ├── vector_search.py
│   │   ├── graph_traversal.py
│   │   └── requirements.txt
│   ├── javascript/                # JavaScript Client Examples
│   │   ├── basic_crud.js
│   │   ├── vector_search.js
│   │   └── package.json
│   ├── curl/                      # cURL Examples
│   │   ├── crud_operations.sh
│   │   ├── vector_queries.sh
│   │   └── aql_queries.sh
│   └── docker/                    # Docker Examples
│       ├── docker-compose.yml
│       ├── docker-compose.qnap.yml
│       └── README.md
│
├── tools/                         # Admin Tools
│   ├── plugin_signer/             # Plugin Signing Tool
│   │   ├── sign_plugin.py
│   │   └── README.md
│   ├── backup/                    # Backup Scripts
│   │   ├── backup_rocksdb.sh
│   │   └── restore_rocksdb.sh
│   ├── migration/                 # Migration Tools
│   │   ├── postgres_import.py
│   │   └── README.md
│   └── monitoring/                # Monitoring Setup
│       ├── prometheus.yml
│       ├── grafana_dashboard.json
│       └── README.md
│
├── scripts/                       # Installation Scripts
│   ├── install.sh                 # Linux Installation
│   ├── install.ps1                # Windows Installation
│   ├── setup_systemd.sh           # Systemd Service Setup
│   └── uninstall.sh               # Uninstallation
│
├── systemd/                       # Systemd Integration
│   └── themisdb.service           # Service Unit File
│
├── LICENSE                        # MIT License
├── README.md                      # Haupt-README
├── CHANGELOG.md                   # Changelog
├── SECURITY.md                    # Security Policy
└── SHA256SUMS                     # Checksums aller Dateien
```

## Package-Varianten

### 1. DEB Package (Debian/Ubuntu)
```
themisdb_1.0.0_amd64.deb
themisdb_1.0.0_arm64.deb
themisdb_1.0.0_armhf.deb
```

**Installierte Dateien:**
- `/usr/local/bin/themis_server`
- `/etc/themisdb/config.json`
- `/etc/themisdb/*.yaml`
- `/lib/systemd/system/themisdb.service`
- `/usr/share/doc/themisdb/*`
- `/var/lib/themisdb/` (data directory)
- `/var/log/themisdb/` (log directory)

### 2. RPM Package (RHEL/Fedora/CentOS)
```
themisdb-1.0.0-1.x86_64.rpm
themisdb-1.0.0-1.aarch64.rpm
```

**Installierte Dateien:**
- `/usr/bin/themis_server`
- `/etc/themisdb/config.yaml`
- `/etc/themisdb/*.yaml`
- `/usr/lib/systemd/system/themisdb.service`
- `/usr/share/doc/themisdb/*`
- `/var/lib/themisdb/` (data directory)

### 3. Portable Archives
```
themisdb-1.0.0-linux-x64.tar.gz
themisdb-1.0.0-linux-arm64.tar.gz
themisdb-1.0.0-windows-x64.zip
themisdb-1.0.0-qnap-x64.tar.gz
```

**Enthält:**
- Komplette Ordnerstruktur wie oben
- Keine System-Integration
- Manuelle Installation erforderlich

### 4. Docker Images
```
themisdb/themisdb:1.0.0
themisdb/themisdb:1.0.0-qnap
themisdb/themisdb:latest
themisdb/themisdb:qnap
```

**Pre-configured:**
- Config in `/etc/themis/config.json`
- Data Volume `/data`
- Port 18765 (QNAP) / 8765 (Standard)

## Benötigte Quelldateien

### Aus Repository Root
- `LICENSE`
- `README.md`
- `CHANGELOG.md`
- `SECURITY.md`
- `CONTRIBUTING.md`

### Aus config/
- Alle `.json` und `.yaml` Dateien
- Processors und Schemas Unterverzeichnisse

### Aus docs/
- Alle Markdown-Dateien
- PDF: `ThemisDB-Documentation.pdf` (generiert)

### Aus examples/
- Python, JavaScript, cURL Beispiele
- Docker-Compose Beispiele

### Aus tools/
- Plugin Signer
- Backup/Restore Scripts
- Monitoring Configs

### Aus scripts/
- Installation Scripts
- Systemd Setup

### Neu zu erstellen
- `INSTALLATION.md` - Detaillierte Installation
- `CONFIGURATION.md` - Config-Referenz
- `DEPLOYMENT.md` - Deployment Guide
- `install.sh` - Linux Installer
- `install.ps1` - Windows Installer
- `setup_systemd.sh` - Systemd Setup

## Checksums

Für jedes Release-Artefakt:
```
SHA256SUMS - Alle Package-Dateien
SHA256SUMS.txt - Binaries im Archiv
```

## Distribution Channels

1. **GitHub Releases**: Alle Packages + Checksums
2. **Docker Hub**: Container Images
3. **APT Repository**: DEB Packages
4. **YUM Repository**: RPM Packages
5. **QNAP App Center**: QNAP-spezifische Packages

## Versions-Schema

- **Stable**: 1.0.0, 1.0.1, 1.1.0
- **Docker Tags**: 1.0.0, 1.0, latest
- **QNAP Tags**: 1.0.0-qnap, qnap
