# ThemisDB Dokumentation

Willkommen bei ThemisDB - einer hochperformanten Multi-Model-Datenbank mit Unterstützung für Dokumente, Graphen, Vektoren, Zeitreihen und Geospatial-Daten.

## Schnellzugriff

📚 **[Vollständige Features-Übersicht](FEATURES.md)** - Alle Funktionen mit Status-Indikatoren  
📊 **[Sachstandsbericht 2025](THEMIS_SACHSTANDSBERICHT_2025.md)** - Umfassender Report für Stakeholder  
📖 **[Dokumentations-Index](DOCUMENTATION_INDEX.md)** - Vollständiger Überblick über alle Dokumente  
🔍 **[Quick Reference](DOCS_QUICKREF.md)** - Schnelleinstieg für Entwickler  
📄 **[PDF-Download](ThemisDB-Documentation.pdf)** - Gesamte Dokumentation als PDF  

## Für wen ist diese Dokumentation?

### 👔 Stakeholder & Management
- **Produktionsreife & Status:** [Sachstandsbericht 2025](THEMIS_SACHSTANDSBERICHT_2025.md)
- **Feature-Übersicht:** [FEATURES.md](FEATURES.md)
- **Compliance & Governance:** [Compliance Features](features/compliance.md) | [Security Overview](security/overview.md)
- **Roadmap:** [ROADMAP.md](ROADMAP.md) | [Database Capabilities](reports/database_capabilities_roadmap.md)

### 👨‍💻 Entwickler
- **Schnelleinstieg:** [Quick Reference](DOCS_QUICKREF.md) | [Build Guide](BUILD_GUIDE.md)
- **Query Language:** [AQL Syntax](aql/syntax.md) | [AQL Overview](aql/README.md)
- **SDKs:** [JavaScript](clients/javascript_sdk_quickstart.md) | [Python](clients/python_sdk_quickstart.md) | [Rust](clients/rust_sdk_quickstart.md)
- **APIs:** [OpenAPI Spec](apis/openapi.md) | [REST API](src/api/README.md)
- **Architektur:** [Architecture Overview](architecture/README.md) | [Strategic Overview](architecture/strategic_overview.md)

### 👩‍💼 Benutzer
- **AQL Tutorial:** [AQL Syntax](aql/syntax.md) | [Pattern Matching](aql/pattern_matching.md)
- **Hybrid Search:** [Hybrid Search Design](search/hybrid_search_design.md) | [Fulltext API](search/fulltext_api.md)
- **Vektor-Operationen:** [Vector Ops](features/vector_ops.md) | [GNN Embeddings](features/gnn_embeddings.md)
- **Graph-Features:** [Property Graph](features/property_graph_model.md) | [Temporal Graphs](features/temporal_graphs.md)
- **Content Management:** [Content Search](content/search_api.md) | [Filesystem API](content/filesystem_api.md)

### 🔧 DevOps & Operators
- **Deployment:** [Deployment Guide](guides/deployment.md) | [Docker Build](deployment/docker_build.md)
- **Operations:** [Operations Runbook](guides/operations_runbook.md) | [TLS Setup](guides/tls_setup.md)
- **Monitoring:** [Prometheus Metrics](observability/prometheus_metrics.md) | [Tracing](observability/tracing.md)
- **Performance:** [Benchmarks](performance/benchmarks.md) | [Memory Tuning](performance/memory_tuning.md)
- **Security:** [RBAC](guides/rbac_authorization.md) | [Hardening Guide](security/hardening_guide.md)

## Hauptfunktionen

### 🔍 Query & Suche
- **AQL (Advanced Query Language):** SQL-ähnliche Syntax mit Graph-Traversierung
- **Hybrid Search:** Kombination von Volltext, Vektor und Graph-Suche
- **Pattern Matching:** Komplexe Graph-Muster mit Cypher-ähnlicher Syntax
- **Semantic Cache:** KI-gestützte Query-Optimierung

### 💾 Storage & Indexes
- **Multi-Model:** Dokumente, Graphen, Vektoren, Zeitreihen, Geo in einer DB
- **HNSW Vector Index:** Hochperformante Nearest-Neighbor-Suche
- **Spatial Indexes:** R-Tree für Geospatial-Queries
- **RocksDB Backend:** Transaktionale Persistenz mit MVCC

### 🔐 Security & Compliance
- **End-to-End Encryption:** Column-Level Encryption, TLS/mTLS
- **RBAC:** Rollen-basierte Zugriffskontrolle
- **PKI Integration:** eIDAS Qualified Signatures, HSM Support
- **Compliance:** GDPR/DSGVO, Audit Logging, PII Detection

### ⚡ Performance
- **Enterprise Scalability:** Rate Limiting, Load Shedding, Connection Pooling
- **Hardware Acceleration:** GPU Support (CUDA, Vulkan), Multi-CPU
- **Compression:** ZSTD, LZ4 für Storage-Optimierung
- **Sharding:** Horizontal Scaling mit Auto-Sharding

