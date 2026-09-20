# METADATA DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\metadata\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\metadata\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 69
- Compounds: 194
- Classes/Structs: 97
- Namespaces: 20
- File Compounds: 69

## Namespaces
- @005112171367161003033362072265145314030333375336
- @014306126241202265340367173125224141245245012137
- @016324355004056132053352175161342063223163252023
- @043064017117253256345033077052030354147260165242
- @045234041246347246101107217244163227350106320205
- @101061077023177027310311252371002261244154021037
- @143214033336365370354032207241042023100176313050
- @166044037042151274240122313023355346034043056102
- @254224336157210270106113270157113341013312116276
- @271131231175165052146366014311245037321044203216
- std
- std::chrono_literals
- testing
- themis
- themis::@326035104315223212044311016013216254237056264237
- themis::aql
- themis::errors
- themis::metadata
- themis::observability
- themisdb::sharding

## Types
### Classes
- CatalogExporterAtlasTest
- CatalogExporterDataHubTest
- ColumnLineageTrackerTest
- DistributedCatalogSyncTest
- DistributedMetadataCatalogTest
- DryRunMigrationTest
- IndexRecommenderCostModelTest
- IndexRecommenderMetricTest
- IndexRecommenderPersistTest
- IndexRecommenderTest
- MetadataShardRouterTest
- MetadataShardTest
- MetadataWALTest
- MigrationRegressionTest
- MigrationScriptTest
- SchemaAuditLogTest
- SchemaConsistencyCheckerTest
- SchemaConstraintsPersistenceTest
- SchemaConstraintsTest
- SchemaManagerTest
- SchemaVersionManagerAuditTest
- SchemaVersionManagerTest
- StatisticsAutoRefreshTest
- StatisticsCollectorTest
- StubMetadataTable
- themis::CatalogExporter
- themis::DistributedMetadataCatalog
- themis::ERDiagramExporter
- themis::InformationSchema
- themis::SchemaAuditLog
- themis::SchemaConsistencyChecker
- themis::SchemaConstraints
- themis::SchemaManager
- themis::SchemaVersionManager
- themis::StatisticsCollector
- themis::metadata::AlwaysExportPolicy
- themis::metadata::ColumnLineageTracker
- themis::metadata::FieldSetMetadataEncryptionProvider
- themis::metadata::FilteredExportPolicy
- themis::metadata::IMetadataChangeListener
- themis::metadata::IMetadataEncryptionProvider
- themis::metadata::IMetadataExportPolicy
- themis::metadata::IMetadataSecurityProvider
- themis::metadata::IMetadataSnapshotStore
- themis::metadata::InMemoryMetadataSnapshotStore
- themis::metadata::InMemoryRbacMetadataSecurityProvider
- themis::metadata::IndexRecommender
- themis::metadata::MetadataAccessDeniedException
- themis::metadata::MetadataEncryptionException
- themis::metadata::MetadataSnapshotException
- themis::metadata::NeverExportPolicy
- themis::metadata::NoOpMetadataEncryptionProvider
- themis::metadata::NoOpMetadataSecurityProvider
- themis::metadata::RecordingMetadataChangeListener
- themis::metadata::SchemaDiffEngine

### Structs
- CatalogExporterAtlasTest::Capture
- CatalogExporterDataHubTest::Capture
- ColumnSpec
- CountingHook
- IndexSpec
- RecordingHook
- themis::AdaptiveTTLConfig
- themis::CatalogExporter::Config
- themis::CatalogExporter::PublishResult
- themis::ColumnConstraint
- themis::ColumnStats
- themis::ConsistencyIssue
- themis::ConstraintViolation
- themis::HistogramBucket
- themis::ISColumn
- themis::ISKeyColumnUsage
- themis::ISReferentialConstraint
- themis::ISStatistic
- themis::ISTable
- themis::IndexStats
- themis::SchemaAuditEntry
- themis::SchemaChange
- themis::SchemaManager::DatabaseMetadata
- themis::SchemaManager::IndexInfo
- themis::SchemaManager::PropertyInfo
- themis::SchemaManager::RelationshipSchema
- themis::SchemaManager::TableSchema
- themis::StatisticsCollector::IMetricsHook
- themis::StatsResult
- themis::TableStats
- themis::VersionResult
- themis::metadata::ColumnAccess
- themis::metadata::ColumnDiff
- themis::metadata::ColumnLineageEntry
- themis::metadata::ColumnLineageRecord
- themis::metadata::ColumnRef
- themis::metadata::ColumnRefHash
- themis::metadata::IndexDiff
- themis::metadata::IndexRecommendation
- themis::metadata::MetadataChangeEvent
- themis::metadata::MetadataSnapshot
- themis::metadata::SchemaDiff

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 982

### CatalogExporterAtlasTest

#### `void SetUp() override`
- Source: `tests/metadata/test_catalog_exporter.cpp`:94
- Brief: n/a
- Parameters: none

#### `void setHttpMock(int status_code, const std::string &response_body="{}")`
- Source: `tests/metadata/test_catalog_exporter.cpp`:81
- Brief: Install a test-double that records calls and returns the configured response.
- Parameters:
  - `status_code` (int): n/a
  - `response_body` (const std::string &): n/a

### CatalogExporterDataHubTest

#### `void SetUp() override`
- Source: `tests/metadata/test_catalog_exporter.cpp`:259
- Brief: n/a
- Parameters: none

#### `void setHttpMock(int status_code, const std::string &response_body="")`
- Source: `tests/metadata/test_catalog_exporter.cpp`:246
- Brief: n/a
- Parameters:
  - `status_code` (int): n/a
  - `response_body` (const std::string &): n/a

### ColumnLineageTrackerTest

#### `ColumnLineageEntry makeEntry(const std::string &src_table, const std::string &src_col, const std::string &tgt_table, const std::string &tgt_col, TransformationType type=TransformationType::DIRECT_COPY, int64_t ts=1000LL)`
- Source: `tests/metadata/test_column_lineage.cpp`:151
- Brief: Helper: build a simple entry with one source.
- Parameters:
  - `src_table` (const std::string &): n/a
  - `src_col` (const std::string &): n/a
  - `tgt_table` (const std::string &): n/a
  - `tgt_col` (const std::string &): n/a
  - `type` (TransformationType): n/a
  - `ts` (int64_t): n/a

### CountingHook

#### `void onCacheHit(std::string_view) override`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:154
- Brief: Called whenever in-memory cache satisfies a getStats() request.
- Parameters:
  - `table_name` (std::string_view): n/a

#### `void onCacheMiss(std::string_view) override`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:155
- Brief: Called whenever getStats() results in a cache miss (loads from RocksDB or re-collects).
- Parameters:
  - `table_name` (std::string_view): n/a

#### `void onCollect(std::string_view, double, size_t, bool success) override`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:147
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): Affected table
  - `duration_ms` (double): Wall-clock duration in milliseconds
  - `rows_sampled` (size_t): Number of rows actually scanned
  - `success` (bool): Whether collection succeeded
- Details: Called after a successful or failed stats collection attempt. table_name Affected table duration_ms Wall-clock duration in milliseconds rows_sampled Number of rows actually scanned success Whether collection succeeded

#### `void onError(std::string_view, int) override`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:156
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): n/a
  - `error_code` (int): StatsErrorCode cast to int
- Details: Called on any internal error (iterator failure, parse error, etc.). error_code StatsErrorCode cast to int

### DistributedCatalogSyncTest

#### `void SetUp() override`
- Source: `tests/metadata/test_distributed_catalog.cpp`:198
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_distributed_catalog.cpp`:220
- Brief: n/a
- Parameters: none

### DistributedMetadataCatalogTest

#### `void SetUp() override`
- Source: `tests/metadata/test_distributed_catalog.cpp`:81
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_distributed_catalog.cpp`:88
- Brief: n/a
- Parameters: none

### DryRunMigrationTest

#### `void SetUp() override`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:25
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:36
- Brief: n/a
- Parameters: none

#### `SchemaManager::TableSchema makeSchema(const std::string &name, const std::vector< std::string > &columns) const`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:43
- Brief: Build a simple TableSchema with given name and column names.
- Parameters:
  - `name` (const std::string &): n/a
  - `columns` (const std::vector< std::string > &): n/a

### IndexRecommenderCostModelTest

#### `void SetUp() override`
- Source: `tests/metadata/test_index_recommender.cpp`:460
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_index_recommender.cpp`:467
- Brief: n/a
- Parameters: none

#### `void seedTableStats(const std::string &table, size_t row_count, const std::string &col, double selectivity)`
- Source: `tests/metadata/test_index_recommender.cpp`:473
- Brief: n/a
- Parameters:
  - `table` (const std::string &): n/a
  - `row_count` (size_t): n/a
  - `col` (const std::string &): n/a
  - `selectivity` (double): n/a

### IndexRecommenderMetricTest

#### `void SetUp() override`
- Source: `tests/metadata/test_index_recommender.cpp`:591
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_index_recommender.cpp`:594
- Brief: n/a
- Parameters: none

### IndexRecommenderPersistTest

#### `void SetUp() override`
- Source: `tests/metadata/test_index_recommender.cpp`:289
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_index_recommender.cpp`:296
- Brief: n/a
- Parameters: none

### MetadataShardRouterTest

#### `void SetUp() override`
- Source: `tests/metadata/test_metadata_shard.cpp`:230
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_metadata_shard.cpp`:251
- Brief: n/a
- Parameters: none

### MetadataShardTest

#### `void SetUp() override`
- Source: `tests/metadata/test_metadata_shard.cpp`:12
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_metadata_shard.cpp`:32
- Brief: n/a
- Parameters: none

### MetadataWALTest

#### `void SetUp() override`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:18
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:26
- Brief: n/a
- Parameters: none

### MigrationRegressionTest

#### `void SetUp() override`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:40
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:54
- Brief: n/a
- Parameters: none

#### `SchemaManager::TableSchema buildSchema(const std::string &table, const std::vector< std::string > &columns)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:64
- Brief: n/a
- Parameters:
  - `table` (const std::string &): n/a
  - `columns` (const std::vector< std::string > &): n/a

#### `uint64_t registerAndVersion(const std::string &table, const std::vector< std::string > &columns, const std::string &author, const std::string &desc)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:80
- Brief: n/a
- Parameters:
  - `table` (const std::string &): n/a
  - `columns` (const std::vector< std::string > &): n/a
  - `author` (const std::string &): n/a
  - `desc` (const std::string &): n/a

### MigrationScriptTest

#### `void SetUp() override`
- Source: `tests/metadata/test_schema_migration_script.cpp`:30
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_schema_migration_script.cpp`:42
- Brief: n/a
- Parameters: none

#### `SchemaManager::TableSchema makeSchema(const std::string &table, const std::vector< std::string > &cols, const std::string &type="string", bool nullable=true) const`
- Source: `tests/metadata/test_schema_migration_script.cpp`:52
- Brief: Build a TableSchema with the given columns (all type "string", nullable).
- Parameters:
  - `table` (const std::string &): n/a
  - `cols` (const std::vector< std::string > &): n/a
  - `type` (const std::string &): n/a
  - `nullable` (bool): n/a

#### `uint64_t snapshot(const std::string &table, const SchemaManager::TableSchema &ts, const std::string &author="test", const std::string &desc="")`
- Source: `tests/metadata/test_schema_migration_script.cpp`:72
- Brief: Register schema in SchemaManager and create a version snapshot.
- Parameters:
  - `table` (const std::string &): n/a
  - `ts` (const SchemaManager::TableSchema &): n/a
  - `author` (const std::string &): n/a
  - `desc` (const std::string &): n/a

### RecordingHook

#### `void onCacheHit(std::string_view) override`
- Source: `tests/metadata/test_statistics_collector.cpp`:620
- Brief: Called whenever in-memory cache satisfies a getStats() request.
- Parameters:
  - `table_name` (std::string_view): n/a

#### `void onCacheMiss(std::string_view) override`
- Source: `tests/metadata/test_statistics_collector.cpp`:621
- Brief: Called whenever getStats() results in a cache miss (loads from RocksDB or re-collects).
- Parameters:
  - `table_name` (std::string_view): n/a

#### `void onCollect(std::string_view, double, size_t, bool success) override`
- Source: `tests/metadata/test_statistics_collector.cpp`:613
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): Affected table
  - `duration_ms` (double): Wall-clock duration in milliseconds
  - `rows_sampled` (size_t): Number of rows actually scanned
  - `success` (bool): Whether collection succeeded
- Details: Called after a successful or failed stats collection attempt. table_name Affected table duration_ms Wall-clock duration in milliseconds rows_sampled Number of rows actually scanned success Whether collection succeeded

#### `void onError(std::string_view, int) override`
- Source: `tests/metadata/test_statistics_collector.cpp`:622
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): n/a
  - `error_code` (int): StatsErrorCode cast to int
- Details: Called on any internal error (iterator failure, parse error, etc.). error_code StatsErrorCode cast to int

### SchemaAuditLogTest

#### `void SetUp() override`
- Source: `tests/metadata/test_schema_audit_log.cpp`:25
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_schema_audit_log.cpp`:34
- Brief: n/a
- Parameters: none

### SchemaConsistencyCheckerTest

#### `void SetUp() override`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:28
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:41
- Brief: n/a
- Parameters: none

#### `void insertRow(const std::string &table, const std::string &id)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:59
- Brief: n/a
- Parameters:
  - `table` (const std::string &): n/a
  - `id` (const std::string &): n/a

#### `void registerTable(const std::string &name, const std::vector< std::string > &cols={"id", "value"})`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:47
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `cols` (const std::vector< std::string > &): n/a

### SchemaConstraintsPersistenceTest

#### `void SetUp() override`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:21
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:30
- Brief: n/a
- Parameters: none

### SchemaManagerTest

#### `void SetUp() override`
- Source: `tests/metadata/test_schema_manager.cpp`:29
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_schema_manager.cpp`:43
- Brief: n/a
- Parameters: none

### SchemaVersionManagerAuditTest

#### `void SetUp() override`
- Source: `tests/metadata/test_schema_version_manager.cpp`:335
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_schema_version_manager.cpp`:352
- Brief: n/a
- Parameters: none

#### `void registerSchema(const std::string &name)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:358
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

### SchemaVersionManagerTest

#### `void SetUp() override`
- Source: `tests/metadata/test_schema_version_manager.cpp`:25
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_schema_version_manager.cpp`:36
- Brief: n/a
- Parameters: none

#### `void registerSchema(const std::string &table_name)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:43
- Brief: Insert a simple schema for testing.
- Parameters:
  - `table_name` (const std::string &): n/a

### StatisticsAutoRefreshTest

#### `void SetUp() override`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:26
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:35
- Brief: n/a
- Parameters: none

#### `void insertRows(const std::string &table, int count)`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:41
- Brief: n/a
- Parameters:
  - `table` (const std::string &): n/a
  - `count` (int): n/a

### StatisticsCollectorTest

#### `void SetUp() override`
- Source: `tests/metadata/test_statistics_collector.cpp`:23
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/metadata/test_statistics_collector.cpp`:32
- Brief: n/a
- Parameters: none

#### `void insertRow(const std::string &table_name, const std::string &row_id, BaseEntity::FieldMap fields)`
- Source: `tests/metadata/test_statistics_collector.cpp`:39
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a
  - `row_id` (const std::string &): n/a
  - `fields` (BaseEntity::FieldMap): n/a

### StubMetadataTable

#### `StubMetadataTable()`
- Source: `tests/metadata/test_metadata_highcardinality_stress.cpp`:38
- Brief: n/a
- Parameters: none

#### `std::string get(const std::string &key) const`
- Source: `tests/metadata/test_metadata_highcardinality_stress.cpp`:52
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `void insert(const std::string &key, const std::string &value)`
- Source: `tests/metadata/test_metadata_highcardinality_stress.cpp`:40
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

#### `uint64_t insertCount() const`
- Source: `tests/metadata/test_metadata_highcardinality_stress.cpp`:63
- Brief: n/a
- Parameters: none

#### `bool isIndexed(const std::string &key) const`
- Source: `tests/metadata/test_metadata_highcardinality_stress.cpp`:47
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::size_t size() const`
- Source: `tests/metadata/test_metadata_highcardinality_stress.cpp`:58
- Brief: n/a
- Parameters: none

### bench_metadata_cache.cpp

#### `Arg(0) -> Arg(100) ->Arg(1000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a

#### `Arg(1) -> Arg(10) ->Arg(50) ->Arg(100) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Arg(10) -> Arg(100) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `BENCHMARK(BM_MetadataCache_GetTable_Miss) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MetadataCache_GetTable_Miss): n/a

#### `BENCHMARK(BM_MetadataCache_HitRate_Hit) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MetadataCache_HitRate_Hit): n/a

#### `BENCHMARK(BM_MetadataCache_HitRate_Miss) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:284
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MetadataCache_HitRate_Miss): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:563
- Brief: n/a
- Parameters: none

#### `void BM_MetadataCache_AdaptiveTTL(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:445
- Brief: getAllTables() throughput with adaptive TTL enabled.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: state.range(0) = simulated mutations recorded before the benchmark loop. A high mutation count drives the effective TTL down (more frequent rebuilds); a count of 0 keeps the adaptive TTL near its configured maximum.

#### `void BM_MetadataCache_ColdScan(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:182
- Brief: Measures the time taken by the very first getAllTables() call on an empty cache (cold start = full RocksDB key scan + schema build).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Parameterised by table count: 1, 10, 50, 100. Target: < 20 ms for 10 tables; < 200 ms for 100 tables.

#### `void BM_MetadataCache_ConcurrentReads(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:492
- Brief: Multi-threaded getAllTables() throughput.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Multiple threads call getAllTables() simultaneously. The SchemaManager uses a shared_mutex for its cache, so readers run fully in parallel once the cache is warm. state.range(0) is not used; thread count is controlled via Threads(). Target: > 200 K ops/sec at 8 threads.

#### `void BM_MetadataCache_GetDatabaseMetadata(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:347
- Brief: getDatabaseMetadata() on a fully warmed cache.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: This path aggregates row counts over all cached table schemas; it is therefore O(n_tables) but entirely in-memory.

#### `void BM_MetadataCache_GetTable_Hit(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:296
- Brief: Warm getTable() hit: schema is cached, lookup is O(log n) in the std::map.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Target: < 5 µs.

#### `void BM_MetadataCache_GetTable_Miss(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:321
- Brief: getTable() miss: key does not exist, returns std::nullopt.
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MetadataCache_HitRate_Hit(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:250
- Brief: Warm hit throughput (ops/sec) – represents > 90 % hit-rate scenario.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: The SchemaManager is warmed once; the benchmark loop calls getAllTables() on the same instance repeatedly (cache always valid).

#### `void BM_MetadataCache_HitRate_Miss(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:272
- Brief: Cache miss throughput — every iteration forces a full rescan.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Simulates a 0 % cache hit rate by calling refreshCache() before every getAllTables() invocation. Comparing this benchmark to BM_MetadataCache_HitRate_Hit quantifies the throughput benefit of caching (expected: several orders of magnitude).

#### `void BM_MetadataCache_RefreshCache(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:377
- Brief: Measures the cost of a forced cache rebuild (refreshCache()).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: This represents the worst-case write-lock path triggered by TTL expiry or explicit invalidation. Parameterised by table count.

#### `void BM_MetadataCache_RocksDBScan_Direct(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:535
- Brief: Direct RocksDB key-prefix iterator scan (no SchemaManager caching).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: This benchmark simulates what SchemaManager::buildCache() does internally — iterating over all keys to discover table prefixes — but without any caching layer. Comparing to BM_MetadataCache_WarmHit provides the empirical speedup ratio that the metadata cache delivers over raw RocksDB. For the comparison to be meaningful, the number of rows matches the WarmHit benchmarks (10 rows/table, 10 tables = 100 keys).

#### `void BM_MetadataCache_TTLVariants(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:412
- Brief: Compare getAllTables() throughput under different fixed TTL settings.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: All runs use a warm cache; the TTL only affects how quickly the cache expires between iterations. With a large TTL the cache remains valid for the entire benchmark run; with TTL=1 s the benchmark may occasionally pay the rebuild cost (< 5 % of iterations in typical runs). state.range(0) encodes the TTL in seconds.

#### `void BM_MetadataCache_WarmHit(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:218
- Brief: Measures getAllTables() when the cache is fully warm.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: A single SchemaManager is created and warmed before the benchmark loop. Every iteration hits the in-memory cache only (no RocksDB I/O). Target: < 10 µs per call for 100 tables.

#### `Threads(1) -> Threads(4) ->Threads(8) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/metadata/bench_metadata_cache.cpp`:515
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

### bench_metadata_consistency_lineage_gates.cpp

#### `BENCHMARK(BM_ColumnRef_JsonRoundTrip) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ColumnRef_JsonRoundTrip): n/a

#### `BENCHMARK(BM_ColumnRef_ToString) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ColumnRef_ToString): n/a

#### `BENCHMARK(BM_ConsistencyIssue_ToJSON) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ConsistencyIssue_ToJSON): n/a

#### `BENCHMARK(BM_TransformationType_StringConversion) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_TransformationType_StringConversion): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp`:115
- Brief: n/a
- Parameters: none

#### `void BM_ColumnRef_JsonRoundTrip(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp`:79
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ColumnRef_ToString(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp`:58
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ConsistencyIssue_ToJSON(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp`:40
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TransformationType_StringConversion(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp`:94
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_metadata_release_gates.cpp

#### `BENCHMARK(BM_MetaError_BatchCast) -> Arg(1000) ->Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/metadata/bench_metadata_release_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MetaError_BatchCast): n/a

#### `BENCHMARK(BM_MetaError_Cast) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/metadata/bench_metadata_release_gates.cpp`:23
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MetaError_Cast): n/a

#### `BENCHMARK(BM_MetaError_RangeCheck) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/metadata/bench_metadata_release_gates.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MetaError_RangeCheck): n/a

#### `BENCHMARK(BM_MetaError_SwitchDispatch) -> Repetitions(5) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/metadata/bench_metadata_release_gates.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MetaError_SwitchDispatch): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/metadata/bench_metadata_release_gates.cpp`:82
- Brief: n/a
- Parameters: none

#### `void BM_MetaError_BatchCast(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_release_gates.cpp`:63
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MetaError_Cast(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_release_gates.cpp`:10
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MetaError_RangeCheck(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_release_gates.cpp`:47
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MetaError_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/metadata/bench_metadata_release_gates.cpp`:25
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### test_catalog_exporter.cpp

#### `TEST(CatalogExporterConfigTest, AtlasTypeIsDefault)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterConfigTest): n/a
  - `<unnamed>` (AtlasTypeIsDefault): n/a

#### `TEST(CatalogExporterConfigTest, DefaultDatabaseNameIsThemisDB)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterConfigTest): n/a
  - `<unnamed>` (DefaultDatabaseNameIsThemisDB): n/a

#### `TEST(CatalogExporterConfigTest, DefaultTimeoutIs10Seconds)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:404
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterConfigTest): n/a
  - `<unnamed>` (DefaultTimeoutIs10Seconds): n/a

#### `TEST_F(CatalogExporterAtlasTest, EmptyTableListSucceedsWithoutHttp)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (EmptyTableListSucceedsWithoutHttp): n/a

#### `TEST_F(CatalogExporterAtlasTest, Http201AlsoCountsAsSuccess)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (Http201AlsoCountsAsSuccess): n/a

#### `TEST_F(CatalogExporterAtlasTest, HttpErrorReturnsFailure)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (HttpErrorReturnsFailure): n/a

#### `TEST_F(CatalogExporterAtlasTest, MutatedEntitiesCountIsReturned)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (MutatedEntitiesCountIsReturned): n/a

#### `TEST_F(CatalogExporterAtlasTest, PayloadContainsDbAndTableEntities)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (PayloadContainsDbAndTableEntities): n/a

#### `TEST_F(CatalogExporterAtlasTest, PublishMultipleTablesInSingleBulkCall)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (PublishMultipleTablesInSingleBulkCall): n/a

#### `TEST_F(CatalogExporterAtlasTest, PublishSetsBasicAuthHeader)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (PublishSetsBasicAuthHeader): n/a

#### `TEST_F(CatalogExporterAtlasTest, PublishSingleTableCallsCorrectUrl)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (PublishSingleTableCallsCorrectUrl): n/a

#### `TEST_F(CatalogExporterAtlasTest, PublishSingleTableViaPublishTable)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (PublishSingleTableViaPublishTable): n/a

#### `TEST_F(CatalogExporterAtlasTest, TableQualifiedNameContainsDatabaseName)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterAtlasTest): n/a
  - `<unnamed>` (TableQualifiedNameContainsDatabaseName): n/a

#### `TEST_F(CatalogExporterDataHubTest, BearerTokenInAuthHeader)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (BearerTokenInAuthHeader): n/a

#### `TEST_F(CatalogExporterDataHubTest, DatasetPropertiesAspectNamePresent)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (DatasetPropertiesAspectNamePresent): n/a

#### `TEST_F(CatalogExporterDataHubTest, EmptySchemaSucceedsWithoutHttp)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (EmptySchemaSucceedsWithoutHttp): n/a

#### `TEST_F(CatalogExporterDataHubTest, HttpErrorStopsAndReturnsFailure)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (HttpErrorStopsAndReturnsFailure): n/a

#### `TEST_F(CatalogExporterDataHubTest, ProposalContainsDatasetUrn)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (ProposalContainsDatasetUrn): n/a

#### `TEST_F(CatalogExporterDataHubTest, ProposalWrappedInProposalKey)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:380
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (ProposalWrappedInProposalKey): n/a

#### `TEST_F(CatalogExporterDataHubTest, PublishCallsIngestProposalEndpoint)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (PublishCallsIngestProposalEndpoint): n/a

#### `TEST_F(CatalogExporterDataHubTest, PublishMultipleTablesCallsIngestForEach)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (PublishMultipleTablesCallsIngestForEach): n/a

#### `TEST_F(CatalogExporterDataHubTest, SchemaMetadataContainsFieldPaths)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (SchemaMetadataContainsFieldPaths): n/a

#### `TEST_F(CatalogExporterDataHubTest, TwoProposalsPerTableDatasetPropertiesAndSchema)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterDataHubTest): n/a
  - `<unnamed>` (TwoProposalsPerTableDatasetPropertiesAndSchema): n/a

#### `CatalogExporter::Config atlasConfig(const std::string &endpoint="http://atlas:21000")`
- Source: `tests/metadata/test_catalog_exporter.cpp`:46
- Brief: Build a minimal CatalogExporter::Config for Apache Atlas.
- Parameters:
  - `endpoint` (const std::string &): n/a

#### `CatalogExporter::Config datahubConfig(const std::string &endpoint="http://datahub-gms:8080")`
- Source: `tests/metadata/test_catalog_exporter.cpp`:58
- Brief: Build a minimal CatalogExporter::Config for DataHub.
- Parameters:
  - `endpoint` (const std::string &): n/a

#### `SchemaManager::TableSchema makeTable(const std::string &name, const std::string &type="relational", size_t row_count=100)`
- Source: `tests/metadata/test_catalog_exporter.cpp`:22
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `type` (const std::string &): n/a
  - `row_count` (size_t): n/a

### test_catalog_exporter_failure_paths_focused.cpp

#### `TEST(CatalogExporterFailurePathsTest, MCHEX01_Http500ReturnsFailure)`
- Source: `tests/metadata/test_catalog_exporter_failure_paths_focused.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterFailurePathsTest): n/a
  - `<unnamed>` (MCHEX01_Http500ReturnsFailure): n/a

#### `TEST(CatalogExporterFailurePathsTest, MCHEX02_Http200ReturnsSuccess)`
- Source: `tests/metadata/test_catalog_exporter_failure_paths_focused.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterFailurePathsTest): n/a
  - `<unnamed>` (MCHEX02_Http200ReturnsSuccess): n/a

#### `TEST(CatalogExporterFailurePathsTest, MCHEX03_EmptyTableListReturnsZeroEntities)`
- Source: `tests/metadata/test_catalog_exporter_failure_paths_focused.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterFailurePathsTest): n/a
  - `<unnamed>` (MCHEX03_EmptyTableListReturnsZeroEntities): n/a

#### `TEST(CatalogExporterFailurePathsTest, MCHEX04_DataHubHttp200ReturnsSuccess)`
- Source: `tests/metadata/test_catalog_exporter_failure_paths_focused.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporterFailurePathsTest): n/a
  - `<unnamed>` (MCHEX04_DataHubHttp200ReturnsSuccess): n/a

### test_column_lineage.cpp

#### `TEST(ColumnLineageEntryTest, ToJSONOmitsEmptyOptionals)`
- Source: `tests/metadata/test_column_lineage.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageEntryTest): n/a
  - `<unnamed>` (ToJSONOmitsEmptyOptionals): n/a

#### `TEST(ColumnLineageEntryTest, ToJSONRequiredFields)`
- Source: `tests/metadata/test_column_lineage.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageEntryTest): n/a
  - `<unnamed>` (ToJSONRequiredFields): n/a

#### `TEST(ColumnLineageRecordTest, ToJSONEmptyRecord)`
- Source: `tests/metadata/test_column_lineage.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageRecordTest): n/a
  - `<unnamed>` (ToJSONEmptyRecord): n/a

#### `TEST(ColumnLineageRecordTest, ToJSONWithEntries)`
- Source: `tests/metadata/test_column_lineage.cpp`:430
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageRecordTest): n/a
  - `<unnamed>` (ToJSONWithEntries): n/a

#### `TEST(ColumnRefTest, EqualityOperator)`
- Source: `tests/metadata/test_column_lineage.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnRefTest): n/a
  - `<unnamed>` (EqualityOperator): n/a

#### `TEST(ColumnRefTest, FromJSONRoundTrip)`
- Source: `tests/metadata/test_column_lineage.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnRefTest): n/a
  - `<unnamed>` (FromJSONRoundTrip): n/a

#### `TEST(ColumnRefTest, ToJSONContainsTableAndColumn)`
- Source: `tests/metadata/test_column_lineage.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnRefTest): n/a
  - `<unnamed>` (ToJSONContainsTableAndColumn): n/a

#### `TEST(ColumnRefTest, ToString)`
- Source: `tests/metadata/test_column_lineage.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnRefTest): n/a
  - `<unnamed>` (ToString): n/a

#### `TEST(TransformationTypeTest, AllValuesToStringNonEmpty)`
- Source: `tests/metadata/test_column_lineage.cpp`:31
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransformationTypeTest): n/a
  - `<unnamed>` (AllValuesToStringNonEmpty): n/a

#### `TEST(TransformationTypeTest, FromStringCaseInsensitive)`
- Source: `tests/metadata/test_column_lineage.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransformationTypeTest): n/a
  - `<unnamed>` (FromStringCaseInsensitive): n/a

