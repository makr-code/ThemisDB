# TEMPORAL DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\temporal\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\temporal\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 56
- Compounds: 192
- Classes/Structs: 110
- Namespaces: 18
- File Compounds: 56

## Namespaces
- @067174174143071074047276015336113314044025003220
- @077046365076147343243171125167111371002042002040
- @110277354371037266121167020262363051121353364041
- @264113116133026351331304230031270105271341224342
- @374316132251215257225236230213215234010225216356
- @375016035317347274112245171314074365271305140147
- testing
- themis
- themis::bench
- themis::bench::temporal_dedicated
- themis::bench::trg
- themis::temporal
- themis::temporal::test
- themisdb
- themisdb::replication
- themisdb::temporal
- themisdb::temporal::@362133167050330323257277041203073060326213357033
- themisdb::temporal::detail

## Types
### Classes
- ApplicationTimeTest
- BiTemporalMergeTest
- BiTemporalMergeV18V19Test
- BiTemporalTableTest
- CDCPersistentLogTest
- ColdStoreFileSystemTest
- ColdStoreInMemoryTest
- ExecuteBiTemporalQueryTest
- ExecuteTemporalQueryTest
- IntervalTreeIndexTest
- RetentionManagerTest
- SnapshotDiffTest
- SnapshotDiffV18V19Test
- StubCdcEmitter
- StubConflictResolver
- StubTemporalStore
- SystemVersionedTableTest
- TemporalAggregationPropertyTest
- TemporalAggregationTest
- TemporalAggregatorTest
- TemporalCDCTest
- TemporalCompressorTest
- TemporalConflictDetectorTest
- TemporalConflictResolverTest
- TemporalGraphTest
- TemporalIndexTest
- TemporalMigratorTest
- TemporalQueryEngineTest
- TemporalSnapshotManagerTest
- ThreeTierQueryTest
- TierManagerTest
- themisdb::temporal::BiTemporalJoin
- themisdb::temporal::BiTemporalTable
- themisdb::temporal::BloomFilter
- themisdb::temporal::CDCPersistentLog
- themisdb::temporal::CustomMergeResolver
- themisdb::temporal::FileSystemBackend
- themisdb::temporal::IColdStoreBackend
- themisdb::temporal::InMemoryBackend
- themisdb::temporal::IntervalTreeIndex
- themisdb::temporal::LWWFieldMergeResolver
- themisdb::temporal::MergeResolver
- themisdb::temporal::QueryCache
- themisdb::temporal::RetentionManager
- themisdb::temporal::SystemVersionedTable
- themisdb::temporal::TemporalAggregator
- themisdb::temporal::TemporalCDC
- themisdb::temporal::TemporalColdStore
- themisdb::temporal::TemporalCompressor
- themisdb::temporal::TemporalConflictDetector
- themisdb::temporal::TemporalConflictResolver
- themisdb::temporal::TemporalIndex
- themisdb::temporal::TemporalMigrator
- themisdb::temporal::TemporalQueryEngine
- themisdb::temporal::TemporalSnapshotManager
- themisdb::temporal::TemporalTierManager
- themisdb::temporal::UnionMergeResolver

### Structs
- ConflictResolutionResult
- WriteResult
- themis::bench::trg::BiTemporalEntry
- themis::temporal::test::BiTemporalRow
- themisdb::temporal::AggregateResult
- themisdb::temporal::AggregationSpec
- themisdb::temporal::ArchivedRecord
- themisdb::temporal::BiTemporalJoin::Config
- themisdb::temporal::BiTemporalJoinResult
- themisdb::temporal::BiTemporalRow
- themisdb::temporal::BiTemporalTable::MergeResult
- themisdb::temporal::ChangeEvent
- themisdb::temporal::ColdStoreStats
- themisdb::temporal::ColumnInfo
- themisdb::temporal::CompressionConfig
- themisdb::temporal::CompressionStats
- themisdb::temporal::Conflict
- themisdb::temporal::ConflictRecord
- themisdb::temporal::IntervalEntry
- themisdb::temporal::IntervalTreeIndex::Node
- themisdb::temporal::IntervalTreeStats
- themisdb::temporal::MigrationPlan
- themisdb::temporal::MigrationReport
- themisdb::temporal::MigrationStats
- themisdb::temporal::QueryCache::CacheKey
- themisdb::temporal::QueryCache::CacheKeyHash
- themisdb::temporal::QueryCache::Entry
- themisdb::temporal::RetentionManager::ScheduledTable
- themisdb::temporal::RetentionPolicy
- themisdb::temporal::RetentionRule
- themisdb::temporal::RetentionStats
- themisdb::temporal::RowFilter
- themisdb::temporal::SnapshotDiff
- themisdb::temporal::SnapshotHandle
- themisdb::temporal::SnapshotMetadata
- themisdb::temporal::SystemVersionedTable::Config
- themisdb::temporal::TemporalCDC::Subscription
- themisdb::temporal::TemporalForeignKey
- themisdb::temporal::TemporalIndexEntry
- themisdb::temporal::TemporalIndexStats
- themisdb::temporal::TemporalQuerySpec
- themisdb::temporal::TemporalSnapshot
- themisdb::temporal::TemporalSnapshotManager::SnapshotData
- themisdb::temporal::TemporalSnapshotManager::SnapshotDiff
- themisdb::temporal::TemporalTierManager::KeyTierStats
- themisdb::temporal::TemporalTierManager::TableTierStats
- themisdb::temporal::TierDecisionContext
- themisdb::temporal::TierPolicy
- themisdb::temporal::TimeRange
- themisdb::temporal::TrendResult
- themisdb::temporal::ValidationResult
- themisdb::temporal::VersionBlock
- themisdb::temporal::VersionedDocument

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 1081

### ApplicationTimeTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:371
- Brief: n/a
- Parameters: none

### CDCPersistentLogTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_cdc.cpp`:264
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/temporal/test_temporal_cdc.cpp`:267
- Brief: n/a
- Parameters: none

#### `ChangeEvent makeEvent(const std::string &table, const std::string &key)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:271
- Brief: n/a
- Parameters:
  - `table` (const std::string &): n/a
  - `key` (const std::string &): n/a

### ColdStoreFileSystemTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:155
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:162
- Brief: n/a
- Parameters: none

### ExecuteBiTemporalQueryTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:643
- Brief: n/a
- Parameters: none

### ExecuteTemporalQueryTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:554
- Brief: n/a
- Parameters: none

### IntervalTreeIndexTest

#### `IntervalEntry makeEntry(const std::string &key, Timestamp start, Timestamp end, nlohmann::json payload={})`
- Source: `tests/temporal/test_interval_tree_index.cpp`:19
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `start` (Timestamp): n/a
  - `end` (Timestamp): n/a
  - `payload` (nlohmann::json): n/a

### RetentionManagerTest

#### `void populateHistory(SystemVersionedTable &t, size_t n_updates)`
- Source: `tests/temporal/test_retention_manager.cpp`:18
- Brief: n/a
- Parameters:
  - `t` (SystemVersionedTable &): n/a
  - `n_updates` (size_t): n/a

### SnapshotDiffTest

#### `SystemVersionedTable buildTable(const std::string &name, std::vector< std::pair< std::string, int > > kv)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:404
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `kv` (std::vector< std::pair< std::string, int > >): n/a

### SnapshotDiffV18V19Test

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:190
- Brief: n/a
- Parameters: none

### StubCdcEmitter

#### `bool emit(uint64_t entity_id, uint64_t ts_ms, const std::string &change_type) noexcept`
- Source: `tests/temporal/test_temporal_highcardinality_stress.cpp`:79
- Brief: n/a
- Parameters:
  - `entity_id` (uint64_t): n/a
  - `ts_ms` (uint64_t): n/a
  - `change_type` (const std::string &): n/a

#### `uint64_t totalEvents() const noexcept`
- Source: `tests/temporal/test_temporal_highcardinality_stress.cpp`:84
- Brief: n/a
- Parameters: none

### StubConflictResolver

#### `ConflictResolutionResult resolve(uint64_t ver_a, uint64_t ver_b) noexcept`
- Source: `tests/temporal/test_temporal_highcardinality_stress.cpp`:67
- Brief: n/a
- Parameters:
  - `ver_a` (uint64_t): n/a
  - `ver_b` (uint64_t): n/a

#### `uint64_t totalOps() const noexcept`
- Source: `tests/temporal/test_temporal_highcardinality_stress.cpp`:72
- Brief: n/a
- Parameters: none

### StubTemporalStore

#### `uint64_t totalWrites() const noexcept`
- Source: `tests/temporal/test_temporal_highcardinality_stress.cpp`:54
- Brief: n/a
- Parameters: none

#### `WriteResult write(uint64_t entity_id, uint64_t ts_ms, const std::string &) noexcept`
- Source: `tests/temporal/test_temporal_highcardinality_stress.cpp`:48
- Brief: n/a
- Parameters:
  - `entity_id` (uint64_t): n/a
  - `ts_ms` (uint64_t): n/a
  - `<unnamed>` (const std::string &): n/a

### TemporalAggregationPropertyTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_aggregation_property.cpp`:11
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/temporal/test_temporal_aggregation_property.cpp`:28
- Brief: n/a
- Parameters: none

#### `void createTemporalTestGraph()`
- Source: `tests/temporal/test_temporal_aggregation_property.cpp`:35
- Brief: n/a
- Parameters: none

### TemporalAggregationTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_aggregation.cpp`:12
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/temporal/test_temporal_aggregation.cpp`:33
- Brief: n/a
- Parameters: none

#### `void createTemporalTestGraph()`
- Source: `tests/temporal/test_temporal_aggregation.cpp`:40
- Brief: n/a
- Parameters: none

### TemporalCDCTest

#### `ChangeEvent makeEvent(const std::string &table, ChangeType type, const std::string &entity_id, Timestamp ts=1000)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:18
- Brief: n/a
- Parameters:
  - `table` (const std::string &): n/a
  - `type` (ChangeType): n/a
  - `entity_id` (const std::string &): n/a
  - `ts` (Timestamp): n/a

### TemporalConflictDetectorTest

#### `TemporalSnapshot makeSnap(const std::string &id, uint64_t physical, uint32_t logical, const std::string &node_id, nlohmann::json data)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:471
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `physical` (uint64_t): n/a
  - `logical` (uint32_t): n/a
  - `node_id` (const std::string &): n/a
  - `data` (nlohmann::json): n/a

### TemporalConflictResolverTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:16
- Brief: n/a
- Parameters: none

#### `TemporalSnapshot createSnapshot(const std::string &id, uint64_t physical, uint32_t logical, const std::string &node_id, const std::string &value)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:20
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `physical` (uint64_t): n/a
  - `logical` (uint32_t): n/a
  - `node_id` (const std::string &): n/a
  - `value` (const std::string &): n/a

### TemporalGraphTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_graph.cpp`:13
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/temporal/test_temporal_graph.cpp`:38
- Brief: n/a
- Parameters: none

#### `themis::BaseEntity createTemporalEdge(const std::string &id, const std::string &from, const std::string &to, std::optional< int64_t > valid_from=std::nullopt, std::optional< int64_t > valid_to=std::nullopt, double weight=1.0)`
- Source: `tests/temporal/test_temporal_graph.cpp`:55
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `from` (const std::string &): n/a
  - `to` (const std::string &): n/a
  - `valid_from` (std::optional< int64_t >): n/a
  - `valid_to` (std::optional< int64_t >): n/a
  - `weight` (double): n/a

#### `int64_t toTimestamp(int year, int month, int day)`
- Source: `tests/temporal/test_temporal_graph.cpp`:45
- Brief: n/a
- Parameters:
  - `year` (int): n/a
  - `month` (int): n/a
  - `day` (int): n/a

### TemporalIndexTest

#### `TemporalIndexEntry makeEntry(const std::string &key, Timestamp start, Timestamp end, nlohmann::json payload={})`
- Source: `tests/temporal/test_temporal_index.cpp`:17
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `start` (Timestamp): n/a
  - `end` (Timestamp): n/a
  - `payload` (nlohmann::json): n/a

### TemporalMigratorTest

#### `std::vector< VersionedDocument > makeHistory(const std::string &key, int versions)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:48
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `versions` (int): n/a

#### `std::unordered_map< std::string, Document > makeSourceDocs(int n=5, const std::string &prefix="emp")`
- Source: `tests/temporal/test_temporal_migrator.cpp`:32
- Brief: n/a
- Parameters:
  - `n` (int): n/a
  - `prefix` (const std::string &): n/a

### TemporalQueryEngineTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:16
- Brief: n/a
- Parameters: none

### TemporalSnapshotManagerTest

#### `void advanceTime(Timestamp ms)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:24
- Brief: n/a
- Parameters:
  - `ms` (Timestamp): n/a

#### `void populateTable(SystemVersionedTable &t)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:26
- Brief: n/a
- Parameters:
  - `t` (SystemVersionedTable &): n/a

### ThreeTierQueryTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:348
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:364
- Brief: n/a
- Parameters: none

### TierManagerTest

#### `void SetUp() override`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:164
- Brief: n/a
- Parameters: none

### bench_interval_tree_erase.cpp

#### `BENCHMARK(BM_ITEB01_SingleKeyErase) -> RangeMultiplier(10) ->Range(100, 10000)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ITEB01_SingleKeyErase): n/a

#### `BENCHMARK(BM_ITEB02_BulkErase) -> RangeMultiplier(10) ->Range(100, 10000)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ITEB02_BulkErase): n/a

#### `BENCHMARK(BM_ITEB03_RebuildBaseline) -> RangeMultiplier(10) ->Range(100, 10000)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ITEB03_RebuildBaseline): n/a

#### `BENCHMARK(BM_ITEB04_MixedReadErase) -> Threads(1) ->Threads(4) ->Threads(8)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ITEB04_MixedReadErase): n/a

#### `BENCHMARK(BM_ITEB05_LargePayloadErase)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ITEB05_LargePayloadErase): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:233
- Brief: n/a
- Parameters: none

#### `void BM_ITEB01_SingleKeyErase(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:80
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Benchmark: erase a single, randomly chosen key from a tree pre-populated with n entries. The state setup re-inserts the erased key so each iteration starts with a full tree.

#### `void BM_ITEB02_BulkErase(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:113
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Benchmark: erase every key in the tree sequentially. Measures cumulative cost; tree is rebuilt in each iteration.

#### `void BM_ITEB03_RebuildBaseline(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:136
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Baseline benchmark: create a brand-new tree and insert n entries. Use this to compare against ITEB-02 (erase vs. rebuild cost trade-off).

#### `void BM_ITEB04_MixedReadErase(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:157
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Benchmark: 80 % reader threads / 20 % writer (erase+reinsert) threads. Validates that the shared_mutex does not cause excessive reader stalls. Each thread runs its own micro-loop; the benchmark measures throughput (ops/s) seen by the reader threads.

#### `void BM_ITEB05_LargePayloadErase(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_interval_tree_erase.cpp`:203
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Benchmark: each entry carries a ~1 KiB JSON blob. Measures the memory-move overhead incurred during erase + rebuild.

### bench_temporal_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:148
- Brief: n/a
- Parameters: none

### bench_temporal_queries.cpp

#### `Arg(1) -> Arg(100) ->Unit(benchmark::kNanosecond)`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Arg(10) -> Arg(100) ->Arg(1000) ->Unit(benchmark::kNanosecond)`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `BENCHMARK(BM_BiTemporalTable_Delete) -> Unit(benchmark::kNanosecond)`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_BiTemporalTable_Delete): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:203
- Brief: n/a
- Parameters: none

#### `void BM_BiTemporalTable_Delete(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:162
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BiTemporalTable_GetHistory(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:184
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BiTemporalTable_Insert(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:58
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BiTemporalTable_QueryBiTemporal(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:85
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BiTemporalTable_QueryCurrentByValidTime(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:112
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_BiTemporalTable_Update(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_queries.cpp`:137
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_temporal_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:324
- Brief: n/a
- Parameters: none

### test_bi_temporal.cpp

#### `TEST_F(BiTemporalMergeTest, BTM_01_MergeEmpty_NoChange)`
- Source: `tests/temporal/test_bi_temporal.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeTest): n/a
  - `<unnamed>` (BTM_01_MergeEmpty_NoChange): n/a

#### `TEST_F(BiTemporalMergeTest, BTM_02_MergeNewKey_Inserted)`
- Source: `tests/temporal/test_bi_temporal.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeTest): n/a
  - `<unnamed>` (BTM_02_MergeNewKey_Inserted): n/a

#### `TEST_F(BiTemporalMergeTest, BTM_03_MergeIdenticalRow_Skipped)`
- Source: `tests/temporal/test_bi_temporal.cpp`:392
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeTest): n/a
  - `<unnamed>` (BTM_03_MergeIdenticalRow_Skipped): n/a

#### `TEST_F(BiTemporalMergeTest, BTM_04_LWW_RemoteWins)`
- Source: `tests/temporal/test_bi_temporal.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeTest): n/a
  - `<unnamed>` (BTM_04_LWW_RemoteWins): n/a

#### `TEST_F(BiTemporalMergeTest, BTM_05_MultipleKeys)`
- Source: `tests/temporal/test_bi_temporal.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeTest): n/a
  - `<unnamed>` (BTM_05_MultipleKeys): n/a

#### `TEST_F(BiTemporalMergeTest, BTM_06_MergeIsIdempotent)`
- Source: `tests/temporal/test_bi_temporal.cpp`:427
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeTest): n/a
  - `<unnamed>` (BTM_06_MergeIsIdempotent): n/a

#### `TEST_F(BiTemporalTableTest, Delete_MatchingValidTime_ClosesRow)`
- Source: `tests/temporal/test_bi_temporal.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (Delete_MatchingValidTime_ClosesRow): n/a

#### `TEST_F(BiTemporalTableTest, Delete_NonExistentKey_ReturnsZero)`
- Source: `tests/temporal/test_bi_temporal.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (Delete_NonExistentKey_ReturnsZero): n/a

#### `TEST_F(BiTemporalTableTest, FindGaps_DeletedRowCreatesGap)`
- Source: `tests/temporal/test_bi_temporal.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindGaps_DeletedRowCreatesGap): n/a

#### `TEST_F(BiTemporalTableTest, FindGaps_FullyCovered_ReturnsEmpty)`
- Source: `tests/temporal/test_bi_temporal.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindGaps_FullyCovered_ReturnsEmpty): n/a

#### `TEST_F(BiTemporalTableTest, FindGaps_InvalidRange_ReturnsEmpty)`
- Source: `tests/temporal/test_bi_temporal.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindGaps_InvalidRange_ReturnsEmpty): n/a

#### `TEST_F(BiTemporalTableTest, FindGaps_LeadingGap)`
- Source: `tests/temporal/test_bi_temporal.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindGaps_LeadingGap): n/a

#### `TEST_F(BiTemporalTableTest, FindGaps_MiddleGap)`
- Source: `tests/temporal/test_bi_temporal.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindGaps_MiddleGap): n/a

#### `TEST_F(BiTemporalTableTest, FindGaps_MultipleGaps)`
- Source: `tests/temporal/test_bi_temporal.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindGaps_MultipleGaps): n/a

#### `TEST_F(BiTemporalTableTest, FindGaps_NoRows_ReturnsFullInterval)`
- Source: `tests/temporal/test_bi_temporal.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindGaps_NoRows_ReturnsFullInterval): n/a

#### `TEST_F(BiTemporalTableTest, FindGaps_TrailingGap)`
- Source: `tests/temporal/test_bi_temporal.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindGaps_TrailingGap): n/a

#### `TEST_F(BiTemporalTableTest, FindOverlaps_NoOverlaps_ReturnsEmpty)`
- Source: `tests/temporal/test_bi_temporal.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindOverlaps_NoOverlaps_ReturnsEmpty): n/a

#### `TEST_F(BiTemporalTableTest, FindOverlaps_WithOverlap_ReturnsConflictingPairs)`
- Source: `tests/temporal/test_bi_temporal.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (FindOverlaps_WithOverlap_ReturnsConflictingPairs): n/a

#### `TEST_F(BiTemporalTableTest, GetAllKeys_EmptyTable_ReturnsEmpty)`
- Source: `tests/temporal/test_bi_temporal.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (GetAllKeys_EmptyTable_ReturnsEmpty): n/a

#### `TEST_F(BiTemporalTableTest, GetAllKeys_IncludesDeletedKeys)`
- Source: `tests/temporal/test_bi_temporal.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (GetAllKeys_IncludesDeletedKeys): n/a

#### `TEST_F(BiTemporalTableTest, GetAllKeys_ReturnsAllKnownKeys)`
- Source: `tests/temporal/test_bi_temporal.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (GetAllKeys_ReturnsAllKnownKeys): n/a

#### `TEST_F(BiTemporalTableTest, HasUniquenessConflict_AfterDelete_ReturnsFalse)`
- Source: `tests/temporal/test_bi_temporal.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (HasUniquenessConflict_AfterDelete_ReturnsFalse): n/a

#### `TEST_F(BiTemporalTableTest, HasUniquenessConflict_ConsistentWithInsert)`
- Source: `tests/temporal/test_bi_temporal.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (HasUniquenessConflict_ConsistentWithInsert): n/a

#### `TEST_F(BiTemporalTableTest, HasUniquenessConflict_InvalidPeriod_ReturnsFalse)`
- Source: `tests/temporal/test_bi_temporal.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (HasUniquenessConflict_InvalidPeriod_ReturnsFalse): n/a

#### `TEST_F(BiTemporalTableTest, HasUniquenessConflict_NoRows_ReturnsFalse)`
- Source: `tests/temporal/test_bi_temporal.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (HasUniquenessConflict_NoRows_ReturnsFalse): n/a

#### `TEST_F(BiTemporalTableTest, HasUniquenessConflict_NonOverlapping_ReturnsFalse)`
- Source: `tests/temporal/test_bi_temporal.cpp`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (HasUniquenessConflict_NonOverlapping_ReturnsFalse): n/a

#### `TEST_F(BiTemporalTableTest, HasUniquenessConflict_Overlapping_ReturnsTrue)`
- Source: `tests/temporal/test_bi_temporal.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (HasUniquenessConflict_Overlapping_ReturnsTrue): n/a

#### `TEST_F(BiTemporalTableTest, Insert_DifferentKeys_NoConflict)`
- Source: `tests/temporal/test_bi_temporal.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (Insert_DifferentKeys_NoConflict): n/a

#### `TEST_F(BiTemporalTableTest, Insert_NoOverlap_Succeeds)`
- Source: `tests/temporal/test_bi_temporal.cpp`:20
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (Insert_NoOverlap_Succeeds): n/a

#### `TEST_F(BiTemporalTableTest, Insert_Overlap_Rejected)`
- Source: `tests/temporal/test_bi_temporal.cpp`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (Insert_Overlap_Rejected): n/a

#### `TEST_F(BiTemporalTableTest, QueryBiTemporal_ReturnsRowValidAtBothTimes)`
- Source: `tests/temporal/test_bi_temporal.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (QueryBiTemporal_ReturnsRowValidAtBothTimes): n/a

#### `TEST_F(BiTemporalTableTest, QueryBiTemporal_ValidTimeNotContained_ReturnsEmpty)`
- Source: `tests/temporal/test_bi_temporal.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (QueryBiTemporal_ValidTimeNotContained_ReturnsEmpty): n/a

#### `TEST_F(BiTemporalTableTest, ScanBiTemporal_MatchingRow_ReturnsIt)`
- Source: `tests/temporal/test_bi_temporal.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (ScanBiTemporal_MatchingRow_ReturnsIt): n/a

#### `TEST_F(BiTemporalTableTest, ScanBiTemporal_MultipleRows_ReturnsAllMatching)`
- Source: `tests/temporal/test_bi_temporal.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (ScanBiTemporal_MultipleRows_ReturnsAllMatching): n/a

#### `TEST_F(BiTemporalTableTest, ScanBiTemporal_ValidTimeNotContained_ReturnsEmpty)`
- Source: `tests/temporal/test_bi_temporal.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (ScanBiTemporal_ValidTimeNotContained_ReturnsEmpty): n/a

#### `TEST_F(BiTemporalTableTest, Statistics_CorrectCounts)`
- Source: `tests/temporal/test_bi_temporal.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (Statistics_CorrectCounts): n/a

#### `TEST_F(BiTemporalTableTest, TemporalForeignKey_ChildPeriodExceedsParent_ReturnsFalse)`
- Source: `tests/temporal/test_bi_temporal.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (TemporalForeignKey_ChildPeriodExceedsParent_ReturnsFalse): n/a

#### `TEST_F(BiTemporalTableTest, TemporalForeignKey_EmptyTableName_SkipsNameCheck)`
- Source: `tests/temporal/test_bi_temporal.cpp`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (TemporalForeignKey_EmptyTableName_SkipsNameCheck): n/a

#### `TEST_F(BiTemporalTableTest, TemporalForeignKey_ExactPeriodMatch_ReturnsTrue)`
- Source: `tests/temporal/test_bi_temporal.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (TemporalForeignKey_ExactPeriodMatch_ReturnsTrue): n/a

#### `TEST_F(BiTemporalTableTest, TemporalForeignKey_ParentKeyMissing_ReturnsFalse)`
- Source: `tests/temporal/test_bi_temporal.cpp`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (TemporalForeignKey_ParentKeyMissing_ReturnsFalse): n/a

#### `TEST_F(BiTemporalTableTest, TemporalForeignKey_ParentRowDeleted_ReturnsFalse)`
- Source: `tests/temporal/test_bi_temporal.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (TemporalForeignKey_ParentRowDeleted_ReturnsFalse): n/a

#### `TEST_F(BiTemporalTableTest, TemporalForeignKey_ValidReference_ReturnsTrue)`
- Source: `tests/temporal/test_bi_temporal.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (TemporalForeignKey_ValidReference_ReturnsTrue): n/a

#### `TEST_F(BiTemporalTableTest, TemporalForeignKey_WrongTableName_ReturnsFalse)`
- Source: `tests/temporal/test_bi_temporal.cpp`:348
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (TemporalForeignKey_WrongTableName_ReturnsFalse): n/a

#### `TEST_F(BiTemporalTableTest, Update_ExistingValidTime_CreatesNewVersion)`
- Source: `tests/temporal/test_bi_temporal.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (Update_ExistingValidTime_CreatesNewVersion): n/a

#### `TEST_F(BiTemporalTableTest, Update_NoMatchingValidTime_ReturnsFalse)`
- Source: `tests/temporal/test_bi_temporal.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTableTest): n/a
  - `<unnamed>` (Update_NoMatchingValidTime_ReturnsFalse): n/a

### test_interval_tree_index.cpp

#### `TEST(IntervalTreeEraseTest, ITX_ERASE_01_EraseExistingKey_ReturnsCount)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeEraseTest): n/a
  - `<unnamed>` (ITX_ERASE_01_EraseExistingKey_ReturnsCount): n/a

#### `TEST(IntervalTreeEraseTest, ITX_ERASE_02_EraseAbsentKey_ReturnsZero)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeEraseTest): n/a
  - `<unnamed>` (ITX_ERASE_02_EraseAbsentKey_ReturnsZero): n/a

#### `TEST(IntervalTreeEraseTest, ITX_ERASE_03_TreeRemainsQueryable_AfterErase)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:362
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeEraseTest): n/a
  - `<unnamed>` (ITX_ERASE_03_TreeRemainsQueryable_AfterErase): n/a

#### `TEST(IntervalTreeEraseTest, ITX_ERASE_04_EraseAll_TreeEmpty)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:374
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeEraseTest): n/a
  - `<unnamed>` (ITX_ERASE_04_EraseAll_TreeEmpty): n/a

#### `TEST_F(IntervalTreeIndexTest, AVL_AfterRemove_HeightStaysLogarithmic)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (AVL_AfterRemove_HeightStaysLogarithmic): n/a

#### `TEST_F(IntervalTreeIndexTest, AVL_AscendingInsert_HeightIsLogarithmic)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (AVL_AscendingInsert_HeightIsLogarithmic): n/a

#### `TEST_F(IntervalTreeIndexTest, AVL_DescendingInsert_HeightIsLogarithmic)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (AVL_DescendingInsert_HeightIsLogarithmic): n/a

#### `TEST_F(IntervalTreeIndexTest, Clear_EmptiesTree)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (Clear_EmptiesTree): n/a

#### `TEST_F(IntervalTreeIndexTest, ConcurrentReads_SharedMutex_NoDeadlock)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (ConcurrentReads_SharedMutex_NoDeadlock): n/a

#### `TEST_F(IntervalTreeIndexTest, Insert_DuplicateKey_DifferentRange_SizeIncreasesByTwo)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (Insert_DuplicateKey_DifferentRange_SizeIncreasesByTwo): n/a

#### `TEST_F(IntervalTreeIndexTest, Insert_Multiple_SizeIncreases)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (Insert_Multiple_SizeIncreases): n/a

#### `TEST_F(IntervalTreeIndexTest, Insert_Single_SizeBecomesOne)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (Insert_Single_SizeBecomesOne): n/a

#### `TEST_F(IntervalTreeIndexTest, KeyIndex_AfterClear_Empty)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (KeyIndex_AfterClear_Empty): n/a

#### `TEST_F(IntervalTreeIndexTest, KeyIndex_AfterRemoveKey_Empty)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (KeyIndex_AfterRemoveKey_Empty): n/a

#### `TEST_F(IntervalTreeIndexTest, KeyIndex_AfterRemove_EntryGone)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (KeyIndex_AfterRemove_EntryGone): n/a

#### `TEST_F(IntervalTreeIndexTest, KeyIndex_QueryKey_ReturnsCorrectCount)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (KeyIndex_QueryKey_ReturnsCorrectCount): n/a

#### `TEST_F(IntervalTreeIndexTest, KeyIndex_QueryKey_WithRange_Filtered)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (KeyIndex_QueryKey_WithRange_Filtered): n/a

#### `TEST_F(IntervalTreeIndexTest, Name_ReturnsGivenName)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (Name_ReturnsGivenName): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryKey_ReturnsAllVersions)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryKey_ReturnsAllVersions): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryKey_WithRange_FiltersVersions)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryKey_WithRange_FiltersVersions): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryOverlap_NoOverlap_ReturnsEmpty)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryOverlap_NoOverlap_ReturnsEmpty): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryOverlap_PartialOverlap)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryOverlap_PartialOverlap): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryOverlap_ReturnsAllOverlapping)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryOverlap_ReturnsAllOverlapping): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryOverlap_TouchingBoundary_ReturnsEmpty)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryOverlap_TouchingBoundary_ReturnsEmpty): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryPoint_AtStart_ReturnsEntry)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryPoint_AtStart_ReturnsEntry): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryPoint_ManyIntervals_ReturnsAllContaining)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryPoint_ManyIntervals_ReturnsAllContaining): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryPoint_MatchingEntries)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryPoint_MatchingEntries): n/a

#### `TEST_F(IntervalTreeIndexTest, QueryPoint_NoMatch_ReturnsEmpty)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (QueryPoint_NoMatch_ReturnsEmpty): n/a

#### `TEST_F(IntervalTreeIndexTest, RemoveKey_RemovesAllVersions)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (RemoveKey_RemovesAllVersions): n/a

#### `TEST_F(IntervalTreeIndexTest, Remove_ExistingEntry_SizeDecreases)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (Remove_ExistingEntry_SizeDecreases): n/a

#### `TEST_F(IntervalTreeIndexTest, Stats_TracksCounts)`
- Source: `tests/temporal/test_interval_tree_index.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndexTest): n/a
  - `<unnamed>` (Stats_TracksCounts): n/a

### test_retention_manager.cpp

#### `TEST(RetentionRuleTest, RR_01_EqualityReflexive)`
- Source: `tests/temporal/test_retention_manager.cpp`:535
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionRuleTest): n/a
  - `<unnamed>` (RR_01_EqualityReflexive): n/a

#### `TEST(RetentionRuleTest, RR_02_EqualitySymmetric)`
- Source: `tests/temporal/test_retention_manager.cpp`:540
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionRuleTest): n/a
  - `<unnamed>` (RR_02_EqualitySymmetric): n/a

#### `TEST(RetentionRuleTest, RR_03_InequalityDifferentPeriod)`
- Source: `tests/temporal/test_retention_manager.cpp`:547
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionRuleTest): n/a
  - `<unnamed>` (RR_03_InequalityDifferentPeriod): n/a

#### `TEST(RetentionRuleTest, RR_04_InequalityDifferentType)`
- Source: `tests/temporal/test_retention_manager.cpp`:553
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionRuleTest): n/a
  - `<unnamed>` (RR_04_InequalityDifferentType): n/a

#### `TEST(RetentionRuleTest, RR_05_LessThanism_TotalOrder)`
- Source: `tests/temporal/test_retention_manager.cpp`:559
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionRuleTest): n/a
  - `<unnamed>` (RR_05_LessThanism_TotalOrder): n/a

#### `TEST(RetentionRuleTest, RR_06_UsableInStdSet)`
- Source: `tests/temporal/test_retention_manager.cpp`:567
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionRuleTest): n/a
  - `<unnamed>` (RR_06_UsableInStdSet): n/a

#### `TEST_F(RetentionManagerTest, Archive_EnabledBeforeDelete_PopulatesArchive)`
- Source: `tests/temporal/test_retention_manager.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (Archive_EnabledBeforeDelete_PopulatesArchive): n/a

#### `TEST_F(RetentionManagerTest, ClearArchive_EmptiesArchive)`
- Source: `tests/temporal/test_retention_manager.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (ClearArchive_EmptiesArchive): n/a

#### `TEST_F(RetentionManagerTest, ComplianceTag_AppearsInArchivedRecords)`
- Source: `tests/temporal/test_retention_manager.cpp`:426
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (ComplianceTag_AppearsInArchivedRecords): n/a

#### `TEST_F(RetentionManagerTest, ComplianceTag_ExplicitArchiveTagTakesPrecedence)`
- Source: `tests/temporal/test_retention_manager.cpp`:451
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (ComplianceTag_ExplicitArchiveTagTakesPrecedence): n/a

#### `TEST_F(RetentionManagerTest, CumulativeStats_AccumulateAcrossRuns)`
- Source: `tests/temporal/test_retention_manager.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (CumulativeStats_AccumulateAcrossRuns): n/a

#### `TEST_F(RetentionManagerTest, EnforceCustom_KeepEvenValues)`
- Source: `tests/temporal/test_retention_manager.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (EnforceCustom_KeepEvenValues): n/a

#### `TEST_F(RetentionManagerTest, EnforceRetention_IncludesDeletedKeyHistory)`
- Source: `tests/temporal/test_retention_manager.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (EnforceRetention_IncludesDeletedKeyHistory): n/a

#### `TEST_F(RetentionManagerTest, EnforceTimeBased_ActuallyDeletesVersions)`
- Source: `tests/temporal/test_retention_manager.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (EnforceTimeBased_ActuallyDeletesVersions): n/a

#### `TEST_F(RetentionManagerTest, EnforceTimeBased_FutureRetention_DeletesNothing)`
- Source: `tests/temporal/test_retention_manager.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (EnforceTimeBased_FutureRetention_DeletesNothing): n/a

#### `TEST_F(RetentionManagerTest, EnforceTimeBased_ZeroRetention_DeletesAllHistory)`
- Source: `tests/temporal/test_retention_manager.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (EnforceTimeBased_ZeroRetention_DeletesAllHistory): n/a

#### `TEST_F(RetentionManagerTest, EnforceVersionCount_ActuallyDeletesVersions)`
- Source: `tests/temporal/test_retention_manager.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (EnforceVersionCount_ActuallyDeletesVersions): n/a

