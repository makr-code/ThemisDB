# ThemisDB Dokumentations-Index

> **📌 Navigation-Hinweis (2026-08-12):** Kanonischer Root-Index für `docs/de/` ist [`00_DOCUMENTATION_INDEX.md`](00_DOCUMENTATION_INDEX.md).
> Diese Datei enthält ältere Version (Stand Januar 2026); bitte den kanonischen Index für den aktuellen Stand nutzen.
> <!-- duplicate-notice: DOC-WEEKLY-2026-33 — DOCS-INV-005 -->

**Letzte Aktualisierung:** 7. Januar 2026
**Version:** 1.3.6 (Monetary Valuation Analysis)

## 🎯 Schnelleinstieg nach Rolle

### Für Entwickler
1. [README.md](../README.md) - Projektübersicht & Quick Start
2. [guides/guides_build_strategy.md](guides/guides_build_strategy.md) - Build-Toolchain (Windows/Linux/Docker)
3. [docs/guides/guides_build.md](guides/guides_build.md) - Detaillierte Build-Anleitung
4. [DEVELOPMENT_AUDITLOG.md](development/DEVELOPMENT_SUMMARY.md) - Aktueller Entwicklungsstand
5. [Enterprise Features](enterprise/README.md) - Enterprise Scalability Features

### Für Stakeholder
1. [THEMISDB_MONETARY_VALUATION_ANALYSIS.md](THEMISDB_MONETARY_VALUATION_ANALYSIS.md) - ⭐ **NEU:** Monetäre Bewertungsanalyse & Hyperscaler-Vergleich (AWS/Azure/GCP), TCO-Analyse, Unternehmenswert €40M-€250M
2. [STRATEGIEPAPIER_INDUSTRIE_4_0_IOT.md](strategie/STRATEGIEPAPIER_INDUSTRIE_4_0_IOT.md) - Industrie 4.0 & IoT Strategie
3. [BLAULICHT_STRATEGIE.md](BLAULICHT_STRATEGIE.md) - Strategie für Rettungsdienst, Feuerwehr, Polizei (KRITIS)
4. [THEMIS_SACHSTANDSBERICHT_2025.md](reports/themis_sachstandsbericht_2025.md) - Executive Summary
5. [COMPARATIVE_ANALYSIS_v1.3.4.md](COMPARATIVE_ANALYSIS_v1.3.4.md) - Benchmark-Vergleiche mit Konkurrenten
6. [deployment/PRICING_MODEL_v1.3.5.md](deployment/PRICING_MODEL_v1.3.5.md) - Preismodell & Lizenz-Editionen
7. [features/features_overview.md](features/features_overview.md) - Feature-Übersicht mit Status
8. [ROADMAP.md](roadmap/roadmap_overview.md) - Entwicklungs-Roadmap

### Für Compliance & Audits
1. [compliance/compliance_dashboard.md](compliance/compliance_dashboard.md) - Executive Compliance Summary
2. [compliance/compliance_full_checklist.md](compliance/compliance_full_checklist.md) - BSI C5, ISO 27001, DSGVO, eIDAS, SOC 2
3. [security/SECURITY_AUDIT_REPORT.md](security/security_audit_report.md) - Security Audit Ergebnisse
4. [SECURITY.md](../SECURITY.md) - Vulnerability Disclosure Policy
5. [legal/LICENSE_COMPATIBILITY_ANALYSIS.md](legal/LICENSE_COMPATIBILITY_ANALYSIS.md) - ⭐ License Compatibility (v1.3.0)
6. [THIRD_PARTY_LICENSES.md](legal/THIRD_PARTY_LICENSES.md) - ⭐ Third-Party License Attribution (v1.3.0)

## 📚 Dokumentationsstruktur

