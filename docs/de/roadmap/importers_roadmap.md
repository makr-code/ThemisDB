# Importers Module: Production-Readiness Assessment & Roadmap

**Version:** 2.1
**Last Updated:** 2026-04-06
**Scope:** Importers module hardening, observability, and feature coverage

---

## Current Status: Production-Ready ✅ (v2.1 feature set)

The importers module (`src/importers/`, `include/importers/`) is production-ready across
all supported connectors (PostgreSQL v2.1, MySQL/MariaDB, MongoDB, SQLite, Oracle, Kafka,
S3, flat files).  All gaps identified in the original assessment have been addressed.
Advanced capabilities added through v2.1 include MDM entity deduplication, adaptive import
optimization, data quality assessment, CRDT-based import, and a GUI import wizard.

> **Note:** "Production-Ready" applies to the v2.1 feature set documented here.
> Additional features remain planned for future versions (Microsoft SQL Server importer,
> FedProx aggregation, Ethereum blockchain anchor, quantum-safe cryptography, and
> Zero-Knowledge Proofs). See [`src/importers/ROADMAP.md`](../src/importers/ROADMAP.md)
> for the full feature roadmap.

For detailed architecture information, see [`src/importers/ARCHITECTURE.md`](../src/importers/ARCHITECTURE.md).

---

## Previously Identified Gaps — All Resolved ✅

The following gaps were identified in v1.0 (2026-02-20) and have since been resolved:

| Gap | Resolution | Version |
|-----|-----------|---------|
| SQL parsing via regex/heuristics | Hardened DDL/DML/COPY parser with full PG 12–16 support | v1.1–v1.3 |
| No streaming / checkpoint support | `ImportOptions::checkpoint_file` with byte-offset resume | v1.2 |
| No multi-threading / async import | Parallel table import; streaming `ProgressCallback` | v1.2 |
| Type mapping incomplete | Full type table: `jsonb`, arrays, `inet`, `money`, `interval`, PostGIS | v1.1 |
| No input validation | `max_row_size_bytes`, `max_statement_size_bytes`, `enforce_utf8` | v1.3 |
| No structured error surface | `ImportErrorCode` enum + `ImportError` struct + `toJson()` | v1.1 |
| No Prometheus/OTel metrics | `ImportOptions::metrics_callback` with label support | v1.4 |
| No monitoring / recovery API | `ImportStats`, `structured_errors`, progress events | v1.2 |
| No COPY/INSERT integration tests | `test_postgres_importer_integration.cpp` (15 cases) | v1.2 |
| No fuzz / chaos tests | `test_postgres_importer_chaos.cpp` (25+ cases); AFL++ harness | v1.3 |
| No cross-version CI | PG 12, PG 14, PG 15, PG 16 fixture matrix | v1.3 |
| No MySQL connector | `mysql_importer.cpp` production-ready | v1.4 |
| No MongoDB connector | `mongo_importer.cpp` production-ready | v1.4 |
| No Kafka connector | `kafka_importer.cpp` (JSON, Avro, plaintext) | v1.5 |
| No S3 connector | `s3_importer.cpp` (multi-part, checksum) | v1.5 |
| No flat-file importer | `flatfile_importer.cpp` (CSV/TSV/Parquet/NDJSON) | v1.5 |
| No conflict resolution | `conflict_resolver.cpp` (SKIP, OVERWRITE, MERGE, ERROR) | v1.5 |

---

## Changes Delivered (v1.1, v1.2, and v1.3)

The following items were implemented across three production-hardening iterations.

### Structured Error API (v1.1) ✅

- Added `ImportErrorCode` enum (ranges: I/O 100–199, SQL parse 200–299, schema 300–399,
  data conversion 400–499, validation 500–599, generic 900–999).
- Added `ImportErrorSeverity` enum (`INFO`, `WARNING`, `ERROR`, `CRITICAL`).
- Added `ImportError` struct (`code`, `severity`, `message`, `location`) with
  `toJson()` serialisation.
