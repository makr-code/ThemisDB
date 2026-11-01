---
Author: "ThemisDB Team"
Created: "2026-08-07"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "benchmarks/timeseries/bench_timeseries_ingestion.cpp; benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp; benchmarks/timeseries/bench_timeseries_release_gates.cpp"
---

> Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/timeseries

## Scope

- Module: src/timeseries
- This file records benchmark mappings and intended expectations. It does not
  implement or prove an automated release gate.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/timeseries/bench_timeseries_ingestion.cpp
  - benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| TSRP-1 | raw and batch ingest paths remain bounded | TimeseriesBenchmarkFixture/RawDataIngestion, TimeseriesBenchmarkFixture/BatchIngestion, TimeseriesBenchmarkFixture/MultipleMetrics, TimeseriesBenchmarkFixture/OutOfOrderWrites |
| TSRP-2 | Gorilla compression, decompression, and downsampling helpers remain bounded | BM_GorillaCompression, BM_GorillaDecompression, TimeseriesBenchmarkFixture/Downsampling, BM_DownsamplingThroughput |
| TSRP-3 | range-query and adaptive flush latency paths remain bounded | TimeseriesBenchmarkFixture/TimeRangeQuery, AdaptiveFlushFixture/SingleThreaded, AdaptiveFlushFixture/MultiThreaded, AdaptiveFlushFixture/P99Latency, AdaptiveFlushFixture/BatchWatermark |
| TSRP-4 | flush-controller and stats exposure helper paths remain bounded | BM_FlushController_Standalone, AdaptiveFlushFixture/StatsExposure |

## Documented target labels (not an enforced gate)

| Gate ID | Expectation | Measurement |
|---|---|---|
| TSRG-1 | Intended regression comparison: <= 10 percent vs a recorded baseline | Requires a separately maintained baseline and comparator |
| TSRG-2 | Intended hot-path p99 comparison against a configured threshold | Requires a separately configured threshold and result |
| TSRG-3 | Intended benchmark-case completeness check | Requires a benchmark run manifest and gate implementation |

## Validation

- A benchmark source and CMake target establish only that a benchmark is
  declared/registered. A pass requires actual run output and an enforcing
  comparison mechanism.
- Mapping should be expanded as additional timeseries benchmark scenarios are introduced.

No build, benchmark run, baseline comparison, or release-gate execution was
performed for this documentation review.

## Sourcecode Verification (Module: timeseries/performance)

- Verified benchmark sources:
  - benchmarks/timeseries/bench_timeseries_ingestion.cpp
  - benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp
- Verified mapping surfaces:
  - ingest, Gorilla codec, range-query, adaptive flush, and downsampling behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Execution, baselines, comparisons, and release-gate enforcement were not verified.