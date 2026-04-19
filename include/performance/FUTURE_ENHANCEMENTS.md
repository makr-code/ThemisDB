# Performance Module Headers - Future Enhancements

<!-- status: current | validated: 2026-04-06 -->
<!-- Links: Primary → src/performance/README.md | Secondary → docs/de/performance/README.md -->

## Scope

- API-level enhancements to `include/performance/` headers — public C++ interfaces for performance instrumentation
- PMU counter interface: `PerfCounters` with typed `PerfCounterType` enum and `get_all()` summary
- RDTSC timing API: header-only `CycleTimer` with `noexcept` start/stop/elapsed methods
- Lock-free histogram API: `LockFreeHistogram<T>` with `record()`, `percentile()`, and `reset()` methods
- Auto-tuning hook interface: `AutoTuneHook` callback type for adaptive cache and queue policy selection
- GPU metrics API: `GPUMetrics` struct with device utilization, memory bandwidth, and kernel timing

## Design Constraints

- [x] Timing API is `noexcept` and header-only for zero-overhead inclusion — `cycle_metrics.h` is header-only; all hot-path methods are `noexcept`
- [x] PMU interface is compile-time optional via `THEMIS_ENABLE_PMU_COUNTERS` preprocessor guard — `pmu_counters.h` guarded by `#ifdef THEMIS_ENABLE_PMU_COUNTERS`
- [x] Histogram API is lock-free and thread-safe using `std::atomic` operations only — `lockfree_metrics_buffer.h` uses cache-line-aligned atomics; no mutex
- [x] Auto-tuning hook must not allocate heap memory in the hot path — `WorkloadPredictor::record()` appends to a fixed-size ring; no dynamic allocation
- [x] GPU metrics API is conditionally compiled under `THEMIS_ENABLE_GPU` — `cycle_metrics.h` wraps GPU path in `#ifdef THEMIS_ENABLE_GPU_CYCLE_METRICS`
- [x] All public types must be trivially copyable or explicitly documented otherwise — POD `CycleSample`, `WorkloadSnapshot`, `PMUCounterSet` are trivially copyable; non-trivial types (`WorkloadPredictor`) explicitly documented

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `CycleTimer` | `include/performance/cycle_metrics.h` | RDTSC-based, `noexcept`, header-only |
| `PerfCounters` | planned PMU header | Requires `THEMIS_ENABLE_PMU`; wraps `perf_event_open` |
| `LockFreeHistogram<T>` | Query profiling, cache stats | `record()` ≤ 20 ns; lock-free via atomics |
| `AutoTuneHook` | `AdaptiveCache`, `LockFreeQueue` | Callback invoked on policy evaluation |
| `GPUMetrics` | `gpu_allocator.h`, query GPU path | Conditionally compiled; wraps CUDA or HIP counters |

## Planned Header Additions

### Advanced Profiling Headers
**Priority:** High
**Target Version:** v1.8.0

New headers for advanced profiling and diagnostics.

**Planned Headers:**

#### `profiler.h`
Comprehensive profiling infrastructure with flamegraph generation.

```cpp
#pragma once
#include <string>
#include <vector>

namespace themis::performance {

class Profiler {
public:
    // Start profiling session
    void start_session(const std::string& name);

    // End profiling session
    void end_session();

    // Profile scope (RAII)
    class ScopedProfile {
    public:
        ScopedProfile(const std::string& name);
        ~ScopedProfile();
    };

    // Export flamegraph
    std::string export_flamegraph() const;

    // Export perf-compatible format
    void export_perf_format(const std::string& filename) const;
};

// Macro for easy scoped profiling
#define THEMIS_PROFILE_SCOPE(name) \
    themis::performance::Profiler::ScopedProfile __profile_##__LINE__(name)

}  // namespace themis::performance
```

#### `memory_profiler.h`
Memory allocation profiling and leak detection.

