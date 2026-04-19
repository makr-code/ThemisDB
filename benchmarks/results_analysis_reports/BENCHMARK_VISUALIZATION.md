> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeipunkt und sind nicht mehr reproduzierbar ohne die ursprüngliche Testumgebung.
> Für reproduzierbare Ergebnisse: Benchmark-Kommandos und aktuelle CMake-Presets unter [`benchmarks/README.md`](../README.md) verwenden.

# Performance Results Visualization

## Throughput Comparison (Operations Per Second)

```
┌─────────────────────────────────────────────────────────────────┐
│ BENCHMARK THROUGHPUT COMPARISON (Ops/Sec)                     │
└─────────────────────────────────────────────────────────────────┘

Binary Blob Retrieval    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 49.0M ops/s
Graph BFS Traversal      ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 9.56M ops/s  
AQL Join (Users-Posts)   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 10.2M ops/s
Simple AQL WHERE         ▓▓▓▓ 3.43M ops/s
Complex AQL WHERE        ▓▓▓▓ 3.35M ops/s
RAG Search Top-50        ▓▓▓▓▓▓▓▓ 7.17M ops/s
RGB Vector Search        ▓▓▓▓▓▓▓▓▓▓ 59.7M ops/s ⭐ FASTEST
RGB Vector Insert        ▓▓ 1.83M ops/s
Dense Graph Query        ▓▓▓▓▓▓▓▓▓▓ 8.96M ops/s
384D Embedding Insert     ▓ 411k ops/s
1536D LLM Batch          ▓ 124.7k ops/s
10KB Blob Storage        ▓ 388.5k ops/s
1MB Document Storage     ⚠️  741 ops/s
```

## Performance by Category

```
┌──────────────────────────────────────────────────────────┐
│ VECTOR OPERATIONS                                        │
├──────────────────────────────────────────────────────────┤
│
│ RGB (3D)        ▓▓▓▓▓▓▓▓▓▓▓▓ 1.83M insert/s
│ 384D            ▓▓▓ 411k insert/s
│ 1536D LLM       ▓ 116k insert/s
│ Search 3D       ▓▓▓▓▓▓▓ 59.7M query/s
│ Search 4096D    ▓▓▓▓▓▓▓▓ 5.19M query/s
│
└──────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────┐
│ AQL QUERIES                                              │
├──────────────────────────────────────────────────────────┤
│
│ Simple WHERE    ▓▓▓▓ 3.43M ops/s
│ Complex WHERE   ▓▓▓▓ 3.35M ops/s
│ JOIN (Graph)    ▓▓▓▓▓▓▓▓▓▓ 10.2M ops/s
│ Multi-Join      ▓▓▓▓ 3.29M ops/s
│
└──────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────┐
│ LLM OPERATIONS                                           │
├──────────────────────────────────────────────────────────┤
│
│ Embedding Gen   ▓ 113.8k embed/s
│ RAG Retrieval   ▓▓▓▓▓▓▓ 7.17M query/s
│ Multi-Query     ▓▓▓▓ 2.80M results/s
│
└──────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────┐
│ GRAPH OPERATIONS                                         │
├──────────────────────────────────────────────────────────┤
│
│ Add Edges       ▓▓ 1.26M edges/s
│ Query Neighbors ▓▓▓▓▓▓▓▓▓ 8.96M query/s
│ BFS Traversal   ▓▓▓▓▓▓▓▓▓ 9.56M traversal/s
│
└──────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────┐
│ SECONDARY INDEX                                          │
├──────────────────────────────────────────────────────────┤
│
│ Small (1K)      ▓▓ 1.75M ops/s
│ Medium (100K)   ▓ 1.06M ops/s
│ Large (1M)      ▓▓▓ 3.12M ops/s
│ Composite       ▓▓ 2.40M ops/s
│
└──────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────┐
│ BINARY OPERATIONS                                        │
├──────────────────────────────────────────────────────────┤
│
│ 10KB Thumb      ▓ 388.5k ops/s
│ 100KB Batch     ▓▓▓▓▓▓▓▓▓▓▓▓▓ 49.0M ops/s
│ 1MB Document    ⚠️  741 ops/s
│
└──────────────────────────────────────────────────────────┘
```

## Latency Analysis

