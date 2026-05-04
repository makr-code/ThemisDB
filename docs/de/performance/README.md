# Performance Documentation

<!-- status: current | validated: 2026-04-06 -->
<!-- Links: Primary → ../../../../src/performance/README.md | Architektur → ../../../../src/performance/ARCHITECTURE.md | Roadmap → ../../../../src/performance/ROADMAP.md -->

**Stand:** 6. April 2026  
**Version:** v1.4.0  
**Kategorie:** ⚡ Performance

---

## 📑 Inhaltsverzeichnis

- [Primäre Dokumentation](#primäre-dokumentation)
- [Übersicht](#übersicht)
- [Benchmark-Ergebnisse](#benchmark-ergebnisse-v100)
- [Hardware-Referenz](#hardware-referenz)
- [Dokumentation](#dokumentation-in-diesem-ordner)

## Primäre Dokumentation

> Die maßgebliche Implementierungsdokumentation liegt in `src/performance/` und `include/performance/`.

| Datei (Primary) | Inhalt |
|-----------------|--------|
| [`src/performance/README.md`](../../../../src/performance/README.md) | Modulübersicht, APIs, Build-Flags, Komponenten |
| [`src/performance/ARCHITECTURE.md`](../../../../src/performance/ARCHITECTURE.md) | Architektur, Komponenten-Diagramm, Datenfluss |
| [`src/performance/ROADMAP.md`](../../../../src/performance/ROADMAP.md) | Implementierungsphasen, verifizierte [x]-Items |
| [`src/performance/FUTURE_ENHANCEMENTS.md`](../../../../src/performance/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen + Forschungsquellen |
| [`include/performance/README.md`](../../../../include/performance/README.md) | Public-Header-Übersicht |

## Übersicht

Performance-Benchmarks und Optimierungs-Strategien für ThemisDB.

## Benchmark-Ergebnisse (v1.0.0)

| Operation | Throughput | Latency (p99) |
|-----------|------------|---------------|
| Write | 45,000 ops/s | 5ms |
| Read | 120,000 ops/s | 2ms |
| Vector Search (k=10) | 8,000 qps | 15ms |
| Graph Traversal (3 hops) | 5,000 qps | 25ms |

## Hardware-Referenz

- **CPU:** AMD EPYC 7543 (32 cores)
- **RAM:** 128 GB DDR4
- **Storage:** NVMe SSD (3,500 MB/s)
- **Network:** 10 Gbit/s

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [PERFORMANCE_INDEX.md](PERFORMANCE_INDEX.md) | 📊 **Hauptindex** - Vollständige Übersicht |
| [performance_benchmarks.md](performance_benchmarks.md) | Benchmark-Ergebnisse |
| [performance_compression_benchmarks.md](performance_compression_benchmarks.md) | Kompression Benchmarks |
| [performance_compression_strategy.md](performance_compression_strategy.md) | Kompression Strategie |
| [performance_cuda.md](performance_cuda.md) | CUDA/GPU Acceleration |
| [performance_enterprise_strategy.md](performance_enterprise_strategy.md) | Enterprise Tuning |
| [performance_gpu_plan.md](performance_gpu_plan.md) | GPU Roadmap |
| [performance_hardware.md](performance_hardware.md) | Hardware Requirements |
| [performance_memory.md](performance_memory.md) | Memory Optimization |
| [performance_multi_cpu.md](performance_multi_cpu.md) | Multi-CPU Scaling |
| [performance_tbb.md](performance_tbb.md) | Intel TBB Integration |

## 🔬 Wissenschaftliche Research

| Datei | Beschreibung |
|-------|--------------|
| [🔬 **Wissenschaftliche Performance-Optimierungen**](../research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md) | **NEW** - Research-basierte Optimierungs-Strategien aus 25+ Papers (SIGMOD, VLDB, OSDI, NeurIPS) mit konkreten Implementierungs-Empfehlungen |

**Highlights:**
- ⚡ **Phase 1 (Quick Wins):** +50-100% bei Read-Heavy Workloads (1-3 Monate)
- 🚀 **Phase 2 (Medium-Term):** +100-200% Overall Performance (3-6 Monate)
- 🎯 **Phase 3 (Long-Term):** +200-500% Domain-Specific (6-12 Monate)

**Bereiche:** LSM-Trees, Vector Search, Graph Processing, GPU Acceleration, MVCC, Query Optimization, Memory Management, Concurrency Control, Compression, Caching

## Neue Performance-Komponenten (v1.9.x)

| Komponente | Header | Beschreibung |
|------------|--------|-------------|
| `LockFreeHistogram<T>` | `include/performance/lockfree_histogram.h` | Lock-free Latenz-Histogramm, ≤ 20 ns `record()`, Exponential/Linear Modi |
| `RequestCoalescer` | `include/cache/request_coalescer.h` | Singleflight: thundering-herd-Schutz, `fn()` exakt einmal pro In-Flight-Key |
| `IoUringBatchedSender` | `include/network/io_uring_batcher.h` | O(1) Syscalls/Runde statt O(N) writev-Aufrufe, writev-Fallback |
| LIRS `shared_mutex` Fix | `include/performance/lirs_cache.h` | TOCTOU-Race in `get()` behoben: `unique_lock` statt `shared_lock` |
| RCU `g_rcu_reader_count` | `include/performance/rcu.h` | `readers_active()` war immer false — globaler atomic Zähler korrigiert |

Vollständige Dokumentation: Compendium Kapitel 21.1.

## Verwandte Dokumentation

- [🔬 Research Documentation](../research/README.md) - Wissenschaftliche Erkenntnisse
- [Enterprise Features](../enterprise/README.md) - Enterprise Scalability
- [Storage Module](../storage/README.md) - Storage Performance

## 🔍 Reality-Check Report

| Datei | Beschreibung |
|-------|--------------|
| [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md) | Fehlende Implementierungen — Reality-Check-Ergebnis (Issue #3525) |
| [missing-implementations.json](missing-implementations.json) | Machine-readable Findings |
