# Importers Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Functional data import pipeline with PostgreSQL support; early-stage production readiness.

## Completed ✅
- [x] PostgreSQL importer
- [x] Schema mapping and transformation
- [x] Batch import operations
- [x] Incremental import support
- [x] Custom import format handlers
- [x] Import pipeline infrastructure

## In Progress 🚧
- [ ] MySQL / MariaDB importer (Target: Q2 2026)
- [ ] MongoDB importer for document collections (Target: Q2 2026)
- [ ] Import progress reporting with streaming callbacks (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] SQLite importer
- [ ] CSV / TSV / Parquet flat-file importer
- [ ] S3-compatible object-storage source
- [ ] Schema auto-detection and validation on import
- [ ] Dry-run mode to preview import without writing data

### Long-term (6-12 months)
- [ ] Kafka consumer importer for real-time streaming ingestion
- [ ] Oracle Database importer
- [ ] Microsoft SQL Server importer
- [ ] Plugin API for third-party importer extensions
- [ ] GUI-based import wizard (web UI)
- [ ] Import conflict resolution strategies (skip, overwrite, merge)

## Implementation Phases

### Phase 1: Core PostgreSQL Importer (Status: Completed ✅)
- [x] PostgreSQL importer (`importers/postgres_importer.cpp`) with connection pooling
- [x] Schema mapping and field-type transformation layer
- [x] Batch import operations with configurable chunk size
- [x] Incremental import support (watermark-based change tracking)
- [x] Custom import format handler registration API
- [x] Import pipeline infrastructure (source → transform → sink)

### Phase 2: Streaming & Conflict Resolution (Status: In Progress 🚧)
- [~] Streaming import for large datasets without full in-memory load (Target: Q2 2026)
- [~] Import progress reporting with streaming callbacks (Target: Q2 2026)
- [ ] Conflict resolution strategies: skip, overwrite, merge (Target: Q3 2026)
- [ ] Dry-run mode to preview import without writing data (Target: Q3 2026)

### Phase 3: Multi-Source & Plugin API (Status: Planned 📋)
- [ ] MySQL / MariaDB importer (`importers/mysql_importer.cpp`) with JDBC-compatible config
- [ ] MongoDB importer (`importers/mongo_importer.cpp`) for document collections
- [ ] Flat-file CSV / TSV / Parquet importer with schema auto-detection
- [ ] Plugin API for third-party importer extensions (`importers/importer_plugin_api.h`)
- [ ] S3-compatible object-storage source connector
- [ ] Schema auto-detection and validation on import

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests against live PostgreSQL
- [ ] Performance benchmarks (rows/sec, GB/hr)
- [ ] Security audit (SQL injection, credential handling)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Only PostgreSQL is supported as a source database at this time.
- Binary/blob field types may require manual mapping.
- No distributed parallel import across multiple nodes.

## Breaking Changes
- Importer plugin API will be stabilised in v1.5.0; breaking changes expected before that milestone.
