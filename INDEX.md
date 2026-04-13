# ThemisDB - Projektstruktur & Index

Dieser Index bietet eine Übersicht über die Ordnerstruktur von ThemisDB und erklärt den Zweck jedes Verzeichnisses.

> **Version:** 1.8.0  
> **Letzte Aktualisierung:** 2026-03-24

---

## 📚 Hauptordner

### 🔧 Build & Configuration

- **`.devcontainer/`** - DevContainer-Konfiguration für VS Code
- **`.github/`** - GitHub Actions Workflows, Issue-Templates, CI/CD-Konfiguration
  - [Workflows README](.github/workflows/README.md)
- **`.tools/`** - Build-Tools und Hilfsskripte
- **`.vscode/`** - VS Code-Konfiguration für Entwickler
  - [VS Code README](.vscode/README.md)
- **`.vscode.example/`** - Beispiel-VS Code-Einstellungen
  - [VS Code Example README](.vscode.example/README.md)
- **`cmake/`** - CMake-Module und Build-Skripte
- **`vcpkg/`** - vcpkg Package Manager (Submodule)

### 📖 Dokumentation

- **`docs/`** - Gesamte Projektdokumentation
  - **`docs/de/`** - Deutsche Dokumentation
  - **`docs/en/`** - Englische Dokumentation
  - **`docs/api/`** - API-Referenzdokumentation
  - **`docs/architecture/`** - Architektur-Dokumente
  - **`docs/audit-framework/`** - Audit & Compliance Framework
  - **`docs/audit-reports/`** - Audit-Berichte nach Version
  - **`docs/certification/`** - Zertifizierungsprogramme
  - **`docs/ci-cd/`** - CI/CD-Dokumentation
  - **`docs/de/llm/`** - LLM/LoRA-Dokumentation (Deutsch)
    - [LoRA README](docs/de/llm/LORA_README.md)
  - **`docs/en/llm/`** - LLM/LoRA-Dokumentation (Englisch)
    - [LoRA README](docs/en/llm/LORA_README.md)
  - **`docs/knowledge-base/`** - Wissensdatenbank
  - **`docs/operations/`** - Operations-Handbücher
  - **`docs/research/`** - Forschungsdokumente
  - **`docs/tutorials/`** - Tutorials
  - **`docs/use-cases/`** - Anwendungsfälle

- **`compendium/`** - ThemisDB Compendium (technisches Handbuch)
  - [Export README](compendium/EXPORT_README.md)
  - [PDF Generation README](compendium/PDF_GENERATION_README.md)

### 💻 Source Code