```cpp
#pragma once
#include <cstddef>
#include <vector>

namespace themis::performance {

struct AllocationInfo {
    void* address;
    size_t size;
    const char* file;
    int line;
    uint64_t timestamp;
    std::vector<void*> callstack;
};

class MemoryProfiler {
public:
    // Track allocation
    void track_allocation(void* ptr, size_t size,
                         const char* file, int line);

    // Track deallocation
    void track_deallocation(void* ptr);

    // Get live allocations
    std::vector<AllocationInfo> get_live_allocations() const;

    // Detect leaks
    std::vector<AllocationInfo> detect_leaks() const;

    // Memory usage statistics
    struct MemoryStats {
        size_t total_allocated;
        size_t total_deallocated;
        size_t current_usage;
        size_t peak_usage;
        size_t allocation_count;
        size_t deallocation_count;
    };

    MemoryStats get_stats() const;
};

}  // namespace themis::performance
```

#### `cache_profiler.h`
Cache performance analysis and optimization suggestions.

```cpp
#pragma once
#include <cstdint>
#include <string>

namespace themis::performance {

struct CacheStats {
    uint64_t l1_hits;
    uint64_t l1_misses;
    uint64_t l2_hits;
    uint64_t l2_misses;
    uint64_t l3_hits;
    uint64_t l3_misses;
    uint64_t tlb_misses;
    double l1_hit_rate;
    double l2_hit_rate;
    double l3_hit_rate;
};

class CacheProfiler {
public:
    // Start cache profiling
    void start();

    // Stop cache profiling
    void stop();

    // Get cache statistics
    CacheStats get_stats() const;

    // Get optimization suggestions
    std::vector<std::string> suggest_optimizations() const;

    // Detect false sharing
    struct FalseSharingInfo {
        void* address;
        std::vector<int> thread_ids;
        uint64_t contention_count;
    };

    std::vector<FalseSharingInfo> detect_false_sharing() const;
};

}  // namespace themis::performance
```

---

### Hardware-Specific Optimization Headers
**Priority:** High
**Target Version:** v1.9.0

Headers for platform-specific optimizations.

**Planned Headers:**

#### `simd_helpers.h`
Cross-platform SIMD abstractions (SSE, AVX, AVX-512, NEON).

```cpp
#pragma once
#include <cstdint>

namespace themis::performance::simd {

// SIMD vector types (platform-independent)
struct Vec4f { float data[4]; };
struct Vec8f { float data[8]; };
struct Vec16f { float data[16]; };

// SIMD operations
Vec4f add(const Vec4f& a, const Vec4f& b);
Vec4f mul(const Vec4f& a, const Vec4f& b);
float horizontal_sum(const Vec4f& v);

// Load/Store
Vec4f load_aligned(const float* ptr);
Vec4f load_unaligned(const float* ptr);
void store_aligned(float* ptr, const Vec4f& v);

// SIMD capabilities detection
bool has_sse() noexcept;
bool has_avx() noexcept;
bool has_avx2() noexcept;
bool has_avx512() noexcept;
bool has_neon() noexcept;

}  // namespace themis::performance::simd
```

#### `gpu_allocator.h`
GPU memory allocation and management.

```cpp
#pragma once
#include <cstddef>

namespace themis::performance::gpu {

enum class MemoryType {
    DEVICE,         // GPU device memory
    HOST,           // CPU pinned memory
    UNIFIED,        // Unified memory (CPU+GPU accessible)
    MANAGED         // Automatically managed
};

class GPUAllocator {
public:
    // Allocate GPU memory
    void* allocate(size_t size, MemoryType type = MemoryType::DEVICE);

    // Deallocate GPU memory
    void deallocate(void* ptr);

    // Copy between host and device
    void copy_to_device(void* device_ptr, const void* host_ptr, size_t size);
    void copy_to_host(void* host_ptr, const void* device_ptr, size_t size);

    // Get GPU memory info
    struct GPUMemoryInfo {
        size_t total_memory;
        size_t free_memory;
        size_t used_memory;
    };

    GPUMemoryInfo get_memory_info() const;
};

}  // namespace themis::performance::gpu
```

#### `numa_allocator.h`
NUMA-aware memory allocation.

```cpp
#pragma once
#include <cstddef>
#include <vector>

namespace themis::performance::numa {

struct NUMATopology {
    size_t num_nodes;
    std::vector<size_t> node_memory_mb;
    std::vector<std::vector<size_t>> node_distances;
};

class NUMAAllocator {
public:
    // Get NUMA topology
    static NUMATopology get_topology();

    // Get current NUMA node
    static int get_current_node();

    // Allocate on specific node
    void* allocate_on_node(size_t size, int node);

    // Allocate on local node
    void* allocate_local(size_t size);

    // Migrate memory to different node
    void migrate_to_node(void* ptr, size_t size, int target_node);

    // Bind thread to NUMA node
    static void bind_thread_to_node(int node);

    // NUMA statistics
    struct NUMAStats {
        uint64_t local_accesses;
        uint64_t remote_accesses;
        double locality_ratio;
    };

    NUMAStats get_stats() const;
};

}  // namespace themis::performance::numa
```

