# Importers Module - Future Enhancements

## Scope

This document covers planned enhancements to the Importers module beyond what is tracked in `ROADMAP.md`. It focuses on `postgres_importer.cpp` and the surrounding import pipeline infrastructure. Features here describe the engineering work required to add additional source connectors (MySQL, MongoDB, SQLite, flat files, S3), a plugin API for third-party importers, and production-hardening of the existing PostgreSQL importer including distributed parallel import and conflict resolution strategies.

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

### MySQL / MariaDB Importer
**Priority:** High
**Target Version:** v1.6.0

Add a MySQL/MariaDB source connector that mirrors the existing `postgres_importer.cpp` structure. MySQL is the second most commonly requested source after PostgreSQL and is needed to support migration workflows from legacy LAMP-stack applications.

**Implementation Notes:**
- Add `mysql_importer.cpp` implementing `IImporter`; use `libmysqlclient` or the MariaDB C Connector (prefer the latter for MariaDB-specific data types).
- Reuse `SchemaMapper` extracted from `postgres_importer.cpp`; add MySQL-specific type mappings (`TINYINT(1)` → `bool`, `TEXT` vs `LONGTEXT` sizing, `JSON` column native support via MySQL 5.7+).
- Support both full and incremental import; incremental mode uses a configurable `updated_at` column as the high-watermark (same pattern as the PostgreSQL importer).
- Add `importers_mysql_rows_imported_total` and `importers_mysql_errors_total` Prometheus counters, consistent with the naming convention in the import pipeline.
- Credentials must come from environment variables or a secrets manager reference; never from the `ImportConfig` struct fields that appear in log output.

**Performance Targets:**
- Import throughput ≥ 50 000 rows/sec from a local MySQL 8.0 instance (measured with a 10 M row benchmark table in `benchmarks/importers_bench.cpp`).
- Batch size of 1 000 rows per `IImporter::importBatch()` call with configurable override via `ImportConfig::batch_size`.

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

### Import Conflict Resolution Strategies
**Priority:** Medium
**Target Version:** v1.7.0

Add an `ImportConflictResolver` that handles documents where the target collection already contains a document with the same key. Currently the PostgreSQL importer silently overwrites or errors on conflict; operators need explicit control to support upsert, merge, and skip workflows.

**Implementation Notes:**
- Add `conflict_resolver.cpp` with four strategies: `SKIP` (do not overwrite existing), `OVERWRITE` (replace existing document entirely), `MERGE` (merge fields: incoming fields win unless the existing field is listed in `protected_fields`), and `ERROR` (abort the batch on first conflict).
- Strategy is configured per import job via `ImportConfig::conflict_strategy`; default is `OVERWRITE` for backward compatibility.
- MERGE strategy uses a configurable `merge_depth` (default 1, meaning top-level fields only; set to -1 for deep recursive merge) to avoid unexpected behavior with nested objects.
- Emit `importers_conflicts_total` Prometheus counter with label `strategy` and `outcome=skipped|overwritten|merged|error` for operator visibility.

**Performance Targets:**
- Conflict resolution overhead ≤ 5 % of import throughput for SKIP and OVERWRITE strategies (one extra key-existence check per document).
- MERGE strategy overhead ≤ 15 % compared to OVERWRITE for documents with ≤ 100 fields.

---

### Importer Plugin API
**Priority:** Low
**Target Version:** v1.9.0

Add a stable plugin API (`IImporter` + `ImporterPlugin` factory) that allows third-party importers to be compiled as shared libraries and loaded at runtime via `ImporterRegistry::loadPlugin(path)`. This is required for proprietary source connectors (Oracle, MSSQL, Salesforce) that cannot be distributed with ThemisDB due to licensing.

**Implementation Notes:**
- Define a C-linkage plugin ABI in `include/importers/importer_plugin.h`; use a `THEMIS_IMPORTER_PLUGIN_V1` versioned struct to allow ABI evolution without breaking existing plugins.
- `ImporterRegistry::loadPlugin(path)` uses `dlopen`/`dlsym` (Linux/macOS) or `LoadLibrary`/`GetProcAddress` (Windows) to load the factory symbol `themis_importer_create`.
- Plugin isolation: each plugin runs in a sandboxed thread group with a configurable memory limit; a plugin that allocates beyond its limit is terminated and the import job fails gracefully.
- Document the plugin API with a worked example Oracle importer skeleton in `docs/importers/plugin_guide.md`.

**Performance Targets:**
- Plugin load time (cold `dlopen`) ≤ 50 ms; negligible impact on import throughput once loaded.
- Plugin API version check on load adds ≤ 1 ms startup overhead.

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
