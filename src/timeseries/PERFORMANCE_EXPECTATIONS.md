# PERFORMANCE_EXPECTATIONS - src/timeseries

## Scope

- Module: src/timeseries
- This file defines measurable timeseries module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_timeseries_ingestion.cpp
  - benchmarks/bench_timeseries_adaptive_flush.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| TSRP-1 | raw and batch ingest paths remain bounded | TimeseriesBenchmarkFixture/RawDataIngestion, TimeseriesBenchmarkFixture/BatchIngestion, TimeseriesBenchmarkFixture/MultipleMetrics, TimeseriesBenchmarkFixture/OutOfOrderWrites |
| TSRP-2 | Gorilla compression, decompression, and downsampling helpers remain bounded | BM_GorillaCompression, BM_GorillaDecompression, TimeseriesBenchmarkFixture/Downsampling, BM_DownsamplingThroughput |
| TSRP-3 | range-query and adaptive flush latency paths remain bounded | TimeseriesBenchmarkFixture/TimeRangeQuery, AdaptiveFlushFixture/SingleThreaded, AdaptiveFlushFixture/MultiThreaded, AdaptiveFlushFixture/P99Latency, AdaptiveFlushFixture/BatchWatermark |
| TSRP-4 | flush-controller and stats exposure helper paths remain bounded | BM_FlushController_Standalone, AdaptiveFlushFixture/StatsExposure |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| TSRG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| TSRG-2 | timeseries hot-path p99 <= release threshold | p99 from mapped timeseries benchmark cases |
| TSRG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional timeseries benchmark scenarios are introduced.

## Sourcecode Verification (Module: timeseries/performance)

- Verified benchmark sources:
  - benchmarks/bench_timeseries_ingestion.cpp
  - benchmarks/bench_timeseries_adaptive_flush.cpp
- Verified mapping surfaces:
  - ingest, Gorilla codec, range-query, adaptive flush, and downsampling behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.