### Root-Level Dokumente
```
/
├── README.md                        # Projektübersicht & Quick Start
├── LICENSE                          # MIT License with Government Clause
├── THIRD_PARTY_LICENSES.md          # ⭐ Third-Party License Attribution (v1.3.0)
├── aql/                             # ⭐ AQL EBNF Grammatik (v1.3.0)
│   ├── AQL_GRAMMAR.ebnf             # Vollständige formale Grammatik
│   └── README.md                    # AQL Übersicht
├── features/features_overview.md    # Feature-Liste mit Status
├── ROADMAP.md                       # Entwicklungs-Roadmap
├── CHANGELOG.md                     # Änderungshistorie
├── guides/guides_build_strategy.md  # Build-Toolchain & Strategie
├── INTEGRATION_ANALYSIS.md          # Enterprise Integration Analysis
├── TEST_REPORT.md                   # Vollständiger Test-Report
├── DEVELOPMENT_AUDITLOG.md          # Entwicklungsstand & Audit
├── DOCKER_DEPLOYMENT.md             # Docker Deployment Guide (v1.3.0)
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
├── legal/                           # ⭐ Legal & Licensing (v1.3.0)
│   └── LICENSE_COMPATIBILITY_ANALYSIS.md  # Dependency License Analysis
├── architecture/                    # Architektur-Dokumentation
├── api/                            # API-Dokumentation
└── guides/                         # User Guides
```

## 🚀 Enterprise Features

### Dokumentation
| Dokument | Zweck | Zielgruppe |
|----------|-------|------------|
| [enterprise/README.md](enterprise/README.md) | Übersicht & Quick Start | Entwickler, DevOps |
| [enterprise/enterprise_scalability.md](enterprise/enterprise_scalability.md) | Feature-Details & Code-Beispiele | Entwickler |
| [enterprise/enterprise_http_pool.md](enterprise/enterprise_http_pool.md) | HTTP Client Implementation | Entwickler |
| [enterprise/enterprise_final_report.md](enterprise/enterprise_final_report.md) | Implementation Summary | Stakeholder |
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
- [architecture.md](architecture/architecture_overview.md) - System-Architektur Übersicht
- [storage/storage_rocksdb.md](storage/storage_rocksdb.md) - RocksDB Storage Layout
- [mvcc_design.md](architecture/architecture_mvcc.md) - MVCC Transaction Design
- [query_engine_aql.md](aql/aql_query_engine.md) - Query Engine & AQL

### Spezielle Features
- [geo/GEO_ARCHITECTURE.md](geo/geo_architecture.md) - Geo/Spatial Architecture
- [vector_ops.md](features/features_vector_ops.md) - Vector Operations & HNSW
- [content_pipeline.md](architecture/architecture_content_pipeline.md) - Content Processing Pipeline
- [search/hybrid_search_design.md](search/hybrid_search_design.md) - Hybrid Search

## 🔒 Security & Compliance

### Security
- [security/security_overview.md](security/security_overview.md) - Security Übersicht
- [encryption_strategy.md](security/security_encryption_strategy.md) - Verschlüsselungsstrategie
- [security/security_key_management.md](security/security_key_management.md) - Key Management
- [security/security_threat_model.md](security/security_threat_model.md) - Threat Model
- [security_hardening_guide.md](security/security_hardening.md) - Hardening Guide

### Compliance
- [compliance/compliance_dashboard.md](compliance/compliance_dashboard.md) - Executive Dashboard
- [compliance/compliance_dpia.md](compliance/compliance_dpia.md) - Datenschutz-Folgenabschätzung (DSGVO)
- [compliance/compliance_bcp_drp.md](compliance/compliance_bcp_drp.md) - Business Continuity & Disaster Recovery
- [compliance_audit.md](features/features_compliance_audit.md) - Compliance Audit
- [AUDIT_LOGGING.md](features/features_audit_logging.md) - Audit Logging

### PKI & eIDAS
- [pki_integration_architecture.md](security/security_pki_architecture.md) - PKI Integration
- [eidas_qualified_signatures.md](security/security_eidas.md) - eIDAS Signaturen
- [security/pki_rsa_integration.md](security/security_pki_rsa.md) - PKI RSA Integration

## 🛠️ Build & Deployment

### Build-Dokumentation
- [guides/guides_build_strategy.md](guides/guides_build_strategy.md) - Build-Strategie & Plattformen
- [guides/guides_build.md](guides/guides_build.md) - Detaillierte Build-Anleitung

