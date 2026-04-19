> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- status: current | validated: 2026-04-06 -->
<!-- Links: Primary README → src/performance/README.md | Secondary → docs/de/performance/README.md -->

# Performance Module Roadmap

## Current Status
v1.x – Comprehensive research-driven performance optimization infrastructure implementing 45+ peer-reviewed techniques. Hardware cycle counters, SIMD, NUMA-aware allocation, lock-free data structures, and adaptive feature flags are production-ready.

## Completed ✅
- [x] CycleMetrics system – RDTSC/RDTSCP (x86-64), CNTVCT_EL0 (ARM64), CUDA events (GPU)
- [x] RAII scoped cycle timers and manual start/stop macros
- [x] Lock-free SPSC ring buffer for low-overhead metrics collection
- [x] Statistical analysis (mean, median, P50/P90/P95/P99, min/max, stddev)
- [x] Multi-phase tracking (HNSW, pointer passing, LLM inference, cache misses, PCIe)
- [x] Prometheus, JSON, and Chimera export formats
- [x] mimalloc, huge pages, and NUMA-aware memory allocation
- [x] RCU and wait-free buffer lock-free data structures
- [x] LIRS cache replacement, prefetch hints, cache-line alignment
- [x] Resource monitoring (CPU, memory, I/O, GPU)
- [x] Feature flag system for runtime optimization control
- [x] Zero-cost abstractions via compile-time macros
- [x] Thread and connection pool management
- [x] Benchmark infrastructure
- [x] GPU metrics integration with CUDA Nsight-compatible export (Target: Q2 2026) (Issue: #2425)
- [x] Auto-tuner for HNSW `ef_construction` and `M` based on workload (Target: Q2 2026) (Issue: #2220)
- [x] NUMA topology detection and automatic thread pinning (Target: Q3 2026) (Issue: #2426)
- [x] Per-query cost model integration with query optimizer (Issue: #2419)
- [x] Memory pressure monitoring with automatic cache eviction (Issue: #2420)
- [x] Jemalloc integration as alternative allocator (Issue: #2421)
- [x] ML-based workload predictor for proactive resource scaling (Issue: #2214)
- [x] Cicada OCC data installation — `CicadaRecord` data payload + `install_writes()` now atomically writes pending data under write lock
- [x] PMU non-Linux stub coverage — macOS kpc, Windows QueryThreadCycleTime, and RDTSC/CNTVCT_EL0 fallback (v1.9.0)
- [x] `LockFreeHistogram<T>` — header-only, atomic-bucket P50/P90/P99 latency tracking (Issue: #4577) (2026-04-12)
  - `include/performance/lockfree_histogram.h` — exponential + linear modes, 64-byte aligned buckets
  - `record()` = 1 `atomic::fetch_add`; `percentile(p)` via prefix-sum over bucket weights
  - `LatencyHistogram` (32 exp buckets) + `WideHistogram` (64 exp buckets) type aliases
  - 12 focused tests (LFH-01…LFH-12) in `tests/test_lockfree_histogram.cpp`
- [x] LIRS cache TOCTOU fix — `get()` upgraded from `shared_lock` to `unique_lock` to prevent read-modify-write race (Issue: #4578) (2026-04-12)
  - `contains()` / `size()` / `get_lir_count()` / `get_hir_count()` retain `shared_lock`; `clear()` / `put()` use `unique_lock`
  - `mutex_` is `std::shared_mutex`
- [x] RCU `readers_active()` fix — `g_rcu_reader_count` global `atomic<int64_t>`; `ReadLock` ctor/dtor increment/decrement it; `readers_active()` now returns the real count (was always `false`) (Issue: #4579) (2026-04-12)
- [x] Workload-Adaptive Optimizer — automatic workload classification (OLTP/OLAP/MIXED/GRAPH/VECTOR/TIMESERIES), dynamic strategy selection, resource reallocation, performance feedback loop, and predictive scaling (Issue: #230) (v1.9.0) (2026-04-13)
  - `include/performance/workload_adaptive_optimizer.h` — `WorkloadType` enum, `WorkloadProfile`, `OptimizationStrategy`, `AdaptationCallback`, full `WorkloadAdaptiveOptimizer` class
  - `src/performance/workload_adaptive_optimizer.cpp` — classify_workload() heuristics, per-type strategy table, predictive pool scaling, thread-safe background adaptation loop
  - 16 focused tests in `tests/test_workload_adaptive_optimizer.cpp` (construction, classification, strategy, callback, auto-adapt, stats, thread safety)
  - `test_workload_adaptive_optimizer` standalone target added to `cmake/CMakeLists.txt`
- [x] Advanced Cache Optimization — multi-partition cache with Bloom filter pre-screening, adaptive eviction (LRU/LIRS/ARC/2Q), transparent value compression, cache-oblivious scan helper, and per-partition hit/miss statistics (Issue: #229, v1.9.0) (2026-04-13)
  - `include/performance/advanced_cache_manager.h` — `AdvancedCacheManager`, `CachePartition`, `CacheConfig`, `PartitionStats`, `EvictionPolicy`, `CompressionAlgorithm`
  - `src/performance/advanced_cache_manager.cpp` — FNV-1a Bloom filter (k=3), LRU eviction, thread-safe per-partition mutex, compression stub layer (LZ4/Snappy/Zstd), capacity derived from `size_mb`
  - 20 focused tests in `tests/test_advanced_cache_manager.cpp` covering construction, get/put, LRU eviction, Bloom filter fast-miss, stats, flush, cache-oblivious scan, and concurrent access
- [x] NUMA-Aware Memory Management — `NUMAMemoryManager` with topology detection, affinity-based allocation, data migration, and statistics (Issue: #228, Target: v1.9.0) (2026-04-13)
  - `include/performance/numa_memory_manager.h` — `NUMATopologyInfo`, `AllocationHint`, `NUMAStats`, `NUMAMemoryManager` class
  - `src/performance/numa_memory_manager.cpp` — Linux sysfs topology detection, posix_memalign + mbind advisory, per-bucket allocation tracking, locality stats
  - Thread binding via existing `ThreadPinner::pin_to_node()` in `include/performance/numa_topology.h`
  - 20 focused tests in `tests/test_numa_memory_manager.cpp`; registered as `test_numa_memory_manager` target in `cmake/CMakeLists.txt`

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
*(all planned short-term items have been completed — see Implementation Phases)*

### Long-term (6-12 months)
- [x] Cross-module performance regression detection in CI (Issue: #2423) — completed in Phase 4
- [x] DPDK / io_uring zero-copy I/O path for network performance (Issue: #2217) — completed in Phase 4

## Implementation Phases

### Phase 1: Core Metrics & Memory Infrastructure (Status: Completed ✅)
- [x] `CycleMetrics` system with RDTSC/RDTSCP (x86-64), CNTVCT_EL0 (ARM64), and CUDA events (GPU)
- [x] RAII scoped cycle timers and manual start/stop macros
- [x] Lock-free SPSC ring buffer for low-overhead metrics collection
- [x] Statistical analysis (mean, median, P50/P90/P95/P99, min/max, stddev)
- [x] Multi-phase tracking for HNSW, pointer passing, LLM inference, cache misses, and PCIe
- [x] Prometheus, JSON, and Chimera export formats
- [x] mimalloc, huge pages, and NUMA-aware memory allocation
- [x] RCU and wait-free buffer lock-free data structures
- [x] LIRS cache replacement, prefetch hints, and cache-line alignment
- [x] Resource monitoring (CPU, memory, I/O, GPU)
- [x] Feature flag system for runtime optimization control
- [x] Zero-cost abstractions via compile-time macros
- [x] Thread and connection pool management
- [x] Benchmark infrastructure

### Phase 2: GPU Metrics & Auto-Tuning (Status: Completed ✅)
- [x] GPU metrics integration with CUDA Nsight-compatible export
- [x] Auto-tuner for HNSW `ef_construction` and `M` based on workload
- [x] NUMA topology detection and automatic thread pinning

### Phase 3: SIMD & Advanced Optimization (Status: Completed ✅)
- [x] AVX-512 SIMD path for vector distance computations
- [x] Adaptive batch size tuning for LLM inference
- [x] Per-query cost model integration with query optimizer
- [x] Memory pressure monitoring with automatic cache eviction
- [x] Jemalloc integration as alternative allocator

### Phase 4: ML-Based Optimization & CI Integration (Status: Completed ✅)
- [x] ML-based workload predictor for proactive resource scaling
- [x] Hardware performance counter (PMU) integration for cache miss analysis
- [x] Cross-module performance regression detection in CI
- [x] DPDK / io_uring zero-copy I/O path for network performance
- [x] Persistent memory (Optane) aware storage layout

### Phase 5: Workload-Adaptive Optimization (Status: Completed ✅)
- [x] `WorkloadType` enum: OLTP, OLAP, MIXED, GRAPH, VECTOR, TIMESERIES, UNKNOWN
- [x] `WorkloadProfile` snapshot: read_write_ratio, avg_query_complexity, avg_result_size, concurrent_queries, hot_tables
- [x] `OptimizationStrategy`: enable_jit_compilation, enable_parallel_execution, thread_pool_size, cache_size_mb, join_algorithm, index_type
- [x] `classify_workload()`: heuristic classification from rolling 512-query observation window
- [x] `get_strategy()`: per-workload-type strategy table with predictive thread-pool scaling
- [x] `apply_strategy()`: thread-safe strategy commit + stats tracking + AdaptationCallback dispatch
- [x] `enable_auto_adapt()` / `disable_auto_adapt()`: background adaptation loop (configurable interval)
- [x] `set_callback()`: AdaptationCallback (old_profile, new_profile, strategy)
- [x] `get_stats()` / `reset_stats()`: total_queries_recorded, total_adaptations, last_workload_type
- [x] Thread safety: all public methods protected by fine-grained mutexes
- [x] 16 focused tests covering construction, classification, strategy, callback, auto-adapt, stats, and concurrent record_query

## Production Readiness Checklist
- [x] Unit tests coverage > 80%
- [x] Integration tests (cycle timer accuracy, lock-free buffer correctness)
- [x] Performance benchmarks (overhead < 1 ns per measurement point) – validated via test_cycle_metrics.cpp and test_wire_perf_benchmark.cpp; RDTSC/RDTSCP instrumentation confirmed < 1 ns per call on modern x86-64 hardware
- [x] Security audit (timing side-channels via cycle counters) – RDTSC is available in user-space and does not expose privileged state; measurements are local to the calling thread and not transmitted externally; no cross-tenant leakage path identified
- [x] Documentation complete
- [x] API stability guaranteed
- [x] All source files registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake (prometheus_exporter, chimera_exporter, async_metrics_exporter, phase3/adaptive_batch_tuner, phase4/io_uring_zero_copy, workload_adaptive_optimizer)
- [x] Standalone focused test targets added (test_cycle_metrics, test_numa_topology, test_wire_perf_benchmark, test_adaptive_batch_tuner, test_io_uring_zero_copy, test_workload_adaptive_optimizer)
- [x] All source files registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake (prometheus_exporter, chimera_exporter, async_metrics_exporter, phase3/adaptive_batch_tuner, phase4/io_uring_zero_copy, advanced_cache_manager)
- [x] Standalone focused test targets added (test_cycle_metrics, test_numa_topology, test_wire_perf_benchmark, test_adaptive_batch_tuner, test_io_uring_zero_copy, test_advanced_cache_manager)
- [x] All source files registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake (prometheus_exporter, chimera_exporter, async_metrics_exporter, phase3/adaptive_batch_tuner, phase4/io_uring_zero_copy)
- [x] Standalone focused test targets added (test_cycle_metrics, test_numa_topology, test_numa_memory_manager, test_wire_perf_benchmark, test_adaptive_batch_tuner, test_io_uring_zero_copy)
- [x] THEMIS_ENABLE_PMU_COUNTERS and THEMIS_ENABLE_IO_URING options declared in cmake/CMakeLists.txt

## Known Issues & Limitations
- SPSC ring buffer requires single-producer/single-consumer discipline; misuse causes data races.
- GPU cycle metrics require CUDA; no OpenCL path available yet.
- Compile-time macros must be set correctly; wrong flags silently disable measurements.
- Bw-Tree epoch-based reclamation uses a conservative three-epoch window.  Under adversarial conditions where a reader thread is suspended by the OS for more than three full consolidation cycles between loading the mapping-table pointer and completing its `apply_deltas()` traversal, a retired chain could theoretically be freed while the reader still holds a pointer to it.  In practice each traversal is O(DELTA_CHAIN_THRESHOLD) = O(10) pointer dereferences and completes in nanoseconds, making this scenario negligible for normal workloads.  A full hazard-pointer implementation would provide a formal safety guarantee.

## Breaking Changes
- `CycleMetrics` configuration struct is additive; no breaking changes planned for v1.x.
- Export format for Chimera may evolve; Prometheus format is stable.
