> **Build:** `cmake --preset release && cmake --build build/release`

# Performance Optimization Headers

<!-- status: current | validated: 2026-04-06 -->
<!-- Links: Primary → src/performance/README.md | Secondary → docs/de/performance/README.md -->

## Overview

This directory contains public headers for ThemisDB's performance optimization infrastructure. These headers provide interfaces for research-based performance enhancements, hardware-level optimizations, profiling tools, and feature flags for runtime optimization control.

All optimizations are based on peer-reviewed academic research (45+ papers) with proven performance gains. The system uses zero-cost abstractions via compile-time flags to ensure zero overhead when features are disabled.

---

## Quick Reference

### Feature Flags
- `feature_flags.h` - Phase 1 optimizations (mimalloc, huge pages, RCU, LIRS)
- `phase2_feature_flags.h` - Phase 2 optimizations (WiscKey, Cicada, Ligra, RaBitQ, Dostoevsky)
- `phase3/feature_flags.h` - Phase 3 optimizations (DiskANN, Bw-Tree, SplinterDB, Gunrock, Bao)
- `feature_flags_examples.h` - Usage examples

### Performance Measurement
- `cycle_metrics.h` - Hardware cycle counters (RDTSC/CNTVCT_EL0)
- `cycle_metrics_config.h` - Zero-cost macros
- `expected_cycles.h` - Expected values for validation
- `lockfree_metrics_buffer.h` - Wait-free SPSC buffer
- `lockfree_histogram.h` - Lock-free histogram for latency distribution
- `runtime_config.h` - Runtime configuration API

### Memory Management
- `allocator.h` - Unified allocator interface (mimalloc/system)
- `huge_pages.h` - 2MB/1GB huge page support
- `numa_memory_manager.h` - NUMA-aware memory allocation
- `numa_topology.h` - NUMA topology detection and queries
- `alignment_helpers.h` - Cache-line/page alignment
- `alignment_examples.h` - Alignment usage examples

### Synchronization
- `rcu.h` - Read-Copy-Update (lock-free reads)
- `rcu_hash_table.h` - RCU-based concurrent hash table

### Cache Optimization
- `lirs_cache.h` - LIRS cache (scan-resistant)
- `prefetch_hints.h` - Software prefetch instructions

### Phase-Specific Optimizations
- `wisckey.h`, `cicada.h`, `ligra.h`, `rabitq.h`, `dostoevsky.h` - Phase 2
- `phase3/diskann.h`, `phase3/bwtree.h`, `phase3/splinterdb.h`, `phase3/gunrock.h`, `phase3/bao.h` - Phase 3
- `phase3/adaptive_batch_tuner.h` - Phase 3 adaptive batch tuning
- `phase3/memory_pressure.h` - Phase 3 memory pressure management
- `phase3/per_query_cost_model.h` - Phase 3 per-query cost model

### Workload-Aware Optimization
- `adaptive_query_compiler.h` - Adaptive query compilation
- `advanced_cache_manager.h` - Advanced multi-tier cache management
- `hardware_accelerator.h` - Hardware accelerator abstraction (SIMD/GPU)
- `intelligent_prefetcher.h` - ML-driven data prefetching
- `workload_adaptive_optimizer.h` - Workload-adaptive optimizer
- `workload_predictor.h` - Workload pattern prediction

---

## Public API and Configuration Surface

| Area | Primary Public Headers | Entry/Integration Notes |
|------|------------------------|-------------------------|
| Cycle metrics + export | `cycle_metrics.h`, `cycle_metrics_config.h`, `runtime_config.h` | Uses compile-time feature flags; integrates with exporters in `src/performance/*exporter*.cpp` |
| Runtime feature flags | `feature_flags.h`, `phase2_feature_flags.h`, `phase3/feature_flags.h`, `phase4/feature_flags.h` | Runtime toggles are effective only for features enabled at compile-time |
| Memory/NUMA | `allocator.h`, `huge_pages.h`, `numa_memory_manager.h`, `numa_topology.h` | NUMA/huge-page behavior depends on OS support and privileges |
| Concurrent structures | `rcu.h`, `rcu_hash_table.h`, `lockfree_metrics_buffer.h`, `lockfree_histogram.h` | `lockfree_metrics_buffer.h` is SPSC-only; violating this contract is unsupported |
| Workload-aware optimizers | `workload_predictor.h`, `workload_adaptive_optimizer.h`, `advanced_cache_manager.h` | Enables adaptive behavior based on observed workload profile |

---

## Usage Examples

