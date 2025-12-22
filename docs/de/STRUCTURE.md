# 📁 Dokumentations-Struktur ThemisDB v1.3.6

**Aktualisiert:** 22. Dezember 2025  
**Verzeichnisse:** 41 logisch gruppierte Module  
**Dateien:** 650+ Dokumente  
**Sprachen:** Deutsch (primär), Englisch (Stubs)

---

## 🎯 Struktur-Übersicht

Die Dokumentation ist nach **funktionalen Domänen** organisiert, nicht nach technischen Ebenen.

```
docs/de/
├── 📘 Kern-Module (8 Verzeichnisse)
│   ├── apis/           - REST, GraphQL, WebSocket, HTTP/2, HTTP/3, MCP APIs
│   ├── architecture/   - Systemarchitektur, Protokolle, Transaktionen  
│   ├── features/       - Feature-Übersichten, Change Data Capture
│   ├── query/          - Query Engine, AQL, Optimization
│   ├── storage/        - Storage Engine, Indizes, Replication
│   ├── search/         - Vector Search (HNSW), Full-Text, Indexing
│   ├── sharding/       - Sharding Strategy, Routing
│   └── timeseries/     - Time-Series Data, Auto-Rollup
│
├── 🔨 Entwicklung (5 Verzeichnisse)
│   ├── build/          - CMake, Build-Toolchain, CI/CD
│   ├── development/    - Development Status, Implementation Details
│   ├── src/            - Source Code Documentation
│   ├── plugins/        - Plugin Architecture, RPC Framework
│   └── clients/        - Client SDKs, Integration Libraries
│
├── 🔒 Sicherheit & Compliance (7 Verzeichnisse)
│   ├── security/       - Encryption, HSM, Audit, PII Detection
│   ├── compliance/     - BSI C5, ISO 27001, DSGVO, eIDAS, SOC 2
│   ├── governance/     - Data Governance, Classification
│   ├── legal/          - Licenses, License Compatibility
│   ├── policies/       - Data Classification, Access Control
│   ├── audit/          - Audit Logs, Security Reports
│   └── auth/           - Authentication, Authorization
│
├── 📊 Operativ (6 Verzeichnisse)
│   ├── deployment/     - Docker, Kubernetes, Cloud Deployment
│   ├── performance/    - Performance Tuning, GPU Acceleration, Benchmarks
│   ├── observability/  - Monitoring, Logging, Analytics, CEP
│   ├── server/         - Server Configuration, Runtime
│   ├── tools/          - Admin Tools, Utilities
│   └── integrations/   - Third-party Integrations
│
├── 🚀 Features (5 Verzeichnisse)
│   ├── llm/            - LLM Integration, LoRA, vLLM
│   ├── geo/            - Geo-Spatial Features, PostGIS
│   ├── content/        - Content Processing, File Format Handlers
│   ├── connectors/     - Data Import/Export, Adapters
│   └── enterprise/     - Enterprise Features, GPU Impact Analysis
│
├── 📚 Dokumentation (5 Verzeichnisse)
│   ├── guides/         - Quick Start, Build Guides, Troubleshooting
│   ├── releases/       - Release Notes, Updates, Manifests
│   ├── reports/        - Analysis, Strategy, Feasibility Studies
│   ├── roadmap/        - Product Roadmap, Future Features
│   └── admin_tools/    - Admin Documentation, CLI Reference
│
└── 🗂️ Spezial (5 Verzeichnisse)
    ├── archive/        - Deprecated Documentation
    ├── compiled/       - Compiled Full Documentation (auto-generated)
    ├── confidencial/   - Geschäftsgeheimnisse (in .gitignore)
    ├── projects/       - Project Management, Enterprise Contracts
    └── legal/          - Licenses, Legal Documents
```

---

## 🔍 Konsolidierungshistorie

### Start (v1.3.0)
- **59 Verzeichnisse** (fragmentiert)
- Viele Mikro-Verzeichnisse mit nur 1-3 Dateien
- Redundante Strukturen (api+apis, release_notes+releases, etc.)

### Erste Konsolidierung (v1.3.5)
```
api/             → apis/
cicd/            → build/
analysis/ 
+ merge_reports/ → reports/
release_notes/ 
+ updates/       → releases/
importers/ 
+ exporters/     → connectors/
cache/ 
+ replication/   → storage/
index/           → search/
analytics/       → observability/
css/             → content/

Ergebnis: 59 → 46 Verzeichnisse (-22%)
```

### Zweite Konsolidierung (v1.3.6)
```
ingestion/       → apis/
troubleshooting/ → guides/
protocol/        → architecture/
transaction/     → architecture/
cdc/             → features/

Ergebnis: 46 → 41 Verzeichnisse (-30% vs. Start)
```

---

## 📖 Wie man die Struktur navigiert

### Nach Rolle

**👨‍💻 Entwickler**
1. Start: [Quick Start](guides/QUICK_START.md)
2. Build: [Build-Strategie](build/guides_build_strategy.md)
3. API: [HTTP API Reference](apis/HTTP_API_REFERENCE.md)
4. Plugins: [Plugin Architecture](plugins/RPC_PLUGIN_ARCHITECTURE.md)