- `ImportStats` now carries a `structured_errors` vector alongside the legacy
  plain-string `errors` list for backward compatibility.

### User-Configurable Type Overrides ✅

- `ImportOptions` gained a `type_overrides` map allowing callers to override any
  PostgreSQL type → ThemisDB type mapping without modifying source code.

### Expanded Type Mapping ✅

Added explicit mappings for previously unmapped types:

| PostgreSQL type | ThemisDB type |
| --- | --- |
| `jsonb` | `json` |
| `integer[]`, `text[]`, etc. | `array` |
| `inet`, `cidr`, `macaddr`, `macaddr8` | `string` |
| `xml` | `string` |
| `money` | `double` |
| `interval` | `string` |
| `real` / `float4` | `float` |
| `double precision` / `float8` | `double` |
| `bigserial`, `bigint`, `int8` | `long` |
| `smallserial`, `smallint`, `int2` | `integer` |
| PostGIS geometry types (`point`, `polygon`, etc.) | `geo` |
| `tsvector`, `tsquery` | `string` |
| `oid`, `xid`, `cid` | `integer` |

### COPY Row Parsing ✅

- `parseCopyRow()` now correctly splits on TAB and handles all standard PostgreSQL COPY
  text-format escape sequences (`\N` → NULL, `\t`, `\n`, `\r`, `\\`).
- Column-count mismatches are recorded as structured `WARNING` errors; the import
  continues (or halts, depending on `continue_on_error`).
- COPY column lists from the header (`COPY table (col1, col2) FROM stdin`) are parsed
  and used to resolve the effective schema.

### INSERT VALUES Parsing ✅

- `parseInsertValues()` now tokenises the VALUES clause, correctly handling:
  - single-quoted strings with `''` escape sequences,
  - unquoted numeric / boolean / `NULL` literals.
- The resolved values are passed to `convertRowToEntity()`.

### Progress Reporting ✅

- `reportProgress()` is called after every table schema is parsed so callers receive
  incremental feedback even before data rows are processed.
- `parseDumpFile()` now accepts the `ProgressCallback` parameter and forwards it
  through to schema and data phases.
- Checkpoint saves also trigger a `reportProgress` event for batched data progress.

### Structured JSON Completion Summary ✅

- `importData()` now logs the full `ImportStats::toJson()` payload at `INFO` level on
  completion, giving operators a machine-readable completion event in the log stream.

### Input Validation / Safety Limits ✅

- `ImportOptions` gains `max_row_size_bytes` (0 = unlimited): COPY rows that exceed
  this limit are rejected with a structured `ROW_TOO_LARGE` (code 205) warning.
- `ImportOptions` gains `max_statement_size_bytes` (0 = unlimited): SQL statements
  that exceed this limit are skipped with a structured `STATEMENT_TOO_LARGE` (code 204)
  warning.  Both respect `continue_on_error`.

### Checkpoint / Resume Support ✅

- `ImportOptions` gains `checkpoint_file`: path to a JSON file where the importer
  persists the current byte offset + accumulated counts after every `batch_size`
  statements.
- On start-up, if the checkpoint file exists, the importer reads the byte offset
  and seeks the input file to that position, resuming where it left off.
- Checkpoint is also saved on clean completion.
- Format: `{"byte_offset": N, "imported_records": N, ...}`.

### Integration Test Fixture ✅

- Added `tests/fixtures/importers/sample_pg15.sql`: a realistic pg_dump SQL file
  containing three tables (`users`, `products`, `orders`) with diverse PostgreSQL
  types (integer, bigint, varchar, boolean, numeric, jsonb, inet, text[]).
- Added `tests/test_postgres_importer_integration.cpp` with 15 integration-test cases
  covering: `validateSource`, `getSourceSchema`, dry-run counts, normal import counts,
  `include_tables` / `exclude_tables` filtering, `max_row_size_bytes` enforcement,
  and COPY row NULL / array / JSON field parsing.

