# MAINTENANCE DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\maintenance\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\maintenance\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 29
- Compounds: 95
- Classes/Structs: 38
- Namespaces: 20
- File Compounds: 29

## Namespaces
- @120015360271013217111206345057267231305211324075
- benchmark
- themis
- themis::bench
- themis::bench::mtn
- themis::bench::mtn::@223335055325354072056021255276227072066257021250
- themis::bench::mtn::themis
- themis::bench::mtn::themis::maintenance
- themis::benchmarks
- themis::benchmarks::docker_raid
- themis::maintenance
- themis::maintenance::@241250374054354327267172123244300177271222370260
- themis::maintenance::test
- themis::modules
- themis::observability
- themis::test
- themis::test::wave_d
- themis::transaction
- themis::updates
- themis::utils

## Types
### Classes
- DeltaEngineBenchFixture
- HotReloadBenchFixture
- SagaBenchmarkFixture
- SnapshotManagerBenchmark
- themis::benchmarks::docker_raid::DockerContainer
- themis::benchmarks::docker_raid::DockerRAIDBenchmarkBase
- themis::benchmarks::docker_raid::DockerRAIDController
- themis::benchmarks::docker_raid::TestDataGenerator
- themis::maintenance::DatabaseMaintenanceOrchestrator
- themis::maintenance::FunctionMaintenanceTaskHandler
- themis::maintenance::IDistributedLock
- themis::maintenance::IMaintenanceTaskHandler
- themis::maintenance::InProcessDistributedLock
- themis::maintenance::MaintenanceScheduleStore
- themis::maintenance::MvccCleanupHandler
- themis::maintenance::ReplicaValidationHandler
- themis::maintenance::StorageCompactionHandler

### Structs
- themis::bench::mtn::themis::maintenance::DispatchOutcome
- themis::bench::mtn::themis::maintenance::MaintenanceHealthReport
- themis::bench::mtn::themis::maintenance::ModuleHealthSignal
- themis::benchmarks::docker_raid::DockerContainerConfig
- themis::maintenance::DispatchOutcome
- themis::maintenance::InProcessDistributedLock::LockEntry
- themis::maintenance::MaintenanceHealthReport
- themis::maintenance::MaintenanceHealthSnapshot
- themis::maintenance::MaintenanceScheduleDescriptor
- themis::maintenance::MaintenanceScheduleEntry
- themis::maintenance::MaintenanceTaskDependency
- themis::maintenance::ModuleHealthSignal
- themis::maintenance::OrchestratorJob
- themis::maintenance::TenantMaintenanceConfig
- themis::maintenance::test::ChurnTracker
- themis::maintenance::test::InFlightGuard
- themis::maintenance::test::MockScheduleStore
- themis::maintenance::test::TestRingBuffer
- themis::test::wave_d::StubCompactionShard
- themis::test::wave_d::StubRetentionEnforcer
- themis::test::wave_d::StubVacuumTaskStore

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 353

### DeltaEngineBenchFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### HotReloadBenchFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:26
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:36
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### SagaBenchmarkFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:24
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### SnapshotManagerBenchmark

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:13
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:35
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void populateTags(int64_t num_tags)`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:45
- Brief: n/a
- Parameters:
  - `num_tags` (int64_t): n/a

#### `void recordEvents(int count)`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:53
- Brief: n/a
- Parameters:
  - `count` (int): n/a

### bench_docker_raid_comprehensive.cpp

#### `int main(int argc, char **argv)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:974
- Brief: n/a
- Parameters:
  - `argc` (int): n/a
  - `argv` (char **): n/a

### bench_hot_reload_manager.cpp

#### `Arg(1) -> Arg(2) ->Arg(4) ->Arg(8)`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, ConcurrentReloads)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (ConcurrentReloads): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, GetCurrentVersionMiss)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (GetCurrentVersionMiss): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, GetCurrentVersionNotRegistered)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (GetCurrentVersionNotRegistered): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, GetStats)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (GetStats): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, OneCallbackDispatch)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (OneCallbackDispatch): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, RegisterUnregister)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (RegisterUnregister): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, RegisteredModulesList)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (RegisteredModulesList): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, ReloadAttemptFastFail)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (ReloadAttemptFastFail): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, ReloadWithStateSaveCallback)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (ReloadWithStateSaveCallback): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, RollbackWithNoBackup)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (RollbackWithNoBackup): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, TenCallbacksDispatch)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (TenCallbacksDispatch): n/a

#### `BENCHMARK_F(HotReloadBenchFixture, ZeroCallbacksDispatch)(benchmark`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (HotReloadBenchFixture): n/a
  - `<unnamed>` (ZeroCallbacksDispatch): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/maintenance/bench_hot_reload_manager.cpp`:242
- Brief: n/a
- Parameters: none

### bench_lock_contention.cpp

#### `BENCHMARK(BM_LockContention_Disjoint) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->Arg(32) ->UseRealTime()`
- Source: `benchmarks/maintenance/bench_lock_contention.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_LockContention_Disjoint): n/a

#### `BENCHMARK(BM_LockContention_Overlapping) -> Arg(1) ->Arg(4) ->Arg(8) ->Arg(16) ->Arg(32) ->UseRealTime()`
- Source: `benchmarks/maintenance/bench_lock_contention.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_LockContention_Overlapping): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/maintenance/bench_lock_contention.cpp`:86
- Brief: n/a
- Parameters: none

#### `void BM_LockContention_Disjoint(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_lock_contention.cpp`:80
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LockContention_Overlapping(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_lock_contention.cpp`:81
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_maintenance_distributed_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/maintenance/bench_maintenance_distributed_gates.cpp`:140
- Brief: n/a
- Parameters: none

### bench_maintenance_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/maintenance/bench_maintenance_release_gates.cpp`:327
- Brief: n/a
- Parameters: none

### bench_metrics_collector.cpp

#### `Arg(10) -> Arg(100) ->Arg(1000)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Arg(100) -> Arg(1000) ->Arg(10000)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Arg(4) -> Arg(16) ->Arg(64)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (4): n/a

#### `BENCHMARK(BM_ConcurrentExport) -> ThreadRange(1, 8)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ConcurrentExport): n/a

#### `BENCHMARK(BM_ConcurrentMixedOperations) -> ThreadRange(1, 16)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ConcurrentMixedOperations): n/a

#### `BENCHMARK(BM_ConcurrentRecording) -> ThreadRange(1, 16)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ConcurrentRecording): n/a

#### `BENCHMARK(BM_HistogramRecording)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_HistogramRecording): n/a

#### `BENCHMARK(BM_MixedMetrics)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MixedMetrics): n/a

#### `BENCHMARK(BM_MultipleHistograms)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:426
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_MultipleHistograms): n/a

#### `BENCHMARK(BM_PrometheusExport_Empty)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PrometheusExport_Empty): n/a

#### `BENCHMARK(BM_PrometheusExport_WithData)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PrometheusExport_WithData): n/a

#### `BENCHMARK(BM_RecordCacheHit)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RecordCacheHit): n/a

#### `BENCHMARK(BM_RecordQuery)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RecordQuery): n/a

#### `BENCHMARK(BM_RecordShardLatency)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RecordShardLatency): n/a

#### `BENCHMARK(BM_RecordTSStoreWrite)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RecordTSStoreWrite): n/a

#### `BENCHMARK(BM_ResetEmpty)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:471
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ResetEmpty): n/a

#### `BENCHMARK(BM_SimulateMonitoringWorkload)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:550
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SimulateMonitoringWorkload): n/a

#### `BENCHMARK(BM_SimulateQueryWorkload)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:526
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SimulateQueryWorkload): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:556
- Brief: n/a
- Parameters: none