---

### Lock-Free Data Structure Headers
**Priority:** Medium
**Target Version:** v1.9.0

Additional lock-free data structures for high-performance concurrency.

**Planned Headers:**

#### `lockfree_queue.h`
Multi-producer, multi-consumer lock-free queue.

```cpp
#pragma once
#include <atomic>
#include <optional>

namespace themis::performance {

template<typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t capacity);

    // Enqueue (lock-free)
    bool enqueue(const T& item);
    bool enqueue(T&& item);

    // Dequeue (lock-free)
    std::optional<T> dequeue();

    // Check if empty/full
    bool empty() const;
    bool full() const;

    // Get size (approximate)
    size_t size() const;
};

}  // namespace themis::performance
```

#### `lockfree_stack.h`
Lock-free stack implementation.

```cpp
#pragma once
#include <atomic>
#include <optional>

namespace themis::performance {

template<typename T>
class LockFreeStack {
public:
    // Push (lock-free)
    void push(const T& item);
    void push(T&& item);

    // Pop (lock-free)
    std::optional<T> pop();

    // Check if empty
    bool empty() const;

    // Get size (approximate)
    size_t size() const;
};

}  // namespace themis::performance
```

#### `wait_free_spsc_queue.h`
Wait-free single-producer, single-consumer queue.

```cpp
#pragma once
#include <atomic>
#include <optional>

namespace themis::performance {

template<typename T>
class WaitFreeSPSCQueue {
public:
    explicit WaitFreeSPSCQueue(size_t capacity);

    // Enqueue (wait-free for single producer)
    bool enqueue(const T& item);

    // Dequeue (wait-free for single consumer)
    std::optional<T> dequeue();

    // Check status
    bool empty() const;
    bool full() const;
    size_t size() const;
};

}  // namespace themis::performance
```

---

### Advanced Cache Management Headers
**Priority:** Medium
**Target Version:** v1.9.0

Enhanced cache management capabilities.

**Planned Headers:**

#### `adaptive_cache.h`
Self-tuning cache that adapts to workload.

```cpp
#pragma once
#include <string>

namespace themis::performance {

enum class CachePolicy {
    LRU,
    LIRS,
    ARC,     // Adaptive Replacement Cache
    TwoQ,    // 2Q algorithm
    CLOCK,
    AUTO     // Automatically select best
};

template<typename Key, typename Value>
class AdaptiveCache {
public:
    AdaptiveCache(size_t capacity, CachePolicy initial_policy = CachePolicy::AUTO);

    // Get/Put operations
    std::optional<Value> get(const Key& key);
    void put(const Key& key, const Value& value);

    // Policy control
    void set_policy(CachePolicy policy);
    CachePolicy get_policy() const;

    // Auto-tuning
    void enable_auto_tuning(bool enabled = true);

    // Statistics
    struct CacheStats {
        size_t hits;
        size_t misses;
        double hit_rate;
        CachePolicy current_policy;
        size_t entries;
        size_t bytes_used;
    };

    CacheStats get_stats() const;
};

}  // namespace themis::performance
```

#### `bloom_filter.h`
Space-efficient probabilistic set membership test.

```cpp
#pragma once
#include <cstddef>
#include <vector>

namespace themis::performance {

class BloomFilter {
public:
    // Create with expected elements and false positive rate
    BloomFilter(size_t expected_elements, double false_positive_rate);

    // Add element
    void add(const void* data, size_t len);

    // Check membership (may have false positives)
    bool contains(const void* data, size_t len) const;

    // Statistics
    size_t size_bytes() const;
    double estimated_fpp() const;
    size_t element_count() const;

    // Clear
    void clear();
};

}  // namespace themis::performance
```

---

### Performance Counter Headers
**Priority:** Medium
**Target Version:** v2.0.0

Hardware performance counter access.

**Planned Headers:**

#### `perf_counters.h`
Hardware performance counter interface.