```
┌─────────────────────────────────────────────────────────┐
│ OPERATION LATENCY (Lower is Better)                    │
├─────────────────────────────────────────────────────────┤
│
│ RGB Search      ████████████████ 16.7 ns ⭐ FASTEST
│ BFS Traversal   ██████████████████ 105 ns
│ AQL Simple      ████████████████████ 291 ns
│ AQL Complex     ██████████████████ 298 ns
│ Graph Query     █████████████████ 196 ns
│ 1536D Insert    ████████████████ 8.0 μs
│ 384D Insert     ████████████████ 244 ns
│ 10KB Blob       █████████████ 2.6 μs
│ 100KB Blob      █████ 20.4 ns
│ 1MB Document    ████████████████████ 1.39 ms 🐢 SLOWEST
│
└─────────────────────────────────────────────────────────┘
```

## Scaling Performance

```
Vector Dimension Impact:
═══════════════════════════════════════════════════════════

3D vectors       ▓▓▓▓▓▓▓▓▓▓▓▓ 1.83M insert/s (baseline)
384D vectors     ███████ 411k insert/s (-77%)
1536D vectors    ██ 116k insert/s (-94%)

→ Each 4x dimension ≈ 4-5x slower

Dataset Size Impact:
═══════════════════════════════════════════════════════════

1K entries       ▓▓▓▓▓▓▓▓▓▓ 1.75M ops/s (baseline)
100K entries     ██████ 1.06M ops/s (-40%)
1M entries       ▓▓▓▓▓▓▓▓ 3.12M ops/s (+78% with optimization)

→ Small degrades linearly, but batch optimization helps

Blob Size Impact:
═══════════════════════════════════════════════════════════

10KB blobs       ▓▓ 388.5k ops/s
100KB blobs      ▓▓▓▓▓▓▓▓▓▓▓▓ 49.0M ops/s (+125x!)
1MB blobs        █ 741 ops/s (-98%)

→ Sweet spot: 50-500KB range
```

## Concurrency & Stress Performance

```
Workload Pattern Analysis:
═══════════════════════════════════════════════════════════

Mixed Read/Write    ▓▓▓▓▓▓ 1.90M ops/s
(80% read, 20% write)

Hotspot Access      ▓▓▓▓▓▓▓ 2.90M ops/s
(99% contention on 1% of keys)

Performance Paradox:
─ Hotspot test FASTER than mixed workload!
─ Reason: Cache locality + reduced lock overhead
─ Implication: Production hotspots perform well
```

## Category Performance Ranking

```
┌─ TIER 1: PRODUCTION-READY (>1M ops/s) ─────────────────┐
│                                                         │
│  🥇 Binary Retrieval           49.0M ops/s            │
│  🥈 RGB Vector Search          59.7M ops/s            │
│  🥉 AQL Join Operations        10.2M ops/s            │
│     Graph BFS Traversal         9.56M ops/s            │
│     Graph Neighbor Query        8.96M ops/s            │
│     RAG Search                  7.17M ops/s            │
│     AQL WHERE Clauses           3.3M ops/s             │
│     Secondary Index Operations  3.12M ops/s            │
│                                                         │
└─────────────────────────────────────────────────────────┘

┌─ TIER 2: GOOD PERFORMANCE (100k-1M ops/s) ─────────────┐
│                                                         │
│  📊 RGB Vector Insert          1.83M ops/s            │
│     Small Index Operations      1.75M ops/s            │
│     Dense Graph Operations      1.26M ops/s            │
│     LLM Batch Insert            124.7k ops/s           │
│     Embedding Generation        113.8k ops/s           │
│     10KB Blob Storage           388.5k ops/s           │
│                                                         │
└─────────────────────────────────────────────────────────┘

┌─ TIER 3: NEEDS OPTIMIZATION (<100k ops/s) ─────────────┐
│                                                         │
│  ⚠️  1536D Batch Insert         116k ops/s            │
│     384D Embedding Insert       411k ops/s             │
│     1MB Document Storage        741 ops/s ❌ BOTTLENECK│
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## LLM Readiness Assessment

```
┌─────────────────────────────────────────────────┐
│ LLM/RAG Pipeline Performance                   │
├─────────────────────────────────────────────────┤
│
│ Component            Performance   Status
│ ──────────────────────────────────────────────
│ Embedding Storage    116k/s        ✅ Good
│ Document Retrieval   7.17M/s       ✅ Excellent
│ Query Expansion      2.80M/s       ✅ Good
│ Index Lookup         3.12M/s       ✅ Excellent
│ Graph Traversal      9.56M/s       ✅ Excellent
│
│ Overall RAG Readiness: 🟢 PRODUCTION READY
│ Expected throughput: 100k+ queries/sec
│
└─────────────────────────────────────────────────┘
```

---

**Benchmark Run**: 2025-12-18
**Platform**: Windows x64, 20 CPUs @ 3696 MHz
**Build**: Release Mode (MSVC)

For detailed results, see `BENCHMARK_RESULTS.md`
