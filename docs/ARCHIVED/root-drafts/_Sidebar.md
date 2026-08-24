## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


> **⚠️ STATUS: STALE – Archivierungskandidat**
> Dieser Inhalt enthält veraltete TODO/FIXME/PLACEHOLDER-Marker und wird im nächsten Archiv-Run nach `docs/ARCHIVED/` verschoben.
> Bitte nicht als aktuelle Referenz nutzen. Inventar: [DOCS_INVENTORY_2026-Q3.md](Audit/DOCS_INVENTORY_2026-Q3.md)

---

## ThemisDB Dokumentation

**Version:** 1.8.0-rc1 | **Stand:** April 2026

---

### 📋 Schnellstart
- [Übersicht](index.md)
- [Home](home.md)
- [Dokumentations-Index](DOCUMENTATION_INDEX.md)
- [Quick Reference](DOCS_QUICKREF.md)
- [Toolchain: Build/Preview/Publish](README-DOCUMENTATION.md)

### 🚀 v1.8.0-rc1 Release
- [Release Notes](de/releases/RELEASE_NOTES_v1.8.0.md)
- [Changelog](../CHANGELOG.md)

### 📝 Recent Features
- [Geo: GeoJSON RFC 7946 + R-tree index](en/geo/README.md)
- [Scraper Plugin v1.1.0](../plugins/scraper/README.md)
- [Analytics: Forecasting Batch/Streaming (v1.9.0)](../src/analytics/CHANGELOG.md)
- [Auth: German eID Authenticator (v1.9.0)](../src/auth/CHANGELOG.md)
- [Query: ShardKey Routing (v1.9.0)](../src/query/CHANGELOG.md)

---

### 📚 Dokumentation
- [Sachstandsbericht 2025](THEMIS_SACHSTANDSBERICHT_2025.md)
- [Features](FEATURES.md)
- [Roadmap](ROADMAP.md)
- [Ecosystem Overview](ECOSYSTEM_OVERVIEW.md)
- [Strategische Übersicht](STRATEGIC_OVERVIEW.md)

---

### 🏗️ Architektur
- [Architektur Überblick](architecture.md)
- [Geo-Architektur](GEO_ARCHITECTURE.md)

---

### 🗄️ Basismodell
- [Base Entity & Keys](base_entity.md)
- [Pfad-Constraints](path_constraints.md)
- [Property Graph Modell](property_graph_model.md)

---

### 💾 Storage & MVCC
- [Geo/Relational Storage](storage/geo_relational_schema.md)
- [RocksDB Storage](storage/rocksdb_layout.md)
- [MVCC Design](mvcc_design.md)
- [Transaktionen](transactions.md)
- [Time-Series](time_series.md)
- [Memory Tuning](memory_tuning.md)
- [Chain of Thought Storage](chain_of_thought_storage.md)

---

### 📇 Indexe & Statistiken
- [Indexe](indexes.md)
- [Index-Statistiken & Wartung](index_stats_maintenance.md)
- [Index Backup](index_backup.md)
- [Cursor/Pagination](cursor_pagination.md)

---

### 🔍 Query & AQL
- [Query Engine & AQL](query_engine_aql.md)
- [AQL Syntax](aql_syntax.md)
- [Explain & Profile](aql_explain_profile.md)
- [Rekursive Pfadabfragen](recursive_path_queries.md)
- [Temporale Graphen](temporal_graphs.md)
- [Zeitbereichs-Abfragen](temporal_time_range_queries.md)
- [Semantischer Cache](semantic_cache.md)
- [Hybrid Queries (Phase 1.5)](hybrid-queries-phase1.5.md)
- [AQL Hybrid Queries](aql-hybrid-queries.md)
- [Hybrid Queries README](HYBRID_QUERIES_README.md)
- [Hybrid Query Benchmarks](HYBRID_QUERY_BENCHMARKS.md)
- [Subquery Quick Reference](SUBQUERY_QUICK_REFERENCE.md)
- [Subquery Implementation](SUBQUERY_IMPLEMENTATION_SUMMARY.md)

---

### 💰 Caching
- [Cache Invalidation Strategy](cache_invalidation_strategy.md)
- [Caching Data Structures](caching_data_structures.md)
- [Caching Lookup Patterns](caching_lookup_patterns.md)

---

