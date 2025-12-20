# ThemisDB Dokumentations-Index

**Stand:** 17. Dezember 2025  
**Version:** 1.3.0 (LLM Integration Release)  
**Typ:** Dokumentations-Index

---

## 🚀 NEW in v1.3.0: Native LLM Integration (Optional Feature) ✅

**AI direkt in der Datenbank - ohne externe API-Kosten!**

> **Wichtig**: LLM-Integration ist ein **optionales Feature** in v1.3.0:
> - Erfordert Build-Flag: `-DTHEMIS_ENABLE_LLM=ON`
> - Benötigt externe Abhängigkeit: llama.cpp (separat klonen)
> - Siehe [Build Guide](guides/guides_build_strategy.md) für Setup-Anweisungen

ThemisDB kann als erste Multi-Model-Datenbank mit **eingebetteter LLM-Engine** erweitert werden:

### Highlights (Production-Ready, wenn aktiviert)

- 🧠 **Embedded llama.cpp** - SLMs/LLMs (1B-70B Parameter) direkt auf GPU ✅
- ⚡ **GPU Acceleration** - Signifikanter Speedup mit NVIDIA CUDA support ✅
- 💾 **PagedAttention** - Optimierte Memory-Verwaltung ✅
- 🎯 **Continuous Batching** - Mehrere concurrent requests ✅
- 🔧 **Kernel Fusion** - CUDA kernels für zusätzlichen Speedup ✅
- 📊 **Production Monitoring** - Grafana/Prometheus Integration ✅
- 🔌 **Plugin Architecture** - Extensible LLM backend system ✅
- 🌐 **RPC Framework** - Inter-Shard Communication für distributed LLM ops ✅

### Performance Metrics (mit GPU)

- **Signifikanter Speedup** mit GPU vs CPU-only
- **Memory-Ersparnis** mit PagedAttention
- **Zusätzliche Optimierung** mit Kernel Fusion
- **Umfassende Test Coverage** mit Unit Tests

### GPU-Tier Empfehlungen

| GPU-Tier | Hardware | Model | Use Case | Kosten/1M Tokens | vs. GPT-4 |
|----------|----------|-------|----------|------------------|-----------|
| **Entry** | RTX 4060 Ti (16GB) | Phi-3-Mini (3.8B) | FAQ, einfache RAG | €0.02 | **1500x günstiger** |
| **Mid-Range** | RTX 4090 (24GB) | Mistral-7B | Production RAG | €0.05 | **600x günstiger** |
| **High-End** | A100 (80GB) | Llama-3-70B | Enterprise Scale | €0.15 | **200x günstiger** |

**Break-Even vs. Hyperscaler:** 2-7 Monate je nach Hardware-Tier

### Dokumentation (v1.3.0)

- **[GPU Inference Guide](llm/GPU_INFERENCE_GUIDE.md)** ⭐ **v1.3.0**
  - CUDA Setup und Konfiguration
  - Performance-Tuning
  - Troubleshooting

- **[Quantization Guide](llm/QUANTIZATION_GUIDE.md)** ⭐ **v1.3.0**
  - Q4_K_M, Q5_K_M, Q8_0 Formate
  - Memory vs. Quality Trade-offs
  - Best Practices

- **[Performance Benchmarks](llm/PERFORMANCE_BENCHMARKS.md)** ⭐ **v1.3.0**
  - GPU vs. CPU Vergleiche
  - Throughput-Messungen
  - Latenz-Analysen

- **[Deployment Guide](llm/DEPLOYMENT_GUIDE.md)** ⭐ **v1.3.0**
  - Docker mit GPU-Support
  - Kubernetes Deployment
  - Production Best Practices

- **[RPC Framework](../docs/plugins/RPC_PLUGIN_ARCHITECTURE.md)** ⭐ **v1.3.0**
  - Inter-Shard Communication
  - TLS/mTLS Security
  - Snapshot/Blob Transfer

- **[GPU-Tier Analyse & Hyperscaler-Vergleich](llm/GPU_TIER_ANALYSIS_HYPERSCALER_COMPARISON.md)**
  - SLM/LLM Performance auf Entry/Mid/High-End GPUs
  - TCO-Analyse über 3 Jahre
  - ROI-Berechnung vs. AWS/Azure/GCP

- **[Alle LLM Dokumentation](llm/README.md)** - Kompletter Index (31 Guides)

---

## 📁 Dokumentations-Struktur (Neu Organisiert)

Die Dokumentation wurde neu strukturiert für bessere Übersichtlichkeit:

**Root-Dokumente (nur essentials):**
- `README.md` - Hauptdokumentation
- `index.md` - Dokumentations-Index
- `glossary.md` - Terminologie