- **`src/`** - Hauptquellcode des ThemisDB-Systems
  - [Module Documentation im README](README.md#-module-documentation)
  - **Core Module:**
    - `src/acceleration/` - Hardware-Beschleunigung (GPU, SIMD)
    - `src/analytics/` - OLAP, Process Mining, CEP
    - `src/api/` - REST API-Layer
    - `src/aql/` - AQL Query Language
    - `src/auth/` - Authentifizierung (JWT, Kerberos, MFA)
    - `src/base/` - Basis-Utilities
    - `src/cache/` - Caching-Layer
    - `src/cdc/` - Change Data Capture
    - `src/chimera/` - CHIMERA Benchmark Adapter
    - `src/compendium/` - Compendium-Integration
    - `src/content/` - Content Management
    - `src/core/` - Core Framework (DI, Logging, Metrics)
    - `src/exporters/` - Datenexport
    - `src/geo/` - Geospatial Features
    - `src/governance/` - Data Governance
    - `src/gpu/` - GPU-Utilities
    - `src/graph/` - Graph-Datenbank-Engine
      - [Advanced Features README](src/graph/ADVANCED_FEATURES_README.md)
    - `src/importers/` - Datenimport
    - `src/index/` - Indexierungssysteme (HNSW, B-Tree, Spatial)
    - `src/llm/` - LLM-Integration (llama.cpp)
      - [GGUF Loader README](src/llm/gguf_loader_README.md)
    - `src/metadata/` - Schema Introspection
    - `src/network/` - Netzwerk-Layer (TLS, Zero-Copy I/O)
    - `src/observability/` - Monitoring (Prometheus, Tracing)
    - `src/performance/` - Performance-Optimierung
    - `src/plugins/` - Plugin-System
    - `src/query/` - Query Processing (Parser, Optimizer, Executor)
    - `src/rag/` - RAG (Retrieval-Augmented Generation)
    - `src/replication/` - Replikation (Raft, Multi-Master)
    - `src/scheduler/` - Job Scheduling
    - `src/search/` - Full-Text Search (BM25)
    - `src/security/` - Security (Encryption, RBAC, Compliance)
    - `src/server/` - Server (HTTP, gRPC, WebSocket, MQTT)
    - `src/sharding/` - Horizontal Partitioning
    - `src/storage/` - Storage Layer (RocksDB, Blob Backends)
    - `src/temporal/` - Time-Travel Queries
    - `src/themis/` - Core Framework & Module Loading
    - `src/timeseries/` - Time-Series Engine
    - `src/transaction/` - Transaction Management (MVCC, SAGA)
    - `src/updates/` - Hot-Reload & Schema Migration
    - `src/utils/` - Utility Functions
    - `src/voice/` - Voice Assistant (STT, TTS, NLU)

- **`include/`** - Public Header-Dateien

### 🧪 Tests & Benchmarks

- **`tests/`** - Unit- und Integrationstests (732 C++ Test-Dateien)
- **`benchmarks/`** - Performance-Benchmarks (122 C++ Benchmark-Dateien)
  - [CHIMERA Suite README](external/chimera/CHIMERA_README.md)
  - [Enterprise Suite README](benchmarks/ENTERPRISE_SUITE_README.md)
  - [RAG Ethics Benchmarks README](benchmarks/BENCH_RAG_ETHICS_README.md)
  - [TPC-H README](benchmarks/tpc/tpc_h_README.md)
  - **`external/chimera/`** - CHIMERA Benchmark Framework (inkl. Python-Analyse)
  - **`benchmarks/tpc/`** - TPC-C, TPC-H Benchmarks
- **`clients/go/`** - Go Client SDK (4 Test-Dateien + 2 Benchmark-Dateien)
- **`fuzz/`** - Fuzzing-Tests
- **[docs/TEST_AND_BENCHMARK_INVENTORY.md](docs/TEST_AND_BENCHMARK_INVENTORY.md)** - Vollständige Test & Benchmark Inventur

### 📦 Clients & SDKs

- **`clients/`** - Client-Bibliotheken (C++, Python, JavaScript, etc.)
- **`sdks/`** - SDKs für verschiedene Sprachen
- **`adapters/`** - Database Adapter

### 🚀 Deployment & Operations

- **`deploy/`** - Deployment-Skripte und Konfigurationen
- **`docker/`** - Docker-Konfigurationen
- **`helm/`** - Kubernetes Helm Charts
- **`debian/`** - Debian-Paketierung
- **`packaging/`** - Plattformspezifische Pakete
- **`scripts/`** - Build- und Deployment-Skripte

### 🔧 Configuration & Data

- **`config/`** - Konfigurationsdateien
- **`data/`** - Beispiel-Daten und Fixtures
- **`certs/`** - SSL/TLS-Zertifikate für Tests

### 📊 Monitoring & Observability

- **`grafana/`** - Grafana-Dashboards
- **`prometheus/`** - Prometheus-Konfiguration

### 🔬 Examples & Projects

- **`examples/`** - Code-Beispiele
- **`projects/`** - Beispielprojekte
- **`aql/`** - AQL Query-Beispiele
- **`archive/`** - Archivierte Dateien

### 🌐 Integrations

- **`openapi/`** - OpenAPI/Swagger-Spezifikationen
- **`proto/`** - Protocol Buffers-Definitionen
- **`wordpress-plugin/`** - WordPress-Integration

### 🗄️ External Dependencies

- **`llama.cpp/`** - llama.cpp Submodule (LLM-Inferenz)
- **`ffmpeg/`** - FFmpeg Submodule (Multimedia)
- **`ports/`** - Custom vcpkg Ports
- **`ports-overlays/`** - vcpkg Port Overlays

### 📝 Metadata & Reports

- **`artifacts/`** - Build-Artefakte
- **`releases/`** - Release-Informationen
- **`symbols/`** - Debug-Symbole
- **`llm_cache/`** - LLM Model Cache
- **`security/`** - Security-Audits und Reports

---

## 🔗 Wichtige Dokumente

### Getting Started
- [README.md](README.md) - Hauptdokumentation
- [QUICKSTART.md](QUICKSTART.md) - 5-Minuten-Setup
- [SETUP.md](SETUP.md) - Ausführliche Setup-Anleitung

### Build & Development
- [CONTRIBUTING.md](CONTRIBUTING.md) - Beitragsrichtlinien
- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Architektur
- [CMAKE_ONLY_BUILD_SYSTEM.md](CMAKE_ONLY_BUILD_SYSTEM.md) - CMake Build-System
- [BUILD_DIRECTORY_NAMING.md](BUILD_DIRECTORY_NAMING.md) - Build-Verzeichnis-Konventionen

### Release & Changelog
- [CHANGELOG.md](CHANGELOG.md) - Änderungsprotokoll
- [VERSIONING.md](VERSIONING.md) - Versionierungsrichtlinie (SemVer, Release-Typen, EOL)
- [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) - Branch-Modell, Edition-Matrix, CI/CD
- [SOP.md](SOP.md) - Standard Operating Procedures (Release, Hotfix, Incident)
- [MIGRATION_POWERSHELL_TO_CMAKE.md](MIGRATION_POWERSHELL_TO_CMAKE.md) - Build-Migration

### Compliance & Security
- [SECURITY.md](SECURITY.md) - Security Policy
- [GOVERNANCE.md](GOVERNANCE.md) - Projekt-Governance (Rollen, Entscheidungen)
- [MAINTAINERS.md](MAINTAINERS.md) - Maintainer-Liste und Verantwortlichkeiten
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) - Verhaltenskodex
- [LICENSE](LICENSE) - MIT-Lizenz

### Operations & Deployment
- [DOCKER_CACHE_GUIDE.md](DOCKER_CACHE_GUIDE.md) - Docker Cache-Strategie
- [VCPKG_DOCKER_CACHE_STRATEGY.md](VCPKG_DOCKER_CACHE_STRATEGY.md) - vcpkg Docker Cache
- [VCPKG_MULTI_PLATFORM_PACKAGES.md](VCPKG_MULTI_PLATFORM_PACKAGES.md) - Multi-Plattform-Pakete

---

## 📊 Statistiken

- **Zeilen Code:** ![Lines of Code](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/makr-code/ThemisDB/develop/.github/badges/lines-of-code.json)
- **Dateianzahl:** ![File Count](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/makr-code/ThemisDB/develop/.github/badges/file-count.json)
- **Module:** 39 Module mit 139 Dokumentationsdateien
- **Sprachen:** C++ (45.5%), HTML (35.3%), C# (7.4%), Python (4.5%)

---

## 🔎 Weitere Ressourcen

- **Website:** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **Docker Hub:** [themisdb/themisdb](https://hub.docker.com/r/themisdb/themisdb)
- **GitHub:** [makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)
