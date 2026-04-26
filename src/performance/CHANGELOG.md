> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Performance Module

## [1.9.0] — PMU Non-Linux Platform Coverage

### Added
- **macOS kpc PMU backend** (`phase4/pmu_counters.cpp`, `#ifdef __APPLE__`): Dynamically loads `kperf` framework; configures `kKpcClassConfigurable` counters for L1d cache refill, LLC miss, and branch misprediction (Intel and Apple Silicon ARM64 event selectors). Falls back to RDTSC / `CNTVCT_EL0` when kpc access is denied (sandbox, missing entitlement). `CacheMissAnalyzer::pmu_accessible()` returns `true` via RDTSC fallback even without kpc.
- **Windows cycle-count backend** (`phase4/pmu_counters.cpp`, `#ifdef _WIN32`): Uses `__rdtsc()` on x86/x86_64 and `QueryThreadCycleTime` on ARM64 Windows. `PmuCounter::open()` always returns `true`; `CacheMissMetrics::available = true`. True cache-miss PMU events via ETW hardware counter session are deferred (require admin).
- **Generic RDTSC / CNTVCT_EL0 / clock_gettime fallback** (all other non-Linux platforms): `PmuCounter::open()` always returns `true`; `PmuCounter::read()` returns elapsed cycles. Replaces the previous pure-zero stubs so the performance measurement infrastructure is functional on all developer workstations.

---

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
