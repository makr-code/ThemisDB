# ThemisDB 1.0.0 Release Package Structure

## Verzeichnisstruktur

```
themisdb-1.0.0/
├── bin/
│   ├── themis_server              # Haupt-Binary (Linux)
│   └── themis_server.exe          # Haupt-Binary (Windows)
│
├── lib/                           # Shared Libraries (Linux/QNAP)
│   ├── librocksdb.so.8.9.1
│   ├── libspdlog.so.1.13
│   ├── libopenssl.so.3
│   ├── libarrow.so.15.0.0
│   ├── libboost_*.so.1.84.0
│   └── ... (alle vcpkg dependencies)
│
├── bin/ (Windows zusätzlich)
│   ├── themis_server.exe
│   ├── rocksdb.dll
│   ├── spdlog.dll
│   ├── libcrypto-3-x64.dll
│   ├── arrow.dll
│   └── ... (alle DLL dependencies)
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
│   ├── updates.yaml               # Update Server Config
│   ├── processors/                # Content Processor Configurations
│   │   ├── pdf.yaml               # PDF extraction settings
│   │   ├── office.yaml            # Office documents (DOCX, XLSX, PPTX)
│   │   ├── image.yaml             # Image processing (EXIF, OCR)
│   │   ├── video.yaml             # Video metadata extraction
│   │   ├── audio.yaml             # Audio processing & transcription
│   │   ├── geo.yaml               # Geospatial data (GeoJSON, Shapefile)
│   │   ├── cad.yaml               # CAD files (STEP, IGES, STL)
│   │   ├── text.yaml              # Text analysis & NLP
│   │   └── binary.yaml            # Binary file analysis
│   └── schemas/                   # JSON Schemas for API validation
│       ├── aql_request.json       # AQL query request schema
│       ├── content_import.json    # Content import schema
│       ├── pki_sign.json          # PKI signature schema
│       └── pki_verify.json        # PKI verification schema
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
├── openapi/                       # OpenAPI Specification
│   └── openapi.yaml               # REST API specification (OpenAPI 3.0)
│
├── clients/                       # Client Libraries (SDK)
│   ├── README.md                  # Client library overview
│   ├── python/                    # Python client library
│   ├── javascript/                # JavaScript/Node.js client
│   ├── java/                      # Java client library
│   ├── go/                        # Go client library
│   ├── csharp/                    # C# client library
│   ├── rust/                      # Rust client library
│   └── swift/                     # Swift client library
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
- `/usr/local/lib/themisdb/*.so*` (shared libraries)
- `/etc/themisdb/config.json`
- `/etc/themisdb/*.yaml`
- `/lib/systemd/system/themisdb.service`
- `/usr/share/doc/themisdb/*`
- `/var/lib/themisdb/` (data directory)
- `/var/log/themisdb/` (log directory)

**LD_LIBRARY_PATH:** `/usr/local/lib/themisdb`

### 2. RPM Package (RHEL/Fedora/CentOS)
```
themisdb-1.0.0-1.x86_64.rpm
themisdb-1.0.0-1.aarch64.rpm
```

**Installierte Dateien:**
- `/usr/bin/themis_server`
- `/usr/lib64/themisdb/*.so*` (shared libraries)
- `/etc/themisdb/config.yaml`
- `/etc/themisdb/*.yaml`
- `/usr/lib/systemd/system/themisdb.service`
- `/usr/share/doc/themisdb/*`
- `/var/lib/themisdb/` (data directory)

**Library Config:** `/etc/ld.so.conf.d/themisdb.conf`

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
- Shared libraries in `/usr/local/lib/themisdb`
- Data Volume `/data`
- Port 18765 (QNAP) / 8765 (Standard)
- `LD_LIBRARY_PATH` automatisch gesetzt

## Shared Library Management

### Linux Packages (DEB/RPM)
Libraries werden nach `/usr/local/lib/themisdb` oder `/usr/lib64/themisdb` installiert und via `ldconfig` registriert.

### Portable Archives
Libraries im `lib/` Verzeichnis. Installation script setzt `LD_LIBRARY_PATH` oder kopiert nach `/usr/local/lib`.

### Windows
DLLs im `bin/` Verzeichnis neben der EXE (Windows sucht DLLs automatisch im Binary-Verzeichnis).

### Docker
Libraries in `/usr/local/lib/themisdb`, `LD_LIBRARY_PATH` in ENV gesetzt.

## Benötigte Quelldateien

### Aus Repository Root
- `LICENSE`
- `README.md`
- `CHANGELOG.md`
- `SECURITY.md`
- `CONTRIBUTING.md`

### Aus openapi/
- `openapi.yaml` - REST API Spezifikation

### Aus clients/
- Komplettes `clients/` Verzeichnis mit allen SDK-Implementierungen
- Python, JavaScript, Java, Go, C#, Rust, Swift
- Ermöglicht Client-Entwicklung ohne separaten Download

### Aus config/
- Alle `.json` und `.yaml` Dateien im Hauptverzeichnis
- **Unterverzeichnisse (WICHTIG für Runtime):**
  - `processors/*.yaml` - 9 Content Processor Configs (pdf, office, image, video, audio, geo, cad, text, binary)
  - `schemas/*.json` - 4 JSON Schema Definitionen (aql_request, content_import, pki_sign, pki_verify)

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

## Minimal Runtime Requirements

**Für eine lauffähige und grundkonfigurierte ThemisDB werden benötigt:**

### Core Binaries & Libraries
- `bin/themis_server` (oder `.exe`)
- `lib/*.so*` oder `bin/*.dll` (alle vcpkg dependencies)

### Configuration (Minimum)
- `config/config.json` - Hauptkonfiguration
  - Muss enthalten: `storage.rocksdb_path`, `server.host`, `server.port`
  - Optional: `features`, `tracing`, `vector_index`, `sse`

### Empfohlene Runtime-Dateien
- `config/policies.json` - Security Policies (sonst Default-Allow!)
- `config/pii_patterns.yaml` - PII-Erkennung (embedded fallback vorhanden)
- `config/processors/*.yaml` - Content Processing (9 Prozessoren)
- `config/schemas/*.json` - API Schema Validation (4 Schemas)
- `openapi/openapi.yaml` - REST API Dokumentation

### Automatische Config-Suche
Der Server sucht Konfiguration in folgender Reihenfolge:
1. `--config <path>` CLI-Parameter
2. `./config.yaml`, `./config.yml`, `./config.json`
3. `./config/config.yaml`, `./config/config.yml`, `./config/config.json`
4. `/etc/vccdb/config.yaml`, `/etc/vccdb/config.yml`, `/etc/vccdb/config.json`

### Feature-Specific Requirements

**Content Processing:**
- `config/content/processors.yaml` - Globale Processor-Settings
- `config/processors/pdf.yaml` - PDF-Extraktion
- `config/processors/office.yaml` - Office-Dokumente
- `config/processors/image.yaml` - Bildanalyse, OCR
- `config/processors/video.yaml` - Video-Metadaten
- `config/processors/audio.yaml` - Audio-Prozessing
- `config/processors/geo.yaml` - Geospatial-Daten
- `config/processors/cad.yaml` - CAD-Dateien (optional)
- `config/processors/text.yaml` - Text-Analyse
- `config/processors/binary.yaml` - Binary-Analyse

**API Validation:**
- `config/schemas/aql_request.json` - AQL Query Validation
- `config/schemas/content_import.json` - Content Import Schema
- `config/schemas/pki_sign.json` - PKI Signatur-Schema
- `config/schemas/pki_verify.json` - PKI Verifikations-Schema

**MIME Detection:**
- `config/mime_types.yaml` - MIME Type Detection Rules (Fallback: embedded)

**Security & Compliance:**
- `config/policies.json` - Access Control (WARNUNG: ohne diese Default-Allow!)
- `config/pii_patterns.yaml` - PII Detection Patterns
- `config/governance.yaml` - Data Governance Policies
- `config/retention_policies.yaml` - Retention Rules

**Client Development:**
- `clients/` - SDK Libraries für alle unterstützten Sprachen
- `openapi/openapi.yaml` - API-Spezifikation für Code-Generierung

### Fallback-Verhalten
- PII Patterns: Embedded default patterns wenn YAML fehlt
- Policies: **WARNUNG** - Default-Allow ohne `policies.json`
- MIME Types: Embedded basic rules
- Processors: Feature deaktiviert wenn Configs fehlen

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
