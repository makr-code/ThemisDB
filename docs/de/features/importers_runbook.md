# PostgreSQL Importer – Operator Runbook

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Scope:** Day-2 operations for the `postgres_importer` module

---

## Overview

The PostgreSQL importer reads `pg_dump` SQL-format files and imports their data into
ThemisDB.  This runbook covers common failure scenarios, diagnostic steps, recovery
procedures, and tuning guidance.

---

## Quick Reference

| Task | How |
| --- | --- |
| Start import with progress | Set `ProgressCallback` in `importData()` call |
| Dry-run (validate only) | `ImportOptions.dry_run = true` |
| Resume interrupted import | Set `ImportOptions.checkpoint_file` to existing checkpoint |
| Skip large rows | Set `ImportOptions.max_row_size_bytes` |
| Enforce input encoding | Set `ImportOptions.enforce_utf8 = true` |
| Exclude a table | Add to `ImportOptions.exclude_tables` |
| Override a type mapping | Add to `ImportOptions.type_overrides` |
| Read structured errors | Inspect `ImportStats.structured_errors` |
| Skip duplicate rows | `ImportOptions.conflict_strategy = ConflictStrategy::SKIP` |
| Overwrite duplicates | `ImportOptions.conflict_strategy = ConflictStrategy::OVERWRITE` (default) |
| Merge duplicate fields | `ImportOptions.conflict_strategy = ConflictStrategy::MERGE` |
| Abort on duplicate | `ImportOptions.conflict_strategy = ConflictStrategy::ERROR` |

---

## Scenario 1: Import Fails to Open the Dump File

### Symptoms

- `ImportStats.errors` contains "Cannot open file: …"
- `ImportStats.structured_errors` contains an entry with code `FILE_OPEN_FAILED` (101)

### Diagnosis

```bash
# Check file exists and is readable
ls -lh /path/to/dump.sql
# Check process user has read permission
stat /path/to/dump.sql
```

### Resolution

1. Ensure the file path passed to `importData()` is an absolute path.
2. Verify the process user has read permission on the file.
3. If the dump is compressed (`.gz`), decompress first:

   ```bash
   gunzip -c dump.sql.gz > dump.sql
   ```

---

## Scenario 2: File Not Recognised as a PostgreSQL Dump

### Symptoms

- `validateSource()` returns `false`
- Error: "File does not appear to be a PostgreSQL dump"
- Structured error code `NOT_A_PG_DUMP` (103)

### Diagnosis

Check the first 100 lines of the file for the standard pg_dump header:

```bash
head -10 /path/to/dump.sql
# Expected: -- PostgreSQL database dump
#           -- Dumped from database version X.Y
```

### Resolution

- Ensure the dump was created with `pg_dump -F p` (plain SQL format).  Binary
  (`-F c`) and directory (`-F d`) formats are not supported.
- If the file was produced by a non-standard tool, verify it conforms to the
  pg_dump SQL output format.

---

## Scenario 3: Import Interrupted Mid-Way

### Symptoms

- Process was killed, crashed, or hit an out-of-disk condition.
- `ImportStats` shows partial `imported_records` count.

### Recovery with Checkpoint/Resume

Enable checkpointing before the import:

```cpp
ImportOptions opts;
opts.checkpoint_file = "/var/lib/themisdb/import_checkpoint.json";
opts.batch_size      = 1000;  // checkpoint every 1000 rows
```

On the next run, pass the **same** `checkpoint_file`.  The importer will:

1. Read the saved byte offset.
2. Seek the input file to that position.
3. Resume from the last successfully committed batch.

The checkpoint file format:

```json
{
  "byte_offset": 1048576,
  "imported_records": 42000,
  "failed_records": 3,
  "skipped_records": 0,
  "total_records": 42003,
  "tables_processed": 5
}
```

**Note:** After a clean completion, the checkpoint file remains on disk.  Delete it
before starting a new import of the same file, or use a different checkpoint path.

---

## Scenario 4: Rows Silently Skipped / Count Lower Than Expected

### Symptoms

- `ImportStats.imported_records` is lower than expected.
- `ImportStats.skipped_records` is non-zero.

### Diagnosis

1. Check `ImportOptions.include_tables` – if non-empty, only listed tables are imported.
2. Check `ImportOptions.exclude_tables` – listed tables are skipped entirely.
3. Check `ImportStats.structured_errors` for `COLUMN_COUNT_MISMATCH` (301) warnings –
   rows with wrong column counts are rejected.
4. Check `ImportStats.warnings` for human-readable summaries.

### Resolution

- Adjust `include_tables` / `exclude_tables` filters.
- If column mismatches are expected (e.g., `ALTER TABLE` changed the schema between
  dumps), pre-process the dump to align column counts.

---

## Scenario 5: Type Mapping Produces Unexpected Results

### Symptoms

- Numeric fields stored as strings, or vice versa.
- `UnknownPgType` entries in logs.

### Diagnosis

Check the column type in the dump:

```bash
grep -A 20 "CREATE TABLE your_table" dump.sql
```

### Resolution

Use `ImportOptions.type_overrides` to override any problematic mapping:

