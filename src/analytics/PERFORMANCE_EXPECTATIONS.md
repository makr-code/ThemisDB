# PERFORMANCE_EXPECTATIONS - src/analytics

## Scope

- Module: src/analytics
- This file defines measurable analytics module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/analytics/bench_analytics_release_gates.cpp
  - benchmarks/analytics/bench_analytics_distributed_coordinator.cpp
  - benchmarks/analytics/bench_streaming_window.cpp
  - benchmarks/analytics/bench_analytics_operability_paths.cpp
  - benchmarks/baselines/analytics/representative_hardware_manifest.json

## Specific Expectations

| Target ID | Expectation | Direct benchmark case |
|---|---|---|
| AN-1 | aggregation throughput remains within release baseline budget | `BM_ARG01_AggregationThroughput` |
| AN-2 | tumbling-window evaluation p99 remains within release threshold | `BM_ARG02_WindowEvaluation` |
| AN-3 | OLAP plan lookup p99 remains within release threshold | `BM_ARG03_OlapPlanLookup` |
| AN-4 | anomaly-check p99 remains within release threshold | `BM_ARG04_AnomalyCheck` |
| AN-5 | CEP pattern-match p99 remains within release threshold | `BM_ARG05_CepPatternMatch` |
| AN-6 | forecasting validation/inference stub p99 remains within release threshold | `BM_ARG06_ForecastInferenceStub` |
| AN-7 | distributed coordinator state transitions and degraded throughput remain bounded | `BenchCircuitBreakerStateTransition`, `BenchConcurrentMergeStartup`, `BenchTimeoutRecoverySwitchover`, `BenchDegradedModeThroughput` |
| AN-8 | streaming runtime limits remain bounded under sustained and high-cardinality load | `BM_TumblingWindow_SustainedLoad_Bounded`, `BM_SlidingWindow_RecordLimitDrop`, `BM_AO03_HighCardinalityStreamingBounded` |
| AN-9 | export serialization paths remain directly measurable | `BM_AO01_ExportJsonToString`, `BM_AO02_ExportCsvToString` |
| AN-10 | serving fail-closed validation and distributed retry paths remain directly measurable | `BM_AO04_DistributedRetryRecovery`, `BM_AO05_ServingInvalidInputFastFail` |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | analytics hot-path p99 <= release threshold | p99 from ARG, distributed coordinator, and operability benchmark suites |
| AG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Representative-hardware closure additionally requires the manifest-defined artifact set for every analytics hardware profile.
- DC-02 gates p99 time from coordinator entry until all shard executors have started. The benchmark also reports full end-to-end p99, including shard work and merge, as a diagnostic counter; that separate value is not the DC-02 startup gate.
- `DistributedAnalyticsSharding` dispatches through a shared `themis::utils::ThreadPool` with `max(1, min(hardware_concurrency, 16))` workers and the pool's default 1,000-task queue; `max_parallel_shards` is capped by pool capacity, and rejected submissions fail the shard explicitly. A targeted Windows Release DC-02 run passed at dispatch-startup p99 152.2µs (end-to-end p99 191.9µs diagnostic); the focused coordinator test passed. The 500µs gate is unchanged; this targeted run is not full-suite completion. Sources: `src/analytics/distributed_analytics.cpp`, `include/utils/thread_pool_manager.h`, `benchmarks/analytics/bench_analytics_distributed_coordinator.cpp`, `tests/analytics/test_analytics_distributed_coordinator_focused.cpp`.
- DC-04 measures completed merged queries with five shards, one failing, under the default 20% failure-rate limit. It compares normal and degraded query rates using their actual elapsed windows; it does not treat an unmerged result as successful throughput.

## Sourcecode Verification (Module: analytics/performance)

- Verified benchmark sources:
  - benchmarks/analytics/bench_analytics_release_gates.cpp
  - benchmarks/analytics/bench_analytics_distributed_coordinator.cpp
  - benchmarks/analytics/bench_streaming_window.cpp
  - benchmarks/analytics/bench_analytics_operability_paths.cpp
  - benchmarks/baselines/analytics/representative_hardware_manifest.json
- Verified mapping surfaces:
  - OLAP hot paths
  - distributed retry/circuit-breaker/merge paths
  - bounded streaming and high-cardinality rejection paths
  - export serialization paths
  - serving fail-closed validation path
- Result:
  - Referenced benchmark cases exist in current analytics benchmark sources.
  - The module-level expectation set no longer depends on proxy-only analytics benchmark mappings.
  - Representative-hardware baseline execution remains tracked via the analytics hardware manifest.