> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Importers Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v2.0 – Production-ready multi-source import pipeline with Foreign Key Preservation. PostgreSQL importer upgraded to v2.0: automatically extracts and preserves FOREIGN KEY constraints (table-level with ON DELETE/ON UPDATE, inline column REFERENCES, deferred ALTER TABLE ADD CONSTRAINT), embeds FK metadata in entity JSON, and exposes FK info through getSourceSchema(). All prior features retained.
v2.1 – PostgreSQL Importer enhanced with FK preservation (v2.0), relationship mapping, extended constraints (CHECK/EXCLUDE/GENERATED), and performance optimizations. All other importers (MySQL/MariaDB, MongoDB, Oracle, SQLite, Kafka, S3/flat-file) remain at v1.x production-ready state.
v2.1+ – ML-assisted schema intelligence, enterprise-grade data quality/audit, and
emerging-tech integrations (federated learning, blockchain integrity, GraphQL federation)
implemented on top of the production-ready v1.x multi-source import pipeline.

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
- [x] **v2.0: Foreign Key Preservation** – automatic extraction of FK constraints from pg_dump files with ON DELETE/ON UPDATE action support, inline REFERENCES, and ALTER TABLE ADD CONSTRAINT support
- [x] PostgreSQL FK preservation v2.0 (`ForeignKeyConstraint`, inline `REFERENCES`, `ALTER TABLE…FK`, DEFERRABLE)
- [x] PostgreSQL relationship mapping v2.1: bidirectional edges (`generateInverseEdges`), `on_delete/update` propagation, self-referential edge naming
- [x] PostgreSQL additional constraint types v2.1: `CheckConstraint`, `ExcludeConstraint`, `GeneratedColumnInfo`
- [x] PostgreSQL parse performance v2.1: static `std::regex`, `unordered_map` lookups, `append`-based string building, `thread_local` `streamReadLine`, pre-reserved buffers
<!-- v2.1+ modules -->
- [x] Schema Inference Engine – implicit FK discovery, semantic type detection, cardinality estimation (`importers/schema_inference.cpp`)
- [x] Column Importance Analyzer – Shannon entropy, Gini impurity, information gain (`importers/column_importance.cpp`)
- [x] CRDT Importer – conflict-free parallel import with LWW merge (`importers/crdt_importer.cpp`)
- [x] PostgreSQL CDC – logical decoding interface for Change Data Capture (`importers/postgres_cdc.cpp`)
- [x] Data Quality Framework – NIST SP 800-188 completeness/accuracy/validity scoring (`importers/data_quality.cpp`)
- [x] Audit Trail – SOX-compliant Merkle-chained immutable audit log (`importers/audit_trail.cpp`)
- [x] Adaptive Import Optimizer – Kahn's topological sort + runtime batch-size adaptation (`importers/adaptive_import.cpp`)
- [x] Polyglot Persistence Mapper – recommend RELATIONAL/DOCUMENT/GRAPH/TIMESERIES/KEYVALUE/VECTORSPACE (`importers/polyglot_mapper.cpp`)
- [x] Temporal Database Support – SQL:2011 temporal detection + point-in-time query builder (`importers/temporal_support.cpp`)
- [x] Blockchain Integrity Verifier – SHA-256 Merkle tree + offline blockchain anchor (`importers/blockchain_integrity.cpp`)
- [x] Federated Learning Coordinator – FedAvg aggregation + Gaussian differential privacy (`importers/federated_learning.cpp`)
- [x] GraphQL Federation Support – Apollo Federation v2 SDL generation (`importers/graphql_federation.cpp`)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] SQLite importer (`importers/sqlite_importer.cpp`) (Issue: #1838)
- [x] CSV / TSV / Parquet flat-file importer (`importers/flatfile_importer.cpp`) (Issue: #1839)
- [x] Schema auto-detection and validation on import (Issue: #1856)
- [x] **v2.0 Foreign Key Preservation** – `ImportOptions::preserve_foreign_keys`, `ImportStats::foreign_keys_preserved`, FK metadata in `getSourceSchema()` and entity JSON

### Long-term (6-12 months)
- [x] Kafka consumer importer for real-time streaming ingestion (`importers/kafka_importer.cpp`) (Issue: #1843)
- [x] Oracle Database importer (`importers/oracle_importer.cpp`) (Issue: #1844)
- [I] Microsoft SQL Server importer (Issue: #1845)
- [x] GUI-based import wizard (web UI) (`importers/gui_import_wizard.cpp`) (Issue: #1847)

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

### Phase 6: v2.0 Foreign Key Preservation (Status: Completed ✅)
- [x] `TableSchema::ForeignKeyConstraint` inner struct with `constraint_name`, `columns`, `ref_table`, `ref_columns`, `on_delete`, `on_update` fields and `toJson()`
- [x] `TableSchema::foreign_keys` vector populated by `parseCreateTable()` – handles table-level `FOREIGN KEY` constraints with optional `CONSTRAINT name`
- [x] `parseInlineReference()` – extracts inline `col TYPE REFERENCES tbl(col) [ON DELETE …]` column-level references
- [x] `parseForeignKeyConstraint()` – parses a single FK clause including ON DELETE / ON UPDATE actions (CASCADE, SET NULL, SET DEFAULT, RESTRICT, NO ACTION)
- [x] `parseAlterTableAddFk()` – handles deferred `ALTER TABLE … ADD CONSTRAINT … FOREIGN KEY` statements
- [x] `ImportOptions::preserve_foreign_keys` (default `true`) – enables/disables FK extraction
- [x] `ImportStats::foreign_keys_preserved` – counts extracted FK constraints per import run
- [x] `getSourceSchema()` returns `foreign_keys` JSON array in each table entry
- [x] `convertRowToEntity()` embeds `_foreign_keys` metadata in entity JSON when schema has FKs
- [x] New fixture `tests/fixtures/importers/sample_pg_fk.sql` covering all FK patterns
- [x] New test file `tests/test_postgres_importer_fk.cpp` (30+ tests) added to `PostgresImporterFocusedTests`

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1857)
- [x] Integration tests against live PostgreSQL (Issue: #1858)
- [I] Performance benchmarks (rows/sec, GB/hr) (Issue: #1859)
- [I] Security audit (SQL injection, credential handling) (Issue: #1860)
- [x] Documentation complete (Issue: #1861)
- [I] API stability guaranteed (Issue: #1862)

## Known Issues & Limitations
- MySQL/Oracle importers require the respective client library at link time (`THEMIS_ENABLE_MYSQL`, `THEMIS_ENABLE_OCI`); builds without those flags return `CONNECTOR_NOT_SUPPORTED`.
- Kafka importer requires `THEMIS_ENABLE_KAFKA` and librdkafka at link time; compiles cleanly without it but every `importData()` call returns an error.
- Binary/blob field types may require manual mapping.
- No distributed parallel import across multiple nodes.
- Microsoft SQL Server importer is planned (Issue: #1845).

## Breaking Changes
- Importer plugin API will be stabilised in v1.5.0; breaking changes expected before that milestone.
- v2.0: `getSourceSchema()` now returns a `foreign_keys` array in each table entry (previously absent). Consumers that relied on the exact key set must be updated.
- v2.0: Entity JSON now contains `_foreign_keys` metadata key when the source table has FK constraints and `preserve_foreign_keys=true`. Consumers must handle or ignore this new key.

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

### Phase 6: ML Foundations – v2.1 (Status: Completed ✅, Target: Q2 2026)
- [x] `SchemaInferenceEngine::inferImplicitRelationships()` – Jaccard FK discovery (Target: Q2 2026)
- [x] `SchemaInferenceEngine::detectSemanticTypes()` – regex-based semantic type detection for EMAIL/UUID/IP/CURRENCY/… (Target: Q2 2026)
- [x] `SchemaInferenceEngine::estimateCardinalities()` – Harmonic Mean + Wilson CI (Target: Q2 2026)
- [x] `ColumnImportanceAnalyzer::analyzeImportance()` – Shannon entropy, Gini impurity, ID3 information gain (Target: Q2 2026)
- [x] `ColumnImportanceAnalyzer::findRedundantColumns()` – Pearson correlation-based redundancy detection (Target: Q2 2026)
- [x] `CRDTTableState::importWithCRDT()` – LWW CRDT with Lamport + wall-clock timestamps (Target: Q2 2026)
- [x] 12 test suites (72 tests) in `tests/test_postgres_importer_v2.cpp`; `PostgresImporterV2FocusedTests` CTest target (Target: Q2 2026)
- [x] Research docs: `docs/research/SCHEMA_INFERENCE_ALGORITHM.md`, `docs/research/CARDINALITY_ESTIMATION.md` (Target: Q2 2026)

### Phase 7: Enterprise Features – v2.2 (Status: Completed ✅, Target: Q3 2026)
- [x] `PostgreSQLCDC::LogicalDecoder` – logical decoding interface (pgoutput protocol); publication + slot lifecycle (Target: Q3 2026)
- [x] `DataQualityFramework::QualityAssessor::assessTable()` – NIST SP 800-188 six-dimension scoring (Target: Q3 2026)
- [x] `AuditedImporter::ImmutableAuditLog` – Merkle-chained SOX-compliant audit log with SIEM export (Splunk/ELK) (Target: Q3 2026)
- [ ] Live libpq replication protocol in `postgres_cdc.cpp` (requires `THEMIS_ENABLE_CDC`) (Target: Q3 2026)

### Phase 8: Advanced Patterns – v2.3 (Status: Completed ✅, Target: Q4 2026)
- [x] `AdaptiveImportOptimizer::optimizeImportPlan()` – Kahn's topological sort respecting FK deps (Target: Q4 2026)
- [x] `AdaptiveImportOptimizer::adaptBatchSize()` – runtime CPU/MEM/IO adaptive batch sizing (Target: Q4 2026)
- [x] `AdaptiveImportOptimizer::PerformancePredictor::predictPerformance()` – linear time/memory/IO model (Target: Q4 2026)
- [x] `PolyglotPersistenceMapper::recommendDataModels()` – heuristic RELATIONAL/DOCUMENT/GRAPH/TIMESERIES/KEYVALUE/VECTORSPACE recommendation (Target: Q4 2026)
- [x] `PolyglotPersistenceMapper::ModelTransformer::tableToDocument()` and `tableToGraph()` (Target: Q4 2026)
- [x] `TemporalDatabaseSupport::detectTemporalDimensions()` – SQL:2011 VALID_TIME/TRANSACTION_TIME/BI_TEMPORAL detection (Target: Q4 2026)
- [x] `TemporalDatabaseSupport::TemporalQueryBuilder::buildPointInTimeQuery()` (Target: Q4 2026)
- [x] Research docs: `docs/research/TEMPORAL_DATABASE_SUPPORT.md` (Target: Q4 2026)

### Phase 9: Emerging Technologies – v3.0 (Status: Completed ✅, Target: 2027)
- [x] `BlockchainIntegrityVerifier::MerkleTreeBuilder::buildMerkleTree()` – SHA-256 Merkle tree (Target: 2027)
- [x] `BlockchainIntegrityVerifier::BlockchainAnchor::anchorToBlockchain()` – offline + production anchor (Target: 2027)
- [x] `FederatedImportCoordinator::FederatedAggregator::aggregateUpdates()` – FedAvg + coordinate-wise median (Target: 2027)
- [x] `FederatedImportCoordinator::DifferentialPrivacyManager` – Gaussian mechanism ε-δ DP with budget tracking (Target: 2027)
- [x] `GraphQLFederationSupport::GraphQLSchemaGenerator::generateFederatedSchema()` – Apollo Federation v2 SDL (Target: 2027)
- [x] Research docs: `docs/research/FEDERATED_LEARNING_DESIGN.md`, `docs/research/BLOCKCHAIN_VERIFICATION.md` (Target: 2027)
- [ ] Quantum-safe cryptography (NIST PQC) for audit trail signatures (Target: 2027)
- [ ] Zero-Knowledge Proofs for privacy-preserving data validation (Target: 2027)

### Phase 10: Stable Plugin ABI – v1.9.0 (Status: Completed ✅)
- [x] Stable C-linkage plugin ABI in `include/importers/importer_plugin.h`; `THEMIS_IMPORTER_PLUGIN_V1` versioned struct (Target: v1.9.0)
- [x] `ImporterRegistry::loadPlugin(path)` and `unloadPlugin(name)` using `themis_importer_create` (Target: v1.9.0)
- [x] `PluginSandboxConfig` — per-job memory limit + timeout enforcement via dedicated thread (Target: v1.9.0)
- [x] `V1ImporterAdapter` — `IImporter` wrapper for V1 plugins with sandbox allocator callbacks (Target: v1.9.0)
- [x] `ImporterRegistry` typedef alias for `ImporterPluginRegistry` (Target: v1.9.0)
- [x] Oracle importer skeleton + V1 ABI docs in `docs/importers/plugin_guide.md` (Target: v1.9.0)
- [x] New tests in `tests/test_importer_plugin_api.cpp` covering V1 ABI, loadPlugin error paths, sandbox config, `V1ImporterAdapter` lifecycle (Target: v1.9.0)

## Production Readiness Checklist
- [x] All 12 v2.1+ modules designd & documented (research docs in `docs/research/`)
- [I] Unit tests coverage > 80% for v2.1+ modules (Issue: #1857)
- [x] Integration tests against live PostgreSQL (Issue: #1858)
- [I] Performance benchmarks (rows/sec, GB/hr) vs. Apache NiFi / Talend (Issue: #1859)
- [I] Security audit (SQL injection, credential handling) (Issue: #1860)
- [x] Documentation complete for v2.1+ modules (Issue: #1861)
- [I] API stability guaranteed (Issue: #1862)

## Known Issues & Limitations
- MySQL/Oracle importers require the respective client library at link time (`THEMIS_ENABLE_MYSQL`, `THEMIS_ENABLE_OCI`); builds without those flags return `CONNECTOR_NOT_SUPPORTED`.
- Kafka importer requires `THEMIS_ENABLE_KAFKA` and librdkafka at link time; compiles cleanly without it but every `importData()` call returns an error.
- Binary/blob field types may require manual mapping.
- `postgres_cdc.cpp` LogicalDecoder provides the interface contract; the live libpq replication stream requires `THEMIS_ENABLE_CDC`.
- `blockchain_integrity.cpp` uses `std::hash` by default; enable `THEMIS_ENABLE_OPENSSL` for FIPS 140-2 compliant SHA-256.
- Microsoft SQL Server importer is planned (Issue: #1845).

## Breaking Changes
- Importer plugin API will be stabilised in v1.5.0; breaking changes expected before that milestone.
- v2.1 introduced `InferenceTableSchema` (shared by schema_inference, adaptive_import, polyglot_mapper, temporal_support, graphql_federation); include `importers/schema_inference.h` for the type definition.
- `SchemaInferenceConfig` is now a free struct (not a nested struct of `SchemaInferenceEngine`) for C++17 default-argument compatibility.
- `CDCOptions` is now a free struct (not a nested struct of `PostgreSQLCDC`) for the same reason.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### ✅ Aktiv — Security-Fix 2026-04-20

- `computeEventHash` – Berechnet kryptographischen Hash für Audit-Trail-Event-Chaining.
  **Implementierungsstatus (ab 2026-04-20):** Vollständig implementiert. Verwendet OpenSSL
  `EVP_DigestInit_ex / EVP_DigestUpdate / EVP_DigestFinal_ex` mit SHA-256. Ausgabe: 64-stelliger
  Hex-String (voller SHA-256). `std::hash`-Placeholder und `// STUB/SIMULATION NOTE:`-Kommentar
  wurden entfernt. Audit-Log-Kette ist damit kollisionsresistent und fälschungssicher.