#### `TEST_F(RetentionManagerTest, EnforceVersionCount_ExcessVersionsDeleted)`
- Source: `tests/temporal/test_retention_manager.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (EnforceVersionCount_ExcessVersionsDeleted): n/a

#### `TEST_F(RetentionManagerTest, EnforceVersionCount_WithinLimit_DeletesNothing)`
- Source: `tests/temporal/test_retention_manager.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (EnforceVersionCount_WithinLimit_DeletesNothing): n/a

#### `TEST_F(RetentionManagerTest, EnforceWithoutPolicy_ReturnsError)`
- Source: `tests/temporal/test_retention_manager.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (EnforceWithoutPolicy_ReturnsError): n/a

#### `TEST_F(RetentionManagerTest, GetPolicy_NonExistent_ReturnsNullopt)`
- Source: `tests/temporal/test_retention_manager.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (GetPolicy_NonExistent_ReturnsNullopt): n/a

#### `TEST_F(RetentionManagerTest, IncrementalBatch_LimitsVersionsDeleted)`
- Source: `tests/temporal/test_retention_manager.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (IncrementalBatch_LimitsVersionsDeleted): n/a

#### `TEST_F(RetentionManagerTest, IncrementalBatch_VersionCount_LimitsDeleted)`
- Source: `tests/temporal/test_retention_manager.cpp`:503
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (IncrementalBatch_VersionCount_LimitsDeleted): n/a

#### `TEST_F(RetentionManagerTest, IncrementalBatch_ZeroMeansUnlimited)`
- Source: `tests/temporal/test_retention_manager.cpp`:489
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (IncrementalBatch_ZeroMeansUnlimited): n/a

#### `TEST_F(RetentionManagerTest, MinimumRetention_ProtectsRecentVersions)`
- Source: `tests/temporal/test_retention_manager.cpp`:410
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (MinimumRetention_ProtectsRecentVersions): n/a

#### `TEST_F(RetentionManagerTest, Retry_SucceedsEvenWithoutErrors)`
- Source: `tests/temporal/test_retention_manager.cpp`:518
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (Retry_SucceedsEvenWithoutErrors): n/a

#### `TEST_F(RetentionManagerTest, Scheduler_EnforcesRetentionInBackground)`
- Source: `tests/temporal/test_retention_manager.cpp`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (Scheduler_EnforcesRetentionInBackground): n/a

#### `TEST_F(RetentionManagerTest, Scheduler_StartTwice_IsNoop)`
- Source: `tests/temporal/test_retention_manager.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (Scheduler_StartTwice_IsNoop): n/a

#### `TEST_F(RetentionManagerTest, Scheduler_StartsAndStops)`
- Source: `tests/temporal/test_retention_manager.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (Scheduler_StartsAndStops): n/a

#### `TEST_F(RetentionManagerTest, SetGetPolicy_RoundTrips)`
- Source: `tests/temporal/test_retention_manager.cpp`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (SetGetPolicy_RoundTrips): n/a

#### `TEST_F(RetentionManagerTest, SpaceFreedBytes_AccumulatesInCumulativeStats)`
- Source: `tests/temporal/test_retention_manager.cpp`:395
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (SpaceFreedBytes_AccumulatesInCumulativeStats): n/a

#### `TEST_F(RetentionManagerTest, SpaceFreedBytes_NonZeroAfterDeletion)`
- Source: `tests/temporal/test_retention_manager.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (SpaceFreedBytes_NonZeroAfterDeletion): n/a

#### `TEST_F(RetentionManagerTest, StorageBased_ArchivesBeforeDelete)`
- Source: `tests/temporal/test_retention_manager.cpp`:364
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (StorageBased_ArchivesBeforeDelete): n/a

#### `TEST_F(RetentionManagerTest, StorageBased_OverLimit_DeletesOldestFirst)`
- Source: `tests/temporal/test_retention_manager.cpp`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (StorageBased_OverLimit_DeletesOldestFirst): n/a

#### `TEST_F(RetentionManagerTest, StorageBased_UnderLimit_DeletesNothing)`
- Source: `tests/temporal/test_retention_manager.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (StorageBased_UnderLimit_DeletesNothing): n/a

#### `TEST_F(RetentionManagerTest, StorageBased_ZeroLimit_IsNoop)`
- Source: `tests/temporal/test_retention_manager.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetentionManagerTest): n/a
  - `<unnamed>` (StorageBased_ZeroLimit_IsNoop): n/a

### test_snapshot_manager.cpp

#### `TEST_F(SnapshotDiffTest, SD2_01_IdenticalSnapshotsAreEmpty)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:414
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffTest): n/a
  - `<unnamed>` (SD2_01_IdenticalSnapshotsAreEmpty): n/a

#### `TEST_F(SnapshotDiffTest, SD2_02_AddedKey)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:424
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffTest): n/a
  - `<unnamed>` (SD2_02_AddedKey): n/a

#### `TEST_F(SnapshotDiffTest, SD2_03_RemovedKey)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:440
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffTest): n/a
  - `<unnamed>` (SD2_03_RemovedKey): n/a

#### `TEST_F(SnapshotDiffTest, SD2_04_ModifiedKey)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:454
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffTest): n/a
  - `<unnamed>` (SD2_04_ModifiedKey): n/a

#### `TEST_F(SnapshotDiffTest, SD2_05_InvalidBaseHandleThrows)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:468
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffTest): n/a
  - `<unnamed>` (SD2_05_InvalidBaseHandleThrows): n/a

#### `TEST_F(SnapshotDiffTest, SD2_06_DiffToJsonContainsTables)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:475
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffTest): n/a
  - `<unnamed>` (SD2_06_DiffToJsonContainsTables): n/a

#### `TEST_F(TemporalSnapshotManagerTest, CreateSnapshot_MultipleTablesAllIncluded)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (CreateSnapshot_MultipleTablesAllIncluded): n/a

#### `TEST_F(TemporalSnapshotManagerTest, CreateSnapshot_ValidHandle)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (CreateSnapshot_ValidHandle): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_KeepsFresh)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GarbageCollectByAge_KeepsFresh): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_OnlyRemovesExpired)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GarbageCollectByAge_OnlyRemovesExpired): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_RemovesExpired)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GarbageCollectByAge_RemovesExpired): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_StatsUpdated)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GarbageCollectByAge_StatsUpdated): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GarbageCollectByAge_ZeroIsNoop)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GarbageCollectByAge_ZeroIsNoop): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GarbageCollectByCount_NoRemovalWhenUnderLimit)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GarbageCollectByCount_NoRemovalWhenUnderLimit): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GarbageCollectByCount_RemovesOldest)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GarbageCollectByCount_RemovesOldest): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GarbageCollectByCount_StatsUpdated)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GarbageCollectByCount_StatsUpdated): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GarbageCollectByCount_ZeroRemovesAll)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GarbageCollectByCount_ZeroRemovesAll): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_AfterRelease_NotValid)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GetSnapshotMetadata_AfterRelease_NotValid): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_InvalidHandle_NotValid)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GetSnapshotMetadata_InvalidHandle_NotValid): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_MultipleTablesRowCount)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GetSnapshotMetadata_MultipleTablesRowCount): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_ToJson)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GetSnapshotMetadata_ToJson): n/a

#### `TEST_F(TemporalSnapshotManagerTest, GetSnapshotMetadata_ValidHandle)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (GetSnapshotMetadata_ValidHandle): n/a

#### `TEST_F(TemporalSnapshotManagerTest, IsAlive_BeforeAndAfterRelease)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (IsAlive_BeforeAndAfterRelease): n/a

#### `TEST_F(TemporalSnapshotManagerTest, Isolation_ConcurrentInsert_NotInSnapshot)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (Isolation_ConcurrentInsert_NotInSnapshot): n/a

#### `TEST_F(TemporalSnapshotManagerTest, QuerySnapshot_InvalidHandle_ReturnsEmpty)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (QuerySnapshot_InvalidHandle_ReturnsEmpty): n/a

#### `TEST_F(TemporalSnapshotManagerTest, QuerySnapshot_ReturnsSnapshotRows)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (QuerySnapshot_ReturnsSnapshotRows): n/a

#### `TEST_F(TemporalSnapshotManagerTest, QuerySnapshot_UnknownTable_ReturnsEmpty)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (QuerySnapshot_UnknownTable_ReturnsEmpty): n/a

#### `TEST_F(TemporalSnapshotManagerTest, QuerySnapshot_WithFilter_ReturnsSubset)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (QuerySnapshot_WithFilter_ReturnsSubset): n/a

#### `TEST_F(TemporalSnapshotManagerTest, ReleaseSnapshot_NonExistentHandle_ReturnsFalse)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (ReleaseSnapshot_NonExistentHandle_ReturnsFalse): n/a

#### `TEST_F(TemporalSnapshotManagerTest, ReleaseSnapshot_SnapshotCountDecreases)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (ReleaseSnapshot_SnapshotCountDecreases): n/a

#### `TEST_F(TemporalSnapshotManagerTest, Statistics_TrackCreatedAndReleased)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (Statistics_TrackCreatedAndReleased): n/a

#### `TEST_F(TemporalSnapshotManagerTest, VersionNumber_MonotonicallyIncreasing)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (VersionNumber_MonotonicallyIncreasing): n/a

#### `TEST_F(TemporalSnapshotManagerTest, VersionNumber_OrderingOperator)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (VersionNumber_OrderingOperator): n/a

#### `TEST_F(TemporalSnapshotManagerTest, VersionNumber_ToJson_ContainsVersionNumber)`
- Source: `tests/temporal/test_snapshot_manager.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalSnapshotManagerTest): n/a
  - `<unnamed>` (VersionNumber_ToJson_ContainsVersionNumber): n/a

### test_system_versioned_table.cpp

#### `TEST(SystemVersionedTableConfigTest, CreateVersionedTable_CustomConfig)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableConfigTest): n/a
  - `<unnamed>` (CreateVersionedTable_CustomConfig): n/a

#### `TEST(SystemVersionedTableConfigTest, CreateVersionedTable_SetsSchemaInStats)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableConfigTest): n/a
  - `<unnamed>` (CreateVersionedTable_SetsSchemaInStats): n/a

#### `TEST(SystemVersionedTableConfigTest, DefaultConfig_HistoryTableNameDerived)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableConfigTest): n/a
  - `<unnamed>` (DefaultConfig_HistoryTableNameDerived): n/a

#### `TEST(SystemVersionedTableConfigTest, ExplicitConfig_Stored)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableConfigTest): n/a
  - `<unnamed>` (ExplicitConfig_Stored): n/a

#### `TEST(SystemVersionedTableConfigTest, Statistics_IncludesConfigFields)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:413
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableConfigTest): n/a
  - `<unnamed>` (Statistics_IncludesConfigFields): n/a

#### `TEST(SystemVersionedTableConfigTest, TrackUserIdFalse_ModifiedByEmpty)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableConfigTest): n/a
  - `<unnamed>` (TrackUserIdFalse_ModifiedByEmpty): n/a

#### `TEST(SystemVersionedTableConfigTest, TrackUserIdTrue_SetsModifiedBy)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableConfigTest): n/a
  - `<unnamed>` (TrackUserIdTrue_SetsModifiedBy): n/a

#### `TEST(SystemVersionedTableRetentionTest, EnforceRetention_CurrentNeverPurged)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:396
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableRetentionTest): n/a
  - `<unnamed>` (EnforceRetention_CurrentNeverPurged): n/a

#### `TEST(SystemVersionedTableRetentionTest, EnforceRetention_YoungVersionsNotPurged)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableRetentionTest): n/a
  - `<unnamed>` (EnforceRetention_YoungVersionsNotPurged): n/a

#### `TEST(SystemVersionedTableRetentionTest, EnforceRetention_ZeroPeriod_IsNoop)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:382
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableRetentionTest): n/a
  - `<unnamed>` (EnforceRetention_ZeroPeriod_IsNoop): n/a

#### `TEST(SystemVersionedTableUpsertTest, Upsert_AfterDelete_ActsAsInsert)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableUpsertTest): n/a
  - `<unnamed>` (Upsert_AfterDelete_ActsAsInsert): n/a

#### `TEST(SystemVersionedTableUpsertTest, Upsert_ExistingKey_ActsAsUpdate)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableUpsertTest): n/a
  - `<unnamed>` (Upsert_ExistingKey_ActsAsUpdate): n/a

#### `TEST(SystemVersionedTableUpsertTest, Upsert_NewKey_ActsAsInsert)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableUpsertTest): n/a
  - `<unnamed>` (Upsert_NewKey_ActsAsInsert): n/a

#### `TEST_F(SystemVersionedTableTest, Delete_ExistingKey_ClosesVersion)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Delete_ExistingKey_ClosesVersion): n/a

#### `TEST_F(SystemVersionedTableTest, Delete_MissingKey_ReturnsFalse)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Delete_MissingKey_ReturnsFalse): n/a

#### `TEST_F(SystemVersionedTableTest, GetAllKeys_EmptyTable_ReturnsEmpty)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (GetAllKeys_EmptyTable_ReturnsEmpty): n/a

#### `TEST_F(SystemVersionedTableTest, GetAllKeys_IncludesDeletedKeys)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (GetAllKeys_IncludesDeletedKeys): n/a

#### `TEST_F(SystemVersionedTableTest, GetAsOf_AfterInsert_ReturnsVersion)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (GetAsOf_AfterInsert_ReturnsVersion): n/a

#### `TEST_F(SystemVersionedTableTest, GetCurrent_ExistingKey_ReturnsCurrentVersion)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (GetCurrent_ExistingKey_ReturnsCurrentVersion): n/a

#### `TEST_F(SystemVersionedTableTest, GetCurrent_MissingKey_ReturnsNullopt)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (GetCurrent_MissingKey_ReturnsNullopt): n/a

#### `TEST_F(SystemVersionedTableTest, GetHistoryInRange_OverlapsInsertTime)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (GetHistoryInRange_OverlapsInsertTime): n/a

#### `TEST_F(SystemVersionedTableTest, Insert_DuplicateKey_Fails)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Insert_DuplicateKey_Fails): n/a

#### `TEST_F(SystemVersionedTableTest, Insert_NewKey_Succeeds)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:21
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Insert_NewKey_Succeeds): n/a

#### `TEST_F(SystemVersionedTableTest, PurgeHistorical_AllKeys_GlobalPurge)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (PurgeHistorical_AllKeys_GlobalPurge): n/a

#### `TEST_F(SystemVersionedTableTest, PurgeHistorical_CurrentVersionNeverRemoved)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (PurgeHistorical_CurrentVersionNeverRemoved): n/a

#### `TEST_F(SystemVersionedTableTest, PurgeHistorical_RemovesMatchingVersions)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (PurgeHistorical_RemovesMatchingVersions): n/a

#### `TEST_F(SystemVersionedTableTest, Scan_AsOf_ReturnsHistoricalRows)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Scan_AsOf_ReturnsHistoricalRows): n/a

#### `TEST_F(SystemVersionedTableTest, Scan_Default_ReturnCurrentRows)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Scan_Default_ReturnCurrentRows): n/a

#### `TEST_F(SystemVersionedTableTest, Statistics_ReflectsState)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Statistics_ReflectsState): n/a

#### `TEST_F(SystemVersionedTableTest, Update_ClosesOldVersion)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Update_ClosesOldVersion): n/a

#### `TEST_F(SystemVersionedTableTest, Update_ExistingKey_CreatesNewVersion)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Update_ExistingKey_CreatesNewVersion): n/a

#### `TEST_F(SystemVersionedTableTest, Update_MissingKey_ReturnsFalse)`
- Source: `tests/temporal/test_system_versioned_table.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (SystemVersionedTableTest): n/a
  - `<unnamed>` (Update_MissingKey_ReturnsFalse): n/a

### test_temporal_aggregation.cpp

#### `TEST_F(TemporalAggregationTest, GetTemporalStats_AllEdgesOverlap)`
- Source: `tests/temporal/test_temporal_aggregation.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationTest): n/a
  - `<unnamed>` (GetTemporalStats_AllEdgesOverlap): n/a

#### `TEST_F(TemporalAggregationTest, GetTemporalStats_EmptyDatabase)`
- Source: `tests/temporal/test_temporal_aggregation.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationTest): n/a
  - `<unnamed>` (GetTemporalStats_EmptyDatabase): n/a

#### `TEST_F(TemporalAggregationTest, GetTemporalStats_FullyContainedOnly)`
- Source: `tests/temporal/test_temporal_aggregation.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationTest): n/a
  - `<unnamed>` (GetTemporalStats_FullyContainedOnly): n/a

#### `TEST_F(TemporalAggregationTest, GetTemporalStats_NoOverlap)`
- Source: `tests/temporal/test_temporal_aggregation.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationTest): n/a
  - `<unnamed>` (GetTemporalStats_NoOverlap): n/a

#### `TEST_F(TemporalAggregationTest, GetTemporalStats_PartialOverlap)`
- Source: `tests/temporal/test_temporal_aggregation.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationTest): n/a
  - `<unnamed>` (GetTemporalStats_PartialOverlap): n/a

#### `TEST_F(TemporalAggregationTest, GetTemporalStats_ToStringFormat)`
- Source: `tests/temporal/test_temporal_aggregation.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationTest): n/a
  - `<unnamed>` (GetTemporalStats_ToStringFormat): n/a

### test_temporal_aggregation_property.cpp

#### `TEST_F(TemporalAggregationPropertyTest, CountAllEdges)`
- Source: `tests/temporal/test_temporal_aggregation_property.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationPropertyTest): n/a
  - `<unnamed>` (CountAllEdges): n/a

#### `TEST_F(TemporalAggregationPropertyTest, NonexistentProperty)`
- Source: `tests/temporal/test_temporal_aggregation_property.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationPropertyTest): n/a
  - `<unnamed>` (NonexistentProperty): n/a

#### `TEST_F(TemporalAggregationPropertyTest, SumAvgMinMaxNoType)`
- Source: `tests/temporal/test_temporal_aggregation_property.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationPropertyTest): n/a
  - `<unnamed>` (SumAvgMinMaxNoType): n/a

#### `TEST_F(TemporalAggregationPropertyTest, TypeFilterSum)`
- Source: `tests/temporal/test_temporal_aggregation_property.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregationPropertyTest): n/a
  - `<unnamed>` (TypeFilterSum): n/a

### test_temporal_aggregator.cpp

#### `TEST_F(TemporalAggregatorTest, AggregateResult_ToJson)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (AggregateResult_ToJson): n/a

#### `TEST_F(TemporalAggregatorTest, AggregateResult_ToJson_WithGroupValues)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:358
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (AggregateResult_ToJson_WithGroupValues): n/a

#### `TEST_F(TemporalAggregatorTest, AnalyzeTrend_EmptyTable_ReturnsZeroTrend)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:572
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (AnalyzeTrend_EmptyTable_ReturnsZeroTrend): n/a

#### `TEST_F(TemporalAggregatorTest, AnalyzeTrend_IncreasingValues)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:546
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (AnalyzeTrend_IncreasingValues): n/a

#### `TEST_F(TemporalAggregatorTest, AnalyzeTrend_ToJson)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:583
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (AnalyzeTrend_ToJson): n/a

#### `TEST_F(TemporalAggregatorTest, FLV_01_FirstValue_Tumbling_ReturnsEarliestInWindow)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:608
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (FLV_01_FirstValue_Tumbling_ReturnsEarliestInWindow): n/a

#### `TEST_F(TemporalAggregatorTest, FLV_02_LastValue_Tumbling_ReturnsLatestInWindow)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:630
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (FLV_02_LastValue_Tumbling_ReturnsLatestInWindow): n/a

#### `TEST_F(TemporalAggregatorTest, FLV_03_FirstLast_SingleRow_ReturnSameValue)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:651
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (FLV_03_FirstLast_SingleRow_ReturnSameValue): n/a

#### `TEST_F(TemporalAggregatorTest, FLV_04_FirstValue_EmptyWindow_ResultIsEmpty)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:670
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (FLV_04_FirstValue_EmptyWindow_ResultIsEmpty): n/a

#### `TEST_F(TemporalAggregatorTest, FLV_05_FirstValue_Differs_From_LastValue)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:687
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (FLV_05_FirstValue_Differs_From_LastValue): n/a

#### `TEST_F(TemporalAggregatorTest, GroupBy_COUNT_TwoGroups)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (GroupBy_COUNT_TwoGroups): n/a

#### `TEST_F(TemporalAggregatorTest, GroupBy_EmptyGroupByFields_ReturnsUnnamedGroup)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:442
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (GroupBy_EmptyGroupByFields_ReturnsUnnamedGroup): n/a

#### `TEST_F(TemporalAggregatorTest, GroupBy_InvalidRange_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:460
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (GroupBy_InvalidRange_ReturnsEmpty): n/a

#### `TEST_F(TemporalAggregatorTest, GroupBy_SUM_GroupValuesPopulated)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (GroupBy_SUM_GroupValuesPopulated): n/a

#### `TEST_F(TemporalAggregatorTest, Session_COUNT_TwoSessions)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Session_COUNT_TwoSessions): n/a

#### `TEST_F(TemporalAggregatorTest, Session_EmptyTable_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:329
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Session_EmptyTable_ReturnsEmpty): n/a

#### `TEST_F(TemporalAggregatorTest, Session_SUM_MergeLargeGap)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Session_SUM_MergeLargeGap): n/a

#### `TEST_F(TemporalAggregatorTest, Sliding_COUNT_RowInMultipleWindows)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Sliding_COUNT_RowInMultipleWindows): n/a

#### `TEST_F(TemporalAggregatorTest, Sliding_DefaultSlide_EqualsTumbling)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Sliding_DefaultSlide_EqualsTumbling): n/a

#### `TEST_F(TemporalAggregatorTest, Snapshots_COUNT_StableRow)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:482
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Snapshots_COUNT_StableRow): n/a

#### `TEST_F(TemporalAggregatorTest, Snapshots_EmptyTable_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:526
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Snapshots_EmptyTable_ReturnsEmpty): n/a

#### `TEST_F(TemporalAggregatorTest, Snapshots_SUM_VisibleAtSnapshotTime)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:504
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Snapshots_SUM_VisibleAtSnapshotTime): n/a

#### `TEST_F(TemporalAggregatorTest, Tumbling_AVG_SingleWindow)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Tumbling_AVG_SingleWindow): n/a

#### `TEST_F(TemporalAggregatorTest, Tumbling_COUNT_EmptyTable)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Tumbling_COUNT_EmptyTable): n/a

#### `TEST_F(TemporalAggregatorTest, Tumbling_COUNT_RowsDistributedAcrossWindows)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Tumbling_COUNT_RowsDistributedAcrossWindows): n/a

#### `TEST_F(TemporalAggregatorTest, Tumbling_MIN_MAX)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Tumbling_MIN_MAX): n/a

#### `TEST_F(TemporalAggregatorTest, Tumbling_NoMeasureFieldForSUM_ValueIsZero)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Tumbling_NoMeasureFieldForSUM_ValueIsZero): n/a

#### `TEST_F(TemporalAggregatorTest, Tumbling_ResultWindowsAreContiguous)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Tumbling_ResultWindowsAreContiguous): n/a

#### `TEST_F(TemporalAggregatorTest, Tumbling_SUM_SingleWindow)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalAggregatorTest): n/a
  - `<unnamed>` (Tumbling_SUM_SingleWindow): n/a

#### `void insertAt(SystemVersionedTable &t, const std::string &key, double value, int extra_sleep_ms=1)`
- Source: `tests/temporal/test_temporal_aggregator.cpp`:21
- Brief: n/a
- Parameters:
  - `t` (SystemVersionedTable &): n/a
  - `key` (const std::string &): n/a
  - `value` (double): n/a
  - `extra_sleep_ms` (int): n/a

### test_temporal_cdc.cpp

#### `TEST_F(CDCPersistentLogTest, CDCPL_01_OpenAndClose)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogTest): n/a
  - `<unnamed>` (CDCPL_01_OpenAndClose): n/a

#### `TEST_F(CDCPersistentLogTest, CDCPL_02_AppendAndReplay)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogTest): n/a
  - `<unnamed>` (CDCPL_02_AppendAndReplay): n/a

#### `TEST_F(CDCPersistentLogTest, CDCPL_03_TotalEventsCounter)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogTest): n/a
  - `<unnamed>` (CDCPL_03_TotalEventsCounter): n/a

#### `TEST_F(CDCPersistentLogTest, CDCPL_04_TotalBytesWritten)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogTest): n/a
  - `<unnamed>` (CDCPL_04_TotalBytesWritten): n/a

#### `TEST_F(CDCPersistentLogTest, CDCPL_05_AppendBeforeOpenThrows)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogTest): n/a
  - `<unnamed>` (CDCPL_05_AppendBeforeOpenThrows): n/a

#### `TEST_F(CDCPersistentLogTest, CDCPL_06_ReplaySegmentByIndex)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogTest): n/a
  - `<unnamed>` (CDCPL_06_ReplaySegmentByIndex): n/a

#### `TEST_F(CDCPersistentLogTest, CDCPL_07_ReplaySegmentOutOfRangeThrows)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:341
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogTest): n/a
  - `<unnamed>` (CDCPL_07_ReplaySegmentOutOfRangeThrows): n/a

#### `TEST_F(CDCPersistentLogTest, CDCPL_08_IdempotentOpen)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogTest): n/a
  - `<unnamed>` (CDCPL_08_IdempotentOpen): n/a

#### `TEST_F(TemporalCDCTest, ChangeEvent_ToJson_FromJson_RoundTrip)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (ChangeEvent_ToJson_FromJson_RoundTrip): n/a

#### `TEST_F(TemporalCDCTest, ClearLog_EmptiesLog)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (ClearLog_EmptiesLog): n/a

#### `TEST_F(TemporalCDCTest, ClearLog_SubscriptionsIntact)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (ClearLog_SubscriptionsIntact): n/a

#### `TEST_F(TemporalCDCTest, LogSize_AfterPublish_Increases)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (LogSize_AfterPublish_Increases): n/a

#### `TEST_F(TemporalCDCTest, PublishEvent_DeliveresToSubscriber)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (PublishEvent_DeliveresToSubscriber): n/a

#### `TEST_F(TemporalCDCTest, PublishEvent_DoesNotDeliverToWrongTable)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (PublishEvent_DoesNotDeliverToWrongTable): n/a

#### `TEST_F(TemporalCDCTest, PublishEvent_MultipleSubscribers_AllReceive)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (PublishEvent_MultipleSubscribers_AllReceive): n/a

#### `TEST_F(TemporalCDCTest, PublishEvent_WildcardSubscriber_ReceivesAll)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (PublishEvent_WildcardSubscriber_ReceivesAll): n/a

#### `TEST_F(TemporalCDCTest, ReplayChanges_AllTables_ReturnsAll)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (ReplayChanges_AllTables_ReturnsAll): n/a

#### `TEST_F(TemporalCDCTest, ReplayChanges_EmptyRange_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (ReplayChanges_EmptyRange_ReturnsEmpty): n/a

#### `TEST_F(TemporalCDCTest, ReplayChanges_ReturnsMatchingEvents)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (ReplayChanges_ReturnsMatchingEvents): n/a

#### `TEST_F(TemporalCDCTest, ReplayChanges_TimeRangeFilter_ExcludesOutOfRange)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (ReplayChanges_TimeRangeFilter_ExcludesOutOfRange): n/a

#### `TEST_F(TemporalCDCTest, RingBuffer_Overflow_OldestEvicted)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (RingBuffer_Overflow_OldestEvicted): n/a

#### `TEST_F(TemporalCDCTest, Subscribe_ReturnsNonEmptySubId)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (Subscribe_ReturnsNonEmptySubId): n/a

#### `TEST_F(TemporalCDCTest, SubscriptionCount_AfterSubscribe_IsOne)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (SubscriptionCount_AfterSubscribe_IsOne): n/a

#### `TEST_F(TemporalCDCTest, SubscriptionCount_AfterUnsubscribe_IsZero)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (SubscriptionCount_AfterUnsubscribe_IsZero): n/a

#### `TEST_F(TemporalCDCTest, SubscriptionCount_MultipleSubscriptions_IsCorrect)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (SubscriptionCount_MultipleSubscriptions_IsCorrect): n/a

#### `TEST_F(TemporalCDCTest, TotalPublished_Increments)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (TotalPublished_Increments): n/a

#### `TEST_F(TemporalCDCTest, Unsubscribe_InvalidId_ReturnsFalse)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (Unsubscribe_InvalidId_ReturnsFalse): n/a

#### `TEST_F(TemporalCDCTest, Unsubscribe_ReducesCount)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (Unsubscribe_ReducesCount): n/a

#### `TEST_F(TemporalCDCTest, Unsubscribe_ValidId_ReturnsTrue)`
- Source: `tests/temporal/test_temporal_cdc.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDCTest): n/a
  - `<unnamed>` (Unsubscribe_ValidId_ReturnsTrue): n/a

### test_temporal_cold_store.cpp

#### `TEST(ColdStoreConcurrencyTest, ConcurrentReads_NoDeadlock)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreConcurrencyTest): n/a
  - `<unnamed>` (ConcurrentReads_NoDeadlock): n/a

#### `TEST_F(ColdStoreFileSystemTest, Clear_RemovesAllFilesFromDisk)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreFileSystemTest): n/a
  - `<unnamed>` (Clear_RemovesAllFilesFromDisk): n/a

#### `TEST_F(ColdStoreFileSystemTest, GetAll_AfterRebuild_AllVersionsReturned)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreFileSystemTest): n/a
  - `<unnamed>` (GetAll_AfterRebuild_AllVersionsReturned): n/a

#### `TEST_F(ColdStoreFileSystemTest, GetAsOf_CorrectVersionFromDisk)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreFileSystemTest): n/a
  - `<unnamed>` (GetAsOf_CorrectVersionFromDisk): n/a

#### `TEST_F(ColdStoreFileSystemTest, RebuildIndex_FromExistingFiles)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreFileSystemTest): n/a
  - `<unnamed>` (RebuildIndex_FromExistingFiles): n/a

#### `TEST_F(ColdStoreFileSystemTest, Remove_DeletesFilesFromDisk)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreFileSystemTest): n/a
  - `<unnamed>` (Remove_DeletesFilesFromDisk): n/a

#### `TEST_F(ColdStoreFileSystemTest, SpecialCharKeys_SafelyEncodedOnDisk)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreFileSystemTest): n/a
  - `<unnamed>` (SpecialCharKeys_SafelyEncodedOnDisk): n/a

#### `TEST_F(ColdStoreFileSystemTest, Stats_BackendReadCountIncremented)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreFileSystemTest): n/a
  - `<unnamed>` (Stats_BackendReadCountIncremented): n/a

#### `TEST_F(ColdStoreFileSystemTest, Store_CreatesFilesOnDisk)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreFileSystemTest): n/a
  - `<unnamed>` (Store_CreatesFilesOnDisk): n/a

#### `TEST_F(ColdStoreInMemoryTest, Clear_ResetsEverything)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (Clear_ResetsEverything): n/a

#### `TEST_F(ColdStoreInMemoryTest, GetAll_ReturnsSortedVersions)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (GetAll_ReturnsSortedVersions): n/a

#### `TEST_F(ColdStoreInMemoryTest, GetAsOf_MatchingVersion_Found)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (GetAsOf_MatchingVersion_Found): n/a

#### `TEST_F(ColdStoreInMemoryTest, GetAsOf_NoMatch_NullOpt)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (GetAsOf_NoMatch_NullOpt): n/a

#### `TEST_F(ColdStoreInMemoryTest, GetAsOf_UnknownKey_NullOpt)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (GetAsOf_UnknownKey_NullOpt): n/a

#### `TEST_F(ColdStoreInMemoryTest, GetRange_FiltersCorrectly)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (GetRange_FiltersCorrectly): n/a

#### `TEST_F(ColdStoreInMemoryTest, RemoveTable_RemovesAllTableVersions)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (RemoveTable_RemovesAllTableVersions): n/a

#### `TEST_F(ColdStoreInMemoryTest, Remove_Key_RemovesAllVersions)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (Remove_Key_RemovesAllVersions): n/a

#### `TEST_F(ColdStoreInMemoryTest, Store_ClosedVersion_Accepted)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (Store_ClosedVersion_Accepted): n/a

#### `TEST_F(ColdStoreInMemoryTest, Store_CurrentVersion_Rejected)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (ColdStoreInMemoryTest): n/a
  - `<unnamed>` (Store_CurrentVersion_Rejected): n/a

#### `VersionedDocument makeClosedDoc(const std::string &key, Timestamp sys_start, Timestamp sys_end, const std::string &value="v")`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:23
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `sys_start` (Timestamp): n/a
  - `sys_end` (Timestamp): n/a
  - `value` (const std::string &): n/a

#### `VersionedDocument makeOpenDoc(const std::string &key, Timestamp sys_start)`
- Source: `tests/temporal/test_temporal_cold_store.cpp`:35
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `sys_start` (Timestamp): n/a

### test_temporal_compressor.cpp

#### `TEST_F(TemporalCompressorTest, AlgorithmName_Delta)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:21
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (AlgorithmName_Delta): n/a

#### `TEST_F(TemporalCompressorTest, AlgorithmName_Dictionary)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (AlgorithmName_Dictionary): n/a

#### `TEST_F(TemporalCompressorTest, AlgorithmName_Gorilla)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (AlgorithmName_Gorilla): n/a

#### `TEST_F(TemporalCompressorTest, AlgorithmName_Zstd)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:25
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (AlgorithmName_Zstd): n/a

#### `TEST_F(TemporalCompressorTest, CompressHistory_CompressImmediately_DoesNotSkip)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (CompressHistory_CompressImmediately_DoesNotSkip): n/a

#### `TEST_F(TemporalCompressorTest, CompressHistory_DeltaAlgo_ProcessesVersions)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (CompressHistory_DeltaAlgo_ProcessesVersions): n/a

#### `TEST_F(TemporalCompressorTest, CompressHistory_DictionaryAlgo_ProcessesVersions)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (CompressHistory_DictionaryAlgo_ProcessesVersions): n/a

#### `TEST_F(TemporalCompressorTest, CompressHistory_EmptyTable_ReturnsZeroStats)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (CompressHistory_EmptyTable_ReturnsZeroStats): n/a

#### `TEST_F(TemporalCompressorTest, CompressHistory_SkipsRecentVersions)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (CompressHistory_SkipsRecentVersions): n/a

#### `TEST_F(TemporalCompressorTest, CompressHistory_ZstdAlgo_ProcessesVersions)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (CompressHistory_ZstdAlgo_ProcessesVersions): n/a

#### `TEST_F(TemporalCompressorTest, Decompress_NonCompressedDoc_ReturnsSame)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (Decompress_NonCompressedDoc_ReturnsSame): n/a

#### `TEST_F(TemporalCompressorTest, TCLZ4_01_AlgorithmName)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (TCLZ4_01_AlgorithmName): n/a

#### `TEST_F(TemporalCompressorTest, TCLZ4_02_CompressHistoryLZ4ProcessesVersions)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (TCLZ4_02_CompressHistoryLZ4ProcessesVersions): n/a

#### `TEST_F(TemporalCompressorTest, TCLZ4_03_DecompressRoundTrip)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (TCLZ4_03_DecompressRoundTrip): n/a

