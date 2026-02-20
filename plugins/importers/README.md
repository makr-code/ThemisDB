# Importer Plugins

## Status: ⚠️ Partial – Production Hardening In Progress

Data import plugins for ThemisDB.

## Available Importers

### PostgreSQL Importer ⚠️
**Path:** `postgres/`

**Status:** Core functionality implemented; production hardening ongoing (see roadmap).

**Implementation:** `src/importers/postgres_importer.cpp`

Import data from PostgreSQL `pg_dump` (SQL format) files into ThemisDB.

## Features

- DDL parsing (`CREATE TABLE`, `CREATE SCHEMA`) via regex heuristics
- DML parsing (`INSERT INTO ... VALUES`) with full value tokenisation
- `COPY ... FROM stdin` parsing with correct tab/escape handling
- Schema mapping and transformation (column/table/type overrides)
- Batch import operations with configurable batch size
- Structured error reporting (`ImportErrorCode`, `ImportErrorSeverity`)
- User-configurable PostgreSQL → ThemisDB type override map
- Cancellation support via `cancel()`

## Known Limitations

- SQL parsing is regex/heuristic-based; complex nested DDL or non-standard extensions
  may not be handled correctly.
- Single-threaded, synchronous processing only; no streaming or checkpoint/resume.
- No Prometheus/OTel metrics export yet.
- See [Importers Roadmap](../../docs/importers_roadmap.md) for the full gap list and
  planned improvements.

## Documentation

For importer documentation, see:
- [Importers Roadmap](../../docs/importers_roadmap.md)
- [Importer Interface](../../include/importers/importer_interface.h)
