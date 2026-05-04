# ThemisDB Dokumentations-Index

> **📝 Hinweis zur Dokumentationssprache**  
> Dies ist die **maßgebliche und aktuellste Dokumentation** von ThemisDB.  
> Übersetzungen sind in anderen Sprachen verfügbar: [English](../en/README.md) | [Français](../fr/README.md) | [Español](../es/README.md) | [日本語](../ja/README.md)

**Stand:** 6. April 2026  
**Version:** 1.8.0-rc1  
**Typ:** Dokumentations-Index  
**Sprache:** Deutsch (Hauptdokumentation)

---

## Schnellnavigation (gepflegte Einstiegspfade)

Diese Links bilden den aktuell gepflegten Einstieg in die Dokumentation:

- Root-Einstieg: [../README.md](../README.md)
- Master-Index: [../00_DOCUMENTATION_INDEX.md](../00_DOCUMENTATION_INDEX.md)
- Rollenhub: [../DOCUMENTATION_HUB.md](../DOCUMENTATION_HUB.md)
- Themenindex: [../CATEGORY_INDEX.md](../CATEGORY_INDEX.md)
- Strukturregeln: [../DOCS_ORGANIZATION_PLAN.md](../DOCS_ORGANIZATION_PLAN.md)
- Docs PR Policy: [../governance/DOCS_PR_POLICY.md](../governance/DOCS_PR_POLICY.md)

Historische Reports wurden aus dem Root ausgelagert und sind hier gebuendelt:

- Summaries: [../implementation-history/summaries/README.md](../implementation-history/summaries/README.md)
- Phasen: [../implementation-history/phases/README.md](../implementation-history/phases/README.md)
- Reviews: [../implementation-history/reviews/README.md](../implementation-history/reviews/README.md)
- Status-Reports: [../implementation-history/status-reports/README.md](../implementation-history/status-reports/README.md)

---

## 🚀 NEU in v1.4.0-alpha: Erweiterte LLM-Funktionen ✅

**AI direkt in der Datenbank mit erweiterten Fähigkeiten - ohne externe API-Kosten!**

### Neue Features in v1.4.0-alpha

- 📝 **Grammatik-gesteuerte Generierung** - EBNF/GBNF-Unterstützung für garantiert valide Ausgaben (95-99% Zuverlässigkeit vs. 60-70%)
  - Eingebaute Grammatiken: JSON, XML, CSV, ReAct Agent
  - Thread-sicherer Grammar-Cache mit LRU-Verdrängung
  - Keine Nachbearbeitung erforderlich
- 🔭 **RoPE Scaling** - Erweitertes Kontextfenster von 4K → 32K Tokens (8-fache Vergrößerung)
  - Skalierungsmethoden: Linear, NTK-aware, YaRN
  - Verarbeitung ganzer Forschungspapiere und Codebases
- 🖼️ **Vision Support** - Multi-modale LLMs mit CLIP-basierter Bildcodierung
  - LLaVA-Integration für Bildanalyse
  - Unterstützung für einzelne und mehrere Bilder
- ⚡ **Flash Attention** - CUDA-Kernel für 15-25% Geschwindigkeitssteigerung, 30% Speicherreduktion
  - Optimierter Attention-Mechanismus
  - Backward Pass für Training-Unterstützung
- 🎯 **Speculative Decoding** - 2-3x schnellere Inferenz mit Draft+Target-Modellen
- 🔄 **Continuous Batching** - 2x+ Durchsatz mit dynamischem Request-Batching

### Dokumentation (v1.4.0-alpha)

- **[Grammatik-gesteuerte Generierung](../../en/llm/GRAMMAR_CONSTRAINED_GENERATION.md)** ⭐ **v1.4.0-alpha**
  - EBNF/GBNF-Grammatik-Unterstützung
  - Eingebaute und benutzerdefinierte Grammatiken
  - Verwendungsbeispiele und Best Practices

- **[RoPE Scaling Implementierung](../../en/llm/ROPE_SCALING_IMPLEMENTATION.md)** ⭐ **v1.4.0-alpha**
  - Erweiterte Kontextfenster (4K→32K)
  - Vergleich der Skalierungsmethoden
  - Konfigurationshandbuch

- **[Vision Support Quick Start](../../en/llm/VISION_SUPPORT_QUICK_START.md)** ⭐ **v1.4.0-alpha**
  - Multi-modale LLM-Einrichtung
  - CLIP-Modell-Integration
  - Bildanalyse-Beispiele