#### `TEST_F(TemporalCompressorTest, TCLZ4_04_CompressionRatioPositive)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (TCLZ4_04_CompressionRatioPositive): n/a

#### `TEST_F(TemporalCompressorTest, TCLZ4_05_EmptyTableNoError)`
- Source: `tests/temporal/test_temporal_compressor.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressorTest): n/a
  - `<unnamed>` (TCLZ4_05_EmptyTableNoError): n/a

### test_temporal_conflict_resolver.cpp

#### `TEST(MergeResolverTest, Custom_CallableInvoked)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:895
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeResolverTest): n/a
  - `<unnamed>` (Custom_CallableInvoked): n/a

#### `TEST(MergeResolverTest, InjectedResolverUsedByCRDTMerge)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:920
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeResolverTest): n/a
  - `<unnamed>` (InjectedResolverUsedByCRDTMerge): n/a

#### `TEST(MergeResolverTest, LWWField_Commutative)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:837
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeResolverTest): n/a
  - `<unnamed>` (LWWField_Commutative): n/a

#### `TEST(MergeResolverTest, LWWField_Idempotent)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:852
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeResolverTest): n/a
  - `<unnamed>` (LWWField_Idempotent): n/a

#### `TEST(MergeResolverTest, NullResolverFallsBackToBuiltInLWW)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:946
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeResolverTest): n/a
  - `<unnamed>` (NullResolverFallsBackToBuiltInLWW): n/a

#### `TEST(MergeResolverTest, Union_Commutative)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:881
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeResolverTest): n/a
  - `<unnamed>` (Union_Commutative): n/a

#### `TEST(MergeResolverTest, Union_IncludesAllFields)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:864
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeResolverTest): n/a
  - `<unnamed>` (Union_IncludesAllFields): n/a

#### `TEST_F(TemporalConflictDetectorTest, AutoResolve_LWW_ReturnsNewer)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:642
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (AutoResolve_LWW_ReturnsNewer): n/a

#### `TEST_F(TemporalConflictDetectorTest, AutoResolve_ManualPolicy_ReturnsNullopt)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:658
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (AutoResolve_ManualPolicy_ReturnsNullopt): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectConcurrentUpdate_ConcurrentHLC)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:492
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectConcurrentUpdate_ConcurrentHLC): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectConcurrentUpdate_OrderedHLC_NoConflict)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectConcurrentUpdate_OrderedHLC_NoConflict): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectConcurrentUpdate_SameNode_SameData_NoConflict)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:518
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectConcurrentUpdate_SameNode_SameData_NoConflict): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectConflicts_IdenticalSnapshots_Empty)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:761
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectConflicts_IdenticalSnapshots_Empty): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectConflicts_TableNameAndEntityIdSet)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:747
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectConflicts_TableNameAndEntityIdSet): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectOverlappingPeriods_MissingFields_NoConflict)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:564
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectOverlappingPeriods_MissingFields_NoConflict): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectOverlappingPeriods_NoOverlap)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:550
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectOverlappingPeriods_NoOverlap): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectOverlappingPeriods_NonIntegerFields_NoConflict)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:792
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectOverlappingPeriods_NonIntegerFields_NoConflict): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectOverlappingPeriods_Overlap)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:532
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectOverlappingPeriods_Overlap): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectReferentialIntegrity_DifferentRef)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:578
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectReferentialIntegrity_DifferentRef): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectReferentialIntegrity_SameRef_NoConflict)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:591
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectReferentialIntegrity_SameRef_NoConflict): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectUniquenessViolation_AsymmetricKeys_Detected)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:771
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectUniquenessViolation_AsymmetricKeys_Detected): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectUniquenessViolation_DifferentNodes_DifferentData)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:604
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectUniquenessViolation_DifferentNodes_DifferentData): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectUniquenessViolation_SameData_NoConflict)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:629
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectUniquenessViolation_SameData_NoConflict): n/a

#### `TEST_F(TemporalConflictDetectorTest, DetectUniquenessViolation_SameNode_NoConflict)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:617
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (DetectUniquenessViolation_SameNode_NoConflict): n/a

#### `TEST_F(TemporalConflictDetectorTest, Queue_AddAndRetrieve)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:675
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (Queue_AddAndRetrieve): n/a

#### `TEST_F(TemporalConflictDetectorTest, Queue_ClearEmptiesQueue)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:728
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (Queue_ClearEmptiesQueue): n/a

#### `TEST_F(TemporalConflictDetectorTest, Queue_DuplicateNotQueued)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:695
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (Queue_DuplicateNotQueued): n/a

#### `TEST_F(TemporalConflictDetectorTest, Queue_SameConflictDifferentTables_BothQueued)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:711
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictDetectorTest): n/a
  - `<unnamed>` (Queue_SameConflictDifferentTables_BothQueued): n/a

#### `TEST_F(TemporalConflictResolverTest, CRDTMerge_FallbackToLWW)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (CRDTMerge_FallbackToLWW): n/a

#### `TEST_F(TemporalConflictResolverTest, CRDTMerge_MergesFieldsFromBothSnapshots)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (CRDTMerge_MergesFieldsFromBothSnapshots): n/a

#### `TEST_F(TemporalConflictResolverTest, ConflictHistory_AfterManualResolution)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:384
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ConflictHistory_AfterManualResolution): n/a

#### `TEST_F(TemporalConflictResolverTest, ConflictHistory_IncludesUnresolved)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ConflictHistory_IncludesUnresolved): n/a

#### `TEST_F(TemporalConflictResolverTest, ConflictHistory_InitiallyEmpty)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ConflictHistory_InitiallyEmpty): n/a

#### `TEST_F(TemporalConflictResolverTest, ConflictHistory_MultipleConflictsAccumulate)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:401
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ConflictHistory_MultipleConflictsAccumulate): n/a

#### `TEST_F(TemporalConflictResolverTest, ConflictHistory_RecordsResolvedConflicts)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:359
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ConflictHistory_RecordsResolvedConflicts): n/a

#### `TEST_F(TemporalConflictResolverTest, ExportAuditLog_EmptyWhenNoConflicts)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:435
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ExportAuditLog_EmptyWhenNoConflicts): n/a

#### `TEST_F(TemporalConflictResolverTest, ExportAuditLog_IsJsonArray)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:413
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ExportAuditLog_IsJsonArray): n/a

#### `TEST_F(TemporalConflictResolverTest, ExportAuditLog_PoliciesEncoded)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:442
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ExportAuditLog_PoliciesEncoded): n/a

#### `TEST_F(TemporalConflictResolverTest, FirstWriteWins_EqualHLC_TiebreakerNodeID)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (FirstWriteWins_EqualHLC_TiebreakerNodeID): n/a

#### `TEST_F(TemporalConflictResolverTest, FirstWriteWins_LocalOlder)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (FirstWriteWins_LocalOlder): n/a

#### `TEST_F(TemporalConflictResolverTest, FirstWriteWins_RemoteOlder)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (FirstWriteWins_RemoteOlder): n/a

#### `TEST_F(TemporalConflictResolverTest, LastWriteWins_EqualHLC_TiebreakerNodeID)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (LastWriteWins_EqualHLC_TiebreakerNodeID): n/a

#### `TEST_F(TemporalConflictResolverTest, LastWriteWins_EqualPhysical_HigherLogical)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (LastWriteWins_EqualPhysical_HigherLogical): n/a

#### `TEST_F(TemporalConflictResolverTest, LastWriteWins_LocalNewer)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (LastWriteWins_LocalNewer): n/a

#### `TEST_F(TemporalConflictResolverTest, LastWriteWins_RemoteNewer)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (LastWriteWins_RemoteNewer): n/a

#### `TEST_F(TemporalConflictResolverTest, ManualResolution_QueueConflict)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ManualResolution_QueueConflict): n/a

#### `TEST_F(TemporalConflictResolverTest, ManualResolution_ResolveManually)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (ManualResolution_ResolveManually): n/a

#### `TEST_F(TemporalConflictResolverTest, NodePriority_LocalHigherPriority)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (NodePriority_LocalHigherPriority): n/a

#### `TEST_F(TemporalConflictResolverTest, NodePriority_RemoteHigherPriority)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (NodePriority_RemoteHigherPriority): n/a

#### `TEST_F(TemporalConflictResolverTest, PolicyOverride_DefaultLWW_OverrideFWW)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (PolicyOverride_DefaultLWW_OverrideFWW): n/a

#### `TEST_F(TemporalConflictResolverTest, Statistics_MultipleConflicts)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (Statistics_MultipleConflicts): n/a

#### `TEST_F(TemporalConflictResolverTest, TemporalSnapshot_FromJson)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (TemporalSnapshot_FromJson): n/a

#### `TEST_F(TemporalConflictResolverTest, TemporalSnapshot_ToJson)`
- Source: `tests/temporal/test_temporal_conflict_resolver.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalConflictResolverTest): n/a
  - `<unnamed>` (TemporalSnapshot_ToJson): n/a

### test_temporal_graph.cpp

#### `TEST_F(TemporalGraphTest, AggregateEdgePropertyInTimeRange_Avg)`
- Source: `tests/temporal/test_temporal_graph.cpp`:739
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (AggregateEdgePropertyInTimeRange_Avg): n/a

#### `TEST_F(TemporalGraphTest, AggregateEdgePropertyInTimeRange_Count)`
- Source: `tests/temporal/test_temporal_graph.cpp`:703
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (AggregateEdgePropertyInTimeRange_Count): n/a

#### `TEST_F(TemporalGraphTest, AggregateEdgePropertyInTimeRange_MinMax)`
- Source: `tests/temporal/test_temporal_graph.cpp`:754
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (AggregateEdgePropertyInTimeRange_MinMax): n/a

#### `TEST_F(TemporalGraphTest, AggregateEdgePropertyInTimeRange_Sum)`
- Source: `tests/temporal/test_temporal_graph.cpp`:721
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (AggregateEdgePropertyInTimeRange_Sum): n/a

#### `TEST_F(TemporalGraphTest, BfsAtTime_EmptyStartNode_ReturnsError)`
- Source: `tests/temporal/test_temporal_graph.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (BfsAtTime_EmptyStartNode_ReturnsError): n/a

#### `TEST_F(TemporalGraphTest, BfsAtTime_FiltersByValidFrom)`
- Source: `tests/temporal/test_temporal_graph.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (BfsAtTime_FiltersByValidFrom): n/a

#### `TEST_F(TemporalGraphTest, BfsAtTime_FiltersByValidRange)`
- Source: `tests/temporal/test_temporal_graph.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (BfsAtTime_FiltersByValidRange): n/a

#### `TEST_F(TemporalGraphTest, BfsAtTime_FiltersByValidTo)`
- Source: `tests/temporal/test_temporal_graph.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (BfsAtTime_FiltersByValidTo): n/a

#### `TEST_F(TemporalGraphTest, BfsAtTime_IsolatedNodeAfterExpiration)`
- Source: `tests/temporal/test_temporal_graph.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (BfsAtTime_IsolatedNodeAfterExpiration): n/a

#### `TEST_F(TemporalGraphTest, BfsAtTime_MaxDepthZero_ReturnsOnlyStart)`
- Source: `tests/temporal/test_temporal_graph.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (BfsAtTime_MaxDepthZero_ReturnsOnlyStart): n/a

#### `TEST_F(TemporalGraphTest, BfsAtTime_MultiplePathsOverTime)`
- Source: `tests/temporal/test_temporal_graph.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (BfsAtTime_MultiplePathsOverTime): n/a

#### `TEST_F(TemporalGraphTest, BfsAtTime_NegativeDepth_ReturnsError)`
- Source: `tests/temporal/test_temporal_graph.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (BfsAtTime_NegativeDepth_ReturnsError): n/a

#### `TEST_F(TemporalGraphTest, BfsAtTime_NoTemporalEdges_ReturnsAllNeighbors)`
- Source: `tests/temporal/test_temporal_graph.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (BfsAtTime_NoTemporalEdges_ReturnsAllNeighbors): n/a

#### `TEST_F(TemporalGraphTest, DijkstraAtTime_EmptyNodes_ReturnsError)`
- Source: `tests/temporal/test_temporal_graph.cpp`:397
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (DijkstraAtTime_EmptyNodes_ReturnsError): n/a

#### `TEST_F(TemporalGraphTest, DijkstraAtTime_FindsShortestPathAtTime)`
- Source: `tests/temporal/test_temporal_graph.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (DijkstraAtTime_FindsShortestPathAtTime): n/a

#### `TEST_F(TemporalGraphTest, DijkstraAtTime_NoPathAtTime)`
- Source: `tests/temporal/test_temporal_graph.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (DijkstraAtTime_NoPathAtTime): n/a

#### `TEST_F(TemporalGraphTest, DijkstraAtTime_PathChangesOverTime)`
- Source: `tests/temporal/test_temporal_graph.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (DijkstraAtTime_PathChangesOverTime): n/a

#### `TEST_F(TemporalGraphTest, GetEdgesInTimeRange_FullContainment)`
- Source: `tests/temporal/test_temporal_graph.cpp`:596
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (GetEdgesInTimeRange_FullContainment): n/a

#### `TEST_F(TemporalGraphTest, GetEdgesInTimeRange_ReturnsOverlappingEdges)`
- Source: `tests/temporal/test_temporal_graph.cpp`:569
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (GetEdgesInTimeRange_ReturnsOverlappingEdges): n/a

#### `TEST_F(TemporalGraphTest, GetOutEdgesInTimeRange_EmptyNodeReturnsError)`
- Source: `tests/temporal/test_temporal_graph.cpp`:643
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (GetOutEdgesInTimeRange_EmptyNodeReturnsError): n/a

#### `TEST_F(TemporalGraphTest, GetOutEdgesInTimeRange_FiltersFromNode)`
- Source: `tests/temporal/test_temporal_graph.cpp`:618
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (GetOutEdgesInTimeRange_FiltersFromNode): n/a

#### `TEST_F(TemporalGraphTest, GetTemporalStats_DurationStatistics)`
- Source: `tests/temporal/test_temporal_graph.cpp`:665
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (GetTemporalStats_DurationStatistics): n/a

#### `TEST_F(TemporalGraphTest, GetTemporalStats_ReturnsCorrectCounts)`
- Source: `tests/temporal/test_temporal_graph.cpp`:648
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (GetTemporalStats_ReturnsCorrectCounts): n/a

#### `TEST_F(TemporalGraphTest, RealWorld_EmploymentHistory)`
- Source: `tests/temporal/test_temporal_graph.cpp`:417
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (RealWorld_EmploymentHistory): n/a

#### `TEST_F(TemporalGraphTest, RealWorld_KnowledgeGraphEvolution)`
- Source: `tests/temporal/test_temporal_graph.cpp`:446
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (RealWorld_KnowledgeGraphEvolution): n/a

#### `TEST_F(TemporalGraphTest, TemporalFilter_BoundaryConditions)`
- Source: `tests/temporal/test_temporal_graph.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TemporalFilter_BoundaryConditions): n/a

#### `TEST_F(TemporalGraphTest, TemporalFilter_NoFilter_AcceptsAll)`
- Source: `tests/temporal/test_temporal_graph.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TemporalFilter_NoFilter_AcceptsAll): n/a

#### `TEST_F(TemporalGraphTest, TemporalFilter_WithTimestamp_FiltersCorrectly)`
- Source: `tests/temporal/test_temporal_graph.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TemporalFilter_WithTimestamp_FiltersCorrectly): n/a

#### `TEST_F(TemporalGraphTest, TemporalStats_ToString_ContainsMetrics)`
- Source: `tests/temporal/test_temporal_graph.cpp`:685
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TemporalStats_ToString_ContainsMetrics): n/a

#### `TEST_F(TemporalGraphTest, TimeRangeFilter_FullyContains_DetectsContainment)`
- Source: `tests/temporal/test_temporal_graph.cpp`:529
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TimeRangeFilter_FullyContains_DetectsContainment): n/a

#### `TEST_F(TemporalGraphTest, TimeRangeFilter_HasOverlap_DetectsOverlap)`
- Source: `tests/temporal/test_temporal_graph.cpp`:488
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TimeRangeFilter_HasOverlap_DetectsOverlap): n/a

#### `TEST_F(TemporalGraphTest, TimeRangeFilter_HasOverlap_UnboundedEdge)`
- Source: `tests/temporal/test_temporal_graph.cpp`:510
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TimeRangeFilter_HasOverlap_UnboundedEdge): n/a

#### `TEST_F(TemporalGraphTest, TimeRangeFilter_NoFilter_AcceptsAll)`
- Source: `tests/temporal/test_temporal_graph.cpp`:479
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TimeRangeFilter_NoFilter_AcceptsAll): n/a

#### `TEST_F(TemporalGraphTest, TimeRangeFilter_Since_OneSidedBound)`
- Source: `tests/temporal/test_temporal_graph.cpp`:547
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TimeRangeFilter_Since_OneSidedBound): n/a

#### `TEST_F(TemporalGraphTest, TimeRangeFilter_Until_OneSidedBound)`
- Source: `tests/temporal/test_temporal_graph.cpp`:557
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalGraphTest): n/a
  - `<unnamed>` (TimeRangeFilter_Until_OneSidedBound): n/a

### test_temporal_highcardinality_stress.cpp

#### `TEST(TemporalHighCardinalityStress, TSTR01_HighCardinalityVersionedWrite)`
- Source: `tests/temporal/test_temporal_highcardinality_stress.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalHighCardinalityStress): n/a
  - `<unnamed>` (TSTR01_HighCardinalityVersionedWrite): n/a

#### `TEST(TemporalHighCardinalityStress, TSTR02_ConcurrentConflictResolutionStress)`
- Source: `tests/temporal/test_temporal_highcardinality_stress.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalHighCardinalityStress): n/a
  - `<unnamed>` (TSTR02_ConcurrentConflictResolutionStress): n/a

#### `TEST(TemporalHighCardinalityStress, TSTR03_CDCEdgeCaseStress)`
- Source: `tests/temporal/test_temporal_highcardinality_stress.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalHighCardinalityStress): n/a
  - `<unnamed>` (TSTR03_CDCEdgeCaseStress): n/a

### test_temporal_index.cpp

#### `TEST_F(TemporalIndexTest, Insert_Multiple_SizeIncreases)`
- Source: `tests/temporal/test_temporal_index.cpp`:31
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (Insert_Multiple_SizeIncreases): n/a

#### `TEST_F(TemporalIndexTest, Insert_Single_SizeBecomesOne)`
- Source: `tests/temporal/test_temporal_index.cpp`:26
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (Insert_Single_SizeBecomesOne): n/a

#### `TEST_F(TemporalIndexTest, QueryKey_ReturnsAllVersionsForKey)`
- Source: `tests/temporal/test_temporal_index.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (QueryKey_ReturnsAllVersionsForKey): n/a

#### `TEST_F(TemporalIndexTest, QueryKey_WithRange_FiltersVersions)`
- Source: `tests/temporal/test_temporal_index.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (QueryKey_WithRange_FiltersVersions): n/a

#### `TEST_F(TemporalIndexTest, QueryPoint_AtStart_ReturnsEntry)`
- Source: `tests/temporal/test_temporal_index.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (QueryPoint_AtStart_ReturnsEntry): n/a

#### `TEST_F(TemporalIndexTest, QueryPoint_MatchingEntries)`
- Source: `tests/temporal/test_temporal_index.cpp`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (QueryPoint_MatchingEntries): n/a

#### `TEST_F(TemporalIndexTest, QueryPoint_NoMatch_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_index.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (QueryPoint_NoMatch_ReturnsEmpty): n/a

#### `TEST_F(TemporalIndexTest, QueryRange_NoOverlap_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_index.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (QueryRange_NoOverlap_ReturnsEmpty): n/a

#### `TEST_F(TemporalIndexTest, QueryRange_OverlappingEntries)`
- Source: `tests/temporal/test_temporal_index.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (QueryRange_OverlappingEntries): n/a

#### `TEST_F(TemporalIndexTest, RemoveKey_RemovesAllVersions)`
- Source: `tests/temporal/test_temporal_index.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (RemoveKey_RemovesAllVersions): n/a

#### `TEST_F(TemporalIndexTest, Remove_ExistingEntry_SizeDecreases)`
- Source: `tests/temporal/test_temporal_index.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (Remove_ExistingEntry_SizeDecreases): n/a

#### `TEST_F(TemporalIndexTest, Stats_TotalEntries)`
- Source: `tests/temporal/test_temporal_index.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalIndexTest): n/a
  - `<unnamed>` (Stats_TotalEntries): n/a

### test_temporal_migrator.cpp

#### `TEST_F(TemporalMigratorTest, AfterMigrate_StatusIsComplete)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:470
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (AfterMigrate_StatusIsComplete): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_BaselineTimestampIsPositive)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_BaselineTimestampIsPositive): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_DefaultHistoryTableName)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_DefaultHistoryTableName): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_DetectsEmptyDocument)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_DetectsEmptyDocument): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_DetectsNullableColumn)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_DetectsNullableColumn): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_EmptySource_ZeroRows)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_EmptySource_ZeroRows): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_InfersBooleanColumn)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_InfersBooleanColumn): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_InfersNumberColumn)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_InfersNumberColumn): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_InfersStringColumn)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_InfersStringColumn): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_KeysAreUniqueForMapInput)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_KeysAreUniqueForMapInput): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_NoEmptyDocumentWhenAllPopulated)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_NoEmptyDocumentWhenAllPopulated): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_NonNullableWhenAllPresent)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_NonNullableWhenAllPresent): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_ReturnsCorrectRowCount)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_ReturnsCorrectRowCount): n/a

#### `TEST_F(TemporalMigratorTest, Analyze_ReturnsCorrectTableName)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Analyze_ReturnsCorrectTableName): n/a

#### `TEST_F(TemporalMigratorTest, Backfill_AcceptsClosedVersions)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Backfill_AcceptsClosedVersions): n/a

#### `TEST_F(TemporalMigratorTest, Backfill_EmptyList_ReturnsZero)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Backfill_EmptyList_ReturnsZero): n/a

#### `TEST_F(TemporalMigratorTest, Backfill_SkipsOpenEndedVersions)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Backfill_SkipsOpenEndedVersions): n/a

#### `TEST_F(TemporalMigratorTest, Backfill_StatsVersionsBackfilledAccessible)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Backfill_StatsVersionsBackfilledAccessible): n/a

#### `TEST_F(TemporalMigratorTest, GetLastReport_AfterVerify_MatchesReturnValue)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:537
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (GetLastReport_AfterVerify_MatchesReturnValue): n/a

#### `TEST_F(TemporalMigratorTest, GetLastReport_BeforeVerify_IsDefault)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:530
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (GetLastReport_BeforeVerify_IsDefault): n/a

#### `TEST_F(TemporalMigratorTest, InitialStatus_IsPending)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:429
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (InitialStatus_IsPending): n/a

#### `TEST_F(TemporalMigratorTest, Migrate_AllRowsInserted)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Migrate_AllRowsInserted): n/a

#### `TEST_F(TemporalMigratorTest, Migrate_CreatesTableWithCorrectName)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Migrate_CreatesTableWithCorrectName): n/a

#### `TEST_F(TemporalMigratorTest, Migrate_ElapsedMsIsNonNegative)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Migrate_ElapsedMsIsNonNegative): n/a

#### `TEST_F(TemporalMigratorTest, Migrate_EmptySource_SucceedsWithZeroRows)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Migrate_EmptySource_SucceedsWithZeroRows): n/a

#### `TEST_F(TemporalMigratorTest, Migrate_PreservesDocumentData)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Migrate_PreservesDocumentData): n/a

#### `TEST_F(TemporalMigratorTest, Migrate_RowsAreCurrent)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Migrate_RowsAreCurrent): n/a

#### `TEST_F(TemporalMigratorTest, Migrate_StatsRowsMigratedEquals5)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Migrate_StatsRowsMigratedEquals5): n/a

#### `TEST_F(TemporalMigratorTest, Migrate_StatusIsCompleteOnSuccess)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Migrate_StatusIsCompleteOnSuccess): n/a

#### `TEST_F(TemporalMigratorTest, MigrationPlan_ToJson_HasColumns)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:495
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (MigrationPlan_ToJson_HasColumns): n/a

#### `TEST_F(TemporalMigratorTest, MigrationPlan_ToJson_HasSourceRowCount)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:488
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (MigrationPlan_ToJson_HasSourceRowCount): n/a

#### `TEST_F(TemporalMigratorTest, MigrationPlan_ToJson_HasSourceTableName)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:481
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (MigrationPlan_ToJson_HasSourceTableName): n/a

#### `TEST_F(TemporalMigratorTest, MigrationReport_ToJson_HasChecksArray)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:512
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (MigrationReport_ToJson_HasChecksArray): n/a

#### `TEST_F(TemporalMigratorTest, MigrationReport_ToJson_HasSuccessField)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:503
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (MigrationReport_ToJson_HasSuccessField): n/a

#### `TEST_F(TemporalMigratorTest, MigrationStats_ToJson_HasRowsMigrated)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:521
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (MigrationStats_ToJson_HasRowsMigrated): n/a

#### `TEST_F(TemporalMigratorTest, ProgressCallback_CanBeCleared)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:455
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (ProgressCallback_CanBeCleared): n/a

#### `TEST_F(TemporalMigratorTest, ProgressCallback_InvokedDuringMigration)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:442
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (ProgressCallback_InvokedDuringMigration): n/a

#### `TEST_F(TemporalMigratorTest, StatusName_ReturnsExpectedStrings)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:433
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (StatusName_ReturnsExpectedStrings): n/a

#### `TEST_F(TemporalMigratorTest, Verify_ChecksContainCurrentVersionOpen)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Verify_ChecksContainCurrentVersionOpen): n/a

#### `TEST_F(TemporalMigratorTest, Verify_ChecksContainKeyCount)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Verify_ChecksContainKeyCount): n/a

#### `TEST_F(TemporalMigratorTest, Verify_ChecksContainNoOverlapping)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:362
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Verify_ChecksContainNoOverlapping): n/a

#### `TEST_F(TemporalMigratorTest, Verify_ChecksContainVersionOrder)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:348
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Verify_ChecksContainVersionOrder): n/a

#### `TEST_F(TemporalMigratorTest, Verify_EmptyTable_StillPasses)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:390
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Verify_EmptyTable_StillPasses): n/a

#### `TEST_F(TemporalMigratorTest, Verify_FailedCheckCountReflectsIssues)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Verify_FailedCheckCountReflectsIssues): n/a

#### `TEST_F(TemporalMigratorTest, Verify_KeyCountCheckPassesWhenEqual)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:410
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Verify_KeyCountCheckPassesWhenEqual): n/a

#### `TEST_F(TemporalMigratorTest, Verify_PassesForFreshMigration)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Verify_PassesForFreshMigration): n/a

#### `TEST_F(TemporalMigratorTest, Verify_ReportHasCorrectTableName)`
- Source: `tests/temporal/test_temporal_migrator.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigratorTest): n/a
  - `<unnamed>` (Verify_ReportHasCorrectTableName): n/a

### test_temporal_query_engine.cpp

#### `TEST(SequencedDistinctTest, SD_01_NoRows_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:776
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequencedDistinctTest): n/a
  - `<unnamed>` (SD_01_NoRows_ReturnsEmpty): n/a

#### `TEST(SequencedDistinctTest, SD_02_SingleVersion_PassesThrough)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:782
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequencedDistinctTest): n/a
  - `<unnamed>` (SD_02_SingleVersion_PassesThrough): n/a

#### `TEST(SequencedDistinctTest, SD_03_AdjacentSameData_Coalesced)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:790
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequencedDistinctTest): n/a
  - `<unnamed>` (SD_03_AdjacentSameData_Coalesced): n/a

#### `TEST(SequencedDistinctTest, SD_04_DifferentData_NotCoalesced)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:807
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequencedDistinctTest): n/a
  - `<unnamed>` (SD_04_DifferentData_NotCoalesced): n/a

#### `TEST(SequencedDistinctTest, SD_05_ForKey_OnlyOneKey)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:823
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequencedDistinctTest): n/a
  - `<unnamed>` (SD_05_ForKey_OnlyOneKey): n/a

#### `TEST(SequencedDistinctTest, SD_06_EmptyCompareFields_ComparesAll)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:834
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequencedDistinctTest): n/a
  - `<unnamed>` (SD_06_EmptyCompareFields_ComparesAll): n/a

#### `TEST(TemporalQuerySpecTest, All_Factory_SetsFlagAndRange)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:766
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQuerySpecTest): n/a
  - `<unnamed>` (All_Factory_SetsFlagAndRange): n/a

#### `TEST(TemporalQuerySpecTest, AsOf_Factory_SetsCorrectClause)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:738
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQuerySpecTest): n/a
  - `<unnamed>` (AsOf_Factory_SetsCorrectClause): n/a

#### `TEST(TemporalQuerySpecTest, BetweenAnd_Factory_SetsCorrectBounds)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:752
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQuerySpecTest): n/a
  - `<unnamed>` (BetweenAnd_Factory_SetsCorrectBounds): n/a

#### `TEST(TemporalQuerySpecTest, ContainedIn_Factory_SetsCorrectBounds)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:759
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQuerySpecTest): n/a
  - `<unnamed>` (ContainedIn_Factory_SetsCorrectBounds): n/a

#### `TEST(TemporalQuerySpecTest, FromTo_Factory_SetsCorrectBounds)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:745
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQuerySpecTest): n/a
  - `<unnamed>` (FromTo_Factory_SetsCorrectBounds): n/a

#### `TEST_F(ApplicationTimeTest, QueryApplicationTimeRange_FullRange_ReturnsAllRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:427
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApplicationTimeTest): n/a
  - `<unnamed>` (QueryApplicationTimeRange_FullRange_ReturnsAllRows): n/a

#### `TEST_F(ApplicationTimeTest, QueryApplicationTimeRange_NoOverlap_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApplicationTimeTest): n/a
  - `<unnamed>` (QueryApplicationTimeRange_NoOverlap_ReturnsEmpty): n/a

#### `TEST_F(ApplicationTimeTest, QueryApplicationTimeRange_OverlappingRange_ReturnsMatchingRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:414
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApplicationTimeTest): n/a
  - `<unnamed>` (QueryApplicationTimeRange_OverlappingRange_ReturnsMatchingRows): n/a

#### `TEST_F(ApplicationTimeTest, QueryApplicationTime_PointAfterAllExpiry_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:400
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApplicationTimeTest): n/a
  - `<unnamed>` (QueryApplicationTime_PointAfterAllExpiry_ReturnsEmpty): n/a

#### `TEST_F(ApplicationTimeTest, QueryApplicationTime_PointInRange_ReturnsActiveRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApplicationTimeTest): n/a
  - `<unnamed>` (QueryApplicationTime_PointInRange_ReturnsActiveRows): n/a

#### `TEST_F(ApplicationTimeTest, QueryApplicationTime_WithFilter_ReturnsFilteredRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (ApplicationTimeTest): n/a
  - `<unnamed>` (QueryApplicationTime_WithFilter_ReturnsFilteredRows): n/a

#### `TEST_F(ExecuteBiTemporalQueryTest, BT_SpecAll_ReturnsAllCurrentRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:713
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteBiTemporalQueryTest): n/a
  - `<unnamed>` (BT_SpecAll_ReturnsAllCurrentRows): n/a

#### `TEST_F(ExecuteBiTemporalQueryTest, BT_SpecAsOf_NoActiveRows_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:719
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteBiTemporalQueryTest): n/a
  - `<unnamed>` (BT_SpecAsOf_NoActiveRows_ReturnsEmpty): n/a

#### `TEST_F(ExecuteBiTemporalQueryTest, BT_SpecAsOf_PointInRange_ReturnsActiveRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:655
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteBiTemporalQueryTest): n/a
  - `<unnamed>` (BT_SpecAsOf_PointInRange_ReturnsActiveRows): n/a

#### `TEST_F(ExecuteBiTemporalQueryTest, BT_SpecBetweenAnd_ClosedBound_IncludesEndpoint)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:680
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteBiTemporalQueryTest): n/a
  - `<unnamed>` (BT_SpecBetweenAnd_ClosedBound_IncludesEndpoint): n/a

#### `TEST_F(ExecuteBiTemporalQueryTest, BT_SpecContainedIn_OnlyFullyContainedRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:694
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteBiTemporalQueryTest): n/a
  - `<unnamed>` (BT_SpecContainedIn_OnlyFullyContainedRows): n/a

#### `TEST_F(ExecuteBiTemporalQueryTest, BT_SpecFromTo_OverlapRange_ReturnsOverlappingRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:673
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteBiTemporalQueryTest): n/a
  - `<unnamed>` (BT_SpecFromTo_OverlapRange_ReturnsOverlappingRows): n/a

#### `TEST_F(ExecuteBiTemporalQueryTest, BT_SpecFromTo_WithFilter_ReturnsFilteredRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:726
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteBiTemporalQueryTest): n/a
  - `<unnamed>` (BT_SpecFromTo_WithFilter_ReturnsFilteredRows): n/a

#### `TEST_F(ExecuteTemporalQueryTest, SpecAll_ReturnsAllVersionsIncludingDeleted)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:623
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteTemporalQueryTest): n/a
  - `<unnamed>` (SpecAll_ReturnsAllVersionsIncludingDeleted): n/a

#### `TEST_F(ExecuteTemporalQueryTest, SpecAsOf_IncludeDeleted_ReturnsBoth)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:576
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteTemporalQueryTest): n/a
  - `<unnamed>` (SpecAsOf_IncludeDeleted_ReturnsBoth): n/a

#### `TEST_F(ExecuteTemporalQueryTest, SpecAsOf_Now_ReturnsBothCurrentRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:568
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteTemporalQueryTest): n/a
  - `<unnamed>` (SpecAsOf_Now_ReturnsBothCurrentRows): n/a

#### `TEST_F(ExecuteTemporalQueryTest, SpecAsOf_WithFilter_FiltersResult)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:630
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteTemporalQueryTest): n/a
  - `<unnamed>` (SpecAsOf_WithFilter_FiltersResult): n/a

#### `TEST_F(ExecuteTemporalQueryTest, SpecBetweenAnd_EmptyRange_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:606
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteTemporalQueryTest): n/a
  - `<unnamed>` (SpecBetweenAnd_EmptyRange_ReturnsEmpty): n/a

#### `TEST_F(ExecuteTemporalQueryTest, SpecBetweenAnd_FullRange_ReturnsAll)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:599
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteTemporalQueryTest): n/a
  - `<unnamed>` (SpecBetweenAnd_FullRange_ReturnsAll): n/a

#### `TEST_F(ExecuteTemporalQueryTest, SpecContainedIn_FullRange_ReturnsVersionsContained)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:613
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteTemporalQueryTest): n/a
  - `<unnamed>` (SpecContainedIn_FullRange_ReturnsVersionsContained): n/a

#### `TEST_F(ExecuteTemporalQueryTest, SpecFromTo_FullRange_ReturnsAllVersions)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:583
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteTemporalQueryTest): n/a
  - `<unnamed>` (SpecFromTo_FullRange_ReturnsAllVersions): n/a

#### `TEST_F(ExecuteTemporalQueryTest, SpecFromTo_NarrowRange_ReturnsVersionsInRange)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:591
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecuteTemporalQueryTest): n/a
  - `<unnamed>` (SpecFromTo_NarrowRange_ReturnsVersionsInRange): n/a

