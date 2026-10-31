---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Module API

## Purpose

This page is an integration-oriented index of the principal public interfaces
under [`include/performance`](../../include/performance). The headers define
the authoritative signatures, ownership rules, and complete field lists;
this page summarizes the behavior callers must account for.

## Interface map

| Interface | Header | Caller contract |
|---|---|---|
| Cycle measurement | [`cycle_metrics.h`](../../include/performance/cycle_metrics.h), `cycle_metrics_config.h` | Capture cycle-oriented operation data; collection can be configured/sampled. |
| Runtime and phase feature controls | [`runtime_config.h`](../../include/performance/runtime_config.h), [`feature_flags.h`](../../include/performance/feature_flags.h), [`phase2_feature_flags.h`](../../include/performance/phase2_feature_flags.h), `phase3/feature_flags.h`, `phase4/feature_flags.h` | Compile-time and runtime availability are separate checks. |
| Workload forecasting and strategy | [`workload_predictor.h`](../../include/performance/workload_predictor.h), [`workload_adaptive_optimizer.h`](../../include/performance/workload_adaptive_optimizer.h) | The predictor returns zero-confidence results with fewer than two observations; auto-adaptation is opt-in. |
| Cache and placement | [`advanced_cache_manager.h`](../../include/performance/advanced_cache_manager.h), [`numa_memory_manager.h`](../../include/performance/numa_memory_manager.h), [`numa_topology.h`](../../include/performance/numa_topology.h) | Cache misses and unavailable NUMA support have documented result/fallback paths; allocations must be released through their manager. |
| Hardware dispatch | [`hardware_accelerator.h`](../../include/performance/hardware_accelerator.h) | `ExecutionResult` carries status, error, elapsed time, and whether an accelerated path was used. |
| Per-query feedback | [`phase3/per_query_cost_model.h`](../../include/performance/phase3/per_query_cost_model.h) | Use `QueryGuard` to record elapsed time and optional row/page counts; calibration consumes bounded recent history. |
| Vector search | [`rabitq.h`](../../include/performance/rabitq.h), [`phase3/diskann.h`](../../include/performance/phase3/diskann.h) | Validate dimensions and index persistence inputs; search algorithms have approximation and storage trade-offs. |
| Phase 4 zero-copy I/O | [`phase4/io_uring_zero_copy.h`](../../include/performance/phase4/io_uring_zero_copy.h) | io_uring is optional; fallback uses standard socket calls. One instance is not concurrent-call safe. |

## Shared error contract

[`performance_api_contract.h`](../../include/performance/performance_api_contract.h)
declares `PerfError` values for compilation timeout, cache eviction failure,
pool exhaustion, invalid cost-model input, no healthy node, stale plan, and
unavailable statistics. `PoolAcquireResult` represents acquisition success or
failure, `CacheStats` reports hits, misses, evictions, and hit rate, and
`CostEstimate` is a non-negative `double`. The API contract is frozen for the
v2.x major line; incompatible changes require major-version and changelog
treatment.

Not every interface reports errors through `PerfError`: for example, NUMA
allocation documents `std::bad_alloc`, RaBitQ construction and dimension
validation can throw, `ThreadPinner` returns `false` on OS failure, and
io_uring send/receive return negative errno values on error. Use each
declaration's documented contract rather than assuming a common error channel.

## Integration requirements

- Keep feature enablement checks aligned with the compile-time build and
  detected capabilities.
- Do not use a forecast or strategy object as proof that the external resource
  was resized; applying a recommendation is the caller's responsibility.
- Treat result fields such as `used_hw_path`, `io_uring_available`, and
  dropped-metric counts as operational evidence; do not infer success from
  requested configuration.
- Observe documented ownership and threading constraints before sharing an
  instance across threads.

For implementation sequencing and focused validation, see
[IMPLEMENTATION.md](IMPLEMENTATION.md) and [MODULE_GUIDE.md](MODULE_GUIDE.md).

Exact model backend not verifiable from final document text alone.