**Organisierte Ordner:**
- `aql/` - **AQL Grammatik (EBNF)** ⭐ **v1.3.0**
- `build/` - Build-System-Dokumentation (BUILD-SYSTEM.md, BUILDGUIDE.md, etc.)
- `development/` - Entwicklungs-Dokumentation (IMPLEMENTATION-*.md, CODE_REVIEW-*.md)
- `guides/` - Benutzer- und Entwickler-Guides (RAILWAY_COMPLETE_GUIDE.md, etc.)
- `architecture/` - Architektur-Dokumentation (ARCHITECTURE_OVERVIEW.md, etc.)
- `stakeholder/` - Stakeholder-Dokumentation
- `releases/` - Release-Notizen (v1.3.0.md, v1.2.0.md, v1.1.0.md, etc.)
- `llm/` - **LLM & AI Integration** ⭐ **v1.3.0 RELEASED**
- `plugins/` - **RPC Framework** ⭐ **v1.3.0**
- `archive/` - Alte/historische Dokumentation

---

> **🔮 COMING SOON - v1.1.0 Optimization Release (Q1 2026):**
> 
> **Fokus:** Bestehende Libraries besser nutzen + vLLM Co-Location  
> **Highlights:**
> - ✅ RocksDB TTL, Incremental Backup, Stats (keine neue Lib!)
> - ✅ TBB Parallel Sort, Concurrent Containers (keine neue Lib!)
> - ✅ Arrow Parquet Export (keine neue Lib!)
> - ✅ **CUDA als Kernbestand** (wenn GPU verfügbar, NICHT Enterprise!)
> - ✅ **🆕 ThemisDB + vLLM Synergie** (optimierte CPU/GPU/RAM Koordination)
> - ✅ mimalloc (einzige neue Dependency, 20-40% Memory Boost)
> 
> **Engineering:** 9-11 Wochen | **Impact:** 3-10x Performance  
> **Details:** [v1.1.0 Variant Strategy](analysis/VARIANT_STRATEGY_v1.1.0.md)

> **🚀 PLANNED - v1.2.0 Enterprise Features (Q2 2026):**
> 
> **Fokus:** vLLM AI Support (LoRA), Geo-Spatial (PostGIS), IoT/Timescale  
> **Highlights:**
> - ✅ **LoRA Manager** - Multi-Tenant LoRA Serving (HuggingFace PEFT)
> - ✅ **FAISS Advanced** - IVF+PQ Vector Search (bereits integriert, erweitern!)
> - ✅ **GEOS + PROJ** - PostGIS Compatibility (Topology + Geography)
> - ✅ **Hypertables** - TimescaleDB-kompatibel via RocksDB CF (nur Code!)
> - ✅ **cuSpatial** - GPU Geo Ops (optional, nutzt Arrow + CUDA)
> 
> **Engineering:** 12-16 Wochen | **Impact:** PostGIS + LoRA + TimescaleDB Compatibility  
> **Details:** [Enterprise Features Strategy](analysis/ENTERPRISE_FEATURES_STRATEGY.md)

---

## 📚 Haupt-Dokumentation

### Übersichtsdokumente
- **[Changelog](../CHANGELOG.md)** - Vollständige Versionshistorie (v1.2.0, v1.1.0, v1.0.2, v1.0.1, v1.0.0)
- **[🆕 Roadmap v1.1.0](roadmap/roadmap_overview.md)** - **AKTUALISIERT:** Q1 2026 Optimization Release
- **[Architecture Overview](architecture/ARCHITECTURE_OVERVIEW.md)** - Komplette Systemarchitektur mit Diagrammen
- **[Source Code Changes v1.0](development/SOURCE_CODE_CHANGES_v1.0.md)** - Detaillierte Quellcode-Dokumentation (191 Dateien, 26 Module)
- **[Features Liste](features/features_overview.md)** - Vollständige Feature-Übersicht mit Status

---

## 🎯 Nach Zielgruppe

### Für Stakeholder & Management
- **[Themis Sachstandsbericht 2025](reports/themis_sachstandsbericht_2025.md)** - Executive Summary, Status v1.0.1
- **[🆕 v1.1.0 Variant Strategy](analysis/VARIANT_STRATEGY_v1.1.0.md)** - **Q1 2026:** Optimierungs-Strategie mit vLLM Co-Location (9-11 Wochen, 1 neue Lib)
- **[🆕 v1.2.0 Enterprise Features](analysis/ENTERPRISE_FEATURES_STRATEGY.md)** - **Q2 2026:** vLLM AI (LoRA), Geo-Spatial (PostGIS), IoT/Timescale (12-16 Wochen, 3 neue Libs)
- **[Projektkostenschätzung & Gesamtwert](reports/reports_project_valuation.md)** - Wirtschaftliche Bewertung (6,3M EUR)
- **[Release Strategy Audit](../RELEASE_STRATEGY_AUDIT.md)** - SLSA Compliance, SBOM (8.5/10 Rating)
- **[Release & Benchmarking Summary](../RELEASE_AND_BENCHMARKING_SESSION_SUMMARY.md)** - v1.0.1 Session Report

