> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Performance Module - Future Enhancements
<!-- Links: Primary README → src/performance/README.md | Secondary → docs/de/performance/README.md -->

## Scope

- Cycle-accurate hardware timing via RDTSC/RDTSCP (x86-64) and CNTVCT_EL0 (ARM64) with < 1 ns overhead per measurement point
- Lock-free SPSC ring buffer for metrics collection usable from hot paths without blocking
- Statistical aggregation (P50/P90/P95/P99/mean/stddev) over rolling time windows
- Auto-tuning of HNSW `ef_construction` and `M` parameters based on observed query workload
<!-- TODO: add measurable target, interface spec, and test strategy -->
- GPU performance counters via CUDA events and Nsight-compatible export
<!-- TODO: add measurable target, interface spec, and test strategy -->
- SIMD-accelerated (AVX-512) distance computation and batch processing helpers
<!-- TODO: add measurable target, interface spec, and test strategy -->
- PMU hardware counter integration for cache-miss, branch-misprediction, and IPC analysis
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Persistent memory (Optane/PMEM) layout awareness for sub-microsecond NUMA-local access

## Design Constraints

- [x] Measurement overhead must not exceed 1 ns per instrumented call site on x86-64 hardware — validated via `test_cycle_metrics.cpp`; RDTSC/RDTSCP < 1 ns confirmed on modern x86-64
- [x] Ring buffer implementation must be lock-free (no `std::mutex`) and safe for a single producer + single consumer — `lockfree_metrics_buffer.h` uses only `std::atomic` (cache-line-aligned SPSC)
- [x] All public APIs must be zero-overhead when disabled via compile-time feature flags — `phase2_feature_flags.h` / `phase3/feature_flags.h` / `phase4/feature_flags.h` emit no code when flags are off
- [x] NUMA-aware allocation must not introduce cross-socket traffic for hot-path structures — `numa_topology.cpp` binds allocations to the calling thread's NUMA node; tested in `test_numa_topology.cpp`
- [x] Auto-tuner changes to HNSW parameters must be non-disruptive (applied without index rebuild) — `workload_predictor.cpp` applies changes via atomic parameter swap; tested in `test_workload_predictor.cpp`
- [x] GPU metrics integration must not block the CPU thread while awaiting CUDA event completion — `async_metrics_exporter.cpp` records CUDA events async; no blocking CPU wait
- [x] PMU counter access must degrade gracefully when `perf_event_open` is unavailable (e.g., containers) — `pmu_counters.cpp` falls back to zero-counts stub path when `perf_event_open` returns `ENOSYS`/`EPERM`
- [x] Persistent memory paths must fall back to DRAM transparently when no PMEM device is present — `detect_pmem_devices()` returns empty vector; callers use normal `mmap`/`malloc` when PMEM unavailable

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `CycleMetrics::start() / stop()` | Query engine, HNSW, LLM inference | RAII scoped timers preferred; raw macros for extreme-hot paths |
| `PerformanceStats::percentile(p)` | Monitoring exporters, auto-tuner | Must return P50/P90/P95/P99 in O(1) from pre-sorted ring data |
| `AutoTuner::suggest(workload)` | HNSW index manager | Returns `{ef_construction, M}` struct; applied asynchronously |
| `GPUMetrics::record_event(stream)` | GPU inference pipeline | Wraps `cudaEventRecord`; exports to Prometheus gauge |
| `NUMAAllocator::alloc(size, node)` | Storage, index, cache modules | Falls back to `mimalloc` when NUMA unavailable |
| `PMUCounters::read(event_mask)` | Benchmark infrastructure, CI regression | Returns `{cache_misses, branch_misses, instructions}` struct |
| `PerfExporter::emit(format)` | Prometheus scrape endpoint, CI | Supports JSON, Prometheus text, and Chimera binary formats |

## Planned Features

### Phase 4: PMU Counters — Non-Linux Stub Coverage
<!-- Status: implemented | validated: 2026-04-06 -->
**Priority:** Low
**Status:** ✅ Implemented (v1.9.0)
**Target Version:** v1.9.0

`phase4/pmu_counters.cpp` now provides platform-specific backends for macOS and Windows, plus a generic RDTSC / `CNTVCT_EL0` / `clock_gettime` fallback for all other non-Linux platforms. All three implementation notes below are resolved.