### 📦 Content Pipeline
- [Content Pipeline](content_pipeline.md)
- [Architektur-Details](content_architecture.md)
- [Ingestion](content/ingestion.md)
- [JSON Ingestion Spec](ingestion/json_ingestion_spec.md)
- [Enterprise Ingestion Interface](ENTERPRISE_INGESTION_INTERFACE.md)
- [Geo-Processor Design](content/geo_processor_design.md)
- [Image-Processor Design](content/image_processor_design.md)

---

### 🔎 Suche
- [Hybrid Search Design](search/hybrid_search_design.md)
- [Fulltext API](search/fulltext_api.md)
- [Hybrid Fusion API](search/hybrid_fusion_api.md)
- [Stemming](search/stemming.md)
- [Performance Tuning](search/performance_tuning.md)
- [Migration Guide](search/migration_guide.md)
- [Future Work](search/future_work.md)
- [Pagination Benchmarks](search/pagination_benchmarks.md)

---

### ⚡ Performance & Benchmarks
- [Performance & Tuning](performance_benchmarks.md)
- [Kompression Benchmarks](compression_benchmarks.md)
- [Kompression Strategie](compression_strategy.md)
- [Encryption Metrics](encryption_metrics.md)

---

### 🏢 Enterprise Features
- [Enterprise README](enterprise/README.md)
- [Scalability Features](ENTERPRISE_SCALABILITY.md)
- [HTTP Client Pool](HTTP_CLIENT_POOL_COMPLETE.md)
- [Build Guide](ENTERPRISE_BUILD_GUIDE.md)
- [Implementation Status](ENTERPRISE_IMPLEMENTATION_STATUS.md)
- [Final Report](ENTERPRISE_FINAL_REPORT.md)
- [Integration Analysis](reports/INTEGRATION_ANALYSIS.md)
- [Enterprise Strategy](performance/ENTERPRISE_SCALABILITY_STRATEGY.md)

---

### ✅ Qualitätssicherung
- [Quality Assurance](quality_assurance.md)

---

### 🧮 Vektor & GNN
- [Vektor-Operationen](vector_ops.md)
- [GNN Embeddings](gnn_embeddings.md)
- [HNSW Persistenz](hnsw_persistence.md)

---

### 🌍 Geo Features
- [Geo 3D Games Acceleration](geo_acceleration_3d_games.md)
- [Geo Execution Plan](geo_execution_plan_over_blob.md)
- [Geo Feature Tiering](geo_feature_tiering.md)
- [Geo Research Report MVP](research_postgis_opensearch_h3s2_mvp.md)

---

### 🛡️ Sicherheit & Governance
- [Security Overview](security/overview.md)
- [RBAC & Authorization](rbac_authorization.md)
- [RBAC](RBAC.md)
- [Policies (MVP)](security/policies.md)

#### Authentication
- [JWT](auth/jwt.md)
- [Benutzerverwaltung](guides/guides_user_management.md)

#### Schlüsselverwaltung
- [Key Management](security/key_management.md)

#### Verschlüsselung
- [Verschlüsselungsstrategie](encryption_strategy.md)
- [Verschlüsselungsdeployment](encryption_deployment.md)
- [Spaltenverschlüsselung](column_encryption.md)
- [Encryption Next Steps](encryption_next_steps.md)
- [Multi-Party Encryption](multi_party_encryption.md)
- [Key Rotation Strategy](key_rotation_strategy.md)
- [Security Encryption Gap Analysis](security_encryption_gap_analysis.md)

#### TLS & Certificates
- [TLS Setup](TLS_SETUP.md)
- [Certificate Pinning](CERTIFICATE_PINNING.md)

#### PKI & Signatures
- [PKI Integration](pki_integration_architecture.md)
- [PKI Signatures](pki_signatures.md)
- [PKI RSA Integration](security/pki_rsa_integration.md)
- [eIDAS Qualified Signatures](eidas_qualified_signatures.md)

#### PII Detection
- [PII-Detection](security/pii_detection.md)
- [PII-Detection Engines](pii_detection_engines.md)
- [PII Engine Signing](pii_engine_signing.md)
- [PII API](pii_api.md)

#### Vault & HSM
- [Vault](VAULT.md)
- [HSM Integration](hsm_integration.md)