## Schnellstart-Guides

### Installation & Build
```bash
# Repository klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Dependencies installieren (vcpkg)
.\setup.ps1

# Build
.\build.ps1
```

Weitere Details: [Build Guide](BUILD_GUIDE.md) | [Build Strategy](BUILD_STRATEGY.md)

### Erste Schritte mit AQL
```sql
-- Dokument erstellen
FOR doc IN documents
  INSERT { name: "ThemisDB", type: "database" } INTO documents

-- Graph-Traversierung
FOR v, e, p IN 1..3 OUTBOUND "users/alice" GRAPH "social"
  RETURN p

-- Hybrid Search (Vektor + Volltext)
FOR doc IN documents
  SEARCH PHRASE(doc.text, "database") OR VECTOR_DISTANCE(doc.embedding, @query_vec) < 0.5
  RETURN doc
```

Weitere Beispiele: [AQL Syntax](aql/syntax.md) | [Hybrid Queries](aql/hybrid-queries.md)

## Aktuelle Schwerpunkte & Entwicklung

### ✅ Abgeschlossen (Q4 2025)
- ✅ Enterprise Scalability Features (Rate Limiter v2, Load Shedder, HTTP Client Pool)
- ✅ HNSW Vector Index mit Persistence
- ✅ Content Pipeline mit Image/Geo Processors
- ✅ Column-Level Encryption
- ✅ Audit Logging & Change Data Capture
- ✅ **TSStore Stabilisierung (Time Series Aggregationen)** - [Report](reports/TSSTORE_STABILIZATION.md)
- ✅ **Tracing/Observability Erweiterungen**
- ✅ **OpenAPI Updates (Keys, Classification, Reports)**
- ✅ **Sharding Phase 2-3 (Automatic Rebalancing)** - [Report](reports/SHARDING_AUTO_REBALANCING.md)

### 🔄 In Arbeit
- ✅ GPU Acceleration (CUDA/Vulkan Backends) - [Dokumentation](performance/GPU_ACCELERATION_PLAN.md)
- ✅ Multi-Tenancy Support - [Dokumentation](features/multi_tenancy.md)
- ✅ GraphQL API - [Dokumentation](apis/graphql.md)
- ✅ Advanced Analytics (OLAP Features) - [Dokumentation](features/olap_analytics.md)

Details: [Roadmap](ROADMAP.md) | [Implementation Status](development/implementation_status.md)

## Dokumentations-Kategorien

### Kern-Dokumentation
- **[Architecture](architecture/README.md)** - System-Design, MVCC, Caching
- **[AQL](aql/README.md)** - Query Language Reference
- **[Features](features/)** - Detaillierte Feature-Beschreibungen
- **[Security](security/overview.md)** - Sicherheitsarchitektur
- **[APIs](apis/openapi.md)** - REST API Spezifikation

### Guides & Tutorials
- **[Build Guide](BUILD_GUIDE.md)** - Compilation & Dependencies
- **[Deployment](guides/deployment.md)** - Production Deployment
- **[Operations Runbook](guides/operations_runbook.md)** - Day-to-day Operations
- **[Quality Assurance](guides/quality_assurance.md)** - Testing & QA

### Erweiterte Themen
- **[Performance](performance/benchmarks.md)** - Benchmarks & Tuning
- **[Search](search/hybrid_search_design.md)** - Hybrid Search Architecture
- **[Content](content/ingestion.md)** - Content Management
- **[Geo](geo/README.md)** - Geospatial Features
- **[Sharding](sharding/README.md)** - Horizontal Scaling

### Entwickler-Ressourcen
- **[Source Code Docs](src/README.md)** - Code-Dokumentation
- **[Development](development/README.md)** - Contributor Guide
- **[Reports](reports/)** - Technical Reports & Analysis

## Externe Links

- 🌐 **GitHub Repository:** [makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)
- 📖 **GitHub Wiki:** [ThemisDB Wiki](https://github.com/makr-code/ThemisDB/wiki)
- 🐛 **Issues:** [Issue Tracker](https://github.com/makr-code/ThemisDB/issues)
- 📊 **Projekt Board:** [GitHub Projects](https://github.com/makr-code/ThemisDB/projects)

## Support & Community

Bei Fragen oder Problemen:

1. **Dokumentation durchsuchen:** Nutze die Suchfunktion oder den [Dokumentations-Index](DOCUMENTATION_INDEX.md)
2. **Known Issues prüfen:** [Known Issues](guides/known_issues.md)
3. **Issue erstellen:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
4. **Pull Request:** Contributions willkommen! Siehe [CONTRIBUTING.md](CONTRIBUTING.md)

## Lizenz

ThemisDB ist unter der [MIT License](../LICENSE) verfügbar.

---

**Letzte Aktualisierung:** 30. November 2025  
**Version:** 1.0.0  
**Dokumentations-Status:** Vollständig (361 Dokumente, 25 Kategorien)