- **[Flash Attention Implementierung](../../en/llm/FLASH_ATTENTION_IMPLEMENTATION.md)** ⭐ **v1.4.0-alpha**
  - CUDA-Kernel-Optimierung
  - Performance-Benchmarks
  - Konfigurationshandbuch

- **[Speculative Decoding](../../en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md)** ⭐ **v1.4.0-alpha**
  - Draft+Target-Modell-Pairing
  - 2-3x Speedup-Anleitung
  - Modellempfehlungen

- **[Continuous Batching](../../en/llm/CONTINUOUS_BATCHING_IMPLEMENTATION.md)** ⭐ **v1.4.0-alpha**
  - Dynamische Batching-Konfiguration
  - Durchsatz-Optimierung
  - Token-Budget-Management

## 🚀 LLM-Integration (Optionales Feature) - v1.3.0 Basis

> **Wichtig**: LLM-Integration ist ein **optionales Feature** in v1.3.0+:
> - Erfordert Build-Flag: `-DTHEMIS_ENABLE_LLM=ON`
> - Benötigt externe Abhängigkeit: llama.cpp (separat klonen)
> - Siehe [Build Guide](guides/guides_build_strategy.md) für Setup-Anweisungen

ThemisDB kann als erste Multi-Model-Datenbank mit **eingebetteter LLM-Engine** erweitert werden:

### Kernfunktionen (v1.3.0)

- 🧠 **Embedded llama.cpp** - SLMs/LLMs (1B-70B Parameter) direkt auf GPU ✅
- ⚡ **GPU Acceleration** - Signifikanter Speedup mit NVIDIA CUDA support ✅
- 💾 **PagedAttention** - Optimierte Memory-Verwaltung ✅
- 🎯 **Continuous Batching** - Mehrere concurrent requests ✅
- 🔧 **Kernel Fusion** - CUDA kernels für zusätzlichen Speedup ✅
- 📊 **Production Monitoring** - Grafana/Prometheus Integration ✅
- 🔌 **Plugin Architecture** - Extensible LLM backend system ✅
- 🌐 **RPC Framework** - Inter-Shard Communication für distributed LLM ops ✅
- 🖼️ **Image Analysis Plugins** - Multi-backend AI (llama.cpp Vision, ONNX CLIP, OpenCV DNN) ✅

### Network Protocol Enhancements (v1.3.0)

- 🌐 **HTTP/2 with Server Push** - CDC/Changefeed mit proaktiver Event-Delivery (~0ms Latenz) ✅
- 🔌 **WebSocket Support** - CDC streaming mit bidirektionaler Echtzeit-Kommunikation ✅
- 📡 **MQTT Broker** - WebSocket transport, Rate limiting, Monitoring-Metriken ✅
- 🚀 **HTTP/3 Base** - QUIC-basierte Implementierung (ngtcp2 + nghttp3) 🚧
- 🐘 **PostgreSQL Wire Protocol** - SQL-to-Cypher Translation für BI-Tool Kompatibilität ✅
- 🤖 **MCP Server** - Model Context Protocol mit cross-platform Support ✅

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

- **[RPC Framework](plugins/RPC_PLUGIN_ARCHITECTURE.md)** ⭐ **v1.3.0**
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
- `reports/` - **Build & Code Reviews** - Build-Reports, Code-Reviews, Analyse-Berichte ⭐ **NEU**
- `guides/` - **Setup & Build-Guides** - Quickstart, Docker, Windows, VS Code Setup ⭐ **NEU**
- `releases/` - **Release-Planung** - RC-Checklisten, Roadmaps ⭐ **NEU**
- `performance/` - **Performance-Optimierung** - Cache, Query, Library Optimizations ⭐ **NEU**
- `implementation/` - **Implementierungs-Summaries** - Feature-Implementierungen (AQL, LoRA, etc.) ⭐ **NEU**
- `features/` - **Feature-Dokumentation** - Cloud Storage, CRON, gRPC, TLS, etc. ⭐ **NEU**
- `phase_reports/` - **Projekt-Phasen** - Phase 3-6 Zusammenfassungen ⭐ **NEU**
- `security/` - **Security Hardening** - Security Summaries & Analysen ⭐ **NEU**
- `lora/` - **LoRA-Stabilisierung** - LoRA-spezifische Analysen & Pläne ⭐ **NEU**
- `architecture/` - **Architektur-Dokumentation** - Vector Indexing, etc. ⭐ **NEU**
- `reference/` - **Referenz-Dokumentation** - Dependencies, Sources ⭐ **NEU**
- `build/` - Build-System-Dokumentation (BUILD-SYSTEM.md, BUILDGUIDE.md, etc.)
- `development/` - Entwicklungs-Dokumentation (IMPLEMENTATION-*.md, CODE_REVIEW-*.md)
- `stakeholder/` - Stakeholder-Dokumentation
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
> **Details:** [v1.1.0 Variant Strategy](reports/VARIANT_STRATEGY_v1.1.0.md)

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
> **Details:** [Enterprise Features Strategy](reports/ENTERPRISE_FEATURES_STRATEGY.md)