#### `TEST_F(TemporalQueryEngineTest, Intersect_NoOverlap_ReturnsEmptyRange)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (Intersect_NoOverlap_ReturnsEmptyRange): n/a

#### `TEST_F(TemporalQueryEngineTest, Intersect_Overlap_ReturnsOverlapRange)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (Intersect_Overlap_ReturnsOverlapRange): n/a

#### `TEST_F(TemporalQueryEngineTest, JoinAsOf_EmptyTable_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (JoinAsOf_EmptyTable_ReturnsEmpty): n/a

#### `TEST_F(TemporalQueryEngineTest, JoinAsOf_MatchingRows_ReturnsJoinedPairs)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (JoinAsOf_MatchingRows_ReturnsJoinedPairs): n/a

#### `TEST_F(TemporalQueryEngineTest, JoinAsOf_NoMatch_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (JoinAsOf_NoMatch_ReturnsEmpty): n/a

#### `TEST_F(TemporalQueryEngineTest, JoinBiTemporal_DifferentValidTimes_PartialMatch)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (JoinBiTemporal_DifferentValidTimes_PartialMatch): n/a

#### `TEST_F(TemporalQueryEngineTest, JoinBiTemporal_EmptyTables_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (JoinBiTemporal_EmptyTables_ReturnsEmpty): n/a

#### `TEST_F(TemporalQueryEngineTest, JoinBiTemporal_MatchingRows_ReturnsPairs)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (JoinBiTemporal_MatchingRows_ReturnsPairs): n/a

#### `TEST_F(TemporalQueryEngineTest, JoinBiTemporal_ValidTimeOutOfRange_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (JoinBiTemporal_ValidTimeOutOfRange_ReturnsEmpty): n/a

#### `TEST_F(TemporalQueryEngineTest, Predicate_Contains_Period)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (Predicate_Contains_Period): n/a

#### `TEST_F(TemporalQueryEngineTest, Predicate_Equals)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (Predicate_Equals): n/a

#### `TEST_F(TemporalQueryEngineTest, Predicate_Meets)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (Predicate_Meets): n/a

#### `TEST_F(TemporalQueryEngineTest, Predicate_Overlaps_False)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (Predicate_Overlaps_False): n/a

#### `TEST_F(TemporalQueryEngineTest, Predicate_Overlaps_True)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (Predicate_Overlaps_True): n/a

#### `TEST_F(TemporalQueryEngineTest, Predicate_Precedes)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (Predicate_Precedes): n/a

#### `TEST_F(TemporalQueryEngineTest, Predicate_Succeeds)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (Predicate_Succeeds): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryAsOfCached_SecondCallHitsCache)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:520
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryAsOfCached_SecondCallHitsCache): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryAsOfCached_WithFilter_FiltersPostCache)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:534
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryAsOfCached_WithFilter_FiltersPostCache): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryAsOfWithIndex_EmptyIndex_FallsBackToFullScan)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:448
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryAsOfWithIndex_EmptyIndex_FallsBackToFullScan): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryAsOfWithIndex_PopulatedIndexNoMatch_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:455
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryAsOfWithIndex_PopulatedIndexNoMatch_ReturnsEmpty): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryAsOfWithIndex_PopulatedIndex_ReturnsCorrectRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:435
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryAsOfWithIndex_PopulatedIndex_ReturnsCorrectRows): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryAsOf_FilterNoMatch_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryAsOf_FilterNoMatch_ReturnsEmpty): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryAsOf_Now_ReturnsBothCurrentRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:32
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryAsOf_Now_ReturnsBothCurrentRows): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryAsOf_WithFilter_ReturnsMatchingRows)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryAsOf_WithFilter_ReturnsMatchingRows): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryBetween_BeforeAnyInsert_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryBetween_BeforeAnyInsert_ReturnsEmpty): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryBetween_FullRange_ReturnsAllVersions)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryBetween_FullRange_ReturnsAllVersions): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryBetween_NarrowRange_ReturnsVersionsInRange)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryBetween_NarrowRange_ReturnsVersionsInRange): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryBetween_WithFilter_ReturnsFilteredVersions)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:356
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryBetween_WithFilter_ReturnsFilteredVersions): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryCache_Clear_RemovesAllEntries)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:496
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryCache_Clear_RemovesAllEntries): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryCache_EvictsLRUWhenFull)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryCache_EvictsLRUWhenFull): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryCache_InvalidateByTable)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:486
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryCache_InvalidateByTable): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryCache_MissAndHit)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:469
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryCache_MissAndHit): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryFromTo_FullRange_ReturnsAllVersions)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryFromTo_FullRange_ReturnsAllVersions): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryKeyFromTo_ReturnsVersionsInRange)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryKeyFromTo_ReturnsVersionsInRange): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryNonSequenced_ReturnsAllVersions)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryNonSequenced_ReturnsAllVersions): n/a

#### `TEST_F(TemporalQueryEngineTest, QueryNonSequenced_WithFilter_FiltersAcrossAllVersions)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QueryNonSequenced_WithFilter_FiltersAcrossAllVersions): n/a

#### `TEST_F(TemporalQueryEngineTest, QuerySequenced_EmptyPeriod_ReturnsEmpty)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:325
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QuerySequenced_EmptyPeriod_ReturnsEmpty): n/a

#### `TEST_F(TemporalQueryEngineTest, QuerySequenced_PeriodOverlap_ReturnsVersionsInRange)`
- Source: `tests/temporal/test_temporal_query_engine.cpp`:284
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalQueryEngineTest): n/a
  - `<unnamed>` (QuerySequenced_PeriodOverlap_ReturnsVersionsInRange): n/a

### test_temporal_tier_manager.cpp

#### `TEST(BloomFilterTierManagerTest, EmptyHasNoFalseNegativesOnInserted)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (BloomFilterTierManagerTest): n/a
  - `<unnamed>` (EmptyHasNoFalseNegativesOnInserted): n/a

#### `TEST(BloomFilterTierManagerTest, FalsePositiveRateAcceptable)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (BloomFilterTierManagerTest): n/a
  - `<unnamed>` (FalsePositiveRateAcceptable): n/a

#### `TEST(BloomFilterTierManagerTest, InsertedAlwaysFound)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (BloomFilterTierManagerTest): n/a
  - `<unnamed>` (InsertedAlwaysFound): n/a

#### `TEST(TierManagerColdTest, CompactTable_RespectsWarmMaxBlocks)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerColdTest): n/a
  - `<unnamed>` (CompactTable_RespectsWarmMaxBlocks): n/a

#### `TEST(TierManagerColdTest, FlushWarmToCold_MoveToColddStore)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerColdTest): n/a
  - `<unnamed>` (FlushWarmToCold_MoveToColddStore): n/a

#### `TEST(TierManagerColdTest, GetAsOf_ResolvesFromColdStore)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerColdTest): n/a
  - `<unnamed>` (GetAsOf_ResolvesFromColdStore): n/a

#### `TEST(TierManagerConcurrencyTest, ConcurrentReads_NoDeadlock)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:409
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerConcurrencyTest): n/a
  - `<unnamed>` (ConcurrentReads_NoDeadlock): n/a

#### `TEST(TierPolicyTest, DecisionFnClearRevertsToThreshold)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierPolicyTest): n/a
  - `<unnamed>` (DecisionFnClearRevertsToThreshold): n/a

#### `TEST(TierPolicyTest, DecisionFnHookOverridesThreshold)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierPolicyTest): n/a
  - `<unnamed>` (DecisionFnHookOverridesThreshold): n/a

#### `TEST(TierPolicyTest, DecisionFnReceivesCorrectContext)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierPolicyTest): n/a
  - `<unnamed>` (DecisionFnReceivesCorrectContext): n/a

#### `TEST(TierPolicyTest, EvaluateFlushHotToWarm)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierPolicyTest): n/a
  - `<unnamed>` (EvaluateFlushHotToWarm): n/a

#### `TEST(TierPolicyTest, EvaluateFlushWarmToCold_BlockCap)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierPolicyTest): n/a
  - `<unnamed>` (EvaluateFlushWarmToCold_BlockCap): n/a

#### `TEST(TierPolicyTest, EvaluateKeep)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierPolicyTest): n/a
  - `<unnamed>` (EvaluateKeep): n/a

#### `TEST_F(ThreeTierQueryTest, GetAsOf_AnyTierResolvable)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:377
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeTierQueryTest): n/a
  - `<unnamed>` (GetAsOf_AnyTierResolvable): n/a

#### `TEST_F(ThreeTierQueryTest, GetHistoryInRange_Filtered)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:387
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeTierQueryTest): n/a
  - `<unnamed>` (GetHistoryInRange_Filtered): n/a

#### `TEST_F(ThreeTierQueryTest, GetHistory_AllVersionsReturned)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeTierQueryTest): n/a
  - `<unnamed>` (GetHistory_AllVersionsReturned): n/a

#### `TEST_F(ThreeTierQueryTest, StatsJson_AllTiersCounted)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:395
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeTierQueryTest): n/a
  - `<unnamed>` (StatsJson_AllTiersCounted): n/a

#### `TEST_F(ThreeTierQueryTest, UnknownKey_NullOptAndEmpty)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:401
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreeTierQueryTest): n/a
  - `<unnamed>` (UnknownKey_NullOptAndEmpty): n/a

#### `TEST_F(TierManagerTest, FlushHotToWarm_BlockMetadata)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerTest): n/a
  - `<unnamed>` (FlushHotToWarm_BlockMetadata): n/a

#### `TEST_F(TierManagerTest, FlushHotToWarm_BloomFilterPopulated)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:250
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerTest): n/a
  - `<unnamed>` (FlushHotToWarm_BloomFilterPopulated): n/a

#### `TEST_F(TierManagerTest, FlushHotToWarm_GetAsOfResolvedFromWarm)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerTest): n/a
  - `<unnamed>` (FlushHotToWarm_GetAsOfResolvedFromWarm): n/a

#### `TEST_F(TierManagerTest, FlushHotToWarm_MovesVersions)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerTest): n/a
  - `<unnamed>` (FlushHotToWarm_MovesVersions): n/a

#### `TEST_F(TierManagerTest, HotTier_CurrentVersionRejected)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerTest): n/a
  - `<unnamed>` (HotTier_CurrentVersionRejected): n/a

#### `TEST_F(TierManagerTest, HotTier_GetAsOf_BeforeAll_NullOpt)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerTest): n/a
  - `<unnamed>` (HotTier_GetAsOf_BeforeAll_NullOpt): n/a

#### `TEST_F(TierManagerTest, HotTier_InsertAndGetAsOf)`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (TierManagerTest): n/a
  - `<unnamed>` (HotTier_InsertAndGetAsOf): n/a

#### `VersionedDocument makeDoc(const std::string &key, Timestamp start, Timestamp end, const std::string &val="v")`
- Source: `tests/temporal/test_temporal_tier_manager.cpp`:26
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `start` (Timestamp): n/a
  - `end` (Timestamp): n/a
  - `val` (const std::string &): n/a

### test_temporal_v18_v19.cpp

#### `TEST(CDCPersistentLogV18V19Test, CDCPL_01_OpenCreatesDirectory)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogV18V19Test): n/a
  - `<unnamed>` (CDCPL_01_OpenCreatesDirectory): n/a

#### `TEST(CDCPersistentLogV18V19Test, CDCPL_02_AppendReplayRoundTrip)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogV18V19Test): n/a
  - `<unnamed>` (CDCPL_02_AppendReplayRoundTrip): n/a

#### `TEST(CDCPersistentLogV18V19Test, CDCPL_03_ReplayFiltersTable)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogV18V19Test): n/a
  - `<unnamed>` (CDCPL_03_ReplayFiltersTable): n/a

#### `TEST(CDCPersistentLogV18V19Test, CDCPL_04_ReplayFiltersTimeRange)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogV18V19Test): n/a
  - `<unnamed>` (CDCPL_04_ReplayFiltersTimeRange): n/a

#### `TEST(CDCPersistentLogV18V19Test, CDCPL_05_TotalBytesWritten)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogV18V19Test): n/a
  - `<unnamed>` (CDCPL_05_TotalBytesWritten): n/a

#### `TEST(CDCPersistentLogV18V19Test, CDCPL_06_SegmentCount)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogV18V19Test): n/a
  - `<unnamed>` (CDCPL_06_SegmentCount): n/a

#### `TEST(CDCPersistentLogV18V19Test, CDCPL_07_SegmentRotation)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogV18V19Test): n/a
  - `<unnamed>` (CDCPL_07_SegmentRotation): n/a

#### `TEST(CDCPersistentLogV18V19Test, CDCPL_08_AppendOnClosedLogThrows)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCPersistentLogV18V19Test): n/a
  - `<unnamed>` (CDCPL_08_AppendOnClosedLogThrows): n/a

#### `TEST_F(BiTemporalMergeV18V19Test, BTM_01_MergeEmptyRemote)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeV18V19Test): n/a
  - `<unnamed>` (BTM_01_MergeEmptyRemote): n/a

#### `TEST_F(BiTemporalMergeV18V19Test, BTM_02_RemoteWinsLWW)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeV18V19Test): n/a
  - `<unnamed>` (BTM_02_RemoteWinsLWW): n/a

#### `TEST_F(BiTemporalMergeV18V19Test, BTM_03_LocalWinsIfNewer)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeV18V19Test): n/a
  - `<unnamed>` (BTM_03_LocalWinsIfNewer): n/a

#### `TEST_F(BiTemporalMergeV18V19Test, BTM_04_NewKeyFromRemote)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:328
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeV18V19Test): n/a
  - `<unnamed>` (BTM_04_NewKeyFromRemote): n/a

#### `TEST_F(BiTemporalMergeV18V19Test, BTM_05_TableNameMismatch)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:337
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeV18V19Test): n/a
  - `<unnamed>` (BTM_05_TableNameMismatch): n/a

#### `TEST_F(BiTemporalMergeV18V19Test, BTM_06_MergeResultStats)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalMergeV18V19Test): n/a
  - `<unnamed>` (BTM_06_MergeResultStats): n/a

#### `TEST_F(SnapshotDiffV18V19Test, SD2_01_IdenticalSnapshots)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffV18V19Test): n/a
  - `<unnamed>` (SD2_01_IdenticalSnapshots): n/a

#### `TEST_F(SnapshotDiffV18V19Test, SD2_02_AddedRow)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffV18V19Test): n/a
  - `<unnamed>` (SD2_02_AddedRow): n/a

#### `TEST_F(SnapshotDiffV18V19Test, SD2_03_RemovedRow)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffV18V19Test): n/a
  - `<unnamed>` (SD2_03_RemovedRow): n/a

#### `TEST_F(SnapshotDiffV18V19Test, SD2_04_ModifiedRow)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffV18V19Test): n/a
  - `<unnamed>` (SD2_04_ModifiedRow): n/a

#### `TEST_F(SnapshotDiffV18V19Test, SD2_05_InvalidHandle)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:250
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffV18V19Test): n/a
  - `<unnamed>` (SD2_05_InvalidHandle): n/a

#### `TEST_F(SnapshotDiffV18V19Test, SD2_06_ToJson)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotDiffV18V19Test): n/a
  - `<unnamed>` (SD2_06_ToJson): n/a

#### `ChangeEvent makeEv(const std::string &table, ChangeType type, const std::string &entity_id, Timestamp ts=1000)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:28
- Brief: n/a
- Parameters:
  - `table` (const std::string &): n/a
  - `type` (ChangeType): n/a
  - `entity_id` (const std::string &): n/a
  - `ts` (Timestamp): n/a

#### `std::vector< ChangeEvent > replayFiltered(const CDCPersistentLog &log, const std::string &table, const TimeRange &range)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:54
- Brief: n/a
- Parameters:
  - `log` (const CDCPersistentLog &): n/a
  - `table` (const std::string &): n/a
  - `range` (const TimeRange &): n/a

#### `std::string tempDir(const std::string &suffix)`
- Source: `tests/temporal/test_temporal_v18_v19.cpp`:44
- Brief: n/a
- Parameters:
  - `suffix` (const std::string &): n/a

### themis::bench::temporal_dedicated

#### `void BM_Temporal_BatchCdcEmit(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:126
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Temporal_ConflictResolve(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:109
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Temporal_HistoryQuery(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:76
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Temporal_VersionedWrite(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:93
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Name("TMP-BM-01/HistoryQuery") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TMP-BM-01/HistoryQuery"): n/a

#### `Name("TMP-BM-02/VersionedWrite") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TMP-BM-02/VersionedWrite"): n/a

#### `Name("TMP-BM-03/ConflictResolve") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TMP-BM-03/ConflictResolve"): n/a

#### `Name("TMP-BM-04/BatchCdcEmit1000") -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` ("TMP-BM-04/BatchCdcEmit1000"): n/a

#### `bool stubCdcEmit(uint64_t entity_id, uint64_t ts_ms) noexcept`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:68
- Brief: n/a
- Parameters:
  - `entity_id` (uint64_t): n/a
  - `ts_ms` (uint64_t): n/a

#### `uint64_t stubConflictResolve(uint64_t ver_a, uint64_t ver_b) noexcept`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:64
- Brief: n/a
- Parameters:
  - `ver_a` (uint64_t): n/a
  - `ver_b` (uint64_t): n/a

#### `uint64_t stubHistoryQuery(uint64_t entity_id, uint64_t range_ms) noexcept`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:55
- Brief: n/a
- Parameters:
  - `entity_id` (uint64_t): n/a
  - `range_ms` (uint64_t): n/a

#### `uint64_t stubVersionedWrite(uint64_t entity_id) noexcept`
- Source: `benchmarks/temporal/bench_temporal_dedicated_gates.cpp`:59
- Brief: n/a
- Parameters:
  - `entity_id` (uint64_t): n/a

### themis::bench::trg

#### `void BM_TRG01_BiTemporalInsert(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:147
- Brief: TRG-01: In-memory bi-temporal insert throughput.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TRG-01: ≥ 100k inserts/s.

#### `void BM_TRG02_IntervalTreeQuery(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:179
- Brief: TRG-02: Point query over 1k pre-loaded intervals.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TRG-02: p99 ≤ 500 µs.

#### `void BM_TRG03_SnapshotRead(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:212
- Brief: TRG-03: Snapshot read over 100 bi-temporal rows.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TRG-03: p99 ≤ 1 ms.

#### `void BM_TRG04_RetentionCheck(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:242
- Brief: TRG-04: Retention boundary check for a single row.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TRG-04: p99 ≤ 100 µs.

#### `void BM_TRG05_PitrRestore(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:268
- Brief: TRG-05: PITR restore scan over 10 history entries.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TRG-05: p99 ≤ 5 ms.

#### `void BM_TRG06_ValidTimeRange(benchmark::State &state)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:299
- Brief: TRG-06: Valid-time interval intersection length computation.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TRG-06: p99 ≤ 50 µs.

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `void btInsert(std::vector< BiTemporalEntry > &store, int id, std::int64_t vs, std::int64_t ve, std::int64_t tx)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:84
- Brief: Simulates an in-memory bi-temporal insert (vector push_back).
- Parameters:
  - `store` (std::vector< BiTemporalEntry > &): n/a
  - `id` (int): n/a
  - `vs` (std::int64_t): n/a
  - `ve` (std::int64_t): n/a
  - `tx` (std::int64_t): n/a

#### `std::size_t intervalQuery(const std::vector< BiTemporalEntry > &store, std::int64_t point)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:90
- Brief: Simulates interval tree point query: linear scan over sorted intervals.
- Parameters:
  - `store` (const std::vector< BiTemporalEntry > &): n/a
  - `point` (std::int64_t): n/a

#### `bool isExpired(std::int64_t valid_end, std::int64_t boundary)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:114
- Brief: Simulates retention check for a single row.
- Parameters:
  - `valid_end` (std::int64_t): n/a
  - `boundary` (std::int64_t): n/a

#### `std::size_t pitrRestore(const std::vector< BiTemporalEntry > &history, std::int64_t target_tx)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:119
- Brief: Simulates PITR restore over a small history (linear scan).
- Parameters:
  - `history` (const std::vector< BiTemporalEntry > &): n/a
  - `target_tx` (std::int64_t): n/a

#### `std::size_t snapshotRead(const std::vector< BiTemporalEntry > &store, std::int64_t t, std::int64_t snapshot_tx)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:102
- Brief: Simulates snapshot read: filter by valid_time ∩ {T} and tx_time ≤ snapshot_tx.
- Parameters:
  - `store` (const std::vector< BiTemporalEntry > &): n/a
  - `t` (std::int64_t): n/a
  - `snapshot_tx` (std::int64_t): n/a

#### `std::int64_t validTimeIntersection(std::int64_t as, std::int64_t ae, std::int64_t bs, std::int64_t be)`
- Source: `benchmarks/temporal/bench_temporal_release_gates.cpp`:131
- Brief: Computes a valid-time interval intersection length (ns).
- Parameters:
  - `as` (std::int64_t): n/a
  - `ae` (std::int64_t): n/a
  - `bs` (std::int64_t): n/a
  - `be` (std::int64_t): n/a

### themis::temporal

#### `bool isHardTemporalError(TemporalErrorCode code) noexcept`
- Source: `include/temporal/temporal_api_contract.h`:156
- Brief: Returns true when the error code is a non-retryable hard error.
- Parameters:
  - `code` (TemporalErrorCode): n/a

#### `bool isLifecycleError(TemporalErrorCode code) noexcept`
- Source: `include/temporal/temporal_api_contract.h`:167
- Brief: Returns true when the error code is a recoverable lifecycle error (e.g., expired snapshot, PITR anchor unavailable).
- Parameters:
  - `code` (TemporalErrorCode): n/a

### themis::temporal::test

#### `TEST(TemporalContractHardening, TCH01_ValidRangeAccepted)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:142
- Brief: TCH-01: A well-formed bi-temporal row (valid range) is accepted.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH01_ValidRangeAccepted): n/a

#### `TEST(TemporalContractHardening, TCH02_InvalidRangeRejected)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:148
- Brief: TCH-02: A row with end < start raises TEMPORAL_RANGE_INVALID.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH02_InvalidRangeRejected): n/a

#### `TEST(TemporalContractHardening, TCH03_TransactionTimeOrdering)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:156
- Brief: TCH-03: Later insertions carry strictly higher transaction_time.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH03_TransactionTimeOrdering): n/a

#### `TEST(TemporalContractHardening, TCH04_OverlappingIntervalsReturned)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:169
- Brief: TCH-04: Two overlapping valid_time intervals are both returned by a point query.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH04_OverlappingIntervalsReturned): n/a

#### `TEST(TemporalContractHardening, TCH05_SnapshotAtTConsistent)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:190
- Brief: TCH-05: Snapshot at T returns rows with valid_time containing T.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH05_SnapshotAtTConsistent): n/a

#### `TEST(TemporalContractHardening, TCH06_SnapshotExcludesOutOfRange)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:202
- Brief: TCH-06: Snapshot excludes rows whose valid_time does not contain T.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH06_SnapshotExcludesOutOfRange): n/a

#### `TEST(TemporalContractHardening, TCH07_ConcurrentWriteNotVisible)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:214
- Brief: TCH-07: Row committed after snapshot_tx is NOT visible in the snapshot.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH07_ConcurrentWriteNotVisible): n/a

#### `TEST(TemporalContractHardening, TCH08_EmptyStoreEmptySnapshot)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:226
- Brief: TCH-08: Snapshot on empty store returns empty result.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH08_EmptyStoreEmptySnapshot): n/a

#### `TEST(TemporalContractHardening, TCH09_ExpiredRowSoftDeleted)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:237
- Brief: TCH-09: Rows past retention boundary are marked soft-deleted.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH09_ExpiredRowSoftDeleted): n/a

#### `TEST(TemporalContractHardening, TCH10_GcPreservesRetainedRows)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:253
- Brief: TCH-10: GC does not remove rows within retention window (not soft-deleted).
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH10_GcPreservesRetainedRows): n/a

#### `TEST(TemporalContractHardening, TCH11_SoftDeleteMarkersPresent)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:265
- Brief: TCH-11: Row past retention boundary has soft_deleted = true after policy run.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH11_SoftDeleteMarkersPresent): n/a

#### `TEST(TemporalContractHardening, TCH12_RetentionPolicyConflict)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:282
- Brief: TCH-12: Retention policy conflict raises RETENTION_POLICY_CONFLICT.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH12_RetentionPolicyConflict): n/a

#### `TEST(TemporalContractHardening, TCH13_ValidPitrTimestamp)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:301
- Brief: TCH-13: Valid PITR timestamp (within anchor range) returns no error.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH13_ValidPitrTimestamp): n/a

#### `TEST(TemporalContractHardening, TCH14_FuturePitrTimestampError)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:311
- Brief: TCH-14: Future PITR timestamp → PITR_TIMESTAMP_BEFORE_OLDEST.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH14_FuturePitrTimestampError): n/a

#### `TEST(TemporalContractHardening, TCH15_PitrBeforeOldestAnchor)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:323
- Brief: TCH-15: PITR with timestamp before oldest anchor → error.
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH15_PitrBeforeOldestAnchor): n/a

#### `TEST(TemporalContractHardening, TCH16_OpenEndSentinelIsInt64Max)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:334
- Brief: TCH-16: kTemporalOpenEnd equals INT64_MAX (sentinel contract).
- Parameters:
  - `<unnamed>` (TemporalContractHardening): n/a
  - `<unnamed>` (TCH16_OpenEndSentinelIsInt64Max): n/a

#### `void mockApplyRetention(std::vector< BiTemporalRow > &store, std::int64_t now_ns, std::int64_t retention_ns)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:103
- Brief: Simulates soft-delete check against retention boundary.
- Parameters:
  - `store` (std::vector< BiTemporalRow > &): n/a
  - `now_ns` (std::int64_t): n/a
  - `retention_ns` (std::int64_t): n/a

#### `std::size_t mockGcCount(const std::vector< BiTemporalRow > &store)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:116
- Brief: Validates that GC only removes rows that are soft-deleted.
- Parameters:
  - `store` (const std::vector< BiTemporalRow > &): n/a

#### `std::optional< TemporalErrorCode > mockPitrRestore(std::int64_t requested_ts, std::int64_t oldest_anchor, std::int64_t now_ns)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:127
- Brief: Simulates PITR restore.
- Parameters:
  - `requested_ts` (std::int64_t): n/a
  - `oldest_anchor` (std::int64_t): n/a
  - `now_ns` (std::int64_t): n/a

#### `std::vector< BiTemporalRow > mockSnapshotAt(const std::vector< BiTemporalRow > &store, std::int64_t t, std::int64_t snapshot_tx)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:84
- Brief: Returns rows whose valid_time interval contains point T.
- Parameters:
  - `store` (const std::vector< BiTemporalRow > &): n/a
  - `t` (std::int64_t): n/a
  - `snapshot_tx` (std::int64_t): n/a

#### `std::optional< TemporalErrorCode > mockValidateRange(std::int64_t start, std::int64_t end)`
- Source: `tests/temporal/test_temporal_contract_hardening_focused.cpp`:72
- Brief: Validates a bi-temporal range pair.
- Parameters:
  - `start` (std::int64_t): n/a
  - `end` (std::int64_t): n/a

### themisdb::temporal

#### `int base64CharValue(char c)`
- Source: `src/temporal/temporal_compressor.cpp`:79
- Brief: Base64 Char Value.
- Parameters:
  - `c` (char): Input parameter.
- Return: Return value.
- Details: c Input parameter. Return value. Implements base64CharValue without additional internal calls.

#### `size_t batchLimit(const RetentionPolicy &policy)`
- Source: `src/temporal/retention_manager.cpp`:341
- Brief: Batch Limit.
- Parameters:
  - `policy` (const RetentionPolicy &): Input parameter.
- Return: Return value.
- Details: policy Input parameter. Return value. Calls: max().

#### `uint64_t estimateVersionSize(const VersionedDocument &v)`
- Source: `src/temporal/retention_manager.cpp`:311
- Brief: Estimate Version Size.
- Parameters:
  - `v` (const VersionedDocument &): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Calls: size(), dump().

#### `Timestamp now()`
- Source: `include/temporal/temporal_types.h`:43
- Brief: Return the current wall-clock time in milliseconds since epoch.
- Parameters: none

#### `std::vector< ChangeEvent > replayFile(const std::string &path)`
- Source: `src/temporal/temporal_cdc.cpp`:561
- Brief: Replay File.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value. Calls: std::fopen(), c_str(), validateSegmentHeaderFile(), std::fclose(), std::fread(), payload(), data(), computeCRC32().

#### `std::string resolveArchiveTag(const RetentionPolicy &policy, const std::string &table_name)`
- Source: `src/temporal/retention_manager.cpp`:324
- Brief: Resolve Archive Tag.
- Parameters:
  - `policy` (const RetentionPolicy &): Input parameter.
  - `table_name` (const std::string &): Name of the table.
- Return: Return value.
- Details: policy Input parameter. table_name Name of the table. Return value. Calls: empty().

