# PERFORMANCE_EXPECTATIONS - src/cache

## Scope

- Module: src/cache
- This file defines measurable cache module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_adaptive_query_cache.cpp
  - benchmarks/bench_embedding_cache_performance.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| CAC-1 | L1 cache put/get hit paths remain within release baseline budget | BM_Cache_L1_Put, BM_Cache_L1_Get_Hit |
| CAC-2 | cache miss and mixed read/write behavior remain bounded | BM_Cache_Get_Miss, BM_Cache_Mixed_ReadWrite |
| CAC-3 | tenant-isolation put/get paths remain bounded | BM_Cache_TenantIsolation_Put, BM_Cache_TenantIsolation_Get_Hit |
| CAC-4 | invalidation and tenant invalidation operations remain bounded | BM_Cache_Invalidate_Pattern, BM_Cache_InvalidateTenant |
| CAC-5 | concurrent cache read/mixed paths remain bounded under benchmark thread settings | BM_Cache_Concurrent_Read, BM_Cache_Concurrent_Mixed |
| CAC-6 | cache warmup and stats collection paths remain bounded | BM_WarmupFromLog, BM_Cache_GetStats |
| CAC-7 | embedding cache query/eviction paths remain within release baseline budget | BM_EmbeddingCache_Query_WithIndex, BM_EmbeddingCache_Eviction |
| CAC-8 | embedding cache batch and cost-savings paths remain bounded | BM_EmbeddingCache_BatchStore, BM_EmbeddingCache_CostSavings |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| CG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| CG-2 | cache hot-path p99 <= release threshold | p99 from mapped adaptive-query-cache benchmark cases |
| CG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: cache/performance)

- Verified benchmark sources:
  - benchmarks/bench_adaptive_query_cache.cpp
  - benchmarks/bench_embedding_cache_performance.cpp
- Verified mapping surfaces:
  - adaptive query cache put/get/miss/invalidation/concurrency/warmup paths
  - tenant isolation and stats paths
  - embedding query, batch, eviction, and cost-savings paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.