---

## 📚 Haupt-Dokumentation

### Übersichtsdokumente
- **[Changelog](releases/CHANGELOG.md)** - Vollständige Versionshistorie (v1.8.0-rc1, v1.5.0, v1.4.0, v1.3.0, …)
- **[🆕 Roadmap v2.0](../../ROADMAP.md)** - **AKTUALISIERT:** Aggregierte Roadmap über alle 46 Module
- **[Architecture Overview](architecture/ARCHITECTURE_OVERVIEW.md)** - Komplette Systemarchitektur mit Diagrammen
- **[Source Code Changes v1.0](development/SOURCE_CODE_CHANGES_v1.0.md)** - Detaillierte Quellcode-Dokumentation (191 Dateien, 44 Module)
- **[Features Liste](features/features_overview.md)** - Vollständige Feature-Übersicht mit Status

---

## 🎯 Nach Zielgruppe

### Für Stakeholder & Management
- **[⭐ Strategiepapier: Industrie 4.0 & IoT](strategie/STRATEGIEPAPIER_INDUSTRIE_4_0_IOT.md)** - **NEU:** ThemisDB für Smart Manufacturing & IoT-Anwendungen
- **[Themis Sachstandsbericht 2025](reports/themis_sachstandsbericht_2025.md)** - Executive Summary, Status v1.0.1
- **[🆕 v1.1.0 Variant Strategy](reports/VARIANT_STRATEGY_v1.1.0.md)** - **Q1 2026:** Optimierungs-Strategie mit vLLM Co-Location (9-11 Wochen, 1 neue Lib)
- **[🆕 v1.2.0 Enterprise Features](reports/ENTERPRISE_FEATURES_STRATEGY.md)** - **Q2 2026:** vLLM AI (LoRA), Geo-Spatial (PostGIS), IoT/Timescale (12-16 Wochen, 3 neue Libs)
- ~~Projektkostenschätzung & Gesamtwert~~ - 🔒 Confidential (available to licensed customers only)
- **[Release Strategy Audit](archive/RELEASE_STRATEGY_AUDIT.md)** - SLSA Compliance, SBOM (8.5/10 Rating)
- **[Release & Benchmarking Summary](archive/RELEASE_AND_BENCHMARKING_SESSION_SUMMARY.md)** - v1.0.1 Session Report

### Für Entwickler
- **[Development Summary](development/DEVELOPMENT_SUMMARY.md)** - Entwicklungsstand v1.0.1
- **[🆕 External Libraries Analysis](reports/EXTERNAL_LIBRARIES_FEATURES_ANALYSIS.md)** - **NEU:** Feature-Gap-Analyse (RocksDB, TBB, CUDA, Arrow)
- **[🆕 Library Interactions](reports/LIBRARY_INTERACTIONS_AND_EXTENSIONS.md)** - **NEU:** Wechselwirkungen & zusätzliche Libraries
- **[Source Code Audit](development/SOURCE_CODE_AUDIT.md)** - Code-Analyse (132 Header, 124 Sources, 90.829 LOC)
- **[Documentation Index](DOCUMENTATION_INDEX.md)** - Vollständiger Dokumentations-Index mit Modul-Mapping
- **[Documentation Verification](reports/documentation_verification_report.md)** - Verifizierung Dokumentation ↔ Code

### Für DevOps & Operations
- **[Operations Runbook](guides/guides_operations_runbook.md)** - Tägliche Operationen
- **[Deployment Guide](deployment/README.md)** - Deployment-Strategien
- **[Build Strategy](guides/guides_build_strategy.md)** - Build-Toolchain
- **[Docker Guide](archive/README.docker.md)** - Container-Deployment

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
- **[Analytics Module](observability/README.md)** - OLAP Engine (CUBE, ROLLUP), CEP, Process Mining (57K LOC)
- **[Search Documentation](search/README.md)** - Fulltext (BM25), Vector, Hybrid Search

### Storage & Index Layer
- **[Storage Module](storage/README.md)** - RocksDB Wrapper, LSM-Tree, MVCC (76K LOC)
- **[Index Module](search/README.md)** - Vector HNSW, Graph, Secondary, Spatial (400K LOC)
- **[Cache Module](storage/README.md)** - Semantic Cache, Result Cache
- **[Timeseries Module](timeseries/README.md)** - Gorilla Compression, Aggregates (39K LOC)