#### Audit & Compliance
- [Audit Logging](AUDIT_LOGGING.md)
- [Audit & Retention](security/audit_and_retention.md)
- [Compliance Audit](compliance_audit.md)
- [Compliance](compliance.md)
- [Extended Compliance Features](EXTENDED_COMPLIANCE_FEATURES.md)
- [Governance-Strategie](compliance_governance_strategy.md)
- [Compliance-Integration](compliance_integration.md)
- [Governance Usage](governance_usage.md)
- [Security/Compliance Review](security/security_compliance_review.md)

#### Security Audits
- [Threat Model](security/threat_model.md)
- [Security Hardening Guide](security_hardening_guide.md)
- [Security Audit Checklist](security_audit_checklist.md)
- [Security Audit Report](security_audit_report.md)
- [Security Implementation](SECURITY_IMPLEMENTATION_SUMMARY.md)

#### Gap Analysis
- [Competitive Gap Analysis](competitive_gap_analysis.md)

---

### 🚀 Deployment & Betrieb
- [Deployment](deployment.md)

#### Docker
- [Docker Build](DOCKER_BUILD.md)
- [Docker Status](DOCKER_STATUS.md)
- [QNAP Deployment](QNAP_DEPLOYMENT.md)

#### Observability
- [Tracing](tracing.md)
- [Prometheus Metrics](observability/prometheus_metrics.md)
- [Metrics](observability/metrics.md)

#### Change Data Capture
- [Change Data Capture](change_data_capture.md)
- [CDC](cdc.md)

#### Operations
- [Operations Runbook](operations_runbook.md)
- [Infrastructure Roadmap](infrastructure_roadmap.md)
- [Horizontal Scaling](horizontal_scaling_implementation_strategy.md)

---

### 💻 Entwicklung
- [Development README](development/README.md)
- [Code Quality Pipeline](code_quality.md)
- [Developers Guide](development/developers.md)
- [Cost Models](development/cost-models.md)
- [Todo Liste](development/todo.md)
- [Tool Todo](development/tool_todo.md)
- [Core Feature Todo](development/core_feature_todo.md)
- [Priorities](development/priorities.md)
- [Implementation Status](development/implementation_status.md)
- [Roadmap](development/ROADMAP.md)
- [Future Work](development/future_work.md)
- [Next Steps Analysis](development/NEXT_STEPS_ANALYSIS.md)
- [AQL LET Implementation](development/aql_let_implementation_guide.md)
- [Development Audit](DEVELOPMENT_AUDIT.md)
- [Sprint Summary (2025-11-17)](development/sprint_summary_2025-11-17.md)
- [WAL Archiving](development/wal-archiving.md)
- [Search Gap Analysis](development/search_gap_analysis.md)
- [Source Documentation Plan](development/src_documentation_plan.md)

#### API Implementations
- [Audit API](development/audit_api_implementation.md)
- [SAGA API](development/saga_api_implementation.md)
- [Code Audit Mockups/Stubs](development/code_audit_mockups_stubs.md)

#### Changefeed
- [Changefeed README](development/changefeed/README.md)
- [Changefeed CMake Patch](development/changefeed/changefeed_cmake_patch.md)
- [Changefeed OpenAPI](development/changefeed/changefeed_openapi.md)
- [Changefeed OpenAPI Auth](development/changefeed/changefeed_openapi_auth.md)
- [Changefeed SSE Examples](development/changefeed/changefeed_sse_examples.md)
- [Changefeed Test Harness](development/changefeed/changefeed_test_harness.md)
- [Changefeed Tests](development/changefeed/changefeed_tests.md)

#### Security Development
- [Security README](development/security/README.md)
- [Content ZSTD HKDF](development/security/content_zstd_hkdf.md)
- [PKI-eIDAS](development/pki-eidas.md)

#### Development Overviews
- [Overview README](development/overviews/README.md)
- [Consolidated Development Overview](development/overviews/consolidated_development_overview.md)
- [Feature Status](development/overviews/feature_status_changefeed_encryption.md)
- [Verification by Area](development/overviews/verification_by_area.md)

---

### 📄 Publikation & Ablage
- [Publishing](publishing.md)

---

### 🔧 Admin-Tools
- [Admin Guide](admin_tools_admin_guide.md)
- [User Guide](admin_tools_user_guide.md)
- [Feature Matrix](admin_tools_feature_matrix.md)
- [Suche/Sortierung/Filter](admin_tools_search_sort_filter.md)
- [Demo-Script](admin_tools_demo_script.md)

