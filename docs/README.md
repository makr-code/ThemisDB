<div align="center">

# ThemisDB Documentation (Language Selector)

</div>

This root file is language-neutral. Bitte wähle eine Sprachversion:

- 🇩🇪 Deutsch (Primär, autoritativ): [de/README.md](de/README.md)
- 🇬🇧 English: [en/README.md](en/README.md)

> [!NOTE]
> Die deutschsprachigen Dokumente sind die authoritative Source. Übersetzungen können zeitlich nachhängen.
### Übersichtsdokumente
- **[Changelog](../CHANGELOG.md)** - Vollständige Versionshistorie (v1.8.0-rc1, v1.9.0, v1.6.0, v1.5.0, v1.4.0, …)
- **[Roadmap v2.0](../roadmap.md)** - Aggregierte Roadmap über alle 46 Module
- **[Architecture Overview](architecture/ARCHITECTURE_OVERVIEW.md)** - Komplette Systemarchitektur mit Diagrammen
- **[🆕 Source Directory Guide](architecture/SOURCE_DIRECTORY_GUIDE.md)** - **NEU:** Comprehensive guide to all 35 src/ directories (100% coverage)
- **[Source Code Changes v1.0](development/SOURCE_CODE_CHANGES_v1.0.md)** - Detaillierte Quellcode-Dokumentation (191 Dateien, 26 Module)
- **[Features Liste](features/features_overview.md)** - Vollständige Feature-Übersicht mit Status
- **[🆕 Examples Index](EXAMPLES_INDEX.md)** - **NEU:** Vollständiger Index aller 37+ Beispiele
- **[🆕 Examples Quickstart](EXAMPLES_QUICKSTART.md)** - **NEU:** In 10 Minuten mit ThemisDB starten

---

## 🎯 Nach Zielgruppe