### Für Entwickler
- **[Development Summary](development/DEVELOPMENT_SUMMARY.md)** - Entwicklungsstand v1.0.1
- **[🆕 External Libraries Analysis](analysis/EXTERNAL_LIBRARIES_FEATURES_ANALYSIS.md)** - **NEU:** Feature-Gap-Analyse (RocksDB, TBB, CUDA, Arrow)
- **[🆕 Library Interactions](analysis/LIBRARY_INTERACTIONS_AND_EXTENSIONS.md)** - **NEU:** Wechselwirkungen & zusätzliche Libraries
- **[Source Code Audit](development/SOURCE_CODE_AUDIT.md)** - Code-Analyse (132 Header, 124 Sources, 90.829 LOC)
- **[Documentation Index](DOCUMENTATION_INDEX.md)** - Vollständiger Dokumentations-Index mit Modul-Mapping
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
- **[AQL Documentation](aql/README.md)** - Advanced Query Language (Parser, Optimizer, 240K LOC)
- **[Query Module](query/README.md)** - Query Engine, Execution
- **[Analytics Module](analytics/README.md)** - OLAP Engine (CUBE, ROLLUP), CEP, Process Mining (57K LOC)
- **[Search Documentation](search/README.md)** - Fulltext (BM25), Vector, Hybrid Search

### Storage & Index Layer
- **[Storage Module](storage/README.md)** - RocksDB Wrapper, LSM-Tree, MVCC (76K LOC)
- **[Index Module](index/README.md)** - Vector HNSW, Graph, Secondary, Spatial (400K LOC)
- **[Cache Module](cache/README.md)** - Semantic Cache, Result Cache
- **[Timeseries Module](timeseries/README.md)** - Gorilla Compression, Aggregates (39K LOC)

### Distribution & Scaling Layer
- **[Sharding Module](sharding/README.md)** - VCC-URN Sharding, Auto-Rebalancing, Gossip (300K LOC)
- **[Replication Module](replication/README.md)** - Leader-Follower, Multi-Master CRDTs (12K LOC)
- **[Transaction Module](transaction/README.md)** - MVCC, SAGA Patterns (42K LOC)

### Acceleration Layer
- **[GPU Acceleration Plan](performance/GPU_ACCELERATION_PLAN.md)** - 10 GPU Backends (173K LOC)
  - CUDA, Vulkan, FAISS, DirectX, HIP, OpenCL, OneAPI, ZLUDA

### Content & Data Processing
- **[Content Module](content/README.md)** - 15 File Format Processors (256K LOC)
- **[CDC Module](cdc/README.md)** - Change Data Capture, Changefeed
- **[Geo Module](geo/README.md)** - Spatial Operations, Plugin System

### Server & API Layer
- **[Server Module](server/README.md)** - HTTP Server, 21 API Handlers (164K LOC)
- **[HTTP API Referenz](apis/HTTP_API_REFERENCE.md)** - **Vollständige HTTP Endpoint-Dokumentation** ⭐
- **[API Documentation](api/README.md)** - REST API Übersicht
- **[LLM Module](llm/README.md)** - LLM Interaction Store, Prompt Manager

### Security & Governance Layer
- **[Security Module](security/README.md)** - Field Encryption, HSM/PKI, RBAC, Ranger (187K LOC)
- **[Governance Module](governance/README.md)** - Policy Engine, Data Classification
- **[Auth Module](auth/README.md)** - JWT Validation, Multi-Tenancy

---

## 🚀 Quick Start Guides

### Installation & Deployment
- **[Main README](../README.md)** - Projekt-Übersicht und Quick Start
- **[Deployment Guide](deployment/README.md)** - Deployment-Optionen
- **[Docker Guide](../README.docker.md)** - Container-Deployment
- **[QNAP Quickstart](../QNAP_QUICKSTART.md)** - ARM-Deployment

### Getting Started
- **[Architecture Overview](ARCHITECTURE_OVERVIEW.md)** - System-Architektur verstehen
- **[Features Overview](features/features_overview.md)** - Verfügbare Features
- **[AQL Tutorial](aql/README.md)** - Query Language lernen

---

## 📖 Referenz-Dokumentation

