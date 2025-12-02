# ThemisDB Dokumentations-Index

**Letzte Aktualisierung:** 30. November 2025

## 🎯 Schnelleinstieg nach Rolle

### Für Entwickler
1. [README.md](../README.md) - Projektübersicht & Quick Start
2. [BUILD_STRATEGY.md](BUILD_STRATEGY.md) - Build-Toolchain (Windows/Linux/Docker)
3. [docs/BUILD_GUIDE.md](BUILD_GUIDE.md) - Detaillierte Build-Anleitung
4. [DEVELOPMENT_AUDITLOG.md](development/DEVELOPMENT_SUMMARY.md) - Aktueller Entwicklungsstand
5. [Enterprise Features](enterprise/README.md) - Enterprise Scalability Features

### Für Stakeholder
1. [THEMIS_SACHSTANDSBERICHT_2025.md](THEMIS_SACHSTANDSBERICHT_2025.md) - Executive Summary
2. [THEMIS_PROJECT_VALUATION.md](THEMIS_PROJECT_VALUATION.md) - Wirtschaftliche Bewertung
3. [FEATURES.md](FEATURES.md) - Feature-Übersicht mit Status
4. [ROADMAP.md](roadmap.md) - Entwicklungs-Roadmap

### Für Compliance & Audits
1. [COMPLIANCE_DASHBOARD.md](COMPLIANCE_DASHBOARD.md) - Executive Compliance Summary
2. [FULL_AUDIT_CHECKLIST.md](FULL_AUDIT_CHECKLIST.md) - BSI C5, ISO 27001, DSGVO, eIDAS, SOC 2
3. [security/SECURITY_AUDIT_REPORT.md](reports/SECURITY_AUDIT_REPORT.md) - Security Audit Ergebnisse
4. [SECURITY.md](../SECURITY.md) - Vulnerability Disclosure Policy

## 📚 Dokumentationsstruktur

### Root-Level Dokumente
```
/
├── README.md                        # Projektübersicht & Quick Start
├── FEATURES.md                      # Feature-Liste mit Status
├── ROADMAP.md                       # Entwicklungs-Roadmap
├── CHANGELOG.md                     # Änderungshistorie
├── BUILD_STRATEGY.md                # Build-Toolchain & Strategie
├── INTEGRATION_ANALYSIS.md          # Enterprise Integration Analysis
├── TEST_REPORT.md                   # Vollständiger Test-Report
├── DEVELOPMENT_AUDITLOG.md          # Entwicklungsstand & Audit
└── CONTRIBUTING.md                  # Contribution Guidelines
```

### docs/ - Strukturierte Dokumentation
```
docs/
├── enterprise/                      # Enterprise Features
│   └── README.md                    # Enterprise Übersicht & Guide
├── performance/                     # Performance & Benchmarks
│   └── ENTERPRISE_SCALABILITY_STRATEGY.md
├── security/                        # Sicherheit & Compliance
├── architecture/                    # Architektur-Dokumentation
├── api/                            # API-Dokumentation
└── guides/                         # User Guides
```

## 🚀 Enterprise Features

### Dokumentation
| Dokument | Zweck | Zielgruppe |
|----------|-------|------------|
| [enterprise/README.md](enterprise/README.md) | Übersicht & Quick Start | Entwickler, DevOps |
| [ENTERPRISE_SCALABILITY.md](ENTERPRISE_SCALABILITY.md) | Feature-Details & Code-Beispiele | Entwickler |
| [HTTP_CLIENT_POOL_COMPLETE.md](HTTP_CLIENT_POOL_COMPLETE.md) | HTTP Client Implementation | Entwickler |
| [ENTERPRISE_BUILD_GUIDE.md](ENTERPRISE_BUILD_GUIDE.md) | Build & Deployment | DevOps |
| [ENTERPRISE_FINAL_REPORT.md](ENTERPRISE_FINAL_REPORT.md) | Implementation Summary | Stakeholder |
| [INTEGRATION_ANALYSIS.md](reports/INTEGRATION_ANALYSIS.md) | Legacy Integration | Entwickler |

### Status
- ✅ **Token Bucket Rate Limiter** - Production Ready (5/5 Tests)
- ✅ **Per-Client Rate Limiter** - Production Ready (3/3 Tests)
- ✅ **Load Shedder** - Production Ready (5/5 Tests)
- ✅ **HTTP Client Pool** - Production Ready (6/6 Tests)
- ✅ **Batch Operations** - Production Ready (1/1 Tests)

**Test Coverage:** 20/20 (100%)

## 📖 Architektur & Design