### Für neue Nutzer & Lernende
- **[🚀 Examples Quickstart Guide](EXAMPLES_QUICKSTART.md)** - **START HERE:** In 10 Minuten mit ThemisDB beginnen
- **[📚 Examples Index](EXAMPLES_INDEX.md)** - Vollständiger Katalog aller 37+ Beispiele
- **[🎓 Learning Paths](EXAMPLES_INDEX.md#-learning-paths)** - Geführte Lernpfade nach Rolle (Web Dev, Data Engineer, ML Engineer, etc.)
- **[Main Examples](../examples/)** - 22 vollständige Beispielanwendungen

### Für Stakeholder & Management
- **[Themis Sachstandsbericht 2025](de/reports/themis_sachstandsbericht_2025.md)** - Executive Summary, Status v1.0.1
- **[🆕 v1.1.0 Variant Strategy](de/reports/VARIANT_STRATEGY_v1.1.0.md)** - **Q1 2026:** Optimierungs-Strategie mit vLLM Co-Location (9-11 Wochen, 1 neue Lib)
- **[🆕 v1.2.0 Enterprise Features](de/reports/ENTERPRISE_FEATURES_STRATEGY.md)** - **Q2 2026:** vLLM AI (LoRA), Geo-Spatial (PostGIS), IoT/Timescale (12-16 Wochen, 3 neue Libs)
- ~~Projektkostenschätzung & Gesamtwert~~ - 🔒 Confidential (available to licensed customers only)
- **[Release Strategy Audit](../RELEASE_STRATEGY_AUDIT.md)** - SLSA Compliance, SBOM (8.5/10 Rating)
- **[Release & Benchmarking Summary](../RELEASE_AND_BENCHMARKING_SESSION_SUMMARY.md)** - v1.0.1 Session Report

### Für Entwickler
- **[Development Summary](de/development/DEVELOPMENT_SUMMARY.md)** - Entwicklungsstand v1.0.1
- **[🆕 External Libraries Analysis](de/reports/EXTERNAL_LIBRARIES_FEATURES_ANALYSIS.md)** - **NEU:** Feature-Gap-Analyse (RocksDB, TBB, CUDA, Arrow)
- **[🆕 Library Interactions](de/reports/LIBRARY_INTERACTIONS_AND_EXTENSIONS.md)** - **NEU:** Wechselwirkungen & zusätzliche Libraries
- **[Source Code Audit](de/development/SOURCE_CODE_AUDIT.md)** - Code-Analyse (132 Header, 124 Sources, 90.829 LOC)
- **🆕 Code Duplicate Audit Template** ([Template](https://github.com/makr-code/ThemisDB/blob/main/.github/ISSUE_TEMPLATE/code_duplicate_audit.yml)) - **NEU:** Systematisches Issue-Template für Code-Duplikat-Audits und unvollständige Implementierungen (CI-fähig)
- **[Documentation Index](de/DOCUMENTATION_INDEX.md)** - Vollständiger Dokumentations-Index mit Modul-Mapping
- **[Documentation Verification](reports/documentation_verification_report.md)** - Verifizierung Dokumentation ↔ Code

### Für DevOps & Operations
- **[Operations Runbook](guides/guides_operations_runbook.md)** - Tägliche Operationen
- **[Deployment Guide](deployment/README.md)** - Deployment-Strategien
- **[Build Strategy](guides/guides_build_strategy.md)** - Build-Toolchain
- **[Docker Guide](../README.docker.md)** - Container-Deployment

### Für Security & Compliance
- **[Compliance Dashboard](compliance/compliance_dashboard.md)** - Übersicht aller Compliance-Aktivitäten
- **[Security Audit Report](reports/SECURITY_AUDIT_REPORT.md)** - Durchgeführtes Security Audit
- **[Compliance Full Checklist](compliance/compliance_full_checklist.md)** - BSI C5, ISO 27001, DSGVO, eIDAS, SOC 2
- **[Security Policy](../SECURITY.md)** - Vulnerability Disclosure
- **[Incident Response Plan](security/security_incident_response.md)** - Notfallplan (BSI IT-Grundschutz & NIST CSF)
- **[SBOM Documentation](security/security_sbom.md)** - Software Bill of Materials (CycloneDX 1.4)
- **[DPIA](compliance/compliance_dpia.md)** - Datenschutz-Folgenabschätzung (DSGVO Art. 35)
- **[BCP/DRP](compliance/compliance_bcp_drp.md)** - Business Continuity (ISO 22301 & NIS2)

---

## 🏗️ Nach Architektur-Ebene

### Query & Analytics Layer
- **[AQL Documentation](de/aql/README.md)** - Advanced Query Language (Parser, Optimizer, 240K LOC)
- **[Query Module](de/query/README.md)** - Query Engine, Execution
- **[Analytics Module](de/observability/README.md)** - OLAP Engine (CUBE, ROLLUP), CEP, Process Mining (57K LOC)
- **[Search Documentation](de/search/README.md)** - Fulltext (BM25), Vector, Hybrid Search

### Storage & Index Layer
- **[Storage Module](de/storage/README.md)** - RocksDB Wrapper, LSM-Tree, MVCC (76K LOC)
- **[Index Module](de/search/README.md)** - Vector HNSW, Graph, Secondary, Spatial (400K LOC)
- **[Cache Module](de/storage/README.md)** - Semantic Cache, Result Cache
- **[Timeseries Module](timeseries/README.md)** - Time-Series Engine, Gorilla Compression, Continuous Aggregates ([German](de/timeseries/README.md))

### Distribution & Scaling Layer
- **[Sharding Module](de/sharding/README.md)** - VCC-URN Sharding, Auto-Rebalancing, Gossip (300K LOC)
- **[Replication Module](de/storage/README.md)** - Leader-Follower, Multi-Master CRDTs (12K LOC)
- **[Transaction Module](de/architecture/README.md)** - MVCC, SAGA Patterns (42K LOC)

### Acceleration Layer
- **[GPU Acceleration Plan](performance/GPU_ACCELERATION_PLAN.md)** - 10 GPU Backends (173K LOC)
  - CUDA, Vulkan, FAISS, DirectX, HIP, OpenCL, OneAPI, ZLUDA

### Content & Data Processing
- **[Content Module](de/content/README.md)** - 15 File Format Processors (256K LOC)
- **[CDC Module](de/features/README.md)** - Change Data Capture, Changefeed
- **[Geo Module](de/geo/README.md)** - Spatial Operations, Plugin System

### Server & API Layer
- **[Server Module](de/server/README.md)** - HTTP Server, 21 API Handlers (164K LOC)
- **[HTTP API Referenz](de/apis/HTTP_API_REFERENCE.md)** - **Vollständige HTTP Endpoint-Dokumentation** ⭐
- **[API Documentation](de/apis/README.md)** - REST API Übersicht
- **[Wire Protocol](architecture/wire-protocol.md)** - Binary protocol for client-server communication with TLS/mTLS
- **[LLM Module](de/llm/README.md)** - LLM Interaction Store, Prompt Manager

### Security & Governance Layer
- **[Security Module](de/security/README.md)** - Field Encryption, HSM/PKI, RBAC, Ranger (187K LOC)
- **[Governance Module](de/governance/README.md)** - Policy Engine, Data Classification
- **[Auth Module](de/auth/README.md)** - JWT Validation, Multi-Tenancy

---

## 🚀 Quick Start Guides

### Installation & Deployment
- **[Main README](../README.md)** - Projekt-Übersicht und Quick Start
- **[Deployment Guide](deployment/README.md)** - Deployment-Optionen
- **[Docker Guide](../README.docker.md)** - Container-Deployment
- **[QNAP Quickstart](../QNAP_QUICKSTART.md)** - ARM-Deployment

### Getting Started
- **[Architecture Overview](de/architecture/README.md)** - System-Architektur verstehen
- **[Features Overview](de/features/features_overview.md)** - Verfügbare Features
- **[AQL Tutorial](de/aql/README.md)** - Query Language lernen

---

## 📖 Referenz-Dokumentation

### Client SDKs
- **[SDK Audit](de/clients/clients_sdk_audit.md)** - Übersicht aller 7 SDKs
- **[Python SDK](de/clients/python_sdk_quickstart.md)** - Python Client
- **[JavaScript SDK](de/clients/javascript_sdk_quickstart.md)** - Node.js/Browser Client
- **[Rust SDK](de/clients/rust_sdk_quickstart.md)** - Rust Client
- **[Go SDK](de/clients/go_sdk_quickstart.md)** - Go Client
- **[Java SDK](de/clients/java_sdk_quickstart.md)** - Java Client
- **[C# SDK](de/clients/csharp_sdk_quickstart.md)** - .NET Client
- **[Swift SDK](de/clients/swift_sdk_quickstart.md)** - iOS/macOS Client

### Data Import/Export
- **[Connectors](de/connectors/README.md)** - Data Import/Export
  - **[JSONL LLM Exporter](de/connectors/JSONL_LLM_EXPORTER.md)** - LLM Training Data Export
  - **[PostgreSQL Importer](de/connectors/POSTGRES_IMPORTER.md)** - PostgreSQL Migration

### Plugin Development
- **[Plugins](plugins/README.md)** - Plugin System
- **[Plugin Security](plugins/PLUGIN_SECURITY.md)** - Security & Sandboxing
- **[Plugin Migration](plugins/PLUGIN_MIGRATION.md)** - Migration Guide
- **[WordPress Plugin Automatic Updates](plugins/WORDPRESS_PLUGIN_AUTOMATIC_UPDATES.md)** - Update-Architektur, Metadaten und Release-Konventionen
- **[WordPress Plugin Update Examples](plugins/WORDPRESS_PLUGIN_UPDATE_EXAMPLES.md)** - Praxisbeispiele fuer Admins, Entwickler und Betrieb

---

## 🔧 Administration & Operations

### Admin Tools
- **[Admin Tools](de/admin_tools/README.md)** - 7 WPF Administration Tools
- **[User Guide](de/admin_tools/user_guide.md)** - Benutzerhandbuch
- **[Admin Guide](de/admin_tools/admin_guide.md)** - Administrator-Handbuch
- **[Feature Matrix](de/admin_tools/feature_matrix.md)** - Tool-Übersicht

### Operations Guides
- **[Operations Runbook](de/guides/guides_operations_runbook.md)** - Tägliche Operationen
- **[TLS Setup](de/guides/tls_setup.md)** - TLS/mTLS Konfiguration
- **[Vault Integration](de/guides/vault.md)** - HashiCorp Vault Setup
- **[RBAC Setup](de/guides/rbac.md)** - Access Control Configuration
- **[Code Quality](de/guides/code_quality.md)** - Code Quality Tools
- **[WordPress Plugin Release Pipeline](ci-cd/WORDPRESS_PLUGIN_RELEASE_PIPELINE.md)** - CI/CD Workflow fuer plugin-spezifische Releases
- **[WordPress Plugin Operations Runbook](ci-cd/WORDPRESS_PLUGIN_OPERATIONS.md)** - Operativer Ablauf fuer Dry-Run, Pilot-Release und Batch-Rollout

### Performance & Monitoring
- **[Performance Tuning](de/performance/README.md)** - Performance-Optimierung
- **[Benchmarks](de/performance/benchmarks.md)** - Performance-Benchmarks
- **[Memory Tuning](de/performance/memory_tuning.md)** - Speicher-Optimierung
- **[Observability & Monitoring Overview](observability/README.md)** - **Central Hub:** Logging, Tracing, Metrics & Alerting
- **[Observability (DE)](de/observability/README.md)** - German Documentation: Monitoring & Metrics

---

## 📊 Reports & Status

### Development Reports
- **[Development Summary](de/development/DEVELOPMENT_SUMMARY.md)** - Aktueller Entwicklungsstand v1.0.1
- **[Audit Log](de/development/auditlog.md)** - Entwicklungs-Audit-Log
- **[Implementation Status](de/development/implementation_status.md)** - Implementierungsstatus
- **[Priorities](de/development/priorities.md)** - Entwicklungs-Prioritäten

### Status Reports
- **[Themis Sachstandsbericht](de/reports/themis_sachstandsbericht_2025.md)** - Haupt-Statusbericht v1.5
- **[Documentation Summary](de/reports/DOCUMENTATION_SUMMARY.md)** - Dokumentations-Übersicht
- **[Benchmark Audit](de/reports/BENCHMARK_AND_TEST_AUDIT.md)** - Test & Benchmark Status
- **[Security Audit](de/reports/SECURITY_AUDIT_REPORT.md)** - Security Audit Ergebnisse

### Roadmap & Planning
- **[Roadmap Overview](de/roadmap/roadmap_overview.md)** - Entwicklungs-Roadmap (2026 komplett!)
- **[Features Priorities](de/features/features_priorities.md)** - Q1 2026 Prioritäten
- **[Database Capabilities](de/reports/database_capabilities_roadmap.md)** - Capabilities Roadmap

---

## 📦 Integration & Ingestion

### Data Ingestion
- **[Ingestion](de/apis/README.md)** - Data Ingestion Patterns
- **[VCC CLARA](../adapters/vcc_clara_ingestion/README.md)** - CLARA Adapter
- **[VCC VERITAS](../adapters/vcc_veritas/README.md)** - VERITAS Adapter
- **[VCC Base](../adapters/vcc_base/README.md)** - Base Adapter Framework

### Enterprise Integration
- **[Enterprise Features](de/enterprise/README.md)** - Rate Limiting, Load Shedding
- **[Integration Analysis](de/reports/INTEGRATION_ANALYSIS.md)** - Legacy-Code Integration

---

## 🔍 Source Code Dokumentation

### Module Documentation (src/)
Alle 26 Module mit detaillierter Dokumentation in [de/src/](de/src/README.md):

- **Acceleration** - GPU/CPU Backends (173K LOC)
- **Analytics** - OLAP, CEP (57K LOC)
- **API** - GraphQL, Geo Hooks
- **Auth** - JWT Validation
- **Cache** - Semantic Cache
- **CDC** - Change Data Capture
- **Content** - 15 File Processors (256K LOC)
- **Exporters** - Data Export
- **Geo** - Spatial Operations
- **Governance** - Policy Engine
- **Importers** - Data Import
- **Index** - Vector, Graph, Secondary (400K LOC)
- **LLM** - LLM Integration
- **Network** - Wire Protocol
- **Observability** - Metrics, Tracing
- **Plugins** - Plugin System
- **Query** - AQL Engine (240K LOC)
- **Replication** - Leader-Follower, Multi-Master (12K LOC)
- **Security** - Encryption, RBAC (187K LOC)
- **Server** - HTTP, API Handlers (164K LOC)
- **Sharding** - VCC-URN, Gossip (300K LOC)
- **Storage** - RocksDB, MVCC (76K LOC)
- **Timeseries** - Gorilla Compression (39K LOC)
- **Transaction** - MVCC, SAGA (42K LOC)
- **Updates** - Schema Migration
- **Utils** - Utilities (120K LOC)

---

## 🎓 Weitere Ressourcen

### External Links
- **[GitHub Wiki](https://github.com/makr-code/ThemisDB/wiki)** - Community Wiki
- **[GitHub Pages](https://makr-code.github.io/ThemisDB/)** - Online Dokumentation
- **[PDF Documentation](https://makr-code.github.io/ThemisDB/themisdb-docs-complete.pdf)** - Vollständige Doku als PDF

### Benchmarking & Performance
- **[Benchmarks Suite](../benchmarks/README.md)** - Benchmark-Framework
- **[Docker Benchmarks](../benchmarks/DOCKER_COMPARATIVE_BENCHMARKS_README.md)** - Competitive Benchmarks
- **[Hardware Constraints](../benchmarks/HARDWARE_CONSTRAINTS_README.md)** - Resource-Constraints Testing

### Release Documentation
- **[v1.0.1 Release Notes](../CHANGELOG.md#101---2025-12-09)** - Latest Release
- **[v1.0.0 Release Notes](../RELEASE_NOTES_v1.0.0.md)** - Production Release
- **[Release Package Structure](../RELEASE_PACKAGE_STRUCTURE.md)** - Package Organization

---

## 📝 Dokumentations-Standards

### Format & Struktur
- **Format**: Markdown (.md)
- **Encoding**: UTF-8
- **Line Endings**: LF (Unix-style)
- **Code Blocks**: Sprache immer angeben
- **Links**: Relative Pfade verwenden

### Contributing
1. **Struktur folgen** - Docs im passenden Unterverzeichnis platzieren
2. **Proper verlinken** - Relative Links zu anderen Dokumenten
3. **README updaten** - Relevante README.md-Dateien aktualisieren
4. **Markdown-Style** - [Style Guide](guides/styleguide.md) befolgen
5. **Aktuell halten** - Docs bei Feature-Änderungen updaten

### Build-Prozess
```powershell
# Dependencies installieren
pip install -r requirements-docs.txt

# Dokumentation bauen
.\build-docs.ps1

# Lokal testen
mkdocs serve
```

Dokumentation wird automatisch zu GitHub Pages deployt bei Merge zu main.

---

## 📞 Support & Community

- **Issues**: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Wiki**: [GitHub Wiki](https://github.com/makr-code/ThemisDB/wiki)
- **Security**: [Security Policy](../SECURITY.md)

---

## 📊 Dokumentations-Statistiken

| Metrik | Wert |
|--------|------|
| **Dokumentationsdateien** | 456+ |
| **Dokumentationsordner** | 71 |
| **Source-Code LOC** | 90.829 |
| **Source Files** | 191 (.cpp) |
| **Header Files** | 132 (.h) |
| **Module** | 26 Verzeichnisse |
| **Logische Komponenten** | 16 |

---

**Version:** 1.8.0-rc1  
**Last Updated:** 6. April 2026  
**License:** See [LICENSE](../LICENSE)