### themisdb::temporal::AggregateResult

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_aggregator.h`:86
- Brief: n/a
- Parameters: none

### themisdb::temporal::BiTemporalJoin

#### `BiTemporalJoin(std::vector< BiTemporalRow > left, std::vector< BiTemporalRow > right)`
- Source: `include/temporal/bitemporal_join.h`:112
- Brief: Construct the join operator with input row sets.
- Parameters:
  - `left` (std::vector< BiTemporalRow >): Left-hand versioned rows.
  - `right` (std::vector< BiTemporalRow >): Right-hand versioned rows.
- Details: left Left-hand versioned rows. right Right-hand versioned rows.

#### `BiTemporalJoin(std::vector< BiTemporalRow > left, std::vector< BiTemporalRow > right, Config config)`
- Source: `include/temporal/bitemporal_join.h`:122
- Brief: Construct the join operator with input row sets.
- Parameters:
  - `left` (std::vector< BiTemporalRow >): Left-hand versioned rows.
  - `right` (std::vector< BiTemporalRow >): Right-hand versioned rows.
  - `config` (Config): Join configuration (mode, point-in-time, etc.).
- Details: left Left-hand versioned rows. right Right-hand versioned rows. config Join configuration (mode, point-in-time, etc.).

#### `const Config & config() const noexcept`
- Source: `include/temporal/bitemporal_join.h`:141
- Brief: n/a
- Parameters: none

#### `bool containedIn(const TimeRange &inner, const TimeRange &outer) noexcept`
- Source: `include/temporal/bitemporal_join.h`:153
- Brief: True iff inner is fully contained within outer.
- Parameters:
  - `inner` (const TimeRange &): n/a
  - `outer` (const TimeRange &): n/a

#### `std::vector< BiTemporalJoinResult > execute() const`
- Source: `include/temporal/bitemporal_join.h`:132
- Brief: Execute the join and return all matching result rows.
- Parameters: none
- Return: Vector of bi-temporal join results, sorted by key then by valid_time_overlap.start.
- Details: Vector of bi-temporal join results, sorted by key then by valid_time_overlap.start.

#### `void forEach(std::function< bool(BiTemporalJoinResult)> cb) const`
- Source: `include/temporal/bitemporal_join.h`:139
- Brief: Stream results row by row via a callback (avoids materialisation).
- Parameters:
  - `cb` (std::function< bool(BiTemporalJoinResult)>): Called once for each result row. Return false to stop early.
- Details: cb Called once for each result row. Return false to stop early.

#### `TimeRange intersection(const TimeRange &a, const TimeRange &b) noexcept`
- Source: `include/temporal/bitemporal_join.h`:161
- Brief: Compute the intersection of two TimeRanges.
- Parameters:
  - `a` (const TimeRange &): n/a
  - `b` (const TimeRange &): n/a
- Return: Intersection, or an empty/invalid range when they do not overlap.
- Details: Intersection, or an empty/invalid range when they do not overlap.

#### `BiTemporalJoinResult makeResult(const BiTemporalRow &l, const BiTemporalRow &r) const noexcept`
- Source: `include/temporal/bitemporal_join.h`:169
- Brief: n/a
- Parameters:
  - `l` (const BiTemporalRow &): n/a
  - `r` (const BiTemporalRow &): n/a

#### `bool overlaps(const TimeRange &a, const TimeRange &b) noexcept`
- Source: `include/temporal/bitemporal_join.h`:148
- Brief: True iff two TimeRanges overlap (non-empty intersection).
- Parameters:
  - `a` (const TimeRange &): n/a
  - `b` (const TimeRange &): n/a

#### `bool rowMatches(const BiTemporalRow &l, const BiTemporalRow &r) const noexcept`
- Source: `include/temporal/bitemporal_join.h`:168
- Brief: n/a
- Parameters:
  - `l` (const BiTemporalRow &): n/a
  - `r` (const BiTemporalRow &): n/a

### themisdb::temporal::BiTemporalJoinResult

#### `bool operator==(const BiTemporalJoinResult &o) const noexcept`
- Source: `include/temporal/bitemporal_join.h`:53
- Brief: n/a
- Parameters:
  - `o` (const BiTemporalJoinResult &): n/a

### themisdb::temporal::BiTemporalTable

#### `BiTemporalTable(BiTemporalTable &&)=delete`
- Source: `include/temporal/bi_temporal.h`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTable &&): n/a

#### `BiTemporalTable(const BiTemporalTable &)=delete`
- Source: `include/temporal/bi_temporal.h`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BiTemporalTable &): n/a
- Details: Move semantics: BiTemporalTable is intentionally non-movable. The internal std::mutex cannot be moved, and moving a live table under concurrent access would create data-race hazards (CWE-362). Wrap in std::shared_ptr<BiTemporalTable> when shared/transferred ownership is required.

#### `BiTemporalTable(std::string table_name, std::string source_node="local")`
- Source: `include/temporal/bi_temporal.h`:96
- Brief: n/a
- Parameters:
  - `table_name` (std::string): n/a
  - `source_node` (std::string): n/a

#### `size_t closeCurrentRows(VersionList &versions, Timestamp close_time, const std::function< bool(const VersionedDocument &)> &pred)`
- Source: `include/temporal/bi_temporal.h`:265
- Brief: n/a
- Parameters:
  - `versions` (VersionList &): n/a
  - `close_time` (Timestamp): n/a
  - `pred` (const std::function< bool(const VersionedDocument &)> &): n/a

#### `size_t deleteForValidTime(const std::string &key, Timestamp valid_at)`
- Source: `include/temporal/bi_temporal.h`:139
- Brief: Delete For Valid Time.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `valid_at` (Timestamp): Input parameter.
- Return: Return value.
- Details: Logically delete all current rows for a key whose valid-time period contains the given timestamp. Returns the number of rows closed. key Input parameter. valid_at Input parameter. Return value. Calls: lock(), find(), end(), now(), closeCurrentRows(), contains().

#### `std::vector< TimeRange > findGaps(const std::string &key, Timestamp from, Timestamp to) const`
- Source: `include/temporal/bi_temporal.h`:178
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `from` (Timestamp): n/a
  - `to` (Timestamp): n/a
- Details: Detect gaps in the valid-time coverage of current rows for a key within the half-open interval [from, to). A gap is a sub-interval within [from, to) not covered by any current row's valid-time period. Returns an empty vector when the period [from, to) is fully covered. Returns {{from, to}} when the key has no current rows or none of them overlap the query range (the entire interval is a gap). Returns an empty vector when from >= to.

#### `std::vector< std::pair< VersionedDocument, VersionedDocument > > findOverlaps(const std::string &key) const`
- Source: `include/temporal/bi_temporal.h`:163
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
- Details: Detect overlaps among current rows for a key. Returns pairs of overlapping VersionedDocuments.

#### `std::vector< std::string > getAllKeys() const`
- Source: `include/temporal/bi_temporal.h`:213
- Brief: n/a
- Parameters: none
- Details: Return all known keys (including keys whose rows have all been logically deleted). Useful for bi-temporal joins that must enumerate every key ever written to the table.

#### `std::vector< VersionedDocument > getHistory(const std::string &key) const`
- Source: `include/temporal/bi_temporal.h`:197
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
- Details: Return all versions (history) for a key.

#### `nlohmann::json getStatistics() const`
- Source: `include/temporal/bi_temporal.h`:252
- Brief: n/a
- Parameters: none

#### `bool hasUniquenessConflict(const std::string &key, const TimeRange &period) const`
- Source: `include/temporal/bi_temporal.h`:191
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `period` (const TimeRange &): n/a
- Details: Check whether inserting a row with the given valid-time period for key would violate the temporal uniqueness constraint (i.e., overlap with an existing current row). Returns false immediately when period is empty or invalid (i.e., period.start >= period.end). Returns true when a conflict exists; false when the insert would succeed.

#### `bool insertWithValidTime(const std::string &key, const Document &doc, const TimeRange &valid_time)`
- Source: `include/temporal/bi_temporal.h`:119
- Brief: Insert With Valid Time.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `doc` (const Document &): Input parameter.
  - `valid_time` (const TimeRange &): Input parameter.
- Return: True when the operation succeeds.
- Details: Insert a row with an explicit valid-time period. Returns false and leaves the table unchanged when the valid-time period would overlap with an existing current row for the same key. key Input parameter. doc Input parameter. valid_time Input parameter. True when the operation succeeds. Calls: lock(), isCurrent(), overlaps(), now(), push_back(), std::move().

#### `size_t keyCount() const`
- Source: `include/temporal/bi_temporal.h`:250
- Brief: n/a
- Parameters: none

#### `MergeResult merge(const BiTemporalTable &other)`
- Source: `include/temporal/bi_temporal.h`:245
- Brief: ============================================================================ BiTemporalTable::merge — cross-node LWW reconciliation (v1.
- Parameters:
  - `other` (const BiTemporalTable &): Input parameter.
- Return: MergeResult with counters for inserted, skipped, and conflict-resolved rows.
- Details: Merge all rows from another BiTemporalTable into this table. The merge follows Last-Writer-Wins (LWW) semantics based on sys_time.start: for each key, a remote row is considered conflicting with the local table when it overlaps a current local row in valid-time. The row with the later sys_time.start wins. Rows whose key has no overlapping current local valid-time are inserted. If table names differ (tableName() != other.tableName()), the merge is treated as a no-op and returns zero counters. The operation is atomic on each key (keys are locked one at a time) and does not modify other. other Source table. Must not be the same object as this. MergeResult with counters for inserted, skipped, and conflict-resolved rows. other Input parameter. Return value. 9.0) ============================================================================ Calls: lk_other(), lk_self(), max(), size(), isCurrent(), overlaps(), push_back().

#### `BiTemporalTable & operator=(BiTemporalTable &&)=delete`
- Source: `include/temporal/bi_temporal.h`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (BiTemporalTable &&): n/a

#### `BiTemporalTable & operator=(const BiTemporalTable &)=delete`
- Source: `include/temporal/bi_temporal.h`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BiTemporalTable &): n/a

#### `std::vector< VersionedDocument > queryBiTemporal(const std::string &key, Timestamp sys_as_of, Timestamp valid_at) const`
- Source: `include/temporal/bi_temporal.h`:148
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `sys_as_of` (Timestamp): n/a
  - `valid_at` (Timestamp): n/a
- Details: Bi-temporal AS-OF query. Returns rows that were current at sys_as_of and whose valid-time period contains valid_at.

#### `std::vector< VersionedDocument > queryCurrentByValidTime(const std::string &key, Timestamp valid_at) const`
- Source: `include/temporal/bi_temporal.h`:156
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `valid_at` (Timestamp): n/a
- Details: Return all current rows for a key whose valid-time period contains valid_at.

#### `std::vector< VersionedDocument > scanBiTemporal(Timestamp sys_as_of, Timestamp valid_at) const`
- Source: `include/temporal/bi_temporal.h`:205
- Brief: n/a
- Parameters:
  - `sys_as_of` (Timestamp): n/a
  - `valid_at` (Timestamp): n/a
- Details: Bi-temporal table scan. Returns all rows where sys_time contains sys_as_of AND valid_time contains valid_at. Equivalent to a full-table AS-OF bi-temporal query.

#### `const std::string & tableName() const noexcept`
- Source: `include/temporal/bi_temporal.h`:249
- Brief: n/a
- Parameters: none

#### `bool updateForValidTime(const std::string &key, const Document &updates, Timestamp valid_at)`
- Source: `include/temporal/bi_temporal.h`:130
- Brief: Update For Valid Time.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `updates` (const Document &): Input parameter.
  - `valid_at` (Timestamp): Input parameter.
- Return: True when the operation succeeds.
- Details: Update the payload of the current row whose valid-time period contains the given timestamp. The old row's sys_time is closed; a new row is created with the merged data and the same valid-time period. Returns false if no matching current row is found. key Input parameter. updates Input parameter. valid_at Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), isCurrent(), contains(), now(), items(), std::move().

#### `size_t versionCount() const`
- Source: `include/temporal/bi_temporal.h`:251
- Brief: n/a
- Parameters: none

### themisdb::temporal::BloomFilter

#### `BloomFilter()`
- Source: `include/temporal/temporal_tier_manager.h`:107
- Brief: n/a
- Parameters: none

#### `BloomFilter(size_t expected_elements, size_t bits_per_elem=8)`
- Source: `include/temporal/temporal_tier_manager.h`:105
- Brief: Construct for expected_elements items with bits_per_elem bits.
- Parameters:
  - `expected_elements` (size_t): n/a
  - `bits_per_elem` (size_t): n/a

#### `void add(int64_t value) noexcept`
- Source: `include/temporal/temporal_tier_manager.h`:109
- Brief: n/a
- Parameters:
  - `value` (int64_t): n/a

#### `size_t bitCount() const noexcept`
- Source: `include/temporal/temporal_tier_manager.h`:112
- Brief: n/a
- Parameters: none

#### `uint64_t h1(uint64_t x) noexcept`
- Source: `include/temporal/temporal_tier_manager.h`:118
- Brief: n/a
- Parameters:
  - `x` (uint64_t): n/a

#### `uint64_t h2(uint64_t x) noexcept`
- Source: `include/temporal/temporal_tier_manager.h`:119
- Brief: n/a
- Parameters:
  - `x` (uint64_t): n/a

#### `uint64_t h3(uint64_t x) noexcept`
- Source: `include/temporal/temporal_tier_manager.h`:120
- Brief: n/a
- Parameters:
  - `x` (uint64_t): n/a

#### `bool mightContain(int64_t value) const noexcept`
- Source: `include/temporal/temporal_tier_manager.h`:110
- Brief: n/a
- Parameters:
  - `value` (int64_t): n/a

#### `void setBit(size_t idx) noexcept`
- Source: `include/temporal/temporal_tier_manager.h`:122
- Brief: n/a
- Parameters:
  - `idx` (size_t): n/a

#### `bool testBit(size_t idx) const noexcept`
- Source: `include/temporal/temporal_tier_manager.h`:123
- Brief: n/a
- Parameters:
  - `idx` (size_t): n/a

### themisdb::temporal::CDCPersistentLog

#### `CDCPersistentLog(const CDCPersistentLog &)=delete`
- Source: `include/temporal/temporal_cdc.h`:400
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CDCPersistentLog &): n/a

#### `CDCPersistentLog(std::string segment_dir, std::string log_prefix="cdc", uint64_t max_segment_bytes=kDefaultMaxSegmentBytes)`
- Source: `include/temporal/temporal_cdc.h`:393
- Brief: n/a
- Parameters:
  - `segment_dir` (std::string): Directory in which WAL segments are stored. Created automatically if it does not exist.
  - `log_prefix` (std::string): Prefix for segment file names (e.g. "cdc" or a table name). Must not be empty.
  - `max_segment_bytes` (uint64_t): Rotate to a new segment after this many bytes.
- Details: Construct a CDCPersistentLog. segment_dir Directory in which WAL segments are stored. Created automatically if it does not exist. log_prefix Prefix for segment file names (e.g. "cdc" or a table name). Must not be empty. max_segment_bytes Rotate to a new segment after this many bytes.

#### `void append(const ChangeEvent &event)`
- Source: `include/temporal/temporal_cdc.h`:432
- Brief: Append.
- Parameters:
  - `event` (const ChangeEvent &): Input parameter.
- Throws:
  - std::runtime_error: when called before open(), or on I/O errors.
  - std::runtime_error: if an error occurs.
- Details: Append a ChangeEvent to the persistent log. The event is JSON-serialised, CRC-32 checked, and written atomically (length-prefixed) to the current segment. If the segment exceeds max_segment_bytes after the write, a new segment is rotated. std::runtime_error when called before open(), or on I/O errors. event Input parameter. std::runtime_error if an error occurs. Calls: toJson(), dump(), lk(), segmentPath(), std::fopen(), c_str(), writeSegmentHeader(), size().

#### `void close()`
- Source: `include/temporal/temporal_cdc.h`:420
- Brief: Close.
- Parameters: none
- Details: Flush and close the current segment file handle. Safe to call multiple times. Calls: lk(), std::fflush(), std::fclose().

#### `uint32_t crc32(const std::string &data) noexcept`
- Source: `include/temporal/temporal_cdc.h`:501
- Brief: n/a
- Parameters:
  - `data` (const std::string &): n/a
- Details: Compute CRC-32/ISO-HDLC of data.

#### `bool isOpen() const noexcept`
- Source: `include/temporal/temporal_cdc.h`:463
- Brief: n/a
- Parameters: none
- Details: true when the log is currently open for writing.

#### `std::vector< uint64_t > listSegmentSeqs() const`
- Source: `include/temporal/temporal_cdc.h`:492
- Brief: n/a
- Parameters: none
- Details: Scan segment_dir_ for existing .wal files and return sorted seqs.

#### `void open()`
- Source: `include/temporal/temporal_cdc.h`:414
- Brief: Open.
- Parameters: none
- Throws:
  - std::runtime_error: on I/O errors during directory creation or segment scanning.
  - std::runtime_error: if an error occurs.
- Details: Open the log: scan existing segments in segment_dir, recover any truncated tail record, and position the write head at the first valid segment for appending. Must be called exactly once before append(). Idempotent: calling open() again after close() re-opens the log. std::runtime_error on I/O errors during directory creation or segment scanning. std::runtime_error if an error occurs. Calls: lk(), std::filesystem::create_directories(), message(), listSegmentSeqs(), empty(), back(), segmentPath(), std::fopen().

#### `CDCPersistentLog & operator=(const CDCPersistentLog &)=delete`
- Source: `include/temporal/temporal_cdc.h`:401
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CDCPersistentLog &): n/a

#### `std::vector< ChangeEvent > replayAll() const`
- Source: `include/temporal/temporal_cdc.h`:442
- Brief: n/a
- Parameters: none
- Throws:
  - std::runtime_error: on segment directory I/O errors.
- Details: Replay all persisted events from all segments in chronological order. Records with invalid CRC are silently skipped (truncation recovery). This is a read-only scan and can be called concurrently with append(). std::runtime_error on segment directory I/O errors.

#### `std::vector< ChangeEvent > replaySegment(uint64_t segment_seq) const`
- Source: `include/temporal/temporal_cdc.h`:451
- Brief: n/a
- Parameters:
  - `segment_seq` (uint64_t): Segment sequence number.
- Throws:
  - std::out_of_range: when segment_seq >= segmentCount().
  - std::runtime_error: on I/O errors.
- Details: Replay events from a specific segment (by 0-based sequence number). segment_seq Segment sequence number. std::out_of_range when segment_seq >= segmentCount(). std::runtime_error on I/O errors.

#### `void rotate()`
- Source: `include/temporal/temporal_cdc.h`:504
- Brief: Rotate.
- Parameters: none
- Details: Rotate: close active segment, increment seq, open new segment. Calls: std::fflush(), std::fclose(), segmentPath(), std::fopen(), c_str(), writeSegmentHeader().

#### `uint64_t segmentCount() const noexcept`
- Source: `include/temporal/temporal_cdc.h`:454
- Brief: n/a
- Parameters: none
- Details: Number of WAL segments that have been created (including active).

#### `std::string segmentPath(uint64_t seq) const`
- Source: `include/temporal/temporal_cdc.h`:489
- Brief: n/a
- Parameters:
  - `seq` (uint64_t): n/a
- Details: Build the full path for a segment with the given sequence number.

#### `uint64_t totalBytesWritten() const noexcept`
- Source: `include/temporal/temporal_cdc.h`:457
- Brief: n/a
- Parameters: none
- Details: Total bytes written to all segments (approximation, not fsynced).

#### `uint64_t totalEventsAppended() const noexcept`
- Source: `include/temporal/temporal_cdc.h`:460
- Brief: n/a
- Parameters: none
- Details: Total events successfully appended since open().

#### `bool validateSegmentHeader(std::FILE *fd)`
- Source: `include/temporal/temporal_cdc.h`:498
- Brief: n/a
- Parameters:
  - `fd` (std::FILE *): n/a
- Details: Validate the header of a segment file; return true on success.

#### `void writeSegmentHeader(std::FILE *fd, uint64_t seq)`
- Source: `include/temporal/temporal_cdc.h`:495
- Brief: n/a
- Parameters:
  - `fd` (std::FILE *): n/a
  - `seq` (uint64_t): n/a
- Details: Write the 22-byte segment header to fd.

#### `~CDCPersistentLog()`
- Source: `include/temporal/temporal_cdc.h`:397
- Brief: n/a
- Parameters: none

### themisdb::temporal::ChangeEvent

#### `ChangeEvent fromJson(const nlohmann::json &j)`
- Source: `include/temporal/temporal_cdc.h`:136
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: Deserialise from JSON. j Input parameter. Return value. Calls: TemporalCDC::changeTypeFromString(), at(), value().

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_cdc.h`:133
- Brief: n/a
- Parameters: none
- Details: Serialise to JSON for transport or storage.

### themisdb::temporal::ColdStoreStats

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_cold_store.h`:95
- Brief: n/a
- Parameters: none

### themisdb::temporal::ColumnInfo

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_migrator.h`:94
- Brief: n/a
- Parameters: none

### themisdb::temporal::CompressionStats

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_compressor.h`:115
- Brief: n/a
- Parameters: none

### themisdb::temporal::CustomMergeResolver

#### `CustomMergeResolver(MergeFn fn)`
- Source: `include/temporal/temporal_conflict_resolver.h`:198
- Brief: n/a
- Parameters:
  - `fn` (MergeFn): Merge function. Must not be null.
- Details: fn Merge function. Must not be null.

#### `TemporalSnapshot merge(const TemporalSnapshot &local, const TemporalSnapshot &remote) const override`
- Source: `include/temporal/temporal_conflict_resolver.h`:200
- Brief: Merge two conflicting snapshots into a single resolved snapshot.
- Parameters:
  - `local` (const TemporalSnapshot &): The locally-held snapshot version.
  - `remote` (const TemporalSnapshot &): The remotely-received snapshot version.
- Return: A new TemporalSnapshot whose data represents the merged state. The metadata fields (snapshot_id, hlc, source_node_id, checksum) are set by the implementation — typically to those of the "dominant" input.
- Details: local The locally-held snapshot version. remote The remotely-received snapshot version. A new TemporalSnapshot whose data represents the merged state. The metadata fields (snapshot_id, hlc, source_node_id, checksum) are set by the implementation — typically to those of the "dominant" input.

### themisdb::temporal::FileSystemBackend

#### `FileSystemBackend(std::filesystem::path base_dir)`
- Source: `include/temporal/temporal_cold_store.h`:220
- Brief: n/a
- Parameters:
  - `base_dir` (std::filesystem::path): Root directory for cold-tier data files. Created automatically if it does not exist.
- Details: base_dir Root directory for cold-tier data files. Created automatically if it does not exist.

#### `const std::filesystem::path & basePath() const noexcept`
- Source: `include/temporal/temporal_cold_store.h`:230
- Brief: n/a
- Parameters: none

#### `void clearAll() override`
- Source: `include/temporal/temporal_cold_store.h`:228
- Brief: Clear All.
- Parameters: none
- Details: Calls: lk(), fs::directory_iterator(), fs::remove_all(), path().

#### `bool del(const std::string &key) override`
- Source: `include/temporal/temporal_cold_store.h`:224
- Brief: Del.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds. Calls: lk(), keyToPath(), fs::remove().

#### `size_t deletePrefix(const std::string &prefix) override`
- Source: `include/temporal/temporal_cold_store.h`:227
- Brief: Delete Prefix.
- Parameters:
  - `prefix` (const std::string &): Input parameter.
- Return: Return value.
- Details: prefix Input parameter. Return value. Calls: lk(), fs::recursive_directory_iterator(), is_regular_file(), path(), extension(), pathToKey(), substr(), size().

#### `std::string get(const std::string &key) const override`
- Source: `include/temporal/temporal_cold_store.h`:223
- Brief: n/a
- Parameters:
  - `composite_key` (const std::string &): n/a
- Details: Retrieve value for key. Returns empty string if not found.

#### `std::filesystem::path keyToPath(const std::string &composite_key) const`
- Source: `include/temporal/temporal_cold_store.h`:237
- Brief: Convert a composite key to a filesystem path under base_dir_.
- Parameters:
  - `composite_key` (const std::string &): n/a

#### `std::vector< std::string > listKeysWithPrefix(const std::string &prefix) const override`
- Source: `include/temporal/temporal_cold_store.h`:226
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a
- Details: Enumerate all composite keys that start with prefix. Used to rebuild the RAM index on startup.

#### `std::string pathToKey(const std::filesystem::path &rel_path) const`
- Source: `include/temporal/temporal_cold_store.h`:246
- Brief: Reconstruct the composite key from a file path relative to base_dir_.
- Parameters:
  - `rel_path` (const std::filesystem::path &): n/a

#### `std::string percentDecode(const std::string &s)`
- Source: `include/temporal/temporal_cold_store.h`:243
- Brief: Reverse of percentEncode.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: Percent Decode. s Input parameter. Return value. Calls: reserve(), size(), hexVal().

#### `std::string percentEncode(const std::string &s)`
- Source: `include/temporal/temporal_cold_store.h`:240
- Brief: Percent-encode a string component so it is safe as a directory name.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: Percent-encode a string so every byte is safe as a filesystem path component. s Input parameter. Return value. Encodes everything except unreserved URI characters (A-Z a-z 0-9 - _ . ~). Calls: reserve(), size().

#### `bool put(const std::string &key, const std::string &value) override`
- Source: `include/temporal/temporal_cold_store.h`:222
- Brief: Put.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. value Input parameter. True when the operation succeeds. Calls: lk(), keyToPath(), fs::create_directories(), parent_path(), fs::path(), string(), ofs(), fs::remove().

### themisdb::temporal::IColdStoreBackend

#### `void clearAll()=0`
- Source: `include/temporal/temporal_cold_store.h`:145
- Brief: n/a
- Parameters: none
- Details: Remove all entries.

#### `bool del(const std::string &composite_key)=0`
- Source: `include/temporal/temporal_cold_store.h`:132
- Brief: n/a
- Parameters:
  - `composite_key` (const std::string &): n/a
- Details: Delete key. Returns true if the key existed.

#### `size_t deletePrefix(const std::string &prefix)=0`
- Source: `include/temporal/temporal_cold_store.h`:142
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a
- Details: Remove all keys that start with prefix.

#### `std::string get(const std::string &composite_key) const =0`
- Source: `include/temporal/temporal_cold_store.h`:129
- Brief: n/a
- Parameters:
  - `composite_key` (const std::string &): n/a
- Details: Retrieve value for key. Returns empty string if not found.

#### `std::vector< std::string > listKeysWithPrefix(const std::string &prefix) const =0`
- Source: `include/temporal/temporal_cold_store.h`:139
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a
- Details: Enumerate all composite keys that start with prefix. Used to rebuild the RAM index on startup.

#### `bool put(const std::string &composite_key, const std::string &json_value)=0`
- Source: `include/temporal/temporal_cold_store.h`:125
- Brief: n/a
- Parameters:
  - `composite_key` (const std::string &): n/a
  - `json_value` (const std::string &): n/a
- Details: Persist key→value. Returns false on I/O failure.

#### `~IColdStoreBackend()=default`
- Source: `include/temporal/temporal_cold_store.h`:122
- Brief: n/a
- Parameters: none

### themisdb::temporal::InMemoryBackend

#### `void clearAll() override`
- Source: `include/temporal/temporal_cold_store.h`:169
- Brief: Clear All.
- Parameters: none
- Details: Calls: lk(), clear().

#### `bool del(const std::string &key) override`
- Source: `include/temporal/temporal_cold_store.h`:165
- Brief: Del.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds. Calls: lk(), erase().

#### `size_t deletePrefix(const std::string &prefix) override`
- Source: `include/temporal/temporal_cold_store.h`:168
- Brief: Delete Prefix.
- Parameters:
  - `prefix` (const std::string &): Input parameter.
- Return: Return value.
- Details: prefix Input parameter. Return value. Calls: lk(), lower_bound(), end(), substr(), size(), erase().

#### `std::string get(const std::string &key) const override`
- Source: `include/temporal/temporal_cold_store.h`:164
- Brief: n/a
- Parameters:
  - `composite_key` (const std::string &): n/a
- Details: Retrieve value for key. Returns empty string if not found.

#### `std::vector< std::string > listKeysWithPrefix(const std::string &prefix) const override`
- Source: `include/temporal/temporal_cold_store.h`:167
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a
- Details: Enumerate all composite keys that start with prefix. Used to rebuild the RAM index on startup.

#### `bool put(const std::string &key, const std::string &value) override`
- Source: `include/temporal/temporal_cold_store.h`:163
- Brief: Put.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. value Input parameter. True when the operation succeeds. Calls: lk().

### themisdb::temporal::IntervalTreeIndex

#### `IntervalTreeIndex(IntervalTreeIndex &&) noexcept=default`
- Source: `include/temporal/interval_tree_index.h`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndex &&): n/a

#### `IntervalTreeIndex(const IntervalTreeIndex &)=delete`
- Source: `include/temporal/interval_tree_index.h`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IntervalTreeIndex &): n/a

#### `IntervalTreeIndex(std::string name)`
- Source: `include/temporal/interval_tree_index.h`:106
- Brief: n/a
- Parameters:
  - `name` (std::string): n/a

#### `std::unique_ptr< Node > balance(std::unique_ptr< Node > n)`
- Source: `include/temporal/interval_tree_index.h`:237
- Brief: n/a
- Parameters:
  - `n` (std::unique_ptr< Node >): n/a

#### `int balanceFactor(const Node *n) noexcept`
- Source: `include/temporal/interval_tree_index.h`:233
- Brief: n/a
- Parameters:
  - `n` (const Node *): n/a

#### `void clear()`
- Source: `include/temporal/interval_tree_index.h`:165
- Brief: Clear.
- Parameters: none
- Details: Remove all entries. O(n). Calls: lk(), reset().

#### `void collectKey(const Node *n, const std::string &key, std::optional< TimeRange > range, std::vector< IntervalEntry > &out) const`
- Source: `include/temporal/interval_tree_index.h`:261
- Brief: n/a
- Parameters:
  - `n` (const Node *): n/a
  - `key` (const std::string &): n/a
  - `range` (std::optional< TimeRange >): n/a
  - `out` (std::vector< IntervalEntry > &): n/a

#### `void collectOverlap(const Node *n, Timestamp from, Timestamp to, std::vector< IntervalEntry > &out) const`
- Source: `include/temporal/interval_tree_index.h`:258
- Brief: n/a
- Parameters:
  - `n` (const Node *): n/a
  - `from` (Timestamp): n/a
  - `to` (Timestamp): n/a
  - `out` (std::vector< IntervalEntry > &): n/a

#### `std::unique_ptr< Node > detachMin(std::unique_ptr< Node > root, std::unique_ptr< Node > &out_node)`
- Source: `include/temporal/interval_tree_index.h`:255
- Brief: Detach and return the leftmost node, repairing the subtree.
- Parameters:
  - `root` (std::unique_ptr< Node >): n/a
  - `out_node` (std::unique_ptr< Node > &): n/a

#### `size_t erase(const std::string &key)`
- Source: `include/temporal/interval_tree_index.h`:162
- Brief: STL-style erase: remove all entries for the given key.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Number of entries actually removed (0 if the key was absent).
- Details: Erase. This is an alias for removeKey() that follows STL container naming conventions (erase vs. remove). It removes every interval entry associated with key from the tree while maintaining the AVL-balance invariant through tree rotations. Complexity: O(k·log n) where k is the number of intervals stored for key and n is the total number of entries in the tree. For the common case of a unique-key index (k = 1) this degenerates to O(log n). Thread-safetyerase() acquires an exclusive (std::unique_lock) on the internal std::shared_mutex. All concurrent readers — including those holding a std::shared_lock via queryPoint(), queryRange(), or queryKey() — are blocked until the erase completes. Callers that received results from a previous query call must not retain raw pointers or references into those result vectors after erase() returns; the returned std::vector<IntervalEntry> copies are safe to use independently. key Logical key whose entries should be removed. Number of entries actually removed (0 if the key was absent). key Input parameter. Return value. Calls: removeKey().

#### `Node * findMin(Node *n) noexcept`
- Source: `include/temporal/interval_tree_index.h`:252
- Brief: Extract the in-order successor (leftmost node of a subtree).
- Parameters:
  - `n` (Node *): n/a

#### `void insert(const IntervalEntry &entry)`
- Source: `include/temporal/interval_tree_index.h`:122
- Brief: Insert.
- Parameters:
  - `entry` (const IntervalEntry &): Input parameter.
- Details: Insert an entry into the tree. Amortised O(log n). entry Input parameter. Calls: lk(), insertNode(), std::move(), push_back().

#### `std::unique_ptr< Node > insertNode(std::unique_ptr< Node > root, const IntervalEntry &entry)`
- Source: `include/temporal/interval_tree_index.h`:239
- Brief: n/a
- Parameters:
  - `root` (std::unique_ptr< Node >): n/a
  - `entry` (const IntervalEntry &): n/a

#### `const std::string & name() const noexcept`
- Source: `include/temporal/interval_tree_index.h`:199
- Brief: n/a
- Parameters: none

#### `int nodeHeight(const Node *n) noexcept`
- Source: `include/temporal/interval_tree_index.h`:232
- Brief: n/a
- Parameters:
  - `n` (const Node *): n/a

#### `Timestamp nodeMaxEnd(const Node *n) noexcept`
- Source: `include/temporal/interval_tree_index.h`:228
- Brief: n/a
- Parameters:
  - `n` (const Node *): n/a

#### `IntervalTreeIndex & operator=(IntervalTreeIndex &&) noexcept=default`
- Source: `include/temporal/interval_tree_index.h`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (IntervalTreeIndex &&): n/a

#### `IntervalTreeIndex & operator=(const IntervalTreeIndex &)=delete`
- Source: `include/temporal/interval_tree_index.h`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IntervalTreeIndex &): n/a

#### `std::vector< IntervalEntry > queryKey(const std::string &key, std::optional< TimeRange > range=std::nullopt) const`
- Source: `include/temporal/interval_tree_index.h`:193
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `range` (std::optional< TimeRange >): n/a
- Details: Return all entries for a specific key, optionally filtered to those whose interval overlaps the given range. O(n) worst-case.

#### `std::vector< IntervalEntry > queryOverlap(Timestamp from, Timestamp to) const`
- Source: `include/temporal/interval_tree_index.h`:181
- Brief: n/a
- Parameters:
  - `from` (Timestamp): n/a
  - `to` (Timestamp): n/a
- Details: Return all entries whose interval overlaps [from, to). An interval [a, b) overlaps [from, to) iff a < to && from < b. O(log n + k).

#### `std::vector< IntervalEntry > queryOverlap(const TimeRange &range) const`
- Source: `include/temporal/interval_tree_index.h`:187
- Brief: n/a
- Parameters:
  - `range` (const TimeRange &): n/a
- Details: Return all entries whose interval overlaps the given range. Convenience overload for queryOverlap(range.start, range.end).

#### `std::vector< IntervalEntry > queryPoint(Timestamp t) const`
- Source: `include/temporal/interval_tree_index.h`:174
- Brief: n/a
- Parameters:
  - `t` (Timestamp): n/a
- Details: Return all entries whose interval contains timestamp t, i.e. entry.range.start <= t < entry.range.end. O(log n + k).

#### `size_t remove(const std::string &key, const TimeRange &range)`
- Source: `include/temporal/interval_tree_index.h`:128
- Brief: Remove.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `range` (const TimeRange &): Input parameter.
- Return: Return value.
- Details: Remove all entries matching both key and range exactly. Returns the number of entries removed. O(log n) expected. key Input parameter. range Input parameter. Return value. Calls: lk(), removeNode(), std::move(), find(), end(), erase(), std::remove_if(), begin().

#### `size_t removeKey(const std::string &key)`
- Source: `include/temporal/interval_tree_index.h`:134
- Brief: Remove Key.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: Remove all entries for the given key regardless of range. O(n) worst-case due to multiple hits with the same key. key Input parameter. Return value. Calls: lk(), removeKeyNode(), std::move(), erase().

#### `std::unique_ptr< Node > removeKeyNode(std::unique_ptr< Node > root, const std::string &key, size_t &removed_count)`
- Source: `include/temporal/interval_tree_index.h`:247
- Brief: n/a
- Parameters:
  - `root` (std::unique_ptr< Node >): n/a
  - `key` (const std::string &): n/a
  - `removed_count` (size_t &): n/a

#### `std::unique_ptr< Node > removeNode(std::unique_ptr< Node > root, const std::string &key, const TimeRange &range, size_t &removed_count)`
- Source: `include/temporal/interval_tree_index.h`:242
- Brief: n/a
- Parameters:
  - `root` (std::unique_ptr< Node >): n/a
  - `key` (const std::string &): n/a
  - `range` (const TimeRange &): n/a
  - `removed_count` (size_t &): n/a

#### `std::unique_ptr< Node > rotateLeft(std::unique_ptr< Node > x)`
- Source: `include/temporal/interval_tree_index.h`:236
- Brief: n/a
- Parameters:
  - `x` (std::unique_ptr< Node >): n/a

#### `std::unique_ptr< Node > rotateRight(std::unique_ptr< Node > y)`
- Source: `include/temporal/interval_tree_index.h`:235
- Brief: n/a
- Parameters:
  - `y` (std::unique_ptr< Node >): n/a

#### `size_t size() const noexcept`
- Source: `include/temporal/interval_tree_index.h`:202
- Brief: n/a
- Parameters: none
- Details: O(1) — maintained as an atomic counter.

#### `IntervalTreeStats stats() const`
- Source: `include/temporal/interval_tree_index.h`:204
- Brief: n/a
- Parameters: none

#### `size_t treeHeight(const Node *n) noexcept`
- Source: `include/temporal/interval_tree_index.h`:265
- Brief: n/a
- Parameters:
  - `n` (const Node *): n/a

#### `void updateHeight(Node *n) noexcept`
- Source: `include/temporal/interval_tree_index.h`:234
- Brief: n/a
- Parameters:
  - `n` (Node *): n/a

#### `void updateSubtreeMax(Node *n) noexcept`
- Source: `include/temporal/interval_tree_index.h`:229
- Brief: n/a
- Parameters:
  - `n` (Node *): n/a

#### `~IntervalTreeIndex()=default`
- Source: `include/temporal/interval_tree_index.h`:114
- Brief: n/a
- Parameters: none

### themisdb::temporal::IntervalTreeIndex::Node

#### `Node(IntervalEntry e)`
- Source: `include/temporal/interval_tree_index.h`:221
- Brief: n/a
- Parameters:
  - `e` (IntervalEntry): n/a

### themisdb::temporal::IntervalTreeStats

#### `nlohmann::json toJson() const`
- Source: `include/temporal/interval_tree_index.h`:64
- Brief: n/a
- Parameters: none

### themisdb::temporal::LWWFieldMergeResolver

#### `TemporalSnapshot merge(const TemporalSnapshot &local, const TemporalSnapshot &remote) const override`
- Source: `include/temporal/temporal_conflict_resolver.h`:151
- Brief: Merge two conflicting snapshots into a single resolved snapshot.
- Parameters:
  - `local` (const TemporalSnapshot &): The locally-held snapshot version.
  - `remote` (const TemporalSnapshot &): The remotely-received snapshot version.
- Return: A new TemporalSnapshot whose data represents the merged state. The metadata fields (snapshot_id, hlc, source_node_id, checksum) are set by the implementation — typically to those of the "dominant" input.
- Details: local The locally-held snapshot version. remote The remotely-received snapshot version. A new TemporalSnapshot whose data represents the merged state. The metadata fields (snapshot_id, hlc, source_node_id, checksum) are set by the implementation — typically to those of the "dominant" input.

### themisdb::temporal::MergeResolver

#### `TemporalSnapshot merge(const TemporalSnapshot &local, const TemporalSnapshot &remote) const =0`
- Source: `include/temporal/temporal_conflict_resolver.h`:131
- Brief: Merge two conflicting snapshots into a single resolved snapshot.
- Parameters:
  - `local` (const TemporalSnapshot &): The locally-held snapshot version.
  - `remote` (const TemporalSnapshot &): The remotely-received snapshot version.
- Return: A new TemporalSnapshot whose data represents the merged state. The metadata fields (snapshot_id, hlc, source_node_id, checksum) are set by the implementation — typically to those of the "dominant" input.
- Details: local The locally-held snapshot version. remote The remotely-received snapshot version. A new TemporalSnapshot whose data represents the merged state. The metadata fields (snapshot_id, hlc, source_node_id, checksum) are set by the implementation — typically to those of the "dominant" input.

#### `~MergeResolver()=default`
- Source: `include/temporal/temporal_conflict_resolver.h`:119
- Brief: n/a
- Parameters: none

### themisdb::temporal::MigrationPlan

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_migrator.h`:151
- Brief: n/a
- Parameters: none

### themisdb::temporal::MigrationReport

#### `size_t failedCheckCount() const`
- Source: `include/temporal/temporal_migrator.h`:203
- Brief: n/a
- Parameters: none
- Details: Aggregate: number of checks that failed.

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_migrator.h`:205
- Brief: n/a
- Parameters: none

### themisdb::temporal::MigrationStats

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_migrator.h`:169
- Brief: n/a
- Parameters: none

### themisdb::temporal::QueryCache

#### `QueryCache(size_t max_entries=256)`
- Source: `include/temporal/temporal_query_engine.h`:489
- Brief: n/a
- Parameters:
  - `max_entries` (size_t): Maximum number of (table, time) entries to retain. Must be > 0.
- Details: Construct a cache with the given maximum number of entries. When the cache is full the least-recently-used entry is evicted. max_entries Maximum number of (table, time) entries to retain. Must be > 0.

#### `void clear()`
- Source: `include/temporal/temporal_query_engine.h`:509
- Brief: Clear.
- Parameters: none
- Details: Discard all cached entries. Calls: lock().

#### `std::optional< std::vector< VersionedDocument > > get(const std::string &table_name, Timestamp as_of) const`
- Source: `include/temporal/temporal_query_engine.h`:497
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a
  - `as_of` (Timestamp): n/a
- Details: Look up a cached result. Returns a copy of the cached vector, or std::nullopt on cache miss. Returning by value avoids returning a pointer into internal storage that can be invalidated by a concurrent put/invalidate/clear call.

#### `void invalidate(const std::string &table_name)`
- Source: `include/temporal/temporal_query_engine.h`:506
- Brief: Invalidate.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
- Details: Invalidate all entries for the given table (e.g. after a write). table_name Name of the table. Calls: lock(), begin(), end(), erase().

#### `void put(const std::string &table_name, Timestamp as_of, std::vector< VersionedDocument > result)`
- Source: `include/temporal/temporal_query_engine.h`:501
- Brief: Put.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `as_of` (Timestamp): Input parameter.
  - `result` (std::vector< VersionedDocument >): Input parameter.
- Details: Store a result in the cache, evicting LRU entry if necessary. table_name Name of the table. as_of Input parameter. result Input parameter. Calls: lock(), find(), end(), std::move(), size(), begin(), erase(), emplace().

#### `size_t size() const`
- Source: `include/temporal/temporal_query_engine.h`:511
- Brief: n/a
- Parameters: none

### themisdb::temporal::QueryCache::CacheKey

#### `bool operator==(const CacheKey &o) const noexcept`
- Source: `include/temporal/temporal_query_engine.h`:517
- Brief: n/a
- Parameters:
  - `o` (const CacheKey &): n/a

### themisdb::temporal::QueryCache::CacheKeyHash

#### `std::size_t operator()(const CacheKey &k) const noexcept`
- Source: `include/temporal/temporal_query_engine.h`:523
- Brief: n/a
- Parameters:
  - `k` (const CacheKey &): n/a

### themisdb::temporal::RetentionManager

#### `RetentionManager()=default`
- Source: `include/temporal/retention_manager.h`:202
- Brief: n/a
- Parameters: none

#### `RetentionStats applyPolicy(SystemVersionedTable &table, const RetentionPolicy &policy)`
- Source: `include/temporal/retention_manager.h`:301
- Brief: Apply Policy.
- Parameters:
  - `table` (SystemVersionedTable &): Input/output parameter.
  - `policy` (const RetentionPolicy &): Input parameter.
- Return: Return value.
- Details: table Input/output parameter. policy Input parameter. Return value. Calls: std::chrono::steady_clock::now(), now(), count(), getAllKeys(), getHistory(), size(), isCurrent(), estimateVersionSize().

#### `void clearArchive()`
- Source: `include/temporal/retention_manager.h`:270
- Brief: Clear Archive.
- Parameters: none
- Details: Clear the in-memory archive. Calls: lock(), clear().

#### `RetentionStats enforceRetention(SystemVersionedTable &table)`
- Source: `include/temporal/retention_manager.h`:220
- Brief: Enforce Retention.
- Parameters:
  - `table` (SystemVersionedTable &): Input/output parameter.
- Return: Return value.
- Details: Apply the registered policy to the given table. Non-current versions that violate the policy are physically deleted (and optionally archived before deletion). table Input/output parameter. Return value. Calls: lock(), find(), tableName(), end(), push_back(), std::max(), applyPolicy(), std::string().

#### `RetentionStats enforceRetention(SystemVersionedTable &table, const RetentionPolicy &policy)`
- Source: `include/temporal/retention_manager.h`:225
- Brief: Enforce Retention.
- Parameters:
  - `table` (SystemVersionedTable &): Input/output parameter.
  - `policy` (const RetentionPolicy &): Input parameter.
- Return: Return value.
- Details: Apply the given policy directly without registering it. table Input/output parameter. policy Input parameter. Return value. Calls: std::max(), applyPolicy(), push_back(), std::string(), what(), empty(), lock().

#### `std::vector< ArchivedRecord > getArchivedRecords() const`
- Source: `include/temporal/retention_manager.h`:263
- Brief: n/a
- Parameters: none
- Details: Return all archived records (across all tables).

#### `std::vector< ArchivedRecord > getArchivedRecords(const std::string &table_name) const`
- Source: `include/temporal/retention_manager.h`:266
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a
- Details: Return archived records for a specific table.

#### `nlohmann::json getCumulativeStats() const`
- Source: `include/temporal/retention_manager.h`:274
- Brief: n/a
- Parameters: none

#### `std::optional< RetentionPolicy > getPolicy(const std::string &table_name) const`
- Source: `include/temporal/retention_manager.h`:211
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a
- Details: Retrieve the retention policy for a table, if set.

#### `void scheduleTable(SystemVersionedTable &table, std::chrono::milliseconds interval)`
- Source: `include/temporal/retention_manager.h`:243
- Brief: Schedule Table.
- Parameters:
  - `table` (SystemVersionedTable &): Input/output parameter.
  - `interval` (std::chrono::milliseconds): Input parameter.
- Details: Register a table for periodic background retention enforcement. The scheduler runs in a dedicated thread. Each registered table is checked every interval and the previously-registered policy (setPolicy) is applied. The table pointer must remain valid until stopScheduler() or the RetentionManager is destroyed. table Reference to the table to maintain. interval How often to enforce the policy (minimum 1 millisecond). Call startScheduler() once to activate background processing. table Input/output parameter. interval Input parameter. Calls: lock(), std::chrono::milliseconds(), std::chrono::steady_clock::now(), push_back(), std::move().

#### `void schedulerLoop()`
- Source: `include/temporal/retention_manager.h`:299
- Brief: Scheduler Loop.
- Parameters: none
- Details: Calls: load(), std::chrono::steady_clock::now(), lock(), push_back(), enforceRetention(), std::this_thread::sleep_for(), std::chrono::milliseconds().

#### `bool schedulerRunning() const noexcept`
- Source: `include/temporal/retention_manager.h`:258
- Brief: n/a
- Parameters: none
- Details: Return true if the background scheduler is currently running.

#### `void setPolicy(const std::string &table_name, const RetentionPolicy &policy)`
- Source: `include/temporal/retention_manager.h`:208
- Brief: Set Policy.
- Parameters:
  - `table_name` (const std::string &): Name of the retention policy.
  - `policy` (const RetentionPolicy &): Input parameter.
- Details: Set the retention policy for a named table. table_name Name of the retention policy. policy Input parameter. Calls: lock().

#### `void startScheduler()`
- Source: `include/temporal/retention_manager.h`:250
- Brief: Start Scheduler.
- Parameters: none
- Details: Start the background retention thread. Calling this more than once is a no-op. Calls: compare_exchange_strong(), store(), std::thread().

#### `void stopScheduler()`
- Source: `include/temporal/retention_manager.h`:255
- Brief: Stop Scheduler.
- Parameters: none
- Details: Stop the background retention thread and wait for it to exit. Calls: load(), store(), joinable(), join().

#### `~RetentionManager()`
- Source: `include/temporal/retention_manager.h`:203
- Brief: n/a
- Parameters: none

### themisdb::temporal::RetentionRule

#### `bool operator<(const RetentionRule &rhs) const noexcept`
- Source: `include/temporal/retention_manager.h`:99
- Brief: n/a
- Parameters:
  - `rhs` (const RetentionRule &): n/a

#### `bool operator==(const RetentionRule &rhs) const noexcept`
- Source: `include/temporal/retention_manager.h`:98
- Brief: n/a
- Parameters:
  - `rhs` (const RetentionRule &): n/a

#### `RetentionRule storageBased(uint64_t max_b, std::string compliance_tag={}) noexcept`
- Source: `include/temporal/retention_manager.h`:92
- Brief: Convenience factory: create a STORAGE_BASED rule.
- Parameters:
  - `max_b` (uint64_t): n/a
  - `compliance_tag` (std::string): n/a

#### `RetentionRule timeBased(std::chrono::milliseconds p, std::string compliance_tag={}) noexcept`
- Source: `include/temporal/retention_manager.h`:75
- Brief: Convenience factory: create a TIME_BASED rule.
- Parameters:
  - `p` (std::chrono::milliseconds): n/a
  - `compliance_tag` (std::string): n/a

#### `RetentionRule versionCount(size_t n, std::string compliance_tag={}) noexcept`
- Source: `include/temporal/retention_manager.h`:83
- Brief: Convenience factory: create a VERSION_COUNT_BASED rule.
- Parameters:
  - `n` (size_t): n/a
  - `compliance_tag` (std::string): n/a

### themisdb::temporal::RetentionStats

#### `nlohmann::json toJson() const`
- Source: `include/temporal/retention_manager.h`:163
- Brief: n/a
- Parameters: none

### themisdb::temporal::RowFilter

#### `bool matches(const Document &doc) const`
- Source: `include/temporal/temporal_query_engine.h`:73
- Brief: n/a
- Parameters:
  - `doc` (const Document &): n/a

### themisdb::temporal::SnapshotDiff

#### `bool empty() const noexcept`
- Source: `include/temporal/snapshot_manager.h`:94
- Brief: n/a
- Parameters: none

#### `nlohmann::json toJson() const`
- Source: `include/temporal/snapshot_manager.h`:102
- Brief: n/a
- Parameters: none

#### `size_t totalChanges() const noexcept`
- Source: `include/temporal/snapshot_manager.h`:98
- Brief: n/a
- Parameters: none

### themisdb::temporal::SnapshotHandle

#### `bool isValid() const noexcept`
- Source: `include/temporal/snapshot_manager.h`:54
- Brief: n/a
- Parameters: none

#### `bool operator<(const SnapshotHandle &other) const noexcept`
- Source: `include/temporal/snapshot_manager.h`:50
- Brief: n/a
- Parameters:
  - `other` (const SnapshotHandle &): n/a

#### `nlohmann::json toJson() const`
- Source: `include/temporal/snapshot_manager.h`:56
- Brief: n/a
- Parameters: none

### themisdb::temporal::SnapshotMetadata

#### `nlohmann::json toJson() const`
- Source: `include/temporal/snapshot_manager.h`:73
- Brief: n/a
- Parameters: none

### themisdb::temporal::SystemVersionedTable

#### `SystemVersionedTable(SystemVersionedTable &&other) noexcept`
- Source: `include/temporal/system_versioned_table.h`:98
- Brief: n/a
- Parameters:
  - `other` (SystemVersionedTable &&): n/a
- Details: Move constructor. Required because std::mutex is not movable; we default-construct the destination mutex (which is always correct for a newly-constructed object that nobody else holds a lock to).

#### `SystemVersionedTable(std::string table_name, Config config, std::string source_node="local")`
- Source: `include/temporal/system_versioned_table.h`:89
- Brief: n/a
- Parameters:
  - `table_name` (std::string): Name of this table.
  - `config` (Config): Runtime configuration (retention, compression, …).
  - `source_node` (std::string): Node/user label written to VersionedDocument::modified_by.
- Details: Construct with full Config. table_name Name of this table. config Runtime configuration (retention, compression, …). source_node Node/user label written to VersionedDocument::modified_by.

#### `SystemVersionedTable(std::string table_name, std::string source_node="local")`
- Source: `include/temporal/system_versioned_table.h`:79
- Brief: n/a
- Parameters:
  - `table_name` (std::string): n/a
  - `source_node` (std::string): n/a
- Details: Construct with explicit table name and optional source-node label. Uses default Config values.

#### `void closeCurrentVersion(VersionList &versions, Timestamp close_time)`
- Source: `include/temporal/system_versioned_table.h`:281
- Brief: Close Current Version.
- Parameters:
  - `versions` (VersionList &): Input/output parameter.
  - `close_time` (Timestamp): Input parameter.
- Details: versions Input/output parameter. close_time Input parameter. Implements closeCurrentVersion without additional internal calls.

#### `SystemVersionedTable createVersionedTable(const std::string &table_name, const Document &schema)`
- Source: `include/temporal/system_versioned_table.h`:122
- Brief: static
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `schema` (const Document &): Input parameter.
- Return: Return value.
- Details: Overload with default Config. table_name Name of the table. schema Input parameter. Return value. Implements createVersionedTable without additional internal calls.

#### `SystemVersionedTable createVersionedTable(const std::string &table_name, const Document &schema, Config config, const std::string &source_node="local")`
- Source: `include/temporal/system_versioned_table.h`:115
- Brief: static
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `schema` (const Document &): Input parameter.
  - `config` (Config): Input parameter.
  - `source_node` (const std::string &): Input parameter.
- Return: Fully configured SystemVersionedTable instance.
- Details: Static factory that creates a system-versioned table from a schema descriptor (arbitrary JSON) and a Config. The schema is stored in the table's statistics under "schema" and can be retrieved via getStatistics(). It does not affect storage behaviour but enables DDL-aware tools to inspect column definitions. table_name Logical table name (used for the history_table_name default if config.history_table_name is empty). schema JSON object describing the table columns/types. config Runtime configuration. source_node Source-node label for DML attribution. Fully configured SystemVersionedTable instance. table_name Name of the table. schema Input parameter. config Input parameter. source_node Input parameter. Return value. Calls: empty(), tbl(), std::move().

#### `bool deleteRow(const std::string &key)`
- Source: `include/temporal/system_versioned_table.h`:151
- Brief: Delete Row.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: Logically delete the current row (closes its sys_time period). Returns false if no current row exists for the key. key Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), closeCurrentVersion(), now().

#### `size_t enforceRetentionPolicy()`
- Source: `include/temporal/system_versioned_table.h`:250
- Brief: Enforce Retention Policy.
- Parameters: none
- Return: Number of historical versions physically removed.
- Details: Apply the configured retention policy to all keys. Historical versions older than Config::retention_period are physically removed. If Config::retention_period is zero the call is a no-op. The current (open-ended) version is never removed. Number of historical versions physically removed. Return value. Calls: count(), now(), purgeHistoricalVersions().

#### `std::vector< std::string > getAllKeys() const`
- Source: `include/temporal/system_versioned_table.h`:182
- Brief: n/a
- Parameters: none
- Details: Return all known keys (including keys whose rows have all been deleted). This allows callers (e.g. RetentionManager) to enumerate every key that ever had data, not just keys with a currently-alive row.

#### `std::optional< VersionedDocument > getAsOf(const std::string &key, Timestamp as_of) const`
- Source: `include/temporal/system_versioned_table.h`:159
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `as_of` (Timestamp): n/a
- Details: Return the version that was current at the given timestamp.

#### `const Config & getConfig() const noexcept`
- Source: `include/temporal/system_versioned_table.h`:257
- Brief: n/a
- Parameters: none
- Details: Returns the active Config for this table.

#### `std::optional< VersionedDocument > getCurrent(const std::string &key) const`
- Source: `include/temporal/system_versioned_table.h`:156
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
- Details: Return the current version of a row, if it exists.

#### `std::vector< VersionedDocument > getHistory(const std::string &key) const`
- Source: `include/temporal/system_versioned_table.h`:163
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
- Details: Return all historical versions of a row.

#### `std::vector< VersionedDocument > getHistoryInRange(const std::string &key, const TimeRange &range) const`
- Source: `include/temporal/system_versioned_table.h`:168
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `range` (const TimeRange &): n/a
- Details: Return all versions of a row whose sys_time overlaps the given range.

#### `nlohmann::json getStatistics() const`
- Source: `include/temporal/system_versioned_table.h`:266
- Brief: n/a
- Parameters: none
- Details: JSON statistics for monitoring.

#### `bool insert(const std::string &key, const Document &doc)`
- Source: `include/temporal/system_versioned_table.h`:129
- Brief: Insert.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `doc` (const Document &): Input parameter.
- Return: True when the operation succeeds.
- Details: Insert a new row. Fails (returns false) if the key already exists. key Input parameter. doc Input parameter. True when the operation succeeds. Calls: lock(), now(), push_back(), makeVersion().

#### `size_t keyCount() const`
- Source: `include/temporal/system_versioned_table.h`:260
- Brief: n/a
- Parameters: none
- Details: Number of distinct keys (including deleted ones).

#### `VersionedDocument makeVersion(const std::string &key, Document data, Timestamp ts) const`
- Source: `include/temporal/system_versioned_table.h`:284
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `data` (Document): n/a
  - `ts` (Timestamp): n/a

#### `size_t purgeHistoricalVersions(const std::function< bool(const VersionedDocument &)> &predicate)`
- Source: `include/temporal/system_versioned_table.h`:217
- Brief: n/a
- Parameters:
  - `predicate` (const std::function< bool(const VersionedDocument &)> &): n/a
- Return: Total number of versions removed.
- Details: Convenience overload: purge all historical versions across every known key that satisfy the predicate. Total number of versions removed.

#### `size_t purgeHistoricalVersions(const std::string &key, const std::function< bool(const VersionedDocument &)> &predicate)`
- Source: `include/temporal/system_versioned_table.h`:194
- Brief: n/a
- Parameters:
  - `key` (const std::string &): The row key to purge.
  - `predicate` (const std::function< bool(const VersionedDocument &)> &): Return true for versions that should be deleted.
- Return: The number of versions actually removed.
- Details: Physically remove all closed (historical) versions for the given key that match the supplied predicate. The current (open-ended) version is NEVER removed. key The row key to purge. predicate Return true for versions that should be deleted. The number of versions actually removed.

#### `size_t purgeHistoricalVersionsKeepLatestN(const std::string &key, size_t keep_latest_n)`
- Source: `include/temporal/system_versioned_table.h`:208
- Brief: Purge Historical Versions Keep Latest N.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `keep_latest_n` (size_t): Input parameter.
- Return: Number of versions physically removed.
- Details: Physically keep only the keep_latest_n most-recent historical (closed) versions for a key, deleting the rest. The current (open-ended) version is always kept regardless of n. key Row key. keep_latest_n Number of historical versions to retain. Number of versions physically removed. key Input parameter. keep_latest_n Input parameter. Return value. Calls: lock(), find(), end(), isCurrent(), push_back(), size(), std::sort(), begin().

#### `bool replaceHistoricalPayload(const std::string &key, Timestamp sys_start, const Document &new_data)`
- Source: `include/temporal/system_versioned_table.h`:235
- Brief: ============================================================================ Payload replacement (used by TemporalCompressor) ============================================================================
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `sys_start` (Timestamp): Input parameter.
  - `new_data` (const Document &): Input parameter.
- Return: true if the version was found and replaced.
- Details: Replace the data payload of an existing historical version in-place. Identifies the version by key and the exact sys_start timestamp. Only closed (non-current) versions may be replaced; attempting to replace a current version returns false. This method is intended for use by TemporalCompressor to substitute a compressed payload without altering the version's time metadata. key Row key. sys_start sys_time.start of the target version. new_data Replacement payload (may be compressed). true if the version was found and replaced. key Input parameter. sys_start Input parameter. new_data Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), isCurrent().

#### `std::vector< VersionedDocument > scan(Timestamp as_of=kMaxTimestamp) const`
- Source: `include/temporal/system_versioned_table.h`:175
- Brief: n/a
- Parameters:
  - `as_of` (Timestamp): n/a
- Details: Return all current rows as a snapshot at the given timestamp. When as_of == kMaxTimestamp the latest current rows are returned.

#### `const std::string & tableName() const noexcept`
- Source: `include/temporal/system_versioned_table.h`:254
- Brief: n/a
- Parameters: none

#### `bool update(const std::string &key, const Document &updates)`
- Source: `include/temporal/system_versioned_table.h`:136
- Brief: Update.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `updates` (const Document &): Input parameter.
- Return: True when the operation succeeds.
- Details: Update an existing current row. The previous version is closed (sys_end set to now) and a new version is opened. Returns false if no current row exists for the key. key Input parameter. updates Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), now(), items(), push_back(), makeVersion(), std::move().

#### `bool upsert(const std::string &key, const Document &doc)`
- Source: `include/temporal/system_versioned_table.h`:145
- Brief: Upsert.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `doc` (const Document &): Input parameter.
- Return: true if an insert was performed, false if an update was performed.
- Details: Insert a new row or update an existing one atomically. If a current version exists the row is updated (patch-merge semantics); otherwise a fresh row is inserted. true if an insert was performed, false if an update was performed. key Input parameter. doc Input parameter. True when the operation succeeds. Calls: lock(), now(), push_back(), makeVersion(), closeCurrentVersion(), items(), std::move().

#### `size_t versionCount() const`
- Source: `include/temporal/system_versioned_table.h`:263
- Brief: n/a
- Parameters: none
- Details: Total number of row versions stored (current + historical).

### themisdb::temporal::TemporalAggregator

#### `TemporalAggregator()=default`
- Source: `include/temporal/temporal_aggregator.h`:131
- Brief: n/a
- Parameters: none

#### `std::vector< AggregateResult > aggregate(const SystemVersionedTable &table, const AggregationSpec &spec) const`
- Source: `include/temporal/temporal_aggregator.h`:150
- Brief: n/a
- Parameters:
  - `table` (const SystemVersionedTable &): n/a
  - `spec` (const AggregationSpec &): n/a
- Details: Convenience overload that uses the full available time range.

#### `std::vector< AggregateResult > aggregate(const SystemVersionedTable &table, const AggregationSpec &spec, Timestamp from, Timestamp to) const`
- Source: `include/temporal/temporal_aggregator.h`:142
- Brief: n/a
- Parameters:
  - `table` (const SystemVersionedTable &): Source table (all current + historical rows are scanned).
  - `spec` (const AggregationSpec &): Window and aggregation specification.
  - `from` (Timestamp): Query range start (inclusive, ms epoch).
  - `to` (Timestamp): Query range end (exclusive, ms epoch).
- Return: One AggregateResult per window, ordered by window_start.
- Details: Run a windowed aggregation. table Source table (all current + historical rows are scanned). spec Window and aggregation specification. from Query range start (inclusive, ms epoch). to Query range end (exclusive, ms epoch). One AggregateResult per window, ordered by window_start.

#### `std::map< std::string, std::vector< AggregateResult > > aggregateByGroup(const SystemVersionedTable &table, const AggregationSpec &spec) const`
- Source: `include/temporal/temporal_aggregator.h`:179
- Brief: n/a
- Parameters:
  - `table` (const SystemVersionedTable &): n/a
  - `spec` (const AggregationSpec &): n/a
- Details: Convenience overload using the full available time range.

#### `std::map< std::string, std::vector< AggregateResult > > aggregateByGroup(const SystemVersionedTable &table, const AggregationSpec &spec, Timestamp from, Timestamp to) const`
- Source: `include/temporal/temporal_aggregator.h`:172
- Brief: n/a
- Parameters:
  - `table` (const SystemVersionedTable &): Source table.
  - `spec` (const AggregationSpec &): Window and aggregation specification, including group_by_fields.
  - `from` (Timestamp): Query range start (inclusive, ms epoch).
  - `to` (Timestamp): Query range end (exclusive, ms epoch).
- Return: Map from group key → ordered window results.
- Details: Grouped windowed aggregation (temporal GROUP BY). Partitions all in-range rows by the combination of values specified in spec.group_by_fields, then runs the requested window aggregation independently for each group. The returned map is keyed by a canonical group-key string (field1=value1\|field2=value2). Each AggregateResult in the vectors has its group_values map populated. When spec.group_by_fields is empty the call falls back to the standard aggregate() behaviour and returns a single entry keyed by the empty string (""), with group_values left empty. table Source table. spec Window and aggregation specification, including group_by_fields. from Query range start (inclusive, ms epoch). to Query range end (exclusive, ms epoch). Map from group key → ordered window results.

#### `std::vector< AggregateResult > aggregateSnapshots(const SystemVersionedTable &table, const AggregationSpec &spec, Timestamp from, Timestamp to) const`
- Source: `include/temporal/temporal_aggregator.h`:202
- Brief: n/a
- Parameters:
  - `table` (const SystemVersionedTable &): Source table.
  - `spec` (const AggregationSpec &): Specification (window_size_ms used as snapshot interval; func and measure_field used for the aggregation).
  - `from` (Timestamp): Range start (inclusive, ms epoch).
  - `to` (Timestamp): Range end (exclusive, ms epoch).
- Return: One AggregateResult per snapshot tick, ordered by window_start.
- Details: Snapshot aggregation at regular intervals. At each tick t in [from, to) advancing by spec.window_size_ms, this method computes the aggregate of all rows that were CURRENT at that exact instant (sys_start ≤ t < sys_end). Each result window spans [t, t + window_size_ms) and represents the state of the table at the snapshot tick. This is distinct from the standard aggregate() call, which buckets rows by when they were WRITTEN (sys_start), not by when they were VISIBLE. table Source table. spec Specification (window_size_ms used as snapshot interval; func and measure_field used for the aggregation). from Range start (inclusive, ms epoch). to Range end (exclusive, ms epoch). One AggregateResult per snapshot tick, ordered by window_start.

#### `TrendResult analyzeTrend(const SystemVersionedTable &table, const std::string &measure_field, Timestamp from, Timestamp to, int64_t window_size_ms=0) const`
- Source: `include/temporal/temporal_aggregator.h`:223
- Brief: n/a
- Parameters:
  - `table` (const SystemVersionedTable &): Source table.
  - `measure_field` (const std::string &): JSON field to aggregate (uses SUM by default).
  - `from` (Timestamp): Range start (inclusive, ms epoch).
  - `to` (Timestamp): Range end (exclusive, ms epoch).
  - `window_size_ms` (int64_t): Tumbling window size for bucketing; 0 = auto.
- Return: TrendResult with slope, intercept, and r².
- Details: Linear trend analysis over time-windowed aggregates. The method first partitions [from, to) into tumbling windows of window_size_ms (auto-derived as range/10 when window_size_ms is 0), computes the requested aggregate per window, and then performs an ordinary-least-squares regression of value vs. window centre time. table Source table. measure_field JSON field to aggregate (uses SUM by default). from Range start (inclusive, ms epoch). to Range end (exclusive, ms epoch). window_size_ms Tumbling window size for bucketing; 0 = auto. TrendResult with slope, intercept, and r².

#### `double applyFunc(AggregateFunc func, const std::vector< double > &values, size_t count)`
- Source: `include/temporal/temporal_aggregator.h`:258
- Brief: Apply Func.
- Parameters:
  - `func` (AggregateFunc): Input parameter.
  - `values` (const std::vector< double > &): Input parameter.
  - `count` (size_t): Input parameter.
- Return: Return value.
- Details: func Input parameter. values Input parameter. count Input parameter. Return value. Calls: std::accumulate(), begin(), end(), empty(), size(), std::min_element(), std::max_element(), front().

#### `std::pair< std::string, std::map< std::string, std::string > > buildGroupKey(const Document &doc, const std::vector< std::string > &fields)`
- Source: `include/temporal/temporal_aggregator.h`:238
- Brief: n/a
- Parameters:
  - `doc` (const Document &): n/a
  - `fields` (const std::vector< std::string > &): n/a

#### `std::tuple< double, double, double > computeLinearRegression(const std::vector< double > &x, const std::vector< double > &y)`
- Source: `include/temporal/temporal_aggregator.h`:265
- Brief: n/a
- Parameters:
  - `x` (const std::vector< double > &): n/a
  - `y` (const std::vector< double > &): n/a

#### `std::vector< AggregateResult > computeSession(const std::vector< VersionedDocument > &rows, const AggregationSpec &spec, Timestamp from, Timestamp to)`
- Source: `include/temporal/temporal_aggregator.h`:252
- Brief: ── SESSION ───────────────────────────────────────────────────────────────────
- Parameters:
  - `rows` (const std::vector< VersionedDocument > &): Input parameter.
  - `spec` (const AggregationSpec &): Input parameter.
  - `from` (Timestamp): Input parameter.
  - `to` (Timestamp): Input parameter.
- Return: Return value.
- Details: rows Input parameter. spec Input parameter. from Input parameter. to Input parameter. Return value. Calls: empty(), isSet(), std::sort(), begin(), end(), clear(), push_back(), applyFunc().

#### `std::vector< AggregateResult > computeSliding(const std::vector< VersionedDocument > &rows, const AggregationSpec &spec, Timestamp from, Timestamp to)`
- Source: `include/temporal/temporal_aggregator.h`:247
- Brief: ── SLIDING ───────────────────────────────────────────────────────────────────
- Parameters:
  - `rows` (const std::vector< VersionedDocument > &): Input parameter.
  - `spec` (const AggregationSpec &): Input parameter.
  - `from` (Timestamp): Input parameter.
  - `to` (Timestamp): Input parameter.
- Return: Return value.
- Details: rows Input parameter. spec Input parameter. from Input parameter. to Input parameter. Return value. Calls: extractMeasure(), has_value(), emplace_back(), push_back(), empty(), std::sort(), begin(), end().

#### `std::vector< AggregateResult > computeTumbling(const std::vector< VersionedDocument > &rows, const AggregationSpec &spec, Timestamp from, Timestamp to)`
- Source: `include/temporal/temporal_aggregator.h`:242
- Brief: ── TUMBLING ─────────────────────────────────────────────────────────────────
- Parameters:
  - `rows` (const std::vector< VersionedDocument > &): Input parameter.
  - `spec` (const AggregationSpec &): Input parameter.
  - `from` (Timestamp): Input parameter.
  - `to` (Timestamp): Input parameter.
- Return: Return value.
- Details: rows Input parameter. spec Input parameter. from Input parameter. to Input parameter. Return value. Calls: extractMeasure(), has_value(), emplace_back(), push_back(), empty(), std::sort(), begin(), end().

#### `std::optional< double > extractMeasure(const Document &doc, const std::string &field)`
- Source: `include/temporal/temporal_aggregator.h`:232
- Brief: Extract Measure.
- Parameters:
  - `doc` (const Document &): Input parameter.
  - `field` (const std::string &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. field Input parameter. Return value. Calls: empty(), find(), end(), is_number().

### themisdb::temporal::TemporalCDC

#### `TemporalCDC(TemporalCDC &&) noexcept=default`
- Source: `include/temporal/temporal_cdc.h`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDC &&): n/a

#### `TemporalCDC(const TemporalCDC &)=delete`
- Source: `include/temporal/temporal_cdc.h`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalCDC &): n/a

#### `TemporalCDC(size_t max_log_size=kDefaultMaxLogSize, OverflowPolicy policy=OverflowPolicy::OVERWRITE)`
- Source: `include/temporal/temporal_cdc.h`:200
- Brief: n/a
- Parameters:
  - `max_log_size` (size_t): Ring-buffer capacity (number of events).
  - `policy` (OverflowPolicy): What to do when the buffer is full. Defaults to OVERWRITE (circular eviction).
- Details: Construct a CDC instance. max_log_size Ring-buffer capacity (number of events). policy What to do when the buffer is full. Defaults to OVERWRITE (circular eviction).

#### `ChangeType changeTypeFromString(const std::string &s)`
- Source: `include/temporal/temporal_cdc.h`:298
- Brief: Change Type From String.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: Convert string to ChangeType enum. s Input parameter. Return value. std::invalid_argument if an error occurs. Implements changeTypeFromString without additional internal calls.

#### `std::string changeTypeName(ChangeType ct)`
- Source: `include/temporal/temporal_cdc.h`:295
- Brief: Change Type Name.
- Parameters:
  - `ct` (ChangeType): Input parameter.
- Return: Return value.
- Details: Convert ChangeType enum to string representation. ct Input parameter. Return value. Implements changeTypeName without additional internal calls.

#### `void clearLog()`
- Source: `include/temporal/temporal_cdc.h`:290
- Brief: Clear Log.
- Parameters: none
- Details: Clear the in-process event log. Active subscriptions are unaffected. Calls: lk(), clear().

#### `size_t logSize() const`
- Source: `include/temporal/temporal_cdc.h`:267
- Brief: n/a
- Parameters: none
- Details: Return the total number of events in the log (≤ max_log_size).

#### `TemporalCDC & operator=(TemporalCDC &&) noexcept=default`
- Source: `include/temporal/temporal_cdc.h`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCDC &&): n/a

#### `TemporalCDC & operator=(const TemporalCDC &)=delete`
- Source: `include/temporal/temporal_cdc.h`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalCDC &): n/a

#### `uint64_t overflowCount() const noexcept`
- Source: `include/temporal/temporal_cdc.h`:287
- Brief: n/a
- Parameters: none
- Details: Return the total number of events that have been silently discarded due to ring-buffer overflow since construction. An overflow occurs when publishEvent() is called while the in-process log already holds max_log_size events. The oldest event is evicted (OVERWRITE policy) and this counter is incremented. A non-zero value indicates that replayChanges() may no longer return the complete history. Consumers that require guaranteed delivery should use the subscription API instead.

#### `void publishEvent(const ChangeEvent &event)`
- Source: `include/temporal/temporal_cdc.h`:246
- Brief: Publish Event.
- Parameters:
  - `event` (const ChangeEvent &): Input parameter.
- Details: Publish a change event. Appends the event to the in-process log (evicting the oldest event if the ring-buffer is full). Invokes all matching subscribers synchronously. This method is intended to be called from write paths (insert/update/ delete) inside the temporal module. event Input parameter. Calls: void(), lk(), size(), fetch_add(), erase(), begin(), push_back(), empty().

#### `std::vector< ChangeEvent > replayChanges(const std::string &table_name, const TimeRange &range) const`
- Source: `include/temporal/temporal_cdc.h`:260
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a
  - `range` (const TimeRange &): n/a
- Details: Replay historical change events from the in-process log. Returns events whose transaction_time falls in the half-open interval [range.start, range.end) and whose table_name matches. Pass an empty table_name to replay events for all tables. Only events retained in the ring-buffer are available. Events evicted due to log overflow are permanently lost.

#### `std::string subscribeToChanges(const std::string &table_name, std::function< void(const ChangeEvent &)> callback)`
- Source: `include/temporal/temporal_cdc.h`:221
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): Table to monitor. Pass an empty string to receive events for ALL tables.
  - `callback` (std::function< void(const ChangeEvent &)>): Invoked synchronously for each matching event.
- Return: Subscription ID for use with unsubscribe().
- Details: Subscribe to change events for a specific table. table_name Table to monitor. Pass an empty string to receive events for ALL tables. callback Invoked synchronously for each matching event. Subscription ID for use with unsubscribe().

#### `size_t subscriptionCount() const`
- Source: `include/temporal/temporal_cdc.h`:232
- Brief: n/a
- Parameters: none
- Details: Return the number of active subscriptions.

#### `uint64_t totalPublished() const noexcept`
- Source: `include/temporal/temporal_cdc.h`:273
- Brief: n/a
- Parameters: none
- Details: Return the total number of events ever published (monotonically increasing, wraps on overflow).

#### `bool unsubscribe(const std::string &sub_id)`
- Source: `include/temporal/temporal_cdc.h`:229
- Brief: Unsubscribe.
- Parameters:
  - `sub_id` (const std::string &): Identifier of the sub.
- Return: true if the subscription was found and removed.
- Details: Cancel a subscription. true if the subscription was found and removed. sub_id Identifier of the sub. True when the operation succeeds. Calls: lk(), erase().

#### `~TemporalCDC()=default`
- Source: `include/temporal/temporal_cdc.h`:209
- Brief: n/a
- Parameters: none

### themisdb::temporal::TemporalColdStore

#### `TemporalColdStore(const TemporalColdStore &)=delete`
- Source: `include/temporal/temporal_cold_store.h`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalColdStore &): n/a

#### `TemporalColdStore(std::unique_ptr< IColdStoreBackend > backend=nullptr)`
- Source: `include/temporal/temporal_cold_store.h`:284
- Brief: Construct with a custom backend.
- Parameters:
  - `backend` (std::unique_ptr< IColdStoreBackend >): n/a