### Client SDKs
- **[SDK Audit](clients/clients_sdk_audit.md)** - Übersicht aller 7 SDKs
- **[Python SDK](clients/python_sdk_quickstart.md)** - Python Client
- **[JavaScript SDK](clients/javascript_sdk_quickstart.md)** - Node.js/Browser Client
- **[Rust SDK](clients/rust_sdk_quickstart.md)** - Rust Client
- **[Go SDK](clients/go_sdk_quickstart.md)** - Go Client
- **[Java SDK](clients/java_sdk_quickstart.md)** - Java Client
- **[C# SDK](clients/csharp_sdk_quickstart.md)** - .NET Client
- **[Swift SDK](clients/swift_sdk_quickstart.md)** - iOS/macOS Client

### Data Import/Export
- **[Exporters](exporters/README.md)** - Data Export
  - **[JSONL LLM Exporter](exporters/JSONL_LLM_EXPORTER.md)** - LLM Training Data Export
- **[Importers](importers/README.md)** - Data Import
  - **[PostgreSQL Importer](importers/POSTGRES_IMPORTER.md)** - PostgreSQL Migration

### Plugin Development
- **[Plugins](plugins/README.md)** - Plugin System
- **[Plugin Security](plugins/PLUGIN_SECURITY.md)** - Security & Sandboxing
- **[Plugin Migration](plugins/PLUGIN_MIGRATION.md)** - Migration Guide

---

## 🔧 Administration & Operations

### Admin Tools
- **[Admin Tools](admin_tools/README.md)** - 7 WPF Administration Tools
- **[User Guide](admin_tools/user_guide.md)** - Benutzerhandbuch
- **[Admin Guide](admin_tools/admin_guide.md)** - Administrator-Handbuch
- **[Feature Matrix](admin_tools/feature_matrix.md)** - Tool-Übersicht

### Operations Guides
- **[Operations Runbook](guides/guides_operations_runbook.md)** - Tägliche Operationen
- **[TLS Setup](guides/tls_setup.md)** - TLS/mTLS Konfiguration
- **[Vault Integration](guides/vault.md)** - HashiCorp Vault Setup
- **[RBAC Setup](guides/rbac.md)** - Access Control Configuration
- **[Code Quality](guides/code_quality.md)** - Code Quality Tools

### Performance & Monitoring
- **[Performance Tuning](performance/README.md)** - Performance-Optimierung
- **[Benchmarks](performance/benchmarks.md)** - Performance-Benchmarks
- **[Memory Tuning](performance/memory_tuning.md)** - Speicher-Optimierung
- **[Observability](observability/README.md)** - Monitoring & Metrics

---

## 📊 Reports & Status

### Development Reports
- **[Development Summary](development/DEVELOPMENT_SUMMARY.md)** - Aktueller Entwicklungsstand v1.0.1
- **[Audit Log](development/auditlog.md)** - Entwicklungs-Audit-Log
- **[Implementation Status](development/implementation_status.md)** - Implementierungsstatus
- **[Priorities](development/priorities.md)** - Entwicklungs-Prioritäten

### Status Reports
- **[Themis Sachstandsbericht](reports/themis_sachstandsbericht_2025.md)** - Haupt-Statusbericht v1.5
- **[Documentation Summary](reports/DOCUMENTATION_SUMMARY.md)** - Dokumentations-Übersicht
- **[Benchmark Audit](reports/BENCHMARK_AND_TEST_AUDIT.md)** - Test & Benchmark Status
- **[Security Audit](reports/SECURITY_AUDIT_REPORT.md)** - Security Audit Ergebnisse

### Roadmap & Planning
- **[Roadmap Overview](roadmap/roadmap_overview.md)** - Entwicklungs-Roadmap (2026 komplett!)
- **[Features Priorities](features/features_priorities.md)** - Q1 2026 Prioritäten
- **[Database Capabilities](reports/database_capabilities_roadmap.md)** - Capabilities Roadmap

---

## 📦 Integration & Ingestion

### Data Ingestion
- **[Ingestion](ingestion/README.md)** - Data Ingestion Patterns
- **[VCC CLARA](../adapters/vcc_clara_ingestion/README.md)** - CLARA Adapter
- **[VCC VERITAS](../adapters/vcc_veritas/README.md)** - VERITAS Adapter
- **[VCC Base](../adapters/vcc_base/README.md)** - Base Adapter Framework

### Enterprise Integration
- **[Enterprise Features](enterprise/README.md)** - Rate Limiting, Load Shedding
- **[Integration Analysis](reports/INTEGRATION_ANALYSIS.md)** - Legacy-Code Integration

---

## 🔍 Source Code Dokumentation

### Module Documentation (src/)
Alle 26 Module mit detaillierter Dokumentation in [src/](src/README.md):

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

**Version:** 1.3.0  
**Last Updated:** 20. Dezember 2025  
**License:** See [LICENSE](../LICENSE)