#### `TEST(TransformationTypeTest, KnownStringValues)`
- Source: `tests/metadata/test_column_lineage.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransformationTypeTest): n/a
  - `<unnamed>` (KnownStringValues): n/a

#### `TEST(TransformationTypeTest, RoundTripFromString)`
- Source: `tests/metadata/test_column_lineage.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransformationTypeTest): n/a
  - `<unnamed>` (RoundTripFromString): n/a

#### `TEST(TransformationTypeTest, UnknownStringMapsToCustom)`
- Source: `tests/metadata/test_column_lineage.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransformationTypeTest): n/a
  - `<unnamed>` (UnknownStringMapsToCustom): n/a

#### `TEST_F(ColumnLineageTrackerTest, DiamondDAGDownstreamDeduplicates)`
- Source: `tests/metadata/test_column_lineage.cpp`:328
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (DiamondDAGDownstreamDeduplicates): n/a

#### `TEST_F(ColumnLineageTrackerTest, DiamondDAGUpstreamDeduplicates)`
- Source: `tests/metadata/test_column_lineage.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (DiamondDAGUpstreamDeduplicates): n/a

#### `TEST_F(ColumnLineageTrackerTest, DownstreamColumns_DirectChild)`
- Source: `tests/metadata/test_column_lineage.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (DownstreamColumns_DirectChild): n/a

#### `TEST_F(ColumnLineageTrackerTest, DownstreamColumns_LeafHasNoDescendants)`
- Source: `tests/metadata/test_column_lineage.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (DownstreamColumns_LeafHasNoDescendants): n/a

#### `TEST_F(ColumnLineageTrackerTest, DownstreamColumns_ThreeGenerations)`
- Source: `tests/metadata/test_column_lineage.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (DownstreamColumns_ThreeGenerations): n/a

#### `TEST_F(ColumnLineageTrackerTest, DownstreamColumns_UnknownColumnReturnsEmpty)`
- Source: `tests/metadata/test_column_lineage.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (DownstreamColumns_UnknownColumnReturnsEmpty): n/a

#### `TEST_F(ColumnLineageTrackerTest, ExportAllLineageContainsAllEntries)`
- Source: `tests/metadata/test_column_lineage.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (ExportAllLineageContainsAllEntries): n/a

#### `TEST_F(ColumnLineageTrackerTest, ExportTableLineageFiltersCorrectly)`
- Source: `tests/metadata/test_column_lineage.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (ExportTableLineageFiltersCorrectly): n/a

#### `TEST_F(ColumnLineageTrackerTest, ExportTableLineageForUnknownTableIsEmpty)`
- Source: `tests/metadata/test_column_lineage.cpp`:386
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (ExportTableLineageForUnknownTableIsEmpty): n/a

#### `TEST_F(ColumnLineageTrackerTest, GetColumnLineageForUnknownColumnReturnsEmptyRecord)`
- Source: `tests/metadata/test_column_lineage.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (GetColumnLineageForUnknownColumnReturnsEmptyRecord): n/a

#### `TEST_F(ColumnLineageTrackerTest, ProvenanceContainsAllFields)`
- Source: `tests/metadata/test_column_lineage.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (ProvenanceContainsAllFields): n/a

#### `TEST_F(ColumnLineageTrackerTest, ProvenanceForUnknownColumnIsEmpty)`
- Source: `tests/metadata/test_column_lineage.cpp`:364
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (ProvenanceForUnknownColumnIsEmpty): n/a

#### `TEST_F(ColumnLineageTrackerTest, RecordAssignsEntryIdWhenEmpty)`
- Source: `tests/metadata/test_column_lineage.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (RecordAssignsEntryIdWhenEmpty): n/a

#### `TEST_F(ColumnLineageTrackerTest, RecordAssignsTimestampWhenZero)`
- Source: `tests/metadata/test_column_lineage.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (RecordAssignsTimestampWhenZero): n/a

#### `TEST_F(ColumnLineageTrackerTest, RecordPreservesProvidedFields)`
- Source: `tests/metadata/test_column_lineage.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (RecordPreservesProvidedFields): n/a

#### `TEST_F(ColumnLineageTrackerTest, TotalEntryCountMatchesRecorded)`
- Source: `tests/metadata/test_column_lineage.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (TotalEntryCountMatchesRecorded): n/a

#### `TEST_F(ColumnLineageTrackerTest, UpstreamColumns_DirectParent)`
- Source: `tests/metadata/test_column_lineage.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (UpstreamColumns_DirectParent): n/a

#### `TEST_F(ColumnLineageTrackerTest, UpstreamColumns_MultipleSourcesMerged)`
- Source: `tests/metadata/test_column_lineage.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (UpstreamColumns_MultipleSourcesMerged): n/a

#### `TEST_F(ColumnLineageTrackerTest, UpstreamColumns_RootHasNoUpstream)`
- Source: `tests/metadata/test_column_lineage.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (UpstreamColumns_RootHasNoUpstream): n/a

#### `TEST_F(ColumnLineageTrackerTest, UpstreamColumns_ThreeGenerations)`
- Source: `tests/metadata/test_column_lineage.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (UpstreamColumns_ThreeGenerations): n/a

#### `TEST_F(ColumnLineageTrackerTest, UpstreamColumns_UnknownColumnReturnsEmpty)`
- Source: `tests/metadata/test_column_lineage.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTrackerTest): n/a
  - `<unnamed>` (UpstreamColumns_UnknownColumnReturnsEmpty): n/a

### test_column_lineage_traversal_edge_focused.cpp

#### `TEST(ColumnLineageTraversalEdgeTest, MCHLLN01_ColumnRefEquality)`
- Source: `tests/metadata/test_column_lineage_traversal_edge_focused.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTraversalEdgeTest): n/a
  - `<unnamed>` (MCHLLN01_ColumnRefEquality): n/a

#### `TEST(ColumnLineageTraversalEdgeTest, MCHLLN02_ColumnRefToString)`
- Source: `tests/metadata/test_column_lineage_traversal_edge_focused.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTraversalEdgeTest): n/a
  - `<unnamed>` (MCHLLN02_ColumnRefToString): n/a

#### `TEST(ColumnLineageTraversalEdgeTest, MCHLLN03_ColumnRefJsonRoundTrip)`
- Source: `tests/metadata/test_column_lineage_traversal_edge_focused.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTraversalEdgeTest): n/a
  - `<unnamed>` (MCHLLN03_ColumnRefJsonRoundTrip): n/a

#### `TEST(ColumnLineageTraversalEdgeTest, MCHLLN04_TransformationTypeRoundTrip)`
- Source: `tests/metadata/test_column_lineage_traversal_edge_focused.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTraversalEdgeTest): n/a
  - `<unnamed>` (MCHLLN04_TransformationTypeRoundTrip): n/a

### test_distributed_catalog.cpp

#### `TEST_F(DistributedCatalogSyncTest, SyncPublishesAllSchemas)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedCatalogSyncTest): n/a
  - `<unnamed>` (SyncPublishesAllSchemas): n/a

#### `TEST_F(DistributedCatalogSyncTest, SyncedSchemaRoundTrips)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedCatalogSyncTest): n/a
  - `<unnamed>` (SyncedSchemaRoundTrips): n/a

#### `TEST_F(DistributedMetadataCatalogTest, FetchNonExistent)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (FetchNonExistent): n/a

#### `TEST_F(DistributedMetadataCatalogTest, ListTableNamesAfterPublish)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (ListTableNamesAfterPublish): n/a

#### `TEST_F(DistributedMetadataCatalogTest, ListTableNamesAfterRemove)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (ListTableNamesAfterRemove): n/a

#### `TEST_F(DistributedMetadataCatalogTest, ListTableNamesEmpty)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (ListTableNamesEmpty): n/a

#### `TEST_F(DistributedMetadataCatalogTest, PublishAndFetch)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (PublishAndFetch): n/a

#### `TEST_F(DistributedMetadataCatalogTest, PublishEmptyNameFails)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (PublishEmptyNameFails): n/a

#### `TEST_F(DistributedMetadataCatalogTest, PublishOverwriteSchema)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (PublishOverwriteSchema): n/a

#### `TEST_F(DistributedMetadataCatalogTest, RemoveNonExistentReturnsFalse)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (RemoveNonExistentReturnsFalse): n/a

#### `TEST_F(DistributedMetadataCatalogTest, RemoveSchema)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (RemoveSchema): n/a

#### `TEST_F(DistributedMetadataCatalogTest, Statistics)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (Statistics): n/a

#### `TEST_F(DistributedMetadataCatalogTest, ThreadSafety)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalogTest): n/a
  - `<unnamed>` (ThreadSafety): n/a

#### `std::pair< std::unique_ptr< MetadataShardRouter >, std::vector< std::shared_ptr< MetadataShard > > > makeRouter(size_t num_shards=3)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:31
- Brief: n/a
- Parameters:
  - `num_shards` (size_t): n/a

#### `SchemaManager::TableSchema makeSchema(const std::string &name, const std::string &type="relational")`
- Source: `tests/metadata/test_distributed_catalog.cpp`:56
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `type` (const std::string &): n/a

#### `std::string makeTempDbPath(const std::string &name)`
- Source: `tests/metadata/test_distributed_catalog.cpp`:21
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

### test_distributed_catalog_diagnostics_focused.cpp

#### `TEST(DistributedCatalogDiagnosticsTest, MCHDC01_StartsWithZeroEvents)`
- Source: `tests/metadata/test_distributed_catalog_diagnostics_focused.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedCatalogDiagnosticsTest): n/a
  - `<unnamed>` (MCHDC01_StartsWithZeroEvents): n/a

#### `TEST(DistributedCatalogDiagnosticsTest, MCHDC02_OnChangedIncrementsCount)`
- Source: `tests/metadata/test_distributed_catalog_diagnostics_focused.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedCatalogDiagnosticsTest): n/a
  - `<unnamed>` (MCHDC02_OnChangedIncrementsCount): n/a

#### `TEST(DistributedCatalogDiagnosticsTest, MCHDC03_LastEventReturnsNewest)`
- Source: `tests/metadata/test_distributed_catalog_diagnostics_focused.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedCatalogDiagnosticsTest): n/a
  - `<unnamed>` (MCHDC03_LastEventReturnsNewest): n/a

#### `TEST(DistributedCatalogDiagnosticsTest, MCHDC04_ClearResetsToZero)`
- Source: `tests/metadata/test_distributed_catalog_diagnostics_focused.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedCatalogDiagnosticsTest): n/a
  - `<unnamed>` (MCHDC04_ClearResetsToZero): n/a

### test_index_recommender.cpp

#### `TEST(IndexRecommenderStructTest, ColumnAccessToJSON)`
- Source: `tests/metadata/test_index_recommender.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderStructTest): n/a
  - `<unnamed>` (ColumnAccessToJSON): n/a

#### `TEST(IndexRecommenderStructTest, IndexRecommendationToJSONAdd)`
- Source: `tests/metadata/test_index_recommender.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderStructTest): n/a
  - `<unnamed>` (IndexRecommendationToJSONAdd): n/a

#### `TEST(IndexRecommenderStructTest, IndexRecommendationToJSONDrop)`
- Source: `tests/metadata/test_index_recommender.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderStructTest): n/a
  - `<unnamed>` (IndexRecommendationToJSONDrop): n/a

#### `TEST_F(IndexRecommenderCostModelTest, CostModel_HighSelectivity_HigherScore)`
- Source: `tests/metadata/test_index_recommender.cpp`:529
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderCostModelTest): n/a
  - `<unnamed>` (CostModel_HighSelectivity_HigherScore): n/a

#### `TEST_F(IndexRecommenderCostModelTest, CostModel_LargeTable_WriteAmplificationPenalty)`
- Source: `tests/metadata/test_index_recommender.cpp`:561
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderCostModelTest): n/a
  - `<unnamed>` (CostModel_LargeTable_WriteAmplificationPenalty): n/a

#### `TEST_F(IndexRecommenderCostModelTest, NoStatisticsCollector_HeuristicUsed)`
- Source: `tests/metadata/test_index_recommender.cpp`:499
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderCostModelTest): n/a
  - `<unnamed>` (NoStatisticsCollector_HeuristicUsed): n/a

#### `TEST_F(IndexRecommenderCostModelTest, StatisticsCollector_NoTableData_FallsBackGracefully)`
- Source: `tests/metadata/test_index_recommender.cpp`:512
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderCostModelTest): n/a
  - `<unnamed>` (StatisticsCollector_NoTableData_FallsBackGracefully): n/a

#### `TEST_F(IndexRecommenderMetricTest, NoMetricsCollector_NoCrash)`
- Source: `tests/metadata/test_index_recommender.cpp`:642
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderMetricTest): n/a
  - `<unnamed>` (NoMetricsCollector_NoCrash): n/a

#### `TEST_F(IndexRecommenderMetricTest, RecommendCounterIncrementsOnce)`
- Source: `tests/metadata/test_index_recommender.cpp`:619
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderMetricTest): n/a
  - `<unnamed>` (RecommendCounterIncrementsOnce): n/a

#### `TEST_F(IndexRecommenderMetricTest, RecommendIncrementsCounter)`
- Source: `tests/metadata/test_index_recommender.cpp`:600
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderMetricTest): n/a
  - `<unnamed>` (RecommendIncrementsCounter): n/a

#### `TEST_F(IndexRecommenderMetricTest, SetMetricsCollectorNullptr_DisablesEmission)`
- Source: `tests/metadata/test_index_recommender.cpp`:652
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderMetricTest): n/a
  - `<unnamed>` (SetMetricsCollectorNullptr_DisablesEmission): n/a

#### `TEST_F(IndexRecommenderPersistTest, BackgroundThreadPersistsWithinInterval)`
- Source: `tests/metadata/test_index_recommender.cpp`:429
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderPersistTest): n/a
  - `<unnamed>` (BackgroundThreadPersistsWithinInterval): n/a

#### `TEST_F(IndexRecommenderPersistTest, ConstructorLoadsPersistedStats)`
- Source: `tests/metadata/test_index_recommender.cpp`:345
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderPersistTest): n/a
  - `<unnamed>` (ConstructorLoadsPersistedStats): n/a

#### `TEST_F(IndexRecommenderPersistTest, DestructorFlushesToDB)`
- Source: `tests/metadata/test_index_recommender.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderPersistTest): n/a
  - `<unnamed>` (DestructorFlushesToDB): n/a

#### `TEST_F(IndexRecommenderPersistTest, MergesPersistedAndInMemoryAccesses)`
- Source: `tests/metadata/test_index_recommender.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderPersistTest): n/a
  - `<unnamed>` (MergesPersistedAndInMemoryAccesses): n/a

#### `TEST_F(IndexRecommenderPersistTest, PersistStatsTotalQueriesKey)`
- Source: `tests/metadata/test_index_recommender.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderPersistTest): n/a
  - `<unnamed>` (PersistStatsTotalQueriesKey): n/a

#### `TEST_F(IndexRecommenderPersistTest, PersistStatsWritesRocksDBKeys)`
- Source: `tests/metadata/test_index_recommender.cpp`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderPersistTest): n/a
  - `<unnamed>` (PersistStatsWritesRocksDBKeys): n/a

#### `TEST_F(IndexRecommenderPersistTest, ResetDeletesRocksDBKeys)`
- Source: `tests/metadata/test_index_recommender.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderPersistTest): n/a
  - `<unnamed>` (ResetDeletesRocksDBKeys): n/a

#### `TEST_F(IndexRecommenderTest, GetAccessStatsUnknownTable)`
- Source: `tests/metadata/test_index_recommender.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (GetAccessStatsUnknownTable): n/a

#### `TEST_F(IndexRecommenderTest, MultipleAccessesAccumulateCounts)`
- Source: `tests/metadata/test_index_recommender.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (MultipleAccessesAccumulateCounts): n/a

#### `TEST_F(IndexRecommenderTest, NoRecommendForAlreadyIndexedHighBenefitColumn)`
- Source: `tests/metadata/test_index_recommender.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (NoRecommendForAlreadyIndexedHighBenefitColumn): n/a

#### `TEST_F(IndexRecommenderTest, RecommendAddIndex)`
- Source: `tests/metadata/test_index_recommender.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (RecommendAddIndex): n/a

#### `TEST_F(IndexRecommenderTest, RecommendAllMultipleTables)`
- Source: `tests/metadata/test_index_recommender.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (RecommendAllMultipleTables): n/a

#### `TEST_F(IndexRecommenderTest, RecommendDropUnusedIndex)`
- Source: `tests/metadata/test_index_recommender.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (RecommendDropUnusedIndex): n/a

#### `TEST_F(IndexRecommenderTest, RecommendEmptyStats)`
- Source: `tests/metadata/test_index_recommender.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (RecommendEmptyStats): n/a

#### `TEST_F(IndexRecommenderTest, RecommendSortPreferRangeIndex)`
- Source: `tests/metadata/test_index_recommender.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (RecommendSortPreferRangeIndex): n/a

#### `TEST_F(IndexRecommenderTest, RecommendSortedByBenefitDesc)`
- Source: `tests/metadata/test_index_recommender.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (RecommendSortedByBenefitDesc): n/a

#### `TEST_F(IndexRecommenderTest, RecordAccessMultipleColumns)`
- Source: `tests/metadata/test_index_recommender.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (RecordAccessMultipleColumns): n/a

#### `TEST_F(IndexRecommenderTest, RecordAccessUpdatesCounts)`
- Source: `tests/metadata/test_index_recommender.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (RecordAccessUpdatesCounts): n/a

#### `TEST_F(IndexRecommenderTest, RecordSortAccess)`
- Source: `tests/metadata/test_index_recommender.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (RecordSortAccess): n/a

#### `TEST_F(IndexRecommenderTest, ResetClearsStats)`
- Source: `tests/metadata/test_index_recommender.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (ResetClearsStats): n/a

#### `TEST_F(IndexRecommenderTest, ToJSONStructure)`
- Source: `tests/metadata/test_index_recommender.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexRecommenderTest): n/a
  - `<unnamed>` (ToJSONStructure): n/a

#### `std::shared_ptr< RocksDBWrapper > openTempDB(const std::string &path)`
- Source: `tests/metadata/test_index_recommender.cpp`:31
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `std::string uniqueTmpPath(const std::string &tag)`
- Source: `tests/metadata/test_index_recommender.cpp`:22
- Brief: n/a
- Parameters:
  - `tag` (const std::string &): n/a

### test_metadata_change_listener.cpp

#### `TEST(MetadataChangeEventTest, ToJSONIncludesRequiredFields)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataChangeEventTest): n/a
  - `<unnamed>` (ToJSONIncludesRequiredFields): n/a

#### `TEST(MetadataChangeEventTest, ToJSONOmitsAbsentOptionalFields)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataChangeEventTest): n/a
  - `<unnamed>` (ToJSONOmitsAbsentOptionalFields): n/a

#### `TEST(MetadataChangeListenerPolymorphismTest, DispatchViaInterface)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataChangeListenerPolymorphismTest): n/a
  - `<unnamed>` (DispatchViaInterface): n/a

#### `TEST(RecordingMetadataChangeListenerTest, CallbackInvokedAfterEachEvent)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (CallbackInvokedAfterEachEvent): n/a

#### `TEST(RecordingMetadataChangeListenerTest, ClearResetsEventCount)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (ClearResetsEventCount): n/a

#### `TEST(RecordingMetadataChangeListenerTest, ConcurrentDispatchIsThreadSafe)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (ConcurrentDispatchIsThreadSafe): n/a

#### `TEST(RecordingMetadataChangeListenerTest, EventsRecordedInFifoOrder)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (EventsRecordedInFifoOrder): n/a

#### `TEST(RecordingMetadataChangeListenerTest, InitiallyEmpty)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (InitiallyEmpty): n/a

#### `TEST(RecordingMetadataChangeListenerTest, LastEventNulloptWhenEmpty)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (LastEventNulloptWhenEmpty): n/a

#### `TEST(RecordingMetadataChangeListenerTest, LastEventReturnsNewestEvent)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (LastEventReturnsNewestEvent): n/a

#### `TEST(RecordingMetadataChangeListenerTest, NoCallbackNoError)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (NoCallbackNoError): n/a

#### `TEST(RecordingMetadataChangeListenerTest, SetCallbackReplacesPrevious)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (SetCallbackReplacesPrevious): n/a

#### `TEST(RecordingMetadataChangeListenerTest, SetCallbackToNullDisablesCallback)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (SetCallbackToNullDisablesCallback): n/a

#### `TEST(RecordingMetadataChangeListenerTest, TableNamesPreserved)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecordingMetadataChangeListenerTest): n/a
  - `<unnamed>` (TableNamesPreserved): n/a

#### `MetadataChangeEvent makeEvent(MetadataChangeType type, std::string_view table)`
- Source: `tests/metadata/test_metadata_change_listener.cpp`:32
- Brief: n/a
- Parameters:
  - `type` (MetadataChangeType): n/a
  - `table` (std::string_view): n/a

### test_metadata_contract_hardening_focused.cpp

#### `TEST(MetadataContractTest, MET01_ErrorCodesUnique)`
- Source: `tests/metadata/test_metadata_contract_hardening_focused.cpp`:16
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataContractTest): n/a
  - `<unnamed>` (MET01_ErrorCodesUnique): n/a

#### `TEST(MetadataContractTest, MET02_ErrorCodesInRange)`
- Source: `tests/metadata/test_metadata_contract_hardening_focused.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataContractTest): n/a
  - `<unnamed>` (MET02_ErrorCodesInRange): n/a

#### `TEST(MetadataContractTest, MET03_CollectionNotFoundLowest)`
- Source: `tests/metadata/test_metadata_contract_hardening_focused.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataContractTest): n/a
  - `<unnamed>` (MET03_CollectionNotFoundLowest): n/a

#### `TEST(MetadataContractTest, MET04_ExportFailedHighest)`
- Source: `tests/metadata/test_metadata_contract_hardening_focused.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataContractTest): n/a
  - `<unnamed>` (MET04_ExportFailedHighest): n/a

#### `TEST(MetadataContractTest, MET05_CollectionDistinctFromField)`
- Source: `tests/metadata/test_metadata_contract_hardening_focused.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataContractTest): n/a
  - `<unnamed>` (MET05_CollectionDistinctFromField): n/a

#### `TEST(MetadataContractTest, MET06_SchemaMismatchDistinctFromExport)`
- Source: `tests/metadata/test_metadata_contract_hardening_focused.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataContractTest): n/a
  - `<unnamed>` (MET06_SchemaMismatchDistinctFromExport): n/a

#### `TEST(MetadataContractTest, MET07_ErrorSwitchDispatch)`
- Source: `tests/metadata/test_metadata_contract_hardening_focused.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataContractTest): n/a
  - `<unnamed>` (MET07_ErrorSwitchDispatch): n/a

#### `TEST(MetadataContractTest, MET08_AllCodesGe7900)`
- Source: `tests/metadata/test_metadata_contract_hardening_focused.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataContractTest): n/a
  - `<unnamed>` (MET08_AllCodesGe7900): n/a

### test_metadata_encryption_provider.cpp

#### `TEST(MetadataEncryptionProviderFocusedTests, FieldSetAlgorithmReturnsXorBasic)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (FieldSetAlgorithmReturnsXorBasic): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, FieldSetDynamicAddRemoveUpdatesPolicy)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (FieldSetDynamicAddRemoveUpdatesPolicy): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, FieldSetEmptyKeyThrowsOnConstruct)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (FieldSetEmptyKeyThrowsOnConstruct): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, FieldSetListedFieldIsEncrypted)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (FieldSetListedFieldIsEncrypted): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, FieldSetRoundTrip)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (FieldSetRoundTrip): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, FieldSetRoundTripEmptyValue)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (FieldSetRoundTripEmptyValue): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, FieldSetThreadSafeConcurrentAccess)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (FieldSetThreadSafeConcurrentAccess): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, FieldSetUnlistedFieldPassThrough)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (FieldSetUnlistedFieldPassThrough): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, FieldSetWildcardEncryptsEveryField)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (FieldSetWildcardEncryptsEveryField): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, NoOpAlgorithmReturnsNone)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (NoOpAlgorithmReturnsNone): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, NoOpDecryptReturnsCiphertextUnchanged)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (NoOpDecryptReturnsCiphertextUnchanged): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, NoOpEncryptReturnsPlaintextUnchanged)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (NoOpEncryptReturnsPlaintextUnchanged): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, NoOpShouldEncryptAlwaysFalse)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (NoOpShouldEncryptAlwaysFalse): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, PolymorphicFieldSetViaInterface)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (PolymorphicFieldSetViaInterface): n/a

#### `TEST(MetadataEncryptionProviderFocusedTests, PolymorphicNoOpViaInterface)`
- Source: `tests/metadata/test_metadata_encryption_provider.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataEncryptionProviderFocusedTests): n/a
  - `<unnamed>` (PolymorphicNoOpViaInterface): n/a

### test_metadata_export_policy.cpp

#### `TEST(AlwaysExportPolicyTest, AllTriggersAccepted)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (AlwaysExportPolicyTest): n/a
  - `<unnamed>` (AllTriggersAccepted): n/a

#### `TEST(AlwaysExportPolicyTest, ExportDelayIsZero)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (AlwaysExportPolicyTest): n/a
  - `<unnamed>` (ExportDelayIsZero): n/a

#### `TEST(AlwaysExportPolicyTest, ShouldExportAlwaysTrue)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (AlwaysExportPolicyTest): n/a
  - `<unnamed>` (ShouldExportAlwaysTrue): n/a

#### `TEST(FilteredExportPolicyTest, ConcurrentAddAndCheck)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (ConcurrentAddAndCheck): n/a

#### `TEST(FilteredExportPolicyTest, ConfiguredDelayIsReturned)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (ConfiguredDelayIsReturned): n/a

#### `TEST(FilteredExportPolicyTest, DefaultDelayIsZero)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (DefaultDelayIsZero): n/a

#### `TEST(FilteredExportPolicyTest, DelayAppliesUniformlyToAllTables)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (DelayAppliesUniformlyToAllTables): n/a

#### `TEST(FilteredExportPolicyTest, ExcludedTableIsBlocked)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (ExcludedTableIsBlocked): n/a

#### `TEST(FilteredExportPolicyTest, ExclusionIsExactMatchOnly)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (ExclusionIsExactMatchOnly): n/a

#### `TEST(FilteredExportPolicyTest, MultipleExclusions)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (MultipleExclusions): n/a

#### `TEST(FilteredExportPolicyTest, NonExcludedTableIsExported)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (NonExcludedTableIsExported): n/a

#### `TEST(FilteredExportPolicyTest, RemoveExclusionRestoresExport)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (RemoveExclusionRestoresExport): n/a

#### `TEST(FilteredExportPolicyTest, RemoveNonExistentExclusionIsNoOp)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (FilteredExportPolicyTest): n/a
  - `<unnamed>` (RemoveNonExistentExclusionIsNoOp): n/a

#### `TEST(MetadataExportPolicyPolymorphismTest, AlwaysViaInterface)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataExportPolicyPolymorphismTest): n/a
  - `<unnamed>` (AlwaysViaInterface): n/a

#### `TEST(MetadataExportPolicyPolymorphismTest, FilteredViaInterface)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataExportPolicyPolymorphismTest): n/a
  - `<unnamed>` (FilteredViaInterface): n/a

#### `TEST(MetadataExportPolicyPolymorphismTest, NeverViaInterface)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataExportPolicyPolymorphismTest): n/a
  - `<unnamed>` (NeverViaInterface): n/a

#### `TEST(NeverExportPolicyTest, AllTriggersBlocked)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (NeverExportPolicyTest): n/a
  - `<unnamed>` (AllTriggersBlocked): n/a

#### `TEST(NeverExportPolicyTest, ShouldExportAlwaysFalse)`
- Source: `tests/metadata/test_metadata_export_policy.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (NeverExportPolicyTest): n/a
  - `<unnamed>` (ShouldExportAlwaysFalse): n/a

### test_metadata_hardening_phase23_focused.cpp

#### `TEST(MetadataHardeningPhase23Test, MCH01_ConsistencyIssueJsonHasAllFields)`
- Source: `tests/metadata/test_metadata_hardening_phase23_focused.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataHardeningPhase23Test): n/a
  - `<unnamed>` (MCH01_ConsistencyIssueJsonHasAllFields): n/a

#### `TEST(MetadataHardeningPhase23Test, MCH02_ConsistencyIssueEmptyColumnSerializes)`
- Source: `tests/metadata/test_metadata_hardening_phase23_focused.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataHardeningPhase23Test): n/a
  - `<unnamed>` (MCH02_ConsistencyIssueEmptyColumnSerializes): n/a

#### `TEST(MetadataHardeningPhase23Test, MCH03_IssueTypeOrphanKeyPreserved)`
- Source: `tests/metadata/test_metadata_hardening_phase23_focused.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataHardeningPhase23Test): n/a
  - `<unnamed>` (MCH03_IssueTypeOrphanKeyPreserved): n/a

#### `TEST(MetadataHardeningPhase23Test, MCH04_IssueTypeStaleStatsPreserved)`
- Source: `tests/metadata/test_metadata_hardening_phase23_focused.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataHardeningPhase23Test): n/a
  - `<unnamed>` (MCH04_IssueTypeStaleStatsPreserved): n/a

#### `TEST(MetadataHardeningPhase23Test, MCH05_IssueTypeMissingConstraintPreserved)`
- Source: `tests/metadata/test_metadata_hardening_phase23_focused.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataHardeningPhase23Test): n/a
  - `<unnamed>` (MCH05_IssueTypeMissingConstraintPreserved): n/a

#### `TEST(MetadataHardeningPhase23Test, MCH06_IssueTypeSchemaMismatchPreserved)`
- Source: `tests/metadata/test_metadata_hardening_phase23_focused.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataHardeningPhase23Test): n/a
  - `<unnamed>` (MCH06_IssueTypeSchemaMismatchPreserved): n/a

#### `TEST(MetadataHardeningPhase23Test, MCH07_ColumnRefToStringFormat)`
- Source: `tests/metadata/test_metadata_hardening_phase23_focused.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataHardeningPhase23Test): n/a
  - `<unnamed>` (MCH07_ColumnRefToStringFormat): n/a