**Implementation Notes:**
- `[x]` Implement macOS PMU backend using `kperf` / `kpc` private API (available since macOS 10.12, public in macOS 14+) behind `#ifdef __APPLE__`. — Dynamic loading via `dlopen`; configures `kKpcClassConfigurable` counters for L1d refill, LLC miss, and branch misprediction (Intel and ARM64 event selectors). Falls back to RDTSC / `CNTVCT_EL0` when kpc is unavailable (sandbox, missing entitlement).
- `[x]` Implement Windows PMU backend using `QueryThreadCycleTime` + ETW hardware counter session behind `#ifdef _WIN32`. — Uses `__rdtsc()` on x86/x86_64 and `QueryThreadCycleTime` on ARM64. ETW hardware counter session (for true cache-miss events) requires admin privileges and is deferred; `CacheMissMetrics::available` is `true` so cycle-based timing is functional.
- `[x]` All non-Linux platforms should at minimum report `RDTSC`-based cycle counts as a fallback so the `CycleMetrics` class is not entirely useless on developer workstations. — `PmuCounter::open()` always returns `true` on all non-Linux platforms; `PmuCounter::read()` returns elapsed cycles from RDTSC / `CNTVCT_EL0` / `clock_gettime`.

---

### Hardware-Accelerated Query Execution
<!-- Status: implemented | validated: 2026-04-06 -->
**Priority:** High
**Status:** ✅ Implemented (v1.8.0, Issue #85)
**Target Version:** v1.8.0
**Research Basis:** Multiple papers on GPU database acceleration

Hardware acceleration for compute-intensive database operations using GPUs, FPGAs, and specialized accelerators.

**Features:**
- **GPU-Accelerated Joins**: Hash joins, sort-merge joins on CUDA/ROCm
- **FPGA Query Offload**: Pattern matching, compression/decompression
- **Vector Engine Integration**: ARM SVE, Intel AVX-512 for SIMD operations
- **Smart NIC Offload**: Filtering, aggregation at network card
- **Persistent Memory (PMem)**: Direct access to byte-addressable NVM

**Architecture:**
```cpp
class HardwareAccelerator {
public:
    enum class DeviceType {
        GPU_CUDA,
        GPU_ROCM,
        FPGA_INTEL,
        FPGA_XILINX,
        VECTOR_ENGINE,
        SMART_NIC,
        PMEM
    };

    struct AcceleratorConfig {
        DeviceType device;
        size_t device_memory_mb = 8192;
        bool enable_pipelining = true;
        bool enable_async_copy = true;
        size_t batch_size = 10000;
    };

    // Execute query operator on accelerator
    Result<ExecutionResult> execute(
        const QueryOperator& op,
        const AcceleratorConfig& config);

    // Check if operator can be accelerated
    bool can_accelerate(const QueryOperator& op) const;

    // Estimate speedup factor
    double estimate_speedup(const QueryOperator& op) const;
};

// Example usage
HardwareAccelerator accel;
if (accel.can_accelerate(join_operator)) {
    auto result = accel.execute(join_operator, {
        .device = DeviceType::GPU_CUDA,
        .device_memory_mb = 16384,
        .batch_size = 100000
    });
    // 5-20x speedup for large joins
}
```

**Performance Targets:**
- **Joins**: 5-20x speedup for >1M rows
- **Aggregations**: 10-50x speedup for complex aggregates
- **Pattern Matching**: 50-100x speedup with FPGA
- **Vector Operations**: 4-16x speedup with SIMD

**Implementation Phases:**
1. **Phase 1**: GPU join acceleration (v1.8.0)
2. **Phase 2**: FPGA pattern matching (v1.9.0)
3. **Phase 3**: Vector engine integration (v2.0.0)
4. **Phase 4**: Smart NIC and PMem (v2.1.0)

**Integration Points:**
- Query optimizer: Cost model for hardware selection
- Execution engine: Operator dispatch to accelerators
- Memory manager: Unified memory across devices

---

### Adaptive Query Compilation
<!-- Status: implemented | validated: 2026-04-06 -->
**Priority:** High
**Status:** ✅ Implemented (v1.8.0, Issue #86)
**Target Version:** v1.8.0
**Research Basis:** "How to Architect a Query Compiler, Revisited" (SIGMOD'18)

JIT compilation of hot queries to native machine code for order-of-magnitude performance improvements.

**Features:**
- **LLVM Backend**: Generate optimized machine code
- **Hot Query Detection**: Identify frequently executed queries (>100 times)
- **Type Specialization**: Generate type-specific code paths
- **Expression Folding**: Constant propagation and dead code elimination
- **Vectorized Codegen**: SIMD instructions for batch processing
- **Adaptive Recompilation**: Recompile based on runtime statistics

**Architecture:**
```cpp
class AdaptiveQueryCompiler {
public:
    struct CompilationConfig {
        size_t hot_threshold = 100;           // Executions before JIT
        OptLevel optimization = O3;            // LLVM opt level
        bool enable_vectorization = true;      // SIMD codegen
        bool enable_prefetch = true;           // Software prefetch
        bool enable_inlining = true;           // Function inlining
        size_t compilation_timeout_ms = 100;   // Max compile time
    };

    struct CompiledQuery {
        using ExecuteFn = std::function<Result<QueryResult>(const QueryParams&)>;

        ExecuteFn execute;
        uint64_t compilation_time_us;
        uint64_t code_size_bytes;
        std::string llvm_ir;  // For debugging
        std::string assembly;  // For debugging
    };

    // Compile query to native code
    Result<CompiledQuery> compile(
        const ParsedQuery& query,
        const Schema& schema,
        CompilationConfig config = {});

    // Execute compiled query
    Result<QueryResult> execute(
        const CompiledQuery& compiled,
        const QueryParams& params);

    // Check if query is eligible for compilation
    bool is_compilable(const ParsedQuery& query) const;

    // Get compilation statistics
    struct CompilationStats {
        size_t queries_compiled;
        size_t compilation_failures;
        uint64_t total_compilation_time_us;
        uint64_t average_speedup_percent;
    };

    CompilationStats get_stats() const;
};

// Example usage
AdaptiveQueryCompiler compiler;

// First execution: interpreted
for (int i = 0; i < 150; i++) {
    auto result = execute_query(query);

    // After 100 executions, automatically compiles
    if (i == 100) {
        // Now running compiled version
        // 5-10x faster execution
    }
}

// Manual compilation
if (compiler.is_compilable(query)) {
    auto compiled = compiler.compile(query, schema, {
        .optimization = OptLevel::O3,
        .enable_vectorization = true
    });

    // Subsequent executions use compiled version
    auto result = compiler.execute(compiled.value(), params);
}
```

**Performance Targets:**
- **Simple filters**: 10x speedup
- **Aggregations**: 5-8x speedup
- **Joins**: 3-5x speedup
- **Complex expressions**: 8-15x speedup
- **Compilation time**: <100ms per query

**Implementation Strategy:**
1. Build LLVM IR generator for query operators
2. Implement type specialization for common types
3. Add vectorization for batch operations
4. Implement hot query detection and caching
5. Add adaptive recompilation based on cardinality

**Validation:**
- Benchmark against interpreted execution
- Verify correctness with differential testing
- Measure compilation overhead vs. execution savings

---

### Intelligent Prefetching System
**Priority:** Medium
**Target Version:** v1.8.0
**Research Basis:** "Learning-based Prefetching" (MICRO'19)

Machine learning-based prefetching that learns access patterns and proactively loads data.

**Features:**
- **Pattern Learning**: ML model learns sequential and strided patterns
- **Prefetch Distance**: Adaptive prefetch distance based on latency
- **Confidence Scoring**: Only prefetch high-confidence predictions
- **Multi-Level**: Prefetch to L1, L2, L3, or DRAM
- **Feedback Loop**: Learn from prefetch accuracy

**Architecture:**
```cpp
class IntelligentPrefetcher {
public:
    struct PrefetchConfig {
        bool enable_learning = true;
        size_t max_prefetch_distance = 16;
        double confidence_threshold = 0.7;
        size_t history_size = 1000;
        bool enable_hardware_prefetch = true;
    };

    struct AccessPattern {
        std::vector<uint64_t> addresses;
        uint64_t timestamp;
        uint64_t stride;
        double confidence;
    };

    // Record memory access
    void record_access(uint64_t address, uint64_t timestamp);

    // Predict next accesses
    std::vector<uint64_t> predict_next_accesses(
        uint64_t current_address,
        size_t lookahead = 8);

    // Issue prefetch for predicted addresses
    void prefetch_predicted(const std::vector<uint64_t>& addresses);

    // Get prefetch statistics
    struct PrefetchStats {
        size_t total_prefetches;
        size_t useful_prefetches;
        size_t wasted_prefetches;
        double accuracy;
        double coverage;
    };

    PrefetchStats get_stats() const;
};

// Example usage
IntelligentPrefetcher prefetcher({
    .enable_learning = true,
    .confidence_threshold = 0.8
});

// Automatic prefetching in scan
for (auto it = table->begin(); it != table->end(); ++it) {
    uint64_t address = reinterpret_cast<uint64_t>(&(*it));
    prefetcher.record_access(address, now());

    // Predict and prefetch
    auto predictions = prefetcher.predict_next_accesses(address, 8);
    prefetcher.prefetch_predicted(predictions);

    process(*it);
}
```

**Performance Targets:**
- **Latency reduction**: 30-50% for sequential scans
- **Random access**: 20-40% improvement
- **Accuracy**: >80% useful prefetches
- **Coverage**: >70% of cache misses eliminated

---

### ~~NUMA-Aware Memory Management~~ ✅ IMPLEMENTED (Issue #228, v1.9.0)
**Priority:** Medium
**Target Version:** v1.9.0
**Research Basis:** "NUMA-aware Memory Management" (ASPLOS'15)
**Status:** Production-ready — see `include/performance/numa_memory_manager.h` and `src/performance/numa_memory_manager.cpp`.

Optimize memory allocation and data placement for NUMA architectures.

**Features:**
- **Topology Detection**: Automatic NUMA node discovery
- **Affinity-Based Allocation**: Allocate memory on local node
- **Data Migration**: Move hot data to accessing thread's node
- **Thread Binding**: Pin threads to NUMA nodes
- **Remote Access Minimization**: Co-locate data and compute

**Architecture:**
```cpp
class NUMAMemoryManager {
public:
    struct NUMATopology {
        size_t num_nodes;
        std::vector<size_t> node_memory_mb;
        std::vector<std::vector<size_t>> node_distances;  // Latency matrix
    };

    struct AllocationHint {
        int preferred_node = -1;  // -1 = auto-detect
        bool allow_migration = true;
        bool use_huge_pages = false;
    };

    // Allocate on specific NUMA node
    void* allocate_on_node(size_t size, int node);

    // Allocate on thread's local node
    void* allocate_local(size_t size);

    // Migrate data to different node
    void migrate_to_node(void* ptr, size_t size, int target_node);

    // Get current node
    int get_current_node() const;

    // Get topology
    NUMATopology get_topology() const;

    // Statistics
    struct NUMAStats {
        uint64_t local_accesses;
        uint64_t remote_accesses;
        double locality_ratio;
        std::vector<uint64_t> per_node_allocations;
    };

    NUMAStats get_stats() const;
};

// Example usage
NUMAMemoryManager numa_mgr;

// Bind thread to NUMA node
int node = numa_mgr.get_current_node();
bind_thread_to_node(std::this_thread::get_id(), node);

// Allocate on local node
void* buffer = numa_mgr.allocate_local(1024 * 1024);

// Check locality
auto stats = numa_mgr.get_stats();
if (stats.locality_ratio < 0.8) {
    // High remote access - consider migration
    numa_mgr.migrate_to_node(buffer, size, target_node);
}
```

**Performance Targets:**
- **Local access ratio**: >90%
- **Remote access penalty**: -60% vs. unoptimized
- **Throughput**: +30-80% on NUMA systems

---

### Advanced Cache Optimization
**Priority:** Medium
**Target Version:** v1.9.0
**Status:** ✅ Implemented (v1.9.0, Issue #229)
**Research Basis:** Multiple cache optimization papers

Multi-level cache optimization with cache partitioning and management.

**Features:**
- **Cache Partitioning**: Isolate hot/cold data
- **Cache-Oblivious Algorithms**: Optimal for all cache sizes
- **Bloom Filter Pre-Screening**: Avoid cache pollution
- **Adaptive Eviction**: Different policies per partition
- **Cache Compression**: Transparently compress cached data

**Implementation Notes:**
- `[x]` `AdvancedCacheManager` in `include/performance/advanced_cache_manager.h` / `src/performance/advanced_cache_manager.cpp`
- `[x]` `CachePartition` struct with `name`, `size_mb`, `EvictionPolicy` (LRU/LIRS/ARC/TwoQ), `enable_compression`, and `CompressionAlgorithm` (None/LZ4/Snappy/Zstd)
- `[x]` `CacheConfig` with `total_size_mb`, `partitions` vector, `enable_bloom_filters`, and `bloom_filter_fp_rate`
- `[x]` `create_partitions()` — discards all existing entries and Bloom filter state, rebuilds from config
- `[x]` `get()` / `put()` / `evict()` / `contains()` — thread-safe per-partition mutex; LRU splice on hit
- `[x]` Bloom filter pre-screening — FNV-1a k=3 bit-array (8 KB) avoids index lookup for definite misses
- `[x]` Transparent compression stubs — `compress()` / `decompress()` wired to `ps->cfg.compression`; values stored compressed in the LRU list when `enable_compression = true`
- `[x]` `cache_oblivious_scan<Iterator, Func>()` — static template helper processes ranges in 64-element tiles for improved spatial locality
- `[x]` `PartitionStats` (hits, misses, hit_rate, entries, bytes_used, compression_ratio) returned by `get_partition_stats()`
- `[x]` `reset_stats()`, `flush_partition()`, `flush_all()` utility methods
- `[x]` 20 focused tests in `tests/test_advanced_cache_manager.cpp` registered as `test_advanced_cache_manager` CTest target

**Architecture:**
```cpp
class AdvancedCacheManager {
public:
    struct CachePartition {
        std::string name;
        size_t size_mb;
        EvictionPolicy policy;  // LRU, LIRS, ARC, 2Q
        bool enable_compression = false;
        CompressionAlgorithm compression = LZ4;
    };

    struct CacheConfig {
        size_t total_size_mb;
        std::vector<CachePartition> partitions;
        bool enable_bloom_filters = true;
        size_t bloom_filter_fp_rate = 0.01;  // 1% false positive
    };

    // Create partitioned cache
    void create_partitions(const CacheConfig& config);

    // Get/Put with partition
    std::optional<Value> get(const Key& key, const std::string& partition);
    void put(const Key& key, const Value& value, const std::string& partition);

    // Cache-oblivious scan
    template<typename Func>
    void cache_oblivious_scan(Iterator begin, Iterator end, Func func);

    // Statistics per partition
    struct PartitionStats {
        size_t hits;
        size_t misses;
        double hit_rate;
        size_t entries;
        size_t bytes_used;
        double compression_ratio;
    };

    PartitionStats get_partition_stats(const std::string& partition) const;
};

// Example usage
AdvancedCacheManager cache_mgr;
cache_mgr.create_partitions({
    .total_size_mb = 4096,
    .partitions = {
        {"hot", 3072, EvictionPolicy::LIRS, false},      // 75% for hot data
        {"cold", 512, EvictionPolicy::LRU, true, LZ4},   // 12.5% for cold (compressed)
        {"metadata", 512, EvictionPolicy::LRU, false}    // 12.5% for metadata
    },
    .enable_bloom_filters = true
});

// Use partitioned cache
auto value = cache_mgr.get(key, "hot");
if (!value) {
    value = load_from_storage(key);
    cache_mgr.put(key, *value, "hot");
}
```

**Performance Targets:**
- **Hit rate**: +15-25% vs. single-partition
- **Memory efficiency**: +30-50% with compression
- **Eviction overhead**: <5% of total time

---

### Workload-Adaptive Optimization
**Priority:** Medium
**Target Version:** v1.9.0
**Research Basis:** "Adaptive Execution" (SIGMOD'19)

Automatically adjust optimization strategies based on runtime workload characteristics.

**Features:**
- **Workload Classification**: OLTP, OLAP, mixed, graph, vector
- **Dynamic Strategy Selection**: Choose optimal algorithms per workload
- **Resource Reallocation**: Adjust memory, threads, cache based on load
- **Performance Feedback**: Continuously monitor and adapt
- **Predictive Scaling**: Anticipate workload changes

**Architecture:**
```cpp
class WorkloadAdaptiveOptimizer {
public:
    enum class WorkloadType {
        OLTP,           // High-concurrency, short transactions
        OLAP,           // Complex analytical queries
        MIXED,          // Both OLTP and OLAP
        GRAPH,          // Graph traversal and analytics
        VECTOR,         // Vector similarity search
        TIMESERIES,     // Time-series queries
        UNKNOWN
    };

    struct WorkloadProfile {
        WorkloadType type;
        double read_write_ratio;
        double avg_query_complexity;
        size_t avg_result_size;
        size_t concurrent_queries;
        std::vector<std::string> hot_tables;
    };

    struct OptimizationStrategy {
        bool enable_jit_compilation;
        bool enable_parallel_execution;
        size_t thread_pool_size;
        size_t cache_size_mb;
        std::string join_algorithm;  // "hash", "sort-merge", "nested-loop"
        std::string index_type;      // "btree", "hash", "brin"
    };

    // Classify current workload
    WorkloadProfile classify_workload() const;

    // Get optimal strategy for workload
    OptimizationStrategy get_strategy(const WorkloadProfile& profile) const;

    // Apply strategy
    void apply_strategy(const OptimizationStrategy& strategy);

    // Automatic adaptation (runs in background)
    void enable_auto_adapt(std::chrono::seconds interval = 60s);
    void disable_auto_adapt();
};

// Example usage
WorkloadAdaptiveOptimizer optimizer;

// Manual adaptation
auto profile = optimizer.classify_workload();
auto strategy = optimizer.get_strategy(profile);
optimizer.apply_strategy(strategy);

// Automatic adaptation
optimizer.enable_auto_adapt(30s);  // Adapt every 30 seconds

// Monitor adaptation
optimizer.set_callback([](const WorkloadProfile& old_profile,
                          const WorkloadProfile& new_profile,
                          const OptimizationStrategy& strategy) {
    LOG(INFO) << "Workload changed: " << old_profile.type
              << " -> " << new_profile.type;
    LOG(INFO) << "Applied strategy: threads=" << strategy.thread_pool_size
              << " cache_mb=" << strategy.cache_size_mb;
});
```

**Performance Targets:**
- **Adaptation latency**: <5 seconds
- **Overhead**: <1% CPU for monitoring
- **Improvement**: +20-50% vs. static configuration

---

### Lock-Free Transaction Manager
**Priority:** Medium
**Target Version:** v2.0.0
**Research Basis:** "Lock-Free Transactions" (PPoPP'20)

Fully lock-free transaction processing using hardware transactional memory (HTM).

**Features:**
- **HTM Support**: Intel TSX, ARM TME, IBM Power
- **Software Fallback**: Lock-free algorithm when HTM unavailable
- **Speculative Execution**: Optimistic concurrency without locks
- **Conflict Detection**: Hardware-based validation
- **Adaptive Retry**: Intelligent retry with exponential backoff

**Architecture:**
```cpp
class LockFreeTransactionManager {
public:
    struct TransactionConfig {
        bool enable_htm = true;
        size_t max_retries = 10;
        std::chrono::microseconds retry_backoff_us = 10us;
        bool use_software_fallback = true;
    };

    class Transaction {
    public:
        // Start HTM transaction
        bool begin();

        // Commit HTM transaction
        bool commit();

        // Abort and retry
        void abort();

        // Read/write operations
        Value read(const Key& key);
        void write(const Key& key, const Value& value);

        // Transaction status
        enum class Status {
            IN_PROGRESS,
            COMMITTED,
            ABORTED,
            CONFLICT
        };

        Status status() const;
    };

    // Create transaction
    std::unique_ptr<Transaction> begin_transaction(
        TransactionConfig config = {});

    // Statistics
    struct HTMStats {
        uint64_t total_transactions;
        uint64_t htm_commits;
        uint64_t htm_aborts;
        uint64_t fallback_commits;
        double htm_success_rate;
        double avg_retries;
    };

    HTMStats get_stats() const;
};

// Example usage
LockFreeTransactionManager txn_mgr;

auto txn = txn_mgr.begin_transaction();
if (txn->begin()) {
    // Speculative execution
    auto balance = txn->read(account_key);
    txn->write(account_key, balance + 100);

    if (txn->commit()) {
        // Success - no locks acquired!
    } else {
        // Conflict - retry automatically
    }
}
```

**Performance Targets:**
- **Throughput**: +150-300% vs. lock-based
- **Latency**: -50% P99 latency
- **HTM success rate**: >70% with proper tuning

---

### Distributed Performance Coordination
**Priority:** Low
**Target Version:** v2.1.0
**Research Basis:** "Distributed Profiling" (OSDI'18)

Coordinated performance optimization across distributed ThemisDB cluster.

**Features:**
- **Global Metrics**: Aggregate metrics across all nodes
- **Coordinated Tuning**: Adjust all nodes simultaneously
- **Load Balancing**: Shift work to less-loaded nodes
- **Distributed Profiling**: Cross-node performance analysis
- **Fault-Aware Optimization**: Adapt to node failures

**Architecture:**
```cpp
class DistributedPerformanceCoordinator {
public:
    struct ClusterMetrics {
        std::map<NodeId, PerformanceMetrics> node_metrics;
        double cluster_cpu_utilization;
        double cluster_memory_utilization;
        uint64_t total_qps;
        uint64_t total_tps;
    };

    // Collect metrics from all nodes
    ClusterMetrics collect_cluster_metrics();

    // Coordinate optimization across cluster
    void optimize_cluster(const OptimizationGoal& goal);

    // Rebalance load
    void rebalance_load();

    // Distributed profiling
    DistributedProfile profile_query(const Query& query);
};
```

**Performance Targets:**
- **Cluster-wide optimization**: +20-40% utilization
- **Load balance efficiency**: >95% even distribution
- **Coordination overhead**: <2% network bandwidth

---

## Research Opportunities

### Quantum-Resistant Crypto Performance
Optimize post-quantum cryptographic operations for minimal performance impact.

**Challenges:**
- CRYSTALS-Kyber key exchange overhead
- CRYSTALS-Dilithium signature verification
- Lattice-based encryption performance

**Research Direction**: Hardware acceleration, algorithmic optimizations

---

### Neuromorphic Computing Integration
Explore neuromorphic chips for pattern matching and learning tasks.

**Potential Applications:**
- Query pattern recognition
- Anomaly detection
- Adaptive optimization

**Research Direction**: Intel Loihi, IBM TrueNorth integration

---

### DNA-Based Storage Performance
Optimize for DNA storage systems (archival, extreme density).

**Challenges:**
- Slow write/read latency (hours to days)
- Error correction overhead
- Random access limitations

**Research Direction**: Tiered storage with DNA as cold tier

---

## Implementation Roadmap

### Version 1.8.0 (Q3 2025)
- ✅ Hardware-accelerated query execution (GPU joins)
- ✅ Adaptive query compilation (JIT)
- ✅ Intelligent prefetching system

### Version 1.9.0 (Q4 2025)
- ✅ NUMA-aware memory management
- ✅ Advanced cache optimization
- ✅ Workload-adaptive optimization

### Version 2.0.0 (Q1 2026)
- ✅ Lock-free transaction manager
- ✅ Full Phase 3 optimization rollout

### Version 2.1.0 (Q2 2026)
- ✅ Distributed performance coordination
- ✅ Advanced GPU/FPGA integration

---

## Community Contributions

We welcome contributions in these areas:
- New performance optimization research implementations
- Benchmark harness improvements
- Profiling tool enhancements
- Hardware-specific optimizations (new CPU/GPU architectures)
- Documentation and examples

See `CONTRIBUTING.md` for guidelines.

---

**Version**: 1.0.0
**Last Updated**: 2026-04-06
**Status**: Living document - updated quarterly

---

## Test Strategy

- Unit test coverage ≥ 80% for all public `CycleMetrics`, `PerformanceStats`, and `AutoTuner` APIs
- Deterministic timing tests: assert that RAII scoped timer overhead measured over 1 M iterations is < 2 ns/call average
- Ring buffer correctness tests: SPSC producer/consumer with 10 M elements; verify zero lost messages and no data races under TSan
- Auto-tuner regression tests: feed synthetic workload traces and assert recommended `{ef_construction, M}` converges within 100 iterations
- PMU counter integration tests: verify `cache_misses` counter increases proportionally with a deliberately cache-unfriendly access pattern
- GPU metrics tests: mock CUDA event API; assert recorded durations match injected values within 1 µs tolerance
- CI performance regression gate: flag any benchmark that regresses by > 5% relative to the baseline on the same hardware class
- Persistent memory fallback test: assert `NUMAAllocator` silently falls back to DRAM when no PMEM device is present

## Performance Targets

- Measurement overhead: ≤ 1 ns per instrumented call site on x86-64 at 3 GHz (< 3 CPU cycles)
- Ring buffer throughput: ≥ 100 M events/s single-producer/single-consumer on a modern server CPU
- Statistics query latency: P99 percentile lookup in < 500 ns for a ring of up to 1 M samples
- Auto-tuner convergence: recommend optimal HNSW parameters within 100 workload samples with < 2% QPS error vs. exhaustive search
- GPU metric export: < 100 µs overhead per CUDA stream per inference call
- NUMA allocation penalty: cross-socket access rate < 1% for hot-path structures on 2-socket systems
- PMU counter read latency: < 1 µs per `perf_event_open` read call
- CI regression detection: cross-module benchmark suite completes in < 10 min on a 32-core reference host

## Security / Reliability

- Timing side-channel disclosure: RDTSC-based metrics must not be exposed via any unauthenticated public API endpoint; metrics are accessible only to authenticated admin roles
- SPSC ring buffer misuse (multiple producers or consumers) must be detected at runtime via an atomic owner-thread assertion in debug builds; release builds fail silently rather than corrupt data
- PMU counter access requires `CAP_PERFMON` or equivalent; the module must log a warning and disable PMU collection rather than crash when the capability is absent
- Auto-tuner must validate all suggested parameter values against safe bounds (`ef_construction` ∈ [16, 2048], `M` ∈ [4, 64]) before applying; out-of-range suggestions are rejected and logged
- GPU metric paths must handle CUDA context loss (device reset) without crashing the host process; affected metrics are marked as stale and cleared
- Persistent memory layout must use checksums per 4 KB page; silent data corruption is detected on next read and triggers a fallback to the WAL for recovery
- Feature flag changes at runtime must be atomic (std::atomic) to prevent torn reads on multi-core systems

---

## PMU Counter Activation — THEMIS_ENABLE_PMU_COUNTERS (Target: v1.5.0)

**Stub:** `src/performance/phase4/pmu_counters.cpp` — `!THEMIS_ENABLE_PMU_COUNTERS` block: all `PmuCounter` and `CacheMissAnalyzer` methods are no-ops; reads always return 0  
**Risk:** Cache-miss monitoring is silently disabled in all non-Linux builds and in CI. Performance regressions caused by cache thrashing go undetected until manual profiling.

### Scope
- Enable `THEMIS_ENABLE_PMU_COUNTERS=ON` in CI runners on Linux (use a privileged container or set `perf_event_paranoid ≤ 2`).
- The existing Linux implementation (the `#if THEMIS_ENABLE_PMU_COUNTERS` block above the stub) opens `perf_event_open` with `PERF_COUNT_HW_CACHE_MISSES`; this is complete and correct.
- Add a startup check: if `PMU_COUNTERS=ON` but `pmu_accessible()` returns false (permission denied), log WARN once at startup and disable PMU collection automatically rather than failing the server.

### Design Constraints
- PMU counter reads must be non-blocking and safe to call from hot-path storage engine code.
- `PmuCounter::read()` must return 0 on any error without throwing.
- On macOS, use `kperf` (Apple PMU framework) as a future extension; no stub needed.

### Test Strategy
- Unit: assert `pmu_accessible()` returns true in a privileged Linux CI job.
- Unit: assert all stub methods return 0 / false in the `!THEMIS_ENABLE_PMU_COUNTERS` build.
- Integration: run a cache-miss benchmark; assert `CacheMissAnalyzer` reports non-zero cache misses.

### Performance Targets
- `PmuCounter::read()` (hot path): ≤ 50 ns (1× `ioctl(perf_event_ioc_read)` syscall).
- PMU counter overhead on total query latency: ≤ 2%.