### UTF-8 Encoding Validation (v1.3) ✅

- `ImportOptions` gains `enforce_utf8` (default `false`).
- When enabled, every COPY row is validated for correct UTF-8 before processing.
- Validation covers: invalid lead bytes, truncated multi-byte sequences, overlong
  encodings, surrogate code points (U+D800–U+DFFF), and values above U+10FFFF.
- Invalid rows are rejected with structured error `INVALID_UTF8` (code 502);
  `continue_on_error` is respected.

### Multi-Version Test Fixtures (v1.3) ✅

- `tests/fixtures/importers/sample_pg12.sql`: PG 12.14 dump with `inventory` and
  `events` tables (6 COPY rows).
- `tests/fixtures/importers/sample_pg14.sql`: PG 14.9 dump with `sensors` and
  `readings` tables (7 COPY rows, including `timestamp with time zone` and `jsonb`).
- Both fixtures exercise version-specific header lines and type usage.

### Chaos & Edge-Case Tests (v1.3) ✅

- `tests/test_postgres_importer_chaos.cpp`: 25+ test cases covering:
  - 10 UTF-8 validation tests (ASCII, multi-byte, surrogates, overlong, truncated)
  - Truncated COPY block (no `\.` terminator)
  - Corrupted `CREATE TABLE` (no closing parenthesis)
  - Statement-size guard skips oversized statement
  - UTF-8 guard with `continue_on_error = false` stops at first bad row
  - Row-size guard with `continue_on_error = false` stops at first oversized row
  - Binary garbage lines do not crash the importer
  - Empty COPY block, empty file, comment-only file
  - COPY escape edge cases (`\q` preserved, `\n` expanded)
  - PG 12 multi-version fixture: schema detection, row counts, NULL handling
  - PG 14 multi-version fixture: jsonb column, sensor readings, dry-run

### Operator Runbook (v1.3) ✅

- `docs/importers_runbook.md`: 9 failure scenarios with diagnosis and resolution steps:
  file not found, not-a-dump, interrupted import (checkpoint/resume), rows skipped,
  type mapping issues, OOM / large files, encoding-damaged rows, slow import,
  COPY header parse failure.
- Tuning reference table and log-message reference.

### Prometheus / OTel Metrics Callback (v1.4) ✅

- `ImportOptions` gains `metrics_callback: MetricsCallback` – a
  `std::function<void(metric, labels, value)>` that the importer calls at:
  - Every imported/failed/skipped row with `{table, status}` labels
  - Import completion: per-status row totals, total duration, total tables
  - Every structured error: `{code}` label
- No hard dependency on any metrics library; callers wire to Prometheus, OTel,
  or any custom backend.  See runbook for wiring example.

### Audit Logging Support (v1.4) ✅

- Added `BULK_IMPORT` and `BULK_IMPORT_COMPLETED` to `SecurityEventType` in
  `include/utils/audit_logger.h`.
- Added string-conversion cases in `securityEventTypeToString()`.
- Operators call `AuditLogger::logSecurityEvent(SecurityEventType::BULK_IMPORT, ...)`
  at import start and `BULK_IMPORT_COMPLETED` at end with the `ImportStats` JSON payload.

### Fuzz-Style Stress Tests (v1.4) ✅

- 20+ property-based tests in `FuzzStyleTest` suite (`test_postgres_importer_chaos.cpp`):
  - 1000 random printable and 500 random binary inputs to `parseCopyRow`
  - 1000 random and 300 binary inputs to `parseInsertValues`
  - 500 random and 300 binary inputs to `parseCreateTable`
  - 1000 random binary inputs to `isValidUtf8`
  - Property invariants: no crash, column count matches tab count, NULL count consistent

---

## v1.5 Changes Delivered

### Changes Delivered (v1.5)

#### Permission Check / ACL Callback ✅

- `PermissionCheckCallback` type alias added to `importer_interface.h`:
  `std::function<bool(resource, action)>`.
