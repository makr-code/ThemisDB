<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Performance Module

## [1.0.0] — All Phases Complete
> All implementation phases are complete. The module is production-ready.

---

## Phase 5 — Zero-Copy I/O & Persistent Memory
### Added
- DPDK and io_uring zero-copy I/O (`phase4/io_uring_zero_copy.cpp`)
- Persistent memory (Intel Optane) aware storage layout (`phase4/pmh_metrics.cpp`)
- Cross-module CI regression detection for performance baselines

---

## Phase 4 — ML Workload Prediction & PMU Counters
### Added
- ML-based workload predictor for adaptive resource pre-allocation (`workload_predictor.cpp`)
- PMU hardware performance counter integration — cycle, instruction, cache, and branch metrics
- Adaptive LLM batch size tuning (`phase3/adaptive_batch_tuner.cpp`)
- Per-query cost model integration for execution planner feedback

---

## Phase 3 — NUMA, SIMD & Cache Optimization
### Added
- NUMA topology detection and thread pinning (`numa_topology.cpp`)
- AVX-512 SIMD vector distance computations (L2, cosine, inner product)
- LIRS cache eviction policy for query result and index caches
- Memory pressure monitoring with automatic cache eviction triggers
- jemalloc integration as an allocator backend option

---

## Phase 2 — GPU Metrics, HNSW Tuning & Feature Flags
### Added
- GPU cycle and throughput metrics — CUDA Nsight-compatible export (`cycle_metrics.cpp` GPU path)
- HNSW auto-tuner: automated `ef_construction` and `M` parameter search (`phase2_feature_flags.cpp`)
- Feature flags system for runtime-toggling of experimental performance paths
- Prometheus metrics exporter (`prometheus_exporter.cpp`)
- Chimera and async metrics exporters (`chimera_exporter.cpp`, `async_metrics_exporter.cpp`)

---

## Phase 1 — Core Metrics & Low-Level Primitives
### Added
- `CycleMetrics` — RDTSC/RDTSCP on x86-64, CNTVCT_EL0 on ARM64, CUDA GPU cycle counters (`cycle_metrics.cpp`)
- RAII scoped cycle timers for zero-overhead function/block profiling
- Lock-free SPSC ring buffer for metrics ingestion pipeline
- Statistical analysis: P50, P90, P95, P99 latency percentiles
- mimalloc allocator integration with huge-page and NUMA-aware allocation
- RCU (Read-Copy-Update) and wait-free buffer implementations
- Additional subsystem implementations: `cicada.cpp`, `dostoevsky.cpp`, `ligra.cpp`, `rabitq.cpp`, `wisckey.cpp`
