<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Performance Module — Architecture Guide

## Overview

The performance module provides low-level tuning primitives for ThemisDB: CPU cycle measurement (PMU/RDTSC/CNTVCT_EL0), NUMA topology awareness, huge-page allocators, lock-free metrics buffers, intelligent prefetchers, SIMD alignment helpers, hardware accelerator APIs, RCU (Read-Copy-Update), workload prediction, and feature-flag infrastructure. It underpins the query engine's adaptive compilation and cache subsystems.

## Design Principles

- **Cross-platform PMU** — `cycle_metrics.h` supports macOS kperf/kpc (dlopen), Windows `QueryThreadCycleTime`/`__rdtsc`, and generic RDTSC/CNTVCT_EL0 fallback.
- **NUMA-aware allocation** — `numa_topology.h` + `allocator.h` place hot data on the socket nearest the executing thread.
- **Lock-free hot paths** — `lockfree_metrics_buffer.h` and `rcu.h` / `rcu_hash_table.h` avoid contention on telemetry collection.
- **Prefetch-guided I/O** — `intelligent_prefetcher.h` and `prefetch_hints.h` issue hardware prefetch instructions ahead of access patterns.
- **Compile-time feature flags** — `feature_flags.h` + `phase2_feature_flags.h` enable/disable experimental subsystems without runtime overhead.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `adaptive_query_compiler.h` | `AdaptiveQueryCompiler` | JIT/LLVM-based adaptive query plan compilation |
| `alignment_examples.h` | (examples) | SIMD alignment usage examples |
| `alignment_helpers.h` | `AlignmentHelpers` | Cache-line and SIMD alignment utilities |
| `allocator.h` | `NumaAllocator`, `HugePageAllocator` | NUMA-local and huge-page memory allocators |
| `cicada.h` | `CicadaContentionManager` | Optimistic concurrency MVCC contention manager |
| `cycle_metrics.h` | `CycleMetrics` | Cross-platform CPU cycle counter |
| `cycle_metrics_config.h` | `CycleMetricsConfig` | Backend selection for PMU/RDTSC/CNTVCT_EL0 |
| `dostoevsky.h` | `DostoevskyFilter` | Dostoevsky LSM compaction cost model filter |
| `expected_cycles.h` | `ExpectedCycles` | Pre-computed expected cycle budgets per operation |
| `feature_flags.h` | `FeatureFlags` | Runtime-toggleable feature flag registry |
| `feature_flags_examples.h` | (examples) | Feature flag usage examples |
| `hardware_accelerator.h` | `HardwareAccelerator` | GPU/FPGA/AVX-512 accelerator dispatch |
| `huge_pages.h` | `HugePageManager` | Transparent huge page lifecycle management |
| `intelligent_prefetcher.h` | `IntelligentPrefetcher` | Pattern-learning hardware prefetch hint issuer |
| `ligra.h` | `LigraGraphProcessor` | Ligra-style parallel graph processing primitives |
| `lirs_cache.h` | `LirsCache` | LIRS (Low Inter-Reference Recency Set) cache replacement |
| `lockfree_metrics_buffer.h` | `LockFreeMetricsBuffer` | SPSC/MPSC lock-free ring buffer for metrics |
| `numa_topology.h` | `NumaTopology` | NUMA node discovery and affinity helpers |
| `phase2_feature_flags.h` | `Phase2FeatureFlags` | Phase-2 experimental feature flags |
| `prefetch_hints.h` | `PrefetchHints` | Portable `__builtin_prefetch` wrappers |
| `rabitq.h` | `RabitqIndex` | RaBitQ binary quantization index for ANN search |
| `rcu.h` | `RcuDomain` | Userspace RCU domain (quiescent-state based) |
| `rcu_hash_table.h` | `RcuHashTable` | RCU-protected concurrent hash table |
| `runtime_config.h` | `RuntimeConfig` | Dynamic runtime configuration hot-reload |
| `wisckey.h` | `WiscKeyStore` | WiscKey key-value separation for large value LSM |
| `workload_predictor.h` | `WorkloadPredictor` | ML-based workload pattern prediction and LLM batch tuning |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `query` | `AdaptiveQueryCompiler` | JIT compilation of hot query plans |
| `observability` | `CycleMetrics`, `LockFreeMetricsBuffer` | PMU-backed cycle counter for span timing |
| `storage` | `HugePageManager`, `NumaAllocator`, `WiscKeyStore` | Storage buffer and compaction tuning |
| `index` | `RabitqIndex`, `LirsCache` | ANN index and cache eviction |
| OS / hardware | `HardwareAccelerator`, `NumaTopology` | Direct system calls and CPU topology |

## Implementation

Implementation in `../../src/performance/`.