### Deployment
- [deployment.md](guides/guides_deployment.md) - Deployment-Strategien
- [DOCKER_MULTI_ARCH_STRATEGY.md](deployment/deployment_docker_multiarch.md) - Multi-Arch Docker
- [docs/CI_CD_MULTIARCH.md](deployment/deployment_cicd_multiarch.md) - Multi-Arch CI/CD

### Platform-Specific
- [ARM_RASPBERRY_PI_BUILD.md](deployment/deployment_arm_build.md) - Raspberry Pi Build
- [ARM_BENCHMARKS.md](deployment/deployment_arm_benchmarks.md) - ARM Performance
- [RASPBERRY_PI_TUNING.md](deployment/deployment_raspberry_tuning.md) - Pi Tuning Guide

## 📊 Performance & Benchmarks

- [performance_benchmarks.md](performance/performance_benchmarks.md) - Performance Übersicht
- [compression_benchmarks.md](performance/performance_compression_benchmarks.md) - Kompression
- [encryption_metrics.md](security/security_encryption_metrics.md) - Verschlüsselung Performance
- [performance/ENTERPRISE_SCALABILITY_STRATEGY.md](enterprise/enterprise_scalability.md) - Enterprise Strategy

## 🔍 API & Query Language

### AQL (Advanced Query Language)
- [aql_syntax.md](aql/aql_syntax.md) - AQL Syntax
- [aql-hybrid-queries.md](aql/aql_hybrid_queries.md) - Hybrid Queries
- [aql_explain_profile.md](aql/aql_explain_profile.md) - EXPLAIN & PROFILE
- [recursive_path_queries.md](features/features_recursive_path.md) - Rekursive Pfade
- [temporal_graphs.md](features/features_temporal_graphs.md) - Temporale Graphen

### APIs
- [apis/openapi.md](apis/apis_openapi.md) - REST API & OpenAPI Spec
- [apis/contentfs_api.md](apis/apis_contentfs.md) - ContentFS API
- [apis/hybrid_search_api.md](apis/apis_hybrid_search.md) - Hybrid Search API

## 👥 Client SDKs

- [clients/javascript_sdk_quickstart.md](clients/clients_javascript_sdk.md) - JavaScript SDK
- [clients/python_sdk_quickstart.md](clients/clients_python_sdk.md) - Python SDK
- [clients/rust_sdk_quickstart.md](clients/clients_rust_sdk.md) - Rust SDK

## 📝 Development

### Guidelines
- [development/developers.md](development/developers.md) - Developer Guide
- [code_quality.md](guides/guides_code_quality.md) - Code Quality Pipeline
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution Guidelines

