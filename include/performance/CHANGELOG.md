<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Performance Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/performance/CHANGELOG.md`.

## [1.9.0] — 2026-02

### Added
- `cycle_metrics.h` / `cycle_metrics_config.h` — macOS kperf/kpc PMU backend via `dlopen`; Intel + ARM64 event selectors. Windows `QueryThreadCycleTime` + `__rdtsc`. Generic RDTSC / CNTVCT_EL0 fallback.
- `workload_predictor.h` — `WorkloadPredictor` ML workload prediction with adaptive LLM batch tuning.
- `hardware_accelerator.h` — `HardwareAccelerator` GPU/FPGA/AVX-512 dispatch abstraction.

## [1.8.0] — 2025-12

### Added
- `adaptive_query_compiler.h` — `AdaptiveQueryCompiler` JIT/LLVM-based adaptive plan compilation.
- `lockfree_metrics_buffer.h` — `LockFreeMetricsBuffer` SPSC/MPSC lock-free ring buffer.
- `phase2_feature_flags.h` — Phase-2 experimental feature flag set.
- `rabitq.h` — `RabitqIndex` binary quantization ANN index.

## [1.7.0] — 2025-10

### Added
- `numa_topology.h` — `NumaTopology` NUMA node discovery and affinity helpers.
- `allocator.h` — `NumaAllocator` and `HugePageAllocator`.
- `alignment_helpers.h` / `alignment_examples.h` — AVX-512 / cache-line alignment utilities.
- `rcu.h` / `rcu_hash_table.h` — Userspace RCU domain and RCU-protected hash table.

## [1.5.0] — 2025-07

### Added
- `intelligent_prefetcher.h` — Pattern-learning hardware prefetch hint issuer.
- `prefetch_hints.h` — Portable `__builtin_prefetch` wrappers.
- `lirs_cache.h` — LIRS cache replacement policy.
- `wisckey.h` — WiscKey key-value separation for large value LSM.
- `dostoevsky.h` — Dostoevsky LSM compaction cost model filter.
- `ligra.h` — Ligra parallel graph processing primitives.

## [1.0.0] — 2025-01

### Added
- `feature_flags.h` / `feature_flags_examples.h` — Runtime-toggleable feature flag registry.
- `runtime_config.h` — Dynamic runtime configuration hot-reload.
- `huge_pages.h` — Transparent huge page lifecycle management.
- `cicada.h` — Optimistic concurrency MVCC contention manager.
- `expected_cycles.h` — Pre-computed expected cycle budgets.
