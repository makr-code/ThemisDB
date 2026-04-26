# Performance Module — Architecture Guide

> **Status:** 2026-04-19 – Architekturtext gegen realen Sourcecode verifizieren; Abweichungen mit `<!-- TODO -->` markiert.

<!-- status: current | validated: 2026-04-06 -->
<!-- Links: Primary README → src/performance/README.md | Secondary → docs/de/performance/README.md -->

**Version:** 1.1
**Last Updated:** 2026-04-06
**Module Path:** `src/performance/`

---

## 1. Overview

The Performance module provides ThemisDB's advanced optimization infrastructure:
hardware-level cycle metrics, research-based algorithmic optimizations (DiskANN/WiscKey/
LIRS/Cicada/Dostoevsky), NUMA-aware memory allocation, SIMD acceleration, lock-free data
structures, phase-based feature flags, and benchmark/profiling tools.

It is organized into implementation phases (Phase 2, 3, 4) reflecting different maturity
levels, with each phase controlled by compile-time and runtime feature flags to ensure
zero overhead when a feature is disabled.

---

## 2. Design Principles

- **Research-Driven** – every optimization references a peer-reviewed paper; no ad-hoc
  tuning without theoretical grounding.
- **Zero-Cost Abstractions** – compile-time `THEMIS_PERF_PHASE_N` flags eliminate all
  overhead when a feature is disabled.
- **Hardware-Aware** – RDTSC/RDTSCP (x86-64), CNTVCT_EL0 (ARM64), CUDA events (GPU)
  are used for cycle-accurate measurement.
- **Adaptive** – runtime feature toggles allow enabling/disabling optimizations without
  restart.
- **Observable** – all cycle metrics are exportable to Prometheus, JSON, or Chimera format.

---

## 3. Component Architecture

### 3.1 Key Components

| File / Dir | Role |
|---|---|
| `cycle_metrics.cpp` | Hardware cycle counter measurement (x86/ARM64/GPU) |
| `phase2_feature_flags.cpp` | Phase 2 runtime feature toggles |
| `phase3/` | Phase 3 optimizations (LIRS, prefetch, alignment) |
| `phase4/` | Phase 4 optimizations (WiscKey, DiskANN, Cicada, RabitQ) |
| `wisckey.cpp` | WiscKey key-value separation for large values |
| `workload_adaptive_optimizer.cpp` | `WorkloadAdaptiveOptimizer` — OLTP/OLAP/MIXED/GRAPH/VECTOR/TIMESERIES classification, dynamic strategy selection, predictive scaling |
| `advanced_cache_manager.cpp` | `AdvancedCacheManager` — multi-partition cache with Bloom filter, adaptive eviction (LRU/LIRS/ARC/2Q), value compression |
| `numa_memory_manager.cpp` | `NUMAMemoryManager` — sysfs topology detection, affinity-based allocation, per-node statistics |
| `hardware_accelerator.cpp` | `HardwareAccelerator` — GPU/FPGA/SIMD/SmartNIC/PMem accelerator dispatch |
| `intelligent_prefetcher.cpp` | `IntelligentPrefetcher` — ML-driven cache prefetcher for query access patterns |
| `lockfree_histogram.h` | `LockFreeHistogram<T>`, `LatencyHistogram` (32-bucket), `WideHistogram` (64-bucket) — header-only atomic P99 tracking |
| `lirs_cache.h` | `LirsCache<K,V>` — LIRS cache replacement with `shared_mutex` (header-only) |
| `dostoevsky.cpp` | Dostoevsky LSM-tree compaction policy |
| `cicada.cpp` | Cicada optimistic concurrency control |
| `rabitq.cpp` | RaBitQ binary quantization for vector search |
| `ligra.cpp` | Ligra parallel graph processing framework |
| `numa_topology.cpp` | NUMA-aware memory allocation and thread pinning |
| `async_metrics_exporter.cpp` | Async Prometheus metrics export |
| `prometheus_exporter.cpp` | Prometheus text format serializer |
| `chimera_exporter.cpp` | Chimera benchmark format exporter |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│               ThemisDB Core Systems                             │
│   Storage / Index / Query engines                               │
└──────────────────────────┬──────────────────────────────────────┘
                           │ call optimized paths
