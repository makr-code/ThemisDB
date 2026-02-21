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
- [I] MySQL / MariaDB importer (Target: Q2 2026) (Issue: #1835)
- [I] MongoDB importer for document collections (Target: Q2 2026) (Issue: #1836)
- [I] Import progress reporting with streaming callbacks (Target: Q3 2026) (Issue: #1864)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] SQLite importer (Issue: #1838)
- [I] CSV / TSV / Parquet flat-file importer (Issue: #1839)
- [I] S3-compatible object-storage source (Issue: #1840)
- [I] Schema auto-detection and validation on import (Issue: #1856)
- [I] Dry-run mode to preview import without writing data (Issue: #1989)

### Long-term (6-12 months)
- [I] Kafka consumer importer for real-time streaming ingestion (Issue: #1843)
- [I] Oracle Database importer (Issue: #1844)
- [I] Microsoft SQL Server importer (Issue: #1845)
- [I] Plugin API for third-party importer extensions (Issue: #1846)
- [I] GUI-based import wizard (web UI) (Issue: #1847)
- [I] Import conflict resolution strategies (skip, overwrite, merge) (Issue: #1848)

## Implementation Phases

### Phase 1: Core PostgreSQL Importer (Status: Completed ✅)
- [x] PostgreSQL importer (`importers/postgres_importer.cpp`) with connection pooling
- [x] Schema mapping and field-type transformation layer
- [x] Batch import operations with configurable chunk size
- [x] Incremental import support (watermark-based change tracking)
- [x] Custom import format handler registration API
- [x] Import pipeline infrastructure (source → transform → sink)

### Phase 2: Streaming & Conflict Resolution (Status: In Progress 🚧)
- [I] Streaming import for large datasets without full in-memory load (Target: Q2 2026) (Issue: #1863)
- [~] Import progress reporting with streaming callbacks (Target: Q2 2026)
- [I] Conflict resolution strategies: skip, overwrite, merge (Target: Q3 2026) (Issue: #1849)
- [ ] Dry-run mode to preview import without writing data (Target: Q3 2026)

### Phase 3: Multi-Source & Plugin API (Status: Planned 📋)
- [I] MySQL / MariaDB importer (`importers/mysql_importer.cpp`) with JDBC-compatible config (Issue: #1851)
- [I] MongoDB importer (`importers/mongo_importer.cpp`) for document collections (Issue: #1852)
- [I] Flat-file CSV / TSV / Parquet importer with schema auto-detection (Issue: #1853)
- [I] Plugin API for third-party importer extensions (`importers/importer_plugin_api.h`) (Issue: #1854)
- [I] S3-compatible object-storage source connector (Issue: #1855)
- [ ] Schema auto-detection and validation on import

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1857)
- [I] Integration tests against live PostgreSQL (Issue: #1858)
- [I] Performance benchmarks (rows/sec, GB/hr) (Issue: #1859)
- [I] Security audit (SQL injection, credential handling) (Issue: #1860)
- [I] Documentation complete (Issue: #1861)
- [I] API stability guaranteed (Issue: #1862)

## Known Issues & Limitations
- Only PostgreSQL is supported as a source database at this time.
- Binary/blob field types may require manual mapping.
- No distributed parallel import across multiple nodes.

## Breaking Changes
- Importer plugin API will be stabilised in v1.5.0; breaking changes expected before that milestone.