#### `TEST(MetadataHardeningPhase23Test, MCH08_ColumnRefJsonRoundTrip)`
- Source: `tests/metadata/test_metadata_hardening_phase23_focused.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataHardeningPhase23Test): n/a
  - `<unnamed>` (MCH08_ColumnRefJsonRoundTrip): n/a

### test_metadata_highcardinality_stress.cpp

#### `TEST(MetadataStress, ConcurrentIndexStress)`
- Source: `tests/metadata/test_metadata_highcardinality_stress.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataStress): n/a
  - `<unnamed>` (ConcurrentIndexStress): n/a

#### `TEST(MetadataStress, DeepConcurrentAccessStress)`
- Source: `tests/metadata/test_metadata_highcardinality_stress.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataStress): n/a
  - `<unnamed>` (DeepConcurrentAccessStress): n/a

#### `TEST(MetadataStress, HighCardinalityMetadataInsert)`
- Source: `tests/metadata/test_metadata_highcardinality_stress.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataStress): n/a
  - `<unnamed>` (HighCardinalityMetadataInsert): n/a

### test_metadata_schema_churn_stress_focused.cpp

#### `TEST(MetadataSchemaChurnStressTest, MCHS01_MinimalSchemaNameIsStableMutationKey)`
- Source: `tests/metadata/test_metadata_schema_churn_stress_focused.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaChurnStressTest): n/a
  - `<unnamed>` (MCHS01_MinimalSchemaNameIsStableMutationKey): n/a

#### `TEST(MetadataSchemaChurnStressTest, MCHS02_AdaptiveTTLConfigDefaults)`
- Source: `tests/metadata/test_metadata_schema_churn_stress_focused.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaChurnStressTest): n/a
  - `<unnamed>` (MCHS02_AdaptiveTTLConfigDefaults): n/a

#### `TEST(MetadataSchemaChurnStressTest, MCHS03_AdaptiveTTLConfigInvariantAfterMutation)`
- Source: `tests/metadata/test_metadata_schema_churn_stress_focused.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaChurnStressTest): n/a
  - `<unnamed>` (MCHS03_AdaptiveTTLConfigInvariantAfterMutation): n/a

#### `TEST(MetadataSchemaChurnStressTest, MCHS04_TableSchemaJsonRoundTrip)`
- Source: `tests/metadata/test_metadata_schema_churn_stress_focused.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaChurnStressTest): n/a
  - `<unnamed>` (MCHS04_TableSchemaJsonRoundTrip): n/a

#### `TEST(MetadataSchemaChurnStressTest, MCHS05_TableSchemaJsonContainsType)`
- Source: `tests/metadata/test_metadata_schema_churn_stress_focused.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaChurnStressTest): n/a
  - `<unnamed>` (MCHS05_TableSchemaJsonContainsType): n/a

#### `TEST(MetadataSchemaChurnStressTest, MCHS06_PropertyInfoJsonRoundTrip)`
- Source: `tests/metadata/test_metadata_schema_churn_stress_focused.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaChurnStressTest): n/a
  - `<unnamed>` (MCHS06_PropertyInfoJsonRoundTrip): n/a

#### `TEST(MetadataSchemaChurnStressTest, MCHS07_IndexInfoJsonRoundTrip)`
- Source: `tests/metadata/test_metadata_schema_churn_stress_focused.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaChurnStressTest): n/a
  - `<unnamed>` (MCHS07_IndexInfoJsonRoundTrip): n/a

#### `TEST(MetadataSchemaChurnStressTest, MCHS08_MultiplePropertiesPreserveOrder)`
- Source: `tests/metadata/test_metadata_schema_churn_stress_focused.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaChurnStressTest): n/a
  - `<unnamed>` (MCHS08_MultiplePropertiesPreserveOrder): n/a

### test_metadata_schema_llm_focused.cpp

#### `TEST(MetadataSchemaLlmFocused, MS1_LlmEnabledMacro_CanBeChecked)`
- Source: `tests/metadata/test_metadata_schema_llm_focused.cpp`:19
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaLlmFocused): n/a
  - `<unnamed>` (MS1_LlmEnabledMacro_CanBeChecked): n/a

#### `TEST(MetadataSchemaLlmFocused, MS2_LlmModelNotFoundCode_InRange)`
- Source: `tests/metadata/test_metadata_schema_llm_focused.cpp`:31
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaLlmFocused): n/a
  - `<unnamed>` (MS2_LlmModelNotFoundCode_InRange): n/a

#### `TEST(MetadataSchemaLlmFocused, MS3_LlmErrorCodes_RegisteredInRegistry)`
- Source: `tests/metadata/test_metadata_schema_llm_focused.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaLlmFocused): n/a
  - `<unnamed>` (MS3_LlmErrorCodes_RegisteredInRegistry): n/a

#### `TEST(MetadataSchemaLlmFocused, MS4_LlmInvalidHandle_SolutionPresent)`
- Source: `tests/metadata/test_metadata_schema_llm_focused.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaLlmFocused): n/a
  - `<unnamed>` (MS4_LlmInvalidHandle_SolutionPresent): n/a

#### `TEST(MetadataSchemaLlmFocused, MS5_LlmKeywords_FindLlmErrors)`
- Source: `tests/metadata/test_metadata_schema_llm_focused.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSchemaLlmFocused): n/a
  - `<unnamed>` (MS5_LlmKeywords_FindLlmErrors): n/a

### test_metadata_security_provider.cpp

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, AccessDeniedExceptionCarriesPrincipalAndResource)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (AccessDeniedExceptionCarriesPrincipalAndResource): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, AdminImpliesAllOperations)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (AdminImpliesAllOperations): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, AdminOnSpecificResourceImpliesAllOpsOnThatResource)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (AdminOnSpecificResourceImpliesAllOpsOnThatResource): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, AssertPermissionDoesNotThrowWhenGranted)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (AssertPermissionDoesNotThrowWhenGranted): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, AssertPermissionThrowsWhenDenied)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (AssertPermissionThrowsWhenDenied): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, ConcurrentGrantAndCheck)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (ConcurrentGrantAndCheck): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, GrantExactTriplePermitted)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (GrantExactTriplePermitted): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, RevokeAllRemovesAllPermissions)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (RevokeAllRemovesAllPermissions): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, RevokeRemovesPermission)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (RevokeRemovesPermission): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, RevokeUnknownIsNoOp)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (RevokeUnknownIsNoOp): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, UnknownPrincipalAllOperationsDenied)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (UnknownPrincipalAllOperationsDenied): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, UnknownPrincipalDenied)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (UnknownPrincipalDenied): n/a

#### `TEST(InMemoryRbacMetadataSecurityProviderTest, WildcardResourceGrantsAllResources)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryRbacMetadataSecurityProviderTest): n/a
  - `<unnamed>` (WildcardResourceGrantsAllResources): n/a

#### `TEST(MetadataSecurityProviderPolymorphismTest, NoOpViaInterface)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSecurityProviderPolymorphismTest): n/a
  - `<unnamed>` (NoOpViaInterface): n/a

#### `TEST(MetadataSecurityProviderPolymorphismTest, RbacViaInterface)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSecurityProviderPolymorphismTest): n/a
  - `<unnamed>` (RbacViaInterface): n/a

#### `TEST(NoOpMetadataSecurityProviderTest, AssertPermissionNeverThrows)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (NoOpMetadataSecurityProviderTest): n/a
  - `<unnamed>` (AssertPermissionNeverThrows): n/a

#### `TEST(NoOpMetadataSecurityProviderTest, HasPermissionAlwaysTrue)`
- Source: `tests/metadata/test_metadata_security_provider.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (NoOpMetadataSecurityProviderTest): n/a
  - `<unnamed>` (HasPermissionAlwaysTrue): n/a

### test_metadata_security_rbac_diagnostics_focused.cpp

#### `TEST(MetadataSecurityRbacDiagnosticsTest, MCHSEC01_AdminGrantImpliesReadSchema)`
- Source: `tests/metadata/test_metadata_security_rbac_diagnostics_focused.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSecurityRbacDiagnosticsTest): n/a
  - `<unnamed>` (MCHSEC01_AdminGrantImpliesReadSchema): n/a

#### `TEST(MetadataSecurityRbacDiagnosticsTest, MCHSEC02_RevokeRemovesSpecificGrant)`
- Source: `tests/metadata/test_metadata_security_rbac_diagnostics_focused.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSecurityRbacDiagnosticsTest): n/a
  - `<unnamed>` (MCHSEC02_RevokeRemovesSpecificGrant): n/a

#### `TEST(MetadataSecurityRbacDiagnosticsTest, MCHSEC03_RevokeAllClearsAllGrants)`
- Source: `tests/metadata/test_metadata_security_rbac_diagnostics_focused.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSecurityRbacDiagnosticsTest): n/a
  - `<unnamed>` (MCHSEC03_RevokeAllClearsAllGrants): n/a

#### `TEST(MetadataSecurityRbacDiagnosticsTest, MCHSEC04_HasPermissionFalseAfterRevoke)`
- Source: `tests/metadata/test_metadata_security_rbac_diagnostics_focused.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSecurityRbacDiagnosticsTest): n/a
  - `<unnamed>` (MCHSEC04_HasPermissionFalseAfterRevoke): n/a

### test_metadata_shard.cpp

#### `TEST_F(MetadataShardRouterTest, ConsistentRouting)`
- Source: `tests/metadata/test_metadata_shard.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardRouterTest): n/a
  - `<unnamed>` (ConsistentRouting): n/a

#### `TEST_F(MetadataShardRouterTest, ListKeysScatterGather)`
- Source: `tests/metadata/test_metadata_shard.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardRouterTest): n/a
  - `<unnamed>` (ListKeysScatterGather): n/a

#### `TEST_F(MetadataShardRouterTest, RouterStatistics)`
- Source: `tests/metadata/test_metadata_shard.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardRouterTest): n/a
  - `<unnamed>` (RouterStatistics): n/a

#### `TEST_F(MetadataShardRouterTest, RoutingPutAndGet)`
- Source: `tests/metadata/test_metadata_shard.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardRouterTest): n/a
  - `<unnamed>` (RoutingPutAndGet): n/a

#### `TEST_F(MetadataShardTest, BasicPutAndGet)`
- Source: `tests/metadata/test_metadata_shard.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (BasicPutAndGet): n/a

#### `TEST_F(MetadataShardTest, BoundedCacheEviction)`
- Source: `tests/metadata/test_metadata_shard.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (BoundedCacheEviction): n/a

#### `TEST_F(MetadataShardTest, CacheHit)`
- Source: `tests/metadata/test_metadata_shard.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (CacheHit): n/a

#### `TEST_F(MetadataShardTest, ConcurrentPutSameKey_VersionIsMonotonicallyIncreasing)`
- Source: `tests/metadata/test_metadata_shard.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (ConcurrentPutSameKey_VersionIsMonotonicallyIncreasing): n/a

#### `TEST_F(MetadataShardTest, GetNonExistentKey)`
- Source: `tests/metadata/test_metadata_shard.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (GetNonExistentKey): n/a

#### `TEST_F(MetadataShardTest, GetStatistics)`
- Source: `tests/metadata/test_metadata_shard.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (GetStatistics): n/a

#### `TEST_F(MetadataShardTest, ListKeys)`
- Source: `tests/metadata/test_metadata_shard.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (ListKeys): n/a

#### `TEST_F(MetadataShardTest, MultiplePartitions)`
- Source: `tests/metadata/test_metadata_shard.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (MultiplePartitions): n/a

#### `TEST_F(MetadataShardTest, PartitionStatistics)`
- Source: `tests/metadata/test_metadata_shard.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (PartitionStatistics): n/a

#### `TEST_F(MetadataShardTest, Remove)`
- Source: `tests/metadata/test_metadata_shard.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (Remove): n/a

#### `TEST_F(MetadataShardTest, ThreadSafety)`
- Source: `tests/metadata/test_metadata_shard.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (ThreadSafety): n/a

#### `TEST_F(MetadataShardTest, UpdateIncrementsVersion)`
- Source: `tests/metadata/test_metadata_shard.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataShardTest): n/a
  - `<unnamed>` (UpdateIncrementsVersion): n/a

### test_metadata_snapshot.cpp

#### `TEST(MetadataSnapshotFocusedTests, ClearEmptiesStore)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (ClearEmptiesStore): n/a

#### `TEST(MetadataSnapshotFocusedTests, ConcurrentSaveLoadIsThreadSafe)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (ConcurrentSaveLoadIsThreadSafe): n/a

#### `TEST(MetadataSnapshotFocusedTests, FindTableReturnsCorrectPointer)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (FindTableReturnsCorrectPointer): n/a

#### `TEST(MetadataSnapshotFocusedTests, FindTableReturnsNullptrForUnknownName)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (FindTableReturnsNullptrForUnknownName): n/a

#### `TEST(MetadataSnapshotFocusedTests, ListSnapshotIdsSorted)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (ListSnapshotIdsSorted): n/a

#### `TEST(MetadataSnapshotFocusedTests, LoadReturnsNulloptForUnknownId)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (LoadReturnsNulloptForUnknownId): n/a

#### `TEST(MetadataSnapshotFocusedTests, LoadReturnsSavedSnapshot)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (LoadReturnsSavedSnapshot): n/a

#### `TEST(MetadataSnapshotFocusedTests, PolymorphicUsageViaInterface)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (PolymorphicUsageViaInterface): n/a

#### `TEST(MetadataSnapshotFocusedTests, RemoveReturnsTrueForExistingFalseForMissing)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (RemoveReturnsTrueForExistingFalseForMissing): n/a

#### `TEST(MetadataSnapshotFocusedTests, SaveEmptyIdThrowsMetadataSnapshotException)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (SaveEmptyIdThrowsMetadataSnapshotException): n/a

#### `TEST(MetadataSnapshotFocusedTests, SaveReturnsSnapshotId)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (SaveReturnsSnapshotId): n/a

#### `TEST(MetadataSnapshotFocusedTests, SizeReflectsAddAndRemove)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (SizeReflectsAddAndRemove): n/a

#### `TEST(MetadataSnapshotFocusedTests, TableCountIsZeroWhenNoTables)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (TableCountIsZeroWhenNoTables): n/a

#### `TEST(MetadataSnapshotFocusedTests, TableCountReturnsCorrectValue)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (TableCountReturnsCorrectValue): n/a

#### `TEST(MetadataSnapshotFocusedTests, ToJsonContainsExpectedFields)`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataSnapshotFocusedTests): n/a
  - `<unnamed>` (ToJsonContainsExpectedFields): n/a

#### `MetadataSnapshot makeSnapshot(const std::string &id, const std::string &author="test-author", const std::string &description="test snapshot", const std::vector< std::string > &table_names={})`
- Source: `tests/metadata/test_metadata_snapshot.cpp`:44
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `author` (const std::string &): n/a
  - `description` (const std::string &): n/a
  - `table_names` (const std::vector< std::string > &): n/a

### test_metadata_wal_recovery.cpp

#### `TEST_F(MetadataWALTest, DeleteOperationsInRecovery)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (DeleteOperationsInRecovery): n/a

#### `TEST_F(MetadataWALTest, LogMetadataOperations)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (LogMetadataOperations): n/a

#### `TEST_F(MetadataWALTest, MetadataShardWithPersistence)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (MetadataShardWithPersistence): n/a

#### `TEST_F(MetadataWALTest, ReadWALEntries)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (ReadWALEntries): n/a

#### `TEST_F(MetadataWALTest, RecoveryFromWAL)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (RecoveryFromWAL): n/a

#### `TEST_F(MetadataWALTest, SnapshotChecksumVerification)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:352
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (SnapshotChecksumVerification): n/a

#### `TEST_F(MetadataWALTest, SnapshotCleanup)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (SnapshotCleanup): n/a

#### `TEST_F(MetadataWALTest, SnapshotCreation)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (SnapshotCreation): n/a

#### `TEST_F(MetadataWALTest, SnapshotLoading)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (SnapshotLoading): n/a

#### `TEST_F(MetadataWALTest, SnapshotThreshold)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (SnapshotThreshold): n/a

#### `TEST_F(MetadataWALTest, WALEntrySerialization)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (WALEntrySerialization): n/a

#### `TEST_F(MetadataWALTest, WALInitialization)`
- Source: `tests/metadata/test_metadata_wal_recovery.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetadataWALTest): n/a
  - `<unnamed>` (WALInitialization): n/a

### test_schema_audit_log.cpp

#### `TEST_F(SchemaAuditLogTest, ConstructAndDestruct)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (ConstructAndDestruct): n/a

#### `TEST_F(SchemaAuditLogTest, EmptyHistoryOnFreshDb)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (EmptyHistoryOnFreshDb): n/a

#### `TEST_F(SchemaAuditLogTest, EntryToFromJSONRoundTrip)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (EntryToFromJSONRoundTrip): n/a

#### `TEST_F(SchemaAuditLogTest, FullHistoryEmptyOnFreshDb)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (FullHistoryEmptyOnFreshDb): n/a

#### `TEST_F(SchemaAuditLogTest, FullHistorySpansAllTables)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (FullHistorySpansAllTables): n/a

#### `TEST_F(SchemaAuditLogTest, FullHistoryToJSONIsArray)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (FullHistoryToJSONIsArray): n/a

#### `TEST_F(SchemaAuditLogTest, HistoryContainsMultipleEntriesInOrder)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (HistoryContainsMultipleEntriesInOrder): n/a

#### `TEST_F(SchemaAuditLogTest, HistoryContainsRecordedEntry)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (HistoryContainsRecordedEntry): n/a

#### `TEST_F(SchemaAuditLogTest, HistoryIsScopedToTable)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (HistoryIsScopedToTable): n/a

#### `TEST_F(SchemaAuditLogTest, HistoryToJSONContainsExtraMetadata)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (HistoryToJSONContainsExtraMetadata): n/a

#### `TEST_F(SchemaAuditLogTest, HistoryToJSONContainsTimestampField)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (HistoryToJSONContainsTimestampField): n/a

#### `TEST_F(SchemaAuditLogTest, HistoryToJSONIsArray)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (HistoryToJSONIsArray): n/a

#### `TEST_F(SchemaAuditLogTest, RecentHistoryRespectsLimit)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (RecentHistoryRespectsLimit): n/a

#### `TEST_F(SchemaAuditLogTest, RecentHistoryReturnsAllWhenUnderLimit)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (RecentHistoryReturnsAllWhenUnderLimit): n/a

#### `TEST_F(SchemaAuditLogTest, RecordReturnsTrueOnSuccess)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (RecordReturnsTrueOnSuccess): n/a

#### `TEST_F(SchemaAuditLogTest, RecordWithExtraMetadata)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (RecordWithExtraMetadata): n/a

#### `TEST_F(SchemaAuditLogTest, RecordWithMinimalArgs)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaAuditLogTest): n/a
  - `<unnamed>` (RecordWithMinimalArgs): n/a

#### `std::string makeTempDbPath(const std::string &prefix)`
- Source: `tests/metadata/test_schema_audit_log.cpp`:17
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a

### test_schema_consistency_checker.cpp

#### `TEST_F(SchemaConsistencyCheckerTest, BackgroundCheckRunsAndStops)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (BackgroundCheckRunsAndStops): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, ConstructWithAllComponents)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (ConstructWithAllComponents): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, ConstructWithNullOptionals)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (ConstructWithNullOptionals): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, DestructorStopsBackgroundThread)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (DestructorStopsBackgroundThread): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, EmptyDatabaseNoIssues)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (EmptyDatabaseNoIssues): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, FreshStatsNotReportedAsStale)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (FreshStatsNotReportedAsStale): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, GetLastCheckResultsAfterRun)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (GetLastCheckResultsAfterRun): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, GetLastCheckResultsBeforeRun)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (GetLastCheckResultsBeforeRun): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, IssueToJSON)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (IssueToJSON): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, LastResultsToJSONIsArray)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (LastResultsToJSONIsArray): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, NoStatsReportedAsStale)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (NoStatsReportedAsStale): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, RegisteredTableKeyNotOrphan)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (RegisteredTableKeyNotOrphan): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, StopBackgroundCheckIdempotent)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (StopBackgroundCheckIdempotent): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, SystemKeyPrefixesNotOrphan)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (SystemKeyPrefixesNotOrphan): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, TableWithConstraintsNotReported)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (TableWithConstraintsNotReported): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, TableWithNoConstraintsReported)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (TableWithNoConstraintsReported): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, UnregisteredPrefixIsDiscoveredAsTable)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (UnregisteredPrefixIsDiscoveredAsTable): n/a

#### `TEST_F(SchemaConsistencyCheckerTest, ZeroIntervalDisablesBackgroundCheck)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyCheckerTest): n/a
  - `<unnamed>` (ZeroIntervalDisablesBackgroundCheck): n/a

#### `std::string makeTempDbPath(const std::string &prefix)`
- Source: `tests/metadata/test_schema_consistency_checker.cpp`:20
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a

### test_schema_consistency_edge_cases_focused.cpp

#### `TEST(SchemaConsistencyEdgeCasesTest, MCHC01_DefaultConstructedIsEmpty)`
- Source: `tests/metadata/test_schema_consistency_edge_cases_focused.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyEdgeCasesTest): n/a
  - `<unnamed>` (MCHC01_DefaultConstructedIsEmpty): n/a

#### `TEST(SchemaConsistencyEdgeCasesTest, MCHC02_EmptyTableNameSerializes)`
- Source: `tests/metadata/test_schema_consistency_edge_cases_focused.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyEdgeCasesTest): n/a
  - `<unnamed>` (MCHC02_EmptyTableNameSerializes): n/a

#### `TEST(SchemaConsistencyEdgeCasesTest, MCHC03_VectorOfIssues)`
- Source: `tests/metadata/test_schema_consistency_edge_cases_focused.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyEdgeCasesTest): n/a
  - `<unnamed>` (MCHC03_VectorOfIssues): n/a

#### `TEST(SchemaConsistencyEdgeCasesTest, MCHC04_DetailFieldRoundTrip)`
- Source: `tests/metadata/test_schema_consistency_edge_cases_focused.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyEdgeCasesTest): n/a
  - `<unnamed>` (MCHC04_DetailFieldRoundTrip): n/a

#### `TEST(SchemaConsistencyEdgeCasesTest, MCHC05_OrphanKeyIssueTypeSerializes)`
- Source: `tests/metadata/test_schema_consistency_edge_cases_focused.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyEdgeCasesTest): n/a
  - `<unnamed>` (MCHC05_OrphanKeyIssueTypeSerializes): n/a

#### `TEST(SchemaConsistencyEdgeCasesTest, MCHC06_StaleStatsIssueTypeSerializes)`
- Source: `tests/metadata/test_schema_consistency_edge_cases_focused.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyEdgeCasesTest): n/a
  - `<unnamed>` (MCHC06_StaleStatsIssueTypeSerializes): n/a

#### `TEST(SchemaConsistencyEdgeCasesTest, MCHC07_MissingConstraintIssueTypeSerializes)`
- Source: `tests/metadata/test_schema_consistency_edge_cases_focused.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyEdgeCasesTest): n/a
  - `<unnamed>` (MCHC07_MissingConstraintIssueTypeSerializes): n/a

#### `TEST(SchemaConsistencyEdgeCasesTest, MCHC08_VectorSerializesToJsonArray)`
- Source: `tests/metadata/test_schema_consistency_edge_cases_focused.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConsistencyEdgeCasesTest): n/a
  - `<unnamed>` (MCHC08_VectorSerializesToJsonArray): n/a

### test_schema_constraints.cpp

#### `TEST(ConstraintFactoryTest, MakeCheck)`
- Source: `tests/metadata/test_schema_constraints.cpp`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConstraintFactoryTest): n/a
  - `<unnamed>` (MakeCheck): n/a

#### `TEST(ConstraintFactoryTest, MakeDefault)`
- Source: `tests/metadata/test_schema_constraints.cpp`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConstraintFactoryTest): n/a
  - `<unnamed>` (MakeDefault): n/a

#### `TEST(ConstraintFactoryTest, MakeForeignKey)`
- Source: `tests/metadata/test_schema_constraints.cpp`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConstraintFactoryTest): n/a
  - `<unnamed>` (MakeForeignKey): n/a

#### `TEST(ConstraintFactoryTest, MakeNotNull)`
- Source: `tests/metadata/test_schema_constraints.cpp`:16
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConstraintFactoryTest): n/a
  - `<unnamed>` (MakeNotNull): n/a

#### `TEST(ConstraintFactoryTest, MakeUnique)`
- Source: `tests/metadata/test_schema_constraints.cpp`:22
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConstraintFactoryTest): n/a
  - `<unnamed>` (MakeUnique): n/a

#### `TEST_F(SchemaConstraintsTest, AddAndGetColumnConstraints)`
- Source: `tests/metadata/test_schema_constraints.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (AddAndGetColumnConstraints): n/a

#### `TEST_F(SchemaConstraintsTest, AddMultipleConstraintsToColumn)`
- Source: `tests/metadata/test_schema_constraints.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (AddMultipleConstraintsToColumn): n/a

#### `TEST_F(SchemaConstraintsTest, ApplyDefaultsDoesNotOverwriteExistingValue)`
- Source: `tests/metadata/test_schema_constraints.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ApplyDefaultsDoesNotOverwriteExistingValue): n/a

#### `TEST_F(SchemaConstraintsTest, ApplyDefaultsIntegerDefault)`
- Source: `tests/metadata/test_schema_constraints.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ApplyDefaultsIntegerDefault): n/a

#### `TEST_F(SchemaConstraintsTest, ApplyDefaultsMissingColumn)`
- Source: `tests/metadata/test_schema_constraints.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ApplyDefaultsMissingColumn): n/a

#### `TEST_F(SchemaConstraintsTest, ApplyDefaultsNullColumn)`
- Source: `tests/metadata/test_schema_constraints.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ApplyDefaultsNullColumn): n/a

#### `TEST_F(SchemaConstraintsTest, ApplyDefaultsStringDefault)`
- Source: `tests/metadata/test_schema_constraints.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ApplyDefaultsStringDefault): n/a

#### `TEST_F(SchemaConstraintsTest, ColumnConstraintToJSONCheck)`
- Source: `tests/metadata/test_schema_constraints.cpp`:346
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ColumnConstraintToJSONCheck): n/a

#### `TEST_F(SchemaConstraintsTest, ColumnConstraintToJSONDefault)`
- Source: `tests/metadata/test_schema_constraints.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ColumnConstraintToJSONDefault): n/a

#### `TEST_F(SchemaConstraintsTest, ColumnConstraintToJSONForeignKey)`
- Source: `tests/metadata/test_schema_constraints.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ColumnConstraintToJSONForeignKey): n/a

#### `TEST_F(SchemaConstraintsTest, ColumnConstraintToJSONNotNull)`
- Source: `tests/metadata/test_schema_constraints.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ColumnConstraintToJSONNotNull): n/a

#### `TEST_F(SchemaConstraintsTest, ConstraintViolationToJSON)`
- Source: `tests/metadata/test_schema_constraints.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ConstraintViolationToJSON): n/a

#### `TEST_F(SchemaConstraintsTest, EnforceCheckEqual)`
- Source: `tests/metadata/test_schema_constraints.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (EnforceCheckEqual): n/a

#### `TEST_F(SchemaConstraintsTest, EnforceCheckGreaterOrEqual)`
- Source: `tests/metadata/test_schema_constraints.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (EnforceCheckGreaterOrEqual): n/a

#### `TEST_F(SchemaConstraintsTest, EnforceCheckPassGreaterThan)`
- Source: `tests/metadata/test_schema_constraints.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (EnforceCheckPassGreaterThan): n/a

#### `TEST_F(SchemaConstraintsTest, EnforceCheckViolationGreaterThan)`
- Source: `tests/metadata/test_schema_constraints.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (EnforceCheckViolationGreaterThan): n/a

#### `TEST_F(SchemaConstraintsTest, EnforceMultipleViolations)`
- Source: `tests/metadata/test_schema_constraints.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (EnforceMultipleViolations): n/a

#### `TEST_F(SchemaConstraintsTest, EnforceNoConstraintsForTable)`
- Source: `tests/metadata/test_schema_constraints.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (EnforceNoConstraintsForTable): n/a

#### `TEST_F(SchemaConstraintsTest, EnforceNotNullExplicitNull)`
- Source: `tests/metadata/test_schema_constraints.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (EnforceNotNullExplicitNull): n/a

#### `TEST_F(SchemaConstraintsTest, EnforceNotNullOK)`
- Source: `tests/metadata/test_schema_constraints.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (EnforceNotNullOK): n/a

#### `TEST_F(SchemaConstraintsTest, EnforceNotNullViolation)`
- Source: `tests/metadata/test_schema_constraints.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (EnforceNotNullViolation): n/a

#### `TEST_F(SchemaConstraintsTest, GetConstraintsMissingTable)`
- Source: `tests/metadata/test_schema_constraints.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (GetConstraintsMissingTable): n/a

#### `TEST_F(SchemaConstraintsTest, GetTableConstraints)`
- Source: `tests/metadata/test_schema_constraints.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (GetTableConstraints): n/a

#### `TEST_F(SchemaConstraintsTest, RemoveColumnConstraints)`
- Source: `tests/metadata/test_schema_constraints.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (RemoveColumnConstraints): n/a

#### `TEST_F(SchemaConstraintsTest, RemoveTableConstraints)`
- Source: `tests/metadata/test_schema_constraints.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (RemoveTableConstraints): n/a

#### `TEST_F(SchemaConstraintsTest, ToJSONAndFromJSON)`
- Source: `tests/metadata/test_schema_constraints.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsTest): n/a
  - `<unnamed>` (ToJSONAndFromJSON): n/a

### test_schema_constraints_persistence.cpp

#### `TEST_F(SchemaConstraintsPersistenceTest, LoadFromEmptyDb)`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsPersistenceTest): n/a
  - `<unnamed>` (LoadFromEmptyDb): n/a

#### `TEST_F(SchemaConstraintsPersistenceTest, LoadTableNotFound)`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsPersistenceTest): n/a
  - `<unnamed>` (LoadTableNotFound): n/a

#### `TEST_F(SchemaConstraintsPersistenceTest, PersistAndReloadAllTables)`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsPersistenceTest): n/a
  - `<unnamed>` (PersistAndReloadAllTables): n/a

#### `TEST_F(SchemaConstraintsPersistenceTest, PersistAndReloadDefaultValue)`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsPersistenceTest): n/a
  - `<unnamed>` (PersistAndReloadDefaultValue): n/a

#### `TEST_F(SchemaConstraintsPersistenceTest, PersistAndReloadForeignKey)`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsPersistenceTest): n/a
  - `<unnamed>` (PersistAndReloadForeignKey): n/a

#### `TEST_F(SchemaConstraintsPersistenceTest, PersistEmptyConstraints)`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsPersistenceTest): n/a
  - `<unnamed>` (PersistEmptyConstraints): n/a

#### `TEST_F(SchemaConstraintsPersistenceTest, PersistReloadAndEnforce)`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsPersistenceTest): n/a
  - `<unnamed>` (PersistReloadAndEnforce): n/a

#### `TEST_F(SchemaConstraintsPersistenceTest, PersistTableAndReload)`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraintsPersistenceTest): n/a
  - `<unnamed>` (PersistTableAndReload): n/a

#### `std::string makeTempDbPath(const std::string &prefix)`
- Source: `tests/metadata/test_schema_constraints_persistence.cpp`:13
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a

### test_schema_diff.cpp

#### `TEST(SchemaDiffFocusedTests, AddedColumnReportedAsAdded)`
- Source: `tests/metadata/test_schema_diff.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (AddedColumnReportedAsAdded): n/a