```cpp
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace themis::performance {

enum class PerfCounterType {
    CYCLES,
    INSTRUCTIONS,
    CACHE_REFERENCES,
    CACHE_MISSES,
    BRANCH_INSTRUCTIONS,
    BRANCH_MISSES,
    PAGE_FAULTS,
    CONTEXT_SWITCHES,
    CPU_MIGRATIONS,
    TLB_MISSES,
    L1_DCACHE_LOADS,
    L1_DCACHE_LOAD_MISSES,
    LLC_LOADS,
    LLC_LOAD_MISSES
};

class PerfCounters {
public:
    // Start counting
    void start(const std::vector<PerfCounterType>& counters);

    // Stop counting
    void stop();

    // Get counter value
    uint64_t get(PerfCounterType counter) const;

    // Get all counters
    struct CounterValues {
        uint64_t cycles;
        uint64_t instructions;
        uint64_t cache_misses;
        double ipc;  // Instructions per cycle
        double cache_miss_rate;
    };

    CounterValues get_all() const;

    // Reset counters
    void reset();
};

}  // namespace themis::performance
```

---

### Compression and Encoding Headers
**Priority:** Low
**Target Version:** v2.0.0

Fast compression for in-memory data.

**Planned Headers:**

#### `fast_compression.h`
High-speed compression algorithms.

```cpp
#pragma once
#include <cstddef>
#include <vector>

namespace themis::performance {

enum class CompressionAlgorithm {
    LZ4,           // Fast compression
    ZSTD,          // Good compression ratio
    SNAPPY,        // Very fast
    NONE           // No compression
};

class FastCompression {
public:
    // Compress data
    std::vector<uint8_t> compress(
        const void* data,
        size_t size,
        CompressionAlgorithm algo = CompressionAlgorithm::LZ4);

    // Decompress data
    std::vector<uint8_t> decompress(
        const void* compressed_data,
        size_t compressed_size,
        CompressionAlgorithm algo = CompressionAlgorithm::LZ4);

    // Get compression ratio
    double get_compression_ratio(
        const void* data,
        size_t size,
        CompressionAlgorithm algo) const;

    // Benchmark algorithms
    CompressionAlgorithm select_best_algorithm(
        const void* sample_data,
        size_t sample_size) const;
};

}  // namespace themis::performance
```

---

## Header Organization Improvements

### Namespace Restructuring
**Priority:** Low
**Target Version:** v2.0.0

Better organize headers into sub-namespaces:

```
themis::performance::
  ├── profiling::     (Profiling headers)
  ├── memory::        (Memory management)
  ├── simd::          (SIMD operations)
  ├── gpu::           (GPU acceleration)
  ├── numa::          (NUMA optimization)
  ├── lockfree::      (Lock-free structures)
  ├── cache::         (Caching strategies)
  └── counters::      (Performance counters)
```

### Header Consolidation
Combine related small headers into larger, more cohesive headers while maintaining backward compatibility via forwarding headers.

---

## Documentation Improvements

### Interactive Examples
**Priority:** Low
**Target Version:** v2.1.0

Add interactive Jupyter notebooks demonstrating:
- Performance profiling workflows
- Optimization selection decision trees
- Benchmark result visualization
- Cache tuning guidelines

### API Reference Generation
Automated API documentation using Doxygen with:
- Detailed parameter descriptions
- Performance characteristics
- Usage examples
- Thread-safety guarantees
- Memory ownership semantics

---

## Testing Infrastructure

### Header-Only Unit Tests
**Priority:** Medium
**Target Version:** v1.9.0

Add header-only test utilities:

```cpp
// test_helpers.h
#pragma once
#include <performance/cycle_metrics.h>

namespace themis::performance::testing {

// Benchmark helper
template<typename Func>
uint64_t benchmark(Func&& func, size_t iterations = 1000);

// Assert performance
void assert_performance(
    uint64_t actual_cycles,
    uint64_t expected_cycles,
    double tolerance = 0.2);

// Compare implementations
struct ComparisonResult {
    uint64_t baseline_cycles;
    uint64_t optimized_cycles;
    double speedup;
};

template<typename BaselineFunc, typename OptimizedFunc>
ComparisonResult compare(BaselineFunc&& baseline,
                         OptimizedFunc&& optimized,
                         size_t iterations = 1000);

}  // namespace themis::performance::testing
```

