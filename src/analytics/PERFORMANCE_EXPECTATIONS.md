# PERFORMANCE_EXPECTATIONS - src/analytics

## Scope

- Module: src/analytics
- This file defines measurable analytics module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_olap_analytics.cpp
  - benchmarks/bench_timeseries_ingestion.cpp
  - benchmarks/bench_timeseries_adaptive_flush.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| AN-1 | OLAP single-query execution overhead remains within release baseline budget | BM_OLAP_GroupBy_Int |
| AN-2 | OLAP window-function path remains within release baseline budget | BM_OLAP_WindowFunction |
| AN-3 | OLAP multi-join path remains within release baseline budget | BM_OLAP_MultiJoin |
| AN-4 | OLAP top-N sorted path remains within release baseline budget | BM_OLAP_TopN_Sorted |
| AN-5 | timeseries ingestion throughput remains bounded | BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, RawDataIngestion), BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, BatchIngestion), BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, MultipleMetrics) |
| AN-6 | timeseries query/downsampling path remains bounded | BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, TimeRangeQuery), BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, Downsampling), BM_DownsamplingThroughput |
| AN-7 | compression/decompression path remains bounded | BM_GorillaCompression, BM_GorillaDecompression |
| AN-8 | adaptive flush single/multi-thread behavior remains bounded | BENCHMARK_REGISTER_F(AdaptiveFlushFixture, SingleThreaded), BENCHMARK_REGISTER_F(AdaptiveFlushFixture, MultiThreaded) |
| AN-9 | adaptive flush p99 and watermark behavior remain bounded | BENCHMARK_REGISTER_F(AdaptiveFlushFixture, P99Latency), BENCHMARK_REGISTER_F(AdaptiveFlushFixture, BatchWatermark) |
| AN-10 | adaptive flush control and stats overhead remain bounded | BM_FlushController_Standalone, BENCHMARK_REGISTER_F(AdaptiveFlushFixture, StatsExposure) |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | OLAP and timeseries path p99 <= release threshold | p99 from mapped bench_olap_analytics and timeseries benchmark cases |
| AG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: analytics/performance)

- Verified benchmark sources:
  - benchmarks/bench_olap_analytics.cpp
  - benchmarks/bench_timeseries_ingestion.cpp
  - benchmarks/bench_timeseries_adaptive_flush.cpp
- Verified mapping surfaces:
  - OLAP execution paths
  - timeseries ingestion/query/compression paths
  - adaptive flush and latency/control paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.