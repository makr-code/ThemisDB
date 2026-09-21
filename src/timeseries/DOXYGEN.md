# TIMESERIES DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\timeseries\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\timeseries\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 63
- Compounds: 204
- Classes/Structs: 118
- Namespaces: 14
- File Compounds: 63

## Namespaces
- benchmark
- rocksdb
- std::chrono_literals
- stubs
- testing
- themis
- themis::@174106252375163056365130103023372316210005175124
- themis::@352244034060275310022233040251213012023122146230
- themis::bench
- themis::bench::tsrg
- themis::timeseries
- themis::timeseries::proto
- themis::timeseries::test
- themis::utils

## Types
### Classes
- AdaptiveFlushFixture
- IncidentTaxonomyTest
- TimeSeriesMetricsTest
- TimeseriesBenchmarkFixture
- themis::AdaptiveFlushController
- themis::AggregateScheduler
- themis::AnomalyDetector
- themis::BackwardFillGapFiller
- themis::BitReader
- themis::BitWriter
- themis::ContinuousAggMaterializationEngine
- themis::ContinuousAggWatermarkStore
- themis::ContinuousAggregateManager
- themis::DistributedAggregateCoordinator
- themis::DownsamplingPipeline
- themis::EncryptedChunkStore
- themis::FlushController
- themis::ForwardFillGapFiller
- themis::GapFiller
- themis::GorillaDecoder
- themis::GorillaEncoder
- themis::GorillaSIMDDecoder
- themis::HeuristicCompressionSelector
- themis::Hypertable
- themis::IAnomalyDetector
- themis::ICompressionSelector
- themis::IGapFiller
- themis::IQRDetector
- themis::LinearInterpolationGapFiller
- themis::PerSeriesCompressionRegistry
- themis::RetentionManager
- themis::TSAutoBuffer
- themis::TSQueryOptimizer
- themis::TSStore
- themis::TierSelector
- themis::TimeSeriesAggregates
- themis::TimeSeriesMetrics
- themis::TimeSeriesStore
- themis::TsEncryptedKeyRotation
- themis::ZScoreDetector
- themis::timeseries::TsEdgeCaseHandler
- themis::timeseries::TsOperatorDiagnostics
- themis::timeseries::TsStreamCursor

### Structs
- MaterializationFixture
- stubs::DownsamplingEngine
- stubs::RemoteWriteClient
- stubs::TSFlush
- stubs::TSIngest
- stubs::TSIngestStore
- stubs::TSQuery
- stubs::TSRemoteWrite
- themis::AdaptiveFlushControllerConfig
- themis::AdaptiveFlushControllerStats
- themis::AggConfig
- themis::AggShardResult
- themis::AggWindow
- themis::AggregateScheduler::Config
- themis::AggregateScheduler::ScheduledAggregate
- themis::AggregateScheduler::Stats
- themis::AnomalyConfig
- themis::AnomalyPoint
- themis::ContinuousAggDefinition
- themis::ContinuousAggMaterializationStatus
- themis::DownsamplingPolicy
- themis::DownsamplingTier
- themis::EncryptedChunkStore::EncryptResult
- themis::FlushControllerConfig
- themis::FlushControllerStats
- themis::GapFillConfig
- themis::HeuristicCompressionSelector::Config
- themis::Hypertable::ChunkHealth
- themis::Hypertable::ChunkInfo
- themis::Hypertable::Config
- themis::Hypertable::Stats
- themis::RetentionAuditEntry
- themis::RetentionPolicy
- themis::RetentionStats
- themis::RollupHierarchy
- themis::SeriesProfile
- themis::StagedDeletionPolicy
- themis::TSAutoBuffer::FlushController
- themis::TSAutoBuffer::MetricBuffer
- themis::TSAutoBufferConfig
- themis::TSAutoBufferStats
- themis::TSQueryOptimizer::IndexHint
- themis::TSQueryOptimizer::OptimizationHint
- themis::TSQueryOptimizer::PredicateFilter
- themis::TSQueryOptimizer::QueryPlan
- themis::TSStore::AggregationResult
- themis::TSStore::BatchWriteResult
- themis::TSStore::Config
- themis::TSStore::DataPoint
- themis::TSStore::KeyComponents
- themis::TSStore::OutOfOrderStats
- themis::TSStore::QueryOptions
- themis::TSStore::Stats
- themis::TSStore::TSRow
- themis::TimeSeriesAggregates::AggregateResult
- themis::TimeSeriesAggregates::Config
- themis::TimeSeriesAggregates::TimeWindow
- themis::TimeSeriesMetrics::AggRefreshStats
- themis::TimeSeriesMetrics::Config
- themis::TimeSeriesMetrics::PerMetricStats
- themis::TimeSeriesStore::Aggregation
- themis::TimeSeriesStore::DataPoint
- themis::TimeSeriesStore::RangeQuery
- themis::TsEncryptedKeyRotationConfig
- themis::bench::tsrg::TimePoint
- themis::timeseries::Incident
- themis::timeseries::IncidentContext
- themis::timeseries::IncidentTimestamp
- themis::timeseries::PromLabel
- themis::timeseries::PromSample
- themis::timeseries::PromTimeSeries
- themis::timeseries::PromWriteRequest
- themis::timeseries::TsIncident
- themis::timeseries::TsStreamCursor::Config
- themis::timeseries::test::TimePoint

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 685

### AdaptiveFlushFixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:45
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:97
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `TSStore::DataPoint makePoint(const std::string &metric, const std::string &entity, int64_t ts_ms, double val)`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:118
- Brief: n/a
- Parameters:
  - `metric` (const std::string &): n/a
  - `entity` (const std::string &): n/a
  - `ts_ms` (int64_t): n/a
  - `val` (double): n/a

### IncidentTaxonomyTest

#### `void SetUp() override`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:56
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:63
- Brief: n/a
- Parameters: none

#### `Incident lastIncident() const`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:69
- Brief: n/a
- Parameters: none

#### `void staticHandler(const Incident &incident) noexcept`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:39
- Brief: n/a
- Parameters:
  - `incident` (const Incident &): n/a

### MaterializationFixture

#### `void SetUp() override`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:37
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:47
- Brief: n/a
- Parameters: none

#### `void insertPoints(const std::string &metric, const std::string &entity, int n, double start_val=1.0, double step_val=1.0, int64_t step_ms=10000LL)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:54
- Brief: n/a
- Parameters:
  - `metric` (const std::string &): n/a
  - `entity` (const std::string &): n/a
  - `n` (int): n/a
  - `start_val` (double): n/a
  - `step_val` (double): n/a
  - `step_ms` (int64_t): n/a

#### `AggConfig makeConfig(const std::string &metric, const std::string &entity, std::chrono::milliseconds window)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:68
- Brief: n/a
- Parameters:
  - `metric` (const std::string &): n/a
  - `entity` (const std::string &): n/a
  - `window` (std::chrono::milliseconds): n/a

### TimeSeriesMetricsTest

#### `void SetUp() override`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:10
- Brief: n/a
- Parameters: none

### TimeseriesBenchmarkFixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:21
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `TimeSeriesStore * tsStore()`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:61
- Brief: n/a
- Parameters: none

### bench_timeseries_adaptive_flush.cpp

#### `Arg(100) -> Arg(500) ->Arg(1000) ->Arg(5000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `BENCHMARK_DEFINE_F(AdaptiveFlushFixture, BatchWatermark)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveFlushFixture): n/a
  - `<unnamed>` (BatchWatermark): n/a

#### `BENCHMARK_DEFINE_F(AdaptiveFlushFixture, MultiThreaded)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveFlushFixture): n/a
  - `<unnamed>` (MultiThreaded): n/a

#### `BENCHMARK_DEFINE_F(AdaptiveFlushFixture, P99Latency)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveFlushFixture): n/a
  - `<unnamed>` (P99Latency): n/a

#### `BENCHMARK_DEFINE_F(AdaptiveFlushFixture, SingleThreaded)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveFlushFixture): n/a
  - `<unnamed>` (SingleThreaded): n/a

#### `BENCHMARK_DEFINE_F(AdaptiveFlushFixture, StatsExposure)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:396
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveFlushFixture): n/a
  - `<unnamed>` (StatsExposure): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:440
- Brief: n/a
- Parameters: none

#### `void BM_FlushController_Standalone(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:353
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Threads(1) -> MinTime(2.0) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Threads(2) -> Threads(4) ->Threads(8) ->MinTime(2.0) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (2): n/a

### bench_timeseries_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:146
- Brief: n/a
- Parameters: none

#### `void BM_TS_BM_01_IngestThroughput(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:71
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TS_BM_02_RangeQueryP95(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:94
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TS_BM_03_FlushLatency(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:114
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TS_BM_04_RemoteWriteThroughput(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:132
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Threads(1) -> Threads(8) ->UseRealTime()`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

### bench_timeseries_ingestion.cpp

#### `Arg(10) -> Arg(100) ->Arg(1000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Arg(100) -> Arg(1000) ->Arg(10000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Arg(60) -> Arg(300) ->Arg(3600) ->Arg(86400) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:352
- Brief: n/a
- Parameters:
  - `<unnamed>` (60): n/a

#### `BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, BatchIngestion)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesBenchmarkFixture): n/a
  - `<unnamed>` (BatchIngestion): n/a

#### `BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, Downsampling)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:362
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesBenchmarkFixture): n/a
  - `<unnamed>` (Downsampling): n/a

#### `BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, MultipleMetrics)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesBenchmarkFixture): n/a
  - `<unnamed>` (MultipleMetrics): n/a

#### `BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, OutOfOrderWrites)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:409
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesBenchmarkFixture): n/a
  - `<unnamed>` (OutOfOrderWrites): n/a

#### `BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, RawDataIngestion)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesBenchmarkFixture): n/a
  - `<unnamed>` (RawDataIngestion): n/a

#### `BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, TimeRangeQuery)(benchmark`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesBenchmarkFixture): n/a
  - `<unnamed>` (TimeRangeQuery): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:550
- Brief: n/a
- Parameters: none

#### `void BM_DownsamplingThroughput(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:450
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaCompression(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:207
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_GorillaDecompression(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:264
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Threads(1) -> Threads(2) ->Threads(4) ->Threads(8) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Unit(benchmark::kMillisecond) -> Iterations(50) ->UseRealTime()`
- Source: `benchmarks/timeseries/bench_timeseries_ingestion.cpp`:546
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

### bench_timeseries_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:342
- Brief: n/a
- Parameters: none

### stubs::DownsamplingEngine

#### `bool downsample(uint64_t series_id, int factor)`
- Source: `tests/timeseries/test_timeseries_highcardinality_stress.cpp`:36
- Brief: n/a
- Parameters:
  - `series_id` (uint64_t): n/a
  - `factor` (int): n/a

### stubs::RemoteWriteClient

#### `bool write(uint64_t batch_id)`
- Source: `tests/timeseries/test_timeseries_highcardinality_stress.cpp`:49
- Brief: n/a
- Parameters:
  - `batch_id` (uint64_t): n/a

### stubs::TSFlush

#### `bool flush()`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:43
- Brief: n/a
- Parameters: none

### stubs::TSIngest

#### `bool ingest(uint64_t series, double value, int64_t ts_ns)`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:25
- Brief: n/a
- Parameters:
  - `series` (uint64_t): n/a
  - `value` (double): n/a
  - `ts_ns` (int64_t): n/a

### stubs::TSIngestStore

#### `bool ingest(uint64_t series_id, double value, int64_t ts_ns)`
- Source: `tests/timeseries/test_timeseries_highcardinality_stress.cpp`:25
- Brief: n/a
- Parameters:
  - `series_id` (uint64_t): n/a
  - `value` (double): n/a
  - `ts_ns` (int64_t): n/a

### stubs::TSQuery

#### `bool range_query(uint64_t series, int64_t start, int64_t end)`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:34
- Brief: n/a
- Parameters:
  - `series` (uint64_t): n/a
  - `start` (int64_t): n/a
  - `end` (int64_t): n/a

### stubs::TSRemoteWrite

#### `bool write(uint64_t batch_id, int batch_size)`
- Source: `benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp`:51
- Brief: n/a
- Parameters:
  - `batch_id` (uint64_t): n/a
  - `batch_size` (int): n/a

### test_continuous_agg_materialization.cpp

#### `TEST_F(MaterializationFixture, AggId_ContainsNameAndDerivedMetric)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:430
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (AggId_ContainsNameAndDerivedMetric): n/a

#### `TEST_F(MaterializationFixture, CreateAggregate_DuplicateName_ReturnsFalse)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (CreateAggregate_DuplicateName_ReturnsFalse): n/a

#### `TEST_F(MaterializationFixture, CreateAggregate_Success_ReturnsTrueAndListed)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (CreateAggregate_Success_ReturnsTrueAndListed): n/a

#### `TEST_F(MaterializationFixture, DropAggregate_ExistingName_ReturnsTrueAndRemoved)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (DropAggregate_ExistingName_ReturnsTrueAndRemoved): n/a

#### `TEST_F(MaterializationFixture, DropAggregate_ThenRecreate_Succeeds)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (DropAggregate_ThenRecreate_Succeeds): n/a

#### `TEST_F(MaterializationFixture, DropAggregate_UnknownName_ReturnsFalse)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (DropAggregate_UnknownName_ReturnsFalse): n/a

#### `TEST_F(MaterializationFixture, GetAggregateStatus_AfterRefresh_WatermarkUpdated)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:388
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (GetAggregateStatus_AfterRefresh_WatermarkUpdated): n/a

#### `TEST_F(MaterializationFixture, GetAggregateStatus_BeforeRefresh_WatermarkZero)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (GetAggregateStatus_BeforeRefresh_WatermarkZero): n/a

#### `TEST_F(MaterializationFixture, GetAggregateStatus_UnknownName_ReturnsNullopt)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (GetAggregateStatus_UnknownName_ReturnsNullopt): n/a

#### `TEST_F(MaterializationFixture, GetAggregate_ExistingName_ReturnsDefinition)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (GetAggregate_ExistingName_ReturnsDefinition): n/a

#### `TEST_F(MaterializationFixture, GetAggregate_UnknownName_ReturnsNullopt)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (GetAggregate_UnknownName_ReturnsNullopt): n/a

#### `TEST_F(MaterializationFixture, GetAllStatus_EmptyRegistry_ReturnsEmptyVector)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (GetAllStatus_EmptyRegistry_ReturnsEmptyVector): n/a

#### `TEST_F(MaterializationFixture, GetAllStatus_ReturnsStatusForEveryAggregate)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (GetAllStatus_ReturnsStatusForEveryAggregate): n/a

#### `TEST_F(MaterializationFixture, InactiveAggregate_SkippedByRefreshAll)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:452
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (InactiveAggregate_SkippedByRefreshAll): n/a

#### `TEST_F(MaterializationFixture, IncrementalRefresh_SecondPassOnlyProcessesNewData)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:485
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (IncrementalRefresh_SecondPassOnlyProcessesNewData): n/a

#### `TEST_F(MaterializationFixture, ListAggregates_PreservesInsertionOrder)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (ListAggregates_PreservesInsertionOrder): n/a

#### `TEST_F(MaterializationFixture, QueryMaterialized_AggregateValuesAreSane)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (QueryMaterialized_AggregateValuesAreSane): n/a

#### `TEST_F(MaterializationFixture, QueryMaterialized_BeforeRefresh_ReturnsEmpty)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (QueryMaterialized_BeforeRefresh_ReturnsEmpty): n/a

#### `TEST_F(MaterializationFixture, QueryMaterialized_OutsideRefreshedRange_ReturnsEmpty)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (QueryMaterialized_OutsideRefreshedRange_ReturnsEmpty): n/a

#### `TEST_F(MaterializationFixture, QueryMaterialized_UnknownAggregate_ReturnsEmpty)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:318
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (QueryMaterialized_UnknownAggregate_ReturnsEmpty): n/a

#### `TEST_F(MaterializationFixture, RefreshAggregate_AlreadyUpToDate_ReturnsZero)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (RefreshAggregate_AlreadyUpToDate_ReturnsZero): n/a

#### `TEST_F(MaterializationFixture, RefreshAggregate_InactiveAggregate_Skipped)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (RefreshAggregate_InactiveAggregate_Skipped): n/a

#### `TEST_F(MaterializationFixture, RefreshAggregate_ProducesDataPoints)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (RefreshAggregate_ProducesDataPoints): n/a

#### `TEST_F(MaterializationFixture, RefreshAggregate_UnknownAggregate_ReturnsZero)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (RefreshAggregate_UnknownAggregate_ReturnsZero): n/a

#### `TEST_F(MaterializationFixture, RefreshAggregate_WatermarkAdvances)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (RefreshAggregate_WatermarkAdvances): n/a

#### `TEST_F(MaterializationFixture, RefreshAll_EmptyRegistry_ReturnsZero)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (RefreshAll_EmptyRegistry_ReturnsZero): n/a

#### `TEST_F(MaterializationFixture, RefreshAll_MultipleAggregates_RefreshesBoth)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (RefreshAll_MultipleAggregates_RefreshesBoth): n/a

#### `TEST_F(MaterializationFixture, RefreshAll_SkipsAutoRefreshFalse)`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaterializationFixture): n/a
  - `<unnamed>` (RefreshAll_SkipsAutoRefreshFalse): n/a

#### `std::string makeMaterialTempPath()`
- Source: `tests/timeseries/test_continuous_agg_materialization.cpp`:24
- Brief: n/a
- Parameters: none

### test_timeseries_error_path_phase3.cpp

#### `TEST_F(IncidentTaxonomyTest, ConcurrentIncidentEmission)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:451
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (ConcurrentIncidentEmission): n/a

#### `TEST_F(IncidentTaxonomyTest, DISABLED_IncidentCreationLatencyBounded)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:521
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (DISABLED_IncidentCreationLatencyBounded): n/a

#### `TEST_F(IncidentTaxonomyTest, DISABLED_IncidentEmissionLatencyBounded)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (DISABLED_IncidentEmissionLatencyBounded): n/a

#### `TEST_F(IncidentTaxonomyTest, HandlerDeregistration)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (HandlerDeregistration): n/a

#### `TEST_F(IncidentTaxonomyTest, HandlerRegistrationAndInvocation)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:431
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (HandlerRegistrationAndInvocation): n/a

#### `TEST_F(IncidentTaxonomyTest, IngestIncidentBufferOverflow)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IngestIncidentBufferOverflow): n/a

#### `TEST_F(IncidentTaxonomyTest, IngestIncidentBufferPressure)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IngestIncidentBufferPressure): n/a

#### `TEST_F(IncidentTaxonomyTest, IngestIncidentFlushTimeout)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IngestIncidentFlushTimeout): n/a

#### `TEST_F(IncidentTaxonomyTest, IngestIncidentInternalError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IngestIncidentInternalError): n/a

#### `TEST_F(IncidentTaxonomyTest, IngestIncidentSeriesCapacityExceeded)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IngestIncidentSeriesCapacityExceeded): n/a

#### `TEST_F(IncidentTaxonomyTest, IngestIncidentSeriesQuotaExceeded)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IngestIncidentSeriesQuotaExceeded): n/a

#### `TEST_F(IncidentTaxonomyTest, IngestIncidentTimestampInvalid)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IngestIncidentTimestampInvalid): n/a

#### `TEST_F(IncidentTaxonomyTest, IngestIncidentTimestampOutOfOrder)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IngestIncidentTimestampOutOfOrder): n/a

#### `TEST_F(IncidentTaxonomyTest, IntegrationIncidentCriticalRemoteWriteFailure)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IntegrationIncidentCriticalRemoteWriteFailure): n/a

#### `TEST_F(IncidentTaxonomyTest, IntegrationIncidentMetricsExportFailed)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:374
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IntegrationIncidentMetricsExportFailed): n/a

#### `TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteClientError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IntegrationIncidentRemoteWriteClientError): n/a

#### `TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteNetworkError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:345
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IntegrationIncidentRemoteWriteNetworkError): n/a

#### `TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteRetriesExhausted)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:364
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IntegrationIncidentRemoteWriteRetriesExhausted): n/a

#### `TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteServerError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IntegrationIncidentRemoteWriteServerError): n/a

#### `TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteValidationError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IntegrationIncidentRemoteWriteValidationError): n/a

#### `TEST_F(IncidentTaxonomyTest, IsBackpressureErrorClassification)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:397
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IsBackpressureErrorClassification): n/a

#### `TEST_F(IncidentTaxonomyTest, IsHardIngestErrorClassification)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IsHardIngestErrorClassification): n/a

#### `TEST_F(IncidentTaxonomyTest, IsHardQueryErrorClassification)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:409
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IsHardQueryErrorClassification): n/a

#### `TEST_F(IncidentTaxonomyTest, IsPermanentIntegrationError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:421
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IsPermanentIntegrationError): n/a

#### `TEST_F(IncidentTaxonomyTest, IsRetryableIntegrationError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:415
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (IsRetryableIntegrationError): n/a

#### `TEST_F(IncidentTaxonomyTest, LifecycleIncidentDeletionFailed)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (LifecycleIncidentDeletionFailed): n/a

#### `TEST_F(IncidentTaxonomyTest, LifecycleIncidentEncryptionKeyNotFound)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (LifecycleIncidentEncryptionKeyNotFound): n/a

#### `TEST_F(IncidentTaxonomyTest, LifecycleIncidentEncryptionRotationFailure)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (LifecycleIncidentEncryptionRotationFailure): n/a

#### `TEST_F(IncidentTaxonomyTest, LifecycleIncidentEncryptionStateInvalid)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (LifecycleIncidentEncryptionStateInvalid): n/a

#### `TEST_F(IncidentTaxonomyTest, LifecycleIncidentGCFailed)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (LifecycleIncidentGCFailed): n/a

#### `TEST_F(IncidentTaxonomyTest, LifecycleIncidentInternalError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (LifecycleIncidentInternalError): n/a

#### `TEST_F(IncidentTaxonomyTest, LifecycleIncidentRetentionExpired)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (LifecycleIncidentRetentionExpired): n/a

#### `TEST_F(IncidentTaxonomyTest, LifecycleIncidentRetentionPolicyViolation)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (LifecycleIncidentRetentionPolicyViolation): n/a

#### `TEST_F(IncidentTaxonomyTest, QueryIncidentConsistencyCheckFailed)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (QueryIncidentConsistencyCheckFailed): n/a

#### `TEST_F(IncidentTaxonomyTest, QueryIncidentDownsamplingInvalid)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (QueryIncidentDownsamplingInvalid): n/a

#### `TEST_F(IncidentTaxonomyTest, QueryIncidentInternalError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (QueryIncidentInternalError): n/a

#### `TEST_F(IncidentTaxonomyTest, QueryIncidentRangeInvalid)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (QueryIncidentRangeInvalid): n/a

#### `TEST_F(IncidentTaxonomyTest, QueryIncidentRetentionBoundaryCrossed)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (QueryIncidentRetentionBoundaryCrossed): n/a

#### `TEST_F(IncidentTaxonomyTest, QueryIncidentSeriesNotFound)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (QueryIncidentSeriesNotFound): n/a

#### `TEST_F(IncidentTaxonomyTest, QueryIncidentTimeout)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (QueryIncidentTimeout): n/a

#### `TEST_F(IncidentTaxonomyTest, SeverityLevelCritical)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:477
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (SeverityLevelCritical): n/a

#### `TEST_F(IncidentTaxonomyTest, SeverityLevelError)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:482
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (SeverityLevelError): n/a

#### `TEST_F(IncidentTaxonomyTest, SeverityLevelInfo)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:492
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (SeverityLevelInfo): n/a

#### `TEST_F(IncidentTaxonomyTest, SeverityLevelWarn)`
- Source: `tests/timeseries/test_timeseries_error_path_phase3.cpp`:487
- Brief: n/a
- Parameters:
  - `<unnamed>` (IncidentTaxonomyTest): n/a
  - `<unnamed>` (SeverityLevelWarn): n/a

### test_timeseries_highcardinality_stress.cpp

#### `TEST(TimeseriesHighCardinalityStress, ConcurrentDownsamplingStress)`
- Source: `tests/timeseries/test_timeseries_highcardinality_stress.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentDownsamplingStress): n/a

#### `TEST(TimeseriesHighCardinalityStress, HighCardinalitySeriesIngest)`
- Source: `tests/timeseries/test_timeseries_highcardinality_stress.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalitySeriesIngest): n/a

#### `TEST(TimeseriesHighCardinalityStress, RemoteWriteEdgeCases)`
- Source: `tests/timeseries/test_timeseries_highcardinality_stress.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesHighCardinalityStress): n/a
  - `<unnamed>` (RemoteWriteEdgeCases): n/a

#### `int main(int argc, char **argv)`
- Source: `tests/timeseries/test_timeseries_highcardinality_stress.cpp`:182
- Brief: n/a
- Parameters:
  - `argc` (int): n/a
  - `argv` (char **): n/a

### test_timeseries_metrics.cpp

#### `TEST_F(TimeSeriesMetricsTest, TestAggregation)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestAggregation): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestBatchWrite)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:32
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestBatchWrite): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestCompression)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestCompression): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestContinuousAggregateMetrics)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestContinuousAggregateMetrics): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestDataPointWrite)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:20
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestDataPointWrite): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestErrorRecording)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestErrorRecording): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestJsonExport)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestJsonExport): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestOutOfOrderWriteRecording)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestOutOfOrderWriteRecording): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestPerMetricStats)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestPerMetricStats): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestPrometheusExport)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestPrometheusExport): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestQuery)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestQuery): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestReset)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestReset): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestRetention)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestRetention): n/a

