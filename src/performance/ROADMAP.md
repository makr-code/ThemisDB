<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

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

## In Progress 🚧
- [ ] GPU metrics integration with CUDA Nsight-compatible export (Target: Q2 2026)
- [ ] Auto-tuner for HNSW `ef_construction` and `M` based on workload (Target: Q2 2026)
- [ ] NUMA topology detection and automatic thread pinning (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] AVX-512 SIMD path for vector distance computations
- [ ] Adaptive batch size tuning for LLM inference
- [ ] Per-query cost model integration with query optimizer
- [ ] Memory pressure monitoring with automatic cache eviction
- [ ] Jemalloc integration as alternative allocator

### Long-term (6-12 months)
- [ ] ML-based workload predictor for proactive resource scaling
- [ ] Hardware performance counter (PMU) integration for cache miss analysis
- [ ] Cross-module performance regression detection in CI
- [ ] DPDK / io_uring zero-copy I/O path for network performance
- [ ] Persistent memory (Optane) aware storage layout

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

### Phase 2: GPU Metrics & Auto-Tuning (Status: In Progress 🚧)
- [~] GPU metrics integration with CUDA Nsight-compatible export
- [~] Auto-tuner for HNSW `ef_construction` and `M` based on workload
- [~] NUMA topology detection and automatic thread pinning

### Phase 3: SIMD & Advanced Optimization (Status: Planned 📋)
- [ ] AVX-512 SIMD path for vector distance computations
- [ ] Adaptive batch size tuning for LLM inference
- [ ] Per-query cost model integration with query optimizer
- [ ] Memory pressure monitoring with automatic cache eviction
- [ ] Jemalloc integration as alternative allocator

### Phase 4: ML-Based Optimization & CI Integration (Status: Planned 📋)
- [ ] ML-based workload predictor for proactive resource scaling
- [ ] Hardware performance counter (PMU) integration for cache miss analysis
- [ ] Cross-module performance regression detection in CI
- [ ] DPDK / io_uring zero-copy I/O path for network performance
- [ ] Persistent memory (Optane) aware storage layout

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (cycle timer accuracy, lock-free buffer correctness)
- [ ] Performance benchmarks (overhead < 1 ns per measurement point)
- [ ] Security audit (timing side-channels via cycle counters)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- SPSC ring buffer requires single-producer/single-consumer discipline; misuse causes data races.
- GPU cycle metrics require CUDA; no OpenCL path available yet.
- Compile-time macros must be set correctly; wrong flags silently disable measurements.

## Breaking Changes
- `CycleMetrics` configuration struct is additive; no breaking changes planned for v1.x.
- Export format for Chimera may evolve; Prometheus format is stable.
