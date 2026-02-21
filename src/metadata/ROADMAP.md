# Metadata Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Production-ready schema introspection layer with thread-safe caching, information schema views, and statistics collection.

## Completed ✅
- [x] SchemaManager – automatic table discovery via RocksDB key scanning
- [x] Property type detection from stored entities
- [x] Index metadata collection from IndexManager
- [x] Relationship discovery (graph edges and foreign keys)
- [x] Thread-safe metadata cache with configurable TTL (default 60 s)
- [x] SystemCatalog – table, column, index, and statistics metadata persistence
- [x] INFORMATION_SCHEMA views (tables, columns, indexes, statistics)
- [x] StatisticsCollector – cardinality, selectivity, and data distribution
- [x] Schema version tracking and change history
- [x] Lazy loading and incremental updates
- [x] AQL integration for metadata queries

## In Progress 🚧
- [I] Schema diff and migration script generation (Target: Q2 2026) (Issue: #1946)
- [I] Real-time schema change notifications via changefeeds (Target: Q2 2026) (Issue: #1947)
- [I] Adaptive TTL based on table mutation rate (Target: Q3 2026) (Issue: #1948)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Column-level statistics histograms for improved query planning (Issue: #1949)
- [I] Cross-collection relationship graph (ER diagram export) (Issue: #1993)
- [?] Metadata API endpoint (`GET /api/v1/schema`)
- [?] Schema validation against user-defined constraints
- [?] Index usage tracking (which indexes are queried most)

### Long-term (6-12 months)
- [I] Distributed metadata catalog across shards (Issue: #1961)
- [?] Schema registry with compatibility enforcement (forward/backward)
- [?] Auto-generated OpenAPI schema from stored documents
- [?] Integration with external data catalogs (Apache Atlas, DataHub)
- [?] Column lineage and data provenance tracking

## Implementation Phases

### Phase 1: Schema Introspection & Catalog (Status: Completed ✅)
- [x] SchemaManager: automatic table discovery via RocksDB key scanning (`metadata/schema_manager.cpp`)
- [x] Property type detection from stored entities
- [x] Index metadata collection from IndexManager
- [x] Relationship discovery (graph edges and foreign keys)
- [x] Thread-safe metadata cache with configurable TTL (default 60 s)
- [x] SystemCatalog: table, column, index, and statistics metadata persistence (`metadata/system_catalog.cpp`)
- [x] INFORMATION_SCHEMA views: tables, columns, indexes, statistics
- [x] StatisticsCollector: cardinality, selectivity, and data distribution
- [x] Schema version tracking and change history
- [x] Lazy loading, incremental updates, and AQL integration

### Phase 2: Live Schema Changes & Adaptive Caching (Status: In Progress 🚧)
- [~] Schema diff and migration script generation (Target: Q2 2026)
- [~] Real-time schema change notifications via changefeeds (Target: Q2 2026)
- [ ] Adaptive TTL based on table mutation rate (Target: Q3 2026)

### Phase 3: Distributed Catalog & Lineage (Status: Planned 📋)
- [ ] Column-level statistics histograms for improved query planning
- [ ] Distributed metadata catalog across shards
- [?] Schema registry with forward/backward compatibility enforcement
- [ ] Cross-collection relationship graph (ER diagram export)
- [ ] Column lineage and data provenance tracking
- [ ] Integration with external data catalogs (Apache Atlas, DataHub)

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (schema discovery, INFORMATION_SCHEMA queries)
- [?] Performance benchmarks (cache hit rate, scan latency)
- [?] Security audit (metadata access control, information disclosure)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- Full table scan required on first load; large databases may experience slow initial discovery.
- Statistics are approximate; histogram support planned for v1.5.0.
- Schema version history is stored in-memory; persistence across restarts is limited.

## Breaking Changes
- INFORMATION_SCHEMA view column names follow SQL standard; no planned breaking changes.
- `SchemaManager` public API is stable from v1.x.