#### `TEST_F(TimeSeriesMetricsTest, TestStorageStats)`
- Source: `tests/timeseries/test_timeseries_metrics.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesMetricsTest): n/a
  - `<unnamed>` (TestStorageStats): n/a

### themis

#### `double bits_to_dbl(uint64_t b)`
- Source: `src/timeseries/gorilla.cpp`:39
- Brief: Bits to dbl.
- Parameters:
  - `b` (uint64_t): Input parameter.
- Return: Return value.
- Details: b Input parameter. Return value. Calls: std::memcpy().

#### `double bits_to_dbl_simd(uint64_t b)`
- Source: `src/timeseries/gorilla_simd.cpp`:73
- Brief: Bits to dbl simd.
- Parameters:
  - `b` (uint64_t): Input parameter.
- Return: Return value.
- Details: b Input parameter. Return value. Calls: std::memcpy().

#### `int clz64(uint64_t x)`
- Source: `src/timeseries/gorilla.cpp`:51
- Brief: Clz64.
- Parameters:
  - `x` (uint64_t): Input parameter.
- Return: Return value.
- Details: x Input parameter. Return value. Calls: defined(), _BitScanReverse64(), __builtin_clzll().

#### `int ctz64(uint64_t x)`
- Source: `src/timeseries/gorilla.cpp`:70
- Brief: Ctz64.
- Parameters:
  - `x` (uint64_t): Input parameter.
- Return: Return value.
- Details: x Input parameter. Return value. Calls: defined(), _BitScanForward64(), __builtin_ctzll().

#### `uint64_t dbl_to_bits(double v)`
- Source: `src/timeseries/gorilla.cpp`:27
- Brief: Dbl to bits.
- Parameters:
  - `v` (double): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Calls: std::memcpy().

#### `bool gorilla_simd_has_avx2() noexcept`
- Source: `src/timeseries/gorilla_simd.cpp`:39
- Brief: Returns true if AVX2 is available on this CPU at runtime (x86-64 only).
- Parameters: none
- Return: True on success.
- Details: True on success. Exception safety: noexcept. On all other platforms this always returns false.

#### `bool gorilla_simd_has_neon() noexcept`
- Source: `src/timeseries/gorilla_simd.cpp`:55
- Brief: Returns true if NEON is available at runtime.
- Parameters: none
- Return: True on success.
- Details: True on success. Exception safety: noexcept. On ARM64 targets this always returns true; on all other platforms false.

#### `AggShardResult mergeShardResults(const std::vector< AggShardResult > &shards)`
- Source: `src/timeseries/continuous_agg.cpp`:30
- Brief: Merge multiple partial shard results into one global result.
- Parameters:
  - `shards` (const std::vector< AggShardResult > &): Input parameter.
- Return: Return value.
- Details: Merge Shard Results. The coordinator calls this after collecting all shards' AggShardResults. Returns a single AggShardResult that reflects the global aggregate. shards Input parameter. Return value. shards Input parameter. Return value. Calls: empty(), max(), lowest().

#### `int64_t nowMs()`
- Source: `src/timeseries/retention.cpp`:25
- Brief: Now Ms.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count().

#### `void prefix_sum_i64(int64_t *arr, size_t n, int64_t seed)`
- Source: `src/timeseries/gorilla_simd.cpp`:87
- Brief: ────────────────────────────────────────────────────────────────────────── Phase 2a: in-place prefix-sum on int64_t array (for timestamps) On exit: arr[i] = seed + arr[0] + arr[1] + … + arr[i] AVX2 path processes 4 × int64_t per iteration using an in-register Kogge-Stone prefix scan.
- Parameters:
  - `arr` (int64_t *): Input/output parameter.
  - `n` (size_t): Input parameter.
  - `seed` (int64_t): Input parameter.
- Details: arr Input/output parameter. n Input parameter. seed Input parameter. Falls back to scalar on non-AVX2 targets. ────────────────────────────────────────────────────────────────────────── Calls: defined(), _mm256_loadu_si256(), _mm256_setzero_si256(), _mm256_permute4x64_epi64(), _mm256_blend_epi32(), _mm256_add_epi64(), _mm256_permute2x128_si256(), _mm256_set1_epi64x().

#### `void prefix_xor_u64(uint64_t *arr, size_t n, uint64_t seed)`
- Source: `src/timeseries/gorilla_simd.cpp`:169
- Brief: ────────────────────────────────────────────────────────────────────────── Phase 2b: in-place prefix-XOR on uint64_t array (for double bit-patterns) On exit: arr[i] = seed XOR arr[0] XOR arr[1] XOR … XOR arr[i] The XOR operation is associative and commutative; the in-register prefix scan uses the same shift structure as the prefix sum above.
- Parameters:
  - `arr` (uint64_t *): Input/output parameter.
  - `n` (size_t): Input parameter.
  - `seed` (uint64_t): Input parameter.
- Details: arr Input/output parameter. n Input parameter. seed Input parameter. ────────────────────────────────────────────────────────────────────────── Calls: defined(), _mm256_loadu_si256(), _mm256_setzero_si256(), _mm256_permute4x64_epi64(), _mm256_blend_epi32(), _mm256_xor_si256(), _mm256_permute2x128_si256(), _mm256_set1_epi64x().

#### `SeriesProfile profileSeries(const std::vector< TSStore::DataPoint > &points)`
- Source: `src/timeseries/compression_selector.cpp`:30
- Brief: Compute a SeriesProfile from a vector of DataPoints.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Input parameter.
- Return: Computed profile. Returns a zero-initialised profile when points is empty or contains a single element.
- Details: Profile Series. points Input data points (need not be sorted, but duplicate timestamps are counted as identical value runs). Computed profile. Returns a zero-initialised profile when points is empty or contains a single element. points Input parameter. Return value. Calls: size(), reserve(), push_back(), empty(), std::count(), begin(), end(), std::abs().

#### `uint32_t readU32BE(const uint8_t *p)`
- Source: `src/timeseries/encrypted_chunk_store.cpp`:77
- Brief: Read U32 BE.
- Parameters:
  - `p` (const uint8_t *): Input parameter.
- Return: Return value.
- Details: p Input parameter. Return value. Implements readU32BE without additional internal calls.

#### `void writeU32BE(std::vector< uint8_t > &buf, uint32_t v)`
- Source: `src/timeseries/encrypted_chunk_store.cpp`:64
- Brief: Write U32 BE.
- Parameters:
  - `buf` (std::vector< uint8_t > &): Input/output parameter.
  - `v` (uint32_t): Input parameter.
- Details: buf Input/output parameter. v Input parameter. Calls: push_back().

### themis::AdaptiveFlushController

#### `AdaptiveFlushController(AdaptiveFlushController &&)=delete`
- Source: `include/timeseries/adaptive_flush_controller.h`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveFlushController &&): n/a

#### `AdaptiveFlushController(TSStore *tsstore, AdaptiveFlushControllerConfig config={})`
- Source: `include/timeseries/adaptive_flush_controller.h`:142
- Brief: Construct controller.
- Parameters:
  - `tsstore` (TSStore *): TSStore backend (not owned; must outlive this object).
  - `config` (AdaptiveFlushControllerConfig): Configuration.
- Details: tsstore TSStore backend (not owned; must outlive this object). config Configuration.

#### `AdaptiveFlushController(const AdaptiveFlushController &)=delete`
- Source: `include/timeseries/adaptive_flush_controller.h`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaptiveFlushController &): n/a

#### `Result< void > add(const TSStore::DataPoint &point)`
- Source: `include/timeseries/adaptive_flush_controller.h`:182
- Brief: Buffer a single data point.
- Parameters:
  - `point` (const TSStore::DataPoint &): Input parameter.
- Return: Result<void> — success, or an error code.
- Details: Add. If backpressure is active (buffer ≥ 80 % full) the call blocks until the buffer drains below the watermark or the controller is stopped. After stop() any blocked producer receives ERR_API_RESOURCE_EXHAUSTED. point Data point to buffer. Result<void> — success, or an error code. point Input parameter. Return value. Calls: validatePoint(), ErrVoid(), watermarkReached(), THEMIS_WARN(), load(), recordBackpressure(), notify_one(), bp_lock().

#### `Result< size_t > addBatch(const std::vector< TSStore::DataPoint > &points)`
- Source: `include/timeseries/adaptive_flush_controller.h`:193
- Brief: Buffer a batch of data points.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Input parameter.
- Return: Result<size_t> — number of points accepted, or an error.
- Details: Add Batch. Equivalent to calling add() for each point in order, but more efficient (single lock acquisition for the whole batch). points Data points to buffer. Result<size_t> — number of points accepted, or an error. points Input parameter. Return value.

#### `size_t flush()`
- Source: `include/timeseries/adaptive_flush_controller.h`:202
- Brief: Flush all currently buffered points to TSStore immediately.
- Parameters: none
- Return: Number of points flushed.
- Details: Flush. Safe to call from any thread while the controller is running or stopped. Number of points flushed. Return value. Calls: flushInternal().

#### `size_t flushInternal()`
- Source: `include/timeseries/adaptive_flush_controller.h`:236
- Brief: Perform one flush cycle: drain up to flush_batch_size points per call.
- Parameters: none
- Return: Return value.
- Details: Flush Internal. Return value. number of points written to TSStore. Return value. Calls: reserve(), lock(), std::min(), size(), push_back(), std::move(), front(), pop_front().

#### `void flushThread()`
- Source: `include/timeseries/adaptive_flush_controller.h`:229
- Brief: Background flush loop (runs in flush_thread_).
- Parameters: none
- Details: Flush Thread. Calls: THEMIS_INFO(), load(), lock(), wait_for(), watermarkReached(), isOverdue(), THEMIS_WARN(), count().

#### `const AdaptiveFlushControllerConfig & getConfig() const`
- Source: `include/timeseries/adaptive_flush_controller.h`:223
- Brief: Return the current configuration.
- Parameters: none

#### `AdaptiveFlushControllerStats getStats() const`
- Source: `include/timeseries/adaptive_flush_controller.h`:210
- Brief: Return a snapshot of all statistics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isBackpressured() const noexcept`
- Source: `include/timeseries/adaptive_flush_controller.h`:220
- Brief: True when the buffer is at or above the backpressure watermark.
- Parameters: none
- Return: True on success.
- Details: This is a lock-free, instantaneous check. Use it to poll backpressure state without blocking. True on success. Exception safety: noexcept.

#### `bool isOverdue() const`
- Source: `include/timeseries/adaptive_flush_controller.h`:245
- Brief: True when the oldest buffered point has been held > overdue threshold.
- Parameters: none

#### `bool isRunning() const`
- Source: `include/timeseries/adaptive_flush_controller.h`:168
- Brief: True while the controller is running.
- Parameters: none

#### `AdaptiveFlushController & operator=(AdaptiveFlushController &&)=delete`
- Source: `include/timeseries/adaptive_flush_controller.h`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveFlushController &&): n/a

#### `AdaptiveFlushController & operator=(const AdaptiveFlushController &)=delete`
- Source: `include/timeseries/adaptive_flush_controller.h`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaptiveFlushController &): n/a

#### `void start()`
- Source: `include/timeseries/adaptive_flush_controller.h`:157
- Brief: Start.
- Parameters: none
- Details: Calls: exchange(), THEMIS_WARN(), THEMIS_INFO(), count(), std::thread().

#### `void stop()`
- Source: `include/timeseries/adaptive_flush_controller.h`:165
- Brief: Stop the background flush thread.
- Parameters: none
- Details: Stop. Flushes all remaining buffered points synchronously before returning. Unblocks any producers waiting on backpressure. Calls: exchange(), THEMIS_INFO(), notify_all(), lock(), joinable(), join(), flushInternal().

#### `const char * validatePoint(const TSStore::DataPoint &p) noexcept`
- Source: `include/timeseries/adaptive_flush_controller.h`:248
- Brief: Validate a single DataPoint; returns non-null error string on failure.
- Parameters:
  - `p` (const TSStore::DataPoint &): n/a

#### `bool watermarkReached() const noexcept`
- Source: `include/timeseries/adaptive_flush_controller.h`:242
- Brief: True when buffer_size_ >= watermark threshold.
- Parameters: none

#### `size_t watermarkThreshold() const noexcept`
- Source: `include/timeseries/adaptive_flush_controller.h`:239
- Brief: Watermark threshold in absolute point count.
- Parameters: none

#### `~AdaptiveFlushController()`
- Source: `include/timeseries/adaptive_flush_controller.h`:145
- Brief: n/a
- Parameters: none

### themis::AggShardResult

#### `double avg() const`
- Source: `include/timeseries/continuous_agg.h`:77
- Brief: n/a
- Parameters: none

### themis::AggregateScheduler

#### `AggregateScheduler(TSStore *store)`
- Source: `include/timeseries/aggregate_scheduler.h`:90
- Brief: Aggregate Scheduler.
- Parameters:
  - `store` (TSStore *): Input/output parameter.
- Return: Return value.
- Details: store Input/output parameter. Return value.

#### `AggregateScheduler(TSStore *store, const Config &config)`
- Source: `include/timeseries/aggregate_scheduler.h`:91
- Brief: n/a
- Parameters:
  - `store` (TSStore *): n/a
  - `config` (const Config &): n/a

#### `void backfill_range(const std::string &agg_id, int64_t start_ms, int64_t end_ms)`
- Source: `include/timeseries/aggregate_scheduler.h`:188
- Brief: Backfill a specific time range for a registered aggregate.
- Parameters:
  - `agg_id` (const std::string &): Identifier of the agg.
  - `start_ms` (int64_t): Input parameter.
  - `end_ms` (int64_t): Input parameter.
- Details: Backfill range. Processes the range [start_ms, end_ms) without touching the watermark, so this can be used to recover from gaps in watermark history. agg_id Aggregate ID as returned by registerAggregate() start_ms Range start (ms since epoch, inclusive) end_ms Range end (ms since epoch, exclusive) agg_id Identifier of the agg. start_ms Input parameter. end_ms Input parameter. Calls: THEMIS_WARN(), Tracer::startSpan(), setAttribute(), lock(), find(), end(), recordError(), THEMIS_INFO().

#### `void catchUpMissedWindows(ScheduledAggregate &agg, int64_t current_time_ms)`
- Source: `include/timeseries/aggregate_scheduler.h`:232
- Brief: Catch Up Missed Windows.
- Parameters:
  - `agg` (ScheduledAggregate &): Input/output parameter.
  - `current_time_ms` (int64_t): Input parameter.
- Details: agg Input/output parameter. current_time_ms Input parameter. agg Input/output parameter. current_time_ms Input parameter. Calls: count(), std::min(), THEMIS_INFO(), Tracer::startSpan(), setAttribute(), refresh(), THEMIS_DEBUG(), THEMIS_ERROR().

#### `void disableAggregate(const std::string &id)`
- Source: `include/timeseries/aggregate_scheduler.h`:134
- Brief: Disable Aggregate.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Details: id Input parameter. id Input parameter. Calls: lock(), find(), end(), THEMIS_INFO().

#### `void enableAggregate(const std::string &id)`
- Source: `include/timeseries/aggregate_scheduler.h`:129
- Brief: Enable Aggregate.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Details: id Input parameter. id Input parameter. Calls: lock(), find(), end(), THEMIS_INFO().

