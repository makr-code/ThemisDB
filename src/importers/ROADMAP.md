# Importers Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Production-ready multi-source import pipeline. PostgreSQL, MySQL/MariaDB, MongoDB, Oracle, SQLite, Kafka, S3/flat-file importers implemented. Plugin API, streaming, conflict resolution, and schema auto-detection complete.

## Completed ✅
- [x] PostgreSQL importer
- [x] Schema mapping and transformation
- [x] Batch import operations
- [x] Incremental import support
- [x] Custom import format handlers
- [x] Import pipeline infrastructure
- [x] MySQL / MariaDB importer (`importers/mysql_importer.cpp`) (Issue: #1835)
- [x] MongoDB importer for document collections (`importers/mongo_importer.cpp`) (Issue: #1836)
- [x] Import progress reporting with streaming callbacks (Issue: #1864)
- [x] SQLite importer (`importers/sqlite_importer.cpp`) (Issue: #1838)
- [x] CSV / TSV / Parquet flat-file importer (`importers/flatfile_importer.cpp`) (Issue: #1839)
- [x] Schema auto-detection and validation on import (`importers/schema_validator.cpp`) (Issue: #1856)
- [x] Kafka consumer importer (`importers/kafka_importer.cpp`) (Issue: #1843)
- [x] Oracle Database importer (`importers/oracle_importer.cpp`) (Issue: #1844)
- [x] Plugin API for third-party importer extensions (`importers/importer_plugin_api.h`) (Issue: #1854)
- [x] S3-compatible object-storage source connector (`importers/s3_importer.cpp`) (Issue: #1855)

## In Progress 🚧
*(none currently in progress)*

## Completed ✅ (additional)
- [x] MySQL / MariaDB importer (`importers/mysql_importer.cpp`) with JDBC-compatible config (Issue: #1835)
- [x] MongoDB importer for document collections (Issue: #1836)
- [x] SQLite importer (`importers/sqlite_importer.cpp`) (Issue: #1838)
- [x] CSV / TSV / Parquet flat-file importer (`importers/flatfile_importer.cpp`) with schema auto-detection (Issue: #1839)
- [x] Kafka consumer importer for real-time streaming ingestion (`importers/kafka_importer.cpp`) (Issue: #1843)
- [x] Oracle Database importer (`importers/oracle_importer.cpp`) (Issue: #1844)
- [I] Import progress reporting with streaming callbacks (Target: Q3 2026) (Issue: #1864)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] SQLite importer (`importers/sqlite_importer.cpp`) (Issue: #1838)
- [x] CSV / TSV / Parquet flat-file importer (`importers/flatfile_importer.cpp`) (Issue: #1839)
- [x] Schema auto-detection and validation on import (Issue: #1856)

### Long-term (6-12 months)
- [x] Kafka consumer importer for real-time streaming ingestion (`importers/kafka_importer.cpp`) (Issue: #1843)
- [x] Oracle Database importer (`importers/oracle_importer.cpp`) (Issue: #1844)
- [I] Microsoft SQL Server importer (Issue: #1845)
- [~] GUI-based import wizard (web UI) (Issue: #1847)

## Implementation Phases

### Phase 1: Core PostgreSQL Importer (Status: Completed ✅)
- [x] PostgreSQL importer (`importers/postgres_importer.cpp`) with connection pooling
- [x] Schema mapping and field-type transformation layer
- [x] Batch import operations with configurable chunk size
- [x] Incremental import support (watermark-based change tracking)
- [x] Custom import format handler registration API
- [x] Import pipeline infrastructure (source → transform → sink)

### Phase 2: Streaming & Conflict Resolution (Status: Completed ✅)
- [x] Streaming import for large datasets without full in-memory load (Target: Q2 2026) (Issue: #1863)
- [x] Import progress reporting with streaming callbacks (Target: Q2 2026)
- [x] Conflict resolution strategies: skip, overwrite, merge (Target: Q3 2026) (Issue: #1849)
- [x] Dry-run mode to preview import without writing data (Target: Q3 2026)

### Phase 3: Multi-Source & Plugin API (Status: Completed ✅)
- [x] MySQL / MariaDB importer (`importers/mysql_importer.cpp`) with JDBC-compatible config (Issue: #1851)
- [x] MongoDB importer (`importers/mongo_importer.cpp`) for document collections (Issue: #1852)
- [x] Flat-file CSV / TSV / Parquet importer with schema auto-detection (`importers/flatfile_importer.cpp`) (Issue: #1853)
- [x] Plugin API for third-party importer extensions (`importers/importer_plugin_api.h`) (Issue: #1854)
- [x] S3-compatible object-storage source connector (`importers/s3_importer.cpp`) (Issue: #1855)
- [x] Schema auto-detection and validation on import (`importers/schema_validator.cpp`) (Issue: #1856)
- [x] SQLite importer (`importers/sqlite_importer.cpp`) (Issue: #1838)
- [x] Kafka consumer importer (`importers/kafka_importer.cpp`) (Issue: #1843)
- [x] Oracle Database importer (`importers/oracle_importer.cpp`) (Issue: #1844)

### Phase 4: Abstract Interface Contracts (Status: Completed ✅)
- [x] `IImportConflictResolver` – stateless conflict-resolution interface (`include/importers/importer_interfaces.h`)
- [x] `IFlatFileSchemaDetector` – advisory flat-file schema detection interface with `SchemaConfidence`
- [x] `IKafkaConsumerSource` – async Kafka consumer source interface with explicit offset commit
- [x] `IIncrementalImportCursor` – pull-based resumable cursor with `CheckpointToken`
- [x] `IImporterPlugin` – URI-scheme-based plugin entry point (`pluginId()`, `supportedSchemes()`, `createImporter()`)
- [x] `IImporterPluginRegistry` – scheme-keyed plugin registry with `resolve(uri)` returning `nullptr` for unknown schemes
- [x] `REGISTER_IMPORTER_PLUGIN` macro – static-init plugin registration
- [x] Unit tests for all abstract interfaces (`tests/test_importer_interfaces.cpp`, 42 tests)

### Phase 5: Build System Audit (Status: Completed ✅)
- [x] All 11 `src/importers/*.cpp` files registered in `cmake/CMakeLists.txt` (`s3_importer.cpp` gated by `THEMIS_ENABLE_S3`)
- [x] All 11 `src/importers/*.cpp` files registered in `cmake/ModularBuild.cmake` (`THEMIS_QUERY_SOURCES`; `s3_importer.cpp` gated by `THEMIS_ENABLE_S3`)
- [x] Focused standalone test targets added in `tests/CMakeLists.txt`: FlatfileImporterFocusedTests, SchemaValidatorImporterFocusedTests, ImporterConflictResolverFocusedTests, ImporterAsyncApiFocusedTests, MySQLImporterFocusedTests, MongoImporterFocusedTests, SQLiteImporterFocusedTests, KafkaImporterFocusedTests, OracleImporterFocusedTests, S3ImporterFocusedTests, PostgresImporterFocusedTests, ImportWizardFocusedTests

### Phase 6: MDM & Entity Deduplication (Status: Completed ✅)

**Master Data Management** – automated entity matching, linking, and golden
record creation for imported PostgreSQL data.

**Files added:**
- `include/importers/entity_matcher.h` – DeterministicMatcher, SemanticMatcher, HybridEntityMatcher
- `include/importers/entity_linker.h` – EntityLinker, EntityLink, LinkAuditEntry
- `include/importers/canonical_resolver.h` – CanonicalEntityResolver, GoldenRecord
- `include/importers/mdm_engine.h` – MDMEngine, MDMConfig, MDMWorkflowResult
- `include/importers/mdm_audit_trail.h` – MDMAuditTrail, AuditEvent (chain-linked)
- `include/importers/mdm_metrics.h` – MDMMetrics, MDMMetricSnapshot
- `src/importers/deterministic_matcher.cpp`
- `src/importers/semantic_matcher.cpp`
- `src/importers/entity_linker.cpp`
- `src/importers/canonical_resolver.cpp`
- `src/importers/mdm_engine.cpp`
- `src/importers/mdm_audit_trail.cpp`
- `src/importers/mdm_metrics.cpp`

**Tests (132 tests across 5 files, 100% pass):**
- `tests/test_entity_matching.cpp` (40 tests)
- `tests/test_entity_linking.cpp` (20 tests)
- `tests/test_canonical_resolver.cpp` (35 tests)
- `tests/test_mdm_engine.cpp` (12 tests)
- `tests/test_mdm_integration.cpp` (9 tests)

**Focused test targets:** MDMDeterministicMatcherFocusedTests, MDMSemanticMatcherFocusedTests, MDMEntityLinkerFocusedTests, MDMCanonicalResolverFocusedTests, MDMEngineFocusedTests, MDMIntegrationFocusedTests

**Documentation:**
- `docs/importers/MDM_USER_GUIDE.md`
- `docs/importers/MDM_ARCHITECTURE.md`
- `docs/importers/MDM_API_REFERENCE.md`

**Key capabilities:**
- [x] Deterministic matching (PK, unique fields, custom identifier mappings)
- [x] Semantic matching (Jaro-Winkler, Levenshtein, Soundex, email, phone, cosine)
- [x] Hybrid ensemble strategy (configurable weights)
- [x] Bidirectional entity linking with 8 typed link types
- [x] Golden record creation with 6 resolution policies
- [x] Field-level conflict resolution with per-field rules
- [x] Protected fields (entity[0]'s original values always preserved)
- [x] Immutable audit trail with FNV-1a chain hash
- [x] Dry-run mode (no persistent storage, zero links_created)
- [x] Prometheus-style metrics + JSON dashboard export
- [x] All 18 new source files registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1857)
- [x] Integration tests against live PostgreSQL (Issue: #1858)
- [I] Performance benchmarks (rows/sec, GB/hr) (Issue: #1859)
- [I] Security audit (SQL injection, credential handling) (Issue: #1860)
- [I] Documentation complete (Issue: #1861)
- [I] API stability guaranteed (Issue: #1862)

## Known Issues & Limitations
- MySQL/Oracle importers require the respective client library at link time (`THEMIS_ENABLE_MYSQL`, `THEMIS_ENABLE_OCI`); builds without those flags return `CONNECTOR_NOT_SUPPORTED`.
- Kafka importer requires `THEMIS_ENABLE_KAFKA` and librdkafka at link time; compiles cleanly without it but every `importData()` call returns an error.
- Binary/blob field types may require manual mapping.
- No distributed parallel import across multiple nodes.
- GUI-based import wizard is planned but not yet implemented (Issue: #1847).
- Microsoft SQL Server importer is planned (Issue: #1845).

## Breaking Changes
- Importer plugin API will be stabilised in v1.5.0; breaking changes expected before that milestone.