#### `TEST(SchemaDiffFocusedTests, AddedIndexReportedAsAdded)`
- Source: `tests/metadata/test_schema_diff.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (AddedIndexReportedAsAdded): n/a

#### `TEST(SchemaDiffFocusedTests, ChangedIndexTypeReportedAsChanged)`
- Source: `tests/metadata/test_schema_diff.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (ChangedIndexTypeReportedAsChanged): n/a

#### `TEST(SchemaDiffFocusedTests, ColumnCountHelpersAreCorrect)`
- Source: `tests/metadata/test_schema_diff.cpp`:250
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (ColumnCountHelpersAreCorrect): n/a

#### `TEST(SchemaDiffFocusedTests, ColumnDiffToJsonHasCorrectFields)`
- Source: `tests/metadata/test_schema_diff.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (ColumnDiffToJsonHasCorrectFields): n/a

#### `TEST(SchemaDiffFocusedTests, ColumnDiffToJsonNullValuesForAdded)`
- Source: `tests/metadata/test_schema_diff.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (ColumnDiffToJsonNullValuesForAdded): n/a

#### `TEST(SchemaDiffFocusedTests, ColumnDiffsSortedAlphabetically)`
- Source: `tests/metadata/test_schema_diff.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (ColumnDiffsSortedAlphabetically): n/a

#### `TEST(SchemaDiffFocusedTests, EmptyFromSchemaGivesAllColumnsAdded)`
- Source: `tests/metadata/test_schema_diff.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (EmptyFromSchemaGivesAllColumnsAdded): n/a

#### `TEST(SchemaDiffFocusedTests, IdenticalSchemasProduceEmptyDiff)`
- Source: `tests/metadata/test_schema_diff.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (IdenticalSchemasProduceEmptyDiff): n/a

#### `TEST(SchemaDiffFocusedTests, IndexChangeReportedAsIndexChanged)`
- Source: `tests/metadata/test_schema_diff.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (IndexChangeReportedAsIndexChanged): n/a

#### `TEST(SchemaDiffFocusedTests, IndexDiffToJsonHasCorrectFields)`
- Source: `tests/metadata/test_schema_diff.cpp`:396
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (IndexDiffToJsonHasCorrectFields): n/a

#### `TEST(SchemaDiffFocusedTests, IndexDiffsSortedAlphabetically)`
- Source: `tests/metadata/test_schema_diff.cpp`:293
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (IndexDiffsSortedAlphabetically): n/a

#### `TEST(SchemaDiffFocusedTests, NullabilityChangeReportedAsNullabilityChanged)`
- Source: `tests/metadata/test_schema_diff.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (NullabilityChangeReportedAsNullabilityChanged): n/a

#### `TEST(SchemaDiffFocusedTests, RemovedColumnReportedAsRemoved)`
- Source: `tests/metadata/test_schema_diff.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (RemovedColumnReportedAsRemoved): n/a

#### `TEST(SchemaDiffFocusedTests, RemovedIndexReportedAsRemoved)`
- Source: `tests/metadata/test_schema_diff.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (RemovedIndexReportedAsRemoved): n/a

#### `TEST(SchemaDiffFocusedTests, ToJsonContainsExpectedFields)`
- Source: `tests/metadata/test_schema_diff.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (ToJsonContainsExpectedFields): n/a

#### `TEST(SchemaDiffFocusedTests, TypeChangeReportedAsTypeChanged)`
- Source: `tests/metadata/test_schema_diff.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDiffFocusedTests): n/a
  - `<unnamed>` (TypeChangeReportedAsTypeChanged): n/a

#### `SchemaManager::TableSchema makeSchema(const std::string &table_name, const std::vector< ColumnSpec > &cols, const std::vector< IndexSpec > &idxs={})`
- Source: `tests/metadata/test_schema_diff.cpp`:52
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a
  - `cols` (const std::vector< ColumnSpec > &): n/a
  - `idxs` (const std::vector< IndexSpec > &): n/a

### test_schema_manager.cpp

#### `TEST_F(SchemaManagerTest, AdaptiveTTLCacheExpiresEarlierUnderLoad)`
- Source: `tests/metadata/test_schema_manager.cpp`:958
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (AdaptiveTTLCacheExpiresEarlierUnderLoad): n/a

#### `TEST_F(SchemaManagerTest, AdaptiveTTLClampedToMinimum)`
- Source: `tests/metadata/test_schema_manager.cpp`:921
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (AdaptiveTTLClampedToMinimum): n/a

#### `TEST_F(SchemaManagerTest, AdaptiveTTLDefaultDisabled)`
- Source: `tests/metadata/test_schema_manager.cpp`:870
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (AdaptiveTTLDefaultDisabled): n/a

#### `TEST_F(SchemaManagerTest, AdaptiveTTLDisableRestoresFixedTTL)`
- Source: `tests/metadata/test_schema_manager.cpp`:942
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (AdaptiveTTLDisableRestoresFixedTTL): n/a

#### `TEST_F(SchemaManagerTest, AdaptiveTTLHighMutationRateReducesTTL)`
- Source: `tests/metadata/test_schema_manager.cpp`:898
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (AdaptiveTTLHighMutationRateReducesTTL): n/a

#### `TEST_F(SchemaManagerTest, AdaptiveTTLNoMutationsUsesMaxTTL)`
- Source: `tests/metadata/test_schema_manager.cpp`:880
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (AdaptiveTTLNoMutationsUsesMaxTTL): n/a

#### `TEST_F(SchemaManagerTest, CacheMechanism)`
- Source: `tests/metadata/test_schema_manager.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (CacheMechanism): n/a

#### `TEST_F(SchemaManagerTest, CacheTTL)`
- Source: `tests/metadata/test_schema_manager.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (CacheTTL): n/a

#### `TEST_F(SchemaManagerTest, CapabilitiesJSON)`
- Source: `tests/metadata/test_schema_manager.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (CapabilitiesJSON): n/a

#### `TEST_F(SchemaManagerTest, CustomSchemaOverridesDiscovered)`
- Source: `tests/metadata/test_schema_manager.cpp`:832
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (CustomSchemaOverridesDiscovered): n/a

#### `TEST_F(SchemaManagerTest, CustomSchemaPersistedAcrossInstances)`
- Source: `tests/metadata/test_schema_manager.cpp`:484
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (CustomSchemaPersistedAcrossInstances): n/a

#### `TEST_F(SchemaManagerTest, DatabaseMetadata)`
- Source: `tests/metadata/test_schema_manager.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (DatabaseMetadata): n/a

#### `TEST_F(SchemaManagerTest, DeleteCustomSchema)`
- Source: `tests/metadata/test_schema_manager.cpp`:590
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (DeleteCustomSchema): n/a

#### `TEST_F(SchemaManagerTest, DeleteNonExistentSchema)`
- Source: `tests/metadata/test_schema_manager.cpp`:612
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (DeleteNonExistentSchema): n/a

#### `TEST_F(SchemaManagerTest, DiscoverMultipleTables)`
- Source: `tests/metadata/test_schema_manager.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (DiscoverMultipleTables): n/a

#### `TEST_F(SchemaManagerTest, DiscoverSingleTable)`
- Source: `tests/metadata/test_schema_manager.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (DiscoverSingleTable): n/a

#### `TEST_F(SchemaManagerTest, EmptyDatabase)`
- Source: `tests/metadata/test_schema_manager.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (EmptyDatabase): n/a

#### `TEST_F(SchemaManagerTest, GetTableByName)`
- Source: `tests/metadata/test_schema_manager.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (GetTableByName): n/a

#### `TEST_F(SchemaManagerTest, IndexDiscovery)`
- Source: `tests/metadata/test_schema_manager.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (IndexDiscovery): n/a

#### `TEST_F(SchemaManagerTest, InternalBinaryKeyspacesDoNotCrashSchemaExport)`
- Source: `tests/metadata/test_schema_manager.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (InternalBinaryKeyspacesDoNotCrashSchemaExport): n/a

#### `TEST_F(SchemaManagerTest, JSONExport)`
- Source: `tests/metadata/test_schema_manager.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (JSONExport): n/a

#### `TEST_F(SchemaManagerTest, ParseTableSchemaFromJSON)`
- Source: `tests/metadata/test_schema_manager.cpp`:747
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ParseTableSchemaFromJSON): n/a

#### `TEST_F(SchemaManagerTest, ParseTableSchemaInvalidJSON)`
- Source: `tests/metadata/test_schema_manager.cpp`:816
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ParseTableSchemaInvalidJSON): n/a

#### `TEST_F(SchemaManagerTest, ParseTableSchemaMinimal)`
- Source: `tests/metadata/test_schema_manager.cpp`:793
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ParseTableSchemaMinimal): n/a

#### `TEST_F(SchemaManagerTest, ParseTableSchemaMissingName)`
- Source: `tests/metadata/test_schema_manager.cpp`:806
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ParseTableSchemaMissingName): n/a

#### `TEST_F(SchemaManagerTest, PatchNonExistentTable)`
- Source: `tests/metadata/test_schema_manager.cpp`:580
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (PatchNonExistentTable): n/a

#### `TEST_F(SchemaManagerTest, PatchSchemaAddProperty)`
- Source: `tests/metadata/test_schema_manager.cpp`:515
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (PatchSchemaAddProperty): n/a

#### `TEST_F(SchemaManagerTest, PatchSchemaUpdateType)`
- Source: `tests/metadata/test_schema_manager.cpp`:558
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (PatchSchemaUpdateType): n/a

#### `TEST_F(SchemaManagerTest, PerformanceCacheHitRate)`
- Source: `tests/metadata/test_schema_manager.cpp`:413
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (PerformanceCacheHitRate): n/a

#### `TEST_F(SchemaManagerTest, PerformanceDiscoveryTime)`
- Source: `tests/metadata/test_schema_manager.cpp`:381
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (PerformanceDiscoveryTime): n/a

#### `TEST_F(SchemaManagerTest, RecordMutationMultipleTablesIndependent)`
- Source: `tests/metadata/test_schema_manager.cpp`:995
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (RecordMutationMultipleTablesIndependent): n/a

#### `TEST_F(SchemaManagerTest, SetCustomSchema)`
- Source: `tests/metadata/test_schema_manager.cpp`:443
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (SetCustomSchema): n/a

#### `TEST_F(SchemaManagerTest, TableToJSON)`
- Source: `tests/metadata/test_schema_manager.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (TableToJSON): n/a

#### `TEST_F(SchemaManagerTest, ValidateDuplicatePropertyNames)`
- Source: `tests/metadata/test_schema_manager.cpp`:659
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ValidateDuplicatePropertyNames): n/a

#### `TEST_F(SchemaManagerTest, ValidateEmptyTableName)`
- Source: `tests/metadata/test_schema_manager.cpp`:623
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ValidateEmptyTableName): n/a

#### `TEST_F(SchemaManagerTest, ValidateIndexReferencesNonExistentProperty)`
- Source: `tests/metadata/test_schema_manager.cpp`:698
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ValidateIndexReferencesNonExistentProperty): n/a

#### `TEST_F(SchemaManagerTest, ValidateInvalidPropertyType)`
- Source: `tests/metadata/test_schema_manager.cpp`:681
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ValidateInvalidPropertyType): n/a

#### `TEST_F(SchemaManagerTest, ValidateInvalidTableName)`
- Source: `tests/metadata/test_schema_manager.cpp`:635
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ValidateInvalidTableName): n/a

#### `TEST_F(SchemaManagerTest, ValidateInvalidTableType)`
- Source: `tests/metadata/test_schema_manager.cpp`:647
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ValidateInvalidTableType): n/a

#### `TEST_F(SchemaManagerTest, ValidateValidSchema)`
- Source: `tests/metadata/test_schema_manager.cpp`:721
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManagerTest): n/a
  - `<unnamed>` (ValidateValidSchema): n/a

#### `std::string makeTempDbPath(const std::string &name)`
- Source: `tests/metadata/test_schema_manager.cpp`:19
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

### test_schema_migration_regression.cpp

#### `TEST_F(MigrationRegressionTest, AuditLogPopulatedDuringMigrations)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (AuditLogPopulatedDuringMigrations): n/a

#### `TEST_F(MigrationRegressionTest, DiffAcrossMigrations)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (DiffAcrossMigrations): n/a

#### `TEST_F(MigrationRegressionTest, ForwardMigration5Versions)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (ForwardMigration5Versions): n/a

#### `TEST_F(MigrationRegressionTest, GetVersionRetrievesCorrectSnapshot)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (GetVersionRetrievesCorrectSnapshot): n/a

#### `TEST_F(MigrationRegressionTest, HistoryToJSON)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (HistoryToJSON): n/a

#### `TEST_F(MigrationRegressionTest, MultipleRollbacks)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (MultipleRollbacks): n/a

#### `TEST_F(MigrationRegressionTest, RollbackFromV5ToV2)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (RollbackFromV5ToV2): n/a

#### `TEST_F(MigrationRegressionTest, RollbackToNonexistentVersion)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (RollbackToNonexistentVersion): n/a

#### `TEST_F(MigrationRegressionTest, RollbackToV1)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (RollbackToV1): n/a

#### `TEST_F(MigrationRegressionTest, RollbackToVersionZero)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (RollbackToVersionZero): n/a

#### `TEST_F(MigrationRegressionTest, ValidateMigration_GatesFullMigrationLifecycle)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:419
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (ValidateMigration_GatesFullMigrationLifecycle): n/a

#### `TEST_F(MigrationRegressionTest, ValidateMigration_PassesAfterRealChange)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:390
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (ValidateMigration_PassesAfterRealChange): n/a

#### `TEST_F(MigrationRegressionTest, ValidateMigration_PassesOnFirstVersion)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:375
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (ValidateMigration_PassesOnFirstVersion): n/a

#### `TEST_F(MigrationRegressionTest, ValidateMigration_RejectsEmptyName)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:454
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (ValidateMigration_RejectsEmptyName): n/a

#### `TEST_F(MigrationRegressionTest, ValidateMigration_RejectsIdenticalSchema)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (ValidateMigration_RejectsIdenticalSchema): n/a

#### `TEST_F(MigrationRegressionTest, ValidateMigration_RejectsNoColumns)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:468
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationRegressionTest): n/a
  - `<unnamed>` (ValidateMigration_RejectsNoColumns): n/a

#### `std::string makeTempDbPath(const std::string &prefix)`
- Source: `tests/metadata/test_schema_migration_regression.cpp`:21
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a

### test_schema_migration_script.cpp

#### `TEST_F(MigrationScriptTest, AddColumnProducesAddStatement)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (AddColumnProducesAddStatement): n/a

#### `TEST_F(MigrationScriptTest, AddMultipleColumns)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (AddMultipleColumns): n/a

#### `TEST_F(MigrationScriptTest, CombinedAddAndDrop)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:435
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (CombinedAddAndDrop): n/a

#### `TEST_F(MigrationScriptTest, DowngradeMigrationScript)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:455
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (DowngradeMigrationScript): n/a

#### `TEST_F(MigrationScriptTest, DropColumnProducesDropStatement)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (DropColumnProducesDropStatement): n/a

#### `TEST_F(MigrationScriptTest, DropMultipleColumns)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (DropMultipleColumns): n/a

#### `TEST_F(MigrationScriptTest, EmptyTableNameFails)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (EmptyTableNameFails): n/a

#### `TEST_F(MigrationScriptTest, IdenticalVersionsProducesHeaderOnly)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (IdenticalVersionsProducesHeaderOnly): n/a

#### `TEST_F(MigrationScriptTest, NonexistentFromVersionFails)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (NonexistentFromVersionFails): n/a

#### `TEST_F(MigrationScriptTest, NonexistentToVersionFails)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (NonexistentToVersionFails): n/a

#### `TEST_F(MigrationScriptTest, NotNullColumnHasNotNullClause)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (NotNullColumnHasNotNullClause): n/a

#### `TEST_F(MigrationScriptTest, NullabilityChangeProducesSetNotNull)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (NullabilityChangeProducesSetNotNull): n/a

#### `TEST_F(MigrationScriptTest, NullabilityRelaxedProducesDropNotNull)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (NullabilityRelaxedProducesDropNotNull): n/a

#### `TEST_F(MigrationScriptTest, TypeChangeProducesAlterStatement)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (TypeChangeProducesAlterStatement): n/a

#### `TEST_F(MigrationScriptTest, TypeMappingBoolean)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (TypeMappingBoolean): n/a

#### `TEST_F(MigrationScriptTest, TypeMappingInteger)`
- Source: `tests/metadata/test_schema_migration_script.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (MigrationScriptTest): n/a
  - `<unnamed>` (TypeMappingInteger): n/a

### test_schema_version_dryrun.cpp

#### `TEST_F(DryRunMigrationTest, DifferentSchemaFromCurrentVersionPasses)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (DryRunMigrationTest): n/a
  - `<unnamed>` (DifferentSchemaFromCurrentVersionPasses): n/a

#### `TEST_F(DryRunMigrationTest, DryRunLeavesHistoryUnchanged)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (DryRunMigrationTest): n/a
  - `<unnamed>` (DryRunLeavesHistoryUnchanged): n/a

#### `TEST_F(DryRunMigrationTest, DuplicateColumnFails)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (DryRunMigrationTest): n/a
  - `<unnamed>` (DuplicateColumnFails): n/a

#### `TEST_F(DryRunMigrationTest, EmptyColumnNameFails)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (DryRunMigrationTest): n/a
  - `<unnamed>` (EmptyColumnNameFails): n/a

#### `TEST_F(DryRunMigrationTest, EmptyNameFails)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (DryRunMigrationTest): n/a
  - `<unnamed>` (EmptyNameFails): n/a

#### `TEST_F(DryRunMigrationTest, IdenticalSchemaToCurrentVersionFails)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (DryRunMigrationTest): n/a
  - `<unnamed>` (IdenticalSchemaToCurrentVersionFails): n/a

#### `TEST_F(DryRunMigrationTest, NoColumnsFails)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (DryRunMigrationTest): n/a
  - `<unnamed>` (NoColumnsFails): n/a

#### `TEST_F(DryRunMigrationTest, ValidSchemaDoesNotPersist)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (DryRunMigrationTest): n/a
  - `<unnamed>` (ValidSchemaDoesNotPersist): n/a

#### `TEST_F(DryRunMigrationTest, ValidSchemaNoVersionPasses)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (DryRunMigrationTest): n/a
  - `<unnamed>` (ValidSchemaNoVersionPasses): n/a

#### `std::string makeTempDbPath(const std::string &prefix)`
- Source: `tests/metadata/test_schema_version_dryrun.cpp`:17
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a

### test_schema_version_manager.cpp

#### `TEST_F(SchemaVersionManagerAuditTest, CreateVersionWritesToAuditLog)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerAuditTest): n/a
  - `<unnamed>` (CreateVersionWritesToAuditLog): n/a

#### `TEST_F(SchemaVersionManagerAuditTest, MultipleVersionsAllAppearInAuditLog)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:428
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerAuditTest): n/a
  - `<unnamed>` (MultipleVersionsAllAppearInAuditLog): n/a

#### `TEST_F(SchemaVersionManagerAuditTest, NoAuditLogSetDoesNotCrash)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerAuditTest): n/a
  - `<unnamed>` (NoAuditLogSetDoesNotCrash): n/a

#### `TEST_F(SchemaVersionManagerAuditTest, RollbackWritesToAuditLog)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:388
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerAuditTest): n/a
  - `<unnamed>` (RollbackWritesToAuditLog): n/a

#### `TEST_F(SchemaVersionManagerTest, CreateFirstVersion)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (CreateFirstVersion): n/a

#### `TEST_F(SchemaVersionManagerTest, CreateVersionUnknownTable)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (CreateVersionUnknownTable): n/a

#### `TEST_F(SchemaVersionManagerTest, DiffVersionsAddedProperty)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (DiffVersionsAddedProperty): n/a

#### `TEST_F(SchemaVersionManagerTest, DiffVersionsRemovedProperty)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (DiffVersionsRemovedProperty): n/a

#### `TEST_F(SchemaVersionManagerTest, EmptyTableName)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (EmptyTableName): n/a

#### `TEST_F(SchemaVersionManagerTest, GetChangeHistoryEmpty)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (GetChangeHistoryEmpty): n/a

#### `TEST_F(SchemaVersionManagerTest, GetChangeHistoryMultiple)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (GetChangeHistoryMultiple): n/a

#### `TEST_F(SchemaVersionManagerTest, GetCurrentVersionAfterCreate)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (GetCurrentVersionAfterCreate): n/a

#### `TEST_F(SchemaVersionManagerTest, GetCurrentVersionNoHistory)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (GetCurrentVersionNoHistory): n/a

#### `TEST_F(SchemaVersionManagerTest, GetVersionNotFound)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (GetVersionNotFound): n/a

#### `TEST_F(SchemaVersionManagerTest, GetVersionSpecific)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (GetVersionSpecific): n/a

#### `TEST_F(SchemaVersionManagerTest, GetVersionZeroInvalid)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (GetVersionZeroInvalid): n/a

#### `TEST_F(SchemaVersionManagerTest, HistoryToJSON)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (HistoryToJSON): n/a

#### `TEST_F(SchemaVersionManagerTest, RollbackToVersion)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (RollbackToVersion): n/a

#### `TEST_F(SchemaVersionManagerTest, RollbackVersionNotFound)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (RollbackVersionNotFound): n/a

#### `TEST_F(SchemaVersionManagerTest, SchemaChangeToJSON)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:300
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (SchemaChangeToJSON): n/a

#### `TEST_F(SchemaVersionManagerTest, VersionsIncrement)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerTest): n/a
  - `<unnamed>` (VersionsIncrement): n/a

#### `std::string makeTempDbPath(const std::string &prefix)`
- Source: `tests/metadata/test_schema_version_manager.cpp`:17
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a

### test_schema_version_manager_lock_contract_focused.cpp

#### `TEST(SchemaVersionManagerLockContractTest, MCHL01_FailureResultCarriesErrorCode)`
- Source: `tests/metadata/test_schema_version_manager_lock_contract_focused.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerLockContractTest): n/a
  - `<unnamed>` (MCHL01_FailureResultCarriesErrorCode): n/a

#### `TEST(SchemaVersionManagerLockContractTest, MCHL02_FailureResultOkIsFalse)`
- Source: `tests/metadata/test_schema_version_manager_lock_contract_focused.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerLockContractTest): n/a
  - `<unnamed>` (MCHL02_FailureResultOkIsFalse): n/a

#### `TEST(SchemaVersionManagerLockContractTest, MCHL03_ErrorCodesAreDistinct)`
- Source: `tests/metadata/test_schema_version_manager_lock_contract_focused.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerLockContractTest): n/a
  - `<unnamed>` (MCHL03_ErrorCodesAreDistinct): n/a

#### `TEST(SchemaVersionManagerLockContractTest, MCHL04_SuccessResultOkIsTrue)`
- Source: `tests/metadata/test_schema_version_manager_lock_contract_focused.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManagerLockContractTest): n/a
  - `<unnamed>` (MCHL04_SuccessResultOkIsTrue): n/a

### test_statistics_auto_refresh.cpp

#### `TEST_F(StatisticsAutoRefreshTest, AutoRefreshUpdatesStats)`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsAutoRefreshTest): n/a
  - `<unnamed>` (AutoRefreshUpdatesStats): n/a

#### `TEST_F(StatisticsAutoRefreshTest, DestructorStopsThread)`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsAutoRefreshTest): n/a
  - `<unnamed>` (DestructorStopsThread): n/a

#### `TEST_F(StatisticsAutoRefreshTest, MetricsHookCalledDuringAutoRefresh)`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsAutoRefreshTest): n/a
  - `<unnamed>` (MetricsHookCalledDuringAutoRefresh): n/a

#### `TEST_F(StatisticsAutoRefreshTest, MultipleSetRefreshIntervalCalls)`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsAutoRefreshTest): n/a
  - `<unnamed>` (MultipleSetRefreshIntervalCalls): n/a

#### `TEST_F(StatisticsAutoRefreshTest, StopRefreshIdempotent)`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsAutoRefreshTest): n/a
  - `<unnamed>` (StopRefreshIdempotent): n/a

#### `TEST_F(StatisticsAutoRefreshTest, ZeroIntervalDisablesThread)`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsAutoRefreshTest): n/a
  - `<unnamed>` (ZeroIntervalDisablesThread): n/a

#### `std::string makeTempDbPath(const std::string &prefix)`
- Source: `tests/metadata/test_statistics_auto_refresh.cpp`:18
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a

### test_statistics_collector.cpp

#### `TEST_F(StatisticsCollectorTest, ClearIndexStats)`
- Source: `tests/metadata/test_statistics_collector.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (ClearIndexStats): n/a

#### `TEST_F(StatisticsCollectorTest, ClearIndexStatsEmptyName)`
- Source: `tests/metadata/test_statistics_collector.cpp`:352
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (ClearIndexStatsEmptyName): n/a

#### `TEST_F(StatisticsCollectorTest, ClearStats)`
- Source: `tests/metadata/test_statistics_collector.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (ClearStats): n/a

#### `TEST_F(StatisticsCollectorTest, ClearStatsEmptyName)`
- Source: `tests/metadata/test_statistics_collector.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (ClearStatsEmptyName): n/a

#### `TEST_F(StatisticsCollectorTest, CollectStatsColumnStats)`
- Source: `tests/metadata/test_statistics_collector.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (CollectStatsColumnStats): n/a

#### `TEST_F(StatisticsCollectorTest, CollectStatsEmptyTable)`
- Source: `tests/metadata/test_statistics_collector.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (CollectStatsEmptyTable): n/a

#### `TEST_F(StatisticsCollectorTest, CollectStatsEmptyTableName)`
- Source: `tests/metadata/test_statistics_collector.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (CollectStatsEmptyTableName): n/a

#### `TEST_F(StatisticsCollectorTest, CollectStatsRowCount)`
- Source: `tests/metadata/test_statistics_collector.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (CollectStatsRowCount): n/a

#### `TEST_F(StatisticsCollectorTest, EquiHeightHistogramEqualFrequencies)`
- Source: `tests/metadata/test_statistics_collector.cpp`:479
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (EquiHeightHistogramEqualFrequencies): n/a

#### `TEST_F(StatisticsCollectorTest, EquiHeightHistogramSkewedData)`
- Source: `tests/metadata/test_statistics_collector.cpp`:513
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (EquiHeightHistogramSkewedData): n/a

#### `TEST_F(StatisticsCollectorTest, GetIndexStatsAfterImport)`
- Source: `tests/metadata/test_statistics_collector.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (GetIndexStatsAfterImport): n/a

#### `TEST_F(StatisticsCollectorTest, GetIndexStatsEmptyName)`
- Source: `tests/metadata/test_statistics_collector.cpp`:318
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (GetIndexStatsEmptyName): n/a

#### `TEST_F(StatisticsCollectorTest, GetIndexStatsMissingTable)`
- Source: `tests/metadata/test_statistics_collector.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (GetIndexStatsMissingTable): n/a

#### `TEST_F(StatisticsCollectorTest, GetStatsCacheHit)`
- Source: `tests/metadata/test_statistics_collector.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (GetStatsCacheHit): n/a

#### `TEST_F(StatisticsCollectorTest, GetStatsEmptyName)`
- Source: `tests/metadata/test_statistics_collector.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (GetStatsEmptyName): n/a

#### `TEST_F(StatisticsCollectorTest, GetStatsNotFound)`
- Source: `tests/metadata/test_statistics_collector.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (GetStatsNotFound): n/a

#### `TEST_F(StatisticsCollectorTest, HistogramBucketToJSON)`
- Source: `tests/metadata/test_statistics_collector.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (HistogramBucketToJSON): n/a

#### `TEST_F(StatisticsCollectorTest, HistogramBuiltForNumericColumn)`
- Source: `tests/metadata/test_statistics_collector.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (HistogramBuiltForNumericColumn): n/a

#### `TEST_F(StatisticsCollectorTest, HistogramPersistedAndRestoredAcrossInstances)`
- Source: `tests/metadata/test_statistics_collector.cpp`:440
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (HistogramPersistedAndRestoredAcrossInstances): n/a

#### `TEST_F(StatisticsCollectorTest, ImportIndexStatsBasic)`
- Source: `tests/metadata/test_statistics_collector.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (ImportIndexStatsBasic): n/a

#### `TEST_F(StatisticsCollectorTest, ImportIndexStatsEmptyName)`
- Source: `tests/metadata/test_statistics_collector.cpp`:325
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (ImportIndexStatsEmptyName): n/a

#### `TEST_F(StatisticsCollectorTest, ImportIndexStatsEmptyVector)`
- Source: `tests/metadata/test_statistics_collector.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (ImportIndexStatsEmptyVector): n/a

#### `TEST_F(StatisticsCollectorTest, IndexStatsPersistenceAcrossInstances)`
- Source: `tests/metadata/test_statistics_collector.cpp`:359
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (IndexStatsPersistenceAcrossInstances): n/a

