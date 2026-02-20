# Importers Module: Production-Readiness Assessment & Roadmap

**Version:** 1.0
**Last Updated:** 2026-02-20
**Scope:** Importers module hardening, observability, and feature coverage

---

## Current Status: Partially Production-Ready

The importers module (`src/importers/`, `include/importers/`) provides a PostgreSQL
`pg_dump` importer with basic DDL/DML/COPY parsing, schema mapping, and batch-import
support.  The following gaps must be addressed before the module is suitable for all
production workloads.

---

## Key Gaps Identified

### Functional / Architecture

- **SQL parsing via regex/heuristics** – `parseCreateTable`, `parseInsert`, and the
  `COPY` header are parsed with regular expressions.  Complex DDL (nested parentheses,
  default expressions with sub-selects, quoted identifiers, domain types, partitioned
  tables) and certain DML corner-cases are not handled correctly.
- **No streaming / checkpoint support** – The entire dump is processed in a single
  pass.  Very large files (>1 GB) block the caller and cannot be resumed after a
  partial failure.
- **No multi-threading / async import** – Import is single-threaded.  No API exists
  for live progress streaming to callers while the import is running.
- **Type mapping incomplete** – Array types (`integer[]`, `text[]`), `jsonb`, `inet`,
  `cidr`, `macaddr`, `money`, `interval`, `xml`, and PostGIS geometry types had no
  dedicated mapping rules.  User-configurable overrides were also missing.
- **No input validation beyond file header check** – Malformed rows, encoding issues,
  or unexpectedly large values can cause silent data loss.

### Observability / Error Handling

- **No structured error surface** – Errors were appended to a plain `vector<string>`.
  Callers could not programmatically distinguish parse errors from I/O errors or
  data-conversion errors.
- **No Prometheus / OpenTelemetry metrics** – Throughput, latency, error rates, and
  per-table import counts are not exported.
- **No monitoring / recovery API** – There is no way to query import progress, retry
  failed batches, or quarantine problematic rows.

### Testing

- **No integration tests for COPY parsing** – The original `parseCopy` contained a
  `TODO` comment and did not parse tab-separated values.
- **No tests for INSERT VALUES parsing** – `parseInsert` similarly contained a `TODO`
  and never extracted row data.
- **No fuzz tests or chaos tests** – Corrupt dumps, truncated files, and encoding
  edge-cases are untested.
- **No cross-version CI** – PostgreSQL dump formats differ between versions 9.x, 12,
  14, 15, and 16.  No test matrix covers version variants.

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
|-----------------|---------------|
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

## Roadmap to Full Production Readiness

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
- [ ] Implement true streaming parser: process the dump file in fixed-size chunks so
      memory usage is bounded regardless of file size.

### Phase 3: Observability (Q2 2026)

- [x] Log structured JSON import-completion summary at `INFO` level on import end.
- [x] Emit Prometheus / OTel metrics via `ImportOptions.metrics_callback`: callers wire
      any metrics backend without a hard library dependency.  Standard metric names:
      `themisdb_import_rows_total{table,status}`,
      `themisdb_import_duration_seconds`,
      `themisdb_import_errors_total{code}`,
      `themisdb_import_tables_total`.
      See the [runbook](importers_runbook.md) for PrometheusMetrics wiring example.
- [x] Prometheus REST endpoint via `ImportApiHandler` (`httplib`, same pattern as
      `branch_api_handler.cpp`).  Routes:
      - `POST /api/v1/import/postgresql` – start async import, returns job handle JSON
      - `GET /api/v1/import/{job_id}/status` – live progress + final stats
      - `POST /api/v1/import/{job_id}/cancel` – cancel a running job
      - `GET /api/v1/import/jobs` – list all known jobs
      - `GET /api/v1/import/metrics` – Prometheus text format (scrape endpoint)
- [ ] Add OpenTelemetry spans around batch processing for distributed tracing.

### Phase 4: Robust SQL Parsing (Q3 2026)

- [ ] Evaluate integration of a SQL parser library (e.g., `libpg_query` or a
      vendored recursive-descent parser) to replace the regex-based DDL parser.
- [ ] Handle nested parentheses in column defaults, complex type expressions, and
      `ALTER TABLE` statements in dumps.
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
- [ ] True libFuzzer / AFL fuzz drivers (requires build infrastructure change).
- [ ] Benchmark suite: throughput in rows/s for 100 MB, 1 GB, and 10 GB dumps.
- [ ] CI matrix: run tests against multiple PostgreSQL dump format variants.

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

## See Also

- [PostgreSQL Importer Source](../src/importers/postgres_importer.cpp)
- [Importer Interface](../include/importers/importer_interface.h)
- [Exporters Roadmap](exporters_roadmap.md)
- [Importers Runbook](importers_runbook.md)
- [Error Handling Guide](error_handling/README.md)
- [Import CLI](../tools/import_cli.cpp)
