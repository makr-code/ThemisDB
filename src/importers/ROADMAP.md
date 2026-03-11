# Importers Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v2.0 – Production-ready multi-source import pipeline with Foreign Key Preservation. PostgreSQL importer upgraded to v2.0: automatically extracts and preserves FOREIGN KEY constraints (table-level with ON DELETE/ON UPDATE, inline column REFERENCES, deferred ALTER TABLE ADD CONSTRAINT), embeds FK metadata in entity JSON, and exposes FK info through getSourceSchema(). All prior features retained.

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
- [x] **v2.0 Foreign Key Preservation** – `ImportOptions::preserve_foreign_keys`, `ImportStats::foreign_keys_preserved`, FK metadata in `getSourceSchema()` and entity JSON

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
- v2.0: `getSourceSchema()` now returns a `foreign_keys` array in each table entry (previously absent). Consumers that relied on the exact key set must be updated.
- v2.0: Entity JSON now contains `_foreign_keys` metadata key when the source table has FK constraints and `preserve_foreign_keys=true`. Consumers must handle or ignore this new key.


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