### Enable Optimizations
```cpp
#include <performance/feature_flags.h>
#include <performance/phase2_feature_flags.h>

// Check if enabled
if (THEMIS_PERF_MIMALLOC_ENABLED()) {
    void* ptr = themis::memory::allocate(size);
}

// Runtime toggle
auto& flags = themis::performance::PerformanceFeatureFlags::instance();
flags.set_mimalloc_enabled(true);
```

### Measure Performance
```cpp
#include <performance/cycle_metrics.h>

uint64_t cycles;
THEMIS_SCOPED_CYCLE_TIMER(cycles);
// Code to measure
```

### Use Memory Allocators
```cpp
#include <performance/allocator.h>

void* buffer = themis::memory::allocate(1024 * 1024);
themis::memory::deallocate(buffer);
```

### RCU Synchronization
```cpp
#include <performance/rcu.h>

{
    themis::rcu::ReadLock guard;
    auto value = shared_data->read(key);
}
```

### LIRS Cache
```cpp
#include <performance/lirs_cache.h>

themis::performance::LIRSCache<Key, Value> cache(10000, 0.9);
cache.put(key, value);
```

---

## Build Configuration

```bash
# Phase 1
cmake -DTHEMIS_ENABLE_MIMALLOC=ON \
      -DTHEMIS_ENABLE_HUGE_PAGES=ON \
      -DTHEMIS_ENABLE_RCU_INDEX=ON \
      -DTHEMIS_ENABLE_LIRS_CACHE=ON

# Metrics
cmake -DTHEMIS_ENABLE_CYCLE_METRICS=ON \
      -DTHEMIS_ENABLE_METRICS_EXPORT=ON
```

---

## Runtime Behavior, Error Cases, and Limits

- Feature toggles are two-layered: CMake (`THEMIS_ENABLE_*`) gates compilation, runtime flags gate activation.
- PMU/GPU metrics and accelerator paths are platform-dependent and may degrade to fallback implementations.
- NUMA pinning and huge page allocation can fail due to host privileges or kernel configuration; callers should tolerate fallback behavior.
- `lockfree_metrics_buffer.h` must be used with one producer and one consumer only.
- Full instrumentation (`THEMIS_BENCHMARK_MODE`) is intended for profiling/benchmarking and can increase runtime overhead.

## Troubleshooting

| Symptom | Check |
|---------|-------|
| Instrumentation compiles but no cycle metrics are exported | Verify `THEMIS_ENABLE_CYCLE_METRICS=ON` and `THEMIS_ENABLE_METRICS_EXPORT=ON` during CMake configure |
| Runtime `set_*_enabled(true)` calls do not change behavior | Confirm corresponding feature was compiled in via `THEMIS_ENABLE_*` |
| Missing GPU/PMU metrics fields | Validate platform/tooling support (CUDA, perf permissions) and rely on fallback counters where unavailable |
| Inconsistent metrics stream under load | Verify strict SPSC usage for lock-free metrics buffers |

---

## Performance Gains

| Optimization | Workload | Gain |
|-------------|----------|------|
| mimalloc | General | +10-20% |
| Huge Pages | Memory-intensive | +15-30% |
| RCU | Read-heavy | +200-500% |
| LIRS | Cache-sensitive | +30-40% |
| WiscKey | Write-heavy | +40-60% |
| Cicada | OLTP | +100-150% |
| Ligra | Graph | +200-300% |
| RaBitQ | Vector | +50-80%, 16x memory |
| DiskANN | Billion-scale | +300-400% |
| Gunrock | GPU graph | +1000-3000% |

---

## Documentation

- **Module implementation overview**: [`../../src/performance/README.md`](../../src/performance/README.md)
- **Architecture**: [`../../src/performance/ARCHITECTURE.md`](../../src/performance/ARCHITECTURE.md)
- **Roadmap**: [`../../src/performance/ROADMAP.md`](../../src/performance/ROADMAP.md)
- **Future enhancements**: [`../../src/performance/FUTURE_ENHANCEMENTS.md`](../../src/performance/FUTURE_ENHANCEMENTS.md)
- **Performance targets/SLO mapping**: [`../../src/performance/PERFORMANCE_EXPECTATIONS.md`](../../src/performance/PERFORMANCE_EXPECTATIONS.md)
- **Module overview (DE)**: [`../../docs/de/performance/README.md`](../../docs/de/performance/README.md)
- **Research papers**: [`../../docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md`](../../docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md)

---

**Last Updated**: 2026-05-13
**Version**: 1.2

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