### Kern-Architektur
- [architecture.md](architecture.md) - System-Architektur Übersicht
- [storage/rocksdb_layout.md](storage/rocksdb_layout.md) - RocksDB Storage Layout
- [mvcc_design.md](architecture/mvcc_design.md) - MVCC Transaction Design
- [query_engine_aql.md](aql/query_engine.md) - Query Engine & AQL

### Spezielle Features
- [geo/GEO_ARCHITECTURE.md](geo/architecture.md) - Geo/Spatial Architecture
- [vector_ops.md](features/vector_ops.md) - Vector Operations & HNSW
- [content_pipeline.md](architecture/content_pipeline.md) - Content Processing Pipeline
- [search/hybrid_search_design.md](search/hybrid_search_design.md) - Hybrid Search

## 🔒 Security & Compliance

### Security
- [security/overview.md](security/overview.md) - Security Übersicht
- [encryption_strategy.md](security/encryption_strategy.md) - Verschlüsselungsstrategie
- [security/key_management.md](security/key_management.md) - Key Management
- [security/threat_model.md](security/threat_model.md) - Threat Model
- [security_hardening_guide.md](security/hardening_guide.md) - Hardening Guide

### Compliance
- [COMPLIANCE_DASHBOARD.md](COMPLIANCE_DASHBOARD.md) - Executive Dashboard
- [compliance/DPIA.md](compliance/DPIA.md) - Datenschutz-Folgenabschätzung (DSGVO)
- [compliance/BCP_DRP.md](compliance/BCP_DRP.md) - Business Continuity & Disaster Recovery
- [compliance_audit.md](features/compliance_audit.md) - Compliance Audit
- [AUDIT_LOGGING.md](features/audit_logging.md) - Audit Logging

### PKI & eIDAS
- [pki_integration_architecture.md](security/pki_integration_architecture.md) - PKI Integration
- [eidas_qualified_signatures.md](security/eidas_qualified_signatures.md) - eIDAS Signaturen
- [security/pki_rsa_integration.md](security/pki_rsa_integration.md) - PKI RSA Integration

## 🛠️ Build & Deployment

### Build-Dokumentation
- [BUILD_STRATEGY.md](BUILD_STRATEGY.md) - Build-Strategie & Plattformen
- [BUILD_GUIDE.md](BUILD_GUIDE.md) - Detaillierte Build-Anleitung
- [ENTERPRISE_BUILD_GUIDE.md](ENTERPRISE_BUILD_GUIDE.md) - Enterprise Build Guide

### Deployment
- [deployment.md](guides/deployment.md) - Deployment-Strategien
- [DOCKER_MULTI_ARCH_STRATEGY.md](DOCKER_MULTI_ARCH_STRATEGY.md) - Multi-Arch Docker
- [docs/CI_CD_MULTIARCH.md](CI_CD_MULTIARCH.md) - Multi-Arch CI/CD

### Platform-Specific
- [ARM_RASPBERRY_PI_BUILD.md](ARM_RASPBERRY_PI_BUILD.md) - Raspberry Pi Build
- [ARM_BENCHMARKS.md](ARM_BENCHMARKS.md) - ARM Performance
- [RASPBERRY_PI_TUNING.md](RASPBERRY_PI_TUNING.md) - Pi Tuning Guide

## 📊 Performance & Benchmarks

- [performance_benchmarks.md](performance/benchmarks.md) - Performance Übersicht
- [compression_benchmarks.md](performance/compression_benchmarks.md) - Kompression
- [encryption_metrics.md](security/encryption_metrics.md) - Verschlüsselung Performance
- [performance/ENTERPRISE_SCALABILITY_STRATEGY.md](performance/ENTERPRISE_SCALABILITY_STRATEGY.md) - Enterprise Strategy

## 🔍 API & Query Language

### AQL (Advanced Query Language)
- [aql_syntax.md](aql/syntax.md) - AQL Syntax
- [aql-hybrid-queries.md](aql/hybrid-queries.md) - Hybrid Queries
- [aql_explain_profile.md](aql/explain_profile.md) - EXPLAIN & PROFILE
- [recursive_path_queries.md](features/recursive_path_queries.md) - Rekursive Pfade
- [temporal_graphs.md](features/temporal_graphs.md) - Temporale Graphen

### APIs
- [apis/openapi.md](apis/openapi.md) - REST API & OpenAPI Spec
- [apis/contentfs_api.md](apis/contentfs_api.md) - ContentFS API
- [apis/hybrid_search_api.md](apis/hybrid_search_api.md) - Hybrid Search API

## 👥 Client SDKs