- `ImportOptions.permission_check` – called once at the start of every
  `importData()` / `importDataAsync()` call.
- On denial: structured `PERMISSION_DENIED` error (code 503, CRITICAL severity)
  is recorded and `importData()` returns immediately with an empty `ImportStats`.
- 6 dedicated tests in `test_postgres_importer_advanced.cpp`.

#### Quarantine Rows ✅

- `ImportOptions.quarantine_file` – when non-empty, every failed COPY row (too
  large, invalid UTF-8, column-count mismatch, binary COPY) is appended to the
  file as a JSON-L entry: `{"table": ..., "row": ..., "error": {...}}`.
- `ImportStats.quarantined_records` – tracks the number of quarantined rows.
- `writeQuarantineRow()` helper in `PostgreSQLImporter`.
- 4 dedicated tests in `test_postgres_importer_advanced.cpp`.

#### Binary COPY Format Detection ✅

- First data line of every COPY block is checked for the PostgreSQL binary COPY
  magic bytes (`PGCOPY…`).
- On detection: structured `BINARY_COPY_FORMAT` error (code 206, ERROR severity)
  is recorded; the block is skipped; `continue_on_error` is respected.
- Clear error message tells the operator to re-export without `--format=binary`.
- 5 dedicated tests in `test_postgres_importer_advanced.cpp`.

#### pg_dump Mode Flags ✅

- `ImportStats.is_schema_only` / `is_data_only` – set by scanning the first 50
  comment lines of the dump for `schema-only` / `data-only` markers.
- `ImportStats.toJson()` includes both flags.
- 4 dedicated tests in `test_postgres_importer_advanced.cpp`.

#### Delta / Incremental Import ✅

- `ImportOptions.delta_hash_file` – path to a flat hex file storing 64-bit
  FNV-1a row-content hashes (one 16-char hex value per line).
- On import start: hashes are loaded into an `unordered_set<uint64_t>`.
- Each COPY row is hashed; already-seen rows are skipped (`skipped_records++`).
- New rows are added to the set; on import end the file is updated atomically.
- `ImportOptions.delta_key_columns` – optional list of column names to use as
  the hash key instead of the full raw row.
- `computeRowHash()` / `loadDeltaHashes()` / `saveDeltaHashes()` in
  `PostgreSQLImporter`.
- 5 dedicated tests in `test_postgres_importer_advanced.cpp`.

#### CLI Tool (`tools/import_cli.cpp`) ✅

- Standalone C++ binary `themisdb-import` wrapping all importer features.
- Supports all `ImportOptions` flags via command-line arguments.
- Builds with `nlohmann/json` (header-only); registered in `tools/CMakeLists.txt`.
- `--output-json` prints final `ImportStats` as JSON to stdout.
- `--progress` prints per-table progress to stderr.
- Exit codes: 0 = success, 1 = fatal error, 2 = row-level errors, 3 = bad args.

---

## Roadmap to Full Production Readiness

### Phase 2: Streaming & Checkpoints (Q2 2026)

- [x] Add checkpoint/resume support: persist byte offset to a JSON file so imports can
      be resumed after interruption.
- [x] Expose async import API (`importDataAsync`) returning a `shared_ptr<ImportHandle>`.
      The handle exposes:
      - `std::shared_future<ImportStats>` for the final result
      - `std::atomic<size_t> current_records / total_records` for live progress
      - `getStatus()` → `PENDING | RUNNING | COMPLETED | CANCELLED | FAILED`
      - `getStage()` → current human-readable step
      - `started_at_ms / finished_at_ms` (epoch ms)
      The background worker is launched in a detached `std::thread`; all fields are
      written from the worker and read safely by any observer thread.
- [x] `ImportJobRegistry` – thread-safe map of job ID → handle for multi-job tracking.
- [x] Progress callback wired: worker thread updates handle counters and calls the
      `ProgressCallback` on every row batch.
