> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Importers Module - Future Enhancements

This document covers planned enhancements to the Importers module beyond what is tracked in `ROADMAP.md`. It focuses on `postgres_importer.cpp` and the surrounding import pipeline infrastructure. Features here describe the engineering work required to add additional source connectors (MySQL, MongoDB, SQLite, flat files, S3, Kafka), a plugin API for third-party importers, and production-hardening of the existing PostgreSQL importer including distributed parallel import and conflict resolution strategies.

## Design Constraints

- All importer implementations must go through a common `IImporter` interface so that the import pipeline, schema validation, and conflict resolution logic can be shared without duplicating code across connectors.
- Credentials (passwords, connection strings, API keys) must never be logged, included in error messages, or stored in checkpoint files; only sanitized connection identifiers are permitted in observability output.
- Batch import must be resumable: if the process is killed mid-import, a restart must continue from the last committed checkpoint without duplicating already-imported rows.
- Schema mapping is the importer's responsibility; the storage layer receives documents that already conform to the target ThemisDB collection schema.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IImporter::importBatch(offset, limit)` | `ImportPipeline` orchestrator | Must be implemented by every connector; drives the common retry + checkpoint loop |
| `SchemaMapper::mapRow(src_schema, target_schema, row)` | `postgres_importer.cpp` and all future connectors | Already partially defined in `postgres_importer.cpp`; must be extracted to a shared header |
| `ImportCheckpoint::commit(source_id, offset)` | `ImportPipeline`, all connectors | Atomic checkpoint write; enables restart-safe incremental import |
| `ImportConflictResolver::resolve(existing, incoming, strategy)` | `ImportPipeline` post-insert path | New interface; strategies: SKIP, OVERWRITE, MERGE (field-level), ERROR |
| `ImporterPlugin::create(config)` | `ImporterRegistry` | Plugin factory method; enables third-party connectors without modifying core |

## Planned Features

### [x] Apache Kafka Consumer Importer
**Status:** ✅ Implemented (`src/importers/kafka_importer.cpp`)

### [x] CSV / Parquet / JSON Lines Flat-File Importer
**Status:** ✅ Implemented (`src/importers/flatfile_importer.cpp`)

### MySQL / MariaDB Importer
**Priority:** High
**Target Version:** v1.8.0
**Status:** ✅ Implemented (`src/importers/mysql_importer.cpp`)

**Implementation Notes:**
- `[x]` `mysql_importer.cpp` registers with `ImporterRegistry` and is reachable from the admin import API.
- `[x]` Add Prometheus counters `importers_mysql_rows_imported_total` and `importers_mysql_errors_total` consistent with other importer naming.
- `[x]` Add integration test using a Docker MySQL 8.0 container.

### MongoDB Document Importer
**Priority:** High
**Target Version:** v1.8.0
**Status:** `src/importers/mongo_importer.cpp` exists; verify it handles all BSON extended JSON types.

**Implementation Notes:**
- `[x]` BSON-to-JSON conversion must handle `ObjectId` → string, `ISODate` → ISO 8601, `Decimal128` → string, `Binary` → base64.
- `[x]` Add integration test using a Docker MongoDB 6.0 container.

**Performance Targets:**
- Import throughput ≥ 30 000 documents/sec from a local MongoDB 6.0 instance with 2 KB average documents.



Add a MySQL/MariaDB source connector that mirrors the existing `postgres_importer.cpp` structure. MySQL is the second most commonly requested source after PostgreSQL and is needed to support migration workflows from legacy LAMP-stack applications.

**Implementation Notes:**
- `[x]` Add `mysql_importer.cpp` implementing `IImporter`; use `libmysqlclient` or the MariaDB C Connector (prefer the latter for MariaDB-specific data types).
- `[x]` Reuse `SchemaMapper` extracted from `postgres_importer.cpp`; add MySQL-specific type mappings (`TINYINT(1)` → `bool`, `TEXT` vs `LONGTEXT` sizing, `JSON` column native support via MySQL 5.7+).
- `[x]` Support both full and incremental import; incremental mode uses a configurable `updated_at` column as the high-watermark (same pattern as the PostgreSQL importer). Implemented via `delta_hash_file` + `delta_key_columns` = `{"updated_at"}`.
- `[x]` Add `importers_mysql_rows_imported_total` and `importers_mysql_errors_total` Prometheus counters, consistent with the naming convention in the import pipeline.
- `[x]` Credentials must come from environment variables or a secrets manager reference; never from the `ImportConfig` struct fields that appear in log output.

