# Importer Plugins

## Status: ✅ Production-Ready (v1.7)

Data import plugins for ThemisDB.

## Available Importers

### PostgreSQL Importer ✅
**Path:** `postgres/`

**Status:** Production-ready. All roadmap items complete as of v1.7.

**Implementation:** `src/importers/postgres_importer.cpp`

Import data from PostgreSQL `pg_dump` (plain SQL format) files into ThemisDB.

### MySQL / MariaDB Importer ✅
**Path:** `mysql/` *(plugin.json forthcoming)*

**Status:** Production-ready as of v1.7.

**Implementation:** `src/importers/mysql_importer.cpp`

Import data from MySQL / MariaDB `mysqldump` (SQL format) files into ThemisDB.

### MongoDB Importer ✅
**Path:** `mongo/`

**Status:** Production-ready as of v1.7.

**Implementation:** `src/importers/mongo_importer.cpp`

Import document collections exported by `mongoexport`. Supports JSON-Lines (NDJSON)
and JSON array formats, with full BSON extended JSON v2 type unwrapping
(`$oid`, `$date`, `$numberDecimal`, `$numberLong`, etc.).

## Features

### Parsing
- DDL parsing (`CREATE TABLE`, `CREATE SCHEMA`, `ALTER TABLE ADD COLUMN`, `CREATE TYPE AS ENUM/AS (...)`) with nested-parenthesis and quoted-string awareness (`splitTopLevelCommas`, `findMatchingParen`)
- DML parsing (`INSERT INTO ... VALUES`) with full single-quoted string + `''`-escape tokeniser
- `COPY ... FROM stdin` parsing: tab-separated, full PostgreSQL escape sequences (`\N`/`\t`/`\n`/`\r`/`\\`)
- Binary COPY format detection: emits clear `BINARY_COPY_FORMAT` (code 206) error instead of silent garbage
- pg_dump `--schema-only` / `--data-only` mode flag detection from dump header

### Type Mapping
- 30+ built-in PostgreSQL → ThemisDB type mappings (jsonb, array types, inet/cidr/macaddr, money, interval, xml, PostGIS geometry, tsvector, tsquery, oid/xid/cid, float4/float8, bigserial, etc.)
- User-configurable `type_overrides` map in `ImportOptions`
- Custom types from `CREATE TYPE` statements registered automatically

### Reliability & Safety
- **Checkpoint / resume**: byte-offset checkpointing to JSON file (`checkpoint_file`); resumes on re-open
- **Bounded streaming line reader** (`streamReadLine`): per-line cap to `max_statement_size_bytes` / `max_row_size_bytes`; prevents OOM on crafted dumps
- **Row-size guard**: rows exceeding `max_row_size_bytes` produce `ROW_TOO_LARGE` (code 205) structured errors
- **Statement-size guard**: statements exceeding `max_statement_size_bytes` produce `STATEMENT_TOO_LARGE` (code 204) errors
- **UTF-8 validation** (`enforce_utf8`): RFC 3629 full validator; rejects overlong encodings, surrogates, truncated sequences
- **Quarantine file** (`quarantine_file`): failed rows appended as JSON-L for inspection and retry
- **Permission check callback** (`permission_check`): ACL enforcement at import start; denial → `PERMISSION_DENIED` (code 503)
- **Delta / incremental import** (`delta_hash_file` + `delta_key_columns`): FNV-1a 64-bit hash; skips already-imported rows

### Observability
- **Structured error API**: `ImportErrorCode` (ranges: I/O 100–199, SQL parse 200–299, schema 300–399, data 400–499, validation 500–599), `ImportErrorSeverity` (INFO/WARNING/ERROR/CRITICAL)
- **Structured JSON completion summary** logged at INFO level on import end
- **Metrics callback** (`metrics_callback`): `MetricsCallback` – wire any Prometheus / OTel backend; standard metric names: `themisdb_import_rows_total{table,status}`, `themisdb_import_duration_seconds`, `themisdb_import_errors_total{code}`, `themisdb_import_tables_total`
- **OTel span callback** (`tracing_callback`): `SpanCallback` – wire any distributed tracing backend; spans: `import_total`, `parse_table`, `copy_block`, `insert_batch`, `alter_column`
- **Audit events**: `BULK_IMPORT` and `BULK_IMPORT_COMPLETED` in `SecurityEventType`

### Async / Multi-Threading
- **`importDataAsync()`**: launches background thread; returns `shared_ptr<ImportHandle>` with `shared_future<ImportStats>`, atomic live-progress counters, stage string, and `getStatus()`
- **`ImportJobRegistry`**: thread-safe map of all in-flight and completed jobs

### REST API
- **`ImportApiHandler`** (httplib): registers routes on any `httplib::Server`:
  - `POST   /api/v1/import/postgresql` – start async import
  - `GET    /api/v1/import/{job_id}/status` – live progress + final stats
  - `POST   /api/v1/import/{job_id}/cancel` – cancel running job
  - `GET    /api/v1/import/jobs` – list all jobs
  - `GET    /api/v1/import/metrics` – Prometheus text-format scrape endpoint

### CLI Tool
- **`themisdb-import`** (`tools/import_cli.cpp`): all `ImportOptions` exposed as CLI flags; `--output-json`, `--progress`, `--dry-run`, `--checkpoint`, `--quarantine`, `--delta-hashes`, `--enforce-utf8`, `--max-row-size`, `--include-table`, `--exclude-table`, etc.

## Known Limitations

- SQL parsing is still regex / hand-written recursive-descent; full libpg_query integration is deferred to Q3 2026 (see [ADR-003](../../docs/architecture/ADR-003-pg-dump-sql-parser.md)). Very complex DDL (partitioned tables, domain types, generated columns) may not parse correctly.
- Binary COPY format (`pg_dump -F c`) is **not** supported; use plain SQL format (`pg_dump -F p`).
- The importer instance is not thread-safe for concurrent `importData()` calls from multiple threads; use one instance per goroutine/thread or the async API (one job at a time per importer instance).

## Documentation

- [Importers Roadmap](../../docs/importers_roadmap.md) – all phases complete
- [Importers Runbook](../../docs/importers_runbook.md) – operator failure scenarios
- [ADR-003: SQL Parser Library](../../docs/architecture/ADR-003-pg-dump-sql-parser.md)
- [Importer Interface](../../include/importers/importer_interface.h)
- [Import API Handler](../../include/server/import_api_handler.h)
- [CLI Tool](../../tools/import_cli.cpp)