#### `TEST_F(StatisticsCollectorTest, IndexStatsToJSON)`
- Source: `tests/metadata/test_statistics_collector.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (IndexStatsToJSON): n/a

#### `TEST_F(StatisticsCollectorTest, MetricsHook_OnCacheHit_CalledOnSecondGetStats)`
- Source: `tests/metadata/test_statistics_collector.cpp`:654
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (MetricsHook_OnCacheHit_CalledOnSecondGetStats): n/a

#### `TEST_F(StatisticsCollectorTest, MetricsHook_OnCacheMiss_CalledWhenCacheEmpty)`
- Source: `tests/metadata/test_statistics_collector.cpp`:677
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (MetricsHook_OnCacheMiss_CalledWhenCacheEmpty): n/a

#### `TEST_F(StatisticsCollectorTest, MetricsHook_OnCollect_CalledAfterCollectStats)`
- Source: `tests/metadata/test_statistics_collector.cpp`:625
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (MetricsHook_OnCollect_CalledAfterCollectStats): n/a

#### `TEST_F(StatisticsCollectorTest, MetricsHook_OnError_CalledForEmptyTableName)`
- Source: `tests/metadata/test_statistics_collector.cpp`:641
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (MetricsHook_OnError_CalledForEmptyTableName): n/a

#### `TEST_F(StatisticsCollectorTest, RangeSelectivityEmpty)`
- Source: `tests/metadata/test_statistics_collector.cpp`:569
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (RangeSelectivityEmpty): n/a

#### `TEST_F(StatisticsCollectorTest, RangeSelectivityFullRange)`
- Source: `tests/metadata/test_statistics_collector.cpp`:555
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (RangeSelectivityFullRange): n/a

#### `TEST_F(StatisticsCollectorTest, RangeSelectivityPartial)`
- Source: `tests/metadata/test_statistics_collector.cpp`:583
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (RangeSelectivityPartial): n/a

#### `TEST_F(StatisticsCollectorTest, TableStatsToJSON)`
- Source: `tests/metadata/test_statistics_collector.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (TableStatsToJSON): n/a

#### `TEST_F(StatisticsCollectorTest, ToJSON)`
- Source: `tests/metadata/test_statistics_collector.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (ToJSON): n/a

#### `TEST_F(StatisticsCollectorTest, ToJSONIncludesIndexStats)`
- Source: `tests/metadata/test_statistics_collector.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (ToJSONIncludesIndexStats): n/a