┌──────────────────────────▼──────────────────────────────────────┐
│                  Performance Module                              │
│                                                                  │
│  Phase 2: cycle_metrics + feature_flags + NUMA                  │
│  Phase 3: LIRS cache + cache prefetch + alignment               │
│  Phase 4: WiscKey + DiskANN + Cicada + RaBitQ + Dostoevsky      │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  CycleMetrics (RDTSC / CNTVCT_EL0 / CUDA events)         │  │
│  │  → ring buffer → statistical analysis → export           │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  Exporters: Prometheus | JSON | Chimera                         │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Cycle Measurement (RAII Timer)

```cpp
uint64_t cycles;
THEMIS_SCOPED_CYCLE_TIMER(cycles);
// ... timed operation ...
// on scope exit: cycles = RDTSC_end - RDTSC_start
// cycle_metrics.record(phase, cycles)
```

### 4.2 WiscKey Value Separation

```
Large value write (> threshold):
    ├─ Store key + value_ptr in LSM-tree (fast compaction)
    └─ Store value in separate value log (vLog)

Large value read:
    ├─ Lookup key in LSM → get value_ptr
    └─ Direct read of value from vLog (single I/O)
```

### 4.3 NUMA-Aware Allocation

```
Thread starts on NUMA node 1:
    │
    ├─ numa_topology.local_node() → node 1
    └─ allocate memory on node 1 (no cross-NUMA latency)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Used by** | `src/storage/` | WiscKey, Dostoevsky, cycle metrics |
| **Used by** | `src/index/` | RaBitQ quantization, DiskANN |
| **Used by** | `src/transaction/` | Cicada OCC |
| **Used by** | `src/graph/` | Ligra parallel graph processing |
| **Provides to** | `src/observability/` | Cycle metrics for Prometheus export |

---

## 6. Threading & Concurrency Model

- Cycle metrics use per-thread SPSC ring buffers (no cross-thread sharing).
- NUMA allocations are per-thread by design (each thread allocates on its local node).
- Phase feature flags are read-only after initialization (no locks on read path).
- Cicada OCC uses optimistic concurrency (no locks; validation on commit).

---

## 7. Performance Architecture

| Technique | Paper | Gain |
|---|---|---|
| WiscKey | WiscKey (FAST '16) | 1.6–14× read, 46–53× write for large values |
| Dostoevsky | Dostoevsky (SIGMOD '18) | Reduces write amplification by 1.5–2× |
| Cicada | Cicada (SIGMOD '17) | 1.5–3× transaction throughput |
| RaBitQ | RaBitQ (SIGMOD '24) | 1.5–3× ANN throughput, 32× compression |
| LIRS | LIRS (SIGMETRICS '02) | 2–5× better cache hit ratio vs LRU |
| NUMA-aware | NUMA locality | Reduces latency by 30–50% on multi-socket |

---

## 8. Security Considerations

- RDTSC is a side-channel timing oracle; avoid exposing raw cycle counts to untrusted
  clients. Aggregate and round before export.
- NUMA pinning is advisory; the kernel may migrate threads; do not rely on strict NUMA
  isolation for security.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `performance.phase2.enabled` | true | Enable Phase 2 optimizations |
| `performance.phase3.enabled` | false | Enable Phase 3 optimizations |
| `performance.phase4.enabled` | false | Enable Phase 4 optimizations |
| `performance.wisckey.threshold_bytes` | 4096 | Min value size for WiscKey separation |
| `performance.numa.enabled` | auto | Enable NUMA-aware allocation |
| `performance.cycle_metrics.export` | "prometheus" | Export format |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| RDTSC not supported | Fall back to `std::chrono::high_resolution_clock` |
| NUMA library unavailable | Disable NUMA; use default allocator |
| Feature flag conflict | Log warning; disable conflicting feature |

---

## 11. Known Limitations & Future Work

- DiskANN integration with the vector index is complete (`src/index/vector_index.cpp`).
- JIT compilation for query operators is planned.
- Phase 4 optimizations are experimental; production deployment requires extensive testing.
- Ligra parallel graph processing is self-contained (uses `std::thread`); no external Ligra library required.

---

## 12. References

- `src/performance/README.md` — module overview
- `src/performance/FUTURE_ENHANCEMENTS.md` — roadmap
- `docs/PERFORMANCE_PHASE_2_COMPLETE.md` — Phase 2 completion summary
- `docs/PERFORMANCE_PHASE_3_COMPLETE.md` — Phase 3 completion summary
- `ARCHITECTURE.md` (root) — full system architecture