**Performance Targets:**
- `[x]` Import throughput ≥ 50 000 rows/sec from a local MySQL 8.0 instance (measured with `BM_MySQLInsertRows_1M` in `benchmarks/bench_importer_throughput.cpp`).
- `[x]` Batch size of 1 000 rows per `IImporter::importBatch()` call with configurable override via `ImportConfig::batch_size`.

---

### MongoDB Document Importer
**Priority:** High
**Target Version:** v1.6.0

Add a MongoDB source connector that reads documents from a specified collection and database using the MongoDB C Driver. MongoDB documents map naturally to ThemisDB's document model; the primary challenge is BSON-to-JSON type coercion and handling of embedded arrays and nested objects.

**Implementation Notes:**
- Add `mongodb_importer.cpp` implementing `IImporter`; use `libmongoc` (MongoDB C Driver) for cursor-based batch reads.
- BSON-to-JSON conversion must handle all MongoDB extended JSON types: `ObjectId` → string, `ISODate` → ISO 8601 string, `Decimal128` → string with precision annotation, `Binary` → base64 string.
- Incremental import uses MongoDB's `_id` ObjectId ordering as the high-watermark (monotonically increasing for time-ordered inserts); configurable fallback to a user-specified `updated_at` field.
- Schema mapping uses `SchemaMapper`; add a `BSON_PASSTHROUGH` mode that preserves the MongoDB document structure verbatim (useful when the target ThemisDB schema mirrors the source exactly).
- Validate that `max_bson_size` (default 16 MB) is not exceeded per document; log a warning and skip documents that exceed the limit rather than aborting the batch.

**Performance Targets:**
- Import throughput ≥ 30 000 documents/sec from a local MongoDB 6.0 instance with documents averaging 2 KB.
- BSON-to-JSON conversion overhead ≤ 10 % of total import wall-clock time.

---

### CSV / Parquet / JSON Lines Flat-File Importer
**Priority:** Medium
**Target Version:** v1.7.0

Add a flat-file importer that reads `CSV`, `.parquet`, and `.jsonl` files from local disk or S3-compatible object storage. This covers the common workflow of importing training data exports, analytics dumps, or migration snapshots stored as files rather than live databases.

**Implementation Notes:**
- Add `flatfile_importer.cpp` implementing `IImporter`; dispatch to format-specific readers based on file extension or an explicit `format` config key.
- CSV reader uses a configurable delimiter, quote character, and header-row flag; maps header names to ThemisDB field names via `SchemaMapper`.
- Parquet reader uses Apache Arrow C++ (`arrow::parquet::StreamReader`); reuse the Arrow dependency already planned for the Exporters module.
- JSONL reader is line-by-line; each line is parsed as a JSON object and passed to `SchemaMapper`.
- S3 source: implement a `S3StreamSource` that wraps `libcurl` (or the AWS SDK if available) to stream file bytes without downloading to local disk first.
- Add dry-run mode: parse and validate schema mapping without writing documents; report field mapping errors and type coercion warnings to stdout.

**Performance Targets:**
- CSV import throughput ≥ 200 000 rows/sec for files with 20 string columns from local NVMe.
- Parquet import throughput ≥ 500 MB/s uncompressed (leveraging Arrow zero-copy column reads).

---

### [x] Import Conflict Resolution Strategies
**Status:** ✅ Implemented (`src/importers/conflict_resolver.cpp`, `include/importers/conflict_resolver.h`)
**Priority:** Medium
**Target Version:** v1.7.0

`ImportConflictResolver` handles documents where the target collection already contains a document with the same key. The PostgreSQL importer (`postgres_importer.cpp`) integrates the resolver on every COPY and INSERT block, giving operators explicit control over upsert, merge, and skip workflows.

**Implementation Notes:**
- `conflict_resolver.cpp` provides four strategies: `SKIP` (do not overwrite existing), `OVERWRITE` (replace existing document entirely), `MERGE` (merge fields: incoming fields win unless the existing field is listed in `protected_fields`), and `ERROR` (abort the batch on first conflict).
- Strategy is configured per import job via `ImportConfig::conflict_strategy` (see `include/importers/importer_interfaces.h`) and `ImportOptions::conflict_strategy` (see `include/importers/importer_interface.h`); default is `OVERWRITE` for backward compatibility.
- MERGE strategy uses a configurable `merge_depth` (default 1, meaning top-level fields only; set to -1 for deep recursive merge) to avoid unexpected behavior with nested objects.
- `importers_conflicts_total` Prometheus counter is emitted via `ImportOptions::metrics_callback` with labels `strategy` and `outcome=skipped|overwritten|merged|error` for operator visibility.
- 28 unit tests in `tests/test_importer_conflict_resolver.cpp` (self-contained; all four strategies, composite keys, deep merge, metrics emission).

