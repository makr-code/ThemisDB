> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/performance/ARCHITECTURE.md -->

# Performance Module — Public Header Architecture

**Module Path:** `include/performance/`
**Implementation:** `../../src/performance/`
**Canonical architecture doc:** [`../../src/performance/ARCHITECTURE.md`](../../src/performance/ARCHITECTURE.md)

---

## 1. Overview

`include/performance/` defines the **public measurement, optimization, and hardware-aware tuning contract** for ThemisDB. The 43 headers cover low-level timing, runtime config and feature flags, cache/NUMA tuning, adaptive optimization, hardware acceleration, and phase-specific advanced performance components.

For runtime composition details — adaptive optimizer behavior, accelerator fallback logic, and benchmarking/export internals — see:
→ [`../../src/performance/ARCHITECTURE.md`](../../src/performance/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Measurement and Runtime Control

| Header | Public Type | Purpose |
|--------|------------|---------|
| `cycle_metrics.h`, `cycle_metrics_config.h`, `expected_cycles.h` | Cycle-metrics types | Low-level timing and expected-cycle baselines |
| `runtime_config.h`, `feature_flags.h`, `phase2_feature_flags.h` | Runtime config and feature flags | Performance feature toggles |
| `feature_flags_examples.h`, `alignment_examples.h` | Example/support headers | Feature-flag and alignment usage examples |

### 2.2 Optimization and Adaptation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `adaptive_query_compiler.h` | `AdaptiveQueryCompiler` | Adaptive query compilation contract |
| `workload_predictor.h`, `workload_adaptive_optimizer.h` | Adaptive workload types | Workload prediction and optimization |
| `intelligent_prefetcher.h`, `prefetch_hints.h` | Prefetch types | Predictive prefetch control |
| `phase3/adaptive_batch_tuner.h`, `phase3/bao.h`, `phase3/per_query_cost_model.h` | Phase-3 optimizer types | Cost-aware and batch tuning |

### 2.3 Memory, Cache, and Concurrency

| Header | Public Type | Purpose |
|--------|------------|---------|
| `advanced_cache_manager.h`, `lirs_cache.h` | Cache-tuning types | Performance-oriented cache strategies |
| `allocator.h`, `numa_memory_manager.h`, `numa_topology.h`, `huge_pages.h` | Memory/NUMA types | Allocation and NUMA control |
| `rcu.h`, `rcu_hash_table.h`, `lockfree_histogram.h`, `lockfree_metrics_buffer.h` | Concurrency primitives | Lock-free data structures and metrics |
| `alignment_helpers.h` | Alignment helpers | Memory/alignment utility surface |

### 2.4 Hardware and Advanced Engines

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hardware_accelerator.h` | `HardwareAccelerator` | Capability-aware accelerator dispatch |
| `cicada.h`, `dostoevsky.h`, `wisckey.h`, `ligra.h`, `rabitq.h` | Engine/algorithm types | Specialized performance engines |
| `phase3/bwtree.h`, `phase3/diskann.h`, `phase3/gunrock.h`, `phase3/splinterdb.h` | Advanced phase-3 types | Data-structure / accelerator integrations |
| `phase4/io_uring_zero_copy.h`, `phase4/pmem_storage.h`, `phase4/pmu_counters.h` | Phase-4 types | IO/PMEM/PMU performance surfaces |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::performance` | Performance measurement and optimization types |
| `themis::performance::phase3` | Phase-3 advanced optimization types |
| `themis::performance::phase4` | Phase-4 hardware/IO types |

---

## 4. Public Contract Notes

- Runtime-config and feature-flag headers remain public so deployments can gate low-level performance features explicitly.
- Adaptive optimization and prefetch headers are public because query, cache, and server layers consume them directly.
- NUMA, allocator, and huge-page contracts provide deployment-level control without exposing implementation internals.
- Advanced phase-3/phase-4 headers remain public to document experimental but consumable optimization entry points.