---

## Integration Improvements

### Better CMake Integration
**Priority:** High
**Target Version:** v1.8.0

Improved CMake functions for performance feature configuration:

```cmake
# themis_configure_performance(<target>
#   PHASE1 [mimalloc huge_pages rcu lirs]
#   PHASE2 [wisckey cicada ligra rabitq dostoevsky]
#   PHASE3 [diskann bwtree splinterdb gunrock bao]
#   METRICS [cycle_metrics prometheus]
# )

themis_configure_performance(my_target
    PHASE1 mimalloc lirs
    METRICS cycle_metrics
)
```

## Package Manager Support
Support for vcpkg, Conan, and other package managers with performance optimizations as optional dependencies.

---

## Platform Support Expansion

### Additional Platforms
**Priority:** Medium
**Target Version:** v2.0.0

Extend platform support:
- **RISC-V**: Cycle counters, SIMD (RVV)
- **Apple Silicon**: M-series optimizations
- **IBM POWER**: Performance monitoring
- **WebAssembly**: Browser-based performance
- **Embedded ARM**: Cortex-M series

### Compiler Support
Extend compiler-specific optimizations:
- **Intel ICC**: Additional intrinsics
- **NVIDIA HPC SDK**: GPU optimizations
- **AMD AOCC**: Zen-specific optimizations

---

## Community Contributions

We welcome community contributions for:
- Additional lock-free data structures
- Platform-specific optimizations
- New cache policies
- Profiling tool integrations
- Documentation improvements
- Benchmark additions

See `CONTRIBUTING.md` for guidelines.

---

## Research Integration

### Upcoming Research
Monitor and integrate new research from:
- **SIGMOD/VLDB**: Database optimizations
- **OSDI/SOSP**: Systems optimizations
- **ASPLOS**: Architecture and systems
- **PPoPP**: Parallel processing
- **ISCA/MICRO**: Computer architecture

### Experimental Branch
Maintain experimental branch for cutting-edge research implementations before production readiness.

---

## Version Timeline

| Version | Date | Headers |
|---------|------|---------|
| v1.8.0 | Q3 2025 | Profiling headers, CMake improvements |
| v1.9.0 | Q4 2025 | Hardware-specific headers, lock-free structures |
| v2.0.0 | Q1 2026 | Performance counters, compression headers |
| v2.1.0 | Q2 2026 | Interactive docs, namespace restructuring |

---

**Last Updated**: 2026-04-06
**Status**: Living document - updated quarterly
**Maintainers**: ThemisDB Performance Team

---

## Test Strategy

- Unit tests for `CycleTimer`: verify elapsed time within 5% of `std::chrono` on same-CPU measurements
- Unit tests for `LockFreeHistogram`: concurrent `record()` from 8 threads; verify no data races under TSan
- Unit tests for `PerfCounters`: mock `perf_event_open`; verify counter enable/disable and reset semantics
- Integration tests: run `THEMIS_PROFILE_SCOPE` macro under GTest; assert flamegraph output is non-empty
- Compile-time tests: verify `THEMIS_ENABLE_PMU=0` excludes PMU symbols from translation unit
- Benchmark tests: assert `CycleTimer` overhead ≤ 10 ns and `LockFreeHistogram::record()` ≤ 20 ns on CI hardware

## Performance Targets

- RDTSC wrapper overhead ≤ 10 ns per measurement (2× RDTSC serialized reads)
- `LockFreeHistogram::record()` ≤ 20 ns per call under no contention
- `AutoTuneHook` invocation ≤ 100 µs end-to-end including policy switch
- `PerfCounters::get_all()` ≤ 500 ns (single `ioctl` call amortized across batch)
- `GPUMetrics` snapshot ≤ 1 ms (async query; result cached for 10 ms)
- Header inclusion compile overhead ≤ 50 ms incremental build impact per translation unit

## Security / Reliability

- PMU counters are not accessible without `CAP_PERFMON` or admin privilege; API returns `std::errc::permission_denied` gracefully
- Performance data (histograms, traces) must never include query text or user data
- `CycleTimer` must not be used as a security-sensitive RNG source; document this constraint explicitly in the header
- GPU metrics API must not expose raw kernel source addresses to unprivileged callers
- All header-only code must be free of UB under `-fsanitize=undefined`; CI enforces this
