# Performance Optimization Headers

This directory contains headers for research-based performance optimizations.

## Files

### `feature_flags.h`
Core header for performance optimization feature flags.

**Usage:**
```cpp
#include <performance/feature_flags.h>

// Check if optimization is enabled
if (THEMIS_PERF_MIMALLOC_ENABLED()) {
    // Use optimized path
}

// Toggle at runtime
auto& flags = themis::performance::PerformanceFeatureFlags::instance();
flags.set_mimalloc_enabled(true);
```

**Features:**
- Compile-time feature flags (CMake)
- Runtime toggles (configuration)
- Thread-safe atomic operations
- Convenience macros

### `feature_flags_examples.h`
Example implementations showing how to use feature flags in various components.

**Examples Included:**
1. Memory allocation with mimalloc
2. Cache implementation with LIRS
3. Index access with RCU
4. Storage engine with WiscKey
5. Server startup configuration
6. Runtime monitoring

## Research Documentation

All optimizations are based on peer-reviewed research. See:
- [`docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md`](../../docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md)
- [`docs/de/research/IMPLEMENTATION_VALIDATION_GUIDE.md`](../../docs/de/research/IMPLEMENTATION_VALIDATION_GUIDE.md)

## Available Optimizations

### Phase 1: Quick Wins
- **Mimalloc**: Fast allocator (+10-20%)
- **Huge Pages**: 2MB/1GB pages (+15-30%)
- **RCU Index**: Read-Copy-Update (+200-500% reads)
- **LIRS Cache**: Improved cache policy (+30-40% hit rate)

### Phase 2: Medium-Term
- **WiscKey**: Value separation (+40-60% writes)
- **Cicada CC**: Optimistic concurrency (+100-150% TX)

### Phase 3: Long-Term
- **DiskANN**: Billion-scale vector search (+300-400%)
- **Bw-Tree**: Lock-free index (+100-200% updates)

## CMake Build Flags

```bash
# Enable specific optimizations at compile-time
cmake -B build -S . \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_RCU_INDEX=ON \
  -DTHEMIS_ENABLE_LIRS_CACHE=ON
```

## Runtime Configuration

```json
{
  "performance": {
    "enable_mimalloc": true,
    "enable_huge_pages": false,
    "enable_rcu_index": true,
    "enable_lirs_cache": true
  }
}
```

## Testing

Unit tests: `tests/test_performance_feature_flags.cpp`

```bash
# Run tests
./build/tests/themis_tests --gtest_filter=PerformanceFeatureFlags*
```

## Contributing

When adding a new optimization:
1. Add CMake option in `CMakeLists.txt`
2. Add flag to `feature_flags.h`
3. Create implementation in appropriate module
4. Add example to `feature_flags_examples.h`
5. Add test case to `tests/test_performance_feature_flags.cpp`
6. Document in research docs
7. Validate with benchmark framework

## References

- **ISMM'19**: Mimalloc allocator
- **FAST'14**: Huge pages optimization
- **ASPLOS'10**: RCU synchronization
- **SIGMETRICS'02**: LIRS cache policy
- **FAST'16**: WiscKey value separation
- **SIGMOD'17**: Cicada concurrency control
- **NeurIPS'19**: DiskANN vector search
- **ICDE'18**: Bw-Tree lock-free index

Full references: [`docs/de/research/`](../../docs/de/research/)

## Cycle-Based Performance Measurement

### Overview

ThemisDB includes a comprehensive cycle-based performance measurement system for system-independent performance comparison.

### Key Files

- **`cycle_metrics.h`**: Hardware cycle counter and metrics structures
- **`cycle_metrics_config.h`**: Zero-cost abstraction macros
- **`expected_cycles.h`**: Expected cycle counts for validation
- **`lockfree_metrics_buffer.h`**: Lock-free SPSC ring buffer
- **`runtime_config.h`**: Runtime configuration API

### Quick Start

```cpp
#include "performance/cycle_metrics.h"
#include "performance/cycle_metrics_config.h"

using namespace themis::performance;

// Measure cycles
uint64_t cycles;
THEMIS_MEASURE_CYCLES_START(cycles);
// ... your code ...
THEMIS_MEASURE_CYCLES_END(cycles);

// Or use RAII timer
{
    THEMIS_SCOPED_CYCLE_TIMER(cycles);
    // ... your code ...
}
```

### Build Flags

```bash
# Production (zero overhead)
cmake -B build -DTHEMIS_ENABLE_CYCLE_METRICS=OFF

# Monitored (low overhead)
cmake -B build -DTHEMIS_ENABLE_CYCLE_METRICS=ON -DTHEMIS_ENABLE_METRICS_EXPORT=ON

# Benchmark mode (all features)
cmake -B build -DTHEMIS_BENCHMARK_MODE=ON
```

### Performance Impact

| Configuration | Overhead | Use Case |
|--------------|----------|----------|
| All OFF | 0% | Production ✅ |
| Sampling 1:100 | <0.1% | Monitoring ✅ |
| Benchmark Mode | 5-10% | Testing only ⚠️ |

### Documentation

Full documentation: [`docs/performance/CYCLE_METRICS.md`](../../docs/performance/CYCLE_METRICS.md)

### Key Insight

The cycle metrics system proves that **pointer passing** in ThemisDB's RAG pipeline:
- Has ~150 cycles overhead (0.000019% of total)
- Is **53x faster** than copying 10KB of memory
- Is essentially **free** compared to HNSW search and LLM inference

---

**Last Updated**: 2025-12-24  
**Version**: 1.0
