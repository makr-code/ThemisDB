> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Metadata Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.6.0 – Full-featured, production-ready metadata layer with RBAC security interfaces.
All Phase 1–4 items are complete.
Schema introspection, statistics with equi-height histograms, changefeed notifications, adaptive
TTL, audit log, consistency checker, ER diagram export, external catalog integration (Apache Atlas,
DataHub), column lineage, distributed catalog, the Schema API REST endpoint, and the pluggable
RBAC/access-control and observer interfaces are all shipped.

## Completed ✅
- [x] SchemaManager – automatic table discovery via RocksDB key scanning
- [x] Property type detection from stored entities
- [x] Index metadata collection from IndexManager
- [x] Relationship discovery (graph edges and foreign keys)
- [x] Thread-safe metadata cache with configurable TTL (default 60 s)
- [x] SystemCatalog – table, column, index, and statistics metadata persistence (implemented within `schema_manager.cpp`)
- [x] INFORMATION_SCHEMA views (tables, columns, indexes, statistics)
- [x] StatisticsCollector – cardinality, selectivity, equi-height histograms, and data distribution
- [x] Schema version tracking, change history, diff, and migration script generation (Issue: #1946)
- [x] Real-time schema change notifications via changefeeds (Issue: #1947)
- [x] Adaptive TTL based on table mutation rate (Issue: #1948)
- [x] Column-level statistics histograms for improved query planning (Issue: #1949)
- [x] Cross-collection relationship graph – ER diagram export (Mermaid, DOT, JSON) (Issue: #1993)
- [x] Metadata API endpoint (`GET /api/v1/schema`, lineage, audit) via SchemaApiHandler
- [x] Schema validation against user-defined constraints (SchemaConstraints)
- [x] Index usage tracking and auto-recommendations (IndexRecommender)
- [x] Schema audit log (SchemaAuditLog – durable, per-table audit trail in RocksDB)
- [x] Schema consistency checker (SchemaConsistencyChecker – background health scan)
- [x] Distributed metadata catalog across shards (DistributedMetadataCatalog) (Issue: #1961)
- [x] Integration with external data catalogs – Apache Atlas and DataHub (Issue: #2414)
- [x] Column lineage and data provenance tracking (ColumnLineageTracker)
- [x] Lazy loading and incremental updates
- [x] AQL integration for metadata queries

## Planned Features 📋

### Long-term (> 12 months)
- [ ] Auto-generated OpenAPI schema from stored documents (Target: v2.0 / Q3 2027)
- [x] Schema migration validation via `validateMigration` API
- [ ] Explicit compat-mode policy enforcement (forward/backward) (Target: v1.9 / Q1 2027)

## Implementation Phases

### Phase 1: Schema Introspection & Catalog (Status: Completed ✅)
- [x] SchemaManager: automatic table discovery via RocksDB key scanning (`metadata/schema_manager.cpp`)
- [x] Property type detection from stored entities
- [x] Index metadata collection from IndexManager
- [x] Relationship discovery (graph edges and foreign keys)
- [x] Thread-safe metadata cache with configurable TTL (default 60 s)
- [x] SystemCatalog: table, column, index, and statistics metadata persistence (implemented within `metadata/schema_manager.cpp`; no separate `system_catalog.cpp`)
- [x] INFORMATION_SCHEMA views: tables, columns, indexes, statistics
- [x] StatisticsCollector: cardinality, selectivity, and data distribution
- [x] Schema version tracking and change history
- [x] Lazy loading, incremental updates, and AQL integration

### Phase 2: Live Schema Changes & Adaptive Caching (Status: Completed ✅)
- [x] Schema diff and migration script generation (`schema_version_manager.cpp`)
- [x] Real-time schema change notifications via changefeeds (`schema_manager.cpp::setChangefeed`)
- [x] Adaptive TTL based on table mutation rate (`schema_manager.cpp::enableAdaptiveTTL`)

### Phase 3: Distributed Catalog & Lineage (Status: Completed ✅)
- [x] Column-level statistics histograms for improved query planning (`statistics_collector.cpp`)
- [x] Distributed metadata catalog across shards (`distributed_catalog.cpp`)
- [x] Cross-collection relationship graph – ER diagram export (`er_diagram_exporter.cpp`)
- [x] Column lineage and data provenance tracking (`column_lineage.cpp`)
- [x] Integration with external data catalogs – Apache Atlas and DataHub (`catalog_exporter.cpp`)
- [x] Schema audit log – durable per-table audit trail (`schema_audit_log.cpp`)
- [x] Schema consistency checker – background health scan (`schema_consistency_checker.cpp`)

### Phase 4: Security & Extensibility Interfaces (Status: Completed ✅)
- [x] `IMetadataSecurityProvider`: pluggable RBAC / access-control interface for all metadata operations
  (`include/metadata/imetadata_security_provider.h`)
- [x] `NoOpMetadataSecurityProvider`: default permit-all implementation (zero overhead)
- [x] `InMemoryRbacMetadataSecurityProvider`: thread-safe in-memory RBAC with grant/revoke/revokeAll,
  wildcard `"*"` resource, ADMIN-implies-all, `MetadataAccessDeniedException`
- [x] `IMetadataChangeListener`: observer interface for schema change events with six event types
  (`include/metadata/imetadata_change_listener.h`)
- [x] `RecordingMetadataChangeListener`: thread-safe in-memory recording listener with callback,
  FIFO ordering, `lastEvent()`, `clear()`
- [x] `MetadataChangeEvent`: structured event with `toJSON()` serialisation
- [x] `IMetadataExportPolicy`: pluggable export-policy interface for external catalog integration
  (`include/metadata/imetadata_export_policy.h`)
- [x] `AlwaysExportPolicy`, `NeverExportPolicy`, `FilteredExportPolicy` implementations
- [x] 49 acceptance-criteria tests across three focused executables
- [x] CI workflow `metadata-interfaces-ci.yml` (ubuntu-22.04 + ubuntu-24.04)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (test_schema_manager, test_statistics_collector, test_information_schema,
  test_schema_version_manager, test_schema_constraints, test_schema_changefeed,
  test_schema_audit_log, test_schema_consistency_checker, test_column_lineage,
  test_catalog_exporter, test_er_diagram_exporter, test_distributed_catalog,
  test_index_recommender, test_statistics_auto_refresh,
  test_metadata_security_provider, test_metadata_change_listener,
  test_metadata_export_policy, …)
- [x] Integration tests (test_information_schema, test_schema_changefeed, test_schema_api_lineage)
- [x] Performance benchmarks (cache hit rate, scan latency) – `benchmarks/bench_metadata_cache.cpp` (META-MISSING-001)
- [x] Security audit (metadata access control, information disclosure) – `IMetadataSecurityProvider` + `InMemoryRbacMetadataSecurityProvider` (META-MISSING-002, v1.6.0)
- [x] Documentation complete (README.md in src/metadata/ and include/metadata/, ARCHITECTURE.md,
  FUTURE_ENHANCEMENTS.md)
- [x] API stability guaranteed (SchemaManager public API stable from v1.x; no breaking changes planned)

## Known Issues & Limitations
- Full table scan required on first load; large databases may experience slow initial discovery
  (< 30 s target for up to 10 M keys).
- Statistics are approximate (sample-based); equi-height histogram accuracy is within ±20% of
  true cardinality for uniform and skewed distributions.
- Schema version history is persisted to RocksDB; bounded to last 1,000 versions in-memory to
  prevent unbounded growth.
- `validateMigration` checks basic structural consistency; a full compat-mode policy engine
  (explicit forward/backward enforcement) is planned for v1.9 / Q1 2027.

## Breaking Changes
- INFORMATION_SCHEMA view column names follow SQL standard; no planned breaking changes.
- `SchemaManager` public API is stable from v1.x.