**Performance Targets:**
- Conflict resolution overhead ≤ 5 % of import throughput for SKIP and OVERWRITE strategies (one extra key-existence check per document).
- MERGE strategy overhead ≤ 15 % compared to OVERWRITE for documents with ≤ 100 fields.

---

### Importer Plugin API
**Priority:** Low
**Target Version:** v1.9.0
**Status:** ✅ Implemented (Issue #251)

Add a stable plugin API (`IImporter` + `ImporterPlugin` factory) that allows third-party importers to be compiled as shared libraries and loaded at runtime via `ImporterRegistry::loadPlugin(path)`. This is required for proprietary source connectors (Oracle, MSSQL, Salesforce) that cannot be distributed with ThemisDB due to licensing.

**Implementation Notes:**
- [x] C-linkage plugin ABI defined in `include/importers/importer_plugin.h`; `THEMIS_IMPORTER_PLUGIN_V1` versioned struct (with `abi_version`, `struct_size`, function table) allows ABI evolution without breaking existing plugins.
- [x] `ImporterRegistry::loadPlugin(path)` (alias for `ImporterPluginRegistry::loadPlugin()`) uses `dlopen`/`dlsym` (Linux/macOS) or `LoadLibrary`/`GetProcAddress` (Windows) to load the factory symbol `themis_importer_create`.
- [x] Plugin isolation: each plugin's `importData()` call runs in a sandboxed thread with a configurable memory limit (`PluginSandboxConfig::memory_limit_bytes`). The sandbox counting allocator (`ThemisImporterAllocator`) is passed to `create_instance`; imports that exceed the limit fail gracefully. A `timeout_ms` field enforces an upper wall-clock limit; `cancel()` is signalled on timeout.
- [x] Oracle importer skeleton documented in `docs/importers/plugin_guide.md` with full V1 ABI walked example, CMake build, runtime loading, and sandbox configuration.

**Performance Targets:**
- Plugin load time (cold `dlopen`) ≤ 50 ms; negligible impact on import throughput once loaded.
- Plugin API version check on load adds ≤ 1 ms startup overhead.

---

### Apache Kafka Consumer Importer
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Implemented (`src/importers/kafka_importer.cpp`, `include/importers/kafka_importer.h`)

Consumes messages from Apache Kafka topics and imports them as ThemisDB entities.
Enables real-time data intake from event-driven systems without polling REST APIs.

**Implementation Notes:**
- `kafka_importer.cpp` implements `IImporter`; uses the librdkafka C API for consumer group management, gated on the `THEMIS_ENABLE_KAFKA` compile-time flag.  When the flag is absent the importer compiles but returns an error at runtime, so builds without Kafka support are unaffected.
- Consumer group ID is configurable via JSON `"consumer_group"` key; offset commit occurs on `rd_kafka_consumer_close()` after all messages have been processed, preserving at-least-once delivery semantics.
- `KafkaImporter::parseKafkaUrl()` accepts `kafka://broker:9092/topic` URLs or bare topic names (broker list provided via `initialize()` config JSON).
- Supports JSON, Avro (Confluent wire format: magic byte + 4-byte schema ID stripped), and plaintext message formats.
- Security: SASL/SSL options are supported; credentials (`sasl_password`) are never written to log messages, error strings, or observability output.
- Plugin descriptor: `plugins/importers/kafka/plugin.json`.
- Unit tests: `tests/test_kafka_importer.cpp` (37 test cases, no live broker required; uses mock injection via `setMessageFetchForTesting()`).

**Performance Targets:**
- Consume throughput ≥ 100 000 small messages/sec from a local Kafka broker (single partition, JSON format, no TLS).
- Per-message JSON parse overhead ≤ 5 µs for messages up to 4 KB.
- Benchmarks to be added to `benchmarks/importers_bench.cpp` once the benchmarking harness covers streaming connectors.

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Each new connector (`mysql_importer.cpp`, `mongodb_importer.cpp`, `flatfile_importer.cpp`) must have unit tests with mocked database cursors and mock S3 responses; `ConflictResolver` tested for all four strategies with known fixture data |
| Integration | Live source databases in CI | PostgreSQL and MySQL integration tests run against Docker containers in CI; MongoDB integration test uses `mongo:6.0` image; assert row counts and field mapping correctness |
| Performance | Throughput regression < 5% on PostgreSQL path | `benchmarks/importers_bench.cpp` runs on every PR touching `postgres_importer.cpp`; new connector benchmarks added at time of implementation |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| PostgreSQL import throughput | ~30 000 rows/sec (estimate) | ≥ 50 000 rows/sec | `benchmarks/importers_bench.cpp`, 10 M row fixture |
| MySQL import throughput | N/A | ≥ 50 000 rows/sec | Same bench harness, MySQL backend |
| MongoDB import throughput | N/A | ≥ 30 000 docs/sec | `benchmarks/importers_bench.cpp`, 2 KB avg doc |
| Flat-file CSV throughput | N/A | ≥ 200 000 rows/sec | Local NVMe, 20-column CSV fixture |

## Security / Reliability

- Connection strings, passwords, and API keys must never appear in log output, error messages, or checkpoint files; only sanitized identifiers (e.g., `host:port/database`) are permitted in observability output.
- Incremental checkpoints are written atomically (temp file + rename) to prevent corrupt state that could cause row duplication or data loss on restart.
- The plugin sandbox limits memory allocation per plugin to prevent a malicious or buggy third-party connector from exhausting process memory and causing denial of service.
- SQL-based connectors (PostgreSQL, MySQL) must use parameterised queries for all schema introspection calls; dynamic SQL constructed from table names must be validated against an allowlist before execution.

---

## v2.1+ Research-Driven Enhancements

### Scope
Scientific and enterprise-grade extensions to the import pipeline, covering ML-assisted
schema intelligence, distributed imports, data quality, and emerging tech integrations.

### Module Descriptions

#### Schema Inference Engine (`importers/schema_inference.cpp`)
**Scope:** Automatic discovery of implicit FK relationships, semantic column types, and
cardinality distributions without user configuration.
**Design Constraints:**
- Stateless; configurable via `SchemaInferenceConfig` (confidence threshold, sample size).
- No external ML runtime dependency; all algorithms run in-process.
**Required Interfaces:** `inferImplicitRelationships()`, `detectSemanticTypes()`, `estimateCardinalities()`
**Implementation Notes:** Jaccard similarity for FK name-stem matching; voting-based regex for semantic types; Wilson-score CI for cardinalities.
**Research:** `docs/research/SCHEMA_INFERENCE_ALGORITHM.md`, `docs/research/CARDINALITY_ESTIMATION.md`
**Performance Target:** ≤ 500 ms for 100-table schema with 10 000 sample values per column.

#### Column Importance Analyzer (`importers/column_importance.cpp`)
**Scope:** Information-theoretic column ranking to identify key and redundant columns.
**Required Interfaces:** `analyzeImportance()`, `findRedundantColumns()`
**Implementation Notes:** Shannon entropy (base-2), Gini impurity, ID3 information gain; Pearson r for redundancy detection.
**Performance Target:** ≤ 200 ms for 50-column table with 10 000 sample rows.

#### CRDT Importer (`importers/crdt_importer.cpp`)
**Scope:** Conflict-free parallel imports using Last-Write-Wins CRDT (Shapiro et al. 2016).
**Design Constraints:** Deterministic merge: wall_clock_ns DESC → lamport_clock DESC → replica_id ASC.
**Required Interfaces:** `importWithCRDT()`, `lookup()`, `CRDTRecord::merge()`
**Implementation Notes:** In-memory `state_` map; production builds should persist to RocksDB.

#### PostgreSQL CDC (`importers/postgres_cdc.cpp`)
**Scope:** True change-data-capture via PostgreSQL logical decoding (pgoutput protocol).
**Design Constraints:** Interface contract only in default build; live libpq requires `THEMIS_ENABLE_CDC`.
**Required Interfaces:** `createPublication()`, `createReplicationSlot()`, `subscribeToChanges()`, `confirmLSN()`

#### Data Quality Framework (`importers/data_quality.cpp`)
**Scope:** NIST SP 800-188 six-dimension quality scoring.
**Required Interfaces:** `assessTable()`, `generateQualityReport()`
**Performance Target:** ≤ 100 ms per table for 1 000-row sample.

#### Audit Trail (`importers/audit_trail.cpp`)
**Scope:** SOX/HIPAA-compliant Merkle-chained immutable event log.
**Design Constraints:** Append-only; `verifyIntegrity()` must detect any tamper in O(n).
**Required Interfaces:** `recordEvent()`, `verifyIntegrity()`, `exportForSIEM()`

#### Adaptive Import Optimizer (`importers/adaptive_import.cpp`)
**Scope:** FK-topology-aware import ordering and runtime batch-size adaptation.
**Required Interfaces:** `optimizeImportPlan()`, `adaptBatchSize()`, `PerformancePredictor::predictPerformance()`
**Performance Target:** ≥ 40% throughput improvement vs. fixed batch size on skewed schemas.

#### Polyglot Persistence Mapper (`importers/polyglot_mapper.cpp`)
**Scope:** Recommend optimal data model (RELATIONAL/DOCUMENT/GRAPH/TIMESERIES/KEYVALUE/VECTORSPACE) per table.
**Required Interfaces:** `recommendDataModels()`, `ModelTransformer::tableToDocument()`, `ModelTransformer::tableToGraph()`

#### Temporal Database Support (`importers/temporal_support.cpp`)
**Scope:** SQL:2011 temporal dimension detection and point-in-time query generation.
**Required Interfaces:** `detectTemporalDimensions()`, `buildPointInTimeQuery()`, `buildSystemTimeQuery()`
**Research:** `docs/research/TEMPORAL_DATABASE_SUPPORT.md`

#### Blockchain Integrity Verifier (`importers/blockchain_integrity.cpp`)
**Scope:** SHA-256 Merkle tree with optional blockchain anchoring for tamper-evidence.
**Design Constraints:** Default uses `std::hash`; enable `THEMIS_ENABLE_OPENSSL` for FIPS 140-2 SHA-256.
**Required Interfaces:** `buildMerkleTree()`, `verifyRecordInTree()`, `anchorToBlockchain()`, `verifyBlockchainAnchor()`
**Research:** `docs/research/BLOCKCHAIN_VERIFICATION.md`

#### Federated Learning Coordinator (`importers/federated_learning.cpp`)
**Scope:** Aggregate schema statistics from distributed PostgreSQL nodes without raw-data transfer.
**Design Constraints:** ε-δ differential privacy (Gaussian mechanism, ε ≤ 1.0 for strong privacy).
**Required Interfaces:** `aggregateUpdates()`, `addDifferentialPrivacy()`, `spendBudget()`, `verifyPrivacyBudget()`
**Research:** `docs/research/FEDERATED_LEARNING_DESIGN.md`

#### GraphQL Federation Support (`importers/graphql_federation.cpp`)
**Scope:** Auto-generate Apollo Federation v2 SDL from relational schemas.
**Required Interfaces:** `generateFederatedSchema()`, `generatePlainSchema()`
**Implementation Notes:** PascalCase type names; `@key` from PKs; `@external` for remote entities; FK columns annotated.

---

## Postgres EXCLUDE Constraint Parsing (Target: v1.5.0 — stub completion)

**Stub:** `src/importers/postgres_importer.cpp` — `parseExcludeConstraint()`: captures raw text + optional name; does NOT parse individual EXCLUDE elements, access method, or `WITH` operators  
**Risk:** Imported schema metadata for EXCLUDE constraints is incomplete; `elements`, `index_method`, `using_clause` not populated; schema validation and index-recreation fail silently.

### Scope
- Implement full EXCLUDE constraint parsing in `parseExcludeConstraint()`.
- Parse: `USING <method>` (e.g., `gist`), per-column `<expr> WITH <operator>` pairs, optional `WHERE (predicate)`.
- Populate `ExcludeConstraint::elements`, `index_method`, and `using_clause` fields.
- Add regex or PEG-based grammar for the complex EXCLUDE syntax.

### Test Strategy
- Parse `CONSTRAINT no_overlap EXCLUDE USING gist (period WITH &&, room_id WITH =)` → `index_method == "gist"`, `elements.size() == 2`.
- Parse EXCLUDE without explicit constraint name → `name.empty()`.
- Malformed EXCLUDE → `parseExcludeConstraint()` returns false.

### Performance Targets
- Parsing overhead: ≤ 50 µs per EXCLUDE constraint clause (regex or PEG, single-threaded).