#### `std::string generateAggregateId(const AggConfig &config) const`
- Source: `include/timeseries/aggregate_scheduler.h`:244
- Brief: Generate Aggregate Id.
- Parameters:
  - `config` (const AggConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `int64_t getCurrentTimeMs() const`
- Source: `include/timeseries/aggregate_scheduler.h`:238
- Brief: Helpers.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Stats getStats() const`
- Source: `include/timeseries/aggregate_scheduler.h`:160
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isRunning() const`
- Source: `include/timeseries/aggregate_scheduler.h`:102
- Brief: n/a
- Parameters: none

#### `std::vector< ScheduledAggregate > listAggregates() const`
- Source: `include/timeseries/aggregate_scheduler.h`:165
- Brief: List Aggregates.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool needsRefresh(const ScheduledAggregate &agg, int64_t current_time_ms) const`
- Source: `include/timeseries/aggregate_scheduler.h`:226
- Brief: Needs Refresh.
- Parameters:
  - `agg` (const ScheduledAggregate &): Input parameter.
  - `current_time_ms` (int64_t): Input parameter.
- Return: True on success.
- Details: agg Input parameter. current_time_ms Input parameter. True on success.

#### `void refreshAggregate(ScheduledAggregate &agg)`
- Source: `include/timeseries/aggregate_scheduler.h`:219
- Brief: Refresh Aggregate.
- Parameters:
  - `agg` (ScheduledAggregate &): Input/output parameter.
- Details: agg Input/output parameter. agg Input/output parameter. Calls: Tracer::startSpan(), setAttribute(), std::chrono::steady_clock::now(), getCurrentTimeMs(), count(), getWatermark(), refreshIncremental(), recordAggRefreshLag().

#### `void refreshAll()`
- Source: `include/timeseries/aggregate_scheduler.h`:144
- Brief: Refresh All.
- Parameters: none
- Details: Calls: Tracer::startSpan(), lock(), refreshAggregate(), setAttribute(), THEMIS_INFO().

#### `void refreshNow(const std::string &id)`
- Source: `include/timeseries/aggregate_scheduler.h`:140
- Brief: Manual operations.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Details: ===== Manual Operations ===== id Input parameter. id Input parameter. Calls: Tracer::startSpan(), setAttribute(), lock(), find(), end(), THEMIS_WARN(), recordError(), refreshAggregate().

#### `std::string registerAggregate(const AggConfig &config, std::chrono::milliseconds refresh_interval=std::chrono::minutes(5))`
- Source: `include/timeseries/aggregate_scheduler.h`:118
- Brief: Register Aggregate.
- Parameters:
  - `config` (const AggConfig &): Input parameter.
  - `refresh_interval` (std::chrono::milliseconds): Input parameter.
- Return: Aggregate ID
- Details: Register a continuous aggregate from basic config config Aggregate configuration refresh_interval How often to refresh (default: 5 minutes) Aggregate ID config Input parameter. refresh_interval Input parameter. Return value. Calls: lock(), generateAggregateId(), std::move(), THEMIS_INFO(), count().

#### `void registerAggregate(const ScheduledAggregate &agg)`
- Source: `include/timeseries/aggregate_scheduler.h`:110
- Brief: Register Aggregate.
- Parameters:
  - `agg` (const ScheduledAggregate &): Input parameter.
- Details: ===== Aggregate Management ===== Register a continuous aggregate for automatic refresh agg Scheduled aggregate configuration agg Input parameter. Calls: lock(), empty(), generateAggregateId(), THEMIS_INFO(), count().

#### `void schedulerLoop()`
- Source: `include/timeseries/aggregate_scheduler.h`:214
- Brief: Scheduler loop.
- Parameters: none
- Details: ===== Scheduler Loop ===== Calls: THEMIS_INFO(), load(), Tracer::startSpan(), getCurrentTimeMs(), lock(), std::chrono::system_clock::now(), needsRefresh(), catchUpMissedWindows().

#### `void setMetrics(std::shared_ptr< TimeSeriesMetrics > metrics)`
- Source: `include/timeseries/aggregate_scheduler.h`:176
- Brief: Attach a TimeSeriesMetrics collector to receive per-aggregate refresh latency and lag measurements.
- Parameters:
  - `metrics` (std::shared_ptr< TimeSeriesMetrics >): Input parameter.
- Details: Set Metrics. Must be called before start() to ensure all refreshes are recorded. Passing nullptr disables metric recording. metrics Shared pointer to the metrics collector (may be null) metrics Input parameter. Calls: lock(), std::move().

#### `void start()`
- Source: `include/timeseries/aggregate_scheduler.h`:97
- Brief: Lifecycle.
- Parameters: none
- Details: ===== Lifecycle ===== Calls: lock(), load(), THEMIS_WARN(), store(), std::thread(), THEMIS_INFO(), size(), count().

#### `void stop()`
- Source: `include/timeseries/aggregate_scheduler.h`:101
- Brief: Stop.
- Parameters: none
- Details: Calls: lock(), load(), store(), notify_all(), joinable(), join(), THEMIS_INFO().

#### `void unregisterAggregate(const std::string &id)`
- Source: `include/timeseries/aggregate_scheduler.h`:124
- Brief: Unregister Aggregate.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Details: id Input parameter. id Input parameter. Calls: lock(), find(), end(), THEMIS_INFO(), deleteWatermark(), erase().

#### `~AggregateScheduler()`
- Source: `include/timeseries/aggregate_scheduler.h`:92
- Brief: n/a
- Parameters: none

### themis::AnomalyDetector

#### `AnomalyDetector(AnomalyConfig cfg={})`
- Source: `include/timeseries/anomaly_detection.h`:163
- Brief: n/a
- Parameters:
  - `cfg` (AnomalyConfig): n/a

#### `const AnomalyConfig & config() const`
- Source: `include/timeseries/anomaly_detection.h`:186
- Brief: n/a
- Parameters: none

#### `std::vector< AnomalyPoint > detect(const std::vector< TSStore::DataPoint > &points) const`
- Source: `include/timeseries/anomaly_detection.h`:170
- Brief: Detect anomalies using the stored configuration.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Input parameter.
- Return: Return value.
- Details: points Input parameter. Return value.

#### `std::vector< AnomalyPoint > detect(const std::vector< TSStore::DataPoint > &points, const AnomalyConfig &cfg) const override`
- Source: `include/timeseries/anomaly_detection.h`:176
- Brief: Detect anomalies using an explicit configuration (overrides stored).
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): n/a
  - `cfg` (const AnomalyConfig &): n/a

#### `void setConfig(const AnomalyConfig &cfg)`
- Source: `include/timeseries/anomaly_detection.h`:185
- Brief: Set Config.
- Parameters:
  - `cfg` (const AnomalyConfig &): Input parameter.
- Details: cfg Input parameter. Implements setConfig without additional internal calls.

### themis::BackwardFillGapFiller

#### `std::vector< TSStore::DataPoint > fill(const std::vector< TSStore::DataPoint > &points, const std::vector< int64_t > &timestamps_to_fill, const GapFillConfig &cfg) const override`
- Source: `include/timeseries/gap_fill.h`:151
- Brief: Fill gaps for the requested timestamps.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Known data points (must be sorted ascending by timestamp_ms).
  - `timestamps_to_fill` (const std::vector< int64_t > &): Timestamps for which a value is required (must be sorted ascending).
  - `cfg` (const GapFillConfig &): Gap-fill configuration.
- Return: One DataPoint per requested timestamp, in ascending timestamp order. Timestamps present in points are returned verbatim; missing timestamps receive a synthesised value.
- Details: points Known data points (must be sorted ascending by timestamp_ms). timestamps_to_fill Timestamps for which a value is required (must be sorted ascending). cfg Gap-fill configuration. One DataPoint per requested timestamp, in ascending timestamp order. Timestamps present in points are returned verbatim; missing timestamps receive a synthesised value.

### themis::BitReader

#### `BitReader(const std::vector< uint8_t > &data) noexcept`
- Source: `include/timeseries/gorilla.h`:104
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a

#### `BitReader(const uint8_t *data, size_t size) noexcept`
- Source: `include/timeseries/gorilla.h`:96
- Brief: n/a
- Parameters:
  - `data` (const uint8_t *): n/a
  - `size` (size_t): n/a

#### `void alignToByte() noexcept`
- Source: `include/timeseries/gorilla.h`:210
- Brief: n/a
- Parameters: none

#### `bool eof() const noexcept`
- Source: `include/timeseries/gorilla.h`:208
- Brief: n/a
- Parameters: none

#### `bool readBit() noexcept`
- Source: `include/timeseries/gorilla.h`:107
- Brief: n/a
- Parameters: none

#### `uint64_t readBits(int bits) noexcept`
- Source: `include/timeseries/gorilla.h`:122
- Brief: n/a
- Parameters:
  - `bits` (int): n/a

#### `uint64_t readVarUInt() noexcept`
- Source: `include/timeseries/gorilla.h`:172
- Brief: n/a
- Parameters: none

#### `int64_t readZigZag64() noexcept`
- Source: `include/timeseries/gorilla.h`:199
- Brief: n/a
- Parameters: none

### themis::BitWriter

#### `void alignToByte()`
- Source: `include/timeseries/gorilla.h`:75
- Brief: Align To Byte.
- Parameters: none
- Details: Calls: push_back().

#### `std::vector< uint8_t > finish()`
- Source: `include/timeseries/gorilla.h`:80
- Brief: Finish.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: push_back().

#### `void writeBit(bool bit)`
- Source: `include/timeseries/gorilla.h`:55
- Brief: Write Bit.
- Parameters:
  - `bit` (bool): Input parameter.
- Details: ---- BitWriter ---- bit Input parameter. bit Input parameter. Calls: push_back().

#### `void writeBits(uint64_t value, int bits)`
- Source: `include/timeseries/gorilla.h`:61
- Brief: Write Bits.
- Parameters:
  - `value` (uint64_t): Input parameter.
  - `bits` (int): Input parameter.
- Details: value Input parameter. bits Input parameter. value Input parameter. bits Input parameter. Calls: writeBit().

#### `void writeVarUInt(uint64_t value)`
- Source: `include/timeseries/gorilla.h`:66
- Brief: Write Var UInt.
- Parameters:
  - `value` (uint64_t): Input parameter.
- Details: value Input parameter. v Input parameter. Calls: push_back().

#### `void writeZigZag64(int64_t value)`
- Source: `include/timeseries/gorilla.h`:71
- Brief: Write Zig Zag64.
- Parameters:
  - `value` (int64_t): Input parameter.
- Details: value Input parameter. value Input parameter. Calls: writeVarUInt().

### themis::ContinuousAggMaterializationEngine

#### `ContinuousAggMaterializationEngine(TSStore *store)`
- Source: `include/timeseries/continuous_agg.h`:346
- Brief: Continuous Agg Materialization Engine.
- Parameters:
  - `store` (TSStore *): Input/output parameter.
- Return: Return value.
- Details: store Input/output parameter. Return value.

#### `bool createAggregate(ContinuousAggDefinition def)`
- Source: `include/timeseries/continuous_agg.h`:363
- Brief: Register a new continuous aggregate definition.
- Parameters:
  - `def` (ContinuousAggDefinition): Input parameter.
- Return: true on success; false if a definition with the same name already exists.
- Details: Create Aggregate. The definition's name must be unique within this engine instance. The agg_id field is populated automatically from name and the derived metric name. true on success; false if a definition with the same name already exists. def Input parameter. def Input parameter. True when the operation succeeds. Calls: count(), ContinuousAggregateManager::derivedMetricName(), push_back(), emplace(), std::move().

#### `bool dropAggregate(const std::string &name)`
- Source: `include/timeseries/continuous_agg.h`:376
- Brief: Remove a continuous aggregate definition.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: true if the definition was found and removed; false otherwise.
- Details: Drop Aggregate. Deletes the watermark entry from the TSStore and removes the definition from the registry. The materialized data points themselves are NOT deleted; callers must clean them up via TSStore retention policies if desired. true if the definition was found and removed; false otherwise. name Input parameter. name Input parameter. True when the operation succeeds. Calls: find(), end(), deleteWatermark(), erase(), std::remove(), begin().

#### `std::optional< ContinuousAggDefinition > getAggregate(const std::string &name) const`
- Source: `include/timeseries/continuous_agg.h`:383
- Brief: Return the definition for the given aggregate name, or nullopt.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: Return value.
- Details: name Input parameter. Return value.

#### `std::optional< ContinuousAggMaterializationStatus > getAggregateStatus(const std::string &name) const`
- Source: `include/timeseries/continuous_agg.h`:448
- Brief: Return the materialization status of a named aggregate.
- Parameters:
  - `name` (const std::string &): n/a
- Return: Status snapshot, or nullopt if the aggregate is not registered.
- Details: Status snapshot, or nullopt if the aggregate is not registered.

#### `std::vector< ContinuousAggMaterializationStatus > getAllStatus() const`
- Source: `include/timeseries/continuous_agg.h`:454
- Brief: Return the materialization status of all registered aggregates.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< std::string > listAggregates() const`
- Source: `include/timeseries/continuous_agg.h`:389
- Brief: List the names of all registered aggregates.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< TSStore::DataPoint > queryMaterialized(const std::string &name, int64_t from_ms, int64_t to_ms) const`
- Source: `include/timeseries/continuous_agg.h`:435
- Brief: Query materialized aggregate data for a named aggregate.
- Parameters:
  - `name` (const std::string &): Aggregate name.
  - `from_ms` (int64_t): Start of the query window (inclusive, ms since epoch).
  - `to_ms` (int64_t): End of the query window (inclusive, ms since epoch).
- Return: TSStore data points, or an empty vector if not found / no data.
- Details: Reads the derived metric from TSStore for the time range [from_ms, to_ms]. Only previously-materialized windows are returned; this method does NOT trigger a refresh. name Aggregate name. from_ms Start of the query window (inclusive, ms since epoch). to_ms End of the query window (inclusive, ms since epoch). TSStore data points, or an empty vector if not found / no data.

#### `size_t refreshAggregate(const std::string &name, int64_t to_ms)`
- Source: `include/timeseries/continuous_agg.h`:406
- Brief: Incrementally refresh a single named aggregate up to to_ms.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `to_ms` (int64_t): Input parameter.
- Return: Number of windows written, or 0 if not found / already current.
- Details: Refresh Aggregate. Uses the persisted watermark to scan only new data, then advances the watermark after a successful write. Returns the number of aggregate windows written (0 if already up-to-date or the aggregate is INACTIVE). name Aggregate name (must have been registered via createAggregate). to_ms Upper bound of the refresh window (ms since epoch). Number of windows written, or 0 if not found / already current. name Input parameter. to_ms Input parameter. Return value. Calls: find(), end(), refreshIncremental().

#### `size_t refreshAll(int64_t to_ms)`
- Source: `include/timeseries/continuous_agg.h`:417
- Brief: Refresh all ACTIVE aggregates with auto_refresh == true.
- Parameters:
  - `to_ms` (int64_t): Input parameter.
- Return: Total number of aggregate windows written across all aggregates.
- Details: Refresh All. Iterates over registered definitions in insertion order and calls refreshAggregate() for each eligible aggregate. to_ms Upper bound applied to every aggregate. Total number of aggregate windows written across all aggregates. to_ms Input parameter. Return value. Calls: find(), end(), refreshIncremental().

### themis::ContinuousAggWatermarkStore

#### `ContinuousAggWatermarkStore(TSStore *store)`
- Source: `include/timeseries/continuous_agg.h`:168
- Brief: n/a
- Parameters:
  - `store` (TSStore *): n/a

#### `void deleteWatermark(const std::string &agg_id)`
- Source: `include/timeseries/continuous_agg.h`:191
- Brief: Remove the watermark entry (e.g., when an aggregate is deleted).
- Parameters:
  - `agg_id` (const std::string &): Identifier of the agg.
- Details: Delete Watermark. agg_id Input parameter. agg_id Identifier of the agg. Calls: std::string(), deleteSystemMeta().

#### `int64_t getWatermark(const std::string &agg_id) const`
- Source: `include/timeseries/continuous_agg.h`:176
- Brief: Read the current watermark for agg_id.
- Parameters:
  - `agg_id` (const std::string &): Input parameter.
- Return: Milliseconds-since-epoch of the last successfully processed upper boundary, or 0 if no watermark has been set yet.
- Details: Milliseconds-since-epoch of the last successfully processed upper boundary, or 0 if no watermark has been set yet. agg_id Input parameter.

#### `void setWatermark(const std::string &agg_id, int64_t watermark_ms)`
- Source: `include/timeseries/continuous_agg.h`:185
- Brief: Persist the watermark to watermark_ms for agg_id.
- Parameters:
  - `agg_id` (const std::string &): Identifier of the agg.
  - `watermark_ms` (int64_t): Input parameter.
- Details: Set Watermark. This write goes through RocksDB's WAL, so it survives node restarts. agg_id Input parameter. watermark_ms Input parameter. agg_id Identifier of the agg. watermark_ms Input parameter. Calls: std::string(), putSystemMeta(), std::to_string().

### themis::ContinuousAggregateManager

#### `ContinuousAggregateManager(TSStore *store)`
- Source: `include/timeseries/continuous_agg.h`:201
- Brief: n/a
- Parameters:
  - `store` (TSStore *): n/a

#### `std::string derivedMetricName(const std::string &base, std::chrono::milliseconds win)`
- Source: `include/timeseries/continuous_agg.h`:251
- Brief: Derived Metric Name.
- Parameters:
  - `base` (const std::string &): Input parameter.
  - `win` (std::chrono::milliseconds): Input parameter.
- Return: Return value.
- Details: base Input parameter. win Input parameter. Return value. base Input parameter. win Input parameter. Return value. Calls: count(), str().

#### `void refresh(const AggConfig &cfg, int64_t from_ms, int64_t to_ms)`
- Source: `include/timeseries/continuous_agg.h`:209
- Brief: Compute aggregates for [from,to] and store as derived metric Derived metric name: metric + "__agg_" + window_ms.
- Parameters:
  - `cfg` (const AggConfig &): Input parameter.
  - `from_ms` (int64_t): Input parameter.
  - `to_ms` (int64_t): Input parameter.
- Details: Refresh. cfg Input parameter. from_ms Input parameter. to_ms Input parameter. cfg Input parameter. from_ms Input parameter. to_ms Input parameter. Calls: count(), derivedMetricName(), has_value(), std::min(), query(), value(), empty(), size().

#### `void refreshHierarchy(const RollupHierarchy &hierarchy, int64_t from_ms, int64_t to_ms)`
- Source: `include/timeseries/continuous_agg.h`:243
- Brief: Refresh Hierarchy.
- Parameters:
  - `hierarchy` (const RollupHierarchy &): Input parameter.
  - `from_ms` (int64_t): Input parameter.
  - `to_ms` (int64_t): Input parameter.
- Details: Refresh all levels of a rollup hierarchy. Processes levels from smallest window to largest. Each level reads from the previous level's output (or raw data for the first level). hierarchy Rollup level definitions from_ms Start of refresh window (milliseconds) to_ms End of refresh window (milliseconds) hierarchy Input parameter. from_ms Input parameter. to_ms Input parameter. Calls: empty(), refresh(), derivedMetricName().

#### `size_t refreshIncremental(const AggConfig &cfg, const std::string &agg_id, int64_t to_ms, ContinuousAggWatermarkStore &wm_store)`
- Source: `include/timeseries/continuous_agg.h`:229
- Brief: Incremental refresh using watermark pushdown.
- Parameters:
  - `cfg` (const AggConfig &): Input parameter.
  - `agg_id` (const std::string &): Identifier of the agg.
  - `to_ms` (int64_t): Input parameter.
  - `wm_store` (ContinuousAggWatermarkStore &): Input/output parameter.
- Return: Number of aggregate windows written.
- Details: Refresh Incremental. Reads the current watermark for agg_id from wm_store, scans only the range [watermark, to_ms) in TSStore (skipping already-processed data), writes the aggregate points, and advances the watermark atomically to to_ms after a successful write. If no watermark exists yet the full range [0, to_ms) is processed, which provides a correct initial catch-up. cfg Aggregate configuration (metric, entity, window). agg_id Unique aggregate identifier used as the watermark key. to_ms Upper bound of the refresh window (ms since epoch). wm_store Watermark store for reading/writing the per-aggregate watermark. Number of aggregate windows written. cfg Input parameter. agg_id Identifier of the agg. to_ms Input parameter. wm_store Input/output parameter. Return value. Calls: getWatermark(), query(), value(), empty(), setWatermark(), count(), refresh().

### themis::DistributedAggregateCoordinator

#### `DistributedAggregateCoordinator(TSStore *local_store, int shard_count=1, ShardQueryFn shard_query=nullptr)`
- Source: `include/timeseries/continuous_agg.h`:121
- Brief: n/a
- Parameters:
  - `local_store` (TSStore *): Local TSStore (used when shard_query is nullptr)
  - `shard_count` (int): Number of shards (1 = single-node)
  - `shard_query` (ShardQueryFn): Callback to query a remote shard; pass nullptr for single-node mode (local_store is used directly)
- Details: local_store Local TSStore (used when shard_query is nullptr) shard_count Number of shards (1 = single-node) shard_query Callback to query a remote shard; pass nullptr for single-node mode (local_store is used directly)

#### `AggShardResult refreshAggregate(const AggConfig &cfg, int64_t from_ms, int64_t to_ms)`
- Source: `include/timeseries/continuous_agg.h`:137
- Brief: Refresh Aggregate.
- Parameters:
  - `cfg` (const AggConfig &): Input parameter.
  - `from_ms` (int64_t): Input parameter.
  - `to_ms` (int64_t): Input parameter.
- Return: Merged aggregate result
- Details: Compute and persist aggregates across all shards. In single-node mode delegates to ContinuousAggregateManager. In multi-shard mode fans out to all shards and merges. Merged aggregate result cfg Input parameter. from_ms Input parameter. to_ms Input parameter. cfg Input parameter. from_ms Input parameter. to_ms Input parameter. Return value. Calls: mgr(), refresh(), ContinuousAggregateManager::derivedMetricName(), value_or(), query(), value(), empty(), max().

#### `int shardCount() const`
- Source: `include/timeseries/continuous_agg.h`:142
- Brief: n/a
- Parameters: none

### themis::DownsamplingPipeline

#### `DownsamplingPipeline(TSStore *store)`
- Source: `include/timeseries/downsampling.h`:145
- Brief: Downsampling Pipeline.
- Parameters:
  - `store` (TSStore *): TSStore to read raw/tier data from and write tier output to. Not owned; must outlive the pipeline.
- Return: Return value.
- Details: store TSStore to read raw/tier data from and write tier output to. Not owned; must outlive the pipeline. Return value.

#### `DownsamplingPipeline(const DownsamplingPipeline &)=delete`
- Source: `include/timeseries/downsampling.h`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DownsamplingPipeline &): n/a

#### `void addPolicy(const DownsamplingPolicy &policy)`
- Source: `include/timeseries/downsampling.h`:158
- Brief: Register a downsampling policy.
- Parameters:
  - `policy` (const DownsamplingPolicy &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: Add Policy. Must be called before any refresh() for the given metric. Registers the policy's tiers with the internal TierSelector. policy Input parameter. policy Input parameter. std::invalid_argument if an error occurs. Calls: empty(), registerPolicy(), THEMIS_INFO(), size().

#### `int64_t getWatermark(const std::string &metric, const std::string &tier_name) const`
- Source: `include/timeseries/downsampling.h`:199
- Brief: Get the current watermark for a metric:tier combination.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `tier_name` (const std::string &): Input parameter.
- Return: Return value.
- Details: Returns 0 if no data has been processed yet for that tier. metric Input parameter. tier_name Input parameter. Return value.

#### `int64_t nowMs()`
- Source: `include/timeseries/downsampling.h`:232
- Brief: Returns current epoch time in milliseconds.
- Parameters: none
- Return: Return value.
- Details: Now Ms. Return value. Return value. Calls: system_clock::now(), time_since_epoch(), count().

#### `DownsamplingPipeline & operator=(const DownsamplingPipeline &)=delete`
- Source: `include/timeseries/downsampling.h`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DownsamplingPipeline &): n/a

#### `size_t refresh(int64_t to_ms=0)`
- Source: `include/timeseries/downsampling.h`:175
- Brief: Process all registered metrics from their stored watermark up to to_ms.
- Parameters:
  - `to_ms` (int64_t): Input parameter.
- Return: Total number of aggregate data points written across all tiers.
- Details: Refresh. For each registered metric, this method iterates through the tier list from finest to coarsest. Each tier reads from its input source (raw data for tier[0], tier[i-1] output for tier[i]) over the window [watermark, to_ms) and writes aggregate results back to TSStore under the derived metric name produced by ContinuousAggregateManager::derivedMetricName(). After a successful write the per-tier watermark is advanced to to_ms. to_ms Upper bound of the refresh window (ms since epoch). Defaults to now() when 0. Total number of aggregate data points written across all tiers. to_ms Input parameter. Return value. Calls: refreshMetric().

#### `size_t refreshMetric(const std::string &metric, int64_t to_ms=0)`
- Source: `include/timeseries/downsampling.h`:184
- Brief: Refresh a single metric.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `to_ms` (int64_t): Input parameter.
- Return: Number of aggregate points written.
- Details: Refresh Metric. metric Metric name (must be registered via addPolicy()). to_ms Upper bound of refresh window (0 = now). Number of aggregate points written. metric Input parameter. to_ms Input parameter. Return value. Calls: find(), end(), THEMIS_WARN(), nowMs(), getWatermark(), THEMIS_DEBUG(), ContinuousAggregateManager::derivedMetricName(), refreshTier().

#### `size_t refreshTier(const DownsamplingPolicy &policy, const DownsamplingTier &tier, const std::string &input_metric, int64_t from_ms, int64_t to_ms)`
- Source: `include/timeseries/downsampling.h`:243
- Brief: Refresh a single tier for a metric, reading from input_metric over [from_ms, to_ms).
- Parameters:
  - `policy` (const DownsamplingPolicy &): Input parameter.
  - `tier` (const DownsamplingTier &): Input parameter.
  - `input_metric` (const std::string &): Input parameter.
  - `from_ms` (int64_t): Input parameter.
  - `to_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: Refresh Tier. policy Input parameter. tier Input parameter. input_metric Input parameter. from_ms Input parameter. to_ms Input parameter. Return value.

#### `void setWatermark(const std::string &metric, const std::string &tier_name, int64_t watermark_ms)`
- Source: `include/timeseries/downsampling.h`:207
- Brief: Manually set a watermark (e.g. for backfill or disaster recovery).
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `tier_name` (const std::string &): Name of the tier.
  - `watermark_ms` (int64_t): Input parameter.
- Details: Set Watermark. metric Input parameter. tier_name Input parameter. watermark_ms Input parameter. metric Input parameter. tier_name Name of the tier. watermark_ms Input parameter.

#### `const TierSelector & tierSelector() const`
- Source: `include/timeseries/downsampling.h`:189
- Brief: Return the internal TierSelector for integration with TSQueryOptimizer.
- Parameters: none

#### `std::string watermarkKey(const std::string &metric, const std::string &tier_name)`
- Source: `include/timeseries/downsampling.h`:226
- Brief: Watermark Key.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `tier_name` (const std::string &): Name of the tier.
- Return: Return value.
- Details: metric Input parameter. tier_name Input parameter. Return value. metric Input parameter. tier_name Name of the tier. Return value.

#### `~DownsamplingPipeline()=default`
- Source: `include/timeseries/downsampling.h`:146
- Brief: n/a
- Parameters: none

### themis::DownsamplingPolicy

#### `DownsamplingPolicy defaultPolicy(const std::string &metric, const std::optional< std::string > &entity=std::nullopt)`
- Source: `include/timeseries/downsampling.h`:70
- Brief: Build a policy with three typical tiers: 1m (7d) → 1h (90d) → 1d (2y).
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::optional< std::string > &): Input parameter.
- Return: Return value.
- Details: Default Policy. metric Input parameter. entity Input parameter. Return value.

### themis::DownsamplingTier

#### `DownsamplingTier days(int n, std::chrono::seconds keep=std::chrono::seconds{0})`
- Source: `include/timeseries/downsampling.h`:44
- Brief: Days.
- Parameters:
  - `n` (int): Input parameter.
  - `keep` (std::chrono::seconds): Input parameter.
- Return: Return value.
- Details: n Input parameter. keep Input parameter. Return value. Calls: std::to_string().

#### `DownsamplingTier hours(int n, std::chrono::seconds keep=std::chrono::seconds{0})`
- Source: `include/timeseries/downsampling.h`:43
- Brief: Hours.
- Parameters:
  - `n` (int): Input parameter.
  - `keep` (std::chrono::seconds): Input parameter.
- Return: Return value.
- Details: n Input parameter. keep Input parameter. Return value. Calls: std::to_string().

#### `DownsamplingTier minutes(int n, std::chrono::seconds keep=std::chrono::seconds{0})`
- Source: `include/timeseries/downsampling.h`:42
- Brief: Convenience factory for common tier sizes.
- Parameters:
  - `n` (int): Input parameter.
  - `keep` (std::chrono::seconds): Input parameter.
- Return: Return value.
- Details: Minutes. n Input parameter. keep Input parameter. Return value. Calls: std::to_string().

### themis::EncryptedChunkStore

#### `EncryptedChunkStore(CurrentKeyFn current_key_fn, LookupKeyFn lookup_key_fn, utils::AuditLogger *audit_logger=nullptr, std::string accessor_identity="tsstore")`
- Source: `include/timeseries/encrypted_chunk_store.h`:147
- Brief: Construct an EncryptedChunkStore.
- Parameters:
  - `current_key_fn` (CurrentKeyFn): Callback returning the current (key_id, key_bytes).
  - `lookup_key_fn` (LookupKeyFn): Callback for historical key lookup by key_id.
  - `audit_logger` (utils::AuditLogger *): Optional audit logger; may be nullptr.
  - `accessor_identity` (std::string): Identity string recorded in audit log entries (e.g. service account or user name).
- Details: current_key_fn Callback returning the current (key_id, key_bytes). lookup_key_fn Callback for historical key lookup by key_id. audit_logger Optional audit logger; may be nullptr. accessor_identity Identity string recorded in audit log entries (e.g. service account or user name).

#### `EncryptedChunkStore(EncryptedChunkStore &&)=delete`
- Source: `include/timeseries/encrypted_chunk_store.h`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (EncryptedChunkStore &&): n/a

#### `EncryptedChunkStore(const EncryptedChunkStore &)=delete`
- Source: `include/timeseries/encrypted_chunk_store.h`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (const EncryptedChunkStore &): n/a

#### `void auditKeyAccess(const std::string &operation, const std::string &series_id, const std::string &key_id, const std::string &chunk_range)`
- Source: `include/timeseries/encrypted_chunk_store.h`:281
- Brief: Audit Key Access.
- Parameters:
  - `operation` (const std::string &): Input parameter.
  - `series_id` (const std::string &): Identifier of the series.
  - `key_id` (const std::string &): Identifier of the key.
  - `chunk_range` (const std::string &): Input parameter.
- Details: operation Input parameter. series_id Input parameter. key_id Input parameter. chunk_range Input parameter. operation Input parameter. series_id Identifier of the series. key_id Identifier of the key. chunk_range Input parameter.

#### `std::vector< uint8_t > decryptChunk(const std::string &series_id, const std::vector< uint8_t > &blob, const std::string &chunk_range="")`
- Source: `include/timeseries/encrypted_chunk_store.h`:197
- Brief: Decrypt a chunk blob produced by encryptChunk().
- Parameters:
  - `series_id` (const std::string &): "{metric}:{entity}" string — must match the value used during encryption (HKDF salt).
  - `blob` (const std::vector< uint8_t > &): Ciphertext blob returned by encryptChunk().
  - `chunk_range` (const std::string &): Human-readable range string for the audit log.
- Return: Decrypted plaintext (Gorilla-compressed bytes).
- Throws:
  - std::runtime_error: if the key is unavailable or authentication fails.
- Details: Looks up the master key by the embedded key_id, re-derives the DEK via HKDF, and decrypts with AES-256-GCM. series_id "{metric}:{entity}" string — must match the value used during encryption (HKDF salt). blob Ciphertext blob returned by encryptChunk(). chunk_range Human-readable range string for the audit log. Decrypted plaintext (Gorilla-compressed bytes). std::runtime_error if the key is unavailable or authentication fails.

#### `std::vector< uint8_t > deriveDEK(const std::vector< uint8_t > &master_key, const std::string &series_id)`
- Source: `include/timeseries/encrypted_chunk_store.h`:271
- Brief: Derive a 32-byte DEK: HKDF(master_key, salt=series_id, info=...).
- Parameters:
  - `master_key` (const std::vector< uint8_t > &): n/a
  - `series_id` (const std::string &): n/a

#### `EncryptResult encryptChunk(const std::string &series_id, const std::vector< uint8_t > &plaintext, const std::string &chunk_range="")`
- Source: `include/timeseries/encrypted_chunk_store.h`:180
- Brief: Encrypt a Gorilla-compressed binary chunk.
- Parameters:
  - `series_id` (const std::string &): "{metric}:{entity}" string used as HKDF salt.
  - `plaintext` (const std::vector< uint8_t > &): Raw Gorilla-compressed bytes to encrypt.
  - `chunk_range` (const std::string &): Human-readable range string "[first_ts,last_ts]" written to the audit log.
- Return: EncryptResult with the ciphertext blob and the key_id embedded in the blob header.
- Throws:
  - std::runtime_error: on OpenSSL or key-provider failure.
- Details: Derives a DEK from the current master key and the series_id via HKDF-SHA256, then encrypts plaintext with AES-256-GCM. The returned EncryptResult contains the ciphertext blob and the key_id actually used during encryption. Always use the returned key_id (not getCurrentKeyId()) when persisting the JSON envelope to guarantee consistency even if the master key rotates during the call. series_id "{metric}:{entity}" string used as HKDF salt. plaintext Raw Gorilla-compressed bytes to encrypt. chunk_range Human-readable range string "[first_ts,last_ts]" written to the audit log. EncryptResult with the ciphertext blob and the key_id embedded in the blob header. std::runtime_error on OpenSSL or key-provider failure.

#### `std::string getCurrentKeyId() const`
- Source: `include/timeseries/encrypted_chunk_store.h`:211
- Brief: Returns the key_id of the current master key without performing any cryptographic operations.
- Parameters: none
- Throws:
  - std::runtime_error: if current_key_fn fails.
- Details: Used by TsEncryptedKeyRotation to identify stale chunks efficiently. For writing new chunks, prefer the key_id returned by encryptChunk() to ensure consistency even under concurrent key rotation. std::runtime_error if current_key_fn fails.

#### `bool isAuditEnabled() const noexcept`
- Source: `include/timeseries/encrypted_chunk_store.h`:216
- Brief: Returns true when an audit logger is attached.
- Parameters: none

#### `EncryptedChunkStore & operator=(EncryptedChunkStore &&)=delete`
- Source: `include/timeseries/encrypted_chunk_store.h`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (EncryptedChunkStore &&): n/a

#### `EncryptedChunkStore & operator=(const EncryptedChunkStore &)=delete`
- Source: `include/timeseries/encrypted_chunk_store.h`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (const EncryptedChunkStore &): n/a

#### `void setAccessorIdentity(std::string identity)`
- Source: `include/timeseries/encrypted_chunk_store.h`:248
- Brief: Replace the accessor identity string used in audit records.
- Parameters:
  - `identity` (std::string): Input parameter.
- Details: Thread-safe: may be called concurrently with encrypt/decrypt. identity Input parameter. Calls: lk(), std::move().

#### `void setAuditLogger(utils::AuditLogger *logger) noexcept`
- Source: `include/timeseries/encrypted_chunk_store.h`:231
- Brief: Replace the audit logger at runtime (nullptr disables auditing).
- Parameters:
  - `logger` (utils::AuditLogger *): n/a
- Details: Thread-safe: may be called concurrently with encrypt/decrypt.

#### `~EncryptedChunkStore()=default`
- Source: `include/timeseries/encrypted_chunk_store.h`:152
- Brief: n/a
- Parameters: none

### themis::FlushController

#### `FlushController(FlushControllerConfig config={})`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:83
- Brief: n/a
- Parameters:
  - `config` (FlushControllerConfig): n/a

#### `bool checkBackpressure(size_t buffered_points, std::chrono::milliseconds timeout=std::chrono::seconds{5})`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:109
- Brief: Notify the controller that buffered_points are queued.
- Parameters:
  - `buffered_points` (size_t): Input parameter.
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: true if the producer may proceed; false if timed out.
- Details: Check Backpressure. If backpressure is active, this call blocks until the queue drains below the low_water_mark or the timeout expires. buffered_points Current number of buffered data points. timeout Maximum time to wait before unblocking. true if the producer may proceed; false if timed out. buffered_points Input parameter. timeout Input parameter. True when the operation succeeds. Calls: lock(), wait_for().

#### `const FlushControllerConfig & config() const noexcept`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:151
- Brief: n/a
- Parameters: none

#### `double ewmaLatencyMs() const noexcept`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:135
- Brief: Return the current EWMA latency in milliseconds.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `bool isBackpressureActive() const noexcept`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:149
- Brief: True when the controller is actively signalling backpressure.
- Parameters: none
- Return: True on success.
- Details: True on success. Exception safety: noexcept.

#### `void notifyDrained(size_t remaining_points)`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:119
- Brief: Signal that the buffer has been partially drained.
- Parameters:
  - `remaining_points` (size_t): Input parameter.
- Details: Notify Drained. Call this from the flush thread after each flush completes. remaining_points Points still in the buffer after the flush. remaining_points Input parameter. Calls: lock(), notify_all().

#### `size_t recommendedBatchSize() const noexcept`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:128
- Brief: Return the current recommended flush batch size.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `void reportFlushLatency(double latency_ms)`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:95
- Brief: Record the measured write latency of a completed flush.
- Parameters:
  - `latency_ms` (double): Input parameter.
- Details: Report Flush Latency. Updates the EWMA and adjusts the recommended batch size. Emits a backpressure metric when the SLO is breached. latency_ms Measured flush write latency in milliseconds. latency_ms Input parameter. Calls: lock(), updateBatchSize(), THEMIS_WARN(), THEMIS_INFO(), notify_all().

#### `FlushControllerStats stats() const noexcept`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:142
- Brief: Return a snapshot of all controller statistics.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `void updateBatchSize()`
- Source: `include/timeseries/ts_auto_buffer_adaptive.h`:169
- Brief: Update Batch Size.
- Parameters: none
- Details: ───────────────────────────────────────────────────────────────────────────── updateBatchSize (mutex_ held by caller) ───────────────────────────────────────────────────────────────────────────── Calls: std::floor(), std::max(), std::ceil(), std::min(), THEMIS_INFO().

### themis::ForwardFillGapFiller

#### `std::vector< TSStore::DataPoint > fill(const std::vector< TSStore::DataPoint > &points, const std::vector< int64_t > &timestamps_to_fill, const GapFillConfig &cfg) const override`
- Source: `include/timeseries/gap_fill.h`:109
- Brief: Fill gaps for the requested timestamps.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Known data points (must be sorted ascending by timestamp_ms).
  - `timestamps_to_fill` (const std::vector< int64_t > &): Timestamps for which a value is required (must be sorted ascending).
  - `cfg` (const GapFillConfig &): Gap-fill configuration.