- [x] Implement true streaming parser: process the dump file in fixed-size chunks so
      memory usage is bounded regardless of file size.
      **Implemented (v1.7):** `streamReadLine()` helper caps per-line allocation to
      `max_statement_size_bytes` (default 64 MB) in `parseDumpFile` and to
      `max_row_size_bytes` (default 64 MB) in `parseCopy`.  Truncated lines are
      recorded as `STATEMENT_TOO_LARGE` / `ROW_TOO_LARGE` structured errors.  The
      stream cursor is always left at the start of the next line so the parser
      continues cleanly after a truncated line.

### Phase 3: Observability (Q2 2026)

- [x] Log structured JSON import-completion summary at `INFO` level on import end.
- [x] Emit Prometheus / OTel metrics via `ImportOptions.metrics_callback`: callers wire
      any metrics backend without a hard library dependency.  Standard metric names:
      `themisdb_import_rows_total{table,status}`,
      `themisdb_import_duration_seconds`,
      `themisdb_import_errors_total{code}`,
      `themisdb_import_tables_total`.
      See the [runbook](../features/importers_runbook.md) for PrometheusMetrics wiring example.
- [x] Prometheus REST endpoint via `ImportApiHandler` (`httplib`, same pattern as
      `branch_api_handler.cpp`).  Routes:
      - `POST /api/v1/import/postgresql` – start async import, returns job handle JSON
      - `GET /api/v1/import/{job_id}/status` – live progress + final stats
      - `POST /api/v1/import/{job_id}/cancel` – cancel a running job
      - `GET /api/v1/import/jobs` – list all known jobs
      - `GET /api/v1/import/metrics` – Prometheus text format (scrape endpoint)
- [x] Add OpenTelemetry spans around batch processing for distributed tracing.
      **Implemented (v1.6):** `SpanCallback` type alias + `ImportOptions.tracing_callback`;
      spans emitted for `import_total`, `parse_table`, `copy_block`, `insert_batch`,
      `alter_column`.  No hard dependency on any tracing library; callers wire to
      OTel/Jaeger/Zipkin/custom.  See `SpanCallback` documentation in
      `include/importers/importer_interface.h`.
- [x] `SpanCallback` type alias + `ImportOptions.tracing_callback`: emits spans for
      `import_total`, `parse_table`, `copy_block`, `insert_batch`, `alter_column`.
      No hard dependency on any tracing library; callers wire to OTel/Jaeger/Zipkin.

### Phase 4: Robust SQL Parsing (Q3 2026)

- [x] Evaluate integration of a SQL parser library (e.g., `libpg_query` or a
      vendored recursive-descent parser) to replace the regex-based DDL parser.
      **Decision recorded (v1.7):** See `docs/architecture/ADR-003-pg-dump-sql-parser.md`.
      Three options evaluated (libpg_query, SQLite3 amalgamation, hand-written
      recursive-descent parser).  **Decision:** Continue with the incremental
      hand-written approach for the current phase; libpg_query adoption deferred to
      Q3 2026 when partition / foreign-key / generated-column support is required.
- [x] Handle nested parentheses in column defaults (`numeric(10,4)`, `varchar(255)`,
      `DEFAULT NOW()`, `CHECK (...)`) via `splitTopLevelCommas()` + `findMatchingParen()`.
- [x] Handle `ALTER TABLE ... ADD COLUMN` statements: updates cached schema so
      subsequent COPY and INSERT rows see the new column.
- [x] Handle `CREATE TYPE ... AS ENUM` / `AS (...)` (composite): stores custom type →
      ThemisDB type mapping in `custom_type_map_`; used by `mapPostgreSQLTypeToThemis`.
- [x] Detect binary COPY format and emit clear error (`BINARY_COPY_FORMAT`, code 206).
- [x] Handle `pg_dump --schema-only` and `--data-only` modes via header detection;
      `ImportStats.is_schema_only` and `is_data_only` flags set automatically.

### Phase 5: Input Validation & Security (Q3 2026)