```cpp
opts.type_overrides["myenum"]          = "string";
opts.type_overrides["custom_domain"]   = "integer";
opts.type_overrides["geography(Point)"] = "geo";
```

The override map is checked (case-insensitively) before the built-in mapping table.

---

## Scenario 6: Memory Usage Grows Without Bound on Large Dumps

### Symptoms

- OOM kill on very large files (> 1 GB).
- Memory usage climbs continuously during import.

### Diagnosis

The importer processes the file line-by-line and does not buffer the entire dump in
memory.  High memory usage is most likely caused by:

- A very large number of distinct table schemas accumulated in `schemas_` map.
- Very large individual SQL statements (e.g., `INSERT INTO … VALUES (…)` with many
  thousands of values).

### Resolution

1. **Use `COPY` format dumps** – `pg_dump --inserts` produces INSERT statements;
   the default produces COPY blocks which are much more memory-efficient.
2. **Limit statement size**: Set `ImportOptions.max_statement_size_bytes` to skip
   pathologically large statements.
3. **Batch imports**: Split the dump into per-table files using `pg_dump -t` and
   import them sequentially.

---

## Scenario 7: Corrupt or Encoding-Damaged Rows

### Symptoms

- Import completes but some rows show garbled text.
- `ImportStats.structured_errors` contains `INVALID_UTF8` (502) entries when
  `enforce_utf8` is enabled.

### Diagnosis

Enable UTF-8 validation to identify offending rows:

```cpp
opts.enforce_utf8 = true;
opts.continue_on_error = true;  // keep running, collect all bad rows
```

After import, inspect:

```cpp
for (auto& e : stats.structured_errors) {
    if (e.code == ImportErrorCode::INVALID_UTF8) {
        // e.location contains "table X, row Y"
        std::cerr << e.location << ": " << e.message << "\n";
    }
}
```

### Resolution

1. Re-export the dump with `PGCLIENTENCODING=UTF8` or `pg_dump --encoding=UTF8`.
2. If the source database uses a non-UTF-8 encoding (e.g., LATIN1), convert:

   ```bash
   iconv -f LATIN1 -t UTF-8 dump.sql > dump_utf8.sql
   ```

3. For row-level corruption, use `ImportOptions.quarantine_file` (planned Phase 5)
   to write rejected rows to a side-channel for manual review.

---

## Scenario 8: Very Slow Import

### Symptoms

- Import throughput significantly below 1000 rows/second on a modern system.

### Diagnosis

1. Check disk I/O: the importer reads the dump file sequentially.
2. Check `THEMIS_DEBUG` logs for per-row entity JSON dumps – they are expensive at
   high row counts.
3. Check whether `THEMIS_DEBUG` logging level is enabled in production.

### Resolution

1. Run at `INFO` or higher log level in production.
2. Increase `ImportOptions.batch_size` (default 1000) to reduce checkpoint overhead.
3. Place the dump file on a local SSD rather than NFS or a network share.
4. Future: use the async import API (`importDataAsync`) when implemented (Phase 2).

---

## Scenario 9: COPY Header Parse Failure

### Symptoms

- `ImportStats.structured_errors` contains `PARSE_COPY_HEADER` (202) entries.
- Rows from an entire table are missing.

### Diagnosis

The importer expects COPY headers in the form:

```sql
COPY [schema.]table [(col1, col2, ...)] FROM stdin;
```

Unusual whitespace, quoted identifiers with special characters, or
non-standard tooling may produce headers the regex cannot match.

### Resolution

1. Check the specific COPY header line in the dump file.
2. If the table name contains special characters, ensure they are ASCII and
   unquoted, or pre-process the dump to normalise identifiers.
3. Report unexpected header formats as a bug for Phase 4 (robust SQL parsing).

---

## Tuning Reference

| Option | Default | Guidance |
| --- | --- | --- |
| `batch_size` | 1000 | Increase to 10 000+ for faster imports; checkpoint less frequently |
| `max_row_size_bytes` | 0 (unlimited) | Set to 1–10 MB to guard against pathological rows |
| `max_statement_size_bytes` | 0 (unlimited) | Set to 1–10 MB to skip runaway statements |
| `enforce_utf8` | false | Enable when source encoding is uncertain |
| `continue_on_error` | true | Set to false for strict validation runs |
| `dry_run` | false | Use for pre-import validation without modifying data |
| `metrics_callback` | null | Wire to Prometheus / OTel for metric emission |
| `checkpoint_file` | "" (disabled) | Set to a file path to enable checkpoint/resume |

---

## Wiring Prometheus Metrics

The importer emits metrics via `ImportOptions.metrics_callback` with the following
standard metric names:

| Metric | Labels | Description |
| --- | --- | --- |
| `themisdb_import_rows_total` | `table`, `status` (imported/failed/skipped) | Row counter per table and outcome |
| `themisdb_import_duration_seconds` | – | Total import wall-clock time |
| `themisdb_import_errors_total` | `code` (ImportErrorCode as uint) | Error counter per error code |
| `themisdb_import_tables_total` | – | Number of tables processed |

**Example wiring to the existing `PrometheusMetrics` class:**

```cpp
#include "sharding/prometheus_metrics.h"

themis::sharding::PrometheusMetrics::Config cfg;
auto prom = std::make_shared<themis::sharding::PrometheusMetrics>(cfg);

ImportOptions opts;
opts.metrics_callback = [&prom](
    const std::string& metric,
    const std::map<std::string, std::string>& labels,
    double value
) {
    prom->addToCounter(metric, static_cast<int64_t>(value), labels);
};

auto stats = importer.importData(path, opts);
// Metrics are now visible at prom->getMetrics()
```

---

## Wiring Audit Logging

Import operations should be logged via the existing `AuditLogger`:

```cpp
#include "utils/audit_logger.h"

// At import start:
audit_logger.logSecurityEvent(
    SecurityEventType::BULK_IMPORT,
    operator_user_id,
    source_path,
    {{"source", source_path}, {"dry_run", options.dry_run}}
);

// At import completion:
audit_logger.logSecurityEvent(
    SecurityEventType::BULK_IMPORT_COMPLETED,
    operator_user_id,
    source_path,
    stats.toJson()
);
```

---

## Scenario 10: Handling Duplicate / Conflicting Rows

### Symptoms

- `ImportStats.conflicts_skipped`, `conflicts_overwritten`, or `conflicts_merged` > 0.
- Prometheus counter `importers_conflicts_total` is non-zero.
- With `ConflictStrategy::ERROR`: `ImportStats.structured_errors` contains entries with
  `ImportErrorCode::CONFLICT_ERROR` (600) and import may have stopped early.

### Diagnosis

Conflicts occur when two or more rows in the dump share the same value(s) in the columns
listed in `ImportOptions.conflict_key_columns`.  If `conflict_key_columns` is empty, no
conflict detection is performed.

```cpp
# Check conflict counts after import
stats.conflicts_skipped     // rows dropped by SKIP strategy
stats.conflicts_overwritten // rows replaced by OVERWRITE strategy
stats.conflicts_merged      // rows merged by MERGE strategy

// Check structured errors for ERROR strategy
for (const auto& err : stats.structured_errors) {
    if (err.code == ImportErrorCode::CONFLICT_ERROR) {
        // err.message contains the conflicting key and table name
        // err.location contains the row number or line number
    }
}
```

### Resolution

Choose the strategy that matches the desired semantics and set it in `ImportOptions`:

```cpp
ImportOptions opts;
opts.conflict_key_columns = {"id"};          // column(s) that identify a duplicate

// Strategy A: keep the first occurrence, discard later duplicates
opts.conflict_strategy = ConflictStrategy::SKIP;

// Strategy B: replace the earlier row with the later one (default)
opts.conflict_strategy = ConflictStrategy::OVERWRITE;

// Strategy C: merge fields from both rows; incoming wins unless field is protected
opts.conflict_strategy    = ConflictStrategy::MERGE;
opts.protected_fields     = {"created_at", "original_owner"};
opts.merge_depth          = 1;   // 1 = top-level only; -1 = deep recursive merge

// Strategy D: abort on first conflict (use continue_on_error to skip instead of abort)
opts.conflict_strategy    = ConflictStrategy::ERROR;
opts.continue_on_error    = false;  // hard abort on first conflict
```

### Tuning notes

- **Composite keys:** list multiple columns in `conflict_key_columns`; the key is formed
  by concatenating their string values with ASCII unit-separator (0x1F).
- **MERGE + protected_fields:** fields listed in `protected_fields` are never overwritten
  by the MERGE strategy even if the incoming row carries a different value.  Use this to
  preserve audit fields such as `created_at` or `inserted_by`.
- **MERGE depth:** `merge_depth = 1` (default) replaces nested objects wholesale;
  `merge_depth = -1` recurses into every level of nesting.  Use `-1` only when nested
  objects are additive (e.g., metadata maps); avoid it for arrays, which are always
  replaced entirely.
- **Throughput impact:** SKIP and OVERWRITE add a single hash-map lookup per row
  (< 5 % overhead).  MERGE adds field-by-field iteration; the overhead is proportional
  to the number of fields per row.

---

## Log Messages Reference

| Log message | Meaning |
| --- | --- |
| `Starting PostgreSQL import from: …` | Import started; Options JSON follows |
| `DRY RUN MODE - No data will be imported` | Dry-run is active |
| `Parsed table schema: <name>` | DDL for a table was successfully parsed |
| `Resuming import from byte offset <N>` | Checkpoint loaded; seeking to offset |
| `Checkpoint saved to <path>: byte_offset=<N>` | Checkpoint written after batch |
| `Could not parse checkpoint file …` | Checkpoint file corrupt; starting fresh |
| `Import summary: {…}` | Full `ImportStats` JSON on completion |
| `Import completed: N imported, N failed, N skipped in Xs` | Human-readable summary |

---

## See Also

- [Importers Roadmap](../roadmap/importers_roadmap.md)
- [Importer Interface](../include/importers/importer_interface.h)
- [PostgreSQL Importer Source](../src/importers/postgres_importer.cpp)