- Return: One DataPoint per requested timestamp, in ascending timestamp order. Timestamps present in points are returned verbatim; missing timestamps receive a synthesised value.
- Details: points Known data points (must be sorted ascending by timestamp_ms). timestamps_to_fill Timestamps for which a value is required (must be sorted ascending). cfg Gap-fill configuration. One DataPoint per requested timestamp, in ascending timestamp order. Timestamps present in points are returned verbatim; missing timestamps receive a synthesised value.

### themis::GapFiller

#### `GapFiller(GapFillConfig cfg={})`
- Source: `include/timeseries/gap_fill.h`:181
- Brief: n/a
- Parameters:
  - `cfg` (GapFillConfig): n/a

#### `const GapFillConfig & config() const`
- Source: `include/timeseries/gap_fill.h`:213
- Brief: n/a
- Parameters: none

#### `std::vector< TSStore::DataPoint > fill(const std::vector< TSStore::DataPoint > &points, const std::vector< int64_t > &timestamps_to_fill) const`
- Source: `include/timeseries/gap_fill.h`:190
- Brief: Fill gaps using the stored configuration.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Known data points (sorted ascending).
  - `timestamps_to_fill` (const std::vector< int64_t > &): Target timestamps (sorted ascending).
- Return: One DataPoint per requested timestamp.
- Details: points Known data points (sorted ascending). timestamps_to_fill Target timestamps (sorted ascending). One DataPoint per requested timestamp.

#### `std::unique_ptr< IGapFiller > makeImpl(GapFillMethod method)`
- Source: `include/timeseries/gap_fill.h`:224
- Brief: Make Impl.
- Parameters:
  - `method` (GapFillMethod): Input parameter.
- Return: Return value.
- Details: method Input parameter. Return value. method Input parameter. Return value. Implements makeImpl without additional internal calls.

#### `std::vector< int64_t > regularTimestamps(int64_t from_ms, int64_t to_ms, int64_t interval_ms)`
- Source: `include/timeseries/gap_fill.h`:204
- Brief: Generate a regular sequence of timestamps within [from_ms, to_ms].
- Parameters:
  - `from_ms` (int64_t): Input parameter.
  - `to_ms` (int64_t): Input parameter.
  - `interval_ms` (int64_t): Input parameter.
- Return: Sorted vector of timestamps. Returns an empty vector when interval_ms ≤ 0 or from_ms > to_ms.
- Details: Regular Timestamps. from_ms Start timestamp (inclusive). to_ms End timestamp (inclusive). interval_ms Step size between consecutive timestamps. Sorted vector of timestamps. Returns an empty vector when interval_ms ≤ 0 or from_ms > to_ms. from_ms Input parameter. to_ms Input parameter. interval_ms Input parameter. Return value. Calls: reserve(), push_back().

#### `void setConfig(const GapFillConfig &cfg)`
- Source: `include/timeseries/gap_fill.h`:212
- Brief: Set Config.
- Parameters:
  - `cfg` (const GapFillConfig &): Input parameter.
- Details: cfg Input parameter. cfg Input parameter. Calls: makeImpl().

### themis::GorillaDecoder

#### `GorillaDecoder(const std::vector< uint8_t > &data)`
- Source: `include/timeseries/gorilla.h`:263
- Brief: Gorilla Decoder.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value.

#### `size_t decodedCount() const`
- Source: `include/timeseries/gorilla.h`:269
- Brief: Returns total number of successfully decoded points.
- Parameters: none

#### `std::vector< uint8_t > gorilla_strip_header(const std::vector< uint8_t > &data, bool &error_out)`
- Source: `include/timeseries/gorilla.h`:293
- Brief: Strips the 3-byte chunk header if present; sets error_out=true on unsupported version.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `error_out` (bool &): Input/output parameter.
- Return: Return value.
- Details: data Input parameter. error_out Input/output parameter. Return value. Returns the payload (header-stripped or original).

#### `bool hasError() const`
- Source: `include/timeseries/gorilla.h`:267
- Brief: Returns true if a decode error was encountered (truncated/corrupt data).
- Parameters: none

#### `std::optional< std::pair< int64_t, double > > next()`
- Source: `include/timeseries/gorilla.h`:264
- Brief: n/a
- Parameters: none

### themis::GorillaEncoder

#### `void add(int64_t timestamp_ms, double value)`
- Source: `include/timeseries/gorilla.h`:238
- Brief: Add.
- Parameters:
  - `timestamp_ms` (int64_t): Input parameter.
  - `value` (double): Input parameter.
- Details: ---- GorillaEncoder ---- timestamp_ms Input parameter. value Input parameter. timestamp_ms Input parameter. value Input parameter. Calls: writeZigZag64(), writeBits(), dbl_to_bits(), alignToByte(), writeBit(), clz64(), ctz64().

#### `std::vector< uint8_t > finish()`
- Source: `include/timeseries/gorilla.h`:243
- Brief: Finish.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: reserve(), size(), push_back(), insert(), end(), begin().

### themis::GorillaSIMDDecoder

#### `GorillaSIMDDecoder(std::vector< uint8_t > data)`
- Source: `include/timeseries/gorilla_simd.h`:70
- Brief: Gorilla SIMDDecoder.
- Parameters:
  - `data` (std::vector< uint8_t >): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value.

#### `size_t decodeAll(std::vector< std::pair< int64_t, double > > &out)`
- Source: `include/timeseries/gorilla_simd.h`:76
- Brief: n/a
- Parameters:
  - `out` (std::vector< std::pair< int64_t, double > > &): n/a
- Return: Number of newly decoded points appended.
- Details: Decode all points from the chunk and append them to out. Number of newly decoded points appended.

#### `size_t decodedCount() const`
- Source: `include/timeseries/gorilla_simd.h`:82
- Brief: Total number of successfully decoded points so far.
- Parameters: none

#### `bool hasError() const`
- Source: `include/timeseries/gorilla_simd.h`:79
- Brief: True if a decode error (truncated / corrupt data) was encountered.
- Parameters: none

### themis::HeuristicCompressionSelector

#### `HeuristicCompressionSelector()`
- Source: `include/timeseries/compression_selector.h`:141
- Brief: n/a
- Parameters: none

#### `HeuristicCompressionSelector(Config cfg)`
- Source: `include/timeseries/compression_selector.h`:147
- Brief: Heuristic Compression Selector.
- Parameters:
  - `cfg` (Config): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. Return value.

#### `const Config & config() const`
- Source: `include/timeseries/compression_selector.h`:154
- Brief: n/a
- Parameters: none

#### `CompressionStrategy select(const SeriesProfile &profile) const override`
- Source: `include/timeseries/compression_selector.h`:149
- Brief: Select a compression strategy from a pre-computed profile.
- Parameters:
  - `profile` (const SeriesProfile &): Input parameter.
- Return: Return value.
- Details: profile Input parameter. Return value.

#### `CompressionStrategy selectForPoints(const std::vector< TSStore::DataPoint > &points) const override`
- Source: `include/timeseries/compression_selector.h`:151
- Brief: Convenience overload: profile the points internally and select.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Input parameter.
- Return: Return value.
- Details: points Input parameter. Return value.

### themis::Hypertable

#### `Hypertable(Hypertable &&) noexcept=default`
- Source: `include/timeseries/hypertable.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (Hypertable &&): n/a

#### `Hypertable(RocksDBWrapper *db, const Config &config)`
- Source: `include/timeseries/hypertable.h`:76
- Brief: Create or open a hypertable.
- Parameters:
  - `db` (RocksDBWrapper *): Input/output parameter.
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: db Input/output parameter. config Input parameter. Return value.

#### `Hypertable(const Hypertable &)=delete`
- Source: `include/timeseries/hypertable.h`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (const Hypertable &): n/a

#### `std::string buildKey(int64_t timestamp, uint64_t sequence_id)`
- Source: `include/timeseries/hypertable.h`:216
- Brief: Build key for time-series entry.
- Parameters:
  - `timestamp` (int64_t): Input parameter.
  - `sequence_id` (uint64_t): Identifier of the sequence.
- Return: Return value.
- Details: Build Key. timestamp Input parameter. sequence_id Input parameter. Return value. timestamp Input parameter. sequence_id Identifier of the sequence. Return value. Calls: std::setfill(), std::setw(), str().

#### `uint32_t compressOldChunks()`
- Source: `include/timeseries/hypertable.h`:154
- Brief: Compress old chunks (> 7 days) manually.
- Parameters: none
- Return: Number of chunks compressed
- Details: Compress Old Chunks. Number of chunks compressed Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), listChunks(), THEMIS_INFO().

#### `uint32_t dropExpiredChunks()`
- Source: `include/timeseries/hypertable.h`:161
- Brief: Drop expired chunks based on retention policy.
- Parameters: none
- Return: Number of chunks dropped
- Details: Drop Expired Chunks. Number of chunks dropped Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), listChunks(), THEMIS_INFO().

#### `std::vector< ChunkHealth > getChunkHealth()`
- Source: `include/timeseries/hypertable.h`:141
- Brief: Get health and lifecycle status for all chunks.
- Parameters: none
- Return: Return value.
- Details: Get Chunk Health. Returns a health report for each tracked chunk, including status (Active / Frozen / Compressible / Compressed / Expired) and diagnostic messages. Return value. Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), listChunks(), push_back(), THEMIS_INFO(), size().

#### `std::string getChunkName(int64_t timestamp)`
- Source: `include/timeseries/hypertable.h`:196
- Brief: Get chunk name for timestamp.
- Parameters:
  - `timestamp` (int64_t): Input parameter.
- Return: Return value.
- Details: Get Chunk Name. timestamp Input parameter. Return value. timestamp Input parameter. Return value. Calls: str().

#### `const Config & getConfig() const`
- Source: `include/timeseries/hypertable.h`:185
- Brief: Get configuration.
- Parameters: none

#### `rocksdb::ColumnFamilyHandle * getOrCreateChunk(int64_t timestamp)`
- Source: `include/timeseries/hypertable.h`:203
- Brief: Get or create chunk for timestamp.
- Parameters:
  - `timestamp` (int64_t): Input parameter.
- Return: Pointer to the result.
- Details: Get Or Create Chunk. timestamp Input parameter. Pointer to the result. timestamp Input parameter. Pointer to the result. Calls: getChunkName(), getOrCreateColumnFamily(), THEMIS_DEBUG(), THEMIS_ERROR(), error(), message().

#### `Stats getStats()`
- Source: `include/timeseries/hypertable.h`:180
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), listChunks(), size(), THEMIS_INFO().

#### `bool insert(int64_t timestamp, const std::string &data)`
- Source: `include/timeseries/hypertable.h`:92
- Brief: Insert time-series data point.
- Parameters:
  - `timestamp` (int64_t): Input parameter.
  - `data` (const std::string &): Input parameter.
- Return: true if successful
- Details: Insert. timestamp Unix timestamp (seconds or milliseconds) data JSON data to store true if successful timestamp Input parameter. data Input parameter. True when the operation succeeds. Calls: getOrCreateChunk(), fetch_add(), buildKey(), value(), begin(), end(), put(), THEMIS_DEBUG().

#### `bool insertBatch(const std::vector< std::pair< int64_t, std::string > > &batch)`
- Source: `include/timeseries/hypertable.h`:97
- Brief: Batch insert for high throughput.
- Parameters:
  - `batch` (const std::vector< std::pair< int64_t, std::string > > &): n/a

#### `std::vector< ChunkInfo > listChunks()`
- Source: `include/timeseries/hypertable.h`:147
- Brief: Get list of chunks.
- Parameters: none
- Return: Return value.
- Details: List Chunks. Return value. Return value. Calls: THEMIS_INFO(), listColumnFamilies(), rfind(), parseChunkTimeRange(), THEMIS_WARN(), push_back(), std::move(), std::sort().

#### `Hypertable & operator=(Hypertable &&) noexcept=default`
- Source: `include/timeseries/hypertable.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (Hypertable &&): n/a

#### `Hypertable & operator=(const Hypertable &)=delete`
- Source: `include/timeseries/hypertable.h`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (const Hypertable &): n/a

#### `std::pair< int64_t, int64_t > parseChunkTimeRange(const std::string &chunk_name)`
- Source: `include/timeseries/hypertable.h`:208
- Brief: Parse chunk name to get time range.
- Parameters:
  - `chunk_name` (const std::string &): n/a

#### `std::vector< std::pair< int64_t, std::string > > query(int64_t start_time, int64_t end_time)`
- Source: `include/timeseries/hypertable.h`:106
- Brief: Query time range.
- Parameters:
  - `start_time` (int64_t): Start timestamp (inclusive)
  - `end_time` (int64_t): End timestamp (exclusive)
- Return: Vector of (timestamp, data) pairs
- Details: start_time Start timestamp (inclusive) end_time End timestamp (exclusive) Vector of (timestamp, data) pairs

#### `~Hypertable()`
- Source: `include/timeseries/hypertable.h`:77
- Brief: n/a
- Parameters: none

### themis::IAnomalyDetector

#### `std::vector< AnomalyPoint > detect(const std::vector< TSStore::DataPoint > &points, const AnomalyConfig &cfg) const =0`
- Source: `include/timeseries/anomaly_detection.h`:84
- Brief: Detect anomalous data points in points.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Input series (order is preserved; sorting not required).
  - `cfg` (const AnomalyConfig &): Detection configuration.
- Return: Subset of points that are considered anomalous, annotated with the detection score and method name.
- Details: points Input series (order is preserved; sorting not required). cfg Detection configuration. Subset of points that are considered anomalous, annotated with the detection score and method name.

#### `~IAnomalyDetector()=default`
- Source: `include/timeseries/anomaly_detection.h`:74
- Brief: IAnomaly Detector.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::ICompressionSelector

#### `CompressionStrategy select(const SeriesProfile &profile) const =0`
- Source: `include/timeseries/compression_selector.h`:100
- Brief: Select a compression strategy from a pre-computed profile.
- Parameters:
  - `profile` (const SeriesProfile &): Input parameter.
- Return: Return value.
- Details: profile Input parameter. Return value.

#### `CompressionStrategy selectForPoints(const std::vector< TSStore::DataPoint > &points) const =0`
- Source: `include/timeseries/compression_selector.h`:107
- Brief: Convenience overload: profile the points internally and select.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Input parameter.
- Return: Return value.
- Details: points Input parameter. Return value.

#### `~ICompressionSelector()=default`
- Source: `include/timeseries/compression_selector.h`:93
- Brief: ICompression Selector.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::IGapFiller

#### `std::vector< TSStore::DataPoint > fill(const std::vector< TSStore::DataPoint > &points, const std::vector< int64_t > &timestamps_to_fill, const GapFillConfig &cfg) const =0`
- Source: `include/timeseries/gap_fill.h`:90
- Brief: Fill gaps for the requested timestamps.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Known data points (must be sorted ascending by timestamp_ms).
  - `timestamps_to_fill` (const std::vector< int64_t > &): Timestamps for which a value is required (must be sorted ascending).
  - `cfg` (const GapFillConfig &): Gap-fill configuration.
- Return: One DataPoint per requested timestamp, in ascending timestamp order. Timestamps present in points are returned verbatim; missing timestamps receive a synthesised value.
- Details: points Known data points (must be sorted ascending by timestamp_ms). timestamps_to_fill Timestamps for which a value is required (must be sorted ascending). cfg Gap-fill configuration. One DataPoint per requested timestamp, in ascending timestamp order. Timestamps present in points are returned verbatim; missing timestamps receive a synthesised value.

#### `~IGapFiller()=default`
- Source: `include/timeseries/gap_fill.h`:75
- Brief: IGap Filler.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::IQRDetector

#### `std::vector< AnomalyPoint > detect(const std::vector< TSStore::DataPoint > &points, const AnomalyConfig &cfg) const override`
- Source: `include/timeseries/anomaly_detection.h`:134
- Brief: Detect anomalous data points in points.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Input series (order is preserved; sorting not required).
  - `cfg` (const AnomalyConfig &): Detection configuration.
- Return: Subset of points that are considered anomalous, annotated with the detection score and method name.
- Details: points Input series (order is preserved; sorting not required). cfg Detection configuration. Subset of points that are considered anomalous, annotated with the detection score and method name.

### themis::LinearInterpolationGapFiller

#### `std::vector< TSStore::DataPoint > fill(const std::vector< TSStore::DataPoint > &points, const std::vector< int64_t > &timestamps_to_fill, const GapFillConfig &cfg) const override`
- Source: `include/timeseries/gap_fill.h`:133
- Brief: Fill gaps for the requested timestamps.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Known data points (must be sorted ascending by timestamp_ms).
  - `timestamps_to_fill` (const std::vector< int64_t > &): Timestamps for which a value is required (must be sorted ascending).
  - `cfg` (const GapFillConfig &): Gap-fill configuration.
- Return: One DataPoint per requested timestamp, in ascending timestamp order. Timestamps present in points are returned verbatim; missing timestamps receive a synthesised value.
- Details: points Known data points (must be sorted ascending by timestamp_ms). timestamps_to_fill Timestamps for which a value is required (must be sorted ascending). cfg Gap-fill configuration. One DataPoint per requested timestamp, in ascending timestamp order. Timestamps present in points are returned verbatim; missing timestamps receive a synthesised value.

### themis::PerSeriesCompressionRegistry

#### `PerSeriesCompressionRegistry()`
- Source: `include/timeseries/compression_selector.h`:183
- Brief: n/a
- Parameters: none

#### `PerSeriesCompressionRegistry(std::unique_ptr< ICompressionSelector > selector)`
- Source: `include/timeseries/compression_selector.h`:189
- Brief: Per Series Compression Registry.
- Parameters:
  - `selector` (std::unique_ptr< ICompressionSelector >): Input parameter.
- Return: Return value.
- Details: selector Input parameter. Return value.

#### `void clear()`
- Source: `include/timeseries/compression_selector.h`:251
- Brief: Remove all pinned and cached entries.
- Parameters: none
- Details: Clear. Implements clear without additional internal calls.

#### `void clearCache()`
- Source: `include/timeseries/compression_selector.h`:240
- Brief: Discard all cached (non-pinned) selections.
- Parameters: none
- Details: Clear Cache. Calls: clear().

#### `void clearPin(const std::string &metric, const std::string &entity)`
- Source: `include/timeseries/compression_selector.h`:235
- Brief: Remove a previously pinned strategy.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::string &): Input parameter.
- Details: Clear Pin. After removal the next strategyFor() call will re-run the selector. metric Input parameter. entity Input parameter. metric Input parameter. entity Input parameter. Calls: erase(), makeKey().

#### `SeriesKey makeKey(const std::string &metric, const std::string &entity)`
- Source: `include/timeseries/compression_selector.h`:264
- Brief: Make Key.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::string &): Input parameter.
- Return: Return value.
- Details: metric Input parameter. entity Input parameter. Return value.

#### `void pinStrategy(const std::string &metric, const std::string &entity, CompressionStrategy strategy)`
- Source: `include/timeseries/compression_selector.h`:224
- Brief: Force a specific strategy for a series, overriding selection.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::string &): Input parameter.
  - `strategy` (CompressionStrategy): Input parameter.
- Details: Pin Strategy. metric Input parameter. entity Input parameter. strategy Input parameter. metric Input parameter. entity Input parameter. strategy Input parameter. Calls: makeKey().

#### `size_t registrySize() const`
- Source: `include/timeseries/compression_selector.h`:246
- Brief: Total number of entries (pinned + cached).
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void setSelector(std::unique_ptr< ICompressionSelector > selector)`
- Source: `include/timeseries/compression_selector.h`:199
- Brief: Replace the internal selector.
- Parameters:
  - `selector` (std::unique_ptr< ICompressionSelector >): Input parameter.
- Details: Set Selector. Clears the strategy cache so that subsequent strategyFor() calls use the new selector. Pinned entries are not affected. selector Input parameter. selector Input parameter. Calls: std::move(), clear().

#### `CompressionStrategy strategyFor(const std::string &metric, const std::string &entity, const std::vector< TSStore::DataPoint > &sample)`
- Source: `include/timeseries/compression_selector.h`:214
- Brief: Return the compression strategy for a series.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::string &): Input parameter.
  - `sample` (const std::vector< TSStore::DataPoint > &): Input parameter.
- Return: Return value.
- Details: Strategy For. Priority: pinned > cached > freshly-selected (result is then cached). metric Metric name. entity Entity identifier. sample Representative data points used to profile the series when no pinned or cached entry exists. May be empty; in that case the selector receives an empty sample and will typically return None. Return value. metric Input parameter. entity Input parameter. sample Input parameter. Return value. Calls: makeKey(), find(), end(), selectForPoints().

### themis::RetentionManager

#### `RetentionManager(TSStore *store, RetentionPolicy policy)`
- Source: `include/timeseries/retention.h`:83
- Brief: n/a
- Parameters:
  - `store` (TSStore *): n/a
  - `policy` (RetentionPolicy): n/a

#### `RetentionManager(const RetentionManager &)=delete`
- Source: `include/timeseries/retention.h`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RetentionManager &): n/a

#### `size_t apply()`
- Source: `include/timeseries/retention.h`:96
- Brief: Apply retention synchronously via the current retention policy.
- Parameters: none
- Return: Return value.
- Details: Apply. Return value. Return value. Calls: nowMs(), lock(), count(), deleteOldDataForMetric(), fetch_add(), std::to_string(), logAudit().

#### `void asyncLoop()`
- Source: `include/timeseries/retention.h`:218
- Brief: Async Loop.
- Parameters: none
- Details: Calls: load(), lock(), wait_for(), apply(), fetch_add().

#### `void clearAuditLog()`
- Source: `include/timeseries/retention.h`:181
- Brief: Clear the in-memory audit log.
- Parameters: none

#### `std::vector< RetentionAuditEntry > getAuditLog() const`
- Source: `include/timeseries/retention.h`:170
- Brief: n/a
- Parameters: none
- Details: Returns the compliance audit log (last N entries kept in memory).

#### `const RetentionPolicy & getPolicy() const`
- Source: `include/timeseries/retention.h`:127
- Brief: n/a
- Parameters: none

#### `RetentionStats getStats() const`
- Source: `include/timeseries/retention.h`:114
- Brief: Get retention statistics.
- Parameters: none

#### `bool hasStagedDeletion() const`
- Source: `include/timeseries/retention.h`:149
- Brief: Returns true if staged deletion is configured.
- Parameters: none

#### `bool isAsyncRunning() const`
- Source: `include/timeseries/retention.h`:111
- Brief: True if background cleanup is running.
- Parameters: none

#### `void logAudit(const RetentionAuditEntry &entry)`
- Source: `include/timeseries/retention.h`:223
- Brief: Log Audit.
- Parameters:
  - `entry` (const RetentionAuditEntry &): Input parameter.
- Details: entry Input parameter. entry Input parameter. Calls: lock(), size(), erase(), begin(), push_back(), audit_callback_().

#### `RetentionManager & operator=(const RetentionManager &)=delete`
- Source: `include/timeseries/retention.h`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RetentionManager &): n/a

#### `void setAuditCallback(std::function< void(const RetentionAuditEntry &)> cb)`
- Source: `include/timeseries/retention.h`:157
- Brief: n/a
- Parameters:
  - `cb` (std::function< void(const RetentionAuditEntry &)>): n/a
- Details: Register a compliance audit callback. Called synchronously after each retention action.

#### `void setPolicy(RetentionPolicy policy)`
- Source: `include/timeseries/retention.h`:117
- Brief: Update policy (takes effect on next apply()).
- Parameters:
  - `policy` (RetentionPolicy): n/a

#### `void setStagedDeletion(const StagedDeletionPolicy &staged)`
- Source: `include/timeseries/retention.h`:137
- Brief: Set Staged Deletion.
- Parameters:
  - `staged` (const StagedDeletionPolicy &): Input parameter.
- Details: Set staged deletion policy. When set, apply() performs graduated deletion. staged Input parameter. Calls: lock().

#### `void startAsync(std::chrono::seconds interval=std::chrono::hours(1))`
- Source: `include/timeseries/retention.h`:102
- Brief: Start Async.
- Parameters:
  - `interval` (std::chrono::seconds): Input parameter.
- Details: Start background async retention cleanup. interval How often to run cleanup interval Input parameter. Calls: exchange(), std::thread().

#### `void stopAsync()`
- Source: `include/timeseries/retention.h`:108
- Brief: Stop Async.
- Parameters: none
- Details: Stop background async retention cleanup. Calls: exchange(), notify_all(), joinable(), join().

#### `~RetentionManager()`
- Source: `include/timeseries/retention.h`:86
- Brief: n/a
- Parameters: none

### themis::RetentionStats

#### `RetentionStats()=default`
- Source: `include/timeseries/retention.h`:72
- Brief: n/a
- Parameters: none

#### `RetentionStats(const RetentionStats &o)`
- Source: `include/timeseries/retention.h`:73
- Brief: n/a
- Parameters:
  - `o` (const RetentionStats &): n/a

### themis::RollupHierarchy

#### `RollupHierarchy defaultHierarchy(const std::string &metric, const std::optional< std::string > &entity=std::nullopt)`
- Source: `include/timeseries/continuous_agg.h`:51
- Brief: Default Hierarchy.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::optional< std::string > &): Input parameter.
- Return: Return value.
- Details: metric Input parameter. entity Input parameter. Return value. Calls: std::chrono::minutes(), std::chrono::hours().

### themis::TSAutoBuffer

#### `TSAutoBuffer(TSAutoBuffer &&)=delete`
- Source: `include/timeseries/ts_auto_buffer.h`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (TSAutoBuffer &&): n/a

#### `TSAutoBuffer(TSStore *tsstore, TSAutoBufferConfig config=TSAutoBufferConfig{})`
- Source: `include/timeseries/ts_auto_buffer.h`:206
- Brief: Construct auto-batching buffer.
- Parameters:
  - `tsstore` (TSStore *): TSStore instance (not owned)
  - `config` (TSAutoBufferConfig): Buffer configuration
- Details: tsstore TSStore instance (not owned) config Buffer configuration

#### `TSAutoBuffer(const TSAutoBuffer &)=delete`
- Source: `include/timeseries/ts_auto_buffer.h`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TSAutoBuffer &): n/a

#### `Result< void > add(const TSStore::DataPoint &point)`
- Source: `include/timeseries/ts_auto_buffer.h`:247
- Brief: Add a data point (will be buffered).
- Parameters:
  - `point` (const TSStore::DataPoint &): Input parameter.
- Return: Result<void> - success or error
- Details: Add. point Data point to buffer Result<void> - success or error point Input parameter. Return value. Calls: Tracer::startSpan(), setAttribute(), empty(), ErrVoid(), load(), isBackpressure(), THEMIS_WARN(), recordBackpressure().

#### `size_t effectiveBatchSize() const`
- Source: `include/timeseries/ts_auto_buffer.h`:472
- Brief: Returns the effective per-metric flush size (adaptive or configured).
- Parameters: none

#### `size_t flush()`
- Source: `include/timeseries/ts_auto_buffer.h`:253
- Brief: Force immediate flush of all buffered points.
- Parameters: none
- Return: Number of points flushed
- Details: Flush. Number of points flushed Return value. Calls: flushInternal().

#### `size_t flushBuffer(const std::string &buffer_key, MetricBuffer &buffer)`
- Source: `include/timeseries/ts_auto_buffer.h`:459
- Brief: Flush Buffer.
- Parameters:
  - `buffer_key` (const std::string &): Input parameter.
  - `buffer` (MetricBuffer &): Input/output parameter.
- Return: Return value.
- Details: buffer_key Input parameter. buffer Input/output parameter. Return value. buffer_key Input parameter. buffer Input/output parameter. Return value. Calls: empty(), Tracer::startSpan(), setAttribute(), size(), std::chrono::steady_clock::now(), points(), begin(), end().

#### `size_t flushFor(const std::string &metric, const std::string &entity)`
- Source: `include/timeseries/ts_auto_buffer.h`:261
- Brief: Flush buffered points for specific metric:entity.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::string &): Input parameter.
- Return: Number of points flushed
- Details: Flush For. metric Metric name entity Entity ID Number of points flushed metric Input parameter. entity Input parameter. Return value. Calls: makeBufferKey(), lock(), find(), end(), empty(), flushBuffer().

#### `size_t flushInternal(bool lock_held=false)`
- Source: `include/timeseries/ts_auto_buffer.h`:452
- Brief: n/a
- Parameters:
  - `lock_held` (bool): n/a

#### `void flushThread()`
- Source: `include/timeseries/ts_auto_buffer.h`:451
- Brief: Flush Thread.
- Parameters: none
- Details: Calls: THEMIS_INFO(), load(), lock(), wait_for(), shouldFlushGlobal(), unlock(), std::chrono::steady_clock::now(), buf_lock().

#### `const TSAutoBufferConfig & getConfig() const`
- Source: `include/timeseries/ts_auto_buffer.h`:272
- Brief: Get current configuration.
- Parameters: none

#### `TSAutoBufferStats getStats() const`
- Source: `include/timeseries/ts_auto_buffer.h`:267
- Brief: Get current buffer statistics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isRunning() const`
- Source: `include/timeseries/ts_auto_buffer.h`:283
- Brief: Check if buffer is running.
- Parameters: none