- [x] Enforce maximum dump row size (`max_row_size_bytes`) – configurable, default unlimited.
- [x] Enforce maximum SQL statement size (`max_statement_size_bytes`) – configurable, default unlimited.
- [x] Validate input encoding (`enforce_utf8 = true`): COPY rows with invalid UTF-8
      byte sequences are rejected with structured error `INVALID_UTF8` (code 502).
      Overlong encodings, surrogates (U+D800–U+DFFF), and truncated multi-byte sequences
      are all detected.
- [x] Audit event types `BULK_IMPORT` and `BULK_IMPORT_COMPLETED` added to
      `SecurityEventType` in `audit_logger.h`; string serialisation added to
      `securityEventTypeToString()`.  Operators can log import start/end via
      `AuditLogger::logSecurityEvent(SecurityEventType::BULK_IMPORT, ...)`.
- [x] ACL / policy check via `PermissionCheckCallback`: operators must hold
      `import:write`; denial produces structured `PERMISSION_DENIED` (code 503) error.
- [x] Quarantine rows that fail conversion into `quarantine_file` (JSON-L) for later
      inspection and retry.

### Phase 6: Testing Completeness (Q3–Q4 2026)

- [x] Integration tests: realistic pg_dump fixture (`sample_pg15.sql`) with 15 test cases.
- [x] Multi-version fixtures: `sample_pg12.sql` (PG 12.14) and `sample_pg14.sql` (PG 14.9)
      with dedicated test cases in `test_postgres_importer_chaos.cpp`.
- [x] Chaos / edge-case tests: truncated COPY block, corrupted CREATE TABLE, empty file,
      binary garbage rows, statement-size guard, UTF-8 validation, row-size guard with
      `continue_on_error = false`, and COPY escape edge cases.
- [x] Fuzz-style stress tests (`FuzzStyleTest` suite in `test_postgres_importer_chaos.cpp`):
      1000+ random/binary inputs for `parseCopyRow`, `parseInsertValues`, `parseCreateTable`,
      and `isValidUtf8`; property-based assertions (no crash, column count consistency,
      NULL count invariant, all-ASCII-is-valid).
- [x] Advanced feature tests (`test_postgres_importer_advanced.cpp`): 35 test cases covering
      permission check, quarantine, binary COPY detection, dump mode flags, delta import,
      FNV-1a hash, and error code values.
- [x] Complex DDL tests (`test_postgres_importer_complex_ddl.cpp`): `SplitTopLevelCommasTest`,
      `FindMatchingParenTest`, `SpanCallbackTest`, `Pg16FixtureTest`, `RealDdlSplitTest` –
      30+ cases covering nested parens, single-quoted defaults, ALTER TABLE, CREATE TYPE,
      and OTel span callback.
- [x] PG 16 fixture (`sample_pg16.sql`): two tables with `numeric(10,4)`, `varchar(255)`,
      `DEFAULT NOW()`, `CHECK` constraints, `CREATE TYPE AS ENUM`, `CREATE TYPE AS (...)`,
      `ALTER TABLE ADD COLUMN`, 7 COPY rows.
- [x] Benchmark: `benchmarks/bench_importer_throughput.cpp` – 6 scenarios (10 k–1 M rows,
      INSERT, mixed, dry-run); reports rows/second; `--csv` export for trend tracking.
- [x] CI workflow: `.github/workflows/importer-tests.yml` – matrix build (gcc-12, clang-15,
      gcc-14); runs all importer GTest suites; uploads XML results as artifacts.
- [x] True libFuzzer / AFL fuzz drivers (requires build infrastructure change).
      **Implemented (v1.7):** `fuzz/harnesses/postgres_importer_harness.cpp` – a
      persistent-mode AFL++ harness (and dual LibFuzzer `LLVMFuzzerTestOneInput`
      entry point) covering all four parsers via a selector byte.  Five corpus seeds
      in `fuzz/corpus/importer/`.  Registered in `fuzz/aflplusplus-config.json`.

### Phase 7: Admin & Operations (Q4 2026)

