<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Performance Module Roadmap

## Current Status

v1.9.0 — production. Cross-platform PMU cycle metrics, NUMA-aware allocation, lock-free buffers, RCU, intelligent prefetching, and ML workload predictor are operational.

## Completed

- [x] Feature flags (runtime registry + phase-2 flags)
- [x] Runtime config hot-reload
- [x] Huge page lifecycle management
- [x] Cicada MVCC contention manager
- [x] LIRS cache, WiscKey, Dostoevsky, Ligra
- [x] Intelligent prefetcher + prefetch hints
- [x] NUMA topology + NUMA/huge-page allocators
- [x] Alignment helpers (AVX-512/cache-line)
- [x] RCU domain + RCU hash table
- [x] Lock-free metrics buffer
- [x] RaBitQ binary quantization ANN index
- [x] Adaptive query compiler (JIT/LLVM)
- [x] macOS kperf/kpc PMU (dlopen), Windows rdtsc/QueryThreadCycleTime, ARM64 CNTVCT_EL0
- [x] WorkloadPredictor with adaptive LLM batch tuning
- [x] HardwareAccelerator GPU/FPGA/AVX-512 dispatch
- [x] LockFreeHistogram\<T\> — atomic-bucket P50/P90/P99 latency tracking

## Implementation Phases

### Phase 1 — Multi-Level Cache Core ✅
- [x] LIRS cache, WiscKey, Dostoevsky filter
- [x] HugePageManager, NumaAllocator

### Phase 2 — Lock-Free & RCU ✅
- [x] LockFreeMetricsBuffer SPSC/MPSC
- [x] RcuDomain + RcuHashTable

### Phase 3 — NUMA Topology & SIMD ✅
- [x] NumaTopology node discovery
- [x] AlignmentHelpers AVX-512 / cache-line

### Phase 4 — ML Workload & PMU Counters ✅
- [x] WorkloadPredictor Isolation Forest / LSTM
- [x] CycleMetrics macOS kperf/kpc + Windows rdtsc
- [x] AdaptiveQueryCompiler JIT/LLVM
- [x] LockFreeHistogram\<T\> — exponential + linear modes, P50/P90/P95/P99, ≤ 20 ns `record()` (2026-04-12)
  - `include/performance/lockfree_histogram.h` — header-only, cache-line-aligned buckets
  - `LatencyHistogram` (32 exp buckets) + `WideHistogram` (64 exp buckets) aliases
  - 12 tests (LFH-01…LFH-12) in `tests/test_lockfree_histogram.cpp`

### Phase 5 — DPDK / io_uring / Persistent Memory (Planned)
- [ ] DPDK kernel-bypass data plane integration (Target: Q3 2026)
- [ ] io_uring zero-copy path for storage (Target: Q3 2026)
- [ ] Intel Optane persistent memory allocator (Target: Q4 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All headers documented in ARCHITECTURE.md
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] CycleMetrics validated on macOS Intel, macOS ARM64, Linux x86, Linux ARM64, Windows x64
- [x] NUMA allocator tested on 2-socket EPYC server
- [x] JIT compiler fuzz-tested against malformed plan inputs
- [ ] DPDK data plane validated on bare-metal NIC (Target: Q3 2026)