#### `std::string makeBufferKey(const std::string &metric, const std::string &entity) const`
- Source: `include/timeseries/ts_auto_buffer.h`:447
- Brief: Helper functions.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::string &): Input parameter.
- Return: Return value.
- Details: metric Input parameter. entity Input parameter. Return value.

#### `TSAutoBuffer & operator=(TSAutoBuffer &&)=delete`
- Source: `include/timeseries/ts_auto_buffer.h`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (TSAutoBuffer &&): n/a

#### `TSAutoBuffer & operator=(const TSAutoBuffer &)=delete`
- Source: `include/timeseries/ts_auto_buffer.h`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TSAutoBuffer &): n/a

#### `size_t persistToWAL(const std::string &wal_path)`
- Source: `include/timeseries/ts_auto_buffer.h`:295
- Brief: Persist To WAL.
- Parameters:
  - `wal_path` (const std::string &): Path to the wal.
- Return: Number of points persisted (0 if buffer is empty)
- Details: ========== WAL Persistence ========== Persist the current in-memory buffer state to a WAL file. This allows crash-recovery: unflushed points can be replayed after restart. wal_path File path where the WAL snapshot is written Number of points persisted (0 if buffer is empty) wal_path Path to the wal. Return value. Calls: lock(), ofs(), is_open(), THEMIS_ERROR(), dump(), THEMIS_INFO().

#### `PushStatus push(const TSStore::DataPoint &point)`
- Source: `include/timeseries/ts_auto_buffer.h`:240
- Brief: Non-blocking single-point push for Gorilla single-point insert buffering.
- Parameters:
  - `point` (const TSStore::DataPoint &): Input parameter.
- Return: PushStatus::OK on success, PushStatus::BUFFER_FULL when buffer is saturated
- Details: Push. Routes single data points through the auto-buffer rather than writing directly to RocksDB. Points accumulate until gorilla_batch_size is reached, at which point they are encoded with Gorilla and written as a single compressed chunk. Unlike add(), this method never blocks producers. When the total in-memory buffer size exceeds config_.max_buffer_bytes it returns BUFFER_FULL so the caller can apply its own backpressure strategy. point Data point to buffer PushStatus::OK on success, PushStatus::BUFFER_FULL when buffer is saturated point Input parameter. Return value. Calls: empty(), makeBufferKey(), lock(), THEMIS_WARN(), add(), fetch_add(), effectiveBatchSize(), size().

#### `bool removeWAL(const std::string &wal_path)`
- Source: `include/timeseries/ts_auto_buffer.h`:316
- Brief: Remove WAL.
- Parameters:
  - `wal_path` (const std::string &): Path to the wal.
- Return: true if file was deleted (or did not exist)
- Details: Delete a WAL file (call after a successful flush to avoid replaying already-flushed data on next startup). wal_path File to remove true if file was deleted (or did not exist) wal_path Path to the wal. True when the operation succeeds. Calls: empty(), std::filesystem::exists(), std::filesystem::remove().

#### `std::ptrdiff_t restoreFromWAL(const std::string &wal_path)`
- Source: `include/timeseries/ts_auto_buffer.h`:306
- Brief: Restore From WAL.
- Parameters:
  - `wal_path` (const std::string &): Path to the wal.
- Return: Number of points restored (-1 on error)
- Details: Restore buffer state from a previously written WAL file. Points are re-enqueued into the buffer; the caller must call flush() or start() to replay them to TSStore. wal_path File path of the WAL snapshot to restore Number of points restored (-1 on error) wal_path Path to the wal. Return value. Calls: ifs(), is_open(), THEMIS_WARN(), std::getline(), empty(), nlohmann::json::parse(), at(), contains().

#### `void setConfig(const TSAutoBufferConfig &config)`
- Source: `include/timeseries/ts_auto_buffer.h`:278
- Brief: Update configuration (takes effect on next flush).
- Parameters:
  - `config` (const TSAutoBufferConfig &): Input parameter.
- Details: Set Config. config Input parameter. config Input parameter. Calls: lock(), reset(), THEMIS_INFO(), count().

#### `bool shouldFlushBuffer(const MetricBuffer &buffer) const`
- Source: `include/timeseries/ts_auto_buffer.h`:465
- Brief: Should Flush Buffer.
- Parameters:
  - `buffer` (const MetricBuffer &): Input parameter.
- Return: True on success.
- Details: buffer Input parameter. True on success.

#### `bool shouldFlushGlobal() const`
- Source: `include/timeseries/ts_auto_buffer.h`:470
- Brief: Should Flush Global.
- Parameters: none
- Return: True on success.
- Details: True on success.

#### `void start()`
- Source: `include/timeseries/ts_auto_buffer.h`:219
- Brief: Start background flush thread.
- Parameters: none
- Details: Start. Calls: exchange(), THEMIS_WARN(), THEMIS_INFO(), count(), std::thread().

#### `void stop()`
- Source: `include/timeseries/ts_auto_buffer.h`:224
- Brief: Stop background flush thread and flush remaining points.
- Parameters: none
- Details: Stop. Calls: exchange(), THEMIS_INFO(), notify_all(), joinable(), join(), flush().

#### `~TSAutoBuffer()`
- Source: `include/timeseries/ts_auto_buffer.h`:208
- Brief: n/a
- Parameters: none

### themis::TSAutoBuffer::FlushController

#### `FlushController(double alpha_, double slo_ms_, size_t batch_min_, size_t batch_max_, size_t initial_batch)`
- Source: `include/timeseries/ts_auto_buffer.h`:377
- Brief: Flush Controller.
- Parameters:
  - `alpha_` (double): Input parameter.
  - `slo_ms_` (double): Input parameter.
  - `batch_min_` (size_t): Input parameter.
  - `batch_max_` (size_t): Input parameter.
  - `initial_batch` (size_t): Input parameter.
- Return: Return value.
- Details: alpha_ Input parameter. slo_ms_ Input parameter. batch_min_ Input parameter. batch_max_ Input parameter. initial_batch Input parameter. Return value.

#### `bool isBackpressure() const`
- Source: `include/timeseries/ts_auto_buffer.h`:412
- Brief: True when EWMA latency exceeds the SLO and backpressure should engage.
- Parameters: none

#### `void updateLatency(double observed_ms)`
- Source: `include/timeseries/ts_auto_buffer.h`:388
- Brief: Feed a new TSStore write latency sample and recompute batch size.
- Parameters:
  - `observed_ms` (double): n/a

### themis::TSAutoBuffer::MetricBuffer

#### `void add(const TSStore::DataPoint &point)`
- Source: `include/timeseries/ts_auto_buffer.h`:330
- Brief: Add.
- Parameters:
  - `point` (const TSStore::DataPoint &): Input parameter.
- Details: point Input parameter. Calls: empty(), std::chrono::steady_clock::now(), push_back(), size(), dump().

#### `void clear()`
- Source: `include/timeseries/ts_auto_buffer.h`:347
- Brief: Clear.
- Parameters: none
- Details: Implements clear without additional internal calls.

### themis::TSAutoBufferStats

#### `TSAutoBufferStats()=default`
- Source: `include/timeseries/ts_auto_buffer.h`:124
- Brief: n/a
- Parameters: none

#### `TSAutoBufferStats(const TSAutoBufferStats &other)`
- Source: `include/timeseries/ts_auto_buffer.h`:126
- Brief: n/a
- Parameters:
  - `other` (const TSAutoBufferStats &): n/a

#### `TSAutoBufferStats & operator=(const TSAutoBufferStats &other)`
- Source: `include/timeseries/ts_auto_buffer.h`:144
- Brief: n/a
- Parameters:
  - `other` (const TSAutoBufferStats &): n/a

### themis::TSQueryOptimizer

#### `TSQueryOptimizer(TSStore *store)`
- Source: `include/timeseries/query_optimizer.h`:136
- Brief: TSQuery Optimizer.
- Parameters:
  - `store` (TSStore *): Input/output parameter.
- Return: Return value.
- Details: store Input/output parameter. Return value.

#### `bool aggregateExists(const std::string &metric, std::chrono::milliseconds window)`
- Source: `include/timeseries/query_optimizer.h`:196
- Brief: Aggregate Exists.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `window` (std::chrono::milliseconds): Input parameter.
- Return: True on success.
- Details: Check if an aggregate exists for a metric with specific window. metric Input parameter. window Input parameter. True on success. metric Input parameter. window Input parameter. True when the operation succeeds. Calls: ContinuousAggregateManager::derivedMetricName(), query(), has_value(), value(), empty().

#### `std::string buildCacheKey(const std::string &metric, const std::optional< std::string > &entity, int64_t from_ms, int64_t to_ms, const OptimizationHint &hint) const`
- Source: `include/timeseries/query_optimizer.h`:343
- Brief: Helpers.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::optional< std::string > &): Input parameter.
  - `from_ms` (int64_t): Input parameter.
  - `to_ms` (int64_t): Input parameter.
  - `hint` (const OptimizationHint &): Input parameter.
- Return: Return value.
- Details: metric Input parameter. entity Input parameter. from_ms Input parameter. to_ms Input parameter. hint Input parameter. Return value.

#### `std::string buildExplanation(const QueryPlan &plan, bool used_agg, size_t raw_points, size_t agg_points) const`
- Source: `include/timeseries/query_optimizer.h`:376
- Brief: Build Explanation.
- Parameters:
  - `plan` (const QueryPlan &): Input parameter.
  - `used_agg` (bool): Input parameter.
  - `raw_points` (size_t): Input parameter.
  - `agg_points` (size_t): Input parameter.
- Return: Return value.
- Details: plan Input parameter. used_agg Input parameter. raw_points Input parameter. agg_points Input parameter. Return value.

#### `uint64_t cacheHits() const`
- Source: `include/timeseries/query_optimizer.h`:232
- Brief: n/a
- Parameters: none
- Details: Returns total cache hits since creation or last reset.

#### `uint64_t cacheMisses() const`
- Source: `include/timeseries/query_optimizer.h`:237
- Brief: n/a
- Parameters: none
- Details: Returns total cache misses since creation or last reset.

#### `size_t cacheSize() const`
- Source: `include/timeseries/query_optimizer.h`:227
- Brief: Cache Size.
- Parameters: none
- Return: Return value.
- Details: Returns the number of plans currently in the cache. Return value.

#### `void clearCache()`
- Source: `include/timeseries/query_optimizer.h`:220
- Brief: Clear Cache.
- Parameters: none
- Details: ========== Query Plan Cache ========== Clear the internal query plan cache. Calls: lock(), clear().

#### `size_t estimateAggregatePointCount(int64_t time_range_ms, std::chrono::milliseconds window) const`
- Source: `include/timeseries/query_optimizer.h`:359
- Brief: Estimate Aggregate Point Count.
- Parameters:
  - `time_range_ms` (int64_t): Input parameter.
  - `window` (std::chrono::milliseconds): Input parameter.
- Return: Return value.
- Details: time_range_ms Input parameter. window Input parameter. Return value.

#### `size_t estimateRawPointCount(int64_t time_range_ms) const`
- Source: `include/timeseries/query_optimizer.h`:352
- Brief: Estimate Raw Point Count.
- Parameters:
  - `time_range_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: time_range_ms Input parameter. Return value.

#### `std::optional< std::string > findBestAggregate(const std::string &metric, int64_t time_range_ms)`
- Source: `include/timeseries/query_optimizer.h`:184
- Brief: Find Best Aggregate.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `time_range_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: Find best available aggregate for a given metric and time range. Searches for aggregates with window sizes: 1m, 5m, 15m, 1h, 6h, 1d Returns aggregate with largest window that fits the query time range. metric Input parameter. time_range_ms Input parameter. Return value. metric Input parameter. time_range_ms Input parameter. Return value. Calls: rbegin(), rend(), count(), ContinuousAggregateManager::derivedMetricName(), aggregateExists(), THEMIS_DEBUG().

#### `std::optional< IndexHint > getIndexHint(const std::string &metric) const`
- Source: `include/timeseries/query_optimizer.h`:276
- Brief: Get Index Hint.
- Parameters:
  - `metric` (const std::string &): Input parameter.
- Return: Return value.
- Details: Retrieve the registered index hint for a metric (if any). metric Input parameter. Return value.

#### `QueryPlan optimizeAggregateQuery(const std::string &metric, const std::optional< std::string > &entity, int64_t from_timestamp_ms, int64_t to_timestamp_ms)`
- Source: `include/timeseries/query_optimizer.h`:167
- Brief: Overload without hint (uses defaults).
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::optional< std::string > &): Input parameter.
  - `from_timestamp_ms` (int64_t): Input parameter.
  - `to_timestamp_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: Optimize Aggregate Query. metric Input parameter. entity Input parameter. from_timestamp_ms Input parameter. to_timestamp_ms Input parameter. Return value. metric Input parameter. entity Input parameter. from_timestamp_ms Input parameter. to_timestamp_ms Input parameter. Return value. Implements optimizeAggregateQuery without additional internal calls.

#### `QueryPlan optimizeAggregateQuery(const std::string &metric, const std::optional< std::string > &entity, int64_t from_timestamp_ms, int64_t to_timestamp_ms, const OptimizationHint &hint)`
- Source: `include/timeseries/query_optimizer.h`:151
- Brief: Optimize Aggregate Query.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::optional< std::string > &): Input parameter.
  - `from_timestamp_ms` (int64_t): Input parameter.
  - `to_timestamp_ms` (int64_t): Input parameter.
  - `hint` (const OptimizationHint &): Input parameter.
- Return: Return value.
- Details: Optimize an aggregate query (min/max/avg/sum/count). Returns optimized query plan that may use pre-computed aggregates. If no suitable aggregate found, returns plan for raw data query. metric Input parameter. entity Input parameter. from_timestamp_ms Input parameter. to_timestamp_ms Input parameter. hint Input parameter. Return value. metric Input parameter. entity Input parameter. from_timestamp_ms Input parameter. to_timestamp_ms Input parameter. hint Input parameter. Return value. Calls: Tracer::startSpan(), setAttribute(), has_value(), buildCacheKey(), lock(), find(), end(), THEMIS_DEBUG().

#### `QueryPlan optimizeWithTiers(const std::string &metric, const std::optional< std::string > &entity, int64_t from_timestamp_ms, int64_t to_timestamp_ms, std::chrono::milliseconds requested_resolution_ms, const OptimizationHint &hint)`
- Source: `include/timeseries/query_optimizer.h`:307
- Brief: Optimise a query taking downsampling tiers into account.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::optional< std::string > &): Input parameter.
  - `from_timestamp_ms` (int64_t): Input parameter.
  - `to_timestamp_ms` (int64_t): Input parameter.
  - `requested_resolution_ms` (std::chrono::milliseconds): Input parameter.
  - `hint` (const OptimizationHint &): Input parameter.
- Return: Return value.
- Details: Optimize With Tiers. Like optimizeAggregateQuery() but additionally consults the registered TierSelector to find the coarsest tier whose resolution is ≤ requested_resolution_ms. If a matching tier exists it is preferred over the standard continuous-aggregate lookup. requested_resolution_ms The finest granularity the caller needs (0 = full resolution, skip tier routing). metric Input parameter. entity Input parameter. from_timestamp_ms Input parameter. to_timestamp_ms Input parameter. hint Input parameter. Return value. metric Input parameter. entity Input parameter. from_timestamp_ms Input parameter. to_timestamp_ms Input parameter. requested_resolution_ms Input parameter. hint Input parameter. Return value.

#### `void registerAvailableAggregate(const std::string &metric, std::chrono::milliseconds window)`
- Source: `include/timeseries/query_optimizer.h`:209
- Brief: Register Available Aggregate.
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `window` (std::chrono::milliseconds): Input parameter.
- Details: Register available aggregates (for caching). Optimizer will check these before querying TSStore. metric Input parameter. window Input parameter. metric Input parameter. window Input parameter. Calls: THEMIS_DEBUG(), count().

#### `void registerIndexHint(IndexHint hint)`
- Source: `include/timeseries/query_optimizer.h`:268
- Brief: Register Index Hint.
- Parameters:
  - `hint` (IndexHint): Input parameter.
- Details: ========== Index-Aware Query Planning ========== Register an index hint for a metric. When the optimizer builds a plan for this metric, it considers the registered index to estimate the effective scan cost. hint Input parameter. hint Input parameter. Calls: lock(), std::move().

#### `void setTierSelector(const TierSelector *selector)`
- Source: `include/timeseries/query_optimizer.h`:288
- Brief: Attach a TierSelector so the optimizer can route queries to downsampling tiers when a suitable resolution is available.
- Parameters:
  - `selector` (const TierSelector *): Input parameter.
- Details: Set Tier Selector. The selector is NOT owned by the optimizer. Pass nullptr to disable tier-based routing. selector Input parameter. selector Input parameter. Implements setTierSelector without additional internal calls.

#### `bool shouldUseAggregate(size_t raw_points, size_t agg_points, const OptimizationHint &hint) const`
- Source: `include/timeseries/query_optimizer.h`:367
- Brief: Should Use Aggregate.
- Parameters:
  - `raw_points` (size_t): Input parameter.
  - `agg_points` (size_t): Input parameter.
  - `hint` (const OptimizationHint &): Input parameter.
- Return: True on success.
- Details: raw_points Input parameter. agg_points Input parameter. hint Input parameter. True on success.

### themis::TSQueryOptimizer::PredicateFilter

#### `PredicateFilter eq(const std::string &key, const std::string &value)`
- Source: `include/timeseries/query_optimizer.h`:98
- Brief: Convenience factory.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. value Input parameter. Return value. Implements eq without additional internal calls.

### themis::TSStore

#### `TSStore(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf, Config config)`
- Source: `include/timeseries/tsstore.h`:219
- Brief: Construct TSStore.
- Parameters:
  - `db` (rocksdb::TransactionDB *): Input/output parameter.
  - `cf` (rocksdb::ColumnFamilyHandle *): Input/output parameter.
  - `config` (Config): Input parameter.
- Return: Return value.
- Details: db RocksDB TransactionDB instance (not owned) cf Optional column family handle (nullptr = default CF) config Compression and storage configuration Main constructor (explicit): accepts DB, optional CF and Config db Input/output parameter. cf Input/output parameter. config Input parameter. Return value.

#### `TSStore(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf=nullptr)`
- Source: `include/timeseries/tsstore.h`:223
- Brief: n/a
- Parameters:
  - `db` (rocksdb::TransactionDB *): n/a
  - `cf` (rocksdb::ColumnFamilyHandle *): n/a

#### `Result< AggregationResult > aggregate(const QueryOptions &options) const`
- Source: `include/timeseries/tsstore.h`:343
- Brief: Compute aggregations over time range.
- Parameters:
  - `options` (const QueryOptions &): Query options
- Return: Result<AggregationResult> - aggregation result or error
- Details: options Query options Result<AggregationResult> - aggregation result or error Automatically uses pre-computed aggregates when available for better performance.

#### `Result< AggregationResult > aggregateOptimized(const QueryOptions &options, bool use_optimizer=true) const`
- Source: `include/timeseries/tsstore.h`:351
- Brief: Compute aggregations with optimizer hints.
- Parameters:
  - `options` (const QueryOptions &): Query options
  - `use_optimizer` (bool): Enable query optimization (default: true)
- Return: Result<AggregationResult> - aggregation result or error
- Details: options Query options use_optimizer Enable query optimization (default: true) Result<AggregationResult> - aggregation result or error

#### `int checkAndUpdateWatermarkLocked(const std::string &wm_key, int64_t timestamp_ms)`
- Source: `include/timeseries/tsstore.h`:541
- Brief: Late-arrival enforcement helper.
- Parameters:
  - `wm_key` (const std::string &): Input parameter.
  - `timestamp_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: Check And Update Watermark Locked. wm_key Input parameter. timestamp_ms Input parameter. Return value. Checks timestamp_ms against the per-series watermark identified by wm_key (format: "{metric}:{entity}") and updates the watermark when the point is newer. Must be called with watermark_mutex_ held. Returns: -1 data point is too old (outside the late-arrival window) – caller must reject 0 data point is in-order or first write – accepted, watermark updated 1 data point is out-of-order but within window – accepted, watermark NOT updated wm_key Input parameter. timestamp_ms Input parameter. Return value. Calls: find(), end().

#### `void clear()`
- Source: `include/timeseries/tsstore.h`:392
- Brief: Clear all time-series data (admin operation).
- Parameters: none
- Details: Clear. Calls: reset(), NewIterator(), Seek(), Valid(), key(), ToString(), compare(), strlen().

#### `Result< void > deleteMetric(const std::string &metric)`
- Source: `include/timeseries/tsstore.h`:387
- Brief: Delete all data for a specific metric.
- Parameters:
  - `metric` (const std::string &): Input parameter.
- Return: Result<void> - success or error
- Details: Delete Metric. metric Metric name Result<void> - success or error metric Input parameter. Return value. Calls: empty(), ErrVoid(), reset(), NewIterator(), Seek(), Valid(), key(), ToString().

#### `size_t deleteOldData(int64_t before_timestamp_ms)`
- Source: `include/timeseries/tsstore.h`:372
- Brief: Delete data older than specified timestamp (retention policy).
- Parameters:
  - `before_timestamp_ms` (int64_t): Input parameter.
- Return: Number of data points deleted
- Details: Delete Old Data. before_timestamp_ms Delete data points with timestamp < this value Number of data points deleted before_timestamp_ms Input parameter. Return value. Calls: std::chrono::steady_clock::now(), reset(), NewIterator(), Seek(), Valid(), key(), ToString(), compare().

#### `size_t deleteOldDataForMetric(const std::string &metric, int64_t before_timestamp_ms)`
- Source: `include/timeseries/tsstore.h`:380
- Brief: Delete old data for a specific metric (retention policy).
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `before_timestamp_ms` (int64_t): Input parameter.
- Return: Number of data points deleted for that metric
- Details: Delete Old Data For Metric. metric Metric name before_timestamp_ms Delete data points with timestamp < this value Number of data points deleted for that metric metric Input parameter. before_timestamp_ms Input parameter. Return value. Calls: empty(), std::chrono::steady_clock::now(), reset(), NewIterator(), Seek(), Valid(), key(), ToString().

#### `Result< void > deleteSystemMeta(const std::string &key)`
- Source: `include/timeseries/tsstore.h`:472
- Brief: Delete a system metadata entry.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Result<void> on success, error on failure
- Details: Delete System Meta. key Logical key to remove Result<void> on success, error on failure key Input parameter. Return value. Calls: std::string(), Delete(), ok(), IsNotFound(), ErrVoid(), fmt::format(), ToString(), OkVoid().

#### `TSAutoBuffer * getAutoBuffer() const`
- Source: `include/timeseries/tsstore.h`:440
- Brief: Return the currently attached TSAutoBuffer, or nullptr if not set.
- Parameters: none

#### `const Config & getConfig() const`
- Source: `include/timeseries/tsstore.h`:230
- Brief: Get current compression configuration.
- Parameters: none

#### `std::shared_ptr< EncryptedChunkStore > getEncryptedChunkStore() const`
- Source: `include/timeseries/tsstore.h`:412
- Brief: Returns the currently attached EncryptedChunkStore (may be null).
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::shared_ptr< TimeSeriesMetrics > getMetrics() const`
- Source: `include/timeseries/tsstore.h`:424
- Brief: Get metrics collector.
- Parameters: none
- Return: Shared pointer to TimeSeriesMetrics instance (may be null)
- Details: Shared pointer to TimeSeriesMetrics instance (may be null)

#### `OutOfOrderStats getOutOfOrderStats() const`
- Source: `include/timeseries/tsstore.h`:365
- Brief: Get out-of-order write statistics.
- Parameters: none
- Return: OutOfOrderStats with counters for accepted and rejected out-of-order points
- Details: OutOfOrderStats with counters for accepted and rejected out-of-order points

#### `Stats getStats() const`
- Source: `include/timeseries/tsstore.h`:359
- Brief: Get time-series statistics.
- Parameters: none
- Return: Stats struct
- Details: Stats struct

#### `Result< std::optional< std::string > > getSystemMeta(const std::string &key) const`
- Source: `include/timeseries/tsstore.h`:465
- Brief: Read a system metadata entry.
- Parameters:
  - `key` (const std::string &): Logical key (same as used in putSystemMeta)
- Return: Result containing the value string, or std::nullopt if not found
- Details: key Logical key (same as used in putSystemMeta) Result containing the value string, or std::nullopt if not found

#### `std::string makeKey(const std::string &metric, const std::string &entity, int64_t timestamp_ms) const`
- Source: `include/timeseries/tsstore.h`:502
- Brief: Key format: "ts:{metric}:{entity}:{timestamp_ms}".
- Parameters:
  - `metric` (const std::string &): Input parameter.
  - `entity` (const std::string &): Input parameter.
  - `timestamp_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: metric Input parameter. entity Input parameter. timestamp_ms Input parameter. Return value.