- Details: If backend is nullptr an InMemoryBackend is created automatically. When using FileSystemBackend the constructor calls rebuildIndexFromBackend() to populate the RAM index from disk.

#### `const IColdStoreBackend & backend() const noexcept`
- Source: `include/temporal/temporal_cold_store.h`:366
- Brief: n/a
- Parameters: none

#### `IColdStoreBackend & backend() noexcept`
- Source: `include/temporal/temporal_cold_store.h`:365
- Brief: n/a
- Parameters: none
- Details: Access the underlying backend (for testing / inspection).

#### `uint64_t biasedTimestamp(Timestamp t) noexcept`
- Source: `include/temporal/temporal_cold_store.h`:405
- Brief: n/a
- Parameters:
  - `t` (Timestamp): n/a

#### `void clear()`
- Source: `include/temporal/temporal_cold_store.h`:320
- Brief: Clear.
- Parameters: none
- Details: Remove everything (index + backend). Calls: lk(), clearAll().

#### `std::string encodeKey(const std::string &table_name, const std::string &doc_key, Timestamp sys_start)`
- Source: `include/temporal/temporal_cold_store.h`:390
- Brief: Encode a (table, doc_key, sys_start) triple into a sortable composite key.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `doc_key` (const std::string &): Input parameter.
  - `sys_start` (Timestamp): Input parameter.
- Return: Return value.
- Details: static Format: <table>\x01<doc_key>\x01<016llx biased timestamp> table_name Name of the table. doc_key Input parameter. sys_start Input parameter. Return value. Calls: std::snprintf(), biasedTimestamp(), reserve(), size().

#### `std::vector< VersionedDocument > getAll(const std::string &table_name, const std::string &doc_key) const`
- Source: `include/temporal/temporal_cold_store.h`:341
- Brief: Return all cold-tier versions for (table_name, doc_key), sorted ascending by sys_start.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a

#### `std::optional< VersionedDocument > getAsOf(const std::string &table_name, const std::string &doc_key, Timestamp as_of) const`
- Source: `include/temporal/temporal_cold_store.h`:333
- Brief: Return the version of (table_name, doc_key) valid at timestamp as_of, i.e. v.sys_time.contains(as_of).
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a
  - `as_of` (Timestamp): n/a
- Return: nullopt if no matching cold-tier version exists.
- Details: Uses upper_bound(prefix + encode(as_of)) on the RAM index — O(log N) — then issues at most O(1) backend reads in the common case. nullopt if no matching cold-tier version exists.

#### `std::vector< VersionedDocument > getRange(const std::string &table_name, const std::string &doc_key, const TimeRange &range) const`
- Source: `include/temporal/temporal_cold_store.h`:348
- Brief: Return cold-tier versions whose sys_time overlaps range, sorted ascending by sys_start.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a
  - `range` (const TimeRange &): n/a

#### `std::string keyPrefix(const std::string &table_name, const std::string &doc_key)`
- Source: `include/temporal/temporal_cold_store.h`:395
- Brief: static
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `doc_key` (const std::string &): Input parameter.
- Return: Return value.
- Details: Prefix for all versions of (table_name, doc_key): <table>\x01<key>\x01 table_name Name of the table. doc_key Input parameter. Return value. Calls: reserve(), size().

#### `TemporalColdStore & operator=(const TemporalColdStore &)=delete`
- Source: `include/temporal/temporal_cold_store.h`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalColdStore &): n/a

#### `std::optional< VersionedDocument > parseDocument(const std::string &json_str)`
- Source: `include/temporal/temporal_cold_store.h`:409
- Brief: Deserialise a JSON string into a VersionedDocument.
- Parameters:
  - `json_str` (const std::string &): n/a

#### `void rebuildIndexFromBackend()`
- Source: `include/temporal/temporal_cold_store.h`:380
- Brief: Rebuild the RAM index from the backend.
- Parameters: none
- Details: Rebuild Index From Backend. Called automatically by the constructor when a non-null backend is provided. May also be called manually after crash recovery. For InMemoryBackend this is a no-op (the RAM map already holds all keys). For FileSystemBackend it scans base_dir and reconstructs key_index_ in O(N) time. Calls: listKeysWithPrefix(), lk(), clear(), insert(), std::move(), size(), load().

#### `size_t remove(const std::string &table_name, const std::string &doc_key)`
- Source: `include/temporal/temporal_cold_store.h`:311
- Brief: Remove all cold-tier versions for (table_name, doc_key).
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `doc_key` (const std::string &): Input parameter.
- Return: Number of versions removed.
- Details: Remove. Number of versions removed. table_name Name of the table. doc_key Input parameter. Return value. Calls: keyPrefix(), lk(), lower_bound(), end(), substr(), size(), push_back(), del().

#### `size_t removeTable(const std::string &table_name)`
- Source: `include/temporal/temporal_cold_store.h`:317
- Brief: Remove all cold-tier versions for table_name.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
- Return: Number of versions removed.
- Details: Remove Table. Number of versions removed. table_name Name of the table. Return value. Calls: tablePrefix(), lk(), lower_bound(), end(), substr(), size(), push_back(), del().

#### `ColdStoreStats stats() const`
- Source: `include/temporal/temporal_cold_store.h`:362
- Brief: n/a
- Parameters: none
- Details: Snapshot of cumulative statistics.

#### `bool store(const std::string &table_name, const VersionedDocument &doc)`
- Source: `include/temporal/temporal_cold_store.h`:305
- Brief: Persist a historical version to the cold store.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `doc` (const VersionedDocument &): Input parameter.
- Return: true on success; false if doc.isCurrent() is true or on backend write failure.
- Details: Store. Storing a version that is still current (isCurrent() == true) is explicitly rejected — only closed (non-current) versions belong in the cold tier. table_name Logical table the version belongs to. doc The historical VersionedDocument to store. true on success; false if doc.isCurrent() is true or on backend write failure. table_name Name of the table. doc Input parameter. True when the operation succeeds. Calls: isCurrent(), encodeKey(), toJson(), dump(), put(), lk(), insert().

#### `std::string tablePrefix(const std::string &table_name)`
- Source: `include/temporal/temporal_cold_store.h`:399
- Brief: static
- Parameters:
  - `table_name` (const std::string &): Name of the table.
- Return: Return value.
- Details: Prefix for all versions of table_name: <table>\x01 table_name Name of the table. Return value. Implements tablePrefix without additional internal calls.

#### `size_t totalVersionCount() const noexcept`
- Source: `include/temporal/temporal_cold_store.h`:359
- Brief: n/a
- Parameters: none
- Details: Total versions in the RAM index. O(1).

#### `size_t versionCount(const std::string &table_name, const std::string &doc_key) const`
- Source: `include/temporal/temporal_cold_store.h`:355
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a
- Details: Number of cold-tier versions for (table_name, doc_key). O(log N + k).

### themisdb::temporal::TemporalCompressor

#### `TemporalCompressor()=default`
- Source: `include/temporal/temporal_compressor.h`:175
- Brief: n/a
- Parameters: none

#### `TemporalCompressor(TemporalCompressor &&) noexcept=default`
- Source: `include/temporal/temporal_compressor.h`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressor &&): n/a

#### `TemporalCompressor(const TemporalCompressor &)=delete`
- Source: `include/temporal/temporal_compressor.h`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalCompressor &): n/a

#### `std::string algorithmName(CompressionAlgorithm algo)`
- Source: `include/temporal/temporal_compressor.h`:212
- Brief: Algorithm Name.
- Parameters:
  - `algo` (CompressionAlgorithm): Input parameter.
- Return: Return value.
- Details: Return the algorithm name as a string. algo Input parameter. Return value. Implements algorithmName without additional internal calls.

#### `nlohmann::json applyDelta(const nlohmann::json &base, const nlohmann::json &current, const std::string &base_ref)`
- Source: `include/temporal/temporal_compressor.h`:217
- Brief: ============================================================================ Algorithm: DELTA (JSON field-level patch) A delta payload looks like: { "__compressed": "delta", "__base_ref": "<key>@<sys_start_ms>", "__patch": { <field>: <new_value>, .
- Parameters:
  - `base` (const nlohmann::json &): Input parameter.
  - `current` (const nlohmann::json &): Input parameter.
  - `base_ref` (const std::string &): Input parameter.
- Return: Return value.
- Details: base Input parameter. current Input parameter. base_ref Input parameter. Return value. .. }, "__removed": [<field>, ...] } Fields absent from __patch and __removed are unchanged from the base. ============================================================================ Calls: nlohmann::json::object(), nlohmann::json::array(), items(), contains(), at(), push_back().

#### `nlohmann::json applyDictionary(const nlohmann::json &doc, std::unordered_map< std::string, std::unordered_map< std::string, int > > &dicts)`
- Source: `include/temporal/temporal_compressor.h`:237
- Brief: n/a
- Parameters:
  - `doc` (const nlohmann::json &): n/a
  - `dicts` (std::unordered_map< std::string, std::unordered_map< std::string, int > > &): n/a

#### `nlohmann::json applyGorilla(const std::string &field_name, const std::vector< std::pair< Timestamp, double > > &series)`
- Source: `include/temporal/temporal_compressor.h`:233
- Brief: Build a Gorilla-encoded payload from a vector of (timestamp, value) pairs.
- Parameters:
  - `field_name` (const std::string &): n/a
  - `series` (const std::vector< std::pair< Timestamp, double > > &): n/a

#### `nlohmann::json applyLz4(const nlohmann::json &doc)`
- Source: `include/temporal/temporal_compressor.h`:227
- Brief: Compress a JSON document with LZ4 block format.
- Parameters:
  - `doc` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: ============================================================================ ============================================================================ Algorithm: LZ4 — high-throughput block compression ============================================================================ Payload format stored in the history table: { "__compressed": "lz4", "__original_size": <int>, // original JSON string byte count "__data": "<base64-encoded LZ4 compressed block>" } doc Input parameter. Return value. Calls: dump(), size(), LZ4_compressBound(), dst(), LZ4_compress_default(), data(), resize(), base64Encode().

#### `nlohmann::json applyZstd(const nlohmann::json &doc, int level)`
- Source: `include/temporal/temporal_compressor.h`:221
- Brief: ============================================================================ Algorithm: ZSTD (simulated via RL-encode + base64) ============================================================================
- Parameters:
  - `doc` (const nlohmann::json &): Input parameter.
  - `level` (int): n/a
- Return: Return value.
- Details: doc Input parameter. int Input parameter. Return value. Calls: dump(), base64Encode(), rlEncode(), size().

#### `std::string base64Decode(const std::string &input)`
- Source: `include/temporal/temporal_compressor.h`:244
- Brief: Base64 Decode.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. Calls: empty(), size(), reserve(), base64CharValue(), uint32_t(), char().

#### `std::string base64Encode(const std::string &input)`
- Source: `include/temporal/temporal_compressor.h`:243
- Brief: Base64 Encode.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. Calls: reserve(), size(), data(), uint32_t().

#### `CompressionStats compressHistory(SystemVersionedTable &table, const TimeRange &range, const CompressionConfig &config)`
- Source: `include/temporal/temporal_compressor.h`:198
- Brief: Compress History.
- Parameters:
  - `table` (SystemVersionedTable &): Input/output parameter.
  - `range` (const TimeRange &): Input parameter.
  - `config` (const CompressionConfig &): Input parameter.
- Return: Aggregated statistics for the compression pass.
- Details: Compress historical versions of all keys in the given time range. Versions whose sys_start timestamp is within the delay_before_compression grace window are silently skipped unless config.compress_immediately == true. table The system-versioned table whose history is compressed. range Only versions with sys_start in [range.start, range.end) are considered. config Algorithm and tuning parameters. Aggregated statistics for the compression pass. table Input/output parameter. range Input parameter. config Input parameter. Return value. Calls: lk(), std::chrono::steady_clock::now(), count(), now(), getAllKeys(), getHistoryInRange(), isCurrent(), push_back().

#### `nlohmann::json decompress(const nlohmann::json &compressed)`
- Source: `include/temporal/temporal_compressor.h`:207
- Brief: Decompress.
- Parameters:
  - `compressed` (const nlohmann::json &): n/a
- Return: Return value.
- Details: Decompress a payload that was previously compressed by this class. Returns the original JSON document on success, or the input unchanged if it is not a compressed payload. doc Input parameter. Return value. Calls: is_object(), contains(), decompressZstd(), decompressLz4().

#### `nlohmann::json decompressLz4(const nlohmann::json &doc)`
- Source: `include/temporal/temporal_compressor.h`:230
- Brief: Decompress a payload that was compressed with applyLz4().
- Parameters:
  - `doc` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: Decompress Lz4. doc Input parameter. Return value. Calls: contains(), base64Decode(), decompressed(), LZ4_decompress_safe(), data(), size(), nlohmann::json::parse().

#### `nlohmann::json decompressZstd(const nlohmann::json &doc)`
- Source: `include/temporal/temporal_compressor.h`:224
- Brief: Decompress Zstd.
- Parameters:
  - `doc` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: doc Input parameter. Return value. Calls: at(), rlDecode(), base64Decode(), nlohmann::json::parse().

#### `TemporalCompressor & operator=(TemporalCompressor &&) noexcept=default`
- Source: `include/temporal/temporal_compressor.h`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalCompressor &&): n/a

#### `TemporalCompressor & operator=(const TemporalCompressor &)=delete`
- Source: `include/temporal/temporal_compressor.h`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalCompressor &): n/a

#### `std::string rlDecode(const std::string &input)`
- Source: `include/temporal/temporal_compressor.h`:247
- Brief: Rl Decode.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. Calls: reserve(), size().

#### `std::string rlEncode(const std::string &input)`
- Source: `include/temporal/temporal_compressor.h`:246
- Brief: Rl Encode.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. Calls: empty(), reserve(), size().

### themisdb::temporal::TemporalConflictDetector

#### `TemporalConflictDetector()=default`
- Source: `include/temporal/temporal_conflict_resolver.h`:358
- Brief: n/a
- Parameters: none

#### `std::optional< TemporalSnapshot > autoResolveConflict(const Conflict &conflict, ConflictPolicy policy)`
- Source: `include/temporal/temporal_conflict_resolver.h`:381
- Brief: Auto Resolve Conflict.
- Parameters:
  - `conflict` (const Conflict &): Input parameter.
  - `policy` (ConflictPolicy): Input parameter.
- Return: The winning snapshot, or std::nullopt when policy is MANUAL (call queueForManualResolution instead).
- Details: Automatically resolve conflict using policy. The winning snapshot, or std::nullopt when policy is MANUAL (call queueForManualResolution instead). conflict Input parameter. policy Input parameter. Return value. Calls: resolver(), resolve().

#### `void clearQueue()`
- Source: `include/temporal/temporal_conflict_resolver.h`:408
- Brief: Clear Queue.
- Parameters: none
- Details: Remove all entries from the manual-resolution queue. Calls: lock(), clear().

#### `std::optional< Conflict > detectConcurrentUpdate(const TemporalSnapshot &local, const TemporalSnapshot &remote)`
- Source: `include/temporal/temporal_conflict_resolver.h`:420
- Brief: Sub-detectors — each returns an optional Conflict if detected.
- Parameters:
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
- Return: Return value.
- Details: Detect Concurrent Update. local Input parameter. remote Input parameter. Return value. Calls: is_object(), items(), contains(), push_back(), std::move().

#### `std::vector< Conflict > detectConflicts(const std::string &table_name, const TemporalSnapshot &local, const TemporalSnapshot &remote)`
- Source: `include/temporal/temporal_conflict_resolver.h`:369
- Brief: Detect Conflicts.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
- Return: A (possibly empty) list of detected Conflict objects. An empty list means the two snapshots are compatible.
- Details: Detect all conflicts between local and remote for table_name. Each returned Conflict has its table_name set to table_name and its entity_id set to local.snapshot_id. A (possibly empty) list of detected Conflict objects. An empty list means the two snapshots are compatible. table_name Name of the table. local Input parameter. remote Input parameter. Return value. Calls: detectConcurrentUpdate(), push_back(), std::move(), detectOverlappingPeriods(), detectReferentialIntegrity(), detectUniquenessViolation().

#### `std::optional< Conflict > detectOverlappingPeriods(const TemporalSnapshot &local, const TemporalSnapshot &remote)`
- Source: `include/temporal/temporal_conflict_resolver.h`:425
- Brief: Detect Overlapping Periods.
- Parameters:
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
- Return: Return value.
- Details: local Input parameter. remote Input parameter. Return value. Calls: is_object(), contains(), at(), is_number_integer().

#### `std::optional< Conflict > detectReferentialIntegrity(const TemporalSnapshot &local, const TemporalSnapshot &remote)`
- Source: `include/temporal/temporal_conflict_resolver.h`:430
- Brief: Detect Referential Integrity.
- Parameters:
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
- Return: Return value.
- Details: local Input parameter. remote Input parameter. Return value. Calls: is_object(), contains(), at().

#### `std::optional< Conflict > detectUniquenessViolation(const TemporalSnapshot &local, const TemporalSnapshot &remote)`
- Source: `include/temporal/temporal_conflict_resolver.h`:435
- Brief: Detect Uniqueness Violation.
- Parameters:
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
- Return: Return value.
- Details: local Input parameter. remote Input parameter. Return value. Calls: is_object(), items(), contains(), push_back(), empty(), std::move().

#### `std::vector< Conflict > getQueuedConflicts() const`
- Source: `include/temporal/temporal_conflict_resolver.h`:403
- Brief: n/a
- Parameters: none
- Details: Return a snapshot of all currently queued conflicts.

#### `std::string makeQueueKey(const std::string &table_name, const Conflict &conflict)`
- Source: `include/temporal/temporal_conflict_resolver.h`:416
- Brief: Helper: generate a deterministic dedup key for a conflict.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `conflict` (const Conflict &): Input parameter.
- Return: Return value.
- Details: Make Queue Key. table_name Name of the table. conflict Input parameter. Return value. Implements makeQueueKey without additional internal calls.

#### `bool queueForManualResolution(const std::string &table_name, const Conflict &conflict)`
- Source: `include/temporal/temporal_conflict_resolver.h`:397
- Brief: Queue For Manual Resolution.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `conflict` (const Conflict &): Input parameter.
- Return: true if the conflict was queued; false if an identical conflict entry is already in the queue.
- Details: Queue conflict for manual resolution within table_name. The stored entry has its table_name field overwritten with table_name so the queued conflict is always self-consistent. The dedup key is derived from table_name + entity_id + conflict type + both snapshot IDs, so the same logical conflict is never queued twice for the same table. true if the conflict was queued; false if an identical conflict entry is already in the queue. table_name Name of the table. conflict Input parameter. True when the operation succeeds. Calls: makeQueueKey(), lock(), count(), std::move().

### themisdb::temporal::TemporalConflictResolver

#### `TemporalConflictResolver(ConflictPolicy default_policy=ConflictPolicy::LAST_WRITE_WINS)`
- Source: `include/temporal/temporal_conflict_resolver.h`:214
- Brief: n/a
- Parameters:
  - `default_policy` (ConflictPolicy): n/a

#### `nlohmann::json exportAuditLog() const`
- Source: `include/temporal/temporal_conflict_resolver.h`:251
- Brief: n/a
- Parameters: none
- Details: Export the complete conflict history as a JSON array. Each entry contains: conflict_id, entity_id, winner, policy, resolved, detected_at_ms.

#### `std::string generateConflictId() const`
- Source: `include/temporal/temporal_conflict_resolver.h`:293
- Brief: n/a
- Parameters: none

#### `std::vector< ConflictRecord > getConflictHistory() const`
- Source: `include/temporal/temporal_conflict_resolver.h`:244
- Brief: n/a
- Parameters: none
- Details: Get the complete conflict history (resolved + unresolved). Useful for audit, compliance and replay.

#### `std::shared_ptr< MergeResolver > getMergeResolver() const`
- Source: `include/temporal/temporal_conflict_resolver.h`:277
- Brief: Return the currently active MergeResolver.
- Parameters: none
- Details: Returns nullptr when the built-in LWW-per-field default is active.

#### `nlohmann::json getStatistics() const`
- Source: `include/temporal/temporal_conflict_resolver.h`:256
- Brief: n/a
- Parameters: none
- Details: Get conflict statistics

#### `std::vector< ConflictRecord > getUnresolvedConflicts() const`
- Source: `include/temporal/temporal_conflict_resolver.h`:233
- Brief: n/a
- Parameters: none
- Details: Get all unresolved conflicts (for MANUAL policy)

#### `TemporalSnapshot resolve(const TemporalSnapshot &local, const TemporalSnapshot &remote, std::optional< ConflictPolicy > policy=std::nullopt)`
- Source: `include/temporal/temporal_conflict_resolver.h`:224
- Brief: Resolve.
- Parameters:
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
  - `policy` (std::optional< ConflictPolicy >): Input parameter.
- Return: Winning snapshot (merged or selected)
- Details: Resolve conflict between local and remote snapshot local Local snapshot version remote Remote snapshot version policy Override default policy (optional) Winning snapshot (merged or selected) local Input parameter. remote Input parameter. policy Input parameter. Return value. Calls: fetch_add(), value_or(), generateConflictId(), std::chrono::system_clock::now(), resolveLastWriteWins(), resolveFirstWriteWins(), resolveNodePriority(), resolveCRDT().

#### `TemporalSnapshot resolveCRDT(const TemporalSnapshot &local, const TemporalSnapshot &remote)`
- Source: `include/temporal/temporal_conflict_resolver.h`:298
- Brief: Resolve CRDT.
- Parameters:
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
- Return: Return value.
- Details: local Input parameter. remote Input parameter. Return value. Calls: lk(), merge().

#### `TemporalSnapshot resolveFirstWriteWins(const TemporalSnapshot &local, const TemporalSnapshot &remote)`
- Source: `include/temporal/temporal_conflict_resolver.h`:296
- Brief: Resolve First Write Wins.
- Parameters:
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
- Return: Return value.
- Details: local Input parameter. remote Input parameter. Return value. Implements resolveFirstWriteWins without additional internal calls.

#### `TemporalSnapshot resolveLastWriteWins(const TemporalSnapshot &local, const TemporalSnapshot &remote)`
- Source: `include/temporal/temporal_conflict_resolver.h`:295
- Brief: Resolve Last Write Wins.
- Parameters:
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
- Return: Return value.
- Details: local Input parameter. remote Input parameter. Return value. Implements resolveLastWriteWins without additional internal calls.

#### `void resolveManually(const std::string &conflict_id, const std::string &winner)`
- Source: `include/temporal/temporal_conflict_resolver.h`:238
- Brief: Resolve Manually.
- Parameters:
  - `conflict_id` (const std::string &): Identifier of the conflict.
  - `winner` (const std::string &): Input parameter.
- Details: Manually resolve a conflict conflict_id Identifier of the conflict. winner Input parameter. Calls: lock(), find(), end(), push_back(), erase(), fetch_add().

#### `TemporalSnapshot resolveNodePriority(const TemporalSnapshot &local, const TemporalSnapshot &remote)`
- Source: `include/temporal/temporal_conflict_resolver.h`:297
- Brief: Resolve Node Priority.
- Parameters:
  - `local` (const TemporalSnapshot &): Input parameter.
  - `remote` (const TemporalSnapshot &): Input parameter.
- Return: Return value.
- Details: local Input parameter. remote Input parameter. Return value. Implements resolveNodePriority without additional internal calls.

#### `void setMergeResolver(std::shared_ptr< MergeResolver > resolver)`
- Source: `include/temporal/temporal_conflict_resolver.h`:270
- Brief: Inject a custom CRDT merge strategy.
- Parameters:
  - `resolver` (std::shared_ptr< MergeResolver >): Input parameter.
- Details: Set Merge Resolver. When resolver is non-null it is used by resolveCRDT() instead of the built-in LWWFieldMergeResolver. Pass nullptr to revert to the default LWW-per-field behaviour. The call is thread-safe; the resolver is replaced atomically under the internal mutex. resolver Strategy to use, or nullptr to reset to the default. resolver Input parameter. Calls: lk(), std::move().

### themisdb::temporal::TemporalForeignKey

#### `bool validate(const BiTemporalTable &parent_table, const std::string &parent_key, const TimeRange &child_period) const`
- Source: `include/temporal/bi_temporal.h`:71
- Brief: n/a
- Parameters:
  - `parent_table` (const BiTemporalTable &): n/a
  - `parent_key` (const std::string &): n/a
  - `child_period` (const TimeRange &): n/a
- Details: Validate that parent_table is the expected table and that it has at least one current row for parent_key whose valid-time period contains child_period. Returns true → referential integrity satisfied. Returns false → constraint violation: either parent_table is the wrong table (name mismatch), or no parent row covers the period.

### themisdb::temporal::TemporalIndex

#### `TemporalIndex(std::string name)`
- Source: `include/temporal/temporal_index.h`:81
- Brief: n/a
- Parameters:
  - `name` (std::string): n/a

#### `void insert(const TemporalIndexEntry &entry)`
- Source: `include/temporal/temporal_index.h`:86
- Brief: Insert.
- Parameters:
  - `entry` (const TemporalIndexEntry &): Input parameter.
- Details: Insert a new entry. entry Input parameter. Calls: lock(), emplace().

#### `const std::string & name() const noexcept`
- Source: `include/temporal/temporal_index.h`:119
- Brief: n/a
- Parameters: none

#### `std::vector< TemporalIndexEntry > queryKey(const std::string &key, std::optional< TimeRange > range=std::nullopt) const`
- Source: `include/temporal/temporal_index.h`:113
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `range` (std::optional< TimeRange >): n/a
- Details: Return all entries for a specific key, optionally filtered by range.

#### `std::vector< TemporalIndexEntry > queryPoint(Timestamp t) const`
- Source: `include/temporal/temporal_index.h`:103
- Brief: n/a
- Parameters:
  - `t` (Timestamp): n/a
- Details: Return all entries that are valid at timestamp t (i.e., entry.range.contains(t) == true).

#### `std::vector< TemporalIndexEntry > queryRange(Timestamp from, Timestamp to) const`
- Source: `include/temporal/temporal_index.h`:108
- Brief: n/a
- Parameters:
  - `from` (Timestamp): n/a
  - `to` (Timestamp): n/a
- Details: Return all entries whose range overlaps [from, to).

#### `size_t remove(const std::string &key, const TimeRange &range)`
- Source: `include/temporal/temporal_index.h`:92
- Brief: Remove.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `range` (const TimeRange &): Input parameter.
- Return: Return value.
- Details: Remove entries matching the given key and range. Returns the number of entries removed. key Input parameter. range Input parameter. Return value. Calls: lock(), begin(), end(), erase().

#### `size_t removeKey(const std::string &key)`
- Source: `include/temporal/temporal_index.h`:95
- Brief: Remove Key.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: Remove all entries for the given key. key Input parameter. Return value. Calls: lock(), begin(), end(), erase().

#### `size_t size() const`
- Source: `include/temporal/temporal_index.h`:120
- Brief: n/a
- Parameters: none

#### `TemporalIndexStats stats() const`
- Source: `include/temporal/temporal_index.h`:121
- Brief: n/a
- Parameters: none