**📊 Stakeholder/Leads**
1. Overview: [README](../README.md)
2. Roadmap: [Product Roadmap](roadmap/roadmap_overview.md)
3. Reports: [Architecture Reports](reports/README.md)
4. Enterprise: [Enterprise Features](enterprise/README.md)

**🔐 Security/Compliance**
1. Status: [Compliance Dashboard](compliance/compliance_dashboard.md)
2. Audit: [Security Audit Report](security/security_audit_report.md)
3. Details: [Full Compliance Checklist](compliance/compliance_full_checklist.md)
4. Policies: [Data Classification](policies/policies_data_classification.md)

**🚀 DevOps/Operations**
1. Deployment: [Docker Deployment](deployment/DOCKER_DEPLOYMENT.md)
2. Monitoring: [Observability Setup](observability/README.md)
3. Performance: [Performance Tuning](performance/PERFORMANCE_TUNING.md)
4. Tools: [Admin Tools](admin_tools/user_guide.md)

---

## 🏗️ Architektur-Domains

### Daten-Layer
- **storage/** - RocksDB, Page Manager, WAL, Compression
- **search/** - HNSW Vectors, Full-Text, B-Tree Indexes
- **sharding/** - Consistent Hashing, Shard Routing
- **replication/** (→ storage/) - Leader-Follower, CRDTs

### Query-Layer  
- **query/** - SQL Parser, Query Optimization, Execution Plans
- **aql/** - Advanced Query Language EBNF Grammar
- **apis/** - Query Endpoints (REST, GraphQL, gRPC)

### Feature-Layer
- **llm/** - LLM Integration, LoRA Fine-tuning, vLLM
- **geo/** - PostGIS Integration, Spatial Queries
- **timeseries/** - Time-Series Aggregation, Auto-Rollup
- **content/** - File Processing (PDF, Images, Audio, etc.)

### Sicherheits-Layer
- **security/** - Encryption, HSM, Key Management
- **auth/** - Authentication, OIDC, LDAP
- **governance/** - Data Classification, Access Control

### Observability-Layer
- **observability/** - Prometheus, Jaeger, Logs
- **performance/** - Benchmarks, GPU Acceleration
- **audit/** - Audit Trails, Compliance Logs

---

## 📝 Dokumentations-Konventionen

### Datei-Benennung
- `README.md` - Modul-Übersicht und Einstiegspunkt
- `*_specification.md` - Formale Spezifikationen
- `*_implementation.md` - Implementierungs-Details
- `*_guide.md` - How-To und Anleitungen
- `*_analysis.md` - Analysen und Feasibility Studies
- `COMPONENT_*.md` - Komponenten-Dokumentation

### Link-Struktur
- **Intra-Modul:** `other_file.md` (relativ im gleichen Verzeichnis)
- **Inter-Modul:** `../other_module/file.md` (relativ zu Vaterverzeichnis)
- **Dokumentations-Index:** `../../DOCUMENTATION_INDEX.md`

### Sprachkonvention
- **Deutsch** (docs/de/) - Primäre Dokumentation
- **English** (docs/en/) - Stubs mit Hinweis auf deutsche Versionen
- All-Upper-Case Dateinamen für kritische Dokumente

---

## 🔐 Vertrauliche Dokumente

**Location:** `docs/de/confidencial/`  
**Status:** `.gitignore` konfiguriert (nicht versioniert)  
**Inhalt:**
- Projekt-Bewertung & Valuation
- Kostenmodelle & Pricing
- Interne Strategien
- Kundenverträge

---

## 📊 Statistiken

### Verzeichnis-Größen (Top 10)
| Verzeichnis | Dateien | LOC |
|-------------|---------|-----|
| src/ | 98 | ~40K |
| development/ | 64 | ~12K |
| security/ | 61 | ~15K |
| reports/ | 52 | ~25K |
| llm/ | 35 | ~8K |
| guides/ | 29 | ~6K |
| deployment/ | 23 | ~5K |
| architecture/ | 23 | ~4K |
| apis/ | 22 | ~4K |
| performance/ | 19 | ~6K |

**Gesamt:** 650+ Dateien, ~150K LOC Dokumentation

---

## ✅ Konsolidierungs-Checkliste

- [x] Duplicate Verzeichnisse zusammengeführt
- [x] Alle Links aktualisiert (350+ Links)
- [x] .gitignore konfiguriert für confidencial/
- [x] README.md aktualisiert
- [x] Struktur-Dokumentation erstellt (STRUCTURE.md)
- [x] Beide Commits zu GitHub gepusht
- [ ] EN Dokumentation konsolidieren (Phase 2)
- [ ] Automated Structure Validation hinzufügen (Future)

---

## 🔗 Siehe auch

- [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md) - Detaillierter Index aller Dokumente
- [README.md](README.md) - Projekt-Übersicht
- [../README.md](../README.md) - Root Projekt-README