#### `bool matchesTagFilter(const DataPoint &point, const nlohmann::json &tag_filter) const`
- Source: `include/timeseries/tsstore.h`:532
- Brief: Check if data point matches tag filter.
- Parameters:
  - `point` (const DataPoint &): Input parameter.
  - `tag_filter` (const nlohmann::json &): Input parameter.
- Return: True on success.
- Details: point Input parameter. tag_filter Input parameter. True on success.

#### `Result< KeyComponents > parseKey(const std::string &key) const`
- Source: `include/timeseries/tsstore.h`:524
- Brief: Public Result-based API.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::optional< KeyComponents > parseKeyInternal(const std::string &key) const`
- Source: `include/timeseries/tsstore.h`:517
- Brief: Internal helper that returns std::optional for compatibility.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `Result< BatchWriteResult > putBatch(std::span< const TSRow > rows)`
- Source: `include/timeseries/tsstore.h`:327
- Brief: High-throughput zero-copy batch write using std::span.
- Parameters:
  - `rows` (std::span< const TSRow >): Input parameter.
- Return: Result<BatchWriteResult> — ok() on successful RocksDB write; error() only when the RocksDB Write() itself fails.
- Details: Put Batch. Accepts a span of TSRow values and writes them using a single rocksdb::WriteBatch commit, amortising WAL and memtable overhead across the entire batch. All valid rows are committed atomically; invalid rows (empty metric / entity) are reported in the BatchWriteResult but do not abort the write for the remaining rows. When Gorilla compression is enabled the rows are grouped by metric:entity, sorted by timestamp, Gorilla-encoded, and stored as compressed chunks — identical to putDataPoints() but without the intermediate std::vector allocation at the call site. rows Span of TSRow values. String views must remain valid for the duration of the call. Result<BatchWriteResult> — ok() on successful RocksDB write; error() only when the RocksDB Write() itself fails. Performance target: ≥ 1 M rows/s at p99 < 2 ms on an 8-core host (see ROADMAP — Multi-metric batch write API). rows Input parameter. Return value. Calls: Tracer::startSpan(), setAttribute(), size(), std::chrono::steady_clock::now(), empty(), Ok(), reserve(), emplace_back().

#### `Result< void > putDataPoint(const DataPoint &point)`
- Source: `include/timeseries/tsstore.h`:250
- Brief: Write a data point.
- Parameters:
  - `point` (const DataPoint &): Input parameter.
- Return: Result<void> - success or error with detailed context
- Details: Put Data Point. point Data point to store Result<void> - success or error with detailed context STORAGE METHOD: Singular RocksDB Entity Single data points are always stored as individual RocksDB entities with key format ts:{metric}:{entity}:{timestamp_ms}, regardless of the compression configuration. For Gorilla compression, use putDataPoints() with multiple points for batch compression. point Input parameter. Return value. Calls: Tracer::startSpan(), setAttribute(), std::chrono::steady_clock::now(), empty(), recordError(), ErrVoid(), lock(), checkAndUpdateWatermarkLocked().

#### `Result< void > putDataPoints(const std::vector< DataPoint > &points)`
- Source: `include/timeseries/tsstore.h`:264
- Brief: Write multiple data points (batch operation).
- Parameters:
  - `points` (const std::vector< DataPoint > &): Input parameter.
- Return: Result<void> - success or error with detailed context
- Details: Put Data Points. points Vector of data points Result<void> - success or error with detailed context STORAGE METHOD: Batch with Gorilla Compression (if enabled) When compression is enabled (config.compression = CompressionType::Gorilla), points are grouped by metric:entity, sorted by timestamp, and compressed as chunks with key format tsc:{metric}:{entity}:{first_ts}:{last_ts}. This provides 10-20x compression ratio. Without compression, points are stored as individual entities like putDataPoint(). points Input parameter. Return value. Calls: Tracer::startSpan(), setAttribute(), size(), std::chrono::steady_clock::now(), empty(), OkVoid(), ErrVoid(), push_back().

#### `Result< void > putSystemMeta(const std::string &key, const std::string &value)`
- Source: `include/timeseries/tsstore.h`:458
- Brief: Write a system metadata key-value pair (WAL-durable).
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Return: Result<void> on success, error on failure
- Details: ============================================================ System Metadata (WAL-durable key-value store for bookkeeping) ============================================================ Used internally for persisting small pieces of bookkeeping data (e.g., continuous-aggregate watermarks) in the same RocksDB instance without mixing them with time-series payload keys. Keys are stored under the "sys:" prefix and must not begin with that prefix when passed here (it is added automatically). key Logical key (e.g., "wm:cagg:my_aggregate") value Value string to store Result<void> on success, error on failure key Input parameter. value Input parameter. Return value. Calls: std::string(), Put(), ok(), ErrVoid(), fmt::format(), ToString(), OkVoid().

#### `Result< std::vector< DataPoint > > query(const QueryOptions &options) const`
- Source: `include/timeseries/tsstore.h`:334
- Brief: Query data points with filters.
- Parameters:
  - `options` (const QueryOptions &): Query options (time range, entity, tags)
- Return: Result<std::vector<DataPoint>> - data points or error
- Details: options Query options (time range, entity, tags) Result<std::vector<DataPoint>> - data points or error

#### `void setAutoBuffer(TSAutoBuffer *buf)`
- Source: `include/timeseries/tsstore.h`:435
- Brief: Wire a TSAutoBuffer to receive single-point inserts when Gorilla compression is enabled. When set, putDataPoint() routes through the buffer instead of writing directly to RocksDB, enabling Gorilla compression for IoT / streaming workloads.
- Parameters:
  - `buf` (TSAutoBuffer *): Pointer to a TSAutoBuffer (not owned, must outlive this TSStore). Pass nullptr to disable buffering and fall back to direct writes.
- Details: buf Pointer to a TSAutoBuffer (not owned, must outlive this TSStore). Pass nullptr to disable buffering and fall back to direct writes. Implements setAutoBuffer without additional internal calls.

#### `void setConfig(const Config &config)`
- Source: `include/timeseries/tsstore.h`:237
- Brief: Update compression configuration.
- Parameters:
  - `config` (const Config &): Input parameter.
- Details: Changes only affect new data points; existing data remains unchanged config Input parameter.

#### `void setEncryptedChunkStore(std::shared_ptr< EncryptedChunkStore > enc_store)`
- Source: `include/timeseries/tsstore.h`:406
- Brief: Attach an EncryptedChunkStore to enable AES-256-GCM encryption.
- Parameters:
  - `enc_store` (std::shared_ptr< EncryptedChunkStore >): Input parameter.
- Details: Set Encrypted Chunk Store. When set, all new Gorilla-compressed chunk writes are encrypted (compress-then-encrypt) and existing encrypted chunks are decrypted transparently on read. Pass nullptr to disable encryption. Key access is audited via the AuditLogger that was configured on the provided EncryptedChunkStore. enc_store Shared EncryptedChunkStore, or nullptr to disable. enc_store Input parameter. Calls: std::move().

#### `void setMetrics(std::shared_ptr< TimeSeriesMetrics > metrics)`
- Source: `include/timeseries/tsstore.h`:418
- Brief: Set metrics collector for monitoring.
- Parameters:
  - `metrics` (std::shared_ptr< TimeSeriesMetrics >): Input parameter.
- Details: Set Metrics. metrics Shared pointer to TimeSeriesMetrics instance metrics Input parameter. Implements setMetrics without additional internal calls.

#### `~TSStore()=default`
- Source: `include/timeseries/tsstore.h`:225
- Brief: n/a
- Parameters: none

### themis::TSStore::BatchWriteResult

#### `bool all_ok() const noexcept`
- Source: `include/timeseries/tsstore.h`:302
- Brief: n/a
- Parameters: none

### themis::TSStore::DataPoint

#### `DataPoint fromJson(const nlohmann::json &j)`
- Source: `include/timeseries/tsstore.h`:168
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: value(), int64_t(), nlohmann::json::object().

#### `nlohmann::json toJson() const`
- Source: `include/timeseries/tsstore.h`:162
- Brief: Serialization.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::TierSelector

#### `void registerPolicy(const DownsamplingPolicy &policy)`
- Source: `include/timeseries/downsampling.h`:96
- Brief: Register a downsampling policy so the selector knows which tiers exist.
- Parameters:
  - `policy` (const DownsamplingPolicy &): Retention policy definition to store.
- Details: Register a retention policy. policy Input parameter. policy Retention policy definition to store. Implements registerPolicy without additional internal calls.

#### `std::optional< std::string > selectTier(const std::string &metric, std::chrono::milliseconds requested_resolution) const`
- Source: `include/timeseries/downsampling.h`:107
- Brief: Find the best tier for the given metric and requested resolution.
- Parameters:
  - `metric` (const std::string &): Source metric name
  - `requested_resolution` (std::chrono::milliseconds): The finest granularity the query needs. Pass std::chrono::milliseconds{0} to request raw data.
- Return: The name of the selected tier's derived metric in TSStore, or std::nullopt if no registered tier fits (caller should use raw data).
- Details: metric Source metric name requested_resolution The finest granularity the query needs. Pass std::chrono::milliseconds{0} to request raw data. The name of the selected tier's derived metric in TSStore, or std::nullopt if no registered tier fits (caller should use raw data).

#### `std::vector< DownsamplingTier > tiersFor(const std::string &metric) const`
- Source: `include/timeseries/downsampling.h`:116
- Brief: Returns all registered tiers for a metric, ordered finest→coarsest.
- Parameters:
  - `metric` (const std::string &): Input parameter.
- Return: Return value.
- Details: metric Input parameter. Return value.

### themis::TimeSeriesAggregates

#### `TimeSeriesAggregates(TimeSeriesAggregates &&) noexcept=default`
- Source: `include/timeseries/aggregates.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesAggregates &&): n/a

#### `TimeSeriesAggregates(const Config &config)`
- Source: `include/timeseries/aggregates.h`:77
- Brief: Time Series Aggregates.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `TimeSeriesAggregates(const TimeSeriesAggregates &)=delete`
- Source: `include/timeseries/aggregates.h`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TimeSeriesAggregates &): n/a

#### `AggregateResult aggregate(const int64_t *timestamps, const double *values, size_t count, const TimeWindow &window, AggregateFunction func)`
- Source: `include/timeseries/aggregates.h`:95
- Brief: Compute aggregates over time windows.
- Parameters:
  - `timestamps` (const int64_t *): Input parameter.
  - `values` (const double *): Input parameter.
  - `count` (size_t): Input parameter.
  - `window` (const TimeWindow &): Input parameter.
  - `func` (AggregateFunction): Input parameter.
- Return: Aggregated results per window
- Details: Aggregate. timestamps Array of timestamps values Array of values count Number of data points window Time window specification func Aggregate function Aggregated results per window timestamps Input parameter. values Input parameter. count Input parameter. window Input parameter. func Input parameter. Return value. Calls: push_back(), applyAggregate(), data(), size(), THEMIS_INFO().

#### `double applyAggregate(const double *values, size_t count, AggregateFunction func)`
- Source: `include/timeseries/aggregates.h`:156
- Brief: Apply aggregate function to window.
- Parameters:
  - `values` (const double *): Input parameter.
  - `count` (size_t): Input parameter.
  - `func` (AggregateFunction): Input parameter.
- Return: Return value.
- Details: Apply Aggregate. values Input parameter. count Input parameter. func Input parameter. Return value. values Input parameter. count Input parameter. func Input parameter. Return value. Calls: std::accumulate(), std::min_element(), std::max_element(), std::sqrt(), computePercentile().

#### `double computePercentile(const double *values, size_t count, double percentile)`
- Source: `include/timeseries/aggregates.h`:165
- Brief: Compute percentile.
- Parameters:
  - `values` (const double *): Input parameter.
  - `count` (size_t): Input parameter.
  - `percentile` (double): Input parameter.
- Return: Return value.
- Details: Compute Percentile. values Input parameter. count Input parameter. percentile Input parameter. Return value. values Input parameter. count Input parameter. percentile Input parameter. Return value. Calls: sorted(), std::sort(), begin(), end(), std::floor(), std::ceil().

#### `const Config & getConfig() const`
- Source: `include/timeseries/aggregates.h`:144
- Brief: Get configuration.
- Parameters: none

#### `TimeSeriesAggregates & operator=(TimeSeriesAggregates &&) noexcept=default`
- Source: `include/timeseries/aggregates.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeSeriesAggregates &&): n/a

#### `TimeSeriesAggregates & operator=(const TimeSeriesAggregates &)=delete`
- Source: `include/timeseries/aggregates.h`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TimeSeriesAggregates &): n/a

#### `AggregateResult resample(const int64_t *timestamps, const double *values, size_t count, int64_t new_interval_seconds, AggregateFunction func)`
- Source: `include/timeseries/aggregates.h`:114
- Brief: Resample time series to different interval.
- Parameters:
  - `timestamps` (const int64_t *): Input parameter.
  - `values` (const double *): Input parameter.
  - `count` (size_t): Input parameter.
  - `new_interval_seconds` (int64_t): Input parameter.
  - `func` (AggregateFunction): Input parameter.
- Return: Return value.
- Details: Resample. Downsampling example: 1-second data → 1-minute aggregates timestamps Input parameter. values Input parameter. count Input parameter. new_interval_seconds Input parameter. func Input parameter. Return value. timestamps Input parameter. values Input parameter. count Input parameter. new_interval_seconds Input parameter. func Input parameter. Return value. Calls: std::min_element(), std::max_element(), aggregate(), THEMIS_INFO().

#### `AggregateResult rollingWindow(const int64_t *timestamps, const double *values, size_t count, int64_t window_size_seconds, AggregateFunction func)`
- Source: `include/timeseries/aggregates.h`:133
- Brief: Rolling window aggregates.
- Parameters:
  - `timestamps` (const int64_t *): Input parameter.
  - `values` (const double *): Input parameter.
  - `count` (size_t): Input parameter.
  - `window_size_seconds` (int64_t): Input parameter.
  - `func` (AggregateFunction): Input parameter.
- Return: Return value.
- Details: Rolling Window. Example: 5-minute moving average timestamps Input parameter. values Input parameter. count Input parameter. window_size_seconds Input parameter. func Input parameter. Return value. timestamps Input parameter. values Input parameter. count Input parameter. window_size_seconds Input parameter. func Input parameter. Return value. Calls: push_back(), empty(), applyAggregate(), data(), size(), THEMIS_INFO().

#### `~TimeSeriesAggregates()`
- Source: `include/timeseries/aggregates.h`:78
- Brief: n/a
- Parameters: none

### themis::TimeSeriesMetrics

#### `TimeSeriesMetrics()`
- Source: `include/timeseries/timeseries_metrics.h`:45
- Brief: n/a
- Parameters: none

#### `TimeSeriesMetrics(const Config &config)`
- Source: `include/timeseries/timeseries_metrics.h`:51
- Brief: Time Series Metrics.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `std::string exportJson() const`
- Source: `include/timeseries/timeseries_metrics.h`:197
- Brief: Export metrics as JSON.
- Parameters: none
- Return: String containing all metrics in JSON format
- Details: String containing all metrics in JSON format

#### `std::string exportPrometheus() const`
- Source: `include/timeseries/timeseries_metrics.h`:191
- Brief: Export metrics in Prometheus text format.
- Parameters: none
- Return: String containing all metrics in Prometheus format
- Details: String containing all metrics in Prometheus format

#### `std::string formatPrometheusMetric(const std::string &name, const std::string &type, const std::string &help, double value) const`
- Source: `include/timeseries/timeseries_metrics.h`:343
- Brief: Format Prometheus Metric.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `type` (const std::string &): Input parameter.
  - `help` (const std::string &): Input parameter.
  - `value` (double): Input parameter.
- Return: Return value.
- Details: name Input parameter. type Input parameter. help Input parameter. value Input parameter. Return value.

#### `std::string formatPrometheusMetric(const std::string &name, const std::string &type, const std::string &help, uint64_t value) const`
- Source: `include/timeseries/timeseries_metrics.h`:333
- Brief: Format Prometheus Metric.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `type` (const std::string &): Input parameter.
  - `help` (const std::string &): Input parameter.
  - `value` (uint64_t): Input parameter.
- Return: Return value.
- Details: name Input parameter. type Input parameter. help Input parameter. value Input parameter. Return value.

#### `double getAggRefreshLag(const std::string &agg_id) const`
- Source: `include/timeseries/timeseries_metrics.h`:245
- Brief: Get last recorded lag for a specific aggregate (for testing).
- Parameters:
  - `agg_id` (const std::string &): Input parameter.
- Return: Last lag in ms, or -1 if no data recorded for agg_id.
- Details: Last lag in ms, or -1 if no data recorded for agg_id. agg_id Input parameter.

#### `double getAggRefreshLatency(const std::string &agg_id) const`
- Source: `include/timeseries/timeseries_metrics.h`:238
- Brief: Get average refresh latency for a specific aggregate (for testing).
- Parameters:
  - `agg_id` (const std::string &): Input parameter.
- Return: Average latency in ms, or -1 if no data recorded for agg_id.
- Details: Average latency in ms, or -1 if no data recorded for agg_id. agg_id Input parameter.

#### `double getAverageCompressionRatio() const`
- Source: `include/timeseries/timeseries_metrics.h`:231
- Brief: Get Average Compression Ratio.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getAverageLatency(double total_latency, uint64_t count) const`
- Source: `include/timeseries/timeseries_metrics.h`:324
- Brief: Get Average Latency.
- Parameters:
  - `total_latency` (double): Input parameter.
  - `count` (uint64_t): Input parameter.
- Return: Return value.
- Details: total_latency Input parameter. count Input parameter. Return value.

#### `double getAverageQueryLatency() const`
- Source: `include/timeseries/timeseries_metrics.h`:226
- Brief: Get Average Query Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `double getAverageWriteLatency() const`
- Source: `include/timeseries/timeseries_metrics.h`:221
- Brief: Get Average Write Latency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t getLateArrivalRejected() const`
- Source: `include/timeseries/timeseries_metrics.h`:213
- Brief: n/a
- Parameters: none

#### `uint64_t getOptimizerHits() const`
- Source: `include/timeseries/timeseries_metrics.h`:210
- Brief: n/a
- Parameters: none

#### `uint64_t getOptimizerMisses() const`
- Source: `include/timeseries/timeseries_metrics.h`:211
- Brief: n/a
- Parameters: none

#### `uint64_t getOutOfOrderAccepted() const`
- Source: `include/timeseries/timeseries_metrics.h`:212
- Brief: n/a
- Parameters: none

#### `uint64_t getTotalAggregationsExecuted() const`
- Source: `include/timeseries/timeseries_metrics.h`:209
- Brief: n/a
- Parameters: none

#### `uint64_t getTotalBackpressureEvents() const`
- Source: `include/timeseries/timeseries_metrics.h`:214
- Brief: n/a
- Parameters: none

#### `uint64_t getTotalBatchesWritten() const`
- Source: `include/timeseries/timeseries_metrics.h`:207
- Brief: n/a
- Parameters: none

#### `uint64_t getTotalDataPointsWritten() const`
- Source: `include/timeseries/timeseries_metrics.h`:206
- Brief: n/a
- Parameters: none

#### `uint64_t getTotalOverdueFlushEvents() const`
- Source: `include/timeseries/timeseries_metrics.h`:215
- Brief: n/a
- Parameters: none

#### `uint64_t getTotalQueriesExecuted() const`
- Source: `include/timeseries/timeseries_metrics.h`:208
- Brief: n/a
- Parameters: none

#### `void recordAggRefreshLag(const std::string &agg_id, double lag_ms)`
- Source: `include/timeseries/timeseries_metrics.h`:183
- Brief: Record the lag between the aggregate watermark and wall-clock now.
- Parameters:
  - `agg_id` (const std::string &): Identifier of the agg.
  - `lag_ms` (double): Input parameter.
- Details: Record Agg Refresh Lag. Lag = now_ms – watermark_ms. A growing lag indicates that the scheduler is falling behind ingestion rate. agg_id Unique aggregate identifier lag_ms Lag in milliseconds (0 if fully caught up) agg_id Identifier of the agg. lag_ms Input parameter. Calls: lock().

#### `void recordAggRefreshLatency(const std::string &agg_id, double latency_ms)`
- Source: `include/timeseries/timeseries_metrics.h`:172
- Brief: Record incremental aggregate refresh latency tagged with aggregate ID.
- Parameters:
  - `agg_id` (const std::string &): Identifier of the agg.
  - `latency_ms` (double): Input parameter.
- Details: Record Agg Refresh Latency. This complements recordContinuousAggregateRefresh() with per-aggregate granularity so operators can distinguish slow aggregates. agg_id Unique aggregate identifier (e.g., "cpu:s1:60000ms") latency_ms Refresh duration in milliseconds agg_id Identifier of the agg. latency_ms Input parameter. Calls: fetch_add(), lock().

#### `void recordAggregation(const std::string &metric_name, double latency_ms, size_t data_points_scanned, bool optimizer_used)`
- Source: `include/timeseries/timeseries_metrics.h`:107
- Brief: Record an aggregation operation.
- Parameters:
  - `metric_name` (const std::string &): Name of the metric
  - `latency_ms` (double): Input parameter.
  - `data_points_scanned` (size_t): Number of data points scanned
  - `optimizer_used` (bool): Input parameter.
- Details: Record Aggregation. metric_name Name of the metric latency_ms Aggregation latency in milliseconds data_points_scanned Number of data points scanned optimizer_used Whether query optimizer was used param Input parameter. latency_ms Input parameter. size_t Input parameter. optimizer_used Input parameter. Calls: fetch_add(), lock(), recordLatency().

#### `void recordBackpressure(const std::string &metric_name="")`
- Source: `include/timeseries/timeseries_metrics.h`:119
- Brief: Record a backpressure event from TSAutoBuffer.
- Parameters:
  - `metric_name` (const std::string &): Name of the metric triggering backpressure (empty = global)
- Details: Record Backpressure. metric_name Name of the metric triggering backpressure (empty = global) param Input parameter. Calls: fetch_add().

#### `void recordBatchWrite(size_t num_points, double latency_ms, bool compressed, bool success=true)`
- Source: `include/timeseries/timeseries_metrics.h`:79
- Brief: Record a batch write operation.
- Parameters:
  - `num_points` (size_t): Input parameter.
  - `latency_ms` (double): Input parameter.
  - `compressed` (bool): Input parameter.
  - `success` (bool): Input parameter.
- Details: Record Batch Write. num_points Number of data points in batch latency_ms Operation latency in milliseconds compressed Whether compression was used success Whether the operation succeeded num_points Input parameter. latency_ms Input parameter. compressed Input parameter. success Input parameter. Calls: fetch_add(), lock(), recordLatency().

#### `void recordCompression(const std::string &metric_name, size_t uncompressed_bytes, size_t compressed_bytes)`
- Source: `include/timeseries/timeseries_metrics.h`:87
- Brief: Record compression statistics.
- Parameters:
  - `metric_name` (const std::string &): Name of the metric.
  - `uncompressed_bytes` (size_t): Input parameter.
  - `compressed_bytes` (size_t): Input parameter.
- Details: Record Compression. metric_name Name of the metric uncompressed_bytes Size before compression compressed_bytes Size after compression metric_name Name of the metric. uncompressed_bytes Input parameter. compressed_bytes Input parameter. Calls: fetch_add(), empty(), lock().

#### `void recordContinuousAggregateRefresh(const std::string &metric_name, int64_t window_ms, double latency_ms, size_t points_processed)`
- Source: `include/timeseries/timeseries_metrics.h`:160
- Brief: Record continuous aggregate refresh.
- Parameters:
  - `metric_name` (const std::string &): Source metric name
  - `window_ms` (int64_t): Aggregation window in milliseconds
  - `latency_ms` (double): Refresh latency in milliseconds
  - `points_processed` (size_t): Input parameter.
- Details: Record Continuous Aggregate Refresh. metric_name Source metric name window_ms Aggregation window in milliseconds latency_ms Refresh latency in milliseconds points_processed Number of points processed param Input parameter. int64_t Input parameter. double Input parameter. points_processed Input parameter. Calls: fetch_add().

#### `void recordDataPointWrite(const std::string &metric_name, double latency_ms, bool success=true)`
- Source: `include/timeseries/timeseries_metrics.h`:62
- Brief: Record a data point write operation.
- Parameters:
  - `metric_name` (const std::string &): Name of the metric.
  - `latency_ms` (double): Input parameter.
  - `success` (bool): Input parameter.
- Details: Record Data Point Write. metric_name Name of the metric latency_ms Operation latency in milliseconds success Whether the operation succeeded metric_name Name of the metric. latency_ms Input parameter. success Input parameter. Calls: fetch_add(), lock(), recordLatency(), empty().

#### `void recordLatency(double &total_latency, uint64_t &count, double latency_ms)`
- Source: `include/timeseries/timeseries_metrics.h`:317
- Brief: Helper methods.
- Parameters:
  - `total_latency` (double &): Input/output parameter.
  - `count` (uint64_t &): Input/output parameter.
  - `latency_ms` (double): Input parameter.
- Details: Record Latency. total_latency Input/output parameter. count Input/output parameter. latency_ms Input parameter. total_latency Input/output parameter. count Input/output parameter. latency_ms Input parameter. Implements recordLatency without additional internal calls.

#### `void recordOptimizerResult(bool hit)`
- Source: `include/timeseries/timeseries_metrics.h`:113
- Brief: Record query optimizer hit/miss.
- Parameters:
  - `hit` (bool): Input parameter.
- Details: Record Optimizer Result. hit Whether optimizer could optimize the query hit Input parameter. Calls: fetch_add().

#### `void recordOutOfOrderWrite(const std::string &metric_name, bool rejected)`
- Source: `include/timeseries/timeseries_metrics.h`:70
- Brief: Record an out-of-order data point write.
- Parameters:
  - `metric_name` (const std::string &): Name of the metric
  - `rejected` (bool): Input parameter.
- Details: Record Out Of Order Write. metric_name Name of the metric rejected true if the point was rejected (outside late-arrival window); false if it was accepted (within window but out-of-order) param Input parameter. rejected Input parameter. Calls: fetch_add().

#### `void recordOverdueFlush(const std::string &metric_name="", double age_ms=0.0)`
- Source: `include/timeseries/timeseries_metrics.h`:131
- Brief: Record an overdue flush event from TSAutoBuffer.
- Parameters:
  - `metric_name` (const std::string &): Name of the metric with overdue data (empty = global)
  - `age_ms` (double): Age of the oldest buffered point in milliseconds
- Details: Record Overdue Flush. An overdue flush occurs when buffered data is older than the configured flush interval but could not be written because TSStore returned errors on the previous flush attempt. metric_name Name of the metric with overdue data (empty = global) age_ms Age of the oldest buffered point in milliseconds param Input parameter. double Input parameter. Calls: fetch_add().

#### `void recordQuery(const std::string &metric_name, double latency_ms, size_t result_count, int64_t time_range_ms)`
- Source: `include/timeseries/timeseries_metrics.h`:98
- Brief: Record a query operation.
- Parameters:
  - `metric_name` (const std::string &): Name of the metric.
  - `latency_ms` (double): Input parameter.
  - `result_count` (size_t): Input parameter.
  - `time_range_ms` (int64_t): Time range covered by query
- Details: Record Query. metric_name Name of the metric latency_ms Query latency in milliseconds result_count Number of data points returned time_range_ms Time range covered by query metric_name Name of the metric. latency_ms Input parameter. result_count Input parameter. int64_t Input parameter. Calls: fetch_add(), lock(), recordLatency(), empty().

#### `void recordRetention(const std::string &metric_name, size_t deleted_points, double latency_ms)`
- Source: `include/timeseries/timeseries_metrics.h`:149
- Brief: Record a retention operation.
- Parameters:
  - `metric_name` (const std::string &): Name of the metric (empty for global)
  - `deleted_points` (size_t): Input parameter.
  - `latency_ms` (double): Operation latency in milliseconds
- Details: Record Retention. metric_name Name of the metric (empty for global) deleted_points Number of data points deleted latency_ms Operation latency in milliseconds param Input parameter. deleted_points Input parameter. double Input parameter. Calls: fetch_add().

#### `void reset()`
- Source: `include/timeseries/timeseries_metrics.h`:202
- Brief: Reset all metrics (for testing).
- Parameters: none
- Details: Reset the modification detection flag. Calls: store(), lock(), clear().

#### `void updateStorageStats(size_t total_data_points, size_t total_metrics, size_t total_size_bytes)`
- Source: `include/timeseries/timeseries_metrics.h`:141
- Brief: Update storage statistics.
- Parameters:
  - `total_data_points` (size_t): Input parameter.
  - `total_metrics` (size_t): Input parameter.
  - `total_size_bytes` (size_t): Input parameter.
- Details: Update Storage Stats. total_data_points Total number of data points total_metrics Number of unique metrics total_size_bytes Total storage size in bytes total_data_points Input parameter. total_metrics Input parameter. total_size_bytes Input parameter. Calls: store().

#### `~TimeSeriesMetrics()=default`
- Source: `include/timeseries/timeseries_metrics.h`:52
- Brief: n/a
- Parameters: none

### themis::TimeSeriesStore

#### `TimeSeriesStore(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf=nullptr)`
- Source: `include/timeseries/timeseries.h`:95
- Brief: Construct TimeSeriesStore.
- Parameters:
  - `db` (rocksdb::TransactionDB *): RocksDB TransactionDB instance (not owned)
  - `cf` (rocksdb::ColumnFamilyHandle *): Optional column family handle (nullptr = default CF)
- Details: db RocksDB TransactionDB instance (not owned) cf Optional column family handle (nullptr = default CF)

#### `Aggregation aggregate(std::string_view metric, std::string_view entity) const`
- Source: `include/timeseries/timeseries.h`:137
- Brief: Aggregate data points in time range (all data).
- Parameters:
  - `metric` (std::string_view): Metric name
  - `entity` (std::string_view): Entity ID
- Return: Aggregation result
- Details: metric Metric name entity Entity ID Aggregation result

#### `Aggregation aggregate(std::string_view metric, std::string_view entity, const RangeQuery &query) const`
- Source: `include/timeseries/timeseries.h`:147
- Brief: Aggregate data points in time range.
- Parameters:
  - `metric` (std::string_view): Metric name
  - `entity` (std::string_view): Entity ID
  - `query` (const RangeQuery &): Range query parameters
- Return: Aggregation result
- Details: metric Metric name entity Entity ID query Range query parameters Aggregation result

#### `size_t deleteOldPoints(std::string_view metric, std::string_view entity, int64_t before_ms)`
- Source: `include/timeseries/timeseries.h`:158
- Brief: Delete old data points (retention policy).
- Parameters:
  - `metric` (std::string_view): Input parameter.
  - `entity` (std::string_view): Input parameter.
  - `before_ms` (int64_t): Input parameter.
- Return: Number of points deleted
- Details: Delete Old Points. metric Metric name entity Entity ID before_ms Delete all points older than this timestamp Number of points deleted metric Input parameter. entity Input parameter. before_ms Input parameter. Return value. Calls: resolveColumnFamily(), THEMIS_ERROR(), makePrefix(), makeKey(), it(), NewIterator(), Seek(), Valid().

#### `std::optional< DataPoint > getLatest(std::string_view metric, std::string_view entity) const`
- Source: `include/timeseries/timeseries.h`:168
- Brief: Get latest value for metric/entity.
- Parameters:
  - `metric` (std::string_view): Metric name
  - `entity` (std::string_view): Entity ID
- Return: Latest data point, or nullopt if not found
- Details: metric Metric name entity Entity ID Latest data point, or nullopt if not found

#### `std::string makeKey(std::string_view metric, std::string_view entity, int64_t timestamp_ms) const`
- Source: `include/timeseries/timeseries.h`:190
- Brief: Make Key.
- Parameters:
  - `metric` (std::string_view): Input parameter.
  - `entity` (std::string_view): Input parameter.
  - `timestamp_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: metric Input parameter. entity Input parameter. timestamp_ms Input parameter. Return value.