- [x] Runbook documentation (`docs/importers_runbook.md`): 9 failure scenarios with
      diagnosis and resolution steps; tuning reference; log-message reference.
- [x] CLI tool `themisdb-import` (`tools/import_cli.cpp`): `--source`, `--dry-run`,
      `--progress`, `--checkpoint`, `--quarantine`, `--delta-hashes`, `--enforce-utf8`,
      `--max-row-size`, `--include-table`, `--exclude-table`, `--output-json`, etc.
- [x] Delta / incremental import: skip already-imported rows using FNV-1a content hashes
      persisted in `delta_hash_file`; configurable per-column key via `delta_key_columns`.
- [x] Operator API: list active imports (`GET /api/v1/import/jobs`), cancel by ID
      (`POST /api/v1/import/{id}/cancel`), live progress (`GET /api/v1/import/{id}/status`).

---

## Changes Delivered (v1.7) – 100% Roadmap Coverage

### Bounded Streaming Line Reader ✅

- `streamReadLine(file, line, max_bytes, truncated)` static helper added to
  `postgres_importer.cpp`:
  - Reads up to `max_bytes` characters per line using `std::istream::get()`.
  - When the limit is exceeded the remainder of the line is drained (no allocation)
    and `truncated = true` is set so the caller can record a structured error and
    continue safely.
  - Falls back to the equivalent of `std::getline` when `max_bytes == 0` (unlimited).
- Used in **all** `getline` call sites inside `parseDumpFile` and `parseCopy`:
  - Main DDL/DML scan loop: capped at `max_statement_size_bytes` (default 64 MB).
  - COPY data loop: capped at `max_row_size_bytes` (default 64 MB).
  - Dry-run COPY skip loop: same cap.
  - Binary-COPY skip loop: same cap.
  - Table-excluded skip loop: `2 × max_row_size_bytes` (or 64 MB default).
  - Header detection loop: 4 KB cap (header comments are always short).
- Truncated lines in the DDL loop produce `STATEMENT_TOO_LARGE` (code 204) errors.
- Truncated COPY rows produce `ROW_TOO_LARGE` (code 205) errors and are quarantined
  (if `quarantine_file` is set) with the synthetic raw value
  `"<truncated at N bytes>"`.

### SQL Parser Library Evaluation ADR ✅

- `docs/architecture/ADR-003-pg-dump-sql-parser.md`: formal Architecture Decision
  Record evaluating three candidates (libpg_query, SQLite3 amalgamation,
  hand-written recursive-descent parser).
- **Decision:** Continue with the hand-written incremental parser for this phase;
  libpg_query adoption deferred to Q3 2026 when partitioned-table / foreign-key
  / generated-column support is needed.

### AFL++ / LibFuzzer Fuzz Harness ✅

- `fuzz/harnesses/postgres_importer_harness.cpp`:
  - AFL++ persistent mode (`__AFL_LOOP`) entry point.
  - LibFuzzer `LLVMFuzzerTestOneInput` entry point (compile with `-DLIBFUZZER_HARNESS`).
  - Standalone file-input mode for regression testing (default build).
  - Selector byte (`data[0] & 0x03`) routes to 4 parsers:
    - `0` – `parseCopyRow` via full COPY pipeline
    - `1` – `parseInsertValues` via INSERT pipeline
    - `2` – `parseCreateTable` via CREATE TABLE pipeline
    - `3` – `isValidUtf8` + `enforce_utf8` COPY pipeline
  - Hard input limit: 256 KB per test case; row limit: 16 KB.
  - Uses `mkstemp` for temporary dump files; always cleaned up.
- `fuzz/corpus/importer/`: 5 seed files covering all 4 selectors.
- Registered in `fuzz/aflplusplus-config.json` as target `postgres_importer` with
  ASan + UBSan, 512 MB memory limit, 2 s timeout.

---

## Changes Delivered (v1.8)

### Import Conflict Resolution Strategies ✅