### Distribution & Scaling Layer
- **[Sharding Module](README.md)** - VCC-URN Sharding, Auto-Rebalancing, Gossip (300K LOC)
- **[Replication Module](storage/README.md)** - Leader-Follower, Multi-Master CRDTs (12K LOC)
- **[Transaction Module](architecture/README.md)** - MVCC, SAGA Patterns (42K LOC)

### Acceleration Layer
- **[GPU Acceleration Plan](performance/GPU_ACCELERATION_PLAN.md)** - 10 GPU Backends (173K LOC)
  - CUDA, Vulkan, FAISS, DirectX, HIP, OpenCL, OneAPI, ZLUDA

### Content & Data Processing
- **[Content Module](content/README.md)** - 15 File Format Processors (256K LOC)
- **[CDC Module](features/README.md)** - Change Data Capture, Changefeed
- **[Geo Module](geo/README.md)** - Spatial Operations, Plugin System

### Server & API Layer
- **[Server Module](server/README.md)** - HTTP Server, 21 API Handlers (164K LOC)
- **[HTTP API Referenz](apis/HTTP_API_REFERENCE.md)** - **Vollständige HTTP Endpoint-Dokumentation** ⭐
- **[API Documentation](apis/README.md)** - REST API Übersicht
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
- **[Docker Guide](archive/README.docker.md)** - Container-Deployment
- **[QNAP Quickstart](archive/QNAP_QUICKSTART.md)** - ARM-Deployment

### Getting Started
- **[Architecture Overview](architecture/ARCHITECTURE_OVERVIEW.md)** - System-Architektur verstehen
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
- **[Exporters](connectors/README.md)** - Data Export
  - **[JSONL LLM Exporter](connectors/JSONL_LLM_EXPORTER.md)** - LLM Training Data Export
- **[Importers](connectors/README.md)** - Data Import
  - **[PostgreSQL Importer](connectors/POSTGRES_IMPORTER.md)** - PostgreSQL Migration

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
- **[Ingestion](README.md)** - Data Ingestion Patterns
- **[VCC CLARA](README.md)** - CLARA Adapter
- **[VCC VERITAS](README.md)** - VERITAS Adapter
- **[VCC Base](README.md)** - Base Adapter Framework

### Enterprise Integration
- **[Enterprise Features](enterprise/README.md)** - Rate Limiting, Load Shedding
- **[Integration Analysis](reports/INTEGRATION_ANALYSIS.md)** - Legacy-Code Integration

---

## 🔍 Source Code Dokumentation

### Gesamtstatus Produktionsbereitschaft

ThemisDB verfügt über **42 produktionsreife Module**, **1 Release-Candidate-Modul** und **1 Beta-Modul** im gesamten Quellcode-Baum. Der gesamte Core-Datenpfad und alle KI/LLM-Ebenen sind produktionsbereit.

| Stufe              | Anzahl | Module                                                                        |
|--------------------|--------|-------------------------------------------------------------------------------|
| Produktionsreif    | 42     | 42 von 44 Modulen — alle außer `security` und `sharding`                      |
| Release-Candidate  | 1      | `security`                                                                    |
| Beta               | 1      | `sharding`                                                                    |

### Module Documentation (src/)
Alle 44 ThemisDB-Module mit vollständiger Dokumentation direkt in den Source-Verzeichnissen:

**Dokumentation pro Modul in `../../src/<module>/`:**
- `README.md` - Modul-Übersicht, Architektur, APIs
- `FUTURE_ENHANCEMENTS.md` - Geplante Features & Verbesserungen (falls vorhanden)

**Header-Dokumentation in `../../include/<module>/`:**
- `README.md` - Header-Dokumentation und API-Referenz

**Module nach Kategorie:**