#### `std::string makePrefix(std::string_view metric, std::string_view entity) const`
- Source: `include/timeseries/timeseries.h`:200
- Brief: Make Prefix.
- Parameters:
  - `metric` (std::string_view): Input parameter.
  - `entity` (std::string_view): Input parameter.
- Return: Return value.
- Details: metric Input parameter. entity Input parameter. Return value.

#### `bool put(std::string_view metric, std::string_view entity, const DataPoint &point)`
- Source: `include/timeseries/timeseries.h`:107
- Brief: Put a data point.
- Parameters:
  - `metric` (std::string_view): Input parameter.
  - `entity` (std::string_view): Input parameter.
  - `point` (const DataPoint &): Input parameter.
- Return: true on success
- Details: Put. metric Metric name (e.g., "cpu_usage", "request_count") entity Entity ID (e.g., "server-1", "user-123") point Data point with timestamp and value true on success metric Input parameter. entity Input parameter. point Input parameter. True when the operation succeeds. Calls: THEMIS_ERROR(), resolveColumnFamily(), makeKey(), toJson(), dump(), Put(), ok(), ToString().

#### `std::vector< DataPoint > query(std::string_view metric, std::string_view entity) const`
- Source: `include/timeseries/timeseries.h`:117
- Brief: Query data points in time range (all data).
- Parameters:
  - `metric` (std::string_view): Metric name
  - `entity` (std::string_view): Entity ID
- Return: Vector of data points
- Details: metric Metric name entity Entity ID Vector of data points

#### `std::vector< DataPoint > query(std::string_view metric, std::string_view entity, const RangeQuery &query) const`
- Source: `include/timeseries/timeseries.h`:127
- Brief: Query data points in time range.
- Parameters:
  - `metric` (std::string_view): Metric name
  - `entity` (std::string_view): Entity ID
  - `query` (const RangeQuery &): Range query parameters
- Return: Vector of data points
- Details: metric Metric name entity Entity ID query Range query parameters Vector of data points

#### `rocksdb::ColumnFamilyHandle * resolveColumnFamily() const`
- Source: `include/timeseries/timeseries.h`:179
- Brief: Resolve Column Family.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result.

#### `~TimeSeriesStore()=default`
- Source: `include/timeseries/timeseries.h`:98
- Brief: n/a
- Parameters: none

### themis::TimeSeriesStore::Aggregation

#### `nlohmann::json toJson() const`
- Source: `include/timeseries/timeseries.h`:87
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::TimeSeriesStore::DataPoint

#### `DataPoint fromJson(const nlohmann::json &j)`
- Source: `include/timeseries/timeseries.h`:66
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: value(), int64_t(), nlohmann::json::object().

#### `nlohmann::json toJson() const`
- Source: `include/timeseries/timeseries.h`:60
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::TsEncryptedKeyRotation

#### `TsEncryptedKeyRotation(const TsEncryptedKeyRotation &)=delete`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TsEncryptedKeyRotation &): n/a

#### `TsEncryptedKeyRotation(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf, std::shared_ptr< EncryptedChunkStore > enc_store, Config config=Config{})`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:84
- Brief: Construct the rotator.
- Parameters:
  - `db` (rocksdb::TransactionDB *): RocksDB TransactionDB (non-owning).
  - `cf` (rocksdb::ColumnFamilyHandle *): Column family handle (nullptr = default CF).
  - `enc_store` (std::shared_ptr< EncryptedChunkStore >): Shared EncryptedChunkStore that owns key callbacks.
  - `config` (Config): Rotation configuration.
- Details: db RocksDB TransactionDB (non-owning). cf Column family handle (nullptr = default CF). enc_store Shared EncryptedChunkStore that owns key callbacks. config Rotation configuration.

#### `bool isRunning() const noexcept`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:111
- Brief: Returns true while the background thread is active.
- Parameters: none

#### `TsEncryptedKeyRotation & operator=(const TsEncryptedKeyRotation &)=delete`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TsEncryptedKeyRotation &): n/a

#### `void rotationLoop()`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:131
- Brief: Rotation Loop.
- Parameters: none

#### `size_t runOnce()`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:118
- Brief: Run one rotation pass synchronously (for testing / manual trigger).
- Parameters: none
- Return: Number of chunks re-encrypted.
- Details: Run Once. Number of chunks re-encrypted. Return value.

#### `void start()`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:98
- Brief: Start.
- Parameters: none

#### `void stop()`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:106
- Brief: Stop the background worker and join the thread.
- Parameters: none
- Details: Stop. Blocks until the worker exits. Safe to call even if start() was never called.

#### `uint64_t totalReencrypted() const noexcept`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:123
- Brief: Total number of chunks re-encrypted across all passes.
- Parameters: none

#### `~TsEncryptedKeyRotation()`
- Source: `include/timeseries/ts_encrypted_key_rotation.h`:89
- Brief: n/a
- Parameters: none

### themis::ZScoreDetector

#### `std::vector< AnomalyPoint > detect(const std::vector< TSStore::DataPoint > &points, const AnomalyConfig &cfg) const override`
- Source: `include/timeseries/anomaly_detection.h`:103
- Brief: Detect anomalous data points in points.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): Input series (order is preserved; sorting not required).
  - `cfg` (const AnomalyConfig &): Detection configuration.
- Return: Subset of points that are considered anomalous, annotated with the detection score and method name.
- Details: points Input series (order is preserved; sorting not required). cfg Detection configuration. Subset of points that are considered anomalous, annotated with the detection score and method name.

#### `std::vector< std::pair< int64_t, double > > scoreAll(const std::vector< TSStore::DataPoint > &points) const`
- Source: `include/timeseries/anomaly_detection.h`:114
- Brief: Return the z-score for every input point.
- Parameters:
  - `points` (const std::vector< TSStore::DataPoint > &): n/a
- Return: Vector of (timestamp_ms, z_score) pairs in input order. Returns an empty vector when points has fewer than 2 elements or when the standard deviation is effectively zero.
- Details: Vector of (timestamp_ms, z_score) pairs in input order. Returns an empty vector when points has fewer than 2 elements or when the standard deviation is effectively zero.

### themis::bench::tsrg

#### `void BM_TSRG01_WriteThroughput(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:156
- Brief: TSRG-01: In-memory write throughput over 10k deterministic points.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TSRG-01: ≥ 1M points/s.

#### `void BM_TSRG02_RangeQuery(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:189
- Brief: TSRG-02: Range query scan over a 1k-point in-memory series.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TSRG-02: p99 ≤ 500 µs.

#### `void BM_TSRG03_GorillaRoundTrip(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:223
- Brief: TSRG-03: Gorilla lossless round-trip for 100 double values.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TSRG-03: p99 ≤ 100 µs.

#### `void BM_TSRG04_Downsampling(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:257
- Brief: TSRG-04: Downsample 1k points to ~100 buckets (10x reduction).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TSRG-04: p99 ≤ 1 ms.

#### `void BM_TSRG05_RetentionCheck(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:288
- Brief: TSRG-05: Retention boundary comparison for a single point.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TSRG-05: p99 ≤ 50 µs.

#### `void BM_TSRG06_SeriesLookup(benchmark::State &state)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:315
- Brief: TSRG-06: Series name → ID lookup in an in-memory hash map.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: GATE-TSRG-06: p99 ≤ 50 µs.

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `std::size_t downsample(const std::vector< TimePoint > &input, std::int64_t resolution_ns)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:118
- Brief: Downsampling: bucket average over resolution_ns intervals.
- Parameters:
  - `input` (const std::vector< TimePoint > &): n/a
  - `resolution_ns` (std::int64_t): n/a

#### `double gorillaDecode(std::uint64_t bits)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:111
- Brief: Gorilla decode: memcpy uint64 → double.
- Parameters:
  - `bits` (std::uint64_t): n/a

#### `std::uint64_t gorillaEncode(double v)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:104
- Brief: Gorilla encode: memcpy double → uint64.
- Parameters:
  - `v` (double): n/a

#### `std::size_t rangeQuery(const std::vector< TimePoint > &series, std::int64_t start, std::int64_t end)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:92
- Brief: Range query: linear scan returning points in [start, end] inclusive.
- Parameters:
  - `series` (const std::vector< TimePoint > &): n/a
  - `start` (std::int64_t): n/a
  - `end` (std::int64_t): n/a

#### `bool retentionExpired(std::int64_t point_ts, std::int64_t boundary)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:135
- Brief: Retention check: compare single timestamp against boundary.
- Parameters:
  - `point_ts` (std::int64_t): n/a
  - `boundary` (std::int64_t): n/a

#### `std::int64_t seriesLookup(const std::unordered_map< std::string, std::int64_t > &index, const std::string &name)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:140
- Brief: Series lookup: unordered_map name → series ID.
- Parameters:
  - `index` (const std::unordered_map< std::string, std::int64_t > &): n/a
  - `name` (const std::string &): n/a

#### `void writePoint(std::vector< TimePoint > &series, std::int64_t ts_ns, double value)`
- Source: `benchmarks/timeseries/bench_timeseries_release_gates.cpp`:86
- Brief: In-memory write: append to a series vector (monotonic ts assumed).
- Parameters:
  - `series` (std::vector< TimePoint > &): n/a
  - `ts_ns` (std::int64_t): n/a
  - `value` (double): n/a

### themis::timeseries

#### `void emitIncident(const Incident &incident) noexcept`
- Source: `src/timeseries/timeseries_incident_taxonomy.cpp`:37
- Brief: Emits an incident via the registered handler (if any).
- Parameters:
  - `incident` (const Incident &): The incident to emit.
- Details: This function has bounded latency and will not throw. It is safe to call from any path (ingest, query, lifecycle, integration). incident The incident to emit. Exception safety: noexcept.

#### `IncidentHandler getIncidentHandler() noexcept`
- Source: `src/timeseries/timeseries_incident_taxonomy.cpp`:33
- Brief: Retrieves the current global incident handler.
- Parameters: none
- Return: The active handler, or nullptr if none is registered.
- Details: The active handler, or nullptr if none is registered.

#### `bool isAdminIncident(LifecycleIncidentCode code) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:238
- Brief: Returns true when lifecycle error requires administrative action.
- Parameters:
  - `code` (LifecycleIncidentCode): n/a

#### `bool isBackpressureError(IngestIncidentCode code) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:127
- Brief: Returns true when backpressure should be applied to writer.
- Parameters:
  - `code` (IngestIncidentCode): n/a

#### `bool isBackpressureError(TimeseriesErrorCode code) noexcept`
- Source: `include/timeseries/timeseries_api_contract.h`:255
- Brief: Returns true when the error code indicates backpressure that writer should handle.
- Parameters:
  - `code` (TimeseriesErrorCode): n/a
- Details: Backpressure errors indicate buffer/queue congestion. Writers should implement exponential backoff or rate limiting.

#### `bool isDataGapError(QueryIncidentCode code) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:182
- Brief: Returns true when the error indicates missing data but query is valid.
- Parameters:
  - `code` (QueryIncidentCode): n/a

#### `bool isHardIngestError(IngestIncidentCode code) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:116
- Brief: Returns true when the ingest error is unrecoverable (not worth retry).
- Parameters:
  - `code` (IngestIncidentCode): n/a

#### `bool isHardLifecycleError(LifecycleIncidentCode code) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:229
- Brief: Returns true when lifecycle error is unrecoverable.
- Parameters:
  - `code` (LifecycleIncidentCode): n/a

#### `bool isHardQueryError(QueryIncidentCode code) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:172
- Brief: Returns true when the query error is unrecoverable.
- Parameters:
  - `code` (QueryIncidentCode): n/a

#### `bool isHardTimeseriesError(TimeseriesErrorCode code) noexcept`
- Source: `include/timeseries/timeseries_api_contract.h`:228
- Brief: Returns true when the error code is a non-retryable hard error.
- Parameters:
  - `code` (TimeseriesErrorCode): n/a

#### `bool isLifecycleError(TimeseriesErrorCode code) noexcept`
- Source: `include/timeseries/timeseries_api_contract.h`:243
- Brief: Returns true when the error code indicates a data-lifecycle condition (series not found, retention expired) that the caller should handle distinctly from hard errors.
- Parameters:
  - `code` (TimeseriesErrorCode): n/a

#### `bool isPermanentIntegrationError(IntegrationIncidentCode code) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:290
- Brief: Returns true when integration error is non-retryable (permanent).
- Parameters:
  - `code` (IntegrationIncidentCode): n/a

#### `bool isRetryableIntegrationError(IntegrationIncidentCode code) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:282
- Brief: Returns true when integration error is retryable (transient).
- Parameters:
  - `code` (IntegrationIncidentCode): n/a

#### `bool isTransientError(TimeseriesErrorCode code) noexcept`
- Source: `include/timeseries/timeseries_api_contract.h`:265
- Brief: Returns true when the error code is potentially retryable.
- Parameters:
  - `code` (TimeseriesErrorCode): n/a
- Details: Transient errors include buffer congestion, timeouts, and remote errors.

#### `void setIncidentHandler(IncidentHandler handler) noexcept`
- Source: `src/timeseries/timeseries_incident_taxonomy.cpp`:29
- Brief: Registers a global incident handler.
- Parameters:
  - `handler` (IncidentHandler): Callback function, or nullptr to disable incident reporting.
- Details: The handler will be called for all incidents emitted by timeseries operations. Only one handler is active at a time; registering a new handler replaces the old one. handler Callback function, or nullptr to disable incident reporting. Exception safety: noexcept.

#### `std::string_view severityName(IncidentSeverity sev) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:64
- Brief: Returns a human-readable string for the given severity.
- Parameters:
  - `sev` (IncidentSeverity): n/a

#### `std::string_view tsSeverityName(TsIncidentSeverity s) noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:55
- Brief: Returns the string name of a severity level.
- Parameters:
  - `s` (TsIncidentSeverity): n/a

### themis::timeseries::Incident