### Status & Planning
- [DEVELOPMENT_AUDITLOG.md](development/DEVELOPMENT_SUMMARY.md) - Development Audit
- [development/implementation_status.md](development/implementation_status.md) - Status
- [development/ROADMAP.md](development/ROADMAP.md) - Roadmap
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
- [![CI](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml)
- [![Security CI](https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_security_security-hardening-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_security_security-hardening-ci.yml)
- [![GPU CI](https://github.com/makr-code/ThemisDB/actions/workflows/06-infrastructure_gpu_gpu-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/06-infrastructure_gpu_gpu-ci.yml)

## 📋 Navigation nach Thema

### Source-Module (44 Module — vollständige Liste in [src/README.md](../../src/README.md))

| Modul | README | Source | Headers | LOC |
|-------|--------|--------|---------|-----|
| analytics | [docs/observability/README.md](observability/README.md) | 2 | 3 | 3,742 |
| cache | [docs/storage/README.md](storage/README.md) | 1 | 6 | 492 |
| cdc | [docs/cdc/README.md](README.md) | 1 | 1 | 510 |
| content | [docs/content/README.md](content/README.md) | 15 | 16 | 9,091 |
| geo | [docs/geo/README.md](geo/README.md) | 3 | 2 | 304 |
| governance | [docs/governance/README.md](governance/README.md) | 1 | 1 | 259 |
| index | [docs/search/README.md](search/README.md) | 11 | 12 | 14,629 |
| llm | [docs/llm/README.md](llm/README.md) | 2 | 2 | 679 |
| query | [docs/query/README.md](query/README.md) | 12 | 12 | 12,560 |
| replication | [docs/storage/README.md](storage/README.md) | 1 | 2 | 1,612 |
| security | [docs/security/README.md](security/README.md) | 16 | 16 | 8,138 |
| server | [docs/server/README.md](server/README.md) | 20 | 20 | 18,282 |
| sharding | [docs/sharding/README.md](README.md) | 19 | 21 | 12,278 |
| storage | [docs/storage/README.md](storage/README.md) | 10 | 9 | 4,591 |
| timeseries | [docs/timeseries/README.md](timeseries/README.md) | 8 | 7 | 2,767 |
| transaction | [docs/architecture/README.md](architecture/README.md) | 2 | 2 | 895 |

**Gesamt:** 124 Source-Dateien, 132 Header-Dateien, 90,829 LOC

**Audit-Report:** [SOURCE_CODE_AUDIT.md](development/SOURCE_CODE_AUDIT.md)

### Multi-Model Features
- **Graph:** [property_graph_model.md](features/features_property_graph.md), [graph_index.cpp.md](src/index/graph_index.cpp.md)
- **Geo/Spatial:** [GEO_ARCHITECTURE.md](geo/geo_architecture.md), [geo_acceleration_3d_games.md](geo/geo_acceleration_3d_games.md)
- **Time-Series:** [time_series.md](features/features_time_series.md), [timeseries/continuous_agg.cpp.md](src/timeseries/continuous_agg.cpp.md)
- **Document:** [content_pipeline.md](architecture/architecture_content_pipeline.md), [content/content_manager.cpp.md](src/content/content_manager.cpp.md)
- **Vector/Embedding:** [vector_ops.md](features/features_vector_ops.md), [gnn_embeddings.md](features/features_gnn_embeddings.md)

### Storage & Persistence
- **RocksDB:** [storage/storage_rocksdb.md](storage/storage_rocksdb.md), [storage/rocksdb_wrapper.cpp.md](src/storage/rocksdb_wrapper.cpp.md)
- **MVCC:** [mvcc_design.md](architecture/architecture_mvcc.md)
- **Transactions:** [transactions.md](features/features_transactions.md), [transaction/saga.cpp.md](src/transaction/saga.cpp.md)
- **Compression:** [compression_strategy.md](performance/performance_compression_strategy.md), [timeseries/gorilla.cpp.md](src/timeseries/gorilla.cpp.md)

### Search & Indexing
- **Fulltext:** [search/fulltext_api.md](search/fulltext_api.md), [search/stemming.md](search/stemming.md)
- **Hybrid Search:** [search/hybrid_search_design.md](search/hybrid_search_design.md)
- **Vector Search:** [vector_ops.md](features/features_vector_ops.md), [index/vector_index.cpp.md](src/index/vector_index.cpp.md)
- **Geo Indexing:** [geo/cpu_backend.cpp.md](src/geo/cpu_backend.cpp.md)

### Governance & PII
- **PII Detection:** [security/pii_detection.md](security/security_pii_detection.md), [pii_api.md](security/security_pii_api.md)
- **Policies:** [security/security_policies.md](security/security_policies.md), [governance/policy_engine.cpp.md](src/governance/policy_engine.cpp.md)
- **RBAC:** [rbac_authorization.md](guides/guides_rbac.md), [RBAC.md](guides/guides_rbac.md)
- **Retention:** [security/audit_and_retention.md](security/security_audit_retention.md)

## ⚠️ Deprecated / Archive

Veraltete oder abgelöste Dokumentation:
- [archive/](archive/) - Archivierte Dokumente
- [reports/](reports/) - Git Merge Reports

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

**Dokumentations-Status:** ✅ Konsolidiert (5. Dezember 2025)  
**Maintainer:** ThemisDB Team  
**Letzte Audit:** 5. Dezember 2025