#### `TEST_F(StatisticsCollectorTest, UpdateStats)`
- Source: `tests/metadata/test_statistics_collector.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollectorTest): n/a
  - `<unnamed>` (UpdateStats): n/a

#### `std::string makeTempDbPath(const std::string &prefix)`
- Source: `tests/metadata/test_statistics_collector.cpp`:15
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a

### themis

#### `bool isNull(const ColumnValue &v)`
- Source: `src/metadata/schema_constraints.cpp`:279
- Brief: Is Null.
- Parameters:
  - `v` (const ColumnValue &): Input parameter.
- Return: True when the operation succeeds.
- Details: v Input parameter. True when the operation succeeds. Implements isNull without additional internal calls.

#### `ColumnConstraint::Kind kindFromString(const std::string &s)`
- Source: `src/metadata/schema_constraints.cpp`:140
- Brief: Kind From String.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: s Input parameter. Return value. std::runtime_error if an error occurs. Implements kindFromString without additional internal calls.

#### `std::string kindToString(ColumnConstraint::Kind kind)`
- Source: `src/metadata/schema_constraints.cpp`:122
- Brief: Kind To String.
- Parameters:
  - `kind` (ColumnConstraint::Kind): Input parameter.
- Return: Return value.
- Details: kind Input parameter. Return value. Implements kindToString without additional internal calls.

#### `std::string mapDataType(const std::string &prop_type)`
- Source: `src/metadata/information_schema.cpp`:152
- Brief: Map SchemaManager property type to SQL data type string.
- Parameters:
  - `prop_type` (const std::string &): Input parameter.
- Return: Return value.
- Details: prop_type Input parameter. Return value. Implements mapDataType without additional internal calls.

#### `std::string mapIndexType(const std::string &idx_type)`
- Source: `src/metadata/information_schema.cpp`:180
- Brief: Map index type to SQL INDEX_TYPE string.
- Parameters:
  - `idx_type` (const std::string &): Input parameter.
- Return: Return value.
- Details: idx_type Input parameter. Return value. Implements mapIndexType without additional internal calls.

#### `std::string tableTypeSQL(const std::string &type)`
- Source: `src/metadata/information_schema.cpp`:139
- Brief: Translate SchemaManager table type to SQL TABLE_TYPE string.
- Parameters:
  - `type` (const std::string &): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value. Implements tableTypeSQL without additional internal calls.

#### `std::string toSqlType(const std::string &themis_type)`
- Source: `src/metadata/schema_version_manager.cpp`:533
- Brief: To Sql Type.
- Parameters:
  - `themis_type` (const std::string &): Input parameter.
- Return: Return value.
- Details: themis_type Input parameter. Return value. Implements toSqlType without additional internal calls.

### themis::CatalogExporter

#### `CatalogExporter(CatalogExporter &&) noexcept=default`
- Source: `include/metadata/catalog_exporter.h`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporter &&): n/a

#### `CatalogExporter(Config config)`
- Source: `include/metadata/catalog_exporter.h`:115
- Brief: n/a
- Parameters:
  - `config` (Config): Catalog connection parameters
- Details: config Catalog connection parameters

#### `CatalogExporter(const CatalogExporter &)=delete`
- Source: `include/metadata/catalog_exporter.h`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CatalogExporter &): n/a

#### `json buildAtlasPayload(const std::vector< SchemaManager::TableSchema > &tables) const`
- Source: `include/metadata/catalog_exporter.h`:156
- Brief: Build the Atlas bulk-entity JSON payload for a list of tables.
- Parameters:
  - `tables` (const std::vector< SchemaManager::TableSchema > &): n/a

#### `json buildDataHubProposals(const std::vector< SchemaManager::TableSchema > &tables) const`
- Source: `include/metadata/catalog_exporter.h`:166
- Brief: Build an array of MetadataChangeProposal JSON objects (one per table).
- Parameters:
  - `tables` (const std::vector< SchemaManager::TableSchema > &): n/a

#### `int httpPost(const std::string &url, const std::string &body, const std::string &auth_header, std::string &response_body)`
- Source: `include/metadata/catalog_exporter.h`:178
- Brief: Http Post.
- Parameters:
  - `url` (const std::string &): Input parameter.
  - `body` (const std::string &): Input parameter.
  - `auth_header` (const std::string &): Input parameter.
  - `response_body` (std::string &): Input/output parameter.
- Return: HTTP status code (0 on transport error)
- Details: Execute an HTTP POST request. Delegates to the injected test function when set; otherwise uses libcurl. HTTP status code (0 on transport error) url Input parameter. body Input parameter. auth_header Input parameter. response_body Input/output parameter. Return value. Calls: http_post_fn_(), curlHttpPost().

#### `CatalogExporter & operator=(CatalogExporter &&) noexcept=default`
- Source: `include/metadata/catalog_exporter.h`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (CatalogExporter &&): n/a

#### `CatalogExporter & operator=(const CatalogExporter &)=delete`
- Source: `include/metadata/catalog_exporter.h`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CatalogExporter &): n/a

#### `PublishResult publishSchema(const std::vector< SchemaManager::TableSchema > &tables)`
- Source: `include/metadata/catalog_exporter.h`:139
- Brief: n/a
- Parameters:
  - `tables` (const std::vector< SchemaManager::TableSchema > &): Tables to export (typically from SchemaManager::getAllTables())
- Return: PublishResult with success flag and entity count
- Details: Publish all tables to the configured catalog. For Apache Atlas: creates/updates rdbms_db + rdbms_table + rdbms_column entities in a single bulk call. For DataHub: emits one MetadataChangeProposal per table with datasetProperties and schemaMetadata aspects. tables Tables to export (typically from SchemaManager::getAllTables()) PublishResult with success flag and entity count

#### `PublishResult publishTable(const SchemaManager::TableSchema &table)`
- Source: `include/metadata/catalog_exporter.h`:144
- Brief: n/a
- Parameters:
  - `table` (const SchemaManager::TableSchema &): Table schema to export
- Return: PublishResult with success flag and entity count
- Details: Publish a single table to the configured catalog. table Table schema to export PublishResult with success flag and entity count

#### `PublishResult sendToAtlas(const json &payload)`
- Source: `include/metadata/catalog_exporter.h`:159
- Brief: Publish entity payload to Atlas; returns the publish result.
- Parameters:
  - `payload` (const json &): Input parameter.
- Return: Return value.
- Details: Send To Atlas. payload Input parameter. Return value. Calls: empty(), size(), dump(), spdlog::info(), httpPost(), json::parse(), contains(), items().

#### `PublishResult sendToDataHub(const json &proposals)`
- Source: `include/metadata/catalog_exporter.h`:169
- Brief: Post each proposal to the DataHub GMS ingest endpoint.
- Parameters:
  - `proposals` (const json &): n/a

#### `void setHttpPostForTesting(HttpPostFn fn)`
- Source: `include/metadata/catalog_exporter.h`:148
- Brief: Set Http Post For Testing.
- Parameters:
  - `fn` (HttpPostFn): Input parameter.
- Details: Replace the real libcurl implementation with a test double. Pass an empty HttpPostFn{} to restore the real implementation. fn Input parameter. Calls: std::move().

#### `~CatalogExporter()=default`
- Source: `include/metadata/catalog_exporter.h`:117
- Brief: n/a
- Parameters: none

### themis::ColumnConstraint

#### `ColumnConstraint makeCheck(std::string constraint_name, std::string expr)`
- Source: `include/metadata/schema_constraints.h`:67
- Brief: Make Check.
- Parameters:
  - `constraint_name` (std::string): Name of the constraint.
  - `expr` (std::string): Input parameter.
- Return: Return value.
- Details: constraint_name Name of the constraint. expr Input parameter. Return value. Calls: std::move().

#### `ColumnConstraint makeDefault(std::string constraint_name, ColumnValue value)`
- Source: `include/metadata/schema_constraints.h`:68
- Brief: Make Default.
- Parameters:
  - `constraint_name` (std::string): Name of the constraint.
  - `value` (ColumnValue): Input parameter.
- Return: Return value.
- Details: constraint_name Name of the constraint. value Input parameter. Return value. Calls: std::move().

#### `ColumnConstraint makeForeignKey(std::string constraint_name, std::string ref_table, std::string ref_column)`
- Source: `include/metadata/schema_constraints.h`:69
- Brief: Make Foreign Key.
- Parameters:
  - `constraint_name` (std::string): Name of the constraint.
  - `ref_table` (std::string): Input parameter.
  - `ref_column` (std::string): Input parameter.
- Return: Return value.
- Details: constraint_name Name of the constraint. ref_table Input parameter. ref_column Input parameter. Return value.

#### `ColumnConstraint makeNotNull(std::string constraint_name)`
- Source: `include/metadata/schema_constraints.h`:65
- Brief: Make Not Null.
- Parameters:
  - `constraint_name` (std::string): Name of the constraint.
- Return: Return value.
- Details: constraint_name Name of the constraint. Return value. Calls: std::move().

#### `ColumnConstraint makeUnique(std::string constraint_name)`
- Source: `include/metadata/schema_constraints.h`:66
- Brief: Make Unique.
- Parameters:
  - `constraint_name` (std::string): Name of the constraint.
- Return: Return value.
- Details: constraint_name Name of the constraint. Return value. Calls: std::move().

#### `json toJSON() const`
- Source: `include/metadata/schema_constraints.h`:64
- Brief: n/a
- Parameters: none

### themis::ColumnStats

#### `double estimateRangeSelectivity(double low, double high) const`
- Source: `include/metadata/statistics_collector.h`:73
- Brief: n/a
- Parameters:
  - `low` (double): n/a
  - `high` (double): n/a
- Details: Estimate selectivity for a range predicate [low, high] using the histogram. Returns fraction of rows in [low, high]; falls back to uniform distribution when no histogram is available.

#### `json toJSON() const`
- Source: `include/metadata/statistics_collector.h`:68
- Brief: n/a
- Parameters: none

### themis::ConsistencyIssue

#### `json toJSON() const`
- Source: `include/metadata/schema_consistency_checker.h`:45
- Brief: n/a
- Parameters: none

### themis::ConstraintViolation

#### `json toJSON() const`
- Source: `include/metadata/schema_constraints.h`:40
- Brief: n/a
- Parameters: none

### themis::DistributedMetadataCatalog

#### `DistributedMetadataCatalog(DistributedMetadataCatalog &&)=delete`
- Source: `include/metadata/distributed_catalog.h`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalog &&): n/a

#### `DistributedMetadataCatalog(const DistributedMetadataCatalog &)=delete`
- Source: `include/metadata/distributed_catalog.h`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedMetadataCatalog &): n/a

#### `DistributedMetadataCatalog(themisdb::sharding::MetadataShardRouter &router)`
- Source: `include/metadata/distributed_catalog.h`:54
- Brief: n/a
- Parameters:
  - `router` (themisdb::sharding::MetadataShardRouter &): Distributed shard router (non-owning reference)
- Details: Constructor router Distributed shard router (non-owning reference)

#### `std::optional< SchemaManager::TableSchema > fetchSchema(const std::string &table_name) const`
- Source: `include/metadata/distributed_catalog.h`:92
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): Table name to look up
- Return: TableSchema if found, std::nullopt otherwise
- Details: Fetch a table schema from the distributed catalog. table_name Table name to look up TableSchema if found, std::nullopt otherwise

#### `nlohmann::json getStatistics() const`
- Source: `include/metadata/distributed_catalog.h`:106
- Brief: n/a
- Parameters: none
- Details: Return catalog statistics as a JSON object. Includes counts of published, sync, fetch, and remove operations.

#### `std::vector< std::string > listTableNames() const`
- Source: `include/metadata/distributed_catalog.h`:98
- Brief: n/a
- Parameters: none
- Return: Sorted vector of table names
- Details: List all table names present in the distributed catalog. Performs a scatter-gather across all shards via the router. Sorted vector of table names

#### `DistributedMetadataCatalog & operator=(DistributedMetadataCatalog &&)=delete`
- Source: `include/metadata/distributed_catalog.h`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMetadataCatalog &&): n/a

#### `DistributedMetadataCatalog & operator=(const DistributedMetadataCatalog &)=delete`
- Source: `include/metadata/distributed_catalog.h`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedMetadataCatalog &): n/a

#### `bool publishSchema(const SchemaManager::TableSchema &schema)`
- Source: `include/metadata/distributed_catalog.h`:71
- Brief: Publish Schema.
- Parameters:
  - `schema` (const SchemaManager::TableSchema &): Input parameter.
- Return: true on success, false if the router rejected the write
- Details: Publish (create or update) a single table schema to the distributed catalog. schema Table schema to publish true on success, false if the router rejected the write schema Input parameter. True when the operation succeeds.

#### `bool removeSchema(const std::string &table_name)`
- Source: `include/metadata/distributed_catalog.h`:83
- Brief: Remove Schema.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
- Return: true if removed, false if not found or router rejected the removal
- Details: Remove a table schema from the distributed catalog. table_name Table name to remove true if removed, false if not found or router rejected the removal table_name Name of the table. True when the operation succeeds.

#### `size_t syncFromSchemaManager(SchemaManager &schema_mgr)`
- Source: `include/metadata/distributed_catalog.h`:78
- Brief: Sync From Schema Manager.
- Parameters:
  - `schema_mgr` (SchemaManager &): Input/output parameter.
- Return: Number of schemas successfully published
- Details: Sync all table schemas from a local SchemaManager to the distributed catalog. Iterates all tables returned by schema_mgr.getAllTables() and calls publishSchema() for each one. schema_mgr Source of truth for local schemas Number of schemas successfully published schema_mgr Input/output parameter. Return value.

#### `~DistributedMetadataCatalog()=default`
- Source: `include/metadata/distributed_catalog.h`:56
- Brief: n/a
- Parameters: none

### themis::ERDiagramExporter

#### `ERDiagramExporter()=default`
- Source: `include/metadata/er_diagram_exporter.h`:59
- Brief: n/a
- Parameters: none

#### `ERDiagramExporter(ERDiagramExporter &&) noexcept=default`
- Source: `include/metadata/er_diagram_exporter.h`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (ERDiagramExporter &&): n/a

#### `ERDiagramExporter(const ERDiagramExporter &)=default`
- Source: `include/metadata/er_diagram_exporter.h`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ERDiagramExporter &): n/a

#### `std::string escapeDOT(const std::string &s)`
- Source: `include/metadata/er_diagram_exporter.h`:147
- Brief: Escape a string for safe inclusion inside a DOT record label.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: Escape DOT. s Input parameter. Return value. Calls: reserve(), size().

#### `std::string escapeMermaid(const std::string &s)`
- Source: `include/metadata/er_diagram_exporter.h`:144
- Brief: Escape a string for safe inclusion inside a Mermaid entity/attribute name.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: Escape Mermaid. s Input parameter. Return value. Calls: reserve(), size(), std::isalnum().

#### `std::string exportDOT(const std::vector< SchemaManager::TableSchema > &tables, const std::vector< SchemaManager::RelationshipSchema > &relationships) const`
- Source: `include/metadata/er_diagram_exporter.h`:109
- Brief: n/a
- Parameters:
  - `tables` (const std::vector< SchemaManager::TableSchema > &): All table/collection schemas (nodes)
  - `relationships` (const std::vector< SchemaManager::RelationshipSchema > &): All edge/relationship schemas (edges)
- Return: DOT language string (UTF-8)
- Details: Export as Graphviz DOT language. Produces a directed graph (digraph schema { ... }) with record-shaped nodes listing each entity's properties and labelled directed edges for each relationship. tables All table/collection schemas (nodes) relationships All edge/relationship schemas (edges) DOT language string (UTF-8)

#### `nlohmann::json exportJSON(const std::vector< SchemaManager::TableSchema > &tables, const std::vector< SchemaManager::RelationshipSchema > &relationships) const`
- Source: `include/metadata/er_diagram_exporter.h`:133
- Brief: n/a
- Parameters:
  - `tables` (const std::vector< SchemaManager::TableSchema > &): All table/collection schemas (nodes)
  - `relationships` (const std::vector< SchemaManager::RelationshipSchema > &): All edge/relationship schemas (edges)
- Return: JSON object with "nodes" and "edges" arrays
- Details: Export as a JSON graph (nodes + edges). Schema: { "nodes":[ {"id":"users","type":"relational", "properties":[{"name":"id","type":"integer"},...]} ], "edges":[ {"from":"users","to":"orders","label":"placed", "properties":[]} ] } tables All table/collection schemas (nodes) relationships All edge/relationship schemas (edges) JSON object with "nodes" and "edges" arrays

#### `std::string exportMermaid(const std::vector< SchemaManager::TableSchema > &tables, const std::vector< SchemaManager::RelationshipSchema > &relationships) const`
- Source: `include/metadata/er_diagram_exporter.h`:95
- Brief: n/a
- Parameters:
  - `tables` (const std::vector< SchemaManager::TableSchema > &): All table/collection schemas (nodes)
  - `relationships` (const std::vector< SchemaManager::RelationshipSchema > &): All edge/relationship schemas (edges)
- Return: Mermaid erDiagram string (UTF-8)
- Details: Export as Mermaid erDiagram syntax. The output is a valid Mermaid erDiagram block that can be embedded in Markdown fences: Relationship cardinality notation: graph_edge relationships use \|\|--o{ (one-to-many) when from_table == to_table the edge is rendered as }o--o{ (many-to-many) tables All table/collection schemas (nodes) relationships All edge/relationship schemas (edges) Mermaid erDiagram string (UTF-8)

#### `ERDiagramExporter & operator=(ERDiagramExporter &&) noexcept=default`
- Source: `include/metadata/er_diagram_exporter.h`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (ERDiagramExporter &&): n/a

#### `ERDiagramExporter & operator=(const ERDiagramExporter &)=default`
- Source: `include/metadata/er_diagram_exporter.h`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ERDiagramExporter &): n/a

#### `~ERDiagramExporter()=default`
- Source: `include/metadata/er_diagram_exporter.h`:60
- Brief: n/a
- Parameters: none

### themis::HistogramBucket

#### `json toJSON() const`
- Source: `include/metadata/statistics_collector.h`:46
- Brief: n/a
- Parameters: none

### themis::ISColumn

#### `json toJSON() const`
- Source: `include/metadata/information_schema.h`:68
- Brief: Serialise this COLUMNS row to JSON.
- Parameters: none
- Return: JSON object representing the row fields.
- Details: JSON object representing the row fields.

### themis::ISKeyColumnUsage

#### `json toJSON() const`
- Source: `include/metadata/information_schema.h`:110
- Brief: Serialise this KEY_COLUMN_USAGE row to JSON.
- Parameters: none
- Return: JSON object representing the row fields.
- Details: JSON object representing the row fields.

### themis::ISReferentialConstraint

#### `json toJSON() const`
- Source: `include/metadata/information_schema.h`:131
- Brief: Serialise this REFERENTIAL_CONSTRAINTS row to JSON.
- Parameters: none
- Return: JSON object representing the row fields.
- Details: JSON object representing the row fields.

### themis::ISStatistic

#### `json toJSON() const`
- Source: `include/metadata/information_schema.h`:88
- Brief: Serialise this STATISTICS row to JSON.
- Parameters: none
- Return: JSON object representing the row fields.
- Details: JSON object representing the row fields.

### themis::ISTable

#### `json toJSON() const`
- Source: `include/metadata/information_schema.h`:47
- Brief: Serialise this TABLES row to JSON.
- Parameters: none
- Return: JSON object representing the row fields.
- Details: JSON object representing the row fields.

### themis::IndexStats

#### `json toJSON() const`
- Source: `include/metadata/statistics_collector.h`:123
- Brief: n/a
- Parameters: none

### themis::InformationSchema

#### `InformationSchema(InformationSchema &&) noexcept=delete`
- Source: `include/metadata/information_schema.h`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (InformationSchema &&): n/a

#### `InformationSchema(SchemaManager &schema_mgr)`
- Source: `include/metadata/information_schema.h`:152
- Brief: Construct an INFORMATION_SCHEMA view provider over a live SchemaManager.
- Parameters:
  - `schema_mgr` (SchemaManager &): SchemaManager that owns the authoritative schema metadata.
- Details: schema_mgr SchemaManager that owns the authoritative schema metadata.

#### `InformationSchema(const InformationSchema &)=delete`
- Source: `include/metadata/information_schema.h`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (const InformationSchema &): n/a

#### `json columnsToJSON(std::string_view table_name) const`
- Source: `include/metadata/information_schema.h`:230
- Brief: Serialise only the COLUMNS view for one table to JSON.
- Parameters:
  - `table_name` (std::string_view): Table whose column metadata should be exported.
- Return: JSON array of COLUMNS rows for the table.
- Details: table_name Table whose column metadata should be exported. JSON array of COLUMNS rows for the table.

#### `std::vector< ISColumn > getColumns(std::optional< std::string_view > table_name=std::nullopt) const`
- Source: `include/metadata/information_schema.h`:177
- Brief: Return the INFORMATION_SCHEMA.COLUMNS view.
- Parameters:
  - `table_name` (std::optional< std::string_view >): Optional table filter; when omitted, all tables are included.
- Return: Column metadata rows for the selected scope.
- Details: table_name Optional table filter; when omitted, all tables are included. Column metadata rows for the selected scope.

#### `std::vector< ISKeyColumnUsage > getKeyColumnUsage(std::optional< std::string_view > table_name=std::nullopt) const`
- Source: `include/metadata/information_schema.h`:195
- Brief: Return the INFORMATION_SCHEMA.KEY_COLUMN_USAGE view.
- Parameters:
  - `table_name` (std::optional< std::string_view >): Optional table filter; when omitted, all tables are included.
- Return: Key-usage rows for the selected scope.
- Details: table_name Optional table filter; when omitted, all tables are included. Key-usage rows for the selected scope.

#### `std::vector< ISReferentialConstraint > getReferentialConstraints(std::optional< std::string_view > table_name=std::nullopt) const`
- Source: `include/metadata/information_schema.h`:204
- Brief: Return the INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS view.
- Parameters:
  - `table_name` (std::optional< std::string_view >): Optional referencing-table filter.
- Return: Referential-constraint rows for the selected scope.
- Details: table_name Optional referencing-table filter. Referential-constraint rows for the selected scope.

#### `std::vector< ISStatistic > getStatistics(std::optional< std::string_view > table_name=std::nullopt) const`
- Source: `include/metadata/information_schema.h`:186
- Brief: Return the INFORMATION_SCHEMA.STATISTICS view.
- Parameters:
  - `table_name` (std::optional< std::string_view >): Optional table filter; when omitted, all tables are included.
- Return: Index metadata rows for the selected scope.
- Details: table_name Optional table filter; when omitted, all tables are included. Index metadata rows for the selected scope.

#### `std::vector< ISTable > getTables() const`
- Source: `include/metadata/information_schema.h`:170
- Brief: Return the INFORMATION_SCHEMA.TABLES view.
- Parameters: none
- Return: One row per table or collection in the default schema.
- Details: One row per table or collection in the default schema.

#### `InformationSchema & operator=(InformationSchema &&) noexcept=delete`
- Source: `include/metadata/information_schema.h`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (InformationSchema &&): n/a

#### `InformationSchema & operator=(const InformationSchema &)=delete`
- Source: `include/metadata/information_schema.h`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (const InformationSchema &): n/a

#### `json referentialConstraintsToJSON() const`
- Source: `include/metadata/information_schema.h`:236
- Brief: Serialise only the REFERENTIAL_CONSTRAINTS view to JSON.
- Parameters: none
- Return: JSON array of referential-constraint rows.
- Details: JSON array of referential-constraint rows.

#### `json tablesToJSON() const`
- Source: `include/metadata/information_schema.h`:223
- Brief: Serialise only the TABLES view to JSON.
- Parameters: none
- Return: JSON array of TABLES rows.
- Details: JSON array of TABLES rows.

#### `json toJSON() const`
- Source: `include/metadata/information_schema.h`:217
- Brief: Serialise the full INFORMATION_SCHEMA surface to JSON.
- Parameters: none
- Return: JSON object with tables, columns, statistics, key_column_usage, and referential_constraints arrays.
- Details: JSON object with tables, columns, statistics, key_column_usage, and referential_constraints arrays.

#### `~InformationSchema()=default`
- Source: `include/metadata/information_schema.h`:154
- Brief: n/a
- Parameters: none

### themis::SchemaAuditEntry

#### `SchemaAuditEntry fromJSON(const json &j)`
- Source: `include/metadata/schema_audit_log.h`:61
- Brief: From JSON.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: value(), std::chrono::system_clock::now(), contains(), is_object(), json::object().

#### `json toJSON() const`
- Source: `include/metadata/schema_audit_log.h`:60
- Brief: n/a
- Parameters: none

### themis::SchemaAuditLog

#### `SchemaAuditLog(RocksDBWrapper &db)`
- Source: `include/metadata/schema_audit_log.h`:91
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): RocksDB instance (non-owning reference)
- Details: Construct with a storage reference. db RocksDB instance (non-owning reference)

#### `SchemaAuditLog(const SchemaAuditLog &)=delete`
- Source: `include/metadata/schema_audit_log.h`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaAuditLog &): n/a

#### `std::string buildKey(std::string_view table_name, uint64_t timestamp_ns)`
- Source: `include/metadata/schema_audit_log.h`:149
- Brief: Build the full RocksDB key for an entry.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `timestamp_ns` (uint64_t): Input parameter.
- Return: Return value.
- Details: Build Key. table_name Name of the table. timestamp_ns Input parameter. Return value. Calls: std::setw(), std::setfill(), str().

#### `json fullHistoryToJSON() const`
- Source: `include/metadata/schema_audit_log.h`:143
- Brief: Serialize the full audit history as a JSON array.
- Parameters: none

#### `std::vector< SchemaAuditEntry > getFullHistory() const`
- Source: `include/metadata/schema_audit_log.h`:127
- Brief: Return audit entries across ALL tables, in ascending timestamp order.
- Parameters: none

#### `std::vector< SchemaAuditEntry > getHistory(std::string_view table_name) const`
- Source: `include/metadata/schema_audit_log.h`:124
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): n/a
- Details: Return all audit entries for a given table, in ascending timestamp order. An empty vector is returned if no entries exist or on storage error.

#### `std::vector< SchemaAuditEntry > getRecentHistory(std::string_view table_name, size_t limit=50) const`
- Source: `include/metadata/schema_audit_log.h`:130
- Brief: Return the most recent N entries for a table (newest-first).
- Parameters:
  - `table_name` (std::string_view): n/a
  - `limit` (size_t): n/a

#### `json historyToJSON(std::string_view table_name) const`
- Source: `include/metadata/schema_audit_log.h`:140
- Brief: Serialize the audit history for a table as a JSON array.
- Parameters:
  - `table_name` (std::string_view): n/a

#### `SchemaAuditLog & operator=(const SchemaAuditLog &)=delete`
- Source: `include/metadata/schema_audit_log.h`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaAuditLog &): n/a

#### `bool record(std::string_view table_name, std::string_view operation, std::string_view author="", std::string_view description="", uint64_t version=0, const json &extra_meta=json::object())`
- Source: `include/metadata/schema_audit_log.h`:109
- Brief: Record.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `operation` (std::string_view): Input parameter.
  - `author` (std::string_view): Input parameter.
  - `description` (std::string_view): Input parameter.
  - `version` (uint64_t): Input parameter.
  - `extra_meta` (const json &): Input parameter.
- Return: true on success; logs a warning and returns false on storage error
- Details: Record a schema change event. table_name Name of the affected table operation Operation type: "create", "update", "delete", "rollback", "import" author Who performed the change (may be empty for system operations) description Human-readable description of the change version Associated schema version number (0 if N/A) extra_meta Optional additional JSON metadata true on success; logs a warning and returns false on storage error table_name Name of the table. operation Input parameter. author Input parameter. description Input parameter. version Input parameter. extra_meta Input parameter. True when the operation succeeds.

#### `std::string tablePrefix(std::string_view table_name)`
- Source: `include/metadata/schema_audit_log.h`:152
- Brief: Prefix used for scanning all entries of a specific table.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Table Prefix. table_name Name of the table. Return value. Calls: std::string().

#### `~SchemaAuditLog()=default`
- Source: `include/metadata/schema_audit_log.h`:92
- Brief: n/a
- Parameters: none

### themis::SchemaChange

#### `SchemaChange fromJSON(const json &j)`
- Source: `include/metadata/schema_version_manager.h`:55
- Brief: Parse a schema-change record from JSON.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Parsed SchemaChange value.
- Details: From JSON. j JSON object created by toJSON(). Parsed SchemaChange value. j Input parameter. Return value. Calls: value(), std::chrono::system_clock::now(), contains(), is_object(), SchemaManager::parseTableSchema().

#### `json toJSON() const`
- Source: `include/metadata/schema_version_manager.h`:49
- Brief: Serialise this schema-change record to JSON.
- Parameters: none
- Return: JSON object containing version, author, snapshot, and timestamps.
- Details: JSON object containing version, author, snapshot, and timestamps.

### themis::SchemaConsistencyChecker

#### `SchemaConsistencyChecker(RocksDBWrapper &db, SchemaManager &schema_mgr, StatisticsCollector *stats=nullptr, SchemaConstraints *constraints=nullptr)`
- Source: `include/metadata/schema_consistency_checker.h`:78
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): RocksDB wrapper for key prefix scanning
  - `schema_mgr` (SchemaManager &): SchemaManager whose table list defines the truth
  - `stats` (StatisticsCollector *): Optional statistics collector (for stale-stats checks)
  - `constraints` (SchemaConstraints *): Optional constraints engine (for missing-constraint checks)
- Details: Constructor db RocksDB wrapper for key prefix scanning schema_mgr SchemaManager whose table list defines the truth stats Optional statistics collector (for stale-stats checks) constraints Optional constraints engine (for missing-constraint checks)

#### `SchemaConsistencyChecker(const SchemaConsistencyChecker &)=delete`
- Source: `include/metadata/schema_consistency_checker.h`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaConsistencyChecker &): n/a

#### `void bgLoop_()`
- Source: `include/metadata/schema_consistency_checker.h`:136
- Brief: Background check loop.
- Parameters: none
- Details: Bg Loop. Calls: load(), lk(), wait_for(), runCheck(), spdlog::error(), what(), spdlog::debug().

#### `std::vector< ConsistencyIssue > checkMissingConstraints_() const`
- Source: `include/metadata/schema_consistency_checker.h`:133
- Brief: Check for tables in the schema that have no constraint definitions.
- Parameters: none

#### `std::vector< ConsistencyIssue > checkOrphanKeys_() const`
- Source: `include/metadata/schema_consistency_checker.h`:127
- Brief: Check for orphaned RocksDB keys (keys whose prefix is not a known table).
- Parameters: none

#### `std::vector< ConsistencyIssue > checkStaleStats_() const`
- Source: `include/metadata/schema_consistency_checker.h`:130
- Brief: Check for tables with stale or missing statistics.
- Parameters: none

#### `std::vector< ConsistencyIssue > getLastCheckResults() const`
- Source: `include/metadata/schema_consistency_checker.h`:112
- Brief: n/a
- Parameters: none
- Details: Return the results of the most recent background (or manual) check. Returns an empty vector if no check has run yet.

#### `json lastResultsToJSON() const`
- Source: `include/metadata/schema_consistency_checker.h`:115
- Brief: Serialise the last check results as a JSON array.
- Parameters: none

#### `SchemaConsistencyChecker & operator=(const SchemaConsistencyChecker &)=delete`
- Source: `include/metadata/schema_consistency_checker.h`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaConsistencyChecker &): n/a

#### `std::vector< ConsistencyIssue > runCheck() const`
- Source: `include/metadata/schema_consistency_checker.h`:99
- Brief: n/a
- Parameters: none
- Details: Run a single synchronous consistency check and return all issues found. This method is thread-safe and can be called while the background thread is also running.

#### `void setMaxStatsAge(std::chrono::hours max_age) noexcept`
- Source: `include/metadata/schema_consistency_checker.h`:119
- Brief: n/a
- Parameters:
  - `max_age` (std::chrono::hours): n/a
- Details: Configure the maximum acceptable statistics age. Issues are raised for any table whose stats are older than this.

#### `void startBackgroundCheck(std::chrono::seconds interval)`
- Source: `include/metadata/schema_consistency_checker.h`:104
- Brief: Start Background Check.
- Parameters:
  - `interval` (std::chrono::seconds): Input parameter.
- Details: Start a background thread that calls runCheck() every interval. Calling this a second time replaces the interval. Pass std::chrono::seconds(0) to stop the background thread. interval Input parameter. Calls: stopBackgroundCheck(), count(), spdlog::debug(), store(), std::thread(), bgLoop_(), spdlog::info().

#### `void stopBackgroundCheck() noexcept`
- Source: `include/metadata/schema_consistency_checker.h`:108
- Brief: n/a
- Parameters: none
- Details: Stop the background thread (blocking until it exits). Called automatically by the destructor.

#### `~SchemaConsistencyChecker()`
- Source: `include/metadata/schema_consistency_checker.h`:86
- Brief: Destructor – stops the background thread if running.
- Parameters: none

### themis::SchemaConstraints

#### `SchemaConstraints()=default`
- Source: `include/metadata/schema_constraints.h`:111
- Brief: n/a
- Parameters: none

#### `SchemaConstraints(SchemaConstraints &&) noexcept=default`
- Source: `include/metadata/schema_constraints.h`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraints &&): n/a

#### `SchemaConstraints(const SchemaConstraints &)=delete`
- Source: `include/metadata/schema_constraints.h`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaConstraints &): n/a

#### `void addConstraint(std::string_view table_name, std::string_view column_name, ColumnConstraint constraint)`
- Source: `include/metadata/schema_constraints.h`:125
- Brief: Add a constraint for a column on a table.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `column_name` (std::string_view): Name of the column.
  - `constraint` (ColumnConstraint): Input parameter.
- Details: Add Constraint. table_name Name of the table. column_name Name of the column. constraint Input parameter.

#### `std::map< std::string, ColumnValue > applyDefaults(std::string_view table_name, std::map< std::string, ColumnValue > row) const`
- Source: `include/metadata/schema_constraints.h`:166
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): n/a
  - `row` (std::map< std::string, ColumnValue >): n/a
- Details: Apply DEFAULT values for any columns that are missing from the row. Returns a copy of row with defaults filled in where applicable.

#### `std::optional< ConstraintViolation > checkCheck(std::string_view table_name, std::string_view column_name, const ColumnConstraint &c, const ColumnValue &value) const`
- Source: `include/metadata/schema_constraints.h`:225
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): n/a
  - `column_name` (std::string_view): n/a
  - `c` (const ColumnConstraint &): n/a
  - `value` (const ColumnValue &): n/a
- Details: Check a CHECK constraint expression (simple key=value string comparison for the initial implementation; real expression evaluation is a future enhancement).

#### `std::optional< ConstraintViolation > checkNotNull(std::string_view table_name, std::string_view column_name, const ColumnConstraint &c, const ColumnValue &value) const`
- Source: `include/metadata/schema_constraints.h`:215
- Brief: Check a NOT NULL constraint for a single column value.
- Parameters:
  - `table_name` (std::string_view): n/a
  - `column_name` (std::string_view): n/a
  - `c` (const ColumnConstraint &): n/a
  - `value` (const ColumnValue &): n/a

#### `std::vector< ConstraintViolation > enforce(std::string_view table_name, const std::map< std::string, ColumnValue > &row) const`
- Source: `include/metadata/schema_constraints.h`:159
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): Table the row belongs to
  - `row` (const std::map< std::string, ColumnValue > &): Map of column_name -> value (missing key = NULL)
- Return: List of constraint violations; empty means valid.
- Details: Validate a row value map against all registered constraints for a table. table_name Table the row belongs to row Map of column_name -> value (missing key = NULL) List of constraint violations; empty means valid.

#### `SchemaConstraints fromJSON(const json &j)`
- Source: `include/metadata/schema_constraints.h`:179
- Brief: Parse constraints from a JSON object produced by toJSON().
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: From JSON. j Input parameter. Return value. Calls: items(), is_object(), is_array(), kindFromString(), value(), std::string(), contains(), is_string().

#### `std::vector< ColumnConstraint > getColumnConstraints(std::string_view table_name, std::string_view column_name) const`
- Source: `include/metadata/schema_constraints.h`:141
- Brief: Retrieve all constraints for a column (empty vector if none).
- Parameters:
  - `table_name` (std::string_view): n/a
  - `column_name` (std::string_view): n/a

#### `std::vector< ColumnConstraint > getTableConstraints(std::string_view table_name) const`
- Source: `include/metadata/schema_constraints.h`:147
- Brief: Retrieve all constraints for a table (all columns).
- Parameters:
  - `table_name` (std::string_view): n/a

#### `size_t loadFrom(RocksDBWrapper &db)`
- Source: `include/metadata/schema_constraints.h`:200
- Brief: Load From.
- Parameters:
  - `db` (RocksDBWrapper &): Input/output parameter.
- Return: Number of tables loaded (0 = none found)
- Details: Load constraints for all tables whose keys are found in RocksDB under the "config:constraints:" prefix. Replaces existing in-memory state. db RocksDB wrapper to read from Number of tables loaded (0 = none found) db Input/output parameter. Return value. Calls: clear(), newIterator(), spdlog::warn(), value(), Seek(), Valid(), key(), ToString().

#### `bool loadTableFrom(RocksDBWrapper &db, std::string_view table_name)`
- Source: `include/metadata/schema_constraints.h`:207
- Brief: Load Table From.
- Parameters:
  - `db` (RocksDBWrapper &): Input/output parameter.
  - `table_name` (std::string_view): Name of the table.
- Return: true if constraints were found and loaded
- Details: Load constraints for a single table from RocksDB. Merges with existing in-memory constraints for that table. db RocksDB wrapper to read from table_name Table to load true if constraints were found and loaded db Input/output parameter. table_name Name of the table. True when the operation succeeds.

#### `SchemaConstraints & operator=(SchemaConstraints &&) noexcept=default`
- Source: `include/metadata/schema_constraints.h`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaConstraints &&): n/a

#### `SchemaConstraints & operator=(const SchemaConstraints &)=delete`
- Source: `include/metadata/schema_constraints.h`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaConstraints &): n/a

#### `bool persistTableTo(RocksDBWrapper &db, std::string_view table_name) const`
- Source: `include/metadata/schema_constraints.h`:193
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): RocksDB wrapper to write to
  - `table_name` (std::string_view): Table whose constraints should be persisted
- Details: Persist constraints for a single table. db RocksDB wrapper to write to table_name Table whose constraints should be persisted

#### `bool persistTo(RocksDBWrapper &db) const`
- Source: `include/metadata/schema_constraints.h`:188
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): RocksDB wrapper to write to
- Return: true if all tables were persisted successfully
- Details: Persist all constraints to RocksDB under "config:constraints:<table>" keys. db RocksDB wrapper to write to true if all tables were persisted successfully

#### `void removeColumnConstraints(std::string_view table_name, std::string_view column_name)`
- Source: `include/metadata/schema_constraints.h`:132
- Brief: Remove all constraints on a specific column.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `column_name` (std::string_view): Name of the column.
- Details: Remove Column Constraints. table_name Name of the table. column_name Name of the column.

#### `void removeTableConstraints(std::string_view table_name)`
- Source: `include/metadata/schema_constraints.h`:138
- Brief: Remove all constraints for an entire table.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Details: Remove Table Constraints. table_name Name of the table. Calls: erase(), std::string().

#### `json toJSON() const`
- Source: `include/metadata/schema_constraints.h`:176
- Brief: Serialise all constraints to JSON.
- Parameters: none

#### `~SchemaConstraints()=default`
- Source: `include/metadata/schema_constraints.h`:112
- Brief: n/a
- Parameters: none

### themis::SchemaManager

#### `SchemaManager(RocksDBWrapper &db, SecondaryIndexManager *index_mgr=nullptr)`
- Source: `include/metadata/schema_manager.h`:175
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): RocksDB wrapper for key scanning
  - `index_mgr` (SecondaryIndexManager *): Secondary index manager (optional, for index metadata)
- Details: Constructor db RocksDB wrapper for key scanning index_mgr Secondary index manager (optional, for index metadata)

#### `SchemaManager(SchemaManager &&) noexcept=delete`
- Source: `include/metadata/schema_manager.h`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManager &&): n/a

#### `SchemaManager(const SchemaManager &)=delete`
- Source: `include/metadata/schema_manager.h`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaManager &): n/a

#### `void buildCache()`
- Source: `include/metadata/schema_manager.h`:372
- Brief: Build cache from scratch.
- Parameters: none
- Details: Build Cache. Calls: spdlog::debug(), std::chrono::steady_clock::now(), clear(), discoverTableNames(), determineTableType(), discoverProperties(), discoverIndexes(), std::getenv().

#### `std::chrono::seconds computeAdaptiveTTL() const`
- Source: `include/metadata/schema_manager.h`:389
- Brief: n/a
- Parameters: none
- Details: Compute the adaptive TTL from current per-table mutation rates. Caller must hold mutation_mutex_.

#### `bool deleteTableSchema(std::string_view table_name)`
- Source: `include/metadata/schema_manager.h`:319
- Brief: Delete a persisted custom schema override.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: true if a custom schema existed and was deleted.
- Details: Delete Table Schema. table_name Table or collection name. true if a custom schema existed and was deleted. table_name Name of the table. True when the operation succeeds. Calls: lock(), find(), std::string(), end(), spdlog::debug(), erase(), del(), spdlog::warn().

#### `std::string determineTableType(std::string_view table_name)`
- Source: `include/metadata/schema_manager.h`:366
- Brief: Determine Table Type.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: "relational", "document", "graph_node", "graph_edge", "vector"
- Details: Determine table type from key schema table_name Table/collection name "relational", "document", "graph_node", "graph_edge", "vector" table_name Name of the table. Return value. Calls: name_lower(), std::transform(), begin(), end(), std::tolower(), find().

#### `void disableAdaptiveTTL()`
- Source: `include/metadata/schema_manager.h`:263
- Brief: Disable adaptive TTL and revert to the fixed TTL set by setCacheTTL().
- Parameters: none
- Details: Disable Adaptive TTL. Calls: lock(), spdlog::info(), count().

#### `std::vector< IndexInfo > discoverIndexes(std::string_view table_name)`
- Source: `include/metadata/schema_manager.h`:356
- Brief: Discover Indexes.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Discover indexes for a table from SecondaryIndexManager table_name Table/collection name table_name Name of the table. Return value. Calls: spdlog::debug(), SecondaryIndexMetadataCache::instance(), get(), push_back(), find(), end(), size(), spdlog::error().

#### `std::vector< PropertyInfo > discoverProperties(std::string_view table_name, size_t sample_size=100)`
- Source: `include/metadata/schema_manager.h`:349
- Brief: Discover Properties.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `sample_size` (size_t): Input parameter.
- Return: Return value.
- Details: Discover properties for a table by sampling entities Parses BaseEntity objects to detect property types table_name Table/collection name sample_size Number of entities to sample (default: 100) table_name Name of the table. sample_size Input parameter. Return value. Calls: contains(), std::string(), spdlog::debug(), newIterator(), spdlog::warn(), error(), message(), std::move().

#### `std::vector< std::string > discoverTableNames()`
- Source: `include/metadata/schema_manager.h`:343
- Brief: Discover Table Names.
- Parameters: none
- Return: Return value.
- Details: Discover all tables by scanning RocksDB keys Scans key prefixes to identify table/collection names Return value. Calls: newIterator(), spdlog::warn(), error(), message(), std::move(), value(), SeekToFirst(), Valid().

#### `void enableAdaptiveTTL(AdaptiveTTLConfig config={})`
- Source: `include/metadata/schema_manager.h`:258
- Brief: Enable adaptive TTL mode.
- Parameters:
  - `config` (AdaptiveTTLConfig): Input parameter.
- Details: Enable Adaptive TTL. The effective cache TTL is recomputed on every cache-validity check based on the per-table mutation rate observed in a sliding window. Calling this method resets any previously collected mutation history. config Adaptive TTL parameters (uses defaults if omitted). config Input parameter. Calls: lock(), clear(), spdlog::info(), count().

#### `size_t estimateRowCount(std::string_view table_name)`
- Source: `include/metadata/schema_manager.h`:361
- Brief: Estimate Row Count.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Estimate row count for a table Uses RocksDB iterator to count keys with table prefix table_name Table/collection name table_name Name of the table. Return value. Calls: std::string(), newIterator(), std::move(), value(), Seek(), Valid(), key(), ToString().

#### `std::vector< RelationshipSchema > getAllRelationships()`
- Source: `include/metadata/schema_manager.h`:210
- Brief: Return all discovered relationship schemas.
- Parameters: none
- Return: Edge and relationship types discovered in the database.
- Details: Get All Relationships. Edge and relationship types discovered in the database. Return value. Calls: lock(), isCacheValid(), unlock(), write_lock(), buildCache(), reserve(), size(), push_back().

#### `std::vector< TableSchema > getAllTables()`
- Source: `include/metadata/schema_manager.h`:197
- Brief: Return all discovered tables and collections.
- Parameters: none
- Return: Cached or freshly discovered table schemas.
- Details: Get All Tables. Cached or freshly discovered table schemas. Return value. Calls: lock(), isCacheValid(), unlock(), write_lock(), buildCache(), reserve(), size(), push_back().

#### `json getCapabilitiesJSON()`
- Source: `include/metadata/schema_manager.h`:292
- Brief: Export build- and runtime-capability flags as JSON.
- Parameters: none
- Return: JSON object listing enabled database capabilities.
- Details: Get Capabilities JSON. JSON object listing enabled database capabilities. Return value. Calls: getDatabaseMetadata().

#### `DatabaseMetadata getDatabaseMetadata()`
- Source: `include/metadata/schema_manager.h`:216
- Brief: Return aggregated database-level metadata.
- Parameters: none
- Return: Version, capability, and estimated-count snapshot.
- Details: Get Database Metadata. Version, capability, and estimated-count snapshot. Return value. Calls: lock(), isCacheValid(), unlock(), write_lock(), buildCache(), size(), push_back(), spdlog::debug().

#### `std::chrono::seconds getEffectiveTTL() const`
- Source: `include/metadata/schema_manager.h`:269
- Brief: Return the currently effective schema-cache TTL.
- Parameters: none
- Return: Fixed TTL when adaptive mode is off, otherwise the rate-adjusted TTL.
- Details: Fixed TTL when adaptive mode is off, otherwise the rate-adjusted TTL.

#### `std::optional< TableSchema > getTable(std::string_view name)`
- Source: `include/metadata/schema_manager.h`:204
- Brief: Return the schema for one table or collection.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Return: Table schema, or std::nullopt if the table is unknown.
- Details: Get Table. name Table or collection name. Table schema, or std::nullopt if the table is unknown. name Input parameter. Return value. Calls: lock(), isCacheValid(), unlock(), write_lock(), buildCache(), find(), std::string(), end().

#### `bool isCacheValid() const`
- Source: `include/metadata/schema_manager.h`:369
- Brief: Check if cache is valid (not expired).
- Parameters: none

#### `void loadCustomSchemas()`
- Source: `include/metadata/schema_manager.h`:380
- Brief: Load custom schemas from RocksDB.
- Parameters: none
- Details: Load Custom Schemas. Calls: newIterator(), spdlog::warn(), std::move(), value(), Seek(), Valid(), key(), ToString().

#### `void notifySchemaChange(std::string_view table_name, std::string_view event_kind)`
- Source: `include/metadata/schema_manager.h`:377
- Brief: Notify Schema Change.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `event_kind` (std::string_view): Input parameter.
- Details: Emit a schema change event to the registered changefeed (if any). table_name Table that changed event_kind "schema_created", "schema_updated", or "schema_deleted" table_name Name of the table. event_kind Input parameter. Calls: std::string(), std::chrono::system_clock::now(), time_since_epoch(), count(), recordEvent(), std::move(), spdlog::debug(), spdlog::warn().

#### `SchemaManager & operator=(SchemaManager &&) noexcept=delete`
- Source: `include/metadata/schema_manager.h`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaManager &&): n/a

#### `SchemaManager & operator=(const SchemaManager &)=delete`
- Source: `include/metadata/schema_manager.h`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaManager &): n/a

#### `TableSchema parseTableSchema(const json &j)`
- Source: `include/metadata/schema_manager.h`:334
- Brief: Parse a TableSchema from JSON.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Parsed TableSchema value.
- Throws:
  - std::runtime_error: if the JSON shape is malformed or required fields are missing.
  - std::runtime_error: if an error occurs.
- Details: Parse Table Schema. j JSON object representing a table schema. Parsed TableSchema value. std::runtime_error if the JSON shape is malformed or required fields are missing. j Input parameter. Return value. std::runtime_error if an error occurs. Calls: contains(), is_string(), is_array(), is_boolean(), push_back(), is_number().

#### `bool patchTableSchema(std::string_view table_name, const json &updates)`
- Source: `include/metadata/schema_manager.h`:312
- Brief: Apply a partial JSON patch to an existing schema.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `updates` (const json &): Input parameter.
- Return: true on success, false if the table is missing or validation fails.
- Details: Patch Table Schema. table_name Table or collection name. updates JSON object with fields to update. true on success, false if the table is missing or validation fails. table_name Name of the table. updates Input parameter. True when the operation succeeds. Calls: lock(), find(), std::string(), end(), spdlog::warn(), contains(), is_string(), is_array().

#### `void recordMutation(std::string_view table_name)`
- Source: `include/metadata/schema_manager.h`:247
- Brief: Record a data mutation for a table.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Details: Record Mutation. When adaptive TTL is enabled, high-frequency mutations cause the cache to expire sooner so that stale statistics are refreshed more quickly. This method is thread-safe and non-blocking. table_name Name of the table that was mutated. table_name Name of the table. Calls: lock(), std::chrono::system_clock::now(), std::string(), push_back(), empty(), front(), pop_front(), spdlog::debug().

#### `void refreshCache()`
- Source: `include/metadata/schema_manager.h`:224
- Brief: Force a full schema-cache refresh.
- Parameters: none
- Details: Refresh Cache. Rescans RocksDB and rebuilds all cached schema views. Call this after structural changes such as create or drop table operations. Calls: lock(), buildCache(), spdlog::info().

#### `void saveCustomSchema(std::string_view table_name, const TableSchema &schema)`
- Source: `include/metadata/schema_manager.h`:385
- Brief: Save Custom Schema.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `schema` (const TableSchema &): Input parameter.
- Details: Save custom schema to RocksDB table_name Table name schema Schema to save table_name Name of the table. schema Input parameter. Calls: std::string(), toJSON(), dump(), put(), begin(), end(), spdlog::error(), spdlog::debug().

#### `void setCacheTTL(std::chrono::seconds ttl)`
- Source: `include/metadata/schema_manager.h`:230
- Brief: Set the fixed schema-cache time-to-live.
- Parameters:
  - `ttl` (std::chrono::seconds): Input parameter.
- Details: Set Cache TTL. ttl Cache expiration duration. ttl Input parameter. Calls: spdlog::debug(), count().

#### `void setChangefeed(Changefeed *changefeed)`
- Source: `include/metadata/schema_manager.h`:236
- Brief: Set Changefeed.
- Parameters:
  - `changefeed` (Changefeed *): Input/output parameter.
- Details: Register a Changefeed for real-time schema change notifications. When set, every schema mutation (create/update/delete) emits a ChangeEvent with key "schema:{table_name}" into the given changefeed. changefeed Non-owning pointer; may be nullptr to disable notifications. changefeed Input/output parameter. Calls: spdlog::info(), spdlog::debug().

#### `bool setTableSchema(std::string_view table_name, const TableSchema &schema)`
- Source: `include/metadata/schema_manager.h`:304
- Brief: Store or replace a custom schema override for a table.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `schema` (const TableSchema &): Input parameter.
- Return: true on success, false on validation failure.
- Details: ============================================================================ Schema Management API (PUT/PATCH) ============================================================================ table_name Table or collection name. schema Custom schema definition to persist. true on success, false on validation failure. table_name Name of the table. schema Input parameter. True when the operation succeeds. Calls: validateSchema(), empty(), spdlog::error(), lock(), std::string(), saveCustomSchema(), unlock(), spdlog::info().

#### `json tableToJSON(std::string_view table_name)`
- Source: `include/metadata/schema_manager.h`:286
- Brief: Export one table schema as JSON.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: JSON object for the table, or an empty object if unknown.
- Details: Table To JSON. table_name Table or collection name. JSON object for the table, or an empty object if unknown. table_name Name of the table. Return value. Calls: getTable(), toJSON(), std::string().

#### `json toJSON()`
- Source: `include/metadata/schema_manager.h`:279
- Brief: Export the full discovered schema as JSON.
- Parameters: none
- Return: JSON representation compatible with REST and MCP consumers.
- Details: To JSON. JSON representation compatible with REST and MCP consumers. Return value. Calls: getAllTables(), getAllRelationships(), getDatabaseMetadata(), json::array(), push_back().

#### `std::string validateSchema(const TableSchema &schema) const`
- Source: `include/metadata/schema_manager.h`:326
- Brief: Validate a table schema before persistence or application.
- Parameters:
  - `schema` (const TableSchema &): Schema candidate to validate.
- Return: Empty string when valid, otherwise a human-readable error message.
- Details: schema Schema candidate to validate. Empty string when valid, otherwise a human-readable error message.

#### `~SchemaManager()=default`
- Source: `include/metadata/schema_manager.h`:181
- Brief: Destructor.
- Parameters: none

### themis::SchemaManager::DatabaseMetadata

#### `json toJSON() const`
- Source: `include/metadata/schema_manager.h`:169
- Brief: Serialise this database metadata snapshot to JSON.
- Parameters: none
- Return: JSON object representing database-level capabilities and counts.
- Details: JSON object representing database-level capabilities and counts.

### themis::SchemaManager::IndexInfo

#### `json toJSON() const`
- Source: `include/metadata/schema_manager.h`:119
- Brief: Serialise this index descriptor to JSON.
- Parameters: none
- Return: JSON object representing the index metadata.
- Details: JSON object representing the index metadata.

### themis::SchemaManager::PropertyInfo

#### `json toJSON() const`
- Source: `include/metadata/schema_manager.h`:103
- Brief: Serialise this property descriptor to JSON.
- Parameters: none
- Return: JSON object representing the property metadata.
- Details: JSON object representing the property metadata.

### themis::SchemaManager::RelationshipSchema

#### `json toJSON() const`
- Source: `include/metadata/schema_manager.h`:152
- Brief: Serialise this relationship schema to JSON.
- Parameters: none
- Return: JSON object representing the relationship metadata.
- Details: JSON object representing the relationship metadata.

### themis::SchemaManager::TableSchema

#### `json toJSON() const`
- Source: `include/metadata/schema_manager.h`:136
- Brief: Serialise this table schema to JSON.
- Parameters: none
- Return: JSON object representing the table metadata.
- Details: JSON object representing the table metadata.

### themis::SchemaVersionManager

#### `SchemaVersionManager(RocksDBWrapper &db, SchemaManager &schema_mgr)`
- Source: `include/metadata/schema_version_manager.h`:139
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): RocksDB wrapper used for persistence
  - `schema_mgr` (SchemaManager &): SchemaManager whose schema is being versioned
- Details: Constructor db RocksDB wrapper used for persistence schema_mgr SchemaManager whose schema is being versioned

#### `SchemaVersionManager(SchemaVersionManager &&) noexcept=delete`
- Source: `include/metadata/schema_version_manager.h`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManager &&): n/a

#### `SchemaVersionManager(const SchemaVersionManager &)=delete`
- Source: `include/metadata/schema_version_manager.h`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaVersionManager &): n/a

#### `VersionResult< uint64_t > createSchemaVersion(std::string_view table_name, std::string_view author="", std::string_view description="")`
- Source: `include/metadata/schema_version_manager.h`:158
- Brief: Create Schema Version.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `author` (std::string_view): Input parameter.
  - `description` (std::string_view): Input parameter.
- Return: The newly assigned version number, or an error result.
- Details: Snapshot the current schema for table_name and store it as a new version. table_name Table whose schema should be versioned author Identity of the change author (may be empty) description Human-readable description of the change The newly assigned version number, or an error result. table_name Name of the table. author Input parameter. description Input parameter. Return value.

#### `std::string currentVersionKey(std::string_view table_name)`
- Source: `include/metadata/schema_version_manager.h`:281
- Brief: Build the RocksDB key for the "current version" counter.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Current Version Key. table_name Name of the table. Return value. Calls: std::string().

#### `VersionResult< json > diffVersions(std::string_view table_name, uint64_t version_a, uint64_t version_b) const`
- Source: `include/metadata/schema_version_manager.h`:210
- Brief: Compute a JSON diff between two schema versions.
- Parameters:
  - `table_name` (std::string_view): Table whose versions should be compared.
  - `version_a` (uint64_t): Older or left-hand version to compare.
  - `version_b` (uint64_t): Newer or right-hand version to compare.
- Return: JSON diff object with added, removed, and modified arrays.
- Details: table_name Table whose versions should be compared. version_a Older or left-hand version to compare. version_b Newer or right-hand version to compare. JSON diff object with added, removed, and modified arrays.

#### `VersionResult< std::string > generateMigrationScript(std::string_view table_name, uint64_t version_from, uint64_t version_to) const`
- Source: `include/metadata/schema_version_manager.h`:244
- Brief: Generate a DDL migration script between two schema versions.
- Parameters:
  - `table_name` (std::string_view): Table whose versions to compare.
  - `version_from` (uint64_t): Source version (the "before" state).
  - `version_to` (uint64_t): Target version (the "after" state).
- Return: VersionResult<std::string> containing the script on success.
- Details: Produces a sequence of ALTER TABLE statements that, when executed in order, transform table_name from the schema at version_from to the schema at version_to. Generated statement types: ADD COLUMN – for columns present in version_to but not in version_from DROP COLUMN – for columns present in version_from but not in version_to ALTER COLUMN – for columns whose type or nullability changed Type mapping (ThemisDB → SQL): string → VARCHAR, integer → INTEGER, double → DOUBLE PRECISION, boolean → BOOLEAN, vector → VECTOR, binary → BYTEA, * → TEXT table_name Table whose versions to compare. version_from Source version (the "before" state). version_to Target version (the "after" state). VersionResult<std::string> containing the script on success.

#### `VersionResult< std::vector< SchemaChange > > getChangeHistory(std::string_view table_name) const`
- Source: `include/metadata/schema_version_manager.h`:176
- Brief: Return the full schema-change history for a table.
- Parameters:
  - `table_name` (std::string_view): Table whose version history should be loaded.
- Return: Ordered list of schema changes in ascending version order.
- Details: table_name Table whose version history should be loaded. Ordered list of schema changes in ascending version order.

#### `VersionResult< uint64_t > getCurrentVersion(std::string_view table_name) const`
- Source: `include/metadata/schema_version_manager.h`:169
- Brief: Return the current highest schema version for a table.
- Parameters:
  - `table_name` (std::string_view): Table whose current version should be queried.
- Return: Latest version number, or VersionErrorCode::TABLE_NOT_FOUND.
- Details: table_name Table whose current version should be queried. Latest version number, or VersionErrorCode::TABLE_NOT_FOUND.

#### `VersionResult< SchemaChange > getVersion(std::string_view table_name, uint64_t version) const`
- Source: `include/metadata/schema_version_manager.h`:186
- Brief: Return one schema version snapshot.
- Parameters:
  - `table_name` (std::string_view): Table whose version should be read.
  - `version` (uint64_t): Version number to retrieve.
- Return: Matching SchemaChange snapshot, or an error result.
- Details: table_name Table whose version should be read. version Version number to retrieve. Matching SchemaChange snapshot, or an error result.

#### `json historyToJSON(std::string_view table_name) const`
- Source: `include/metadata/schema_version_manager.h`:221
- Brief: Export all schema versions for a table as JSON.
- Parameters:
  - `table_name` (std::string_view): Table whose history should be exported.
- Return: JSON array of SchemaChange records.
- Details: table_name Table whose history should be exported. JSON array of SchemaChange records.

#### `std::optional< SchemaChange > loadChange(std::string_view table_name, uint64_t version) const`
- Source: `include/metadata/schema_version_manager.h`:290
- Brief: Load a SchemaChange record from RocksDB by (table, version).
- Parameters:
  - `table_name` (std::string_view): n/a
  - `version` (uint64_t): n/a

#### `SchemaVersionManager & operator=(SchemaVersionManager &&) noexcept=delete`
- Source: `include/metadata/schema_version_manager.h`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaVersionManager &&): n/a

#### `SchemaVersionManager & operator=(const SchemaVersionManager &)=delete`
- Source: `include/metadata/schema_version_manager.h`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaVersionManager &): n/a

#### `bool persistChange(const SchemaChange &change)`
- Source: `include/metadata/schema_version_manager.h`:287
- Brief: Persist a SchemaChange record to RocksDB.
- Parameters:
  - `change` (const SchemaChange &): Input parameter.
- Return: True when the operation succeeds.
- Details: Persist Change. change Input parameter. True when the operation succeeds. Calls: versionKey(), toJSON(), dump(), data(), begin(), end(), put(), spdlog::error().

#### `uint64_t readCurrentVersion(std::string_view table_name) const`
- Source: `include/metadata/schema_version_manager.h`:284
- Brief: Read the raw "current version" counter from RocksDB (0 = none).
- Parameters:
  - `table_name` (std::string_view): n/a

#### `VersionResult< bool > rollbackToVersion(std::string_view table_name, uint64_t version, std::string_view author="")`
- Source: `include/metadata/schema_version_manager.h`:197
- Brief: Rollback To Version.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `version` (uint64_t): Input parameter.
  - `author` (std::string_view): Input parameter.
- Return: Return value.
- Details: Roll the live schema back to a specific version. Applies the schema snapshot stored at version via SchemaManager::setTableSchema. Records the rollback itself as a new version entry so history is preserved. table_name Table to roll back version Target version author Identity of who initiated the rollback table_name Name of the table. version Input parameter. author Input parameter. Return value.

#### `void setAuditLog(SchemaAuditLog *audit_log) noexcept`
- Source: `include/metadata/schema_version_manager.h`:270
- Brief: n/a
- Parameters:
  - `audit_log` (SchemaAuditLog *): n/a
- Details: Attach an audit log. If set, every schema change is also recorded there. The pointer is non-owning; caller manages the lifetime.

#### `VersionResult< bool > validateMigration(std::string_view table_name, const SchemaManager::TableSchema &new_schema) const`
- Source: `include/metadata/schema_version_manager.h`:263
- Brief: Dry-run whether a new schema can be applied to a table.
- Parameters:
  - `table_name` (std::string_view): Table to validate against.
  - `new_schema` (const SchemaManager::TableSchema &): Proposed new schema.
- Return: VersionResult<bool> where ok=true means the migration is valid.
- Details: Checks performed: The new schema has a non-empty name field. The new schema has a columns or properties array. No column appears more than once in the new schema. If the table already has a versioned schema the new schema is not identical. table_name Table to validate against. new_schema Proposed new schema. VersionResult<bool> where ok=true means the migration is valid.

#### `std::string versionKey(std::string_view table_name, uint64_t version)`
- Source: `include/metadata/schema_version_manager.h`:278
- Brief: Build the RocksDB key for a specific version entry.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `version` (uint64_t): Input parameter.
- Return: Return value.
- Details: Version Key. table_name Name of the table. version Input parameter. Return value.

#### `~SchemaVersionManager()=default`
- Source: `include/metadata/schema_version_manager.h`:141
- Brief: n/a
- Parameters: none

### themis::StatisticsCollector

#### `StatisticsCollector(RocksDBWrapper &db)`
- Source: `include/metadata/statistics_collector.h`:240
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): RocksDB wrapper for key scanning and persisting statistics
- Details: Constructor db RocksDB wrapper for key scanning and persisting statistics

#### `StatisticsCollector(StatisticsCollector &&)=delete`
- Source: `include/metadata/statistics_collector.h`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollector &&): n/a

#### `StatisticsCollector(const StatisticsCollector &)=delete`
- Source: `include/metadata/statistics_collector.h`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StatisticsCollector &): n/a

#### `ColumnStats buildColumnStats(std::string_view column_name, const std::vector< std::string > &values, size_t num_histogram_buckets=kDefaultHistogramBuckets)`
- Source: `include/metadata/statistics_collector.h`:327
- Brief: Build column statistics from a set of sampled raw values.
- Parameters:
  - `column_name` (std::string_view): Name of the column.
  - `values` (const std::vector< std::string > &): Input parameter.
  - `num_histogram_buckets` (size_t): Input parameter.
- Return: Return value.
- Details: Build Column Stats. column_name Name of the column. values Input parameter. num_histogram_buckets Input parameter. Return value.

#### `std::vector< HistogramBucket > buildHistogram(const std::vector< double > &sorted_values, size_t num_buckets)`
- Source: `include/metadata/statistics_collector.h`:334
- Brief: Build an equi-height histogram from a sorted list of double values.
- Parameters:
  - `sorted_values` (const std::vector< double > &): Input parameter.
  - `num_buckets` (size_t): Input parameter.
- Return: Return value.
- Details: Build Histogram. sorted_values Input parameter. num_buckets Input parameter. Return value.

#### `StatsResult< bool > clearIndexStats(std::string_view table_name)`
- Source: `include/metadata/statistics_collector.h`:316
- Brief: Clear Index Stats.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Remove cached and persisted index statistics for a table. table_name Table/collection name table_name Name of the table. Return value. Calls: empty(), failure(), lock(), erase(), std::string(), del(), spdlog::debug(), success().

#### `StatsResult< bool > clearStats(std::string_view table_name)`
- Source: `include/metadata/statistics_collector.h`:297
- Brief: Clear Stats.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Remove cached and persisted statistics for a table. table_name Table/collection name table_name Name of the table. Return value. Calls: empty(), failure(), lock(), erase(), std::string(), del(), spdlog::debug(), success().

#### `StatsResult< TableStats > collectStats(std::string_view table_name, size_t sample_size=0)`
- Source: `include/metadata/statistics_collector.h`:280
- Brief: Collect Stats.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `sample_size` (size_t): Input parameter.
- Return: Return value.
- Details: Collect full statistics for a table by sampling stored entities. Writes the result to in-memory cache and persists it to RocksDB. table_name Table/collection name sample_size Number of rows to sample (0 = use kDefaultSampleSize) table_name Name of the table. sample_size Input parameter. Return value.

#### `StatsResult< std::vector< IndexStats > > getIndexStats(std::string_view table_name)`
- Source: `include/metadata/statistics_collector.h`:312
- Brief: Get Index Stats.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Retrieve cached index statistics for a table. Loads from RocksDB persistence if not already in memory. table_name Table/collection name table_name Name of the table. Return value.

#### `StatsResult< TableStats > getStats(std::string_view table_name)`
- Source: `include/metadata/statistics_collector.h`:288
- Brief: Get Stats.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Retrieve cached statistics for a table (no re-scan). Loads from RocksDB persistence if not already in memory. table_name Table/collection name table_name Name of the table. Return value. Calls: empty(), onError(), failure(), lock(), find(), std::string(), end(), onCacheHit().

#### `StatsResult< bool > importIndexStats(std::string_view table_name, const std::vector< IndexStats > &stats)`
- Source: `include/metadata/statistics_collector.h`:304
- Brief: Import Index Stats.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `stats` (const std::vector< IndexStats > &): Input parameter.
- Return: Return value.
- Details: Import index statistics for a table exported from the index module. Stores the stats in-memory and persists them to RocksDB under "idxstats:<table_name>". table_name Table/collection name stats Index stats to import (replaces any previously cached stats) table_name Name of the table. stats Input parameter. Return value.

#### `std::optional< std::vector< IndexStats > > loadIndexStats(std::string_view table_name)`
- Source: `include/metadata/statistics_collector.h`:349
- Brief: Load index stats from RocksDB; returns nullopt if not found.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Load Index Stats. table_name Name of the table. Return value.

#### `std::optional< TableStats > loadStats(std::string_view table_name)`
- Source: `include/metadata/statistics_collector.h`:343
- Brief: Load TableStats from RocksDB; returns nullopt if not found.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Load Stats. table_name Name of the table. Return value. Calls: std::string(), get(), has_value(), empty(), json_str(), begin(), end(), json::parse().

#### `StatisticsCollector & operator=(StatisticsCollector &&)=delete`
- Source: `include/metadata/statistics_collector.h`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (StatisticsCollector &&): n/a

#### `StatisticsCollector & operator=(const StatisticsCollector &)=delete`
- Source: `include/metadata/statistics_collector.h`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StatisticsCollector &): n/a

#### `void persistIndexStats(std::string_view table_name, const std::vector< IndexStats > &stats)`
- Source: `include/metadata/statistics_collector.h`:346
- Brief: Persist index stats to RocksDB under "idxstats:<table_name>".
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `stats` (const std::vector< IndexStats > &): Input parameter.
- Details: Persist Index Stats. table_name Name of the table. stats Input parameter.

#### `void persistStats(const TableStats &stats)`
- Source: `include/metadata/statistics_collector.h`:340
- Brief: Persist TableStats to RocksDB under "stats:<table_name>".
- Parameters:
  - `stats` (const TableStats &): Input parameter.
- Details: Persist Stats. stats Input parameter. Calls: toJSON(), dump(), data(), begin(), end(), put(), spdlog::warn(), spdlog::error().

#### `void refreshLoop_()`
- Source: `include/metadata/statistics_collector.h`:371
- Brief: Background refresh loop (runs on refresh_thread_).
- Parameters: none
- Details: Refresh Loop. Calls: load(), lk(), wait_for(), sl(), reserve(), size(), push_back(), collectStats().

#### `void setMetricsHook(IMetricsHook *hook) noexcept`
- Source: `include/metadata/statistics_collector.h`:236
- Brief: n/a
- Parameters:
  - `hook` (IMetricsHook *): n/a
- Details: Attach a metrics hook. The pointer is non-owning; caller manages lifetime. Pass nullptr to remove the hook.

#### `void setRefreshInterval(std::chrono::seconds interval)`
- Source: `include/metadata/statistics_collector.h`:266
- Brief: Set Refresh Interval.
- Parameters:
  - `interval` (std::chrono::seconds): Input parameter.
- Details: Configure the background auto-refresh interval. When interval > 0, a background thread wakes every interval seconds and calls collectStats() for every table that has already been sampled at least once. Calling this again updates the interval live. Passing std::chrono::seconds(0) (the default) stops the background thread. The background thread does NOT block the caller; it runs at low priority and skips a table if collectStats() is already running for it. Thread-safety: safe to call from any thread. interval Input parameter. Calls: stopRefresh(), count(), spdlog::debug(), store(), std::thread(), refreshLoop_(), spdlog::info().

#### `void stopRefresh() noexcept`
- Source: `include/metadata/statistics_collector.h`:270
- Brief: n/a
- Parameters: none
- Details: Stop the background refresh thread immediately (blocking until it exits). Called automatically by the destructor.

#### `json toJSON() const`
- Source: `include/metadata/statistics_collector.h`:319
- Brief: Export all cached statistics as a JSON object.
- Parameters: none

#### `StatsResult< bool > updateStats(std::string_view table_name)`
- Source: `include/metadata/statistics_collector.h`:293
- Brief: Update Stats.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
- Return: Return value.
- Details: Force a fresh collection pass and update persisted statistics. Equivalent to collectStats() but returns only a success/failure bool. table_name Table/collection name table_name Name of the table. Return value. Calls: collectStats(), failure(), success().

#### `~StatisticsCollector()`
- Source: `include/metadata/statistics_collector.h`:243
- Brief: Destructor – stops the background refresh thread if running.
- Parameters: none

### themis::StatisticsCollector::IMetricsHook

#### `void onCacheHit(std::string_view table_name)=0`
- Source: `include/metadata/statistics_collector.h`:224
- Brief: Called whenever in-memory cache satisfies a getStats() request.
- Parameters:
  - `table_name` (std::string_view): n/a

#### `void onCacheMiss(std::string_view table_name)=0`
- Source: `include/metadata/statistics_collector.h`:227
- Brief: Called whenever getStats() results in a cache miss (loads from RocksDB or re-collects).
- Parameters:
  - `table_name` (std::string_view): n/a

#### `void onCollect(std::string_view table_name, double duration_ms, size_t rows_sampled, bool success)=0`
- Source: `include/metadata/statistics_collector.h`:218
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): Affected table
  - `duration_ms` (double): Wall-clock duration in milliseconds
  - `rows_sampled` (size_t): Number of rows actually scanned
  - `success` (bool): Whether collection succeeded
- Details: Called after a successful or failed stats collection attempt. table_name Affected table duration_ms Wall-clock duration in milliseconds rows_sampled Number of rows actually scanned success Whether collection succeeded

#### `void onError(std::string_view table_name, int error_code)=0`
- Source: `include/metadata/statistics_collector.h`:231
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): n/a
  - `error_code` (int): StatsErrorCode cast to int
- Details: Called on any internal error (iterator failure, parse error, etc.). error_code StatsErrorCode cast to int

#### `~IMetricsHook()=default`
- Source: `include/metadata/statistics_collector.h`:211
- Brief: n/a
- Parameters: none

### themis::StatsResult

#### `StatsResult< T > failure(StatsErrorCode code, std::string msg)`
- Source: `include/metadata/statistics_collector.h`:164
- Brief: n/a
- Parameters:
  - `code` (StatsErrorCode): n/a
  - `msg` (std::string): n/a

#### `StatsResult< T > success(T v)`
- Source: `include/metadata/statistics_collector.h`:157
- Brief: n/a
- Parameters:
  - `v` (T): n/a

### themis::TableStats

#### `json toJSON() const`
- Source: `include/metadata/statistics_collector.h`:136
- Brief: n/a
- Parameters: none

### themis::VersionResult

#### `VersionResult< T > failure(VersionErrorCode code, std::string msg)`
- Source: `include/metadata/schema_version_manager.h`:98
- Brief: Construct a failed result.
- Parameters:
  - `code` (VersionErrorCode): Typed error code describing the failure.
  - `msg` (std::string): Human-readable failure description stored in error_message.
- Return: VersionResult with ok == false.
- Details: code Typed error code describing the failure. msg Human-readable failure description stored in error_message. VersionResult with ok == false.

#### `VersionResult< T > success(T v)`
- Source: `include/metadata/schema_version_manager.h`:85
- Brief: Construct a successful result.
- Parameters:
  - `v` (T): Payload value received by value; callers can pass std::move(...) to avoid an extra copy for large payloads.
- Return: VersionResult with ok == true.
- Details: v Payload value received by value; callers can pass std::move(...) to avoid an extra copy for large payloads. VersionResult with ok == true.

### themis::aql

#### `CollectionMetadata fromTableSchema(const SchemaManager::TableSchema &ts)`
- Source: `include/metadata/aql_schema_bridge.h`:47
- Brief: n/a
- Parameters:
  - `ts` (const SchemaManager::TableSchema &): Source table schema from SchemaManager. An empty properties list is valid and results in a CollectionMetadata with no fields.
- Return: CollectionMetadata populated with name, type, estimated_count, and one CollectionFieldInfo per property in ts.
- Details: Convert a live SchemaManager::TableSchema to a lightweight CollectionMetadata snapshot. This helper is the bridge between the metadata module (which owns live schema data) and the aql module (which consumes a portable snapshot for query generation and validation). ts Source table schema from SchemaManager. An empty properties list is valid and results in a CollectionMetadata with no fields. CollectionMetadata populated with name, type, estimated_count, and one CollectionFieldInfo per property in ts. Usage example: SchemaManagerschema_mgr(db,idx_mgr); autotables=schema_mgr.getAllTables(); std::vector<aql::CollectionMetadata>meta; meta.reserve(tables.size()); for(constauto&t:tables){ meta.push_back(aql::fromTableSchema(t)); } builder.setSchema(meta);

### themis::metadata

#### `TransformationType transformationTypeFromString(const std::string &s)`
- Source: `src/metadata/column_lineage.cpp`:87
- Brief: Convert a string label back to TransformationType.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Matching TransformationType value.
- Details: Transformation Type From String. s Transformation label to parse (case-insensitive). Matching TransformationType value. s Input parameter. Return value. Calls: std::transform(), begin(), end().

#### `std::string transformationTypeToString(TransformationType t)`
- Source: `src/metadata/column_lineage.cpp`:67
- Brief: Convert TransformationType to a string label.
- Parameters:
  - `t` (TransformationType): Input parameter.
- Return: Return value.
- Details: ─── TransformationType helpers ────────────────────────────────────────────── t Input parameter. Return value. Implements transformationTypeToString without additional internal calls.

### themis::metadata::AlwaysExportPolicy

#### `std::chrono::milliseconds exportDelay(std::string_view, MetadataExportTrigger) const override`
- Source: `include/metadata/imetadata_export_policy.h`:110
- Brief: Return the delay to apply before exporting table_name.
- Parameters:
  - `table_name` (std::string_view): The name of the table/collection.
  - `trigger` (MetadataExportTrigger): The event that triggered the export request.
- Return: Delay in milliseconds (0 = immediate).
- Details: A non-zero delay lets the exporter batch several changes that arrive within the window. Zero means export immediately. table_name The name of the table/collection. trigger The event that triggered the export request. Delay in milliseconds (0 = immediate).

#### `bool shouldExport(std::string_view, MetadataExportTrigger) const override`
- Source: `include/metadata/imetadata_export_policy.h`:105
- Brief: Return true if table_name should be exported on trigger.
- Parameters:
  - `table_name` (std::string_view): The name of the table/collection.
  - `trigger` (MetadataExportTrigger): The event that would trigger the export.
- Return: true if the export should proceed; false to suppress it.
- Details: table_name The name of the table/collection. trigger The event that would trigger the export. true if the export should proceed; false to suppress it.

### themis::metadata::ColumnAccess

#### `json toJSON() const`
- Source: `include/metadata/index_recommender.h`:50
- Brief: n/a
- Parameters: none

### themis::metadata::ColumnDiff

#### `json toJSON() const`
- Source: `include/metadata/schema_diff.h`:64
- Brief: Serialise this diff to a JSON object.
- Parameters: none
- Details: Schema: { "diff_type":"ADDED"\|"REMOVED"\|"TYPE_CHANGED"\|..., "column_name":"<name>", "old_value":"<string>"\|null, "new_value":"<string>"\|null }

### themis::metadata::ColumnLineageEntry

#### `nlohmann::json toJSON() const`
- Source: `include/metadata/column_lineage.h`:105
- Brief: Serialise this lineage entry to JSON.
- Parameters: none
- Return: JSON representation of the recorded derivation step.
- Details: JSON representation of the recorded derivation step.

### themis::metadata::ColumnLineageRecord

#### `nlohmann::json toJSON() const`
- Source: `include/metadata/column_lineage.h`:118
- Brief: Serialise this lineage record to JSON.
- Parameters: none
- Return: JSON object containing the target column and its direct entries.
- Details: JSON object containing the target column and its direct entries.

### themis::metadata::ColumnLineageTracker

#### `ColumnLineageTracker()=default`
- Source: `include/metadata/column_lineage.h`:144
- Brief: n/a
- Parameters: none

#### `ColumnLineageTracker(ColumnLineageTracker &&) noexcept=delete`
- Source: `include/metadata/column_lineage.h`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTracker &&): n/a

#### `ColumnLineageTracker(const ColumnLineageTracker &)=delete`
- Source: `include/metadata/column_lineage.h`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ColumnLineageTracker &): n/a

#### `std::string assignEntryId()`
- Source: `include/metadata/column_lineage.h`:239
- Brief: Generate a unique entry_id.
- Parameters: none
- Return: Return value.
- Details: ─── ColumnLineageTracker ──────────────────────────────────────────────────── Return value. Calls: fetch_add(), str().

#### `nlohmann::json exportAllLineage() const`
- Source: `include/metadata/column_lineage.h`:215
- Brief: Export the full lineage graph as JSON.
- Parameters: none
- Return: JSON object containing all recorded lineage entries.
- Details: Returns a JSON object: {"entries": [...], "total_entries": N} JSON object containing all recorded lineage entries.

#### `nlohmann::json exportTableLineage(const std::string &table_name) const`
- Source: `include/metadata/column_lineage.h`:207
- Brief: Export all lineage entries for every column in table_name.
- Parameters:
  - `table_name` (const std::string &): Table/collection name.
- Return: JSON array of ColumnLineageRecord objects.
- Details: table_name Table/collection name. JSON array of ColumnLineageRecord objects.

#### `ColumnLineageRecord getColumnLineage(const ColumnRef &col) const`
- Source: `include/metadata/column_lineage.h`:165
- Brief: Return all recorded derivation entries whose target is col.
- Parameters:
  - `col` (const ColumnRef &): n/a

#### `nlohmann::json getColumnProvenance(const ColumnRef &col) const`
- Source: `include/metadata/column_lineage.h`:199
- Brief: Return a structured provenance record for col.
- Parameters:
  - `col` (const ColumnRef &): Column to inspect.
- Return: JSON provenance object.
- Details: The returned JSON object contains: "column": the queried column "entries": direct derivation entries for this column "upstream_columns": all transitive source columns "downstream_columns": all transitive derived columns col Column to inspect. JSON provenance object.

#### `std::vector< ColumnRef > getDownstreamColumns(const ColumnRef &col) const`
- Source: `include/metadata/column_lineage.h`:185
- Brief: Transitively return all columns derived from col.
- Parameters:
  - `col` (const ColumnRef &): n/a
- Return: Unique ColumnRef values in BFS order (nearest descendants first).
- Details: Performs a breadth-first traversal of the lineage DAG from col downward (toward leaves). The returned set does not include col itself. Unique ColumnRef values in BFS order (nearest descendants first).

#### `std::vector< ColumnRef > getUpstreamColumns(const ColumnRef &col) const`
- Source: `include/metadata/column_lineage.h`:175
- Brief: Transitively return all source columns that contributed to col.
- Parameters:
  - `col` (const ColumnRef &): n/a
- Return: Unique ColumnRef values in BFS order (nearest ancestors first).
- Details: Performs a breadth-first traversal of the lineage DAG from col upward (toward roots). The returned set does not include col itself. Unique ColumnRef values in BFS order (nearest ancestors first).

#### `ColumnLineageTracker & operator=(ColumnLineageTracker &&) noexcept=delete`
- Source: `include/metadata/column_lineage.h`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColumnLineageTracker &&): n/a

#### `ColumnLineageTracker & operator=(const ColumnLineageTracker &)=delete`
- Source: `include/metadata/column_lineage.h`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ColumnLineageTracker &): n/a

#### `void recordDerivation(ColumnLineageEntry entry)`
- Source: `include/metadata/column_lineage.h`:160
- Brief: Record a derivation step for a target column.
- Parameters:
  - `entry` (ColumnLineageEntry): Input parameter.
- Details: Record Derivation. entry.entry_id is auto-assigned if empty. entry.timestamp_ms is auto-assigned to the current wall-clock time if 0. entry Fully or partially populated ColumnLineageEntry. entry Input parameter. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), empty(), assignEntryId(), spdlog::info(), transformationTypeToString(), size().

#### `size_t totalEntryCount() const`
- Source: `include/metadata/column_lineage.h`:221
- Brief: Return the total number of recorded lineage entries.
- Parameters: none
- Return: Count of append-only derivation records currently stored.
- Details: Count of append-only derivation records currently stored.

### themis::metadata::ColumnRef

#### `ColumnRef fromJSON(const nlohmann::json &j)`
- Source: `include/metadata/column_lineage.h`:55
- Brief: Parse a column reference from JSON.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Parsed ColumnRef value.
- Details: From JSON. j JSON object containing table and column. Parsed ColumnRef value. j Input parameter. Return value. Calls: at().

#### `bool operator==(const ColumnRef &other) const noexcept`
- Source: `include/metadata/column_lineage.h`:34
- Brief: n/a
- Parameters:
  - `other` (const ColumnRef &): n/a

#### `nlohmann::json toJSON() const`
- Source: `include/metadata/column_lineage.h`:48
- Brief: Serialise this column reference to JSON.
- Parameters: none
- Return: JSON object with table and column fields.
- Details: JSON object with table and column fields.

#### `std::string toString() const`
- Source: `include/metadata/column_lineage.h`:42
- Brief: Render the reference as table_name.column_name.
- Parameters: none
- Return: Fully qualified column name string.
- Details: Fully qualified column name string.

### themis::metadata::ColumnRefHash

#### `std::size_t operator()(const ColumnRef &ref) const noexcept`
- Source: `include/metadata/column_lineage.h`:60
- Brief: n/a
- Parameters:
  - `ref` (const ColumnRef &): n/a

### themis::metadata::FieldSetMetadataEncryptionProvider

#### `FieldSetMetadataEncryptionProvider(std::string_view key)`
- Source: `include/metadata/imetadata_encryption_provider.h`:179
- Brief: Construct a provider with the given repeating XOR key.
- Parameters:
  - `key` (std::string_view): Non-empty byte string used as the XOR mask.
- Throws:
  - MetadataEncryptionException: if key is empty.
- Details: key Non-empty byte string used as the XOR mask. MetadataEncryptionException if key is empty.

#### `void addField(std::string_view field_name)`
- Source: `include/metadata/imetadata_encryption_provider.h`:194
- Brief: Register field_name as a field that should be encrypted.
- Parameters:
  - `field_name` (std::string_view): n/a
- Details: Pass "*" to encrypt every field regardless of name.

#### `MetadataEncryptionAlgorithm algorithm() const override`
- Source: `include/metadata/imetadata_encryption_provider.h`:250
- Brief: Returns the algorithm identifier for this provider.
- Parameters: none

#### `std::string decrypt(std::string_view field_name, std::string_view cipher_text) const override`
- Source: `include/metadata/imetadata_encryption_provider.h`:245
- Brief: XOR-decrypt cipher_text.
- Parameters:
  - `field_name` (std::string_view): n/a
  - `cipher_text` (std::string_view): n/a
- Details: Because XOR is its own inverse, this is identical to encrypt().

#### `std::string encrypt(std::string_view field_name, std::string_view value) const override`
- Source: `include/metadata/imetadata_encryption_provider.h`:235
- Brief: XOR-encrypt value.
- Parameters:
  - `field_name` (std::string_view): n/a
  - `value` (std::string_view): n/a
- Throws:
  - MetadataEncryptionException: if the key is empty (should not happen after construction, but guards against future mutations).
- Details: Each byte of the value is XOR'd with the corresponding byte of the key (wrapping around using modulo indexing). MetadataEncryptionException if the key is empty (should not happen after construction, but guards against future mutations).

#### `size_t fieldCount() const`
- Source: `include/metadata/imetadata_encryption_provider.h`:213
- Brief: Returns the number of explicitly registered field names.
- Parameters: none
- Details: Note: when the set contains "*", every field is encrypted even though fieldCount() may return 1.

#### `void removeField(std::string_view field_name)`
- Source: `include/metadata/imetadata_encryption_provider.h`:202
- Brief: Deregister field_name. No-op if it was not registered.
- Parameters:
  - `field_name` (std::string_view): n/a

#### `bool shouldEncrypt(std::string_view field_name) const override`
- Source: `include/metadata/imetadata_encryption_provider.h`:220
- Brief: Returns true if the named field should be encrypted.
- Parameters:
  - `field_name` (std::string_view): Metadata field name (e.g. "connection_string").
- Return: true if this provider will encrypt values for that field.
- Details: field_name Metadata field name (e.g. "connection_string"). true if this provider will encrypt values for that field.

#### `std::string xorTransform(std::string_view field_name, std::string_view data) const`
- Source: `include/metadata/imetadata_encryption_provider.h`:255
- Brief: n/a
- Parameters:
  - `field_name` (std::string_view): n/a
  - `data` (std::string_view): n/a

### themis::metadata::FilteredExportPolicy

#### `FilteredExportPolicy(std::chrono::milliseconds delay=kDefaultDelay)`
- Source: `include/metadata/imetadata_export_policy.h`:159
- Brief: n/a
- Parameters:
  - `delay` (std::chrono::milliseconds): n/a

#### `void addExclusion(std::string_view table_name)`
- Source: `include/metadata/imetadata_export_policy.h`:168
- Brief: Add table_name to the exclusion list.
- Parameters:
  - `table_name` (std::string_view): n/a
- Details: Subsequent shouldExport() calls for that name will return false.

#### `std::chrono::milliseconds exportDelay(std::string_view, MetadataExportTrigger) const override`
- Source: `include/metadata/imetadata_export_policy.h`:191
- Brief: Return the delay to apply before exporting table_name.
- Parameters:
  - `table_name` (std::string_view): The name of the table/collection.
  - `trigger` (MetadataExportTrigger): The event that triggered the export request.
- Return: Delay in milliseconds (0 = immediate).
- Details: A non-zero delay lets the exporter batch several changes that arrive within the window. Zero means export immediately. table_name The name of the table/collection. trigger The event that triggered the export request. Delay in milliseconds (0 = immediate).

#### `void removeExclusion(std::string_view table_name)`
- Source: `include/metadata/imetadata_export_policy.h`:178
- Brief: Remove table_name from the exclusion list.
- Parameters:
  - `table_name` (std::string_view): n/a
- Details: No-op if the name was not excluded.

#### `bool shouldExport(std::string_view table_name, MetadataExportTrigger) const override`
- Source: `include/metadata/imetadata_export_policy.h`:185
- Brief: Return true if table_name should be exported on trigger.
- Parameters:
  - `table_name` (std::string_view): The name of the table/collection.
  - `trigger` (MetadataExportTrigger): The event that would trigger the export.
- Return: true if the export should proceed; false to suppress it.
- Details: table_name The name of the table/collection. trigger The event that would trigger the export. true if the export should proceed; false to suppress it.

### themis::metadata::IMetadataChangeListener

#### `void onMetadataChanged(const MetadataChangeEvent &event)=0`
- Source: `include/metadata/imetadata_change_listener.h`:111
- Brief: Called when a metadata change event occurs.
- Parameters:
  - `event` (const MetadataChangeEvent &): Details of the change.
- Details: event Details of the change. Implementations should return quickly to avoid blocking the dispatcher. Heavy processing should be queued asynchronously.

#### `~IMetadataChangeListener()=default`
- Source: `include/metadata/imetadata_change_listener.h`:101
- Brief: n/a
- Parameters: none

### themis::metadata::IMetadataEncryptionProvider

#### `MetadataEncryptionAlgorithm algorithm() const =0`
- Source: `include/metadata/imetadata_encryption_provider.h`:111
- Brief: Returns the algorithm identifier for this provider.
- Parameters: none

#### `std::string decrypt(std::string_view field_name, std::string_view cipher_text) const =0`
- Source: `include/metadata/imetadata_encryption_provider.h`:105
- Brief: Decrypt cipher_text for the field identified by field_name.
- Parameters:
  - `field_name` (std::string_view): Metadata field name.
  - `cipher_text` (std::string_view): Cipher-text previously produced by encrypt().
- Return: Recovered plain-text value.
- Throws:
  - MetadataEncryptionException: on key / configuration errors or if the cipher-text is malformed.
- Details: field_name Metadata field name. cipher_text Cipher-text previously produced by encrypt(). Recovered plain-text value. MetadataEncryptionException on key / configuration errors or if the cipher-text is malformed.

#### `std::string encrypt(std::string_view field_name, std::string_view value) const =0`
- Source: `include/metadata/imetadata_encryption_provider.h`:93
- Brief: Encrypt value for the field identified by field_name.
- Parameters:
  - `field_name` (std::string_view): Metadata field name; used for key-derivation or AAD.
  - `value` (std::string_view): Plain-text value to encrypt.
- Return: Cipher-text representation (algorithm-specific encoding).
- Throws:
  - MetadataEncryptionException: on key / configuration errors.
- Details: field_name Metadata field name; used for key-derivation or AAD. value Plain-text value to encrypt. Cipher-text representation (algorithm-specific encoding). MetadataEncryptionException on key / configuration errors.

#### `bool shouldEncrypt(std::string_view field_name) const =0`
- Source: `include/metadata/imetadata_encryption_provider.h`:83
- Brief: Returns true if the named field should be encrypted.
- Parameters:
  - `field_name` (std::string_view): Metadata field name (e.g. "connection_string").
- Return: true if this provider will encrypt values for that field.
- Details: field_name Metadata field name (e.g. "connection_string"). true if this provider will encrypt values for that field.

#### `~IMetadataEncryptionProvider()=default`
- Source: `include/metadata/imetadata_encryption_provider.h`:75
- Brief: n/a
- Parameters: none

### themis::metadata::IMetadataExportPolicy

#### `std::chrono::milliseconds exportDelay(std::string_view table_name, MetadataExportTrigger trigger) const =0`
- Source: `include/metadata/imetadata_export_policy.h`:93
- Brief: Return the delay to apply before exporting table_name.
- Parameters:
  - `table_name` (std::string_view): The name of the table/collection.
  - `trigger` (MetadataExportTrigger): The event that triggered the export request.
- Return: Delay in milliseconds (0 = immediate).
- Details: A non-zero delay lets the exporter batch several changes that arrive within the window. Zero means export immediately. table_name The name of the table/collection. trigger The event that triggered the export request. Delay in milliseconds (0 = immediate).

#### `bool shouldExport(std::string_view table_name, MetadataExportTrigger trigger) const =0`
- Source: `include/metadata/imetadata_export_policy.h`:80
- Brief: Return true if table_name should be exported on trigger.
- Parameters:
  - `table_name` (std::string_view): The name of the table/collection.
  - `trigger` (MetadataExportTrigger): The event that would trigger the export.
- Return: true if the export should proceed; false to suppress it.
- Details: table_name The name of the table/collection. trigger The event that would trigger the export. true if the export should proceed; false to suppress it.

#### `~IMetadataExportPolicy()=default`
- Source: `include/metadata/imetadata_export_policy.h`:71
- Brief: n/a
- Parameters: none

### themis::metadata::IMetadataSecurityProvider

#### `void assertPermission(std::string_view principal, MetadataOperation op, std::string_view resource) const =0`
- Source: `include/metadata/imetadata_security_provider.h`:123
- Brief: Assert that principal may perform op on resource.
- Parameters:
  - `principal` (std::string_view): n/a
  - `op` (MetadataOperation): n/a
  - `resource` (std::string_view): n/a
- Throws:
  - MetadataAccessDeniedException: if permission is denied.
- Details: MetadataAccessDeniedException if permission is denied.

#### `bool hasPermission(std::string_view principal, MetadataOperation op, std::string_view resource) const =0`
- Source: `include/metadata/imetadata_security_provider.h`:114
- Brief: Return true if principal may perform op on resource.
- Parameters:
  - `principal` (std::string_view): Identity string (user, service account, role, …).
  - `op` (MetadataOperation): The requested metadata operation.
  - `resource` (std::string_view): The affected resource name, or "*" for any resource.
- Return: true if the operation is permitted; false otherwise.
- Details: principal Identity string (user, service account, role, …). op The requested metadata operation. resource The affected resource name, or "*" for any resource. true if the operation is permitted; false otherwise.

#### `~IMetadataSecurityProvider()=default`
- Source: `include/metadata/imetadata_security_provider.h`:104
- Brief: n/a
- Parameters: none

### themis::metadata::IMetadataSnapshotStore

#### `std::vector< std::string > listSnapshotIds()=0`
- Source: `include/metadata/metadata_snapshot.h`:172
- Brief: Return a sorted list of all stored snapshot IDs.
- Parameters: none

#### `std::optional< MetadataSnapshot > load(std::string_view id)=0`
- Source: `include/metadata/metadata_snapshot.h`:167
- Brief: Load the snapshot identified by id.
- Parameters:
  - `id` (std::string_view): Snapshot ID.
- Return: The loaded snapshot, or std::nullopt if id is not found.
- Details: id Snapshot ID. The loaded snapshot, or std::nullopt if id is not found.

#### `bool remove(std::string_view id)=0`
- Source: `include/metadata/metadata_snapshot.h`:180
- Brief: Remove the snapshot identified by id.
- Parameters:
  - `id` (std::string_view): n/a
- Return: true if the snapshot existed and was removed; false if id was not found.
- Details: true if the snapshot existed and was removed; false if id was not found.

#### `std::string save(const MetadataSnapshot &snapshot)=0`
- Source: `include/metadata/metadata_snapshot.h`:159
- Brief: Persist snapshot and return its ID.
- Parameters:
  - `snapshot` (const MetadataSnapshot &): The snapshot to store. snapshot.snapshot_id must be non-empty.
- Return: The snapshot ID (same as snapshot.snapshot_id).
- Throws:
  - MetadataSnapshotException: if snapshot_id is empty or on I/O error.
- Details: snapshot The snapshot to store. snapshot.snapshot_id must be non-empty. The snapshot ID (same as snapshot.snapshot_id). MetadataSnapshotException if snapshot_id is empty or on I/O error.

#### `size_t size()=0`
- Source: `include/metadata/metadata_snapshot.h`:185
- Brief: Return the number of snapshots currently held in the store.
- Parameters: none

#### `~IMetadataSnapshotStore()=default`
- Source: `include/metadata/metadata_snapshot.h`:148
- Brief: n/a
- Parameters: none

### themis::metadata::InMemoryMetadataSnapshotStore

#### `void clear()`
- Source: `include/metadata/metadata_snapshot.h`:288
- Brief: Remove all snapshots from the store.
- Parameters: none
- Details: Useful for resetting state between unit-test cases.

#### `std::vector< std::string > listSnapshotIds() override`
- Source: `include/metadata/metadata_snapshot.h`:253
- Brief: Return all snapshot IDs in ascending lexicographic order.
- Parameters: none

#### `std::optional< MetadataSnapshot > load(std::string_view id) override`
- Source: `include/metadata/metadata_snapshot.h`:241
- Brief: Load snapshot by ID.
- Parameters:
  - `id` (std::string_view): n/a
- Return: The snapshot, or std::nullopt if not found.
- Details: The snapshot, or std::nullopt if not found.

#### `bool remove(std::string_view id) override`
- Source: `include/metadata/metadata_snapshot.h`:270
- Brief: Remove the snapshot with the given ID.
- Parameters:
  - `id` (std::string_view): n/a
- Return: true if the snapshot was found and removed; false otherwise.
- Details: true if the snapshot was found and removed; false otherwise.

#### `std::string save(const MetadataSnapshot &snapshot) override`
- Source: `include/metadata/metadata_snapshot.h`:226
- Brief: Store snapshot.
- Parameters:
  - `snapshot` (const MetadataSnapshot &): n/a
- Throws:
  - MetadataSnapshotException: if snapshot.snapshot_id is empty.
- Details: If a snapshot with the same ID already exists it is overwritten. MetadataSnapshotException if snapshot.snapshot_id is empty.

#### `size_t size() override`
- Source: `include/metadata/metadata_snapshot.h`:276
- Brief: n/a
- Parameters: none
- Return: Number of snapshots currently held.
- Details: Number of snapshots currently held.

### themis::metadata::InMemoryRbacMetadataSecurityProvider

#### `void assertPermission(std::string_view principal, MetadataOperation op, std::string_view resource) const override`
- Source: `include/metadata/imetadata_security_provider.h`:225
- Brief: Assert that principal may perform op on resource.
- Parameters:
  - `principal` (std::string_view): n/a
  - `op` (MetadataOperation): n/a
  - `resource` (std::string_view): n/a
- Throws:
  - MetadataAccessDeniedException: if permission is denied.
- Details: MetadataAccessDeniedException if permission is denied.

#### `void grant(std::string_view principal, MetadataOperation op, std::string_view resource)`
- Source: `include/metadata/imetadata_security_provider.h`:181
- Brief: Grant principal permission to execute op on resource.
- Parameters:
  - `principal` (std::string_view): Identity string.
  - `op` (MetadataOperation): The operation to permit.
  - `resource` (std::string_view): Resource name, or "*" for all resources.
- Details: principal Identity string. op The operation to permit. resource Resource name, or "*" for all resources.

#### `bool hasPermission(std::string_view principal, MetadataOperation op, std::string_view resource) const override`
- Source: `include/metadata/imetadata_security_provider.h`:218
- Brief: Return true if principal may perform op on resource.
- Parameters:
  - `principal` (std::string_view): Identity string (user, service account, role, …).
  - `op` (MetadataOperation): The requested metadata operation.
  - `resource` (std::string_view): The affected resource name, or "*" for any resource.
- Return: true if the operation is permitted; false otherwise.
- Details: principal Identity string (user, service account, role, …). op The requested metadata operation. resource The affected resource name, or "*" for any resource. true if the operation is permitted; false otherwise.

#### `bool hasPermission_(std::string_view principal, MetadataOperation op, std::string_view resource) const`
- Source: `include/metadata/imetadata_security_provider.h`:236
- Brief: n/a
- Parameters:
  - `principal` (std::string_view): n/a
  - `op` (MetadataOperation): n/a
  - `resource` (std::string_view): n/a

#### `void revoke(std::string_view principal, MetadataOperation op, std::string_view resource)`
- Source: `include/metadata/imetadata_security_provider.h`:193
- Brief: Revoke a previously granted permission.
- Parameters:
  - `principal` (std::string_view): n/a
  - `op` (MetadataOperation): n/a
  - `resource` (std::string_view): n/a
- Details: No-op if the (principal, op, resource) triple was never granted.

#### `void revokeAll(std::string_view principal)`
- Source: `include/metadata/imetadata_security_provider.h`:211
- Brief: Remove all permissions for principal.
- Parameters:
  - `principal` (std::string_view): n/a

### themis::metadata::IndexDiff

#### `json toJSON() const`
- Source: `include/metadata/schema_diff.h`:113
- Brief: Serialise this diff to a JSON object.
- Parameters: none
- Details: Schema: { "diff_type":"ADDED"\|"REMOVED"\|"CHANGED", "index_name":"<name>" }

### themis::metadata::IndexRecommendation

#### `json toJSON() const`
- Source: `include/metadata/index_recommender.h`:64
- Brief: n/a
- Parameters: none

### themis::metadata::IndexRecommender

#### `IndexRecommender()=default`
- Source: `include/metadata/index_recommender.h`:99
- Brief: Default constructor — in-memory only (no persistence).
- Parameters: none

#### `IndexRecommender(RocksDBWrapper *db, std::chrono::milliseconds persist_interval=std::chrono::seconds(300))`
- Source: `include/metadata/index_recommender.h`:108
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper *): RocksDB instance for loading and persisting access stats. Pass nullptr for in-memory-only mode (same as default ctor). The pointed-to instance MUST outlive this IndexRecommender.
  - `persist_interval` (std::chrono::milliseconds): Background thread flush interval. Defaults to 5 minutes. Pass 0 to disable the background thread (stats are still flushed on destruction and reset()).
- Details: Persistence-enabled constructor. db RocksDB instance for loading and persisting access stats. Pass nullptr for in-memory-only mode (same as default ctor). The pointed-to instance MUST outlive this IndexRecommender. persist_interval Background thread flush interval. Defaults to 5 minutes. Pass 0 to disable the background thread (stats are still flushed on destruction and reset()).

#### `IndexRecommender(const IndexRecommender &)=delete`
- Source: `include/metadata/index_recommender.h`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IndexRecommender &): n/a

#### `double computeBenefit(const ColumnAccess &ca) const`
- Source: `include/metadata/index_recommender.h`:187
- Brief: Compute the benefit score for a ColumnAccess record.
- Parameters:
  - `ca` (const ColumnAccess &): n/a

#### `double computeCostModelBenefit(const ColumnAccess &ca, const TableStats &tbl_stats) const`
- Source: `include/metadata/index_recommender.h`:192
- Brief: n/a
- Parameters:
  - `ca` (const ColumnAccess &): n/a
  - `tbl_stats` (const TableStats &): n/a
- Details: Compute the benefit score using StatisticsCollector data (cost-model). Uses StatisticsCollector column selectivity for a more accurate estimate and applies a write-amplification penalty based on table row count.

#### `std::vector< ColumnAccess > getAccessStats(std::string_view table_name) const`
- Source: `include/metadata/index_recommender.h`:154
- Brief: Return the raw access statistics for a table.
- Parameters:
  - `table_name` (std::string_view): n/a

#### `void loadStats()`
- Source: `include/metadata/index_recommender.h`:195
- Brief: Load access stats from RocksDB into stats_ (called once in constructor).
- Parameters: none
- Details: Load Stats. Calls: start_key(), back(), lock(), iterateRange(), size(), table_name(), substr(), std::stoull().

#### `IndexRecommender & operator=(const IndexRecommender &)=delete`
- Source: `include/metadata/index_recommender.h`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IndexRecommender &): n/a

#### `void persistLoop_()`
- Source: `include/metadata/index_recommender.h`:198
- Brief: Background persist loop — wakes every persist_interval_ and calls persistStats().
- Parameters: none
- Details: Persist Loop. Calls: load(), lk(), wait_for(), persistStats().

#### `void persistStats()`
- Source: `include/metadata/index_recommender.h`:164
- Brief: Persist Stats.
- Parameters: none
- Details: Flush all in-memory access stats to RocksDB immediately. No-op when no RocksDB instance was provided at construction. Calls: lock(), load(), json::array(), push_back(), toJSON(), dump(), put(), spdlog::warn().

#### `std::vector< IndexRecommendation > recommend(std::string_view table_name, const std::vector< std::string > &existing_indexes={}) const`
- Source: `include/metadata/index_recommender.h`:143
- Brief: n/a
- Parameters:
  - `table_name` (std::string_view): Table to analyse
  - `existing_indexes` (const std::vector< std::string > &): Names of columns that already have an index
- Return: Sorted vector of recommendations (highest benefit first)
- Details: Generate recommendations for a specific table. table_name Table to analyse existing_indexes Names of columns that already have an index Sorted vector of recommendations (highest benefit first)

#### `std::map< std::string, std::vector< IndexRecommendation > > recommendAll(const std::map< std::string, std::vector< std::string > > &existing_indexes={}) const`
- Source: `include/metadata/index_recommender.h`:149
- Brief: Generate recommendations for all tracked tables.
- Parameters:
  - `existing_indexes` (const std::map< std::string, std::vector< std::string > > &): n/a

#### `void recordAccess(std::string_view table_name, std::string_view column_name, AccessType access_type, double selectivity=1.0)`
- Source: `include/metadata/index_recommender.h`:129
- Brief: Record Access.
- Parameters:
  - `table_name` (std::string_view): Name of the table.
  - `column_name` (std::string_view): Name of the column.
  - `access_type` (AccessType): Input parameter.
  - `selectivity` (double): Input parameter.
- Details: Record that a column was accessed in a query. table_name Table the column belongs to column_name Column that was accessed access_type Whether the column was used as a filter or sort key selectivity Fraction of rows that matched the predicate (0 = very selective) table_name Name of the table. column_name Name of the column. access_type Input parameter. selectivity Input parameter.

#### `void recordQuery()`
- Source: `include/metadata/index_recommender.h`:137
- Brief: Increment the total query counter (used for normalising benefit scores).
- Parameters: none
- Details: Record Query. Implements recordQuery without additional internal calls.

#### `void reset()`
- Source: `include/metadata/index_recommender.h`:157
- Brief: Reset all tracked access statistics.
- Parameters: none
- Details: Reset the modification detection flag. Calls: lock(), push_back(), clear(), store(), del(), spdlog::debug(), size().

#### `void setMetricsCollector(observability::MetricsCollector *metrics)`
- Source: `include/metadata/index_recommender.h`:179
- Brief: Set Metrics Collector.
- Parameters:
  - `metrics` (observability::MetricsCollector *): Input/output parameter.
- Details: Attach a MetricsCollector for emitting recommendation telemetry. When set, each call to recommend() increments the counter metadata.index_recommendation.generated_total labelled with the table name. Pass nullptr to stop emitting metrics. The pointed-to instance MUST outlive this IndexRecommender. metrics Input/output parameter. Implements setMetricsCollector without additional internal calls.

#### `void setStatisticsCollector(StatisticsCollector *collector)`
- Source: `include/metadata/index_recommender.h`:172
- Brief: Set Statistics Collector.
- Parameters:
  - `collector` (StatisticsCollector *): Input/output parameter.
- Details: Attach a StatisticsCollector to enable cost-model benefit scoring. When set, recommend() uses StatisticsCollector cardinality and selectivity data together with a write-amplification penalty to produce more accurate benefit scores than the simple heuristic model. Pass nullptr to revert to the heuristic model. The pointed-to instance MUST outlive this IndexRecommender. collector Input/output parameter. Implements setStatisticsCollector without additional internal calls.

#### `json toJSON() const`
- Source: `include/metadata/index_recommender.h`:160
- Brief: Serialise all access stats to JSON.
- Parameters: none

#### `~IndexRecommender()`
- Source: `include/metadata/index_recommender.h`:114
- Brief: Destructor — stops the background persist thread and flushes stats to RocksDB.
- Parameters: none

### themis::metadata::MetadataAccessDeniedException

#### `MetadataAccessDeniedException(std::string_view principal, MetadataOperation op, std::string_view resource)`
- Source: `include/metadata/imetadata_security_provider.h`:74
- Brief: n/a
- Parameters:
  - `principal` (std::string_view): n/a
  - `op` (MetadataOperation): n/a
  - `resource` (std::string_view): n/a

#### `MetadataOperation operation() const noexcept`
- Source: `include/metadata/imetadata_security_provider.h`:86
- Brief: n/a
- Parameters: none

#### `const std::string & principal() const noexcept`
- Source: `include/metadata/imetadata_security_provider.h`:85
- Brief: n/a
- Parameters: none

#### `const std::string & resource() const noexcept`
- Source: `include/metadata/imetadata_security_provider.h`:87
- Brief: n/a
- Parameters: none

### themis::metadata::MetadataChangeEvent

#### `json toJSON() const`
- Source: `include/metadata/imetadata_change_listener.h`:75
- Brief: n/a
- Parameters: none

### themis::metadata::MetadataEncryptionException

#### `MetadataEncryptionException(std::string_view field_name, std::string_view reason)`
- Source: `include/metadata/imetadata_encryption_provider.h`:45
- Brief: n/a
- Parameters:
  - `field_name` (std::string_view): n/a
  - `reason` (std::string_view): n/a

#### `const std::string & fieldName() const noexcept`
- Source: `include/metadata/imetadata_encryption_provider.h`:54
- Brief: The metadata field that triggered the exception.
- Parameters: none

#### `const std::string & reason() const noexcept`
- Source: `include/metadata/imetadata_encryption_provider.h`:57
- Brief: Human-readable description of the failure.
- Parameters: none

### themis::metadata::MetadataSnapshot

#### `const SchemaManager::TableSchema * findTable(std::string_view name) const`
- Source: `include/metadata/metadata_snapshot.h`:91
- Brief: Find a table schema by name.
- Parameters:
  - `name` (std::string_view): Table name to look up (case-sensitive).
- Return: Pointer to the matching TableSchema, or nullptr if not found.
- Details: name Table name to look up (case-sensitive). Pointer to the matching TableSchema, or nullptr if not found.

#### `size_t tableCount() const noexcept`
- Source: `include/metadata/metadata_snapshot.h`:101
- Brief: n/a
- Parameters: none
- Return: Number of tables captured in this snapshot.
- Details: Number of tables captured in this snapshot.

#### `json toJSON() const`
- Source: `include/metadata/metadata_snapshot.h`:118
- Brief: Serialise the snapshot to JSON.
- Parameters: none
- Details: Schema: { "snapshot_id":"<id>", "created_at":"<ISO-8601>", "author":"<string>", "description":"<string>", "table_count":<n>, "tables":[<TableSchemaJSON>,...] }

### themis::metadata::MetadataSnapshotException

#### `MetadataSnapshotException(std::string_view snapshot_id, std::string_view reason)`
- Source: `include/metadata/metadata_snapshot.h`:38
- Brief: n/a
- Parameters:
  - `snapshot_id` (std::string_view): n/a
  - `reason` (std::string_view): n/a

#### `const std::string & reason() const noexcept`
- Source: `include/metadata/metadata_snapshot.h`:51
- Brief: Human-readable description of the failure.
- Parameters: none

#### `const std::string & snapshotId() const noexcept`
- Source: `include/metadata/metadata_snapshot.h`:48
- Brief: The snapshot ID involved in the failed operation.
- Parameters: none

### themis::metadata::NeverExportPolicy

#### `std::chrono::milliseconds exportDelay(std::string_view, MetadataExportTrigger) const override`
- Source: `include/metadata/imetadata_export_policy.h`:132
- Brief: Return the delay to apply before exporting table_name.
- Parameters:
  - `table_name` (std::string_view): The name of the table/collection.
  - `trigger` (MetadataExportTrigger): The event that triggered the export request.
- Return: Delay in milliseconds (0 = immediate).
- Details: A non-zero delay lets the exporter batch several changes that arrive within the window. Zero means export immediately. table_name The name of the table/collection. trigger The event that triggered the export request. Delay in milliseconds (0 = immediate).

#### `bool shouldExport(std::string_view, MetadataExportTrigger) const override`
- Source: `include/metadata/imetadata_export_policy.h`:127
- Brief: Return true if table_name should be exported on trigger.
- Parameters:
  - `table_name` (std::string_view): The name of the table/collection.
  - `trigger` (MetadataExportTrigger): The event that would trigger the export.
- Return: true if the export should proceed; false to suppress it.
- Details: table_name The name of the table/collection. trigger The event that would trigger the export. true if the export should proceed; false to suppress it.

### themis::metadata::NoOpMetadataEncryptionProvider

#### `MetadataEncryptionAlgorithm algorithm() const override`
- Source: `include/metadata/imetadata_encryption_provider.h`:138
- Brief: Returns the algorithm identifier for this provider.
- Parameters: none

#### `std::string decrypt(std::string_view, std::string_view cipher_text) const override`
- Source: `include/metadata/imetadata_encryption_provider.h`:133
- Brief: Decrypt cipher_text for the field identified by field_name.
- Parameters:
  - `field_name` (std::string_view): Metadata field name.
  - `cipher_text` (std::string_view): Cipher-text previously produced by encrypt().
- Return: Recovered plain-text value.
- Throws:
  - MetadataEncryptionException: on key / configuration errors or if the cipher-text is malformed.
- Details: field_name Metadata field name. cipher_text Cipher-text previously produced by encrypt(). Recovered plain-text value. MetadataEncryptionException on key / configuration errors or if the cipher-text is malformed.

#### `std::string encrypt(std::string_view, std::string_view value) const override`
- Source: `include/metadata/imetadata_encryption_provider.h`:128
- Brief: Encrypt value for the field identified by field_name.
- Parameters:
  - `field_name` (std::string_view): Metadata field name; used for key-derivation or AAD.
  - `value` (std::string_view): Plain-text value to encrypt.
- Return: Cipher-text representation (algorithm-specific encoding).
- Throws:
  - MetadataEncryptionException: on key / configuration errors.
- Details: field_name Metadata field name; used for key-derivation or AAD. value Plain-text value to encrypt. Cipher-text representation (algorithm-specific encoding). MetadataEncryptionException on key / configuration errors.

#### `bool shouldEncrypt(std::string_view) const override`
- Source: `include/metadata/imetadata_encryption_provider.h`:124
- Brief: Returns true if the named field should be encrypted.
- Parameters:
  - `field_name` (std::string_view): Metadata field name (e.g. "connection_string").
- Return: true if this provider will encrypt values for that field.
- Details: field_name Metadata field name (e.g. "connection_string"). true if this provider will encrypt values for that field.

### themis::metadata::NoOpMetadataSecurityProvider

#### `void assertPermission(std::string_view, MetadataOperation, std::string_view) const override`
- Source: `include/metadata/imetadata_security_provider.h`:145
- Brief: Assert that principal may perform op on resource.
- Parameters:
  - `principal` (std::string_view): n/a
  - `op` (MetadataOperation): n/a
  - `resource` (std::string_view): n/a
- Throws:
  - MetadataAccessDeniedException: if permission is denied.
- Details: MetadataAccessDeniedException if permission is denied.

#### `bool hasPermission(std::string_view, MetadataOperation, std::string_view) const override`
- Source: `include/metadata/imetadata_security_provider.h`:139
- Brief: Return true if principal may perform op on resource.
- Parameters:
  - `principal` (std::string_view): Identity string (user, service account, role, …).
  - `op` (MetadataOperation): The requested metadata operation.
  - `resource` (std::string_view): The affected resource name, or "*" for any resource.
- Return: true if the operation is permitted; false otherwise.
- Details: principal Identity string (user, service account, role, …). op The requested metadata operation. resource The affected resource name, or "*" for any resource. true if the operation is permitted; false otherwise.

### themis::metadata::RecordingMetadataChangeListener

#### `RecordingMetadataChangeListener(EventCallback cb={})`
- Source: `include/metadata/imetadata_change_listener.h`:135
- Brief: n/a
- Parameters:
  - `cb` (EventCallback): n/a

#### `void clear()`
- Source: `include/metadata/imetadata_change_listener.h`:182
- Brief: Clear all recorded events.
- Parameters: none

#### `std::size_t eventCount() const`
- Source: `include/metadata/imetadata_change_listener.h`:163
- Brief: Return the number of events recorded so far.
- Parameters: none

#### `std::vector< MetadataChangeEvent > events() const`
- Source: `include/metadata/imetadata_change_listener.h`:155
- Brief: Return a snapshot of all recorded events, oldest first.
- Parameters: none

#### `std::optional< MetadataChangeEvent > lastEvent() const`
- Source: `include/metadata/imetadata_change_listener.h`:171
- Brief: Return the most recently recorded event, or nullopt if none.
- Parameters: none

#### `void onMetadataChanged(const MetadataChangeEvent &event) override`
- Source: `include/metadata/imetadata_change_listener.h`:140
- Brief: Called when a metadata change event occurs.
- Parameters:
  - `event` (const MetadataChangeEvent &): Details of the change.
- Details: event Details of the change. Implementations should return quickly to avoid blocking the dispatcher. Heavy processing should be queued asynchronously.

#### `void setCallback(EventCallback cb)`
- Source: `include/metadata/imetadata_change_listener.h`:192
- Brief: Replace the event callback.
- Parameters:
  - `cb` (EventCallback): n/a
- Details: Thread-safe; the new callback takes effect for the next event.

### themis::metadata::SchemaDiff

#### `size_t addedColumnCount() const`
- Source: `include/metadata/schema_diff.h`:142
- Brief: n/a
- Parameters: none
- Return: Number of columns added in the new schema.
- Details: Number of columns added in the new schema.

#### `bool isEmpty() const`
- Source: `include/metadata/schema_diff.h`:137
- Brief: n/a
- Parameters: none
- Return: true if no column or index changes were found.
- Details: true if no column or index changes were found.

#### `size_t modifiedColumnCount() const`
- Source: `include/metadata/schema_diff.h`:163
- Brief: n/a
- Parameters: none
- Return: Number of columns that exist in both schemas but whose attributes (type, nullability, index) have changed.
- Details: Number of columns that exist in both schemas but whose attributes (type, nullability, index) have changed.

#### `size_t removedColumnCount() const`
- Source: `include/metadata/schema_diff.h`:151
- Brief: n/a
- Parameters: none
- Return: Number of columns removed in the new schema.
- Details: Number of columns removed in the new schema.

#### `json toJSON() const`
- Source: `include/metadata/schema_diff.h`:190
- Brief: Serialise the full diff to JSON.
- Parameters: none
- Details: Schema: { "table_name":"<name>", "column_diffs":[...], "index_diffs":[...], "summary":{ "added_columns":<n>, "removed_columns":<n>, "modified_columns":<n>, "index_changes":<n> } }

### themis::metadata::SchemaDiffEngine

#### `SchemaDiff diff(const SchemaManager::TableSchema &from, const SchemaManager::TableSchema &to) const`
- Source: `include/metadata/schema_diff.h`:248
- Brief: Compute the structural diff between from and to.
- Parameters:
  - `from` (const SchemaManager::TableSchema &): The baseline (old) schema.
  - `to` (const SchemaManager::TableSchema &): The target (new) schema.
- Return: A SchemaDiff describing every detected change. Returns an empty diff when the schemas are identical.
- Details: from The baseline (old) schema. to The target (new) schema. A SchemaDiff describing every detected change. Returns an empty diff when the schemas are identical.