- [clients/javascript_sdk_quickstart.md](clients/javascript_sdk_quickstart.md) - JavaScript SDK
- [clients/python_sdk_quickstart.md](clients/python_sdk_quickstart.md) - Python SDK
- [clients/rust_sdk_quickstart.md](clients/rust_sdk_quickstart.md) - Rust SDK

## 📝 Development

### Guidelines
- [development/developers.md](development/developers.md) - Developer Guide
- [code_quality.md](guides/code_quality.md) - Code Quality Pipeline
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution Guidelines

### Status & Planning
- [DEVELOPMENT_AUDITLOG.md](development/DEVELOPMENT_SUMMARY.md) - Development Audit
- [development/implementation_status.md](development/implementation_status.md) - Status
- [development/roadmap.md](development/roadmap.md) - Roadmap
- [development/priorities.md](development/priorities.md) - Prioritäten

### API Implementations
- [development/audit_api_implementation.md](development/audit_api_implementation.md) - Audit API
- [development/saga_api_implementation.md](development/saga_api_implementation.md) - SAGA API

## 🔗 External Resources

### GitHub
- **Repository:** https://github.com/makr-code/ThemisDB
- **Wiki:** https://github.com/makr-code/ThemisDB/wiki
- **Issues:** https://github.com/makr-code/ThemisDB/issues

### Badges
- [![CI](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml)
- [![Code Quality](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml)
- [![ARM Build](https://github.com/makr-code/ThemisDB/actions/workflows/arm-build.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/arm-build.yml)

## 📋 Navigation nach Thema

### Multi-Model Features
- **Graph:** [property_graph_model.md](features/property_graph_model.md), [graph_index.cpp.md](src/index/graph_index.cpp.md)
- **Geo/Spatial:** [GEO_ARCHITECTURE.md](geo/architecture.md), [geo_acceleration_3d_games.md](geo/geo_acceleration_3d_games.md)
- **Time-Series:** [time_series.md](features/time_series.md), [timeseries/continuous_agg.cpp.md](src/timeseries/continuous_agg.cpp.md)
- **Document:** [content_pipeline.md](architecture/content_pipeline.md), [content/content_manager.cpp.md](src/content/content_manager.cpp.md)
- **Vector/Embedding:** [vector_ops.md](features/vector_ops.md), [gnn_embeddings.md](features/gnn_embeddings.md)

### Storage & Persistence
- **RocksDB:** [storage/rocksdb_layout.md](storage/rocksdb_layout.md), [storage/rocksdb_wrapper.cpp.md](src/storage/rocksdb_wrapper.cpp.md)
- **MVCC:** [mvcc_design.md](architecture/mvcc_design.md)
- **Transactions:** [transactions.md](features/transactions.md), [transaction/saga.cpp.md](src/transaction/saga.cpp.md)
- **Compression:** [compression_strategy.md](performance/compression_strategy.md), [timeseries/gorilla.cpp.md](src/timeseries/gorilla.cpp.md)

### Search & Indexing
- **Fulltext:** [search/fulltext_api.md](search/fulltext_api.md), [search/stemming.md](search/stemming.md)
- **Hybrid Search:** [search/hybrid_search_design.md](search/hybrid_search_design.md)
- **Vector Search:** [vector_ops.md](features/vector_ops.md), [index/vector_index.cpp.md](src/index/vector_index.cpp.md)
- **Geo Indexing:** [geo/cpu_backend.cpp.md](src/geo/cpu_backend.cpp.md)

### Governance & PII
- **PII Detection:** [security/pii_detection.md](security/pii_detection.md), [pii_api.md](security/pii_api.md)
- **Policies:** [security/policies.md](security/policies.md), [governance/policy_engine.cpp.md](src/governance/policy_engine.cpp.md)
- **RBAC:** [rbac_authorization.md](guides/rbac.md), [RBAC.md](guides/rbac.md)
- **Retention:** [security/audit_and_retention.md](security/audit_and_retention.md)

## ⚠️ Deprecated / Archive

Veraltete oder abgelöste Dokumentation:
- [archive/](archive/) - Archivierte Dokumente
- [merge_reports/](merge_reports/) - Git Merge Reports

## 🔄 Synchronisation

### Wiki Sync
```powershell
./sync-wiki.ps1
```

### Lokale Vorschau
```powershell
./build-docs.ps1  # MkDocs → site/
```

### GitHub Pages
- **Primär:** GitHub Wiki (maßgeblich)
- **Sekundär:** MkDocs Build (für Entwicklung)

## 📞 Support

- **Issues:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Diskussionen:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Security:** Siehe [SECURITY.md](../SECURITY.md)

---

**Dokumentations-Status:** ✅ Konsolidiert (30. November 2025)  
**Maintainer:** ThemisDB Team  
**Letzte Audit:** 30. November 2025
