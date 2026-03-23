<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Performance Module

- **Last Audit:** 2026-03-22
- **Auditor:** Copilot
- **Status:** ✅ Pass

## Summary

| Metric | Count |
|---|---|
| Header files audited | 26 |
| Exported symbol groups | 26 |
| Open stubs | 0 |
| Critical findings | 0 |

## Header Files Audited

| File | Exported Symbols | Notes |
|---|---|---|
| `adaptive_query_compiler.h` | `AdaptiveQueryCompiler` | JIT/LLVM adaptive compilation |
| `alignment_examples.h` | (examples) | Documentation examples only |
| `alignment_helpers.h` | `AlignmentHelpers` | Cache-line + SIMD alignment |
| `allocator.h` | `NumaAllocator`, `HugePageAllocator` | NUMA and huge-page allocation |
| `cicada.h` | `CicadaContentionManager` | MVCC optimistic concurrency |
| `cycle_metrics.h` | `CycleMetrics` | PMU/RDTSC/CNTVCT_EL0 |
| `cycle_metrics_config.h` | `CycleMetricsConfig` | Backend selection |
| `dostoevsky.h` | `DostoevskyFilter` | LSM compaction cost model |
| `expected_cycles.h` | `ExpectedCycles` | Pre-computed cycle budgets |
| `feature_flags.h` | `FeatureFlags` | Runtime feature flag registry |
| `feature_flags_examples.h` | (examples) | Documentation examples only |
| `hardware_accelerator.h` | `HardwareAccelerator` | GPU/FPGA/AVX-512 dispatch |
| `huge_pages.h` | `HugePageManager` | THP lifecycle |
| `intelligent_prefetcher.h` | `IntelligentPrefetcher` | Pattern-learning prefetch |
| `ligra.h` | `LigraGraphProcessor` | Parallel graph primitives |
| `lirs_cache.h` | `LirsCache` | LIRS cache replacement |
| `lockfree_metrics_buffer.h` | `LockFreeMetricsBuffer` | SPSC/MPSC ring buffer |
| `numa_topology.h` | `NumaTopology` | NUMA node discovery |
| `phase2_feature_flags.h` | `Phase2FeatureFlags` | Phase-2 experimental flags |
| `prefetch_hints.h` | `PrefetchHints` | `__builtin_prefetch` wrappers |
| `rabitq.h` | `RabitqIndex` | Binary quantization ANN |
| `rcu.h` | `RcuDomain` | Userspace RCU domain |
| `rcu_hash_table.h` | `RcuHashTable` | RCU-protected hash table |
| `runtime_config.h` | `RuntimeConfig` | Hot-reload runtime config |
| `wisckey.h` | `WiscKeyStore` | Key-value separation LSM |
| `workload_predictor.h` | `WorkloadPredictor` | ML workload prediction |

## Findings

### Resolved
- macOS kperf backend uses `dlopen` to avoid hard link dependency.
- Windows fallback uses `QueryThreadCycleTime` + `__rdtsc` correctly gated by `_WIN32`.

### Open
- None.