#### `void BM_CacheMetricsBatch(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:342
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ConcurrentExport(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:269
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ConcurrentMixedOperations(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:248
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ConcurrentRecording(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:235
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_HighVolumeRecording(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:129
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_HistogramRecording(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:390
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ManyUniqueMetrics(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:148
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MemoryFootprint(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:432
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MixedMetrics(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:95
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MultipleHistograms(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:407
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PrometheusExport_Empty(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:172
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PrometheusExport_LargeDataset(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:205
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PrometheusExport_WithData(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:185
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RecordCacheHit(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:55
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RecordQuery(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:43
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RecordShardLatency(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:79
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RecordTSStoreWrite(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:67
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ResetEmpty(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:462
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ResetWithData(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:473
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SecurityMetricsBatch(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:365
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ShardingMetricsBatch(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:317
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SimulateMonitoringWorkload(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:528
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SimulateQueryWorkload(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:500
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_TSStoreMetricsBatch(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:293
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void ResetCollector(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:24
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `std::vector< std::string > generateMetricNames(int count)`
- Source: `benchmarks/maintenance/bench_metrics_collector.cpp`:30
- Brief: n/a
- Parameters:
  - `count` (int): n/a

### bench_saga_compensation.cpp

#### `Arg(2) -> Arg(5) ->Arg(10) ->Arg(20) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (2): n/a

#### `Arg(5) -> Arg(10) ->Arg(20) ->Arg(50) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (5): n/a

#### `BENCHMARK_DEFINE_F(SagaBenchmarkFixture, CompensationWithErrors)(benchmark`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (SagaBenchmarkFixture): n/a
  - `<unnamed>` (CompensationWithErrors): n/a

#### `BENCHMARK_DEFINE_F(SagaBenchmarkFixture, DatabaseWriteCompensation)(benchmark`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (SagaBenchmarkFixture): n/a
  - `<unnamed>` (DatabaseWriteCompensation): n/a

#### `BENCHMARK_DEFINE_F(SagaBenchmarkFixture, NestedSagaPattern)(benchmark`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:329
- Brief: n/a
- Parameters:
  - `<unnamed>` (SagaBenchmarkFixture): n/a
  - `<unnamed>` (NestedSagaPattern): n/a

#### `BENCHMARK_DEFINE_F(SagaBenchmarkFixture, PartialCompensation)(benchmark`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (SagaBenchmarkFixture): n/a
  - `<unnamed>` (PartialCompensation): n/a

#### `BENCHMARK_DEFINE_F(SagaBenchmarkFixture, SimpleCompensation)(benchmark`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (SagaBenchmarkFixture): n/a
  - `<unnamed>` (SimpleCompensation): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:433
- Brief: n/a
- Parameters: none

#### `void BM_CompensationLatencyDistribution(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:385
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ConcurrentSagaCompensation(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:223
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/maintenance/bench_saga_compensation.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

### bench_sanity.cpp

#### `BENCHMARK(BM_Sanity)`
- Source: `benchmarks/maintenance/bench_sanity.cpp`:13
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Sanity): n/a

#### `void BM_Sanity(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_sanity.cpp`:3
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_snapshot_manager.cpp

#### `Arg(0) -> Unit(benchmark::kMicrosecond) ->Iterations(1000)`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a

#### `Arg(1) -> Unit(benchmark::kNanosecond) ->Iterations(10000)`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Arg(10) -> Unit(benchmark::kMicrosecond) ->Iterations(1000)`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Arg(100) -> Unit(benchmark::kMillisecond) ->Iterations(500)`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Arg(1000) -> Unit(benchmark::kMillisecond) ->Iterations(100)`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (1000): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ConcurrentCreateTag)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (ConcurrentCreateTag): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, CreateTag)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (CreateTag): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, DeleteTag)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (DeleteTag): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, GetSequenceForTag)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (GetSequenceForTag): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, GetStatistics)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (GetStatistics): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, GetTag)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (GetTag): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTags10)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (ListTags10): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTags100)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (ListTags100): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTags1000)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (ListTags1000): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTagsSortedByName)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (ListTagsSortedByName): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTagsSortedBySequence)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (ListTagsSortedBySequence): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTagsWithLimit)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (ListTagsWithLimit): n/a

#### `BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, TagExists)(benchmark`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManagerBenchmark): n/a
  - `<unnamed>` (TagExists): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/maintenance/bench_snapshot_manager.cpp`:291
- Brief: n/a
- Parameters: none

### bench_update_pipeline.cpp

#### `Arg(1) -> Arg(5) ->Arg(20) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `BENCHMARK(BM_UpdateStateMachine_CurrentState) -> Unit(benchmark::kNanosecond)`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_UpdateStateMachine_CurrentState): n/a

#### `BENCHMARK(BM_UpdateStateMachine_RollbackPath) -> Unit(benchmark::kNanosecond)`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_UpdateStateMachine_RollbackPath): n/a

#### `BENCHMARK(BM_UpdateStateMachine_Transition) -> Unit(benchmark::kNanosecond)`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_UpdateStateMachine_Transition): n/a

#### `BENCHMARK_DEFINE_F(DeltaEngineBenchFixture, ApplyPatch)(benchmark`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeltaEngineBenchFixture): n/a
  - `<unnamed>` (ApplyPatch): n/a

#### `BENCHMARK_DEFINE_F(DeltaEngineBenchFixture, GeneratePatch)(benchmark`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeltaEngineBenchFixture): n/a
  - `<unnamed>` (GeneratePatch): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:200
- Brief: n/a
- Parameters: none

#### `void BM_ReleaseManifest_JsonRoundTrip(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:91
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_UpdateStateMachine_CurrentState(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:54
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_UpdateStateMachine_RollbackPath(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:72
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_UpdateStateMachine_Transition(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:32
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMicrosecond) -> Iterations(200)`
- Source: `benchmarks/maintenance/bench_update_pipeline.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

### themis::bench::mtn

#### `void BM_MTN01_ErrorEnumCast(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_maintenance_release_gates.cpp`:66
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MTN02_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_maintenance_release_gates.cpp`:83
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MTN03_StructAlloc(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_maintenance_release_gates.cpp`:122
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MTN04_BatchCast(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_maintenance_release_gates.cpp`:141
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MTN05_InFlightGuard(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_maintenance_release_gates.cpp`:180
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MTN06_PersistReloadRoundTrip(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_maintenance_release_gates.cpp`:246
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MTN07_RingBufferWrite(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_maintenance_release_gates.cpp`:297
- Brief: Measures the cost of a single DispatchOutcome push to the ring buffer (mutex + deque push_back + conditional pop_front).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Gate: p99 ≤ 200 ns.

#### `void BM_MTN_DIST01_LeaderGatedDispatch(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_maintenance_distributed_gates.cpp`:72
- Brief: Simulates the hot path for leader-gated schedule dispatch:
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Acquire a mock distributed lock (std::mutex + bool flag). Check whether this node is the leader. If leader, dispatch (increment counter as a proxy for real work). Release lock. Gate: p99 ≤ 500 µs.

#### `void BM_MTN_DIST02_ScheduleListing(benchmark::State &state)`
- Source: `benchmarks/maintenance/bench_maintenance_distributed_gates.cpp`:104
- Brief: Measures the cost of listing 1000 maintenance schedules from an unordered_map (the listSchedules() hot path in the orchestrator).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Gate: p99 ≤ 5 ms.

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/maintenance/bench_maintenance_distributed_gates.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

### themis::bench::mtn::themis::maintenance

#### `std::string dispatchOutcomeTypeToString(DispatchOutcomeType t)`
- Source: `include/maintenance/maintenance_health_report.h`:75
- Brief: n/a
- Parameters:
  - `t` (DispatchOutcomeType): n/a

#### `std::string moduleHealthStatusToString(ModuleHealthStatus s)`
- Source: `include/maintenance/maintenance_health_report.h`:24
- Brief: n/a
- Parameters:
  - `s` (ModuleHealthStatus): n/a

### themis::bench::mtn::themis::maintenance::DispatchOutcome

#### `nlohmann::json toJson() const`
- Source: `include/maintenance/maintenance_health_report.h`:104
- Brief: n/a
- Parameters: none

### themis::bench::mtn::themis::maintenance::MaintenanceHealthReport

#### `nlohmann::json toJson() const`
- Source: `include/maintenance/maintenance_health_report.h`:142
- Brief: n/a
- Parameters: none

### themis::bench::mtn::themis::maintenance::ModuleHealthSignal

#### `nlohmann::json toJson() const`
- Source: `include/maintenance/maintenance_health_report.h`:45
- Brief: n/a
- Parameters: none

### themis::benchmarks::docker_raid

#### `Args({3, 0, 0}) -> Args({4, 5, 0}) ->Args({6, 10, 0}) ->Iterations(5) ->UseRealTime() ->Unit(::benchmark::kMillisecond)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:959
- Brief: n/a
- Parameters:
  - `<unnamed>` ({3, 0, 0}): n/a

#### `Args({3, 0, 100}) -> Args({3, 0, 1000}) ->Args({6, 0, 100}) ->Args({6, 0, 1000}) ->Args({3, 1, 100}) ->Args({3, 1, 1000}) ->Args({6, 1, 100}) ->Args({4, 5, 100}) ->Args({4, 5, 1000}) ->Args({6, 5, 100}) ->Args({4, 10, 100}) ->Args({6, 10, 1000}) ->MinTime(10.0) ->Unit(::benchmark::kMillisecond)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:559
- Brief: n/a
- Parameters:
  - `<unnamed>` ({3, 0, 100}): n/a

#### `Args({3, 0, 10}) -> Args({4, 5, 10}) ->Args({6, 1, 5}) ->Args({6, 10, 20}) ->MinTime(20.0) ->UseRealTime() ->Unit(::benchmark::kSecond)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:632
- Brief: n/a
- Parameters:
  - `<unnamed>` ({3, 0, 10}): n/a

#### `Args({3, 0, 4}) -> Args({3, 0, 8}) ->Args({4, 5, 4}) ->Args({6, 1, 8}) ->Args({6, 10, 16}) ->MinTime(15.0) ->UseRealTime() ->Unit(::benchmark::kMillisecond)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:767
- Brief: n/a
- Parameters:
  - `<unnamed>` ({3, 0, 4}): n/a

#### `Args({3, 0, 50}) -> Args({3, 0, 500}) ->Args({4, 5, 50}) ->Args({4, 5, 500}) ->Args({6, 1, 50}) ->Args({6, 10, 500}) ->MinTime(15.0) ->UseRealTime() ->Unit(::benchmark::kMillisecond)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:600
- Brief: n/a
- Parameters:
  - `<unnamed>` ({3, 0, 50}): n/a

#### `Args({3, 0, 90}) -> Args({3, 0, 70}) ->Args({3, 0, 50}) ->Args({4, 5, 90}) ->Args({6, 1, 70}) ->Args({6, 10, 50}) ->MinTime(18.0) ->Unit(::benchmark::kMillisecond)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:819
- Brief: n/a
- Parameters:
  - `<unnamed>` ({3, 0, 90}): n/a

#### `Args({3, 1, 0}) -> Args({4, 5, 0}) ->Args({6, 6, 0}) ->Args({6, 10, 0}) ->Iterations(10) ->UseRealTime() ->Unit(::benchmark::kMillisecond)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:726
- Brief: n/a
- Parameters:
  - `<unnamed>` ({3, 1, 0}): n/a

#### `Args({3, 1, 100}) -> Args({4, 5, 100}) ->Args({6, 6, 100}) ->MinTime(12.0) ->Unit(::benchmark::kMicrosecond)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:863
- Brief: n/a
- Parameters:
  - `<unnamed>` ({3, 1, 100}): n/a

#### `state SetBytesProcessed(state.iterations() *batch_size *SMALL_DOCUMENT_SIZE)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:551
- Brief: n/a
- Parameters:
  - `SMALL_DOCUMENT_SIZE` (state.iterations() *batch_size *): n/a

#### `state SetBytesProcessed(state.iterations() *num_blobs *BLOB_SIZE)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:626
- Brief: n/a
- Parameters:
  - `BLOB_SIZE` (state.iterations() *num_blobs *): n/a

#### `state SetItemsProcessed(state.iterations() *(initial_documents/10))`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:955
- Brief: n/a
- Parameters:
  - `iterations` (state.): n/a

#### `state SetItemsProcessed(state.iterations() *queries_per_iteration *num_containers_)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:904
- Brief: n/a
- Parameters:
  - `num_containers_` (state.iterations() *queries_per_iteration *): n/a

#### `state SetItemsProcessed(state.iterations() *reads_per_iteration)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:667
- Brief: n/a
- Parameters:
  - `reads_per_iteration` (state.iterations() *): n/a

#### `state SetItemsProcessed(state.iterations() *threads *50)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:762
- Brief: n/a
- Parameters:
  - `50` (state.iterations() *threads *): n/a

#### `std::uniform_int_distribution< int > dist(0, num_documents - 1)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:655
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a
  - `1` (num_documents -): n/a

#### `std::uniform_int_distribution< int > doc_dist(0, num_documents - 1)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:791
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a
  - `1` (num_documents -): n/a

#### `for(auto _ :state)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:544
- Brief: n/a
- Parameters:
  - `<unnamed>` (auto _ :state): n/a

#### `for(int i=0;i< num_documents;++i)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:649
- Brief: n/a
- Parameters: none

#### `std::uniform_real_distribution< double > op_dist(0.0, 1.0)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:792
- Brief: n/a
- Parameters:
  - `0` (0.): n/a
  - `0` (1.): n/a

#### `std::mt19937 rng(42)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:654
- Brief: n/a
- Parameters:
  - `<unnamed>` (42): n/a

### themis::benchmarks::docker_raid::DockerContainer

#### `DockerContainer(const DockerContainerConfig &config)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:116
- Brief: n/a
- Parameters:
  - `config` (const DockerContainerConfig &): n/a

#### `size_t getBytesRead() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:169
- Brief: n/a
- Parameters: none

#### `size_t getBytesWritten() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:168
- Brief: n/a
- Parameters: none

#### `const DockerContainerConfig & getConfig() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:143
- Brief: n/a
- Parameters: none

#### `size_t getDataCount() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:163
- Brief: n/a
- Parameters: none

#### `std::string getId() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:141
- Brief: n/a
- Parameters: none

#### `bool isRunning() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:139
- Brief: n/a
- Parameters: none

#### `std::vector< uint8_t > readData(size_t index) const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:154
- Brief: n/a
- Parameters:
  - `index` (size_t): n/a

#### `bool restart()`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:135
- Brief: n/a
- Parameters: none

#### `bool start()`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:119
- Brief: n/a
- Parameters: none

#### `bool stop()`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:129
- Brief: n/a
- Parameters: none

#### `bool writeData(const std::vector< uint8_t > &data)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:146
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a

### themis::benchmarks::docker_raid::DockerRAIDBenchmarkBase

#### `void SetUp(::benchmark::State &state) override`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:507
- Brief: n/a
- Parameters:
  - `state` (::benchmark::State &): n/a

#### `void TearDown(::benchmark::State &state) override`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:522
- Brief: n/a
- Parameters:
  - `state` (::benchmark::State &): n/a

### themis::benchmarks::docker_raid::DockerRAIDController

#### `DockerRAIDController(RAIDLevel level, int num_containers)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:187
- Brief: n/a
- Parameters:
  - `level` (RAIDLevel): n/a
  - `num_containers` (int): n/a

#### `std::vector< uint8_t > computeDualParity(const std::vector< uint8_t > &data)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:419
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a

#### `std::vector< uint8_t > computeParity(const std::vector< uint8_t > &data)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:411
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a

#### `bool failContainer(int index)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:249
- Brief: n/a
- Parameters:
  - `index` (int): n/a

#### `int getNumContainers() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:287
- Brief: n/a
- Parameters: none

#### `int getNumFailedContainers() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:289
- Brief: n/a
- Parameters: none

#### `size_t getTotalBytesRead() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:279
- Brief: n/a
- Parameters: none

#### `size_t getTotalBytesWritten() const`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:271
- Brief: n/a
- Parameters: none

#### `void initializeContainers()`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:292
- Brief: n/a
- Parameters: none

#### `std::vector< uint8_t > readData(int stripe_id, size_t offset)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:231
- Brief: n/a
- Parameters:
  - `stripe_id` (int): n/a
  - `offset` (size_t): n/a

#### `std::vector< uint8_t > readRAID0(int stripe_id, size_t offset)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:315
- Brief: n/a
- Parameters:
  - `stripe_id` (int): n/a
  - `offset` (size_t): n/a

#### `std::vector< uint8_t > readRAID1(size_t offset)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:334
- Brief: n/a
- Parameters:
  - `offset` (size_t): n/a

#### `std::vector< uint8_t > readRAID10(int stripe_id, size_t offset)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:399
- Brief: n/a
- Parameters:
  - `stripe_id` (int): n/a
  - `offset` (size_t): n/a

#### `std::vector< uint8_t > readRAID5(int stripe_id, size_t offset)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:356
- Brief: n/a
- Parameters:
  - `stripe_id` (int): n/a
  - `offset` (size_t): n/a

#### `std::vector< uint8_t > readRAID6(int stripe_id, size_t offset)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:380
- Brief: n/a
- Parameters:
  - `stripe_id` (int): n/a
  - `offset` (size_t): n/a

#### `std::vector< uint8_t > reconstructFromDualParity(int failed_idx, size_t offset)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:433
- Brief: n/a
- Parameters:
  - `failed_idx` (int): n/a
  - `offset` (size_t): n/a

#### `std::vector< uint8_t > reconstructFromParity(int failed_idx, size_t offset)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:427
- Brief: n/a
- Parameters:
  - `failed_idx` (int): n/a
  - `offset` (size_t): n/a

#### `bool recoverContainer(int index)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:259
- Brief: n/a
- Parameters:
  - `index` (int): n/a

#### `bool shutdown()`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:205
- Brief: n/a
- Parameters: none

#### `bool start()`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:196
- Brief: n/a
- Parameters: none

#### `bool writeData(const std::vector< uint8_t > &data, int stripe_id)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:213
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `stripe_id` (int): n/a

#### `bool writeRAID0(const std::vector< uint8_t > &data, int stripe_id)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:310
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `stripe_id` (int): n/a

#### `bool writeRAID1(const std::vector< uint8_t > &data)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:324
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a

#### `bool writeRAID10(const std::vector< uint8_t > &data, int stripe_id)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:391
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `stripe_id` (int): n/a

#### `bool writeRAID5(const std::vector< uint8_t > &data, int stripe_id)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:345
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `stripe_id` (int): n/a

#### `bool writeRAID6(const std::vector< uint8_t > &data, int stripe_id)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:368
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `stripe_id` (int): n/a

#### `~DockerRAIDController()`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:192
- Brief: n/a
- Parameters: none

### themis::benchmarks::docker_raid::TestDataGenerator

#### `std::vector< uint8_t > generateData(size_t size, uint64_t seed=0)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:450
- Brief: n/a
- Parameters:
  - `size` (size_t): n/a
  - `seed` (uint64_t): n/a

#### `std::string generateDocumentJSON(size_t target_size)`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:461
- Brief: n/a
- Parameters:
  - `target_size` (size_t): n/a

#### `std::string generateUUID()`
- Source: `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp`:482
- Brief: n/a
- Parameters: none

### themis::maintenance

#### `MaintenanceScheduleEntry defaultDailySchedule()`
- Source: `src/maintenance/maintenance_registry.cpp`:31
- Brief: Default Daily Schedule.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaultDailySchedule without additional internal calls.

#### `MaintenanceScheduleEntry defaultMonthlySchedule()`
- Source: `src/maintenance/maintenance_registry.cpp`:77
- Brief: Default Monthly Schedule.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaultMonthlySchedule without additional internal calls.

#### `MaintenanceScheduleEntry defaultQuarterlySchedule()`
- Source: `src/maintenance/maintenance_registry.cpp`:101
- Brief: Default Quarterly Schedule.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaultQuarterlySchedule without additional internal calls.

#### `MaintenanceScheduleEntry defaultWeeklySchedule()`
- Source: `src/maintenance/maintenance_registry.cpp`:53
- Brief: Default Weekly Schedule.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaultWeeklySchedule without additional internal calls.

#### `std::string dispatchOutcomeTypeToString(DispatchOutcomeType t)`
- Source: `include/maintenance/maintenance_health_report.h`:74
- Brief: n/a
- Parameters:
  - `t` (DispatchOutcomeType): n/a

#### `ScheduleFrequency frequencyFromString(const std::string &s)`
- Source: `include/maintenance/maintenance_schedule.h`:50
- Brief: n/a
- Parameters:
  - `s` (const std::string &): n/a

#### `std::string frequencyToCron(ScheduleFrequency frequency, int window_start_hour=2)`
- Source: `include/maintenance/maintenance_schedule.h`:69
- Brief: n/a
- Parameters:
  - `frequency` (ScheduleFrequency): Must not be CUSTOM.
  - `window_start_hour` (int): Hour-of-day (0–23) for the maintenance window.
- Details: Returns the canonical cron expression for a built-in frequency. frequency Must not be CUSTOM. window_start_hour Hour-of-day (0–23) for the maintenance window.

#### `std::string frequencyToString(ScheduleFrequency f)`
- Source: `include/maintenance/maintenance_schedule.h`:39
- Brief: n/a
- Parameters:
  - `f` (ScheduleFrequency): n/a

#### `bool isMaintenanceFailClosed(MaintenanceError e) noexcept`
- Source: `include/maintenance/maintenance_api_contract.h`:170
- Brief: Returns true when the given error mandates fail-closed denial of dispatch.
- Parameters:
  - `e` (MaintenanceError): n/a
- Details: Callers MUST check this before scheduling further work when handling an error.

#### `std::string jobStateToString(MaintenanceJobState s)`
- Source: `include/maintenance/maintenance_task.h`:165
- Brief: n/a
- Parameters:
  - `s` (MaintenanceJobState): n/a

#### `HealthProbe makeIndexMaintenanceHealthProbe(std::shared_ptr< IndexMaintenanceManager > mgr)`
- Source: `src/maintenance/maintenance_registry.cpp`:126
- Brief: Make Index Maintenance Health Probe.
- Parameters:
  - `mgr` (std::shared_ptr< IndexMaintenanceManager >): Input parameter.
- Return: Return value.
- Details: mgr Input parameter. Return value.

#### `std::shared_ptr< ReplicaValidationHandler > makeReplicaValidationHandler(std::shared_ptr< themis::sharding::ShardRepairEngine > engine)`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:251
- Brief: n/a
- Parameters:
  - `engine` (std::shared_ptr< themis::sharding::ShardRepairEngine >): n/a

#### `std::string moduleHealthStatusToString(ModuleHealthStatus s)`
- Source: `include/maintenance/maintenance_health_report.h`:23
- Brief: n/a
- Parameters:
  - `s` (ModuleHealthStatus): n/a

#### `void registerDefaultMaintenanceSetup(DatabaseMaintenanceOrchestrator &orchestrator, std::shared_ptr< IndexMaintenanceManager > index_mgr)`
- Source: `src/maintenance/maintenance_registry.cpp`:176
- Brief: Register Default Maintenance Setup.
- Parameters:
  - `orchestrator` (DatabaseMaintenanceOrchestrator &): Input/output parameter.
  - `index_mgr` (std::shared_ptr< IndexMaintenanceManager >): Input parameter.
- Details: orchestrator Input/output parameter. index_mgr Input parameter.

#### `MaintenanceTaskType taskTypeFromString(const std::string &s)`
- Source: `include/maintenance/maintenance_task.h`:88
- Brief: n/a
- Parameters:
  - `s` (const std::string &): n/a
- Details: Parse a MaintenanceTaskType from its string representation. Returns MaintenanceTaskType::METRICS_COLLECTION if not recognized.

#### `std::string taskTypeToString(MaintenanceTaskType t)`
- Source: `include/maintenance/maintenance_task.h`:61
- Brief: Convert a MaintenanceTaskType to its string representation.
- Parameters:
  - `t` (MaintenanceTaskType): n/a

### themis::maintenance::DatabaseMaintenanceOrchestrator

#### `DatabaseMaintenanceOrchestrator(TaskScheduler *scheduler, std::shared_ptr< IndexMaintenanceManager > index_maintenance=nullptr, std::shared_ptr< utils::AuditLogger > audit_logger=nullptr, IStorageEngine *storage=nullptr)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:129
- Brief: Construct the orchestrator.
- Parameters:
  - `scheduler` (TaskScheduler *): TaskScheduler used for cron-based scheduling. Must outlive this object.
  - `index_maintenance` (std::shared_ptr< IndexMaintenanceManager >): IndexMaintenanceManager for index operations. May be nullptr; index operations will return an error in that case.
  - `audit_logger` (std::shared_ptr< utils::AuditLogger >): Optional audit logger. May be nullptr.
  - `storage` (IStorageEngine *): Optional storage engine for schedule persistence (RocksDB via StorageEngine). When non-null, schedules are written through on every CRUD mutation and reloaded on start(). May be nullptr for in-memory-only operation (e.g., in tests).
- Details: scheduler TaskScheduler used for cron-based scheduling. Must outlive this object. index_maintenance IndexMaintenanceManager for index operations. May be nullptr; index operations will return an error in that case. audit_logger Optional audit logger. May be nullptr. storage Optional storage engine for schedule persistence (RocksDB via StorageEngine). When non-null, schedules are written through on every CRUD mutation and reloaded on start(). May be nullptr for in-memory-only operation (e.g., in tests).

#### `DatabaseMaintenanceOrchestrator(const DatabaseMaintenanceOrchestrator &)=delete`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DatabaseMaintenanceOrchestrator &): n/a

#### `Result< void > cancelJob(const std::string &job_id)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:267
- Brief: Cancel a running job.
- Parameters:
  - `job_id` (const std::string &): Identifier of the job.
- Return: Result<void> or error if the job is not found / not running.
- Details: Cancel Job. Signals the job's execution thread to abort. Already-completed tasks within the job are not rolled back. job_id ID of the job to cancel. Result<void> or error if the job is not found / not running. job_id Identifier of the job. Return value. Calls: lock(), find(), end(), tl::unexpected(), Error(), nowMs(), spdlog::warn(), logEvent().

#### `bool checkChurnLimit(const std::string &schedule_id, uint32_t max_changes_per_interval)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:426
- Brief: ------------------------------------------------------------------------ Phase 2: Churn rate-limit check ------------------------------------------------------------------------
- Parameters:
  - `schedule_id` (const std::string &): Identifier of the schedule.
  - `max_changes_per_interval` (uint32_t): Input parameter.
- Return: True when the operation succeeds.
- Details: Check and enforce churn rate limit for a schedule. Returns true if the change is permitted; false if the limit is exceeded. schedule_id Identifier of the schedule. max_changes_per_interval Input parameter. True when the operation succeeds.

#### `Result< MaintenanceScheduleEntry > createSchedule(MaintenanceScheduleEntry entry)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:181
- Brief: Create a new maintenance schedule.
- Parameters:
  - `entry` (MaintenanceScheduleEntry): Input parameter.
- Return: Result with the persisted entry (including assigned id) or error.
- Details: Create Schedule. Assigns a UUID, timestamps created_at_ms, derives cron_expression from frequency (unless CUSTOM), and registers the task in the TaskScheduler if the orchestrator is running and the schedule is enabled. Validation: name must be non-empty. tasks must contain at least one entry. For CUSTOM frequency, cron_expression must be non-empty. window_start_hour and window_end_hour must be in [0, 23]. entry Schedule definition. id, created_at_ms, updated_at_ms, and runtime fields (last_run_ms, last_job_id, …) are ignored on input and set by the orchestrator. Result with the persisted entry (including assigned id) or error. entry Input parameter. Return value.

#### `Result< void > deleteSchedule(const std::string &id)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:238
- Brief: Delete a schedule.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Return: Result<void> or error if not found.
- Details: Delete Schedule. Deregisters the corresponding task from the TaskScheduler and removes the entry from the registry. Active jobs spawned by this schedule are allowed to complete. id ID of the schedule to delete. Result<void> or error if not found. id Input parameter. Return value. Calls: lock(), find(), end(), tl::unexpected(), Error(), remove(), has_value(), error().

#### `void deregisterFromScheduler(const std::string &schedule_id)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:409
- Brief: Deregister From Scheduler.
- Parameters:
  - `schedule_id` (const std::string &): Identifier of the schedule.
- Details: schedule_id Identifier of the schedule.

#### `void executeSchedule(const std::string &schedule_id, const std::string &job_id, bool force=false)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:412
- Brief: Execute Schedule.
- Parameters:
  - `schedule_id` (const std::string &): Identifier of the schedule.
  - `job_id` (const std::string &): Identifier of the job.
  - `force` (bool): Input parameter.
- Details: schedule_id Identifier of the schedule. job_id Identifier of the job. force Input parameter.

#### `void executeTask(MaintenanceTaskType task_type, OrchestratorJob &job)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:414
- Brief: Execute Task.
- Parameters:
  - `task_type` (MaintenanceTaskType): Input parameter.
  - `job` (OrchestratorJob &): Input/output parameter.
- Details: task_type Input parameter. job Input/output parameter.

#### `std::string generateId() const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:405
- Brief: n/a
- Parameters: none

#### `MaintenanceHealthReport getHealthReport() const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:301
- Brief: Aggregate and return a health report from all registered modules.
- Parameters: none
- Details: Calls each registered HealthProbe synchronously, combines the signals, and derives the overall status.

#### `Result< OrchestratorJob > getJob(const std::string &job_id) const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:274
- Brief: Get the current status of a job.
- Parameters:
  - `job_id` (const std::string &): Job identifier.
- Return: Result with the OrchestratorJob or error if not found.
- Details: job_id Job identifier. Result with the OrchestratorJob or error if not found.

#### `Result< MaintenanceScheduleEntry > getSchedule(const std::string &id) const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:187
- Brief: Retrieve a schedule by ID.
- Parameters:
  - `id` (const std::string &): n/a
- Return: Result with the entry or error if not found.
- Details: Result with the entry or error if not found.

#### `nlohmann::json getStatus() const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:293
- Brief: Return a structured status snapshot of the orchestrator.
- Parameters: none
- Details: Includes version, running state, schedule counts, active job count, and the time of the most recently completed job.

#### `TenantMaintenanceConfig getTenantMaintenanceConfig(const std::string &tenant_id) const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:328
- Brief: Retrieve the per-tenant maintenance configuration.
- Parameters:
  - `tenant_id` (const std::string &): Tenant identifier.
- Return: The registered TenantMaintenanceConfig, or a default-constructed one (enforce_window=false, max_concurrent_jobs=0) if none is registered.
- Details: tenant_id Tenant identifier. The registered TenantMaintenanceConfig, or a default-constructed one (enforce_window=false, max_concurrent_jobs=0) if none is registered.

#### `bool isRunning() const noexcept`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:159
- Brief: n/a
- Parameters: none

#### `std::vector< OrchestratorJob > listJobs(bool active_only=false) const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:283
- Brief: List all known jobs (active + recently completed).
- Parameters:
  - `active_only` (bool): When true, only PENDING and RUNNING jobs are returned.
- Details: Completed jobs are retained for a configurable TTL (default 24 h). active_only When true, only PENDING and RUNNING jobs are returned.

#### `std::vector< MaintenanceScheduleEntry > listSchedules(const std::string &tenant_id_filter="") const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:197
- Brief: List schedules, optionally filtered by tenant.
- Parameters:
  - `tenant_id_filter` (const std::string &): When non-empty, only schedules whose tenant_id equals this value are returned. An empty string returns all schedules (global + all tenants).
- Return: Vector of matching schedule entries.
- Details: tenant_id_filter When non-empty, only schedules whose tenant_id equals this value are returned. An empty string returns all schedules (global + all tenants). Vector of matching schedule entries.

#### `std::map< std::string, std::string > listTaskHandlers() const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:387
- Brief: Return a map of registered task handlers.
- Parameters: none
- Details: Keys are the string representation of the task type (see taskTypeToString()). Values are the handler names (IMaintenanceTaskHandler::handlerName()). Used by GET /api/v1/maintenance/task-handlers to let operators diagnose which task types have no handler registered.

#### `int64_t nowMs() const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:406
- Brief: n/a
- Parameters: none

#### `DatabaseMaintenanceOrchestrator & operator=(const DatabaseMaintenanceOrchestrator &)=delete`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DatabaseMaintenanceOrchestrator &): n/a

#### `Result< MaintenanceScheduleEntry > patchSchedule(const std::string &id, const nlohmann::json &patch)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:225
- Brief: Partial update of a schedule (PATCH semantics).
- Parameters:
  - `id` (const std::string &): Input parameter.
  - `patch` (const nlohmann::json &): Input parameter.
- Return: Result with the updated entry or error if not found / invalid.
- Details: Patch Schedule. Only the JSON fields present in patch are applied; absent fields retain their current values. See MaintenanceScheduleEntry::applyPatch. id ID of the schedule to patch. patch JSON object with only the fields to change. Result with the updated entry or error if not found / invalid. id Input parameter. patch Input parameter. Return value.

#### `void pruneCompletedJobs()`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:417
- Brief: Prune Completed Jobs.
- Parameters: none
- Details: Calls: themis::maintenance::nowMs(), lock(), begin(), end(), erase().

#### `void recordDispatchOutcome(DispatchOutcome outcome)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:422
- Brief: Record a DispatchOutcome into the ring buffer (thread-safe).
- Parameters:
  - `outcome` (DispatchOutcome): Input parameter.
- Details: ------------------------------------------------------------------------ Phase 4: DispatchOutcome ring buffer ------------------------------------------------------------------------ outcome Input parameter. Calls: lock(), push_back(), std::move(), size(), pop_front().

#### `void registerHealthProbe(const std::string &module_name, HealthProbe probe)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:342
- Brief: Register a health probe for a module.
- Parameters:
  - `module_name` (const std::string &): Name of the module.
  - `probe` (HealthProbe): Input parameter.
- Details: Register Health Probe. Modules call this once at startup to contribute their health signal to the aggregated report returned by getHealthReport(). module_name Human-readable module name (e.g. "index_maintenance"). probe Callable returning a ModuleHealthSignal. module_name Name of the module. probe Input parameter.

#### `void registerTaskHandler(MaintenanceTaskType task_type, std::shared_ptr< IMaintenanceTaskHandler > handler)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:374
- Brief: Register a task handler for a specific task type.
- Parameters:
  - `task_type` (MaintenanceTaskType): Input parameter.
  - `handler` (std::shared_ptr< IMaintenanceTaskHandler >): Input parameter.
- Details: Register Task Handler. Modules call this to wire real execution logic into the orchestrator for their task type(s). A previously registered handler for the same type is silently replaced. Thread-safe. task_type The task type this handler owns. handler Non-null shared pointer to the handler implementation. task_type Input parameter. handler Input parameter.

#### `void registerWithScheduler(const MaintenanceScheduleEntry &entry)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:408
- Brief: Register With Scheduler.
- Parameters:
  - `entry` (const MaintenanceScheduleEntry &): Input parameter.
- Details: entry Input parameter.

#### `std::vector< MaintenanceTaskType > resolveTaskExecutionOrder(const MaintenanceScheduleEntry &entry)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:399
- Brief: Resolve task execution order from task_dependencies using topological sort.
- Parameters:
  - `entry` (const MaintenanceScheduleEntry &): n/a
- Details: When task_dependencies is empty, returns entry.tasks in positional order. Otherwise performs a stable Kahn's-algorithm topological sort seeded in entry.tasks order, so unrelated tasks preserve their original relative position. Throws std::invalid_argument if: any task_type or depends_on reference is not present in entry.tasks, or a cycle is detected.

#### `std::string schedulerTaskId(const std::string &schedule_id) const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:410
- Brief: n/a
- Parameters:
  - `schedule_id` (const std::string &): n/a

#### `void setDistributedLock(std::shared_ptr< IDistributedLock > lock)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:362
- Brief: Inject a distributed lock implementation.
- Parameters:
  - `lock` (std::shared_ptr< IDistributedLock >): Input parameter.
- Details: Set Distributed Lock. When set, the orchestrator calls dist_lock_->tryAcquire(schedule_id, ttl_ms) before firing each scheduled job. Only the cluster node that successfully acquires the lock runs the job; all other nodes log a DEBUG message and mark the job as SKIPPED. The TTL is derived from MaintenanceScheduleEntry::lock_ttl_ms when non-zero, otherwise it is computed as the maintenance-window duration plus a 30-second safety margin. This method is thread-safe. Pass nullptr to disable distributed locking (single-node or test deployments). lock Shared pointer to the IDistributedLock implementation, or nullptr to disable distributed locking. lock Input parameter.

#### `void setTenantMaintenanceConfig(const std::string &tenant_id, TenantMaintenanceConfig config)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:318
- Brief: Register or update per-tenant maintenance configuration.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
  - `config` (TenantMaintenanceConfig): Input parameter.
- Details: Set Tenant Maintenance Config. Affects all schedules whose tenant_id matches tenant_id: If config.enforce_window is true, the tenant-level window overrides the per-schedule window during executeSchedule(). If config.max_concurrent_jobs > 0, the orchestrator enforces the quota in executeSchedule() and SKIPs the job when the limit is reached. Call with a default-constructed TenantMaintenanceConfig (enforce_window=false, max_concurrent_jobs=0) to effectively remove constraints for a tenant. tenant_id Tenant identifier. Must not be empty. config Configuration to apply for this tenant. tenant_id Identifier of the tenant. config Input parameter.

#### `Result< void > start()`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:149
- Brief: Start the orchestrator.
- Parameters: none
- Return: Result<void> – error if the scheduler is not available.
- Details: Start. Re-registers all enabled schedules in the TaskScheduler and begins accepting ad-hoc job submissions. Result<void> – error if the scheduler is not available. Return value. Calls: load(), loadAll(), has_value(), spdlog::error(), error(), message(), lock(), insert_or_assign().

#### `void stop()`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:157
- Brief: Stop the orchestrator.
- Parameters: none
- Details: Stop. Deregisters all schedules from the TaskScheduler. Running jobs are allowed to complete. Calls: exchange(), lock(), deregisterFromScheduler(), spdlog::info().

#### `Result< OrchestratorJob > triggerNow(const std::string &schedule_id, bool force=false)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:255
- Brief: Immediately trigger a schedule regardless of its cron time.
- Parameters:
  - `schedule_id` (const std::string &): Identifier of the schedule.
  - `force` (bool): Input parameter.
- Return: Result with the newly created job (in RUNNING state) or error.
- Details: Trigger Now. Creates an ad-hoc OrchestratorJob and runs the tasks synchronously in a background thread. schedule_id ID of the schedule to run now. force When true, bypass the UTC maintenance window check. Requires maintenance:admin scope at the API layer. The resulting job has forced=true and the audit log entry carries "forced": true. Result with the newly created job (in RUNNING state) or error. schedule_id Identifier of the schedule. force Input parameter. Return value.

#### `Result< MaintenanceScheduleEntry > updateSchedule(const std::string &id, MaintenanceScheduleEntry entry)`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:212
- Brief: Full-replace update of a schedule (PUT semantics).
- Parameters:
  - `id` (const std::string &): Input parameter.
  - `entry` (MaintenanceScheduleEntry): Input parameter.
- Return: Result with the updated entry or error if not found / invalid.
- Details: Update Schedule. Replaces all mutable fields with the values from entry. Read-only fields (created_at_ms, created_by) are preserved. The TaskScheduler registration is updated accordingly. id ID of the schedule to update. entry New schedule definition. The id field is ignored; the path parameter id is authoritative. Result with the updated entry or error if not found / invalid. id Input parameter. entry Input parameter. Return value.

#### `void validateEntry(const MaintenanceScheduleEntry &entry) const`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:419
- Brief: n/a
- Parameters:
  - `entry` (const MaintenanceScheduleEntry &): n/a

#### `~DatabaseMaintenanceOrchestrator()`
- Source: `include/maintenance/database_maintenance_orchestrator.h`:135
- Brief: n/a
- Parameters: none

### themis::maintenance::DispatchOutcome

#### `nlohmann::json toJson() const`
- Source: `include/maintenance/maintenance_health_report.h`:103
- Brief: n/a
- Parameters: none

### themis::maintenance::FunctionMaintenanceTaskHandler

#### `FunctionMaintenanceTaskHandler(std::string name, ExecuteFn fn)`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:196
- Brief: n/a
- Parameters:
  - `name` (std::string): n/a
  - `fn` (ExecuteFn): n/a

#### `Result< std::string > execute(const std::string &job_id, MaintenanceTaskType task_type) override`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:199
- Brief: Execute the maintenance task.
- Parameters:
  - `job_id` (const std::string &): The orchestrator job identifier (for logging/tracing).
  - `task_type` (MaintenanceTaskType): The task type being executed.
- Return: Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).
- Details: job_id The orchestrator job identifier (for logging/tracing). task_type The task type being executed. Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).

#### `std::string handlerName() const override`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:209
- Brief: Human-readable handler name for diagnostics.
- Parameters: none
- Details: Returned by GET /api/v1/maintenance/task-handlers. Should be unique and stable across restarts (e.g. "StorageCompactionHandler").

### themis::maintenance::IDistributedLock

#### `std::string getHolderNodeId(const std::string &key) const =0`
- Source: `include/maintenance/i_distributed_lock.h`:67
- Brief: Return the node ID currently holding the lock for key.
- Parameters:
  - `key` (const std::string &): n/a
- Return: Node ID string, or empty string if the lock is not currently held (e.g. expired or never acquired).
- Details: Used only for logging ("schedule {id} skipped — lock held by peer {node_id}"). Node ID string, or empty string if the lock is not currently held (e.g. expired or never acquired).

#### `std::string nodeId() const =0`
- Source: `include/maintenance/i_distributed_lock.h`:72
- Brief: Return the identifier of this node.
- Parameters: none

#### `void release(const std::string &key)=0`
- Source: `include/maintenance/i_distributed_lock.h`:57
- Brief: Release the lock for key previously acquired by this node.
- Parameters:
  - `key` (const std::string &): Lock key to release.
- Details: A no-op when this node does not currently hold the lock. key Lock key to release.

#### `bool tryAcquire(const std::string &key, int64_t ttl_ms)=0`
- Source: `include/maintenance/i_distributed_lock.h`:48
- Brief: Try to acquire the lock for key with the given TTL.
- Parameters:
  - `key` (const std::string &): Lock key; typically the schedule ID.
  - `ttl_ms` (int64_t): Time-to-live in milliseconds. The lock is automatically released after this duration even if release() is never called — prevents orphaned locks when a node crashes.
- Return: true if this node successfully acquired the lock.
- Details: Non-blocking: returns immediately with true (acquired) or false (held by another node or a prior acquisition by this node has not yet expired). key Lock key; typically the schedule ID. ttl_ms Time-to-live in milliseconds. The lock is automatically released after this duration even if release() is never called — prevents orphaned locks when a node crashes. true if this node successfully acquired the lock.

#### `~IDistributedLock()=default`
- Source: `include/maintenance/i_distributed_lock.h`:33
- Brief: n/a
- Parameters: none

### themis::maintenance::IMaintenanceTaskHandler

#### `Result< std::string > execute(const std::string &job_id, MaintenanceTaskType task_type)=0`
- Source: `include/maintenance/i_maintenance_task_handler.h`:39
- Brief: Execute the maintenance task.
- Parameters:
  - `job_id` (const std::string &): The orchestrator job identifier (for logging/tracing).
  - `task_type` (MaintenanceTaskType): The task type being executed.
- Return: Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).
- Details: job_id The orchestrator job identifier (for logging/tracing). task_type The task type being executed. Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).

#### `std::string handlerName() const =0`
- Source: `include/maintenance/i_maintenance_task_handler.h`:48
- Brief: Human-readable handler name for diagnostics.
- Parameters: none
- Details: Returned by GET /api/v1/maintenance/task-handlers. Should be unique and stable across restarts (e.g. "StorageCompactionHandler").

#### `~IMaintenanceTaskHandler()=default`
- Source: `include/maintenance/i_maintenance_task_handler.h`:28
- Brief: n/a
- Parameters: none

### themis::maintenance::InProcessDistributedLock

#### `InProcessDistributedLock(std::string node_id="local-node")`
- Source: `include/maintenance/i_distributed_lock.h`:90
- Brief: n/a
- Parameters:
  - `node_id` (std::string): n/a

#### `std::string getHolderNodeId(const std::string &key) const override`
- Source: `include/maintenance/i_distributed_lock.h`:122
- Brief: Return the node ID currently holding the lock for key.
- Parameters:
  - `key` (const std::string &): n/a
- Return: Node ID string, or empty string if the lock is not currently held (e.g. expired or never acquired).
- Details: Used only for logging ("schedule {id} skipped — lock held by peer {node_id}"). Node ID string, or empty string if the lock is not currently held (e.g. expired or never acquired).

#### `std::string nodeId() const override`
- Source: `include/maintenance/i_distributed_lock.h`:131
- Brief: Return the identifier of this node.
- Parameters: none

#### `void release(const std::string &key) override`
- Source: `include/maintenance/i_distributed_lock.h`:114
- Brief: Release the lock for key previously acquired by this node.
- Parameters:
  - `key` (const std::string &): Lock key to release.
- Details: A no-op when this node does not currently hold the lock. key Lock key to release.

#### `bool tryAcquire(const std::string &key, int64_t ttl_ms) override`
- Source: `include/maintenance/i_distributed_lock.h`:93
- Brief: Try to acquire the lock for key with the given TTL.
- Parameters:
  - `key` (const std::string &): Lock key; typically the schedule ID.
  - `ttl_ms` (int64_t): Time-to-live in milliseconds. The lock is automatically released after this duration even if release() is never called — prevents orphaned locks when a node crashes.
- Return: true if this node successfully acquired the lock.
- Details: Non-blocking: returns immediately with true (acquired) or false (held by another node or a prior acquisition by this node has not yet expired). key Lock key; typically the schedule ID. ttl_ms Time-to-live in milliseconds. The lock is automatically released after this duration even if release() is never called — prevents orphaned locks when a node crashes. true if this node successfully acquired the lock.

### themis::maintenance::MaintenanceHealthReport

#### `nlohmann::json toJson() const`
- Source: `include/maintenance/maintenance_health_report.h`:141
- Brief: n/a
- Parameters: none

### themis::maintenance::MaintenanceScheduleEntry

#### `void applyPatch(const nlohmann::json &patch)`
- Source: `include/maintenance/maintenance_schedule.h`:348
- Brief: Apply a partial JSON patch (PATCH semantics: only provided fields are updated).
- Parameters:
  - `patch` (const nlohmann::json &): n/a

#### `MaintenanceScheduleEntry fromJson(const nlohmann::json &j)`
- Source: `include/maintenance/maintenance_schedule.h`:267
- Brief: n/a
- Parameters:
  - `j` (const nlohmann::json &): n/a

#### `nlohmann::json toJson() const`
- Source: `include/maintenance/maintenance_schedule.h`:231
- Brief: n/a
- Parameters: none

### themis::maintenance::MaintenanceScheduleStore

#### `MaintenanceScheduleStore(IStorageEngine *engine)`
- Source: `include/maintenance/maintenance_schedule_store.h`:46
- Brief: Construct a store backed by engine.
- Parameters:
  - `engine` (IStorageEngine *): n/a
- Details: engine must outlive this object. The pointer must be non-null.

#### `MaintenanceScheduleStore(const MaintenanceScheduleStore &)=delete`
- Source: `include/maintenance/maintenance_schedule_store.h`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MaintenanceScheduleStore &): n/a

#### `Result< void > loadAll(std::map< std::string, MaintenanceScheduleEntry > &schedules)`
- Source: `include/maintenance/maintenance_schedule_store.h`:89
- Brief: Load all previously persisted schedules into schedules.
- Parameters:
  - `schedules` (std::map< std::string, MaintenanceScheduleEntry > &): Output map keyed by schedule id.
- Return: Result<void> – ok even if some entries were skipped; a hard storage error is returned only when the scan itself fails.
- Details: Scans the "maint_sched::" prefix using IStorageEngine::scanPrefix(). For each entry: • Valid JSON → deserialised and inserted into schedules. • Corrupt / unparseable JSON → WARN log, entry skipped. Existing contents of schedules are not cleared; loaded entries are merged (overwrite on id collision). schedules Output map keyed by schedule id. Result<void> – ok even if some entries were skipped; a hard storage error is returned only when the scan itself fails.

#### `std::string makeKey(const std::string &id)`
- Source: `include/maintenance/maintenance_schedule_store.h`:92
- Brief: Make Key.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value. Calls: reserve(), size(), append().

#### `MaintenanceScheduleStore & operator=(const MaintenanceScheduleStore &)=delete`
- Source: `include/maintenance/maintenance_schedule_store.h`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MaintenanceScheduleStore &): n/a

#### `Result< void > remove(const std::string &id)`
- Source: `include/maintenance/maintenance_schedule_store.h`:72
- Brief: Delete the persisted entry for id.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Return: Result<void> – ok on success, storage error on failure.
- Details: Remove. Calls IStorageEngine::del() for key "maint_sched::<id>". Returns ok even when the key does not exist (idempotent). Result<void> – ok on success, storage error on failure. id Input parameter. Return value. Calls: del(), makeKey().

#### `Result< void > save(const MaintenanceScheduleEntry &entry)`
- Source: `include/maintenance/maintenance_schedule_store.h`:62
- Brief: Persist (or overwrite) a single schedule entry.
- Parameters:
  - `entry` (const MaintenanceScheduleEntry &): Input parameter.
- Return: Result<void> – ok on success, storage error on failure.
- Details: Save. Serialises entry to JSON and calls IStorageEngine::put() with key "maint_sched::<entry.id>". Result<void> – ok on success, storage error on failure. entry Input parameter. Return value. Calls: makeKey(), toJson(), dump(), put().

#### `~MaintenanceScheduleStore()=default`
- Source: `include/maintenance/maintenance_schedule_store.h`:48
- Brief: n/a
- Parameters: none

### themis::maintenance::MaintenanceTaskDependency

#### `MaintenanceTaskDependency fromJson(const nlohmann::json &j)`
- Source: `include/maintenance/maintenance_schedule.h`:114
- Brief: n/a
- Parameters:
  - `j` (const nlohmann::json &): n/a

#### `nlohmann::json toJson() const`
- Source: `include/maintenance/maintenance_schedule.h`:103
- Brief: n/a
- Parameters: none

### themis::maintenance::ModuleHealthSignal

#### `nlohmann::json toJson() const`
- Source: `include/maintenance/maintenance_health_report.h`:44
- Brief: n/a
- Parameters: none

### themis::maintenance::MvccCleanupHandler

#### `MvccCleanupHandler(std::shared_ptr< MVCCStore > mvcc_store, int64_t watermark_ms=kDefaultWatermarkMs)`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:136
- Brief: n/a
- Parameters:
  - `mvcc_store` (std::shared_ptr< MVCCStore >): n/a
  - `watermark_ms` (int64_t): n/a

#### `Result< std::string > execute(const std::string &, MaintenanceTaskType) override`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:142
- Brief: Execute the maintenance task.
- Parameters:
  - `job_id` (const std::string &): The orchestrator job identifier (for logging/tracing).
  - `task_type` (MaintenanceTaskType): The task type being executed.
- Return: Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).
- Details: job_id The orchestrator job identifier (for logging/tracing). task_type The task type being executed. Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).

#### `std::string handlerName() const override`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:164
- Brief: Human-readable handler name for diagnostics.
- Parameters: none
- Details: Returned by GET /api/v1/maintenance/task-handlers. Should be unique and stable across restarts (e.g. "StorageCompactionHandler").

### themis::maintenance::OrchestratorJob

#### `nlohmann::json toJson() const`
- Source: `include/maintenance/maintenance_task.h`:193
- Brief: n/a
- Parameters: none

### themis::maintenance::ReplicaValidationHandler

#### `ReplicaValidationHandler(CheckFn check_fn)`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:93
- Brief: n/a
- Parameters:
  - `check_fn` (CheckFn): n/a

#### `Result< std::string > execute(const std::string &, MaintenanceTaskType) override`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:96
- Brief: Execute the maintenance task.
- Parameters:
  - `job_id` (const std::string &): The orchestrator job identifier (for logging/tracing).
  - `task_type` (MaintenanceTaskType): The task type being executed.
- Return: Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).
- Details: job_id The orchestrator job identifier (for logging/tracing). task_type The task type being executed. Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).

#### `std::string handlerName() const override`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:105
- Brief: Human-readable handler name for diagnostics.
- Parameters: none
- Details: Returned by GET /api/v1/maintenance/task-handlers. Should be unique and stable across restarts (e.g. "StorageCompactionHandler").

### themis::maintenance::StorageCompactionHandler

#### `StorageCompactionHandler(std::shared_ptr< CompactionManager > compaction_manager)`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:42
- Brief: n/a
- Parameters:
  - `compaction_manager` (std::shared_ptr< CompactionManager >): n/a

#### `Result< std::string > execute(const std::string &, MaintenanceTaskType) override`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:46
- Brief: Execute the maintenance task.
- Parameters:
  - `job_id` (const std::string &): The orchestrator job identifier (for logging/tracing).
  - `task_type` (MaintenanceTaskType): The task type being executed.
- Return: Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).
- Details: job_id The orchestrator job identifier (for logging/tracing). task_type The task type being executed. Ok with a human-readable result summary string on success, or an Error on failure (the orchestrator will propagate the error message to the job's error_message field).

#### `std::string handlerName() const override`
- Source: `include/maintenance/maintenance_task_handler_impls.h`:59
- Brief: Human-readable handler name for diagnostics.
- Parameters: none
- Details: Returned by GET /api/v1/maintenance/task-handlers. Should be unique and stable across restarts (e.g. "StorageCompactionHandler").

### themis::maintenance::test

#### `TEST(MaintenanceChurnHardening, MTN17_ConcurrentScheduleTriggerSkipped)`
- Source: `tests/maintenance/test_maintenance_churn_hardening_focused.cpp`:105
- Brief: Two threads try to enter executeSchedule() for the same schedule_id. The second thread must see the "already in-flight" condition and produce a SKIPPED_CONCURRENT outcome.
- Parameters:
  - `<unnamed>` (MaintenanceChurnHardening): n/a
  - `<unnamed>` (MTN17_ConcurrentScheduleTriggerSkipped): n/a

#### `TEST(MaintenanceChurnHardening, MTN18_RapidChurnLoopBoundedState)`
- Source: `tests/maintenance/test_maintenance_churn_hardening_focused.cpp`:139
- Brief: Validates that 100 rapid add/remove churn cycles on schedule state complete without panics and the in-flight set returns to empty.
- Parameters:
  - `<unnamed>` (MaintenanceChurnHardening): n/a
  - `<unnamed>` (MTN18_RapidChurnLoopBoundedState): n/a

#### `TEST(MaintenanceChurnHardening, MTN19_ChurnLimitPolicyTriggered)`
- Source: `tests/maintenance/test_maintenance_churn_hardening_focused.cpp`:192
- Brief: Validates that the churn rate-limit policy correctly rejects excess mutations beyond max_schedule_changes_per_interval.
- Parameters:
  - `<unnamed>` (MaintenanceChurnHardening): n/a
  - `<unnamed>` (MTN19_ChurnLimitPolicyTriggered): n/a

#### `TEST(MaintenanceChurnHardening, MTN20_SkippedConcurrentInDiagnosticLog)`
- Source: `tests/maintenance/test_maintenance_churn_hardening_focused.cpp`:227
- Brief: Validates that a DispatchOutcome with SKIPPED_CONCURRENT outcome serialises to JSON with the correct fields, enabling operators to observe it in the health log / ring buffer snapshot.
- Parameters:
  - `<unnamed>` (MaintenanceChurnHardening): n/a
  - `<unnamed>` (MTN20_SkippedConcurrentInDiagnosticLog): n/a

#### `TEST(MaintenanceContractHardening, MTN01_ErrorCodeUniqueness)`
- Source: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceContractHardening): n/a
  - `<unnamed>` (MTN01_ErrorCodeUniqueness): n/a

#### `TEST(MaintenanceContractHardening, MTN02_ErrorCodeRange)`
- Source: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceContractHardening): n/a
  - `<unnamed>` (MTN02_ErrorCodeRange): n/a

#### `TEST(MaintenanceContractHardening, MTN03_SwitchDispatch)`
- Source: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceContractHardening): n/a
  - `<unnamed>` (MTN03_SwitchDispatch): n/a

#### `TEST(MaintenanceContractHardening, MTN04_ScheduleDescriptorDefaults)`
- Source: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceContractHardening): n/a
  - `<unnamed>` (MTN04_ScheduleDescriptorDefaults): n/a

#### `TEST(MaintenanceContractHardening, MTN05_HealthSnapshotDefaults)`
- Source: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceContractHardening): n/a
  - `<unnamed>` (MTN05_HealthSnapshotDefaults): n/a

#### `TEST(MaintenanceContractHardening, MTN06_ScheduleDescriptorCopy)`
- Source: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceContractHardening): n/a
  - `<unnamed>` (MTN06_ScheduleDescriptorCopy): n/a

#### `TEST(MaintenanceContractHardening, MTN07_ScheduleDescriptorMove)`
- Source: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceContractHardening): n/a
  - `<unnamed>` (MTN07_ScheduleDescriptorMove): n/a

#### `TEST(MaintenanceContractHardening, MTN08_FailClosedPredicate)`
- Source: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceContractHardening): n/a
  - `<unnamed>` (MTN08_FailClosedPredicate): n/a

#### `TEST(MaintenanceDiagnostics, MTN29_AllOutcomeTypesVisibleInHealthReport)`
- Source: `tests/maintenance/test_maintenance_diagnostics_focused.cpp`:89
- Brief: Validates that all five DispatchOutcomeType values can be recorded and are visible in MaintenanceHealthReport::recent_dispatch_outcomes.
- Parameters:
  - `<unnamed>` (MaintenanceDiagnostics): n/a
  - `<unnamed>` (MTN29_AllOutcomeTypesVisibleInHealthReport): n/a

#### `TEST(MaintenanceDiagnostics, MTN30_RingBufferOverflowDropsOldest)`
- Source: `tests/maintenance/test_maintenance_diagnostics_focused.cpp`:138
- Brief: Fills the ring buffer beyond capacity (256) and verifies:
- Parameters:
  - `<unnamed>` (MaintenanceDiagnostics): n/a
  - `<unnamed>` (MTN30_RingBufferOverflowDropsOldest): n/a
- Details: No crash occurs Buffer size stays at capacity The oldest entries are dropped (FIFO eviction)

#### `TEST(MaintenanceDiagnostics, MTN31_ReportSerializationConsistentFields)`
- Source: `tests/maintenance/test_maintenance_diagnostics_focused.cpp`:174
- Brief: Verifies that MaintenanceHealthReport::toJson() produces a complete JSON object with all expected top-level fields populated.
- Parameters:
  - `<unnamed>` (MaintenanceDiagnostics): n/a
  - `<unnamed>` (MTN31_ReportSerializationConsistentFields): n/a

#### `TEST(MaintenanceDiagnostics, MTN32_DispatchOutcomeLatencyNonNegativePlausible)`
- Source: `tests/maintenance/test_maintenance_diagnostics_focused.cpp`:228
- Brief: Validates that latency_us in DispatchOutcome is always >= 0 and within a plausible range for normal maintenance operations.
- Parameters:
  - `<unnamed>` (MaintenanceDiagnostics): n/a
  - `<unnamed>` (MTN32_DispatchOutcomeLatencyNonNegativePlausible): n/a

#### `TEST(MaintenanceEndurance, MTN_ENDURANCE_01_TenThousandCyclesNoLeaks)`
- Source: `tests/maintenance/test_maintenance_endurance_focused.cpp`:49
- Brief: Runs 10 000 add→execute→remove lifecycle cycles against an in-memory schedule map with a RAII counter to verify no state leaks.
- Parameters:
  - `<unnamed>` (MaintenanceEndurance): n/a
  - `<unnamed>` (MTN_ENDURANCE_01_TenThousandCyclesNoLeaks): n/a
- Details: Acceptance criteria: Completes within 60 seconds wall-clock time. Zero accumulated errors. Schedule count returns to exactly 0 at end (no leaks).

#### `TEST(MaintenanceHardeningPhase23, MTN09_FrequencyToCronNonEmpty)`
- Source: `tests/maintenance/test_maintenance_hardening_phase23_focused.cpp`:54
- Brief: Validates that every non-CUSTOM ScheduleFrequency maps to a non-empty, syntactically plausible cron expression.
- Parameters:
  - `<unnamed>` (MaintenanceHardeningPhase23): n/a
  - `<unnamed>` (MTN09_FrequencyToCronNonEmpty): n/a
- Details: Covers Phase 2 item: "align registry and execution behavior to bounded runtime contracts" — the cron derivation is part of the schedule activation contract.

#### `TEST(MaintenanceHardeningPhase23, MTN10_ScheduleEntryRoundTrip)`
- Source: `tests/maintenance/test_maintenance_hardening_phase23_focused.cpp`:92
- Brief: Validates that all key fields of MaintenanceScheduleEntry survive a JSON serialization → deserialization round-trip without data loss.
- Parameters:
  - `<unnamed>` (MaintenanceHardeningPhase23): n/a
  - `<unnamed>` (MTN10_ScheduleEntryRoundTrip): n/a
- Details: Covers Phase 2 item: "complete hardening for orchestrator and schedule-store internals" — the schedule store relies on toJson/fromJson for persistence.

#### `TEST(MaintenanceHardeningPhase23, MTN11_TaskDependencyRoundTrip)`
- Source: `tests/maintenance/test_maintenance_hardening_phase23_focused.cpp`:158
- Brief: Validates that MaintenanceTaskDependency survives a JSON round-trip preserving all fields.
- Parameters:
  - `<unnamed>` (MaintenanceHardeningPhase23): n/a
  - `<unnamed>` (MTN11_TaskDependencyRoundTrip): n/a
- Details: Covers Phase 3 item: "unify diagnostics across scheduling/persistence/ execution incidents" — the dependency structure is part of the schedule's durable state.

#### `TEST(MaintenanceHardeningPhase23, MTN12_TaskTypeStringRoundTrip)`
- Source: `tests/maintenance/test_maintenance_hardening_phase23_focused.cpp`:185
- Brief: Validates that every MaintenanceTaskType survives a string ↔ enum round-trip without loss.
- Parameters:
  - `<unnamed>` (MaintenanceHardeningPhase23): n/a
  - `<unnamed>` (MTN12_TaskTypeStringRoundTrip): n/a
- Details: Covers Phase 2 item: "align registry and execution behavior to bounded runtime contracts" — task type dispatch relies on deterministic string conversion.

#### `TEST(MaintenanceHardeningPhase23, MTN13_JobStateStringCoverage)`
- Source: `tests/maintenance/test_maintenance_hardening_phase23_focused.cpp`:236
- Brief: Validates that every MaintenanceJobState maps to a non-empty, distinct string (full coverage of the operator-visible diagnostics path).
- Parameters:
  - `<unnamed>` (MaintenanceHardeningPhase23): n/a
  - `<unnamed>` (MTN13_JobStateStringCoverage): n/a
- Details: Covers Phase 3 item: "unify diagnostics across scheduling/persistence/ execution incidents" — job state strings appear in operator logs and REST responses.

#### `TEST(MaintenanceHardeningPhase23, MTN14_ScheduleEntryDefaults)`
- Source: `tests/maintenance/test_maintenance_hardening_phase23_focused.cpp`:272
- Brief: Validates that a default-constructed MaintenanceScheduleEntry has well-defined, bounded default values consistent with the persistence and execution contracts.
- Parameters:
  - `<unnamed>` (MaintenanceHardeningPhase23): n/a
  - `<unnamed>` (MTN14_ScheduleEntryDefaults): n/a
- Details: Covers Phase 2 item: "complete hardening for orchestrator and schedule-store internals" — default-constructed entries must never silently submit invalid schedules.

#### `TEST(MaintenanceHardeningPhase23, MTN15_HaltOnTaskFailureRoundTrip)`
- Source: `tests/maintenance/test_maintenance_hardening_phase23_focused.cpp`:305
- Brief: Validates that the halt_on_task_failure field survives serialization so that the fail-safe execution mode is preserved on persistence reload.
- Parameters:
  - `<unnamed>` (MaintenanceHardeningPhase23): n/a
  - `<unnamed>` (MTN15_HaltOnTaskFailureRoundTrip): n/a
- Details: Covers Phase 3 item: "standardize fail-safe behavior for missing handlers and invalid persisted schedules."

#### `TEST(MaintenanceHardeningPhase23, MTN16_ScheduleWithDependenciesRoundTrip)`
- Source: `tests/maintenance/test_maintenance_hardening_phase23_focused.cpp`:331
- Brief: Validates that a MaintenanceScheduleEntry with a non-empty task_dependencies list serializes and deserializes correctly.
- Parameters:
  - `<unnamed>` (MaintenanceHardeningPhase23): n/a
  - `<unnamed>` (MTN16_ScheduleWithDependenciesRoundTrip): n/a
- Details: Covers Phase 3 item: "standardize fail-safe behavior for missing handlers and invalid persisted schedules" — a corrupt dependency list after reload would produce an unexpected execution order or a cycle detection error.

#### `TEST(MaintenanceStress, MTN21_CorruptStorePersistenceCorrupt)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceStress): n/a
  - `<unnamed>` (MTN21_CorruptStorePersistenceCorrupt): n/a

#### `TEST(MaintenanceStress, MTN22_MissingHandlerSkippedOutcome)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceStress): n/a
  - `<unnamed>` (MTN22_MissingHandlerSkippedOutcome): n/a

#### `TEST(MaintenanceStress, MTN23_FiveHundredCycleStressLoop)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceStress): n/a
  - `<unnamed>` (MTN23_FiveHundredCycleStressLoop): n/a

#### `TEST(MaintenanceStress, MTN24_EmptyScheduleListPersistReload)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceStress): n/a
  - `<unnamed>` (MTN24_EmptyScheduleListPersistReload): n/a

#### `TEST(MaintenanceStress, MTN25_EmptyTaskListRoundTrip)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceStress): n/a
  - `<unnamed>` (MTN25_EmptyTaskListRoundTrip): n/a

#### `TEST(MaintenanceStress, MTN26_MaxScheduleCountPersistReload)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceStress): n/a
  - `<unnamed>` (MTN26_MaxScheduleCountPersistReload): n/a

#### `TEST(MaintenanceStress, MTN27_CorruptMidArrayReturnsPersistenceCorrupt)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceStress): n/a
  - `<unnamed>` (MTN27_CorruptMidArrayReturnsPersistenceCorrupt): n/a

#### `TEST(MaintenanceStress, MTN28_PartialRegistryHandlerMismatch)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (MaintenanceStress): n/a
  - `<unnamed>` (MTN28_PartialRegistryHandlerMismatch): n/a

#### `DispatchOutcome makeOutcome(DispatchOutcomeType t, const std::string &sched_id, const std::string &task, int64_t latency_us=100)`
- Source: `tests/maintenance/test_maintenance_diagnostics_focused.cpp`:69
- Brief: n/a
- Parameters:
  - `t` (DispatchOutcomeType): n/a
  - `sched_id` (const std::string &): n/a
  - `task` (const std::string &): n/a
  - `latency_us` (int64_t): n/a

#### `MaintenanceScheduleEntry makeSchedule(const std::string &name, const std::vector< MaintenanceTaskType > &tasks={MaintenanceTaskType::QUOTA_CHECK})`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:92
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `tasks` (const std::vector< MaintenanceTaskType > &): n/a

### themis::maintenance::test::ChurnTracker

#### `bool check(const std::string &schedule_id, uint32_t max_per_interval)`
- Source: `tests/maintenance/test_maintenance_churn_hardening_focused.cpp`:80
- Brief: Returns true if the change is permitted; false if limit exceeded.
- Parameters:
  - `schedule_id` (const std::string &): n/a
  - `max_per_interval` (uint32_t): n/a

#### `int64_t nowMs()`
- Source: `tests/maintenance/test_maintenance_churn_hardening_focused.cpp`:74
- Brief: n/a
- Parameters: none

### themis::maintenance::test::InFlightGuard

#### `void leave(const std::string &id)`
- Source: `tests/maintenance/test_maintenance_churn_hardening_focused.cpp`:58
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a

#### `bool tryEnter(const std::string &id)`
- Source: `tests/maintenance/test_maintenance_churn_hardening_focused.cpp`:53
- Brief: Returns false if schedule_id already in-flight (SKIPPED_CONCURRENT case).
- Parameters:
  - `id` (const std::string &): n/a

### themis::maintenance::test::MockScheduleStore

#### `bool loadAll(std::map< std::string, MaintenanceScheduleEntry > &out, std::string &error_out)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:62
- Brief: Returns false and sets error on corruption.
- Parameters:
  - `out` (std::map< std::string, MaintenanceScheduleEntry > &): n/a
  - `error_out` (std::string &): n/a

#### `void remove(const std::string &id)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:57
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a

#### `void save(const MaintenanceScheduleEntry &entry)`
- Source: `tests/maintenance/test_maintenance_stress_focused.cpp`:53
- Brief: n/a
- Parameters:
  - `entry` (const MaintenanceScheduleEntry &): n/a

### themis::maintenance::test::TestRingBuffer

#### `TestRingBuffer(int cap=kDefaultCapacity)`
- Source: `tests/maintenance/test_maintenance_diagnostics_focused.cpp`:49
- Brief: n/a
- Parameters:
  - `cap` (int): n/a

#### `void push(DispatchOutcome o)`
- Source: `tests/maintenance/test_maintenance_diagnostics_focused.cpp`:51
- Brief: n/a
- Parameters:
  - `o` (DispatchOutcome): n/a

#### `std::vector< DispatchOutcome > snapshot()`
- Source: `tests/maintenance/test_maintenance_diagnostics_focused.cpp`:59
- Brief: n/a
- Parameters: none

### themis::test::wave_d

#### `TEST(WaveD_MaintenanceStress, ConcurrentCompactionStress)`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_MaintenanceStress): n/a
  - `<unnamed>` (ConcurrentCompactionStress): n/a

#### `TEST(WaveD_MaintenanceStress, HighCardinalityVacuumTask)`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_MaintenanceStress): n/a
  - `<unnamed>` (HighCardinalityVacuumTask): n/a

#### `TEST(WaveD_MaintenanceStress, RetentionPolicyStress)`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_MaintenanceStress): n/a
  - `<unnamed>` (RetentionPolicyStress): n/a

### themis::test::wave_d::StubCompactionShard

#### `bool compact(uint32_t shard_id)`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:65
- Brief: n/a
- Parameters:
  - `shard_id` (uint32_t): n/a

#### `long compactCount() const`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:72
- Brief: n/a
- Parameters: none

#### `long stallCount() const`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:73
- Brief: n/a
- Parameters: none

### themis::test::wave_d::StubRetentionEnforcer

#### `long checked() const`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:96
- Brief: n/a
- Parameters: none

#### `bool enforce(uint64_t record_id, uint64_t age_ms, uint64_t ttl_ms)`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:83
- Brief: n/a
- Parameters:
  - `record_id` (uint64_t): n/a
  - `age_ms` (uint64_t): n/a
  - `ttl_ms` (uint64_t): n/a

#### `long evicted() const`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:97
- Brief: n/a
- Parameters: none

#### `long violations() const`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:98
- Brief: n/a
- Parameters: none

### themis::test::wave_d::StubVacuumTaskStore

#### `std::size_t size() const`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:51
- Brief: n/a
- Parameters: none

#### `bool submit(const std::string &task_id)`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:44
- Brief: n/a
- Parameters:
  - `task_id` (const std::string &): n/a

#### `long writes() const`
- Source: `tests/maintenance/test_maintenance_highcardinality_stress.cpp`:56
- Brief: n/a
- Parameters: none