---

### 🔌 APIs
- [OpenAPI & Endpunkte](apis/openapi.md)
- [ContentFS API](apis/contentfs_api.md)
- [Hybrid Search API](apis/hybrid_search_api.md)

---

### 📚 Client SDKs
- [JavaScript SDK](clients/javascript_sdk_quickstart.md)
- [Python SDK](clients/python_sdk_quickstart.md)
- [Rust SDK](clients/rust_sdk_quickstart.md)

---

### 📊 Implementierungs-Zusammenfassungen
- [ThemisDB Implementation](THEMIS_IMPLEMENTATION_SUMMARY.md)
- [Database Capabilities Roadmap](DATABASE_CAPABILITIES_ROADMAP.md)
- [Release Scope Core](release_scope_core.md)

---

### 📅 Planung & Reports
- [Phase 1.5 Completion Report](PHASE_1.5_COMPLETION_REPORT.md)
- [Phase 2 Plan](PHASE_2_PLAN.md)
- [Phase 3 Plan](PHASE_3_PLAN.md)
- [Phase 4 Plan](PHASE_4_PLAN.md)
- [Sprint A Plan](sprint_a_plan.md)

---

### 📖 Dokumentation
- [Dokumentations-Inventar](_inventory.md)
- [Documentation Summary](DOCUMENTATION_SUMMARY.md)
- [Documentation TODO](DOCUMENTATION_TODO.md)
- [Documentation Gap Analysis](DOCUMENTATION_GAP_ANALYSIS.md)
- [Documentation Consolidation](DOCUMENTATION_CONSOLIDATION_PLAN.md)
- [Documentation Final Status](DOCUMENTATION_FINAL_STATUS.md)
- [Documentation Phase 3](DOCUMENTATION_PHASE3_REPORT.md)
- [Documentation Cleanup Validation](DOCUMENTATION_CLEANUP_VALIDATION_REPORT.md)

---

### 📝 Release Notes
- [AQL Fulltext](RELEASE_NOTES_AQL_FULLTEXT.md)
- [Temporal Aggregation (2025-11-11)](release_notes/2025-11-11-temporal-aggregation.md)

---

### 📖 Styleguide & Glossar
- [Styleguide](styleguide.md)
- [Glossar](glossary.md)

---

### 🗺️ Roadmap & Changelog
- [Roadmap](ROADMAP.md)
- [Changelog](changelog.md)

---

### 💾 Source Code Documentation
- [Source Root README](src/root_README.md)
- [Source Documentation](src/README.md)

#### Main Programs
- [Main](src/main.cpp.md)
- [Main (Detailed)](src/main.cpp/main.cpp.md)
- [Main Server](src/main_server.cpp.md)
- [Main Server (Detailed)](src/main_server.cpp/main_server.cpp.md)
- [Demo Encryption](src/demo_encryption.cpp.md)
- [Demo Encryption (Detailed)](src/demo_encryption.cpp/demo_encryption.cpp.md)

#### Source Code Module
- [API](src/api/README.md)
- [Authentication](src/auth/README.md)
- [Cache](src/cache/README.md)
- [CDC](src/cdc/README.md)
- [Content](src/content/README.md)
- [Geo](src/geo/README.md)
- [Governance](src/governance/README.md)
- [Index](src/index/README.md)
- [LLM](src/llm/README.md)
- [Query](src/query/README.md)
- [Security](src/security/README.md)
- [Server](src/server/README.md)
- [Storage](src/storage/README.md)
- [Time Series](src/timeseries/README.md)
- [Transaction](src/transaction/README.md)
- [Utils](src/utils/README.md)

---

### 🗄️ Archive
- [Archive README](archive/README.md)
- [CDC Legacy](archive/cdc_legacy.md)
- [Geo Research Report MVP](archive/geo_research_report_mvp.md)
- [Path Constraints Concept](archive/path_constraints_concept.md)
- [Release Scope Core Draft](archive/release_scope_core_draft.md)

---

### 🤝 Community & Support
- [GitHub Repository](https://github.com/makr-code/ThemisDB)
- [Contributing](CONTRIBUTING.md)
- [Security Policy](SECURITY.md)

---

**Vollständige Dokumentation:** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
