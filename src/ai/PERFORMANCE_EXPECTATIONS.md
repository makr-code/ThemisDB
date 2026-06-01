# PERFORMANCE_EXPECTATIONS - src/ai

## Scope

- Module: src/ai
- This file defines measurable AI module performance expectations for release gating.

## Benchmark Reference

- Current benchmark coverage is proxy-based via plugin subsystem benchmarks.
- Relevant benchmark files:
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_plugin_hot_plug.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| AI-1 | prompt-validation-path overhead remains within release baseline budget | BM_LoadNonexistentPlugin, BM_ManifestParsing |
| AI-2 | generation-orchestration error-path overhead remains bounded | BM_LoadUnloadPlugin, BM_ReloadPlugin |
| AI-3 | concurrent lookup/scan pressure does not exceed release threshold | BM_ConcurrentQueries, BM_ConcurrentScans, BM_ConcurrentGetAllPlugins |
| AI-4 | monitor and lifecycle churn overhead remains bounded | BENCHMARK_F(HotPlugBenchmarkFixture, EnableDisableMonitoring), BENCHMARK_F(HotPlugBenchmarkFixture, RapidEnableDisable), BENCHMARK_F(HotPlugBenchmarkFixture, MixedOperations) |
| AI-5 | memory overhead for plugin lifecycle path remains bounded | BM_MemoryOverhead, BENCHMARK_F(HotPlugBenchmarkFixture, MonitorMemoryFootprint) |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | proxy lifecycle path p99 <= release threshold | p99 from mapped plugin_system and plugin_hot_plug cases |
| AG-3 | no mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Proxy mapping remains temporary until a dedicated ai benchmark target is added.

## Sourcecode Verification (Module: ai/performance)

- Verified benchmark sources:
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_plugin_hot_plug.cpp
- Verified mapping surfaces:
  - lifecycle and manifest handling benchmarks
  - concurrent access benchmarks
  - hot-plug monitoring and memory-footprint benchmarks
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
