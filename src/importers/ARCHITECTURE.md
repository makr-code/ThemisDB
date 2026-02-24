# Importers Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-02-24  
**Module Path:** `src/importers/`

---

## 1. Overview

The Importers module provides one-time and incremental data import from external database
systems into ThemisDB. It handles schema discovery, schema mapping to ThemisDB's multi-model
layout, batch data transfer, and incremental sync via change tracking.

Current connectors: PostgreSQL (production-ready), MySQL and MongoDB (in progress).

---

## 2. Design Principles

- **Schema Mapping First** – the importer translates source schema to ThemisDB schema
  before any data is transferred; this allows validation and preview without side effects.
- **Batch-Oriented** – large imports are split into configurable batches to limit memory
  usage and enable resumption on failure.
- **Incremental Support** – after initial import, incremental runs transfer only changed
  rows using source-side change tracking (last-modified timestamp or WAL-based CDC).
- **Source-Agnostic Pipeline** – `import_pipeline.cpp` orchestrates all importers; adding
  a new source requires only a new connector, not pipeline changes.
- **Quarantine on Error** – rows that fail validation or conversion are written to a
  quarantine table rather than causing the import to abort.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `postgres_importer.cpp` | PostgreSQL source connector: schema discovery, data transfer |
| `mysql_importer.cpp` | MySQL source connector (in progress) |
| `mongo_importer.cpp` | MongoDB source connector (in progress) |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                  Import API (src/server/)                        │
│   POST /import  { source: "postgres", config: {...} }            │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     Import Pipeline                              │
│                                                                  │
│  1. SchemaMapper: discover source schema → ThemisDB schema       │
│  2. Validate mapping (user confirmation or auto-approve)         │
│  3. Batch iterator: fetch records in batches                     │
│  4. Transform + validate each record                             │
│  5. Write to ThemisDB storage/index                              │
│  6. Quarantine failed records                                    │
└──────────────────────────┬──────────────────────────────────────┘
                           │ per-source connector
         ┌─────────────────┴───────────────────┐
         │                                     │
┌────────▼──────────┐             ┌────────────▼──────────────────┐
│ PostgresImporter  │             │ MySQLImporter / MongoImporter  │
│ libpq connection  │             │ (in progress)                  │
│ schema discovery  │             └───────────────────────────────┘
│ batch cursor      │
└───────────────────┘
```

---

## 4. Data Flow

### 4.1 Initial Import

```
ImportRequest { source: "postgres", dsn: "...", tables: ["users", "orders"] }
    │
    ▼
PostgresImporter::discoverSchema(tables)
    → {tables, columns, types, primary_keys, foreign_keys}
    │
    ▼
SchemaMapper::map(source_schema) → ThemisDB collection schema
    │
    ▼
User confirms mapping (or auto-approve via config)
    │
    ▼
Import loop:
    batch_cursor.next(batch_size) → [rows]
    for each row:
        ├─ transform types (postgres → ThemisDB types)
        ├─ validate (not-null, type constraints)
        ├─ valid → write to storage
        └─ invalid → write to quarantine_table
    │
    ▼
Import complete: {imported: N, quarantined: M, duration: Xs}
```

### 4.2 Incremental Import

```
IncrementalImportRequest { last_cursor: "2026-01-01T00:00:00Z" }
    │
    ▼
PostgresImporter::fetchChangedRows(since: last_cursor)
    → rows modified after cursor
    │
    ▼
Same transform/validate/write pipeline
    │
    ▼
Update cursor to current timestamp
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Writes to** | `src/storage/` | Batch writes via `RocksDBWrapper` |
| **Registers with** | `src/index/` | Secondary index updates during import |
| **Uses** | `src/metadata/` | Schema registration after mapping |
| **Consumed by** | `src/server/` | Import API endpoints |

---

## 6. Threading & Concurrency Model

- Each import job runs on a dedicated background thread.
- Concurrent imports from different sources are supported; each uses an independent connection.
- Batch writes use ThemisDB transactions for atomicity per batch.
- Quarantine writes are separate transactions to avoid blocking the main import.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Batch fetching | Configurable batch size (default: 1000 rows) reduces round-trips |
| Parallel table import | Multiple tables imported concurrently (configurable parallelism) |
| Server-side cursor | PostgreSQL server-side cursor avoids loading entire table into memory |

---

## 8. Security Considerations

- Source database credentials are never logged; stored only in memory for the duration
  of the import.
- Connection strings are validated to prevent injection (SSRF, etc.).
- Import is scoped to the authenticated user's tenant; cross-tenant imports are rejected.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `importers.batch_size` | 1000 | Records per batch |
| `importers.parallel_tables` | 4 | Concurrent table imports |
| `importers.quarantine_enabled` | true | Enable quarantine on error |
| `importers.auto_approve_mapping` | false | Skip user confirmation for schema mapping |
| `importers.incremental.strategy` | "timestamp" | Incremental mode: timestamp / wal-cdc |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Connection failure | Abort import; return structured error |
| Schema discovery failure | Abort import; return error |
| Row transformation failure | Quarantine row; continue batch |
| Batch write failure | Retry batch (up to 3 times); then abort import |

---

## 11. Known Limitations & Future Work

- MySQL and MongoDB importers are in progress.
- WAL-based CDC incremental import (PostgreSQL logical replication) is planned.
- Flat-file importers (CSV, JSON Lines) are planned.
- Schema migration on re-import (schema drift detection) is not yet implemented.

---

## 12. References

- `src/importers/README.md` — module overview
- `docs/importers_roadmap.md` — roadmap
- `docs/importers_runbook.md` — operational runbook
- `ARCHITECTURE.md` (root) — full system architecture