### themisdb::temporal::TemporalIndexStats

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_index.h`:54
- Brief: n/a
- Parameters: none

### themisdb::temporal::TemporalMigrator

#### `TemporalMigrator()=default`
- Source: `include/temporal/temporal_migrator.h`:237
- Brief: n/a
- Parameters: none

#### `TemporalMigrator(TemporalMigrator &&) noexcept=default`
- Source: `include/temporal/temporal_migrator.h`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigrator &&): n/a

#### `TemporalMigrator(const TemporalMigrator &)=delete`
- Source: `include/temporal/temporal_migrator.h`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalMigrator &): n/a

#### `MigrationPlan analyzeMigration(const std::string &table_name, const std::unordered_map< std::string, Document > &source_docs)`
- Source: `include/temporal/temporal_migrator.h`:265
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): Logical name of the source table / collection.
  - `source_docs` (const std::unordered_map< std::string, Document > &): Map from row key to document payload. An empty map is accepted (the plan will have source_row_count=0).
- Return: A MigrationPlan ready for inspection and use in migrateToTemporal().
- Details: Analyse a source document collection and produce a MigrationPlan. table_name Logical name of the source table / collection. source_docs Map from row key to document payload. An empty map is accepted (the plan will have source_row_count=0). A MigrationPlan ready for inspection and use in migrateToTemporal().

#### `size_t backfillHistory(SystemVersionedTable &table, const std::vector< VersionedDocument > &history_entries)`
- Source: `include/temporal/temporal_migrator.h`:304
- Brief: ============================================================================ Step 3: backfillHistory ============================================================================
- Parameters:
  - `table` (SystemVersionedTable &): Input/output parameter.
  - `history_entries` (const std::vector< VersionedDocument > &): Input parameter.
- Return: Number of historical versions successfully inserted.
- Details: Insert historical versions from an audit log into an already-migrated SystemVersionedTable. Each VersionedDocument in history_entries is inserted as a closed (historical) version. Entries with sys_time.end == kMaxTimestamp are treated as "open" and are skipped with a warning because they would conflict with the current version inserted during migrateToTemporal(). table Target SystemVersionedTable (already migrated). history_entries Audit-log snapshots to backfill. Number of historical versions successfully inserted. table Input/output parameter. history_entries Input parameter. Return value. Calls: push_back(), getAsOf(), has_value(), insert(), replaceHistoricalPayload(), std::to_string().

#### `const MigrationReport & getLastReport() const noexcept`
- Source: `include/temporal/temporal_migrator.h`:339
- Brief: n/a
- Parameters: none
- Details: Return the MigrationReport from the most recent verifyMigration() call. Returns a default-constructed report if verifyMigration() has not been called yet.

#### `const MigrationStats & getStats() const noexcept`
- Source: `include/temporal/temporal_migrator.h`:332
- Brief: n/a
- Parameters: none
- Details: Return accumulated stats from the most recent migration run.

#### `MigrationStatus getStatus() const noexcept`
- Source: `include/temporal/temporal_migrator.h`:329
- Brief: n/a
- Parameters: none
- Details: Return the current migration lifecycle state.

#### `std::vector< ColumnInfo > inferColumns(const std::unordered_map< std::string, Document > &docs)`
- Source: `include/temporal/temporal_migrator.h`:356
- Brief: n/a
- Parameters:
  - `docs` (const std::unordered_map< std::string, Document > &): n/a
- Details: Infer column metadata by sampling all documents.

#### `std::string inferType(const nlohmann::json &value)`
- Source: `include/temporal/temporal_migrator.h`:360
- Brief: Infer Type.
- Parameters:
  - `value` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: Extract the inferred JSON type name for a single value. value Input parameter. Return value. Calls: is_null(), is_boolean(), is_number(), is_string(), is_array(), is_object().

#### `std::pair< SystemVersionedTable, bool > migrateToTemporal(const MigrationPlan &plan, const std::unordered_map< std::string, Document > &source_docs)`
- Source: `include/temporal/temporal_migrator.h`:285
- Brief: n/a
- Parameters:
  - `plan` (const MigrationPlan &): Plan produced by analyzeMigration().
  - `source_docs` (const std::unordered_map< std::string, Document > &): The actual key/document pairs to migrate.
- Return: Pair of (populated SystemVersionedTable, success flag). On failure, stats.errors describes what went wrong.
- Details: Execute the migration described by plan. Creates a SystemVersionedTable and inserts every entry from source_docs as the initial current version. The sys_start timestamp for each row is taken from plan.baseline_timestamp unless the document contains a field named "_created_at" (interpreted as a millisecond-epoch integer). plan Plan produced by analyzeMigration(). source_docs The actual key/document pairs to migrate. Pair of (populated SystemVersionedTable, success flag). On failure, stats.errors describes what went wrong.

#### `TemporalMigrator & operator=(TemporalMigrator &&) noexcept=default`
- Source: `include/temporal/temporal_migrator.h`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (TemporalMigrator &&): n/a

#### `TemporalMigrator & operator=(const TemporalMigrator &)=delete`
- Source: `include/temporal/temporal_migrator.h`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalMigrator &): n/a

#### `void setProgressCallback(ProgressCallback cb)`
- Source: `include/temporal/temporal_migrator.h`:252
- Brief: Set Progress Callback.
- Parameters:
  - `cb` (ProgressCallback): Input parameter.
- Details: Register a callback that is invoked on every MigrationStatus transition. Pass nullptr to remove any previously registered callback. cb Input parameter. Calls: std::move().

#### `void setStatus(MigrationStatus s, const std::string &msg="")`
- Source: `include/temporal/temporal_migrator.h`:353
- Brief: Set Status.
- Parameters:
  - `s` (MigrationStatus): Input parameter.
  - `msg` (const std::string &): Input parameter.
- Details: s Input parameter. msg Input parameter. Calls: progress_cb_(), empty(), statusName().

#### `std::string statusName(MigrationStatus s)`
- Source: `include/temporal/temporal_migrator.h`:344
- Brief: Status Name.
- Parameters:
  - `s` (MigrationStatus): Input parameter.
- Return: Return value.
- Details: Convert MigrationStatus to a human-readable string. s Input parameter. Return value. Implements statusName without additional internal calls.

#### `MigrationReport verifyMigration(const SystemVersionedTable &table)`
- Source: `include/temporal/temporal_migrator.h`:324
- Brief: ============================================================================ Step 4: verifyMigration ============================================================================
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
- Return: MigrationReport with all check results and aggregated stats.
- Details: Run a suite of data-integrity checks against a migrated table. Checks performed: KEY_COUNT: total key count matches plan.source_row_count (if plan was set) VERSION_ORDER: for every key, versions are in ascending sys_start order NO_OVERLAPPING_VERSIONS: no two versions for the same key overlap in sys_time CURRENT_VERSION_OPEN: all current versions have sys_time.end == kMaxTimestamp HISTORY_CONTINUITY: closed versions form a contiguous chain (no gaps) (only checked when has_history_to_backfill was true in the plan) table The migrated SystemVersionedTable to verify. MigrationReport with all check results and aggregated stats. table Input parameter. Return value. Calls: setStatus(), tableName(), keyCount(), std::to_string(), push_back(), getAllKeys(), getHistory(), size().

### themisdb::temporal::TemporalQueryEngine

#### `bool evaluatePredicate(TemporalOperator op, const TimeRange &lhs, const TimeRange &rhs) noexcept`
- Source: `include/temporal/temporal_query_engine.h`:198
- Brief: n/a
- Parameters:
  - `op` (TemporalOperator): The temporal operator to evaluate
  - `lhs` (const TimeRange &): Left-hand period
  - `rhs` (const TimeRange &): Right-hand period
- Return: true if the predicate holds
- Details: Apply a temporal predicate between two time ranges. op The temporal operator to evaluate lhs Left-hand period rhs Right-hand period true if the predicate holds

#### `std::vector< VersionedDocument > executeTemporalQuery(const BiTemporalTable &table, const TemporalQuerySpec &spec, const std::vector< RowFilter > &filters={})`
- Source: `include/temporal/temporal_query_engine.h`:397
- Brief: Execute Temporal Query.
- Parameters:
  - `table` (const BiTemporalTable &): Input parameter.
  - `spec` (const TemporalQuerySpec &): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: Execute a SQL:2011 FOR APPLICATION_TIME query over a BiTemporalTable. Dispatches based on spec.clause: AS_OF → queryApplicationTime(table, spec.start_time, filters) FROM_TO → queryApplicationTimeRange(table, spec.start_time, spec.end_time, filters) BETWEEN_AND → queryApplicationTimeRange with closed upper bound CONTAINED_IN → current rows whose valid_time ⊆ [start, end) ALL → all current rows (all valid-time periods) table Source bi-temporal table. spec Temporal query specification (clause type + timestamps). filters Optional field-level row filters. table Input parameter. spec Input parameter. filters Input parameter. Return value. Calls: queryApplicationTime(), queryApplicationTimeRange(), reserve(), size(), push_back(), std::move(), applyDeletedFilter().

#### `std::vector< VersionedDocument > executeTemporalQuery(const SystemVersionedTable &table, const TemporalQuerySpec &spec, const std::vector< RowFilter > &filters={})`
- Source: `include/temporal/temporal_query_engine.h`:377
- Brief: Execute Temporal Query.
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `spec` (const TemporalQuerySpec &): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: Execute a SQL:2011 FOR SYSTEM_TIME query over a SystemVersionedTable. Dispatches to the appropriate lower-level method based on spec.clause: AS_OF → queryAsOf(table, spec.start_time, filters) FROM_TO → queryFromTo(table, spec.start_time, spec.end_time, filters) BETWEEN_AND → queryBetween(table, spec.start_time, spec.end_time, filters) CONTAINED_IN → versions whose entire sys_time ⊆ [start, end) ALL → all stored versions (NON_SEQUENCED over full time range) When spec.include_deleted is false (default) rows whose data contains the field "deleted" with value true are excluded from the result. table Source system-versioned table. spec Temporal query specification (clause type + timestamps). filters Optional field-level row filters applied after the temporal predicate. table Input parameter. spec Input parameter. filters Input parameter. Return value. Calls: queryAsOf(), queryFromTo(), queryBetween(), reserve(), size(), push_back(), std::move(), queryWithSemantics().

#### `TimeRange intersect(const TimeRange &a, const TimeRange &b) noexcept`
- Source: `include/temporal/temporal_query_engine.h`:344
- Brief: n/a
- Parameters:
  - `a` (const TimeRange &): n/a
  - `b` (const TimeRange &): n/a
- Details: Compute the overlap intersection of two time ranges. Returns an empty range (start==end) when there is no overlap.

#### `std::vector< std::pair< VersionedDocument, VersionedDocument > > joinAsOf(const SystemVersionedTable &left, const SystemVersionedTable &right, Timestamp as_of, const std::function< bool(const VersionedDocument &, const VersionedDocument &)> &predicate)`
- Source: `include/temporal/temporal_query_engine.h`:214
- Brief: n/a
- Parameters:
  - `left` (const SystemVersionedTable &): Left-hand table.
  - `right` (const SystemVersionedTable &): Right-hand table.
  - `as_of` (Timestamp): Point-in-time for both tables.
  - `predicate` (const std::function< bool(const VersionedDocument &, const VersionedDocument &)> &): Join condition evaluated on every (left, right) pair. Return true to include the pair in the result.
- Details: Temporal AS-OF join between two tables. Returns pairs (left_row, right_row) where both rows were current at the given system time and the join predicate returns true. left Left-hand table. right Right-hand table. as_of Point-in-time for both tables. predicate Join condition evaluated on every (left, right) pair. Return true to include the pair in the result.

#### `std::vector< std::pair< VersionedDocument, VersionedDocument > > joinBiTemporal(const BiTemporalTable &left, const BiTemporalTable &right, Timestamp sys_as_of, Timestamp valid_at, const std::function< bool(const VersionedDocument &, const VersionedDocument &)> &predicate)`
- Source: `include/temporal/temporal_query_engine.h`:239
- Brief: n/a
- Parameters:
  - `left` (const BiTemporalTable &): Left-hand bi-temporal table.
  - `right` (const BiTemporalTable &): Right-hand bi-temporal table.
  - `sys_as_of` (Timestamp): System-time point applied to both tables.
  - `valid_at` (Timestamp): Valid-time point applied to both tables.
  - `predicate` (const std::function< bool(const VersionedDocument &, const VersionedDocument &)> &): Join condition evaluated on every (left, right) pair.
- Details: Bi-temporal join between two BiTemporalTables. Returns pairs (left_row, right_row) where: Both rows have sys_time.contains(sys_as_of) Both rows have valid_time.contains(valid_at) predicate(left_row, right_row) returns true This implements the combined transaction-time + valid-time join semantics defined in SQL:2011. left Left-hand bi-temporal table. right Right-hand bi-temporal table. sys_as_of System-time point applied to both tables. valid_at Valid-time point applied to both tables. predicate Join condition evaluated on every (left, right) pair.

#### `bool matchesFilters(const VersionedDocument &doc, const std::vector< RowFilter > &filters)`
- Source: `include/temporal/temporal_query_engine.h`:352
- Brief: Matches Filters.
- Parameters:
  - `doc` (const VersionedDocument &): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: True when the operation succeeds.
- Details: Apply a list of field-level row filters to a document. Returns true only when the document satisfies every filter. Accessible as a public helper so that code outside the class (e.g. detail::queryAsOfCached) can reuse the same filter logic. doc Input parameter. filters Input parameter. True when the operation succeeds. Calls: matches().

#### `std::vector< VersionedDocument > queryApplicationTime(const BiTemporalTable &table, Timestamp valid_at, const std::vector< RowFilter > &filters={})`
- Source: `include/temporal/temporal_query_engine.h`:297
- Brief: ============================================================================ FOR APPLICATION_TIME queries (SQL:2011 §7.
- Parameters:
  - `table` (const BiTemporalTable &): Input parameter.
  - `valid_at` (Timestamp): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: FOR APPLICATION_TIME AS OF valid_at (SQL:2011 §7.6) Returns all rows from a BiTemporalTable that are current in system-time (latest versions, i.e., sys_time end == kMaxTimestamp) and whose valid-time period contains valid_at. This is the application-time counterpart of queryAsOf(). table Source bi-temporal table. valid_at Point-in-application-time to query. filters Optional field-level row filters. table Input parameter. valid_at Input parameter. filters Input parameter. Return value. 6) ============================================================================ Calls: scanBiTemporal(), now(), empty(), reserve(), size(), matchesFilters(), push_back(), std::move().

#### `std::vector< VersionedDocument > queryApplicationTimeRange(const BiTemporalTable &table, Timestamp valid_from, Timestamp valid_to, const std::vector< RowFilter > &filters={})`
- Source: `include/temporal/temporal_query_engine.h`:315
- Brief: Query Application Time Range.
- Parameters:
  - `table` (const BiTemporalTable &): Input parameter.
  - `valid_from` (Timestamp): Input parameter.
  - `valid_to` (Timestamp): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: FOR APPLICATION_TIME FROM valid_from TO valid_to (SQL:2011 §7.6) Returns all rows from a BiTemporalTable whose valid-time period overlaps the half-open interval [valid_from, valid_to). Only rows that are currently active (sys_time end == kMaxTimestamp) are returned. table Source bi-temporal table. valid_from Start of the valid-time range (inclusive). valid_to End of the valid-time range (exclusive). filters Optional field-level row filters. table Input parameter. valid_from Input parameter. valid_to Input parameter. filters Input parameter. Return value. Calls: getAllKeys(), getHistory(), isCurrent(), overlaps(), empty(), matchesFilters(), push_back(), std::move().

#### `std::vector< VersionedDocument > queryAsOf(const SystemVersionedTable &table, Timestamp as_of, const std::vector< RowFilter > &filters={})`
- Source: `include/temporal/temporal_query_engine.h`:166
- Brief: Query As Of.
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `as_of` (Timestamp): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: Query all rows that were current at a specific system time. table Source table as_of Point-in-time (ms since epoch) filters Optional field-level row filters table Input parameter. as_of Input parameter. filters Input parameter. Return value. Calls: scan(), empty(), reserve(), size(), matchesFilters(), push_back(), std::move().

#### `std::vector< VersionedDocument > queryAsOfWithIndex(const SystemVersionedTable &table, const TemporalIndex &index, Timestamp as_of, const std::vector< RowFilter > &filters={})`
- Source: `include/temporal/temporal_query_engine.h`:334
- Brief: ============================================================================ Index-accelerated query (query optimization) ============================================================================
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `index` (const TemporalIndex &): Input parameter.
  - `as_of` (Timestamp): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: Index-accelerated AS-OF query (query optimization). Uses an externally managed TemporalIndex to identify candidate keys before consulting the table, reducing the scan space for large history tables. Falls back to a full scan when the index returns no candidates. table Source table. index Temporal index built over the same table. as_of Point-in-time to query. filters Optional field-level row filters. table Input parameter. index Input parameter. as_of Input parameter. filters Input parameter. Return value. Calls: queryPoint(), empty(), size(), queryAsOf(), reserve(), getHistoryInRange(), contains(), matchesFilters().

#### `std::vector< VersionedDocument > queryBetween(const SystemVersionedTable &table, Timestamp start, Timestamp end, const std::vector< RowFilter > &filters={})`
- Source: `include/temporal/temporal_query_engine.h`:279
- Brief: ============================================================================ FOR SYSTEM_TIME BETWEEN.
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `start` (Timestamp): Input parameter.
  - `end` (Timestamp): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: FOR SYSTEM_TIME BETWEEN start AND end (SQL:2011 §7.6) Returns all row versions whose sys_time overlaps the closed interval [start, end]. This differs from queryFromTo() which uses the half-open interval [from, to). table Source table. start Inclusive range start. end Inclusive range end. filters Optional field-level row filters. table Input parameter. start Input parameter. end Input parameter. filters Input parameter. Return value. ..AND (SQL:2011 §7.6) ============================================================================ Calls: queryFromTo().

#### `std::vector< VersionedDocument > queryFromTo(const SystemVersionedTable &table, Timestamp from, Timestamp to, const std::vector< RowFilter > &filters={})`
- Source: `include/temporal/temporal_query_engine.h`:174
- Brief: Query From To.
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `from` (Timestamp): Input parameter.
  - `to` (Timestamp): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: Query all row versions whose sys_time overlaps [from, to). table Input parameter. from Input parameter. to Input parameter. filters Input parameter. Return value. Calls: getAllKeys(), getHistoryInRange(), empty(), matchesFilters(), push_back(), std::move().

#### `std::vector< VersionedDocument > queryKeyFromTo(const SystemVersionedTable &table, const std::string &key, Timestamp from, Timestamp to)`
- Source: `include/temporal/temporal_query_engine.h`:184
- Brief: Query Key From To.
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `key` (const std::string &): Input parameter.
  - `from` (Timestamp): Input parameter.
  - `to` (Timestamp): Input parameter.
- Return: Return value.
- Details: Query all row versions for a specific key whose sys_time overlaps [from, to). table Input parameter. key Input parameter. from Input parameter. to Input parameter. Return value. Calls: getHistoryInRange().

#### `std::vector< VersionedDocument > queryWithSemantics(const SystemVersionedTable &table, TemporalSemantics semantics, const TimeRange &period, const std::vector< RowFilter > &filters={})`
- Source: `include/temporal/temporal_query_engine.h`:261
- Brief: ============================================================================ Sequenced / Non-Sequenced query semantics ============================================================================
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `semantics` (TemporalSemantics): Input parameter.
  - `period` (const TimeRange &): Input parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: Query a SystemVersionedTable with explicit SEQUENCED or NON-SEQUENCED semantics (SQL:2011 §4.16.5). SEQUENCED: Returns rows whose sys_time overlaps the given period. Temporal predicates are respected per individual period. NON_SEQUENCED: Returns all row versions regardless of sys_time, treating the table as an atemporal relation. table Source table. semantics SEQUENCED or NON_SEQUENCED. period Reference period (used for SEQUENCED filtering only). filters Optional field-level row filters. table Input parameter. semantics Input parameter. period Input parameter. filters Input parameter. Return value. Calls: getAllKeys(), getHistoryInRange(), empty(), matchesFilters(), push_back(), std::move(), queryFromTo().

#### `std::vector< VersionedDocument > sequencedDistinct(const SystemVersionedTable &table, const std::vector< std::string > &compare_fields={})`
- Source: `include/temporal/temporal_query_engine.h`:439
- Brief: SQL:2011 §13.4 SEQUENCED DISTINCT — remove temporally redundant rows.
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `compare_fields` (const std::vector< std::string > &): Input parameter.
- Return: Coalesced rows, sorted by key then sys_start.
- Details: Sequenced Distinct. Returns the minimal set of VersionedDocument rows that captures the complete version history of each logical key, eliminating rows whose non-temporal data is identical to an adjacent version for the same key. Adjacent periods with identical data are merged into a single, longer interval. Definition (SQL:2011 §13.4) A row R is temporally redundant when there exists another row R' for the same key such that: R'.sys_time overlaps or is immediately adjacent to R.sys_time, AND the compared fields of R' are identical to those of R. The SEQUENCED DISTINCT result coalesces all such adjacent equal-data intervals into a single row whose sys_time spans the merged range. Example key="x",data={"v":1},sys_time=[0,10) key="x",data={"v":1},sys_time=[10,20)←samedata,adjacent→merge key="x",data={"v":2},sys_time=[20,30)←differentdata→keepseparate Result: key="x",data={"v":1},sys_time=[0,20) key="x",data={"v":2},sys_time=[20,30) table Source table (all historical versions are scanned). compare_fields JSON field names used for equality comparison. Pass an empty vector to compare the entire data document (all fields must match for merging). Coalesced rows, sorted by key then sys_start. table Input parameter. compare_fields Input parameter. Return value. Calls: getAllKeys(), coalesceVersions(), getHistory(), push_back(), std::move(), std::sort(), begin(), end().

#### `std::vector< VersionedDocument > sequencedDistinctForKey(const SystemVersionedTable &table, const std::string &key, const std::vector< std::string > &compare_fields={})`
- Source: `include/temporal/temporal_query_engine.h`:455
- Brief: SEQUENCED DISTINCT restricted to a single key.
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `key` (const std::string &): Input parameter.
  - `compare_fields` (const std::vector< std::string > &): Input parameter.
- Return: Coalesced rows for key, sorted by sys_start.
- Details: Sequenced Distinct For Key. Same semantics as the table-wide overload, applied only to versions of the given key. Useful when the caller already knows the key and wants to avoid scanning the full table. table Source table. key Key whose versions should be coalesced. compare_fields Fields used for equality comparison (empty = all fields). Coalesced rows for key, sorted by sys_start. table Input parameter. key Input parameter. compare_fields Input parameter. Return value. Calls: coalesceVersions(), getHistory().

### themisdb::temporal::TemporalQuerySpec

#### `TemporalQuerySpec all() noexcept`
- Source: `include/temporal/temporal_query_engine.h`:143
- Brief: Convenience factory — FOR SYSTEM_TIME ALL.
- Parameters: none

#### `TemporalQuerySpec asOf(Timestamp t) noexcept`
- Source: `include/temporal/temporal_query_engine.h`:127
- Brief: Convenience factory — FOR SYSTEM_TIME AS OF <t>.
- Parameters:
  - `t` (Timestamp): n/a

#### `TemporalQuerySpec betweenAnd(Timestamp s, Timestamp e) noexcept`
- Source: `include/temporal/temporal_query_engine.h`:135
- Brief: Convenience factory — FOR SYSTEM_TIME BETWEEN AND <e>.
- Parameters:
  - `s` (Timestamp): n/a
  - `e` (Timestamp): n/a

#### `TemporalQuerySpec containedIn(Timestamp s, Timestamp e) noexcept`
- Source: `include/temporal/temporal_query_engine.h`:139
- Brief: Convenience factory — FOR SYSTEM_TIME CONTAINED IN PERIOD (, <e>).
- Parameters:
  - `s` (Timestamp): n/a
  - `e` (Timestamp): n/a

#### `TemporalQuerySpec fromTo(Timestamp s, Timestamp e) noexcept`
- Source: `include/temporal/temporal_query_engine.h`:131
- Brief: Convenience factory — FOR SYSTEM_TIME FROM TO <e>.
- Parameters:
  - `s` (Timestamp): n/a
  - `e` (Timestamp): n/a

### themisdb::temporal::TemporalSnapshot

#### `std::optional< TemporalSnapshot > fromJson(const nlohmann::json &j)`
- Source: `include/temporal/temporal_conflict_resolver.h`:60
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: at().

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_conflict_resolver.h`:59
- Brief: n/a
- Parameters: none

### themisdb::temporal::TemporalSnapshotManager

#### `TemporalSnapshotManager(ClockFn clock=&now)`
- Source: `include/temporal/snapshot_manager.h`:137
- Brief: n/a
- Parameters:
  - `clock` (ClockFn): Callable that returns the current time in milliseconds since epoch. Defaults to the module-level now(). Primarily used in tests to advance time deterministically.
- Details: Construct a manager with an optional custom clock. clock Callable that returns the current time in milliseconds since epoch. Defaults to the module-level now(). Primarily used in tests to advance time deterministically.

#### `SnapshotHandle createSnapshot(const std::map< std::string, const SystemVersionedTable * > &tables)`
- Source: `include/temporal/snapshot_manager.h`:147
- Brief: n/a
- Parameters:
  - `tables` (const std::map< std::string, const SystemVersionedTable * > &): Map of table_name → reference to the live table. The snapshot captures the tables' state atomically under a common timestamp.
- Return: Handle to the new snapshot.
- Details: Create a snapshot of the given tables at the current time. tables Map of table_name → reference to the live table. The snapshot captures the tables' state atomically under a common timestamp. Handle to the new snapshot.

#### `SnapshotDiff diff(const SnapshotHandle &base, const SnapshotHandle &other) const`
- Source: `include/temporal/snapshot_manager.h`:252
- Brief: n/a
- Parameters:
  - `base` (const SnapshotHandle &): Older snapshot handle.
  - `other` (const SnapshotHandle &): Newer snapshot handle.
- Return: SnapshotDiff; empty() == true when the snapshots are equal.
- Throws:
  - std::invalid_argument: when either handle is invalid or refers to a released snapshot.
- Details: Compute the incremental difference between two snapshots. Both handles must be valid (not released). The function compares each table that appears in both snapshots. Tables that exist in only one snapshot are treated as entirely added or entirely removed. Complexity: O(R log R) where R is the total number of rows across all shared tables (sort + linear scan per table). base Older snapshot handle. other Newer snapshot handle. SnapshotDiff; empty() == true when the snapshots are equal. std::invalid_argument when either handle is invalid or refers to a released snapshot.

#### `size_t garbageCollectByAge(Timestamp max_age_ms)`
- Source: `include/temporal/snapshot_manager.h`:192
- Brief: Garbage Collect By Age.
- Parameters:
  - `max_age_ms` (Timestamp): Input parameter.
- Return: Number of snapshots removed.
- Details: Garbage-collect snapshots whose creation time is older than (clock() - max_age_ms). Snapshots whose age exceeds the threshold are released automatically. max_age_ms Maximum allowed age in milliseconds. Pass 0 to skip TTL-based collection. Number of snapshots removed. max_age_ms Input parameter. Return value. Calls: clock_(), lock(), push_back(), erase(), size().

#### `size_t garbageCollectByCount(size_t max_snapshots)`
- Source: `include/temporal/snapshot_manager.h`:202
- Brief: Garbage Collect By Count.
- Parameters:
  - `max_snapshots` (size_t): Input parameter.
- Return: Number of snapshots removed.
- Details: Garbage-collect snapshots exceeding a maximum count. The oldest snapshots (by version_number) are removed first until at most max_snapshots remain. max_snapshots Maximum number of snapshots to keep. Number of snapshots removed. max_snapshots Input parameter. Return value. Calls: lock(), size(), reserve(), emplace_back(), std::sort(), begin(), end(), erase().

#### `std::string generateSnapshotId()`
- Source: `include/temporal/snapshot_manager.h`:270
- Brief: Generate Snapshot Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), gen(), rd(), lock(), dist(), str().

#### `SnapshotMetadata getSnapshotMetadata(const SnapshotHandle &handle) const`
- Source: `include/temporal/snapshot_manager.h`:181
- Brief: n/a
- Parameters:
  - `handle` (const SnapshotHandle &): A valid snapshot handle.
- Return: Metadata, or a default-constructed (is_valid=false) struct if the snapshot does not exist.
- Details: Return metadata for a live snapshot. handle A valid snapshot handle. Metadata, or a default-constructed (is_valid=false) struct if the snapshot does not exist.

#### `nlohmann::json getStatistics() const`
- Source: `include/temporal/snapshot_manager.h`:204
- Brief: n/a
- Parameters: none

#### `bool isAlive(const SnapshotHandle &handle) const`
- Source: `include/temporal/snapshot_manager.h`:169
- Brief: n/a
- Parameters:
  - `handle` (const SnapshotHandle &): n/a
- Details: Return true if the snapshot handle is still alive.

#### `std::vector< VersionedDocument > querySnapshot(const SnapshotHandle &handle, const std::string &table_name, const std::vector< std::pair< std::string, nlohmann::json > > &filters={}) const`
- Source: `include/temporal/snapshot_manager.h`:159
- Brief: n/a
- Parameters:
  - `handle` (const SnapshotHandle &): Snapshot handle (must be valid and not released).
  - `table_name` (const std::string &): Name of the table to query within the snapshot.
  - `filters` (const std::vector< std::pair< std::string, nlohmann::json > > &): Optional field-level filters applied to rows.
- Return: Matching rows, or empty if the snapshot/table is invalid.
- Details: Query a snapshot for rows that were current at its creation time. handle Snapshot handle (must be valid and not released). table_name Name of the table to query within the snapshot. filters Optional field-level filters applied to rows. Matching rows, or empty if the snapshot/table is invalid.

#### `bool releaseSnapshot(const SnapshotHandle &handle)`
- Source: `include/temporal/snapshot_manager.h`:166
- Brief: Release Snapshot.
- Parameters:
  - `handle` (const SnapshotHandle &): Input parameter.
- Return: True when the operation succeeds.
- Details: Release a snapshot and free its resources. handle Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), erase().

#### `size_t snapshotCount() const`
- Source: `include/temporal/snapshot_manager.h`:172
- Brief: n/a
- Parameters: none
- Details: Number of live snapshots.

### themisdb::temporal::TemporalSnapshotManager::SnapshotDiff

#### `bool empty() const noexcept`
- Source: `include/temporal/snapshot_manager.h`:229
- Brief: n/a
- Parameters: none
- Details: true when base and other are identical across all shared tables.

#### `nlohmann::json toJson() const`
- Source: `include/temporal/snapshot_manager.h`:233
- Brief: n/a
- Parameters: none

### themisdb::temporal::TemporalTierManager

#### `TemporalTierManager(TierPolicy policy={}, std::shared_ptr< TemporalColdStore > cold_store=nullptr)`
- Source: `include/temporal/temporal_tier_manager.h`:319
- Brief: n/a
- Parameters:
  - `policy` (TierPolicy): n/a
  - `cold_store` (std::shared_ptr< TemporalColdStore >): n/a

#### `TemporalTierManager(const TemporalTierManager &)=delete`
- Source: `include/temporal/temporal_tier_manager.h`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalTierManager &): n/a

#### `std::vector< VersionedDocument > allFromBlock(const VersionBlock &block)`
- Source: `include/temporal/temporal_tier_manager.h`:501
- Brief: Collect all versions from a VersionBlock.
- Parameters:
  - `block` (const VersionBlock &): n/a

#### `size_t compactTable(const std::string &table_name)`
- Source: `include/temporal/temporal_tier_manager.h`:400
- Brief: Compact all keys in table_name according to current policy.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
- Return: Total versions moved across all tiers.
- Details: Compact Table. Total versions moved across all tiers. table_name Name of the table. Return value. Calls: lk(), find(), end(), push_back(), std::find(), begin(), makeContext(), evaluate().

#### `void compactionLoop()`
- Source: `include/temporal/temporal_tier_manager.h`:472
- Brief: Compaction Loop.
- Parameters: none
- Details: Calls: lk(), wait_for(), load(), push_back(), std::find(), begin(), end(), compactTable().

#### `size_t flushHotToWarm(const std::string &table_name, const std::string &doc_key)`
- Source: `include/temporal/temporal_tier_manager.h`:383
- Brief: Flush oldest hot-tier versions to a new warm VersionBlock.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `doc_key` (const std::string &): Input parameter.
- Return: Number of versions moved.
- Details: Flush Hot To Warm. Moves the oldest (hot_count - policy.hot_max_versions_per_key) closed versions from the hot map into a new VersionBlock appended to the warm tier. Number of versions moved. table_name Name of the table. doc_key Input parameter. Return value. Calls: lk(), flushHotToWarmLocked().

#### `size_t flushHotToWarmLocked(const std::string &table_name, const std::string &doc_key, HotMap &hot_map, WarmBlocks &warm_blocks)`
- Source: `include/temporal/temporal_tier_manager.h`:481
- Brief: Flush hot → warm for key. Exclusive lock must be held.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a
  - `hot_map` (HotMap &): n/a
  - `warm_blocks` (WarmBlocks &): n/a

#### `size_t flushWarmToCold(const std::string &table_name, const std::string &doc_key)`
- Source: `include/temporal/temporal_tier_manager.h`:393
- Brief: Flush the oldest warm VersionBlock(s) to cold.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `doc_key` (const std::string &): Input parameter.
- Return: Number of versions moved.
- Details: Flush Warm To Cold. Moves versions from the oldest warm block(s) into TemporalColdStore. Number of versions moved. table_name Name of the table. doc_key Input parameter. Return value. Calls: lk(), flushWarmToColdLocked().

#### `size_t flushWarmToColdLocked(const std::string &table_name, const std::string &doc_key, WarmBlocks &warm_blocks)`
- Source: `include/temporal/temporal_tier_manager.h`:487
- Brief: Flush oldest warm block → cold for key. Exclusive lock must be held.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a
  - `warm_blocks` (WarmBlocks &): n/a

#### `std::optional< VersionedDocument > getAsOf(const std::string &table_name, const std::string &doc_key, Timestamp as_of) const`
- Source: `include/temporal/temporal_tier_manager.h`:351
- Brief: Return the version valid at timestamp as_of.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a
  - `as_of` (Timestamp): n/a
- Details: Queries hot → warm → cold in order, returning the first match.

#### `std::vector< VersionedDocument > getHistory(const std::string &table_name, const std::string &doc_key) const`
- Source: `include/temporal/temporal_tier_manager.h`:360
- Brief: Return all stored historical versions, sorted by sys_start.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a
- Details: Merges hot + warm + cold tiers.

#### `std::vector< VersionedDocument > getHistoryInRange(const std::string &table_name, const std::string &doc_key, const TimeRange &range) const`
- Source: `include/temporal/temporal_tier_manager.h`:367
- Brief: Return versions whose sys_time overlaps range, sorted by sys_start.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a
  - `range` (const TimeRange &): n/a

#### `bool insert(const std::string &table_name, const VersionedDocument &doc)`
- Source: `include/temporal/temporal_tier_manager.h`:342
- Brief: Insert a VersionedDocument into the hot tier.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `doc` (const VersionedDocument &): Input parameter.
- Return: false if doc.isCurrent() is true (current versions belong to the live table, not the history tiers).
- Details: Insert. The version is inserted into the sorted hot-tier map for its key. After insertion the tier-decision is evaluated; if FLUSH_HOT_TO_WARM or FLUSH_WARM_TO_COLD is returned the corresponding operation is performed synchronously before returning. false if doc.isCurrent() is true (current versions belong to the live table, not the history tiers). table_name Name of the table. doc Input parameter. True when the operation succeeds. Calls: isCurrent(), lk(), makeContext(), evaluate(), flushWarmToColdLocked(), size(), flushHotToWarmLocked().

#### `KeyTierStats keyStats(const std::string &table_name, const std::string &doc_key) const`
- Source: `include/temporal/temporal_tier_manager.h`:426
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a

#### `VersionBlock makeBlock(const std::string &doc_key, std::vector< VersionedDocument > versions)`
- Source: `include/temporal/temporal_tier_manager.h`:492
- Brief: Build an immutable VersionBlock from a sorted vector of documents.
- Parameters:
  - `doc_key` (const std::string &): Input parameter.
  - `versions` (std::vector< VersionedDocument >): Input parameter.
- Return: Return value.
- Details: static doc_key Input parameter. versions Input parameter. Return value. Calls: std::sort(), begin(), end(), size(), BloomFilter(), reserve(), std::min(), std::max().

#### `TierDecisionContext makeContext(const std::string &table_name, const std::string &doc_key) const`
- Source: `include/temporal/temporal_tier_manager.h`:477
- Brief: Build a TierDecisionContext for (table, key). Lock must be held.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `doc_key` (const std::string &): n/a

#### `TemporalTierManager & operator=(const TemporalTierManager &)=delete`
- Source: `include/temporal/temporal_tier_manager.h`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TemporalTierManager &): n/a

#### `const TierPolicy & policy() const noexcept`
- Source: `include/temporal/temporal_tier_manager.h`:405
- Brief: n/a
- Parameters: none

#### `std::vector< VersionedDocument > rangeFromBlock(const VersionBlock &block, const TimeRange &range)`
- Source: `include/temporal/temporal_tier_manager.h`:505
- Brief: Collect overlapping versions from a VersionBlock.
- Parameters:
  - `block` (const VersionBlock &): n/a
  - `range` (const TimeRange &): n/a

#### `std::optional< VersionedDocument > searchBlock(const VersionBlock &block, Timestamp as_of)`
- Source: `include/temporal/temporal_tier_manager.h`:497
- Brief: Search a single VersionBlock for the version containing as_of.
- Parameters:
  - `block` (const VersionBlock &): n/a
  - `as_of` (Timestamp): n/a

#### `void setPolicy(const TierPolicy &policy)`
- Source: `include/temporal/temporal_tier_manager.h`:404
- Brief: Set Policy.
- Parameters:
  - `policy` (const TierPolicy &): Input parameter.
- Details: policy Input parameter. Calls: lk().

#### `void startCompactionWorker()`
- Source: `include/temporal/temporal_tier_manager.h`:410
- Brief: Start the periodic background compaction thread (idempotent).
- Parameters: none
- Details: Start Compaction Worker. Calls: joinable(), std::thread(), compactionLoop().

#### `nlohmann::json statsJson(const std::string &table_name) const`
- Source: `include/temporal/temporal_tier_manager.h`:442
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a

#### `void stopCompactionWorker()`
- Source: `include/temporal/temporal_tier_manager.h`:413
- Brief: Stop the background compaction thread and wait for it to exit.
- Parameters: none
- Details: Stop Compaction Worker. Calls: notify_all(), joinable(), join().

#### `TableTierStats tableStats(const std::string &table_name) const`
- Source: `include/temporal/temporal_tier_manager.h`:440
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a

#### `~TemporalTierManager()`
- Source: `include/temporal/temporal_tier_manager.h`:323
- Brief: n/a
- Parameters: none

### themisdb::temporal::TierPolicy

#### `TierDecision evaluate(const TierDecisionContext &ctx) const`
- Source: `include/temporal/temporal_tier_manager.h`:272
- Brief: Built-in threshold-based evaluation. Called by evaluate() when decision_fn is nullptr.
- Parameters:
  - `ctx` (const TierDecisionContext &): n/a

### themisdb::temporal::TimeRange

#### `bool contains(Timestamp t) const noexcept`
- Source: `include/temporal/temporal_types.h`:57
- Brief: n/a
- Parameters:
  - `t` (Timestamp): n/a

#### `TimeRange fromJson(const nlohmann::json &j)`
- Source: `include/temporal/temporal_types.h`:89
- Brief: n/a
- Parameters:
  - `j` (const nlohmann::json &): n/a

#### `bool meets(const TimeRange &other) const noexcept`
- Source: `include/temporal/temporal_types.h`:73
- Brief: n/a
- Parameters:
  - `other` (const TimeRange &): n/a

#### `bool operator!=(const TimeRange &other) const noexcept`
- Source: `include/temporal/temporal_types.h`:81
- Brief: n/a
- Parameters:
  - `other` (const TimeRange &): n/a

#### `bool operator==(const TimeRange &other) const noexcept`
- Source: `include/temporal/temporal_types.h`:77
- Brief: n/a
- Parameters:
  - `other` (const TimeRange &): n/a

#### `bool overlaps(const TimeRange &other) const noexcept`
- Source: `include/temporal/temporal_types.h`:61
- Brief: n/a
- Parameters:
  - `other` (const TimeRange &): n/a

#### `bool precedes(const TimeRange &other) const noexcept`
- Source: `include/temporal/temporal_types.h`:65
- Brief: n/a
- Parameters:
  - `other` (const TimeRange &): n/a

#### `bool succeeds(const TimeRange &other) const noexcept`
- Source: `include/temporal/temporal_types.h`:69
- Brief: n/a
- Parameters:
  - `other` (const TimeRange &): n/a

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_types.h`:85
- Brief: n/a
- Parameters: none

### themisdb::temporal::TrendResult

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_aggregator.h`:109
- Brief: n/a
- Parameters: none

### themisdb::temporal::UnionMergeResolver

#### `TemporalSnapshot merge(const TemporalSnapshot &local, const TemporalSnapshot &remote) const override`
- Source: `include/temporal/temporal_conflict_resolver.h`:174
- Brief: Merge two conflicting snapshots into a single resolved snapshot.
- Parameters:
  - `local` (const TemporalSnapshot &): The locally-held snapshot version.
  - `remote` (const TemporalSnapshot &): The remotely-received snapshot version.
- Return: A new TemporalSnapshot whose data represents the merged state. The metadata fields (snapshot_id, hlc, source_node_id, checksum) are set by the implementation — typically to those of the "dominant" input.
- Details: local The locally-held snapshot version. remote The remotely-received snapshot version. A new TemporalSnapshot whose data represents the merged state. The metadata fields (snapshot_id, hlc, source_node_id, checksum) are set by the implementation — typically to those of the "dominant" input.

### themisdb::temporal::ValidationResult

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_migrator.h`:182
- Brief: n/a
- Parameters: none

### themisdb::temporal::VersionBlock

#### `VersionBlock()`
- Source: `include/temporal/temporal_tier_manager.h`:156
- Brief: n/a
- Parameters: none

### themisdb::temporal::VersionedDocument

#### `bool isCurrent() const noexcept`
- Source: `include/temporal/temporal_types.h`:109
- Brief: n/a
- Parameters: none

#### `nlohmann::json toJson() const`
- Source: `include/temporal/temporal_types.h`:113
- Brief: n/a
- Parameters: none

### themisdb::temporal::detail

#### `std::vector< VersionedDocument > queryAsOfCached(const SystemVersionedTable &table, Timestamp as_of, QueryCache &cache, const std::vector< RowFilter > &filters)`
- Source: `src/temporal/temporal_query_engine.cpp`:779
- Brief: Query As Of Cached.
- Parameters:
  - `table` (const SystemVersionedTable &): Input parameter.
  - `as_of` (Timestamp): Input parameter.
  - `cache` (QueryCache &): Input/output parameter.
  - `filters` (const std::vector< RowFilter > &): Input parameter.
- Return: Return value.
- Details: Cached AS-OF query. Returns the cached result if available; otherwise executes TemporalQueryEngine::queryAsOf(), caches the result, and returns it. The cache is keyed on (table.tableName(), as_of); field-level filters are applied after cache lookup so that the cache stores unfiltered result sets and multiple filter combinations can reuse the same entry. table Source table. as_of Point-in-time (ms since epoch). cache Shared QueryCache instance. filters Optional field-level row filters (applied post-cache). table Input parameter. as_of Input parameter. cache Input/output parameter. filters Input parameter. Return value. Calls: get(), tableName(), has_value(), std::move(), value(), TemporalQueryEngine::queryAsOf(), put(), empty().