#### `Incident criticalIngest(IngestIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:388
- Brief: Constructs a CRITICAL ingest incident.
- Parameters:
  - `code` (IngestIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident criticalIntegration(IntegrationIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:532
- Brief: Constructs a CRITICAL integration incident.
- Parameters:
  - `code` (IntegrationIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident criticalLifecycle(LifecycleIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:484
- Brief: Constructs a CRITICAL lifecycle incident.
- Parameters:
  - `code` (LifecycleIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident criticalQuery(QueryIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:436
- Brief: Constructs a CRITICAL query incident.
- Parameters:
  - `code` (QueryIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident errorIngest(IngestIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:400
- Brief: Constructs an ERROR ingest incident.
- Parameters:
  - `code` (IngestIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident errorIntegration(IntegrationIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:544
- Brief: Constructs an ERROR integration incident.
- Parameters:
  - `code` (IntegrationIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident errorLifecycle(LifecycleIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:496
- Brief: Constructs an ERROR lifecycle incident.
- Parameters:
  - `code` (LifecycleIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident errorQuery(QueryIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:448
- Brief: Constructs an ERROR query incident.
- Parameters:
  - `code` (QueryIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident infoIngest(IngestIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:424
- Brief: Constructs an INFO ingest incident.
- Parameters:
  - `code` (IngestIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident infoIntegration(IntegrationIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:568
- Brief: Constructs an INFO integration incident.
- Parameters:
  - `code` (IntegrationIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident infoLifecycle(LifecycleIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:520
- Brief: Constructs an INFO lifecycle incident.
- Parameters:
  - `code` (LifecycleIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident infoQuery(QueryIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:472
- Brief: Constructs an INFO query incident.
- Parameters:
  - `code` (QueryIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident warnIngest(IngestIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:412
- Brief: Constructs a WARN ingest incident.
- Parameters:
  - `code` (IngestIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident warnIntegration(IntegrationIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:556
- Brief: Constructs a WARN integration incident.
- Parameters:
  - `code` (IntegrationIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident warnLifecycle(LifecycleIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:508
- Brief: Constructs a WARN lifecycle incident.
- Parameters:
  - `code` (LifecycleIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

#### `Incident warnQuery(QueryIncidentCode code, IncidentContext ctx={}) noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:460
- Brief: Constructs a WARN query incident.
- Parameters:
  - `code` (QueryIncidentCode): n/a
  - `ctx` (IncidentContext): n/a

### themis::timeseries::IncidentTimestamp

#### `IncidentTimestamp now() noexcept`
- Source: `include/timeseries/timeseries_incident_taxonomy.h`:308
- Brief: Constructs with current time.
- Parameters: none

### themis::timeseries::PromTimeSeries

#### `std::string labelValue(const std::string &name) const`
- Source: `include/timeseries/prometheus_remote_write.h`:83
- Brief: Returns the value of the label with the given name, or an empty string.
- Parameters:
  - `name` (const std::string &): n/a

#### `std::string metricName() const`
- Source: `include/timeseries/prometheus_remote_write.h`:80
- Brief: Returns the value of the __name__ label, or an empty string when absent.
- Parameters: none

### themis::timeseries::PromWriteRequest

#### `Result< PromWriteRequest > decode(const uint8_t *data, size_t size)`
- Source: `include/timeseries/prometheus_remote_write.h`:101
- Brief: Decode.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Decoded request or an error string.
- Throws:
  - Returns: an error when the payload is malformed, including invalid/truncated protobuf wire start for the first timeseries field.
- Details: Decode a raw (uncompressed) protobuf-encoded WriteRequest. data Pointer to the beginning of the serialised bytes. size Number of bytes. Returns an error when the payload is malformed, including invalid/truncated protobuf wire start for the first timeseries field. Decoded request or an error string. data Input parameter. size Input parameter. Return value. Calls: proto::readVarint64(), proto::readLenDelim(), proto::decodeWriteRequest().

#### `Result< PromWriteRequest > decodeSnappy(const uint8_t *data, size_t size)`
- Source: `include/timeseries/prometheus_remote_write.h`:114
- Brief: Decode Snappy.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Decoded request or an error string.
- Details: Decode a snappy-compressed protobuf-encoded WriteRequest. Prometheus remote-write clients always compress with snappy before sending; this is the primary entry-point used by the HTTP handler. data Pointer to the beginning of the snappy-compressed bytes. size Number of bytes. Decoded request or an error string. data Input parameter. size Input parameter. Return value. Calls: snappy::GetUncompressedLength(), resize(), snappy::RawUncompress(), decode(), data(), size().

### themis::timeseries::TsEdgeCaseHandler

#### `TsEdgeCaseHandler(IncidentCallback on_incident=nullptr) noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:80
- Brief: Construct the handler with an optional incident callback.
- Parameters:
  - `on_incident` (IncidentCallback): Callback invoked on incident emission (may be null).
- Details: on_incident Callback invoked on incident emission (may be null).

#### `TsEdgeCaseHandler(TsEdgeCaseHandler &&) noexcept=default`
- Source: `include/timeseries/ts_edge_case_handler.h`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (TsEdgeCaseHandler &&): n/a

#### `TsEdgeCaseHandler(const TsEdgeCaseHandler &)=delete`
- Source: `include/timeseries/ts_edge_case_handler.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TsEdgeCaseHandler &): n/a

#### `void emitIncident(std::string_view id, std::string_view desc) const noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:193
- Brief: Emit Incident.
- Parameters:
  - `id` (std::string_view): Input parameter.
  - `desc` (std::string_view): Input parameter.
- Details: id Input parameter. desc Input parameter. Exception safety: noexcept.

#### `TsEdgeCaseResult handleKeyRotationDuringWrite(const uint8_t *new_key_bytes, std::size_t new_key_len) noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:148
- Brief: Handle an encryption key rotation event during an active chunk write.
- Parameters:
  - `new_key_bytes` (const uint8_t *): Pointer to new key material.
  - `new_key_len` (std::size_t): Length of new key in bytes.
- Return: nullopt when rotation is queued; ENCRYPTION_KEY_NOT_FOUND on invalid key.
- Details: Implements isolation semantics: in-flight write uses old key; new key is queued for subsequent writes. Returns ENCRYPTION_KEY_NOT_FOUND if the new key fails validation. new_key_bytes Pointer to new key material. new_key_len Length of new key in bytes. nullopt when rotation is queued; ENCRYPTION_KEY_NOT_FOUND on invalid key.

#### `TsEdgeCaseResult handleRemoteWriteTimeout(std::string_view endpoint, uint32_t timeout_ms) noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:116
- Brief: Handle a remote-write timeout event.
- Parameters:
  - `endpoint` (std::string_view): The endpoint that timed out.
  - `timeout_ms` (uint32_t): Timeout duration in milliseconds.
- Return: REMOTE_WRITE_RETRIES_EXHAUSTED always; incident emitted.
- Details: Records the timeout incident and returns REMOTE_WRITE_RETRIES_EXHAUSTED. endpoint The endpoint that timed out. timeout_ms Timeout duration in milliseconds. REMOTE_WRITE_RETRIES_EXHAUSTED always; incident emitted.

#### `bool isOutOfOrderFlushRequired(std::size_t current_buffer_size) const noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:163
- Brief: Returns true when the out-of-order buffer has reached capacity.
- Parameters:
  - `current_buffer_size` (std::size_t): Current out-of-order buffer element count.
- Return: true when flush is required.
- Details: A forced re-sort flush must be triggered when this returns true. current_buffer_size Current out-of-order buffer element count. true when flush is required.

#### `uint64_t keyRotationCount() const noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:174
- Brief: Number of encryption key rotation events handled.
- Parameters: none

#### `TsEdgeCaseHandler & operator=(TsEdgeCaseHandler &&) noexcept=default`
- Source: `include/timeseries/ts_edge_case_handler.h`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (TsEdgeCaseHandler &&): n/a

#### `TsEdgeCaseHandler & operator=(const TsEdgeCaseHandler &)=delete`
- Source: `include/timeseries/ts_edge_case_handler.h`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TsEdgeCaseHandler &): n/a

#### `uint64_t remoteWriteTimeoutCount() const noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:171
- Brief: Number of remote-write timeout incidents recorded.
- Parameters: none

#### `TsEdgeCaseResult validateEncryptionKey(const uint8_t *key_bytes, std::size_t key_len) const noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:134
- Brief: Validate an encryption key before a chunk write operation.
- Parameters:
  - `key_bytes` (const uint8_t *): Pointer to the raw key material.
  - `key_len` (std::size_t): Length of the key in bytes.
- Return: nullopt on success; ENCRYPTION_STATE_INVALID on failure.
- Details: Returns ENCRYPTION_STATE_INVALID if: key_bytes is null or empty key_len is not 16, 24, or 32 (AES-128/192/256) key_bytes Pointer to the raw key material. key_len Length of the key in bytes. nullopt on success; ENCRYPTION_STATE_INVALID on failure.

#### `TsEdgeCaseResult validateRemoteWriteEndpoint(std::string_view endpoint) const noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:104
- Brief: Validate a remote-write endpoint URL.
- Parameters:
  - `endpoint` (std::string_view): The remote-write endpoint URL to validate.
- Return: nullopt on success; REMOTE_WRITE_VALIDATION_ERROR on failure.
- Details: Checks: Non-empty URL URL starts with http:// or https:// URL does not exceed kMaxEndpointLength (2048) URL does not contain control characters endpoint The remote-write endpoint URL to validate. nullopt on success; REMOTE_WRITE_VALIDATION_ERROR on failure.

#### `uint64_t validationFailureCount() const noexcept`
- Source: `include/timeseries/ts_edge_case_handler.h`:177
- Brief: Number of validation failures recorded.
- Parameters: none

### themis::timeseries::TsOperatorDiagnostics

#### `TsOperatorDiagnostics()=default`
- Source: `include/timeseries/ts_operator_diagnostics.h`:104
- Brief: n/a
- Parameters: none

#### `TsOperatorDiagnostics(TsOperatorDiagnostics &&) noexcept=default`
- Source: `include/timeseries/ts_operator_diagnostics.h`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (TsOperatorDiagnostics &&): n/a

#### `TsOperatorDiagnostics(const TsOperatorDiagnostics &)=delete`
- Source: `include/timeseries/ts_operator_diagnostics.h`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TsOperatorDiagnostics &): n/a

#### `void clearIncidents() noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:189
- Brief: Clear all incidents from the buffer.
- Parameters: none

#### `std::size_t countBySeverity(TsIncidentSeverity severity) const noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:168
- Brief: Count incidents with the given severity.
- Parameters:
  - `severity` (TsIncidentSeverity): n/a

#### `std::string formatSummary(std::size_t max_count=10) const noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:186
- Brief: Format recent incidents as an operator summary string.
- Parameters:
  - `max_count` (std::size_t): Maximum incidents to include (0 = all).
- Return: Multi-line formatted summary.
- Details: max_count Maximum incidents to include (0 = all). Multi-line formatted summary.

#### `bool hasCriticalIncidents() const noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:171
- Brief: True when any CRITICAL incidents are in the buffer.
- Parameters: none

#### `std::vector< TsIncident > incidentsBySeverity(TsIncidentSeverity min_severity) const noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:162
- Brief: Return all incidents with severity >= min_severity (newest first).
- Parameters:
  - `min_severity` (TsIncidentSeverity): n/a

#### `int64_t nowNs() noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:201
- Brief: Now Ns.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `TsOperatorDiagnostics & operator=(TsOperatorDiagnostics &&) noexcept=default`
- Source: `include/timeseries/ts_operator_diagnostics.h`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (TsOperatorDiagnostics &&): n/a

#### `TsOperatorDiagnostics & operator=(const TsOperatorDiagnostics &)=delete`
- Source: `include/timeseries/ts_operator_diagnostics.h`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TsOperatorDiagnostics &): n/a

#### `std::vector< TsIncident > recentIncidents(std::size_t max_count=0) const noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:156
- Brief: Return the N most-recent incidents (newest first).
- Parameters:
  - `max_count` (std::size_t): Maximum to return (0 = all).
- Details: max_count Maximum to return (0 = all).

#### `void recordFromCallback(std::string_view incident_id, std::string_view description) noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:144
- Brief: Convenience overload for use as a TsEdgeCaseHandler incident callback.
- Parameters:
  - `incident_id` (std::string_view): Input parameter.
  - `description` (std::string_view): Input parameter.
- Details: Infers severity from the incident_id suffix convention: *-CRITICAL / *-PERSISTENT → CRITICAL *-TIMEOUT / *-ROTATION-INVALID → ERROR *-UNAVAILABLE / *-FALLBACK → WARNING other → INFO incident_id Input parameter. description Input parameter. Exception safety: noexcept.

#### `void recordIncident(std::string_view incident_id, TsIncidentSeverity severity, std::string_view description, std::string_view remediation, std::optional< TimeseriesErrorCode > error_code=std::nullopt) noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:125
- Brief: Record a structured incident.
- Parameters:
  - `incident_id` (std::string_view): Unique incident identifier.
  - `severity` (TsIncidentSeverity): Severity level.
  - `description` (std::string_view): Human-readable description.
  - `remediation` (std::string_view): Operator action hint.
  - `error_code` (std::optional< TimeseriesErrorCode >): Optional associated TimeseriesErrorCode.
- Details: incident_id Unique incident identifier. severity Severity level. description Human-readable description. remediation Operator action hint. error_code Optional associated TimeseriesErrorCode.

#### `std::string remediationForId(std::string_view id) noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:215
- Brief: Remediation For Id.
- Parameters:
  - `id` (std::string_view): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value. Exception safety: noexcept.

#### `TsIncidentSeverity severityFromId(std::string_view id) noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:208
- Brief: Severity From Id.
- Parameters:
  - `id` (std::string_view): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value. Exception safety: noexcept.

#### `uint64_t totalIncidentCount() const noexcept`
- Source: `include/timeseries/ts_operator_diagnostics.h`:174
- Brief: Total incidents recorded since construction (monotonic).
- Parameters: none

### themis::timeseries::TsStreamCursor

#### `TsStreamCursor(TSStore &store, TSStore::QueryOptions options, Config cfg)`
- Source: `include/timeseries/ts_stream_cursor.h`:141
- Brief: Ts Stream Cursor.
- Parameters:
  - `store` (TSStore &): Input/output parameter.
  - `options` (TSStore::QueryOptions): Input parameter.
  - `cfg` (Config): Input parameter.
- Return: Return value.
- Details: store Input/output parameter. options Input parameter. cfg Input parameter. Return value.

#### `TsStreamCursor(TsStreamCursor &&) noexcept=default`
- Source: `include/timeseries/ts_stream_cursor.h`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (TsStreamCursor &&): n/a

#### `TsStreamCursor(const TsStreamCursor &)=delete`
- Source: `include/timeseries/ts_stream_cursor.h`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TsStreamCursor &): n/a

#### `Result< void > advance()`
- Source: `include/timeseries/ts_stream_cursor.h`:99
- Brief: Advance.
- Parameters: none
- Return: Return value.
- Details: Advance an iterator within the validated range. Return value. None. Calls: size(), fetchNextPage().

#### `void close() noexcept`
- Source: `include/timeseries/ts_stream_cursor.h`:107
- Brief: Release all resources and mark the cursor as exhausted.
- Parameters: none
- Details: After close(), valid() returns false. The cursor may not be re-opened. Exception safety: noexcept.

#### `const TSStore::DataPoint & current() const noexcept`
- Source: `include/timeseries/ts_stream_cursor.h`:93
- Brief: Reference to the current DataPoint.
- Parameters: none
- Return: Return value.
- Details: valid() == true. Behaviour is undefined otherwise. The reference is valid until the next advance() call that crosses a page boundary (i.e. until the internal page buffer is refilled). Copy the DataPoint if you need to retain it across advance() calls. Return value. Exception safety: noexcept.

#### `Result< void > fetchNextPage()`
- Source: `include/timeseries/ts_stream_cursor.h`:149
- Brief: Fetch Next Page.
- Parameters: none
- Return: Return value.
- Details: ------------------------------------------------------------------------ Internal: paginated fetch ------------------------------------------------------------------------ Return value. Return value. Calls: tl::unexpected(), Error(), clear(), query(), error(), std::move(), empty(), back().

#### `Result< std::unique_ptr< TsStreamCursor > > open(TSStore &store, TSStore::QueryOptions options)`
- Source: `include/timeseries/ts_stream_cursor.h`:50
- Brief: Open a streaming cursor over store.
- Parameters:
  - `store` (TSStore &): Input/output parameter.
  - `options` (TSStore::QueryOptions): Input parameter.
- Return: A ready cursor, or an error if the initial fetch failed.
- Details: Open. The first page is fetched eagerly during open() so that valid() is immediately usable. store The TSStore to scan (must outlive the cursor). options Query filter (metric, time range, entity, tags). A ready cursor, or an error if the initial fetch failed. store Input/output parameter. options Input parameter. Return value.

#### `Result< std::unique_ptr< TsStreamCursor > > open(TSStore &store, TSStore::QueryOptions options, Config cfg)`
- Source: `include/timeseries/ts_stream_cursor.h`:65
- Brief: Open a streaming cursor over store.
- Parameters:
  - `store` (TSStore &): Input/output parameter.
  - `options` (TSStore::QueryOptions): Input parameter.
  - `cfg` (Config): Input parameter.
- Return: A ready cursor, or an error if the initial fetch failed.
- Details: Open. The first page is fetched eagerly during open() so that valid() is immediately usable. store The TSStore to scan (must outlive the cursor). options Query filter (metric, time range, entity, tags). cfg Tuning parameters (page size). A ready cursor, or an error if the initial fetch failed. store Input/output parameter. options Input parameter. cfg Input parameter. Return value.

#### `TsStreamCursor & operator=(TsStreamCursor &&) noexcept=default`
- Source: `include/timeseries/ts_stream_cursor.h`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (TsStreamCursor &&): n/a

#### `TsStreamCursor & operator=(const TsStreamCursor &)=delete`
- Source: `include/timeseries/ts_stream_cursor.h`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TsStreamCursor &): n/a

#### `uint64_t pagesFetched() const noexcept`
- Source: `include/timeseries/ts_stream_cursor.h`:123
- Brief: Pages fetched.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `uint64_t rowsConsumed() const noexcept`
- Source: `include/timeseries/ts_stream_cursor.h`:116
- Brief: Rows consumed.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `bool valid() const noexcept`
- Source: `include/timeseries/ts_stream_cursor.h`:80
- Brief: Return true while the cursor has at least one more DataPoint.
- Parameters: none
- Return: True on success.
- Details: Returns false once the last DataPoint has been consumed or after close() has been called. True on success. Exception safety: noexcept.

#### `~TsStreamCursor()`
- Source: `include/timeseries/ts_stream_cursor.h`:125
- Brief: n/a
- Parameters: none

### themis::timeseries::proto

#### `bool decodeLabel(const uint8_t *buf, size_t size, PromLabel &out)`
- Source: `src/timeseries/prometheus_remote_write.cpp`:164
- Brief: ── Label ─────────────────────────────────────────────────────────────────────
- Parameters:
  - `buf` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `out` (PromLabel &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: buf Input parameter. size Input parameter. out Input/output parameter. True when the operation succeeds. Calls: readVarint64(), readLenDelim(), assign(), skipField().

#### `bool decodeSample(const uint8_t *buf, size_t size, PromSample &out)`
- Source: `src/timeseries/prometheus_remote_write.cpp`:205
- Brief: ── Sample ────────────────────────────────────────────────────────────────────
- Parameters:
  - `buf` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `out` (PromSample &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: buf Input parameter. size Input parameter. out Input/output parameter. True when the operation succeeds. Calls: readVarint64(), static_assert(), std::memcpy(), skipField().

#### `bool decodeTimeSeries(const uint8_t *buf, size_t size, PromTimeSeries &out)`
- Source: `src/timeseries/prometheus_remote_write.cpp`:249
- Brief: ── TimeSeries ────────────────────────────────────────────────────────────────
- Parameters:
  - `buf` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `out` (PromTimeSeries &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: buf Input parameter. size Input parameter. out Input/output parameter. True when the operation succeeds. Calls: readVarint64(), readLenDelim(), decodeLabel(), push_back(), std::move(), decodeSample(), skipField().

#### `bool decodeWriteRequest(const uint8_t *buf, size_t size, PromWriteRequest &out)`
- Source: `src/timeseries/prometheus_remote_write.cpp`:298
- Brief: ── WriteRequest ──────────────────────────────────────────────────────────────
- Parameters:
  - `buf` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `out` (PromWriteRequest &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: buf Input parameter. size Input parameter. out Input/output parameter. True when the operation succeeds. Calls: readVarint64(), readLenDelim(), decodeTimeSeries(), push_back(), std::move(), skipField().

#### `bool readLenDelim(const uint8_t *buf, size_t &pos, size_t end, const uint8_t *&span_begin, size_t &span_len)`
- Source: `src/timeseries/prometheus_remote_write.cpp`:100
- Brief: Read Len Delim.
- Parameters:
  - `buf` (const uint8_t *): Input parameter.
  - `pos` (size_t &): Input/output parameter.
  - `end` (size_t): Input parameter.
  - `span_begin` (const uint8_t *&): Input parameter.
  - `span_len` (size_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: buf Input parameter. pos Input/output parameter. end Input parameter. span_begin Input parameter. span_len Input/output parameter. True when the operation succeeds. Calls: readVarint64().

#### `bool readVarint64(const uint8_t *buf, size_t &pos, size_t end, uint64_t &out)`
- Source: `src/timeseries/prometheus_remote_write.cpp`:75
- Brief: Read Varint64.
- Parameters:
  - `buf` (const uint8_t *): Input parameter.
  - `pos` (size_t &): Input/output parameter.
  - `end` (size_t): Input parameter.
  - `out` (uint64_t &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: buf Input parameter. pos Input/output parameter. end Input parameter. out Input/output parameter. True when the operation succeeds. Implements readVarint64 without additional internal calls.

#### `bool skipField(const uint8_t *buf, size_t &pos, size_t end, uint32_t wire_type)`
- Source: `src/timeseries/prometheus_remote_write.cpp`:125
- Brief: Skip Field.
- Parameters:
  - `buf` (const uint8_t *): Input parameter.
  - `pos` (size_t &): Input/output parameter.
  - `end` (size_t): Input parameter.
  - `wire_type` (uint32_t): Input parameter.
- Return: True when the operation succeeds.
- Details: buf Input parameter. pos Input/output parameter. end Input parameter. wire_type Input parameter. True when the operation succeeds. Calls: readVarint64(), readLenDelim().

### themis::timeseries::test

#### `TEST(TimeseriesContractHardening, TSCH01_MonotonicTimestampsAccepted)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:152
- Brief: TSCH-01: Monotonic timestamps are accepted without error.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH01_MonotonicTimestampsAccepted): n/a

#### `TEST(TimeseriesContractHardening, TSCH02_OutOfOrderTimestampError)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:162
- Brief: TSCH-02: Out-of-order timestamp raises TIMESTAMP_OUT_OF_ORDER.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH02_OutOfOrderTimestampError): n/a

#### `TEST(TimeseriesContractHardening, TSCH03_ZeroTimestampError)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:171
- Brief: TSCH-03: Zero (null) timestamp raises TIMESTAMP_OUT_OF_ORDER.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH03_ZeroTimestampError): n/a

#### `TEST(TimeseriesContractHardening, TSCH04_DuplicateTimestampError)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:178
- Brief: TSCH-04: Duplicate timestamp (== tail) raises TIMESTAMP_OUT_OF_ORDER.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH04_DuplicateTimestampError): n/a

#### `TEST(TimeseriesContractHardening, TSCH05_InclusiveBoundsQuery)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:190
- Brief: TSCH-05: Range query [start, end] returns all points with ts in range.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH05_InclusiveBoundsQuery): n/a

#### `TEST(TimeseriesContractHardening, TSCH06_EmptyRangeIsNotError)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:202
- Brief: TSCH-06: Empty range (no points in [start, end]) → empty result, no error.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH06_EmptyRangeIsNotError): n/a

#### `TEST(TimeseriesContractHardening, TSCH07_SeriesNotFoundError)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:209
- Brief: TSCH-07: Non-existent series → SERIES_NOT_FOUND.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH07_SeriesNotFoundError): n/a

#### `TEST(TimeseriesContractHardening, TSCH08_BoundaryPointsIncluded)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:217
- Brief: TSCH-08: Points at exact start and end boundaries are included.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH08_BoundaryPointsIncluded): n/a

#### `TEST(TimeseriesContractHardening, TSCH09_GorillaRoundTripArbitrary)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:232
- Brief: TSCH-09: Gorilla round-trip preserves arbitrary IEEE 754 float64.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH09_GorillaRoundTripArbitrary): n/a

#### `TEST(TimeseriesContractHardening, TSCH10_NanPreservedGorilla)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:250
- Brief: TSCH-10: NaN is preserved through Gorilla encode/decode (bit-identical).
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH10_NanPreservedGorilla): n/a

#### `TEST(TimeseriesContractHardening, TSCH11_PosInfPreservedGorilla)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:263
- Brief: TSCH-11: +Inf is preserved through Gorilla encode/decode.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH11_PosInfPreservedGorilla): n/a

#### `TEST(TimeseriesContractHardening, TSCH12_NegInfPreservedGorilla)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:271
- Brief: TSCH-12: -Inf is preserved through Gorilla encode/decode.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH12_NegInfPreservedGorilla): n/a

#### `TEST(TimeseriesContractHardening, TSCH13_DownsamplingCountDeterministic)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:283
- Brief: TSCH-13: Output bucket count is deterministic for given resolution.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH13_DownsamplingCountDeterministic): n/a

#### `TEST(TimeseriesContractHardening, TSCH14_EmptyInputEmptyOutput)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:298
- Brief: TSCH-14: Empty input → empty output.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH14_EmptyInputEmptyOutput): n/a

#### `TEST(TimeseriesContractHardening, TSCH15_SinglePointPassthrough)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:307
- Brief: TSCH-15: Single point passes through unchanged as single bucket.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH15_SinglePointPassthrough): n/a

#### `TEST(TimeseriesContractHardening, TSCH16_ZeroResolutionError)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:317
- Brief: TSCH-16: Zero resolution → DOWNSAMPLING_RESOLUTION_INVALID.
- Parameters:
  - `<unnamed>` (TimeseriesContractHardening): n/a
  - `<unnamed>` (TSCH16_ZeroResolutionError): n/a

#### `TEST(TimeseriesQ4Hardening, TEH01_EmptyEndpointReturnsValidationError)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH01_EmptyEndpointReturnsValidationError): n/a
- Details: TestTEH-01: Empty endpoint URL returns REMOTE_WRITE_VALIDATION_ERROR.

#### `TEST(TimeseriesQ4Hardening, TEH02_InvalidProtocolReturnsError)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH02_InvalidProtocolReturnsError): n/a
- Details: TestTEH-02: Non-http/https URL returns REMOTE_WRITE_VALIDATION_ERROR.

#### `TEST(TimeseriesQ4Hardening, TEH03_ValidHttpsEndpointReturnsSuccess)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH03_ValidHttpsEndpointReturnsSuccess): n/a
- Details: TestTEH-03: Valid https URL returns success (nullopt).

#### `TEST(TimeseriesQ4Hardening, TEH04_TooLongEndpointReturnsError)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH04_TooLongEndpointReturnsError): n/a
- Details: TestTEH-04: URL exceeding kMaxEndpointLength returns REMOTE_WRITE_VALIDATION_ERROR.

#### `TEST(TimeseriesQ4Hardening, TEH05_TimeoutHandlerReturnsRetriesExhausted)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH05_TimeoutHandlerReturnsRetriesExhausted): n/a
- Details: TestTEH-05: Timeout handler returns REMOTE_WRITE_RETRIES_EXHAUSTED.

#### `TEST(TimeseriesQ4Hardening, TEH06_TimeoutCounterIncrementsPerCall)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH06_TimeoutCounterIncrementsPerCall): n/a
- Details: TestTEH-06: Each timeout call increments the timeout counter.

#### `TEST(TimeseriesQ4Hardening, TEH07_IncidentCallbackInvokedOnTimeout)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH07_IncidentCallbackInvokedOnTimeout): n/a
- Details: TestTEH-07: Incident callback is invoked on remote-write timeout.

#### `TEST(TimeseriesQ4Hardening, TEH08_ControlCharInUrlReturnsError)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH08_ControlCharInUrlReturnsError): n/a
- Details: TestTEH-08: URL containing control characters returns REMOTE_WRITE_VALIDATION_ERROR.

#### `TEST(TimeseriesQ4Hardening, TEH09_NullKeyReturnsEncryptionError)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH09_NullKeyReturnsEncryptionError): n/a
- Details: TestTEH-09: Null key pointer returns ENCRYPTION_STATE_INVALID.

#### `TEST(TimeseriesQ4Hardening, TEH10_ZeroLengthKeyReturnsEncryptionError)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH10_ZeroLengthKeyReturnsEncryptionError): n/a
- Details: TestTEH-10: Zero-length key returns ENCRYPTION_STATE_INVALID.

#### `TEST(TimeseriesQ4Hardening, TEH11_InvalidKeyLengthReturnsEncryptionError)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH11_InvalidKeyLengthReturnsEncryptionError): n/a
- Details: TestTEH-11: Invalid key length (e.g. 20 bytes) returns ENCRYPTION_STATE_INVALID.

#### `TEST(TimeseriesQ4Hardening, TEH12_ValidKeyLengthsReturnSuccess)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH12_ValidKeyLengthsReturnSuccess): n/a
- Details: TestTEH-12: Valid AES key lengths (16/24/32) return success (nullopt).

#### `TEST(TimeseriesQ4Hardening, TEH13_RotationWithInvalidKeyReturnsError)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH13_RotationWithInvalidKeyReturnsError): n/a
- Details: TestTEH-13: Rotation with invalid new key returns ENCRYPTION_KEY_NOT_FOUND.

#### `TEST(TimeseriesQ4Hardening, TEH14_RotationWithValidKeySucceeds)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH14_RotationWithValidKeySucceeds): n/a
- Details: TestTEH-14: Rotation with valid 32-byte key returns success and increments counter.

#### `TEST(TimeseriesQ4Hardening, TEH15_IncidentCallbackInvokedOnValidRotation)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH15_IncidentCallbackInvokedOnValidRotation): n/a
- Details: TestTEH-15: Incident callback is invoked on successful key rotation.

#### `TEST(TimeseriesQ4Hardening, TEH16_MultipleRotationsIncrementCounter)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH16_MultipleRotationsIncrementCounter): n/a
- Details: TestTEH-16: Multiple successful rotations increment counter correctly.

#### `TEST(TimeseriesQ4Hardening, TEH17_BufferBelowCapacityNoFlushRequired)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH17_BufferBelowCapacityNoFlushRequired): n/a
- Details: TestTEH-17: Buffer below kMaxOutOfOrderBuffer does not require flush.

#### `TEST(TimeseriesQ4Hardening, TEH18_BufferAtCapacityRequiresFlush)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH18_BufferAtCapacityRequiresFlush): n/a
- Details: TestTEH-18: Buffer at kMaxOutOfOrderBuffer triggers flush requirement.

#### `TEST(TimeseriesQ4Hardening, TEH19_OperatorDiagnosticsRecordsIncidentCorrectly)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH19_OperatorDiagnosticsRecordsIncidentCorrectly): n/a
- Details: TestTEH-19: TsOperatorDiagnostics records incident with correct severity.

#### `TEST(TimeseriesQ4Hardening, TEH20_FormatSummaryIncludesIncidentDetails)`
- Source: `tests/timeseries/test_timeseries_q4_hardening_focused.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (TimeseriesQ4Hardening): n/a
  - `<unnamed>` (TEH20_FormatSummaryIncludesIncidentDetails): n/a
- Details: TestTEH-20: formatSummary includes incident ID and severity.

#### `std::optional< TimeseriesErrorCode > mockDownsample(const std::vector< TimePoint > &input, std::int64_t resolution_ns, std::vector< double > &out_buckets)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:122
- Brief: Downsampling: groups points into buckets of resolution_ns width.
- Parameters:
  - `input` (const std::vector< TimePoint > &): n/a
  - `resolution_ns` (std::int64_t): n/a
  - `out_buckets` (std::vector< double > &): n/a

#### `double mockGorillaDecode(std::uint64_t bits)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:115
- Brief: Gorilla decode: restores the raw IEEE 754 bits.
- Parameters:
  - `bits` (std::uint64_t): n/a

#### `std::uint64_t mockGorillaEncode(double v)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:108
- Brief: Gorilla encode: stores the raw IEEE 754 bits (lossless mock).
- Parameters:
  - `v` (double): n/a

#### `std::vector< TimePoint > mockRangeQuery(const std::vector< TimePoint > &series, std::int64_t start_ns, std::int64_t end_ns, bool series_exists=true)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:84
- Brief: Range query: returns all points in [start_ns, end_ns] inclusive.
- Parameters:
  - `series` (const std::vector< TimePoint > &): n/a
  - `start_ns` (std::int64_t): n/a
  - `end_ns` (std::int64_t): n/a
  - `series_exists` (bool): n/a

#### `std::optional< TimeseriesErrorCode > mockSeriesCheck(bool exists)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:100
- Brief: n/a
- Parameters:
  - `exists` (bool): n/a

#### `std::optional< TimeseriesErrorCode > mockWritePoint(std::int64_t tail_ts, std::int64_t new_ts)`
- Source: `tests/timeseries/test_timeseries_contract_hardening_focused.cpp`:72
- Brief: Validates that the new point's timestamp is strictly greater than tail.
- Parameters:
  - `tail_ts` (std::int64_t): n/a
  - `new_ts` (std::int64_t): n/a

