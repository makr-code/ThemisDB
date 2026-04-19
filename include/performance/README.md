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
- `runtime_config.h` - Runtime configuration API

### Memory Management
- `allocator.h` - Unified allocator interface (mimalloc/system)
- `huge_pages.h` - 2MB/1GB huge page support
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

- **Full Documentation**: `src/performance/README.md`
- **Future Enhancements**: `src/performance/FUTURE_ENHANCEMENTS.md`, `include/performance/FUTURE_ENHANCEMENTS.md`
- **Research Papers**: `docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md`
- **Cycle Metrics**: `docs/performance/CYCLE_METRICS.md`

---

**Last Updated**: 2026-04-06
**Version**: 1.1

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