- **acceleration** 🟢 - GPU/CPU Backends (CUDA, Vulkan)
- **analytics** 🟢 - OLAP, CEP, Process Mining (57K LOC)
- **api** 🟢 - GraphQL, Geo Hooks, HTTP API
- **aql** 🟢 - AQL-Sprachengine, Multi-Paradigma-Abfragen
- **auth** 🟢 - JWT, RBAC, Enterprise SSO/MFA
- **base** 🟢 - Grundlegende Abstraktionen
- **cache** 🟢 - Semantischer Cache, Abfrageergebnis-Cache
- **cdc** 🟢 - Change Data Capture, Changefeeds
- **chimera** 🟢 - Hybride Multi-Modell-Schicht
- **config** 🟢 - Konfigurationsverwaltung
- **content** 🟢 - 15 Dateiformat-Prozessoren (256K LOC)
- **core** 🟢 - Core-Datenbank-Runtime
- **exporters** 🟢 - Datenexport (JSONL, LLM-Formate)
- **geo** 🟢 - Geospatiale Abfragen und Indizierung
- **governance** 🟢 - Policy Engine, Compliance-Governance
- **gpu** 🟢 - GPU-Compute-Integration
- **graph** 🟢 - Property-Graph-Abfragen und -Traversal
- **importers** 🟢 - Datenimport (PostgreSQL u. a.)
- **index** 🟢 - HNSW, R-Baum, adaptive Indizierung (400K LOC)
- **ingestion** 🟢 - Datenaufnahme-Pipeline
- **llm** 🟢 - LLM-Interaktionsspeicher, Chain-of-Thought
- **metadata** 🟢 - Metadatenverwaltung und -katalog
- **network** 🟢 - Netzwerkschicht und Peer-Kommunikation
- **observability** 🟢 - Metriken, Tracing und Logging
- **performance** 🟢 - Benchmarking und Leistungsoptimierung
- **plugins** 🟢 - Plugin-System-Infrastruktur
- **prompt_engineering** 🟢 - LLM-Prompt-Verwaltung
- **query** 🟢 - AQL-Optimierer, kostenbasierter Planer, Ausführungsengine (240K LOC)
- **rag** 🟢 - Retrieval-Augmented Generation Pipeline
- **replication** 🟢 - Raft-basierte Replikation (12K LOC)
- **scheduler** 🟢 - Aufgaben- und Job-Planung
- **search** 🟢 - Volltext- und Hybrid-Suche
- **security** 🟡 RC - Verschlüsselung, Schlüsselverwaltung, PKI-Integration (187K LOC)
- **server** 🟢 - HTTP-Server, API-Handler (164K LOC)
- **sharding** 🟡 Beta - Horizontale Skalierung, VCC-URN, Gossip (300K LOC)
- **storage** 🟢 - RocksDB-Wrapper, MVCC, Backup/Recovery (76K LOC)
- **temporal** 🟢 - Temporale und bitemporale Datenverwaltung
- **themis** 🟢 - Core ThemisDB-Orchestrierungsschicht
- **timeseries** 🟢 - Zeitreihendatenverwaltung, Gorilla-Kompression (39K LOC)
- **training** 🟢 - ML-Modell-Training-Integration
- **transaction** 🟢 - SAGA-Muster, verteilte Transaktionen (42K LOC)
- **updates** 🟢 - Schema- und Daten-Update-Verwaltung
- **utils** 🟢 - Gemeinsame Hilfsfunktionen (120K LOC)
- **voice** 🟢 - Sprach-Abfrage-Schnittstelle

---

## 🎓 Weitere Ressourcen

### External Links
- **[GitHub Wiki](https://github.com/makr-code/ThemisDB/wiki)** - Community Wiki
- **[GitHub Pages](https://makr-code.github.io/ThemisDB/)** - Online Dokumentation
- **[PDF Documentation](https://makr-code.github.io/ThemisDB/themisdb-docs-complete.pdf)** - Vollständige Doku als PDF

### Benchmarking & Performance
- **[Benchmarks Suite](README.md)** - Benchmark-Framework
- **[Docker Benchmarks](../benchmarks/DOCKER_COMPARATIVE_BENCHMARKS_README.md)** - Competitive Benchmarks
- **[Hardware Constraints](../benchmarks/HARDWARE_CONSTRAINTS_README.md)** - Resource-Constraints Testing

### Release Documentation
- **[v1.0.1 Release Notes](releases/CHANGELOG.md#101---2025-12-09)** - Latest Release
- **[v1.0.0 Release Notes](archive/RELEASE_NOTES_v1.0.0.md)** - Production Release
- **[Release Package Structure](archive/RELEASE_PACKAGE_STRUCTURE.md)** - Package Organization

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
| **Dokumentationsdateien** | 887+ |
| **Dokumentationsordner** | 71+ |
| **Source-Code LOC** | 90.829+ |
| **Source Files** | 191+ (.cpp) |
| **Header Files** | 132+ (.h) |
| **Module** | 44 |
| **Produktionsreife Module** | 42 🟢 |
| **Release-Candidate Module** | 1 🟡 |
| **Beta Module** | 1 🟡 |

---

**Version:** 1.8.0-rc1  
**Last Updated:** 6. April 2026  
**License:** See [LICENSE](../LICENSE)