Adds an `ImportConflictResolver` that handles documents where the target collection
already contains a document with the same key.  Operators now have explicit control
over upsert, merge, and skip workflows via `ImportOptions.conflict_strategy`.

**Files added / modified:**

| File | Change |
| --- | --- |
| `include/importers/conflict_resolver.h` | New: `ImportConflictResolver` class declaration |
| `src/importers/conflict_resolver.cpp` | New: full strategy implementation |
| `src/importers/postgres_importer.cpp` | Integrated in `parseInsert` and `parseCopy` paths |
| `tests/test_importer_conflict_resolver.cpp` | New: 28 unit tests covering all strategies |
| `cmake/ModularBuild.cmake` | `conflict_resolver.cpp` added to `themis_core` sources |

**Strategy summary:**

| `ConflictStrategy` | Behaviour |
| --- | --- |
| `OVERWRITE` *(default)* | Replace existing entity with the incoming one |
| `SKIP` | Keep existing entity; discard the incoming duplicate |
| `MERGE` | Field-level merge; incoming fields win unless listed in `protected_fields` |
| `ERROR` | Treat the conflict as a fatal error; honours `continue_on_error` |

**Key `ImportOptions` fields:**

- `conflict_strategy` – one of the four strategies above; default `OVERWRITE` for backward
  compatibility.
- `conflict_key_columns` – list of column names whose values are concatenated to form the
  per-row conflict key.  When empty, conflict detection is disabled.
- `protected_fields` – fields the MERGE strategy must not overwrite.
- `merge_depth` – recursion depth for MERGE (`1` = top-level only, `-1` = unlimited deep
  merge).

**Metrics:**

`importers_conflicts_total` Prometheus counter incremented on every conflict, labelled:

- `table` – source table name
- `strategy` – `skip` | `overwrite` | `merge` | `error`
- `outcome` – `skipped` | `overwritten` | `merged` | `error`

**Test coverage:**

- `ConflictResolverSkip` – 4 cases: first occurrence imported; duplicate discarded;
  multiple duplicates all discarded; different keys not treated as conflicts.
- `ConflictResolverOverwrite` – 3 cases: duplicate replaces original; OVERWRITE is the
  default strategy; no conflict counter when no duplicate.
- `ConflictResolverMerge` – 3 cases: top-level incoming fields win; protected fields
  preserved; depth-1 nested object replaced entirely.
- `ConflictResolverMergeDeep` – 2 cases: nested objects merged recursively at depth −1;
  depth-2 limits recursion correctly.
- `ConflictResolverError` – 3 cases: conflict produces structured error;
  `continue_on_error = false` aborts import; `continue_on_error = true` skips conflicting row.
- `ConflictResolverCompositeKey` – 2 cases: two-column composite key; keys must match
  exactly.
- `ConflictResolverNoKey` – all rows imported when key columns list is empty.
- `ConflictResolverMetrics` – 2 cases: SKIP and MERGE each emit `importers_conflicts_total`.
- `ConflictResolverComputeKey` – 4 cases: empty columns; single column; string column;
  missing column contributes empty segment.
- `MergeEntitiesTest` – 4 cases: non-object incoming wins; all incoming fields added;
  protected fields skipped; deep merge preserves existing nested keys.

---

## See Also

- [PostgreSQL Importer Source](../src/importers/postgres_importer.cpp)
- [Importer Interface](../include/importers/importer_interface.h)
- [Import API Handler](../include/server/import_api_handler.h)
- [Exporters Roadmap](exporters_roadmap.md)
- [Importers Runbook](../features/importers_runbook.md)
- [Error Handling Guide](error_handling/README.md)
- [Import CLI](../tools/import_cli.cpp)
- [ADR-003: SQL Parser Library](architecture/ADR-003-pg-dump-sql-parser.md)
- [AFL++ Fuzz Harness](../fuzz/harnesses/postgres_importer_harness.cpp)
- [Fuzz Corpus Seeds](../fuzz/corpus/importer/)
