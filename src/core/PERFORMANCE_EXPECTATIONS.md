# PERFORMANCE_EXPECTATIONS - src/core

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Scope

- Module: src/core
- This file defines measurable performance expectations for the core dependency-injection and cross-cutting-concerns infrastructure.
- Scope is limited to direct core benchmarks for `ConcernsContext`, logging adapters, metrics wrappers, and cache paths.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_di_logging.cpp
- Excluded from this module on purpose:
  - benchmarks/bench_themis_core.cpp belongs to src/themis performance coverage, not src/core.

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| COREP-1 | `ConcernsContext` creation overhead remains bounded for no-op and custom injection paths | `BM_ConcernsContext_CreateNoOp`, `BM_ConcernsContext_CreateCustom` |
| COREP-2 | no-op logging remains the zero-overhead floor for hot-path instrumentation | `BM_NoOpLogger_Info`, `BM_NoOpLogger_LogStructured` |
| COREP-3 | spdlog adapter formatting and dispatch overhead remains bounded in plain-text and JSON modes | `BM_SpdlogAdapter_Info_PlainText`, `BM_SpdlogAdapter_Info_JsonMode`, `BM_SpdlogAdapter_AllLevels` |
| COREP-4 | structured and trace-context log enrichment overhead remains bounded | `BM_SpdlogAdapter_LogStructured_PlainText`, `BM_SpdlogAdapter_LogStructured_JsonMode`, `BM_SpdlogAdapter_LogWithContext_PlainText`, `BM_SpdlogAdapter_LogWithContext_JsonMode` |
| COREP-5 | convenience wrappers on `ConcernsContext` remain bounded for logger and metrics dispatch | `DIConcernsBenchFixture/LogInfo_ViaContext`, `DIConcernsBenchFixture/LogError_ViaContext`, `DIConcernsBenchFixture/MetricsIncrementCounter`, `DIConcernsBenchFixture/MetricsObserveHistogram` |
| COREP-6 | cache dispatch and real in-memory cache operations remain bounded | `DIConcernsBenchFixture/CacheGetMiss`, `DIConcernsBenchFixture/CachePut`, `InMemoryCacheBenchFixture/GetHit`, `InMemoryCacheBenchFixture/GetMiss`, `InMemoryCacheBenchFixture/Put` |

## Module Hard Gates

| Gate ID | Expectation | Measurement |
|---|---|---|
| CG-1 | Regression <= 10 percent vs release baseline | `(current - baseline) / baseline` |
| CG-2 | `ConcernsContext` creation p99 remains within release threshold | p99 from `BM_ConcernsContext_CreateNoOp` and `BM_ConcernsContext_CreateCustom` |
| CG-3 | structured logging and context-enriched logging p99 remain within release threshold | p99 from mapped `BM_SpdlogAdapter_*` structured/context cases |
| CG-4 | metrics and cache wrapper overhead remain within release threshold | p99 from mapped `DIConcernsBenchFixture/*` and `InMemoryCacheBenchFixture/*` cases |
| CG-5 | no mapped benchmark case is missing in the release run manifest | benchmark run manifest completeness |

## Validation

- Expectations are met when the mapped benchmarks run reproducibly in release profile and stay within configured release thresholds.
- No TPCC, vector-search, or other parent-module proxy benchmarks are used for this module anymore.
- `config_validator`, `production_mode`, and `security_initialization` do not yet have dedicated microbenchmarks; performance claims for those paths must remain qualitative until direct benchmark coverage exists.

## Sourcecode Verification (Module: core/performance)

- Verified benchmark sources:
  - benchmarks/bench_di_logging.cpp
- Verified mapping surfaces:
  - `ConcernsContext` creation and convenience wrappers
  - no-op versus spdlog logging hot paths
  - structured logging and trace-context enrichment
  - metrics dispatch and in-memory cache operations
- Result:
  - the mapped benchmark cases exist in current benchmark sources
  - the previous parent-proxy mapping to TPCC/vector benchmarks has been removed