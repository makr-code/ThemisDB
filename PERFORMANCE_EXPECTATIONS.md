# ThemisDB ÔÇô Performance-Erwartungswerte & Messergebnisse

> Stand: 2026-04-02 | Quellen: `FUTURE_ENHANCEMENTS.md` je Modul, `benchmarks/results_analysis_reports/`, `benchmarks/baselines/`, `benchmarks/VERSION_HISTORY.csv`, `benchmarks/chimera/`
>
> **Benchmark-Plattformen:**
> - Run **20251223** (v1.3.0-baseline): MSVC Release x64, AVX2, 20-Core @ 3.7 GHz, 20 MB L3
> - Run **20251223_085556** (v1.3.3-dev): MSVC Release x64, AVX2, 20-Core @ 3.7 GHz, 20 MB L3
> - Run **20251229_184507** (v1.3.4): Windows x64, 20 Cores @ 3.696 GHz, 20 MB L3-Cache, L1=32KB, L2=256KB

---

## Legende

| Symbol | Bedeutung |
|--------|-----------|
| Ô£à | Ziel erf├╝llt (gemessen ÔëÑ Ziel) |
| ÔØî | Ziel nicht erf├╝llt (gemessen < Ziel) |
| ÔÜá´©Å | Partiell / bekannte Regression |
| ÔØô | Kein Messwert vorhanden |
| ÔÇô  | Nicht gemessen in dieser Version |

---

---

## Inhaltsverzeichnis

> **Struktur: Allgemein ÔåÆ Spezifisch (Module) ÔåÆ Rohdaten ÔåÆ Interface-SLOs**

| # | Abschnitt | Typ |
|---|-----------|-----|
| ÔÇô | Legende | Referenz |
| 1 | Versionshistorie ÔÇô Kernmetriken | Messung (allgemein) |
| 2ÔÇô29 | Modul-Spezifische Erwartungswerte | Modul-SLOs |
| 30 | Chimera-Baseline & Suite | Benchmark-Framework |
| 31ÔÇô32 | Prompt Engineering / Ethics AI | Modul-SLOs |
| 33 | System-Level TPC/YCSB | Benchmarks |
| 34 | CI Regression-Schwellwerte | CI |
| 35 | Bekannte Performance-L├╝cken | L├╝cken |
| 36 | Rohdaten: Google Benchmark C++ | Prim├ñre Messungen |
| 37 | Performance-Ma├ƒnahmen (GitHub-PR) | Ma├ƒnahmen nach Modul |
| 38 | Rohdaten: HTTP-API & Docker Benchmarks | Prim├ñre Messungen |
| 39 | API/Interface Performance-Annahmen | Interface SLOs |

**Hinweis zur Statusbewertung:** Felder mit ÔØô sind Erwartungswerte ohne vorliegende Messung.
Typ-Kennung in ┬º39: **[M]** = gemessen ┬À **[Z]** = Ziel ┬À **[I]** = implementiert/best├ñtigt.

## 1. Versionshistorie ÔÇô Kernmetriken

> Quelle: `benchmarks/VERSION_HISTORY.csv` + `benchmarks/results_analysis_reports/benchmark_summary.csv`
> Testplattform v1.3.0ÔÇôv1.3.3: Intel i9-10900K (10C/20T @ 3.70 GHz), 31 GB RAM, WSL2 Linux
> Testplattform v1.3.4: Windows x64, 20 Cores @ 3.696 GHz, 20 MB L3-Cache

| Metrik | Ziel | v1.3.0 | v1.3.1 | v1.3.2 | v1.3.3 | **v1.3.4** | **v1.8.0 Ziel** | ╬ö v1.3.0ÔåÆv1.3.4 | Status |
|--------|------|--------|--------|--------|--------|-----------|-----------------|-----------------|--------|
| Query Engine Throughput | ÔÇô | 700 M ops/s | 750 M ops/s | 800 M ops/s | 800 M ops/s | **814,5 M ops/s** | **ÔëÑ 900 M ops/s** | +16 % | ÔØô |
| Vector Insert | ÔÇô | 280 k/s | 300 k/s | 330 k/s | 340 k/s | **351,4 k/s** | **ÔëÑ 600 k/s** | +25 % | ÔØô |
| Secondary Index Insert | ÔÇô | 180 k/s | 190 k/s | 210 k/s | 215 k/s | **217,2 k/s** | **ÔëÑ 1 M/s** | +21 % | ÔØô |
| Embedding Cache Hit-Rate | ÔÇô | ÔÇô | ÔÇô | ÔÇô | ÔÇô | **155,8 M/s** | **ÔëÑ 200 M/s** | n/a | ÔØô |
| 2PC Throughput | ÔÇô | ÔÇô | ÔÇô | ÔÇô | ÔÇô | **6,4 k/s** | **ÔëÑ 10 k/s** | n/a | ÔØô |
| Graph Edge Ops | ÔÇô | ÔÇô | ÔÇô | ÔÇô | ÔÇô | **628,7 k/s** | **ÔëÑ 1 M/s** | n/a | ÔØô |
| Timeseries Insert | ÔÇô | ÔÇô | ÔÇô | ÔÇô | ÔÇô | **49,0 M pts/s** | **ÔëÑ 60 M pts/s** | n/a | ÔØô |
| Gesamt Benchmark-Tests | ÔÇô | 450 | 480 | 520 | 780 | **1.078** | **ÔëÑ 1.200** | +140 % | Ô£à |

---

## 2. Query-Engine ÔÇô Detailergebnisse

> Quelle: `BENCHMARK_RESULTS.md` (Run 2025-12-18), `benchmark_summary.csv` (Run 2025-12-29)

| Benchmark | Ziel | v1.3.| Simple AQL WHERE | ÔëÑ 10.000 Queries/s bei P99 < 20 ms | 3,43 M ops/s @ ~0,3 ┬Ás | Ô£à |
| Complex WHERE | ÔëÑ 1 M ops/s | 3,35 M ops/s | Ô£à |
| JOIN (Users-Posts) | ÔëÑ 5 M ops/s | 10,2 M ops/s | Ô£à |
| QueryEngineBench/SimpleEvaluation | ÔëÑ 750 M items/s | 814,5 M items/s (1,23 ns) | Ô£à |
| Parse + Optimize P99 (Ôëñ10 Collections) | Ôëñ 5 ms | ÔØô | ÔØô |
| Query-Cache Lookup P99 (Exact) | < 1 ms | ÔØô | ÔØô |
| Query-Cache Lookup P99 (Semantic) | Ôëñ 10 ms | ÔØô | ÔØô |
| JIT Erstcompilierung | Ôëñ 50 ms | ÔØô | ÔØô |
| Federation Plan-Overhead (5 Cluster) | Ôëñ 20 ms | ÔØô | ÔØô |
| Streaming First-Chunk Latenz | Ôëñ 50 ms | ÔØô | ÔØô |

---

## 3. Index-Modul

> Quelle: `benchmark_summary.csv` (Run 2025-12-29), `baselines/acceleration/baseline.json` (v1.0.0)

| Benchmark | Ziel | v1.0.0 Gemessen | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|-----------------|--------|
| VectorIndexBench/InsertPlaintext | ÔëÑ 280 k/s | ÔÇô | 351,4 k/s (2,84 ┬Ás) | Ô£à |
| SecondaryIndexBench/IndexInsert | ÔëÑ 180 k/s | ÔÇô | 217,2 k/s (4,60 ┬Ás) | Ô£à |
| SecondaryIndexBench/RawWriteOnly | ÔëÑ 500 k/s | ÔÇô | 885,0 k/s (1,13 ┬Ás) | Ô£à |
| Small Index Insert (1K entities) | ÔëÑ 1 M/s | ÔÇô | 1,75 M/s | Ô£à |
| Medium Index Insert (100K) | ÔëÑ 500 k/s | ÔÇô | 1,06 M/s | Ô£à |
| Large Index Lookup (1M) | ÔëÑ 1 M/s | ÔÇô | 3,12 M/s | Ô£à |
| Composite Index Lookup | ÔëÑ 1 M/s | ÔÇô | 2,40 M/s | Ô£à |
| L2Distance/1000/512 | ÔëÑ 250 k/s | 313 k/s (3.200 ns) | ÔØô | Ô£à |
| CosineDistance/1000/512 | ÔëÑ 200 k/s | 250 k/s (4.000 ns) | ÔØô | Ô£à |
| TopK/5000/50 | ÔëÑ 10 M/s | 12,5 M/s (400 ns) | ÔØô | Ô£à |
| HNSW Vektor-Suche (CPU) | ÔëÑ 5.000 QPS | ÔØô | ÔØô | ÔØô |
| HNSW Vektor-Suche (GPU RTX-class) | ÔëÑ 50.000 QPS | ÔØô | ÔØô | ÔØô |
| B-Tree Point-Lookup P99 (10M Keys) | < 500 ┬Ás | ÔØô | ÔØô | ÔØô |
| R-Tree Spatial Range Query P99 | < 10 ms | ÔØô | ÔØô | ÔØô |
| GPU Index-Build (1M ├ù 128-dim) | < 60 s | ÔØô | ÔØô | ÔØô |
| RocksDB WriteBatch Commit P99 | < 2 ms | ÔØô | ÔØô | ÔØô |

---

## 4. Cache-Modul

> Quelle: `FUTURE_ENHANCEMENTS.md`; kein direkter Bench-Run gefunden

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| C-1 L1 Hit-Path | ÔëÑ 5 M ops/s/Core (16-Thread) | ÔØô | ÔØô |
| C-2 L2 Hit-Path | ÔëÑ 500 k ops/s | ÔØô | ÔØô |
| C-3 L3 Hit-Path P99 | Ôëñ 5 ms | ÔØô | ÔØô |
| C-4 Warmup Throughput | ÔëÑ 500 k Entries/s | ÔØô | ÔØô |
| C-5 Admin-API Response | Ôëñ 5 ms | ÔØô | ÔØô |
| C-6 Prefetch Latenz | Ôëñ 100 ┬Ás/Call | ÔØô | ÔØô |
| C-7 Prefetch Overfetch | Ôëñ 10 % | ÔØô | ÔØô |

---

## 5. Storage-Modul

> Quelle: `scientific_benchmarks_20251204_212220/summary.csv` (v1.0.0, HTTP-API-Level)

| Benchmark | Ziel | v1.0.0 Gemessen | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|-----------------|--------|
| INSERT 1 KB | ÔÇô | 759 ops/s @ 1,317 ms | ÔØô | ÔØô |
| READ 1 KB | ÔÇô | 834 ops/s @ 1,204 ms | ÔØô | ÔØô |
| UPDATE 1 KB | ÔÇô | 806 ops/s @ 1,240 ms | ÔØô | ÔØô |
| INSERT 10 KB | ÔÇô | 510 ops/s @ 1,959 ms | ÔØô | ÔØô |
| INSERT 100 KB | ÔÇô | 126 ops/s @ 7,913 ms | ÔØô | ÔØô |
| INSERT 1 MB | ÔÇô | 16 ops/s @ 61,402 ms | ÔØô | ÔØô |
| Concurrent 1 Client | ÔÇô | 776 ops/s @ 1,28 ms | ÔØô | ÔØô |
| Concurrent 5 Clients | ÔÇô | 721 ops/s @ 6,80 ms | ÔØô | ÔØô |
| Concurrent 50 Clients | ÔÇô | 948 ops/s @ 60,3 ms ÔÜá´©Å CV=38% | ÔØô | ÔØô |
| Sustained Write NVMe | ÔëÑ 100.000 ops/s | ÔÇô | ÔØô | ÔØô |
| Point-Read Latenz P99 | Ôëñ 1 ms (Bloom Filter) | ÔÇô | ÔØô | ÔØô |
| Incremental Backup | ÔëÑ 500 MB/s | ÔÇô | ÔØô | ÔØô |
| 1MB Blob Storage | ÔÇô | ÔÇô | 741 ops/s @ 1,39 ms ÔÜá´©Å | ÔÜá´©Å |
| 10KB Thumbnail Storage | ÔÇô | ÔÇô | 388,5 k blobs/s | ÔØô |
| 100KB Blob Retrieval | ÔÇô | ÔÇô | 49,0 M lookups/s | ÔØô |

---

## 6. Analytics-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| AN-1 Streaming Aggregation Memory | Ôëñ 512 MB/Fenster | ÔØô | ÔØô |
| AN-2 IVM Delta-Application | Ôëñ 50 ms (10k Rows) | ÔØô | ÔØô |
| AN-3 Parquet Export 1M Rows | Ôëñ 2 s | ÔØô | ÔØô |
| AN-4 CSV Export 1M Rows | Ôëñ 500 ms | ÔØô | ÔØô |
| AN-5 CEPEngine::stop() | Ôëñ 100 ms | ÔØô | ÔØô |
| AN-7 IsolationForest Training | Ôëñ 10 ms (1k-Punkt-Fenster) | ÔØô | ÔØô |
| AN-8 predictBatch() | Ôëñ 50 ms (1k Serien ├ù 30 Steps) | ÔØô | ÔØô |
| AN-9 Auto-Tune Grid | Ôëñ 5 ms (9 ╬▒, n=500, parallel) | ÔØô | ÔØô |
| AN-10 ARM NEON Aggregation | ÔëÑ 4 GB/s (Cortex-A78) | ÔØô | ÔØô |

---

## 7. Timeseries-Modul

> Quelle: `FUTURE_ENHANCEMENTS.md` (explizite Ist-Stand-Angaben)

| Ziel-ID | Erwartungswert | Bekannter Ist-Stand | v1.3.4 Gemessen | Status |
|---------|----------------|---------------------|-----------------|--------|
| TS-1 Write Throughput/Node | > 500 k pts/s | ~200 k pts/s | 49,0 M pts/s* | ÔÜá´©Å |
| TS-2 Gorilla Decode Throughput | > 2 GB/s/Core | ~400 MB/s | ÔØô | ÔÜá´©Å |
| TS-3 Range Scan P99 (1M pts) | < 50 ms | ÔÇô | ÔØô | ÔØô |
| TS-4 Continuous Aggregate Refresh | < 500 ms/1-min-Intervall | ÔÇô | ÔØô | ÔØô |
| TS-5 Write Amplification | < 1,5├ù | ÔÇô | ÔØô | ÔØô |
| TS-6 Downsampling Throughput | > 10 M pts/s ÔåÆ 1-min-Aggregate | ÔÇô | ÔØô | ÔØô |
| TS-7 Storage Reduction | > 50├ù (raw ÔåÆ 1-day Tier) | ÔÇô | ÔØô | ÔØô |
| TS-9 Buffer-to-Storage Flush P99 | < 10 ms | ÔÇô | ÔØô | ÔØô |
| TS-10 Gorilla Insert P99 | Ôëñ 50 ┬Ás | ÔÇô | ÔØô | ÔØô |
| TS-11 AES-256-GCM Throughput | > 1 GB/s/Core (AES-NI) | ÔÇô | ÔØô | ÔØô |

*`TimeseriesBench/InsertTimepoints` 49,0 M/s misst In-Memory-Append, nicht persistiertes Schreiben

---

## 8. Geo-Modul

> Quelle: `baselines/acceleration/baseline.json` (v1.0.0)

| Benchmark | Ziel | v1.0.0 Gemessen | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|-----------------|--------|
| Geo_HaversineDistance/100000 | ÔëÑ 20 M/s | 22,2 M/s (4.500 ns) | ÔØô | Ô£à |
| Geo_PointInPolygon/100000 | ÔëÑ 30 M/s | 35,7 M/s (2.800 ns) | ÔØô | Ô£à |
| intersects-Query P99 (1M Punkte) | Ôëñ 5 ms (R-Tree) | ÔÇô | ÔØô | ÔØô |
| R-Tree Bulk-Load (1M Geometrien) | Ôëñ 3 s | ÔÇô | ÔØô | ÔØô |
| Buffer 10K Punkte @ 500 m | Ôëñ 200 ms/Core | ÔÇô | ÔØô | ÔØô |
| Spatial JOIN (2├ù100K, 1 km) | Ôëñ 500 ms (erste 1k Ergebnisse) | ÔÇô | ÔØô | ÔØô |
| GeoJSON Parse (100K MultiPolygon) | Ôëñ 2 s | ÔÇô | ÔØô | ÔØô |
| GPU Contains (1M Punkte, A10G) | Ôëñ 50 ms | ÔÇô | ÔØô | ÔØô |
| DBSCAN GPU Speedup (100K Punkte) | > 100├ù vs. CPU | ÔÇô | ÔØô | ÔØô |

---

## 9. Graph-Modul

> Quelle: `BENCHMARK_RESULTS.md` (Run 2025-12-18)

| Benchmark | Ziel | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|--------|
| GraphIndexBench/AddEdges | ÔëÑ 500 k edges/s | 628,7 k edges/s (1,59 ┬Ás) | Ô£à |
| Sparse Graph Edge Addition | ÔëÑ 500 k edges/s | 1,26 M edges/s | Ô£à |
| Dense Graph Neighbor Query | ÔëÑ 5 M queries/s | 8,96 M queries/s | Ô£à |
| Graph BFS Traversal (Depth-3) | ÔëÑ 5 M traversals/s | 9,56 M traversals/s | Ô£à |
| RAG Search Top-50 | ÔëÑ 5 M ops/s | 7,17 M ops/s (140 ns) | Ô£à |
| Algorithmus-Selektion P99 (10M Nodes) | < 1 ms | ÔØô | ÔØô |
| Plan-Cache Lookup P99 | < 100 ┬Ás | ÔØô | ÔØô |
| Single-Refresh (10K Nodes) | Ôëñ 5 s / Ôëñ 200 ms (8 Worker) | ÔØô | ÔØô |
| Subgraph-Isomorphismus P95 | < 500 ms (100-Node-Pattern, 1M-Graph) | ÔØô | ÔØô |

---

## 10. Acceleration-Modul

> Quelle: `baselines/acceleration/baseline.json` (v1.0.0)

| Benchmark | Ziel | v1.0.0 Gemessen | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|-----------------|--------|
| L2Distance/1000/64 | ÔëÑ 1,5 M/s | 2,0 M/s (500 ns) | ÔØô | Ô£à |
| L2Distance/1000/512 | ÔëÑ 250 k/s | 313 k/s (3.200 ns) | ÔØô | Ô£à |
| CosineDistance/1000/512 | ÔëÑ 200 k/s | 250 k/s (4.000 ns) | ÔØô | Ô£à |
| InnerProduct/1000/512 | ÔëÑ 250 k/s | 313 k/s (3.200 ns) | ÔØô | Ô£à |
| TopK/1000/10 | ÔëÑ 15 M/s | 20,0 M/s (50 ns) | ÔØô | Ô£à |
| TopK/5000/50 | ÔëÑ 10 M/s | 12,5 M/s (400 ns) | ÔØô | Ô£à |
| Vec Search L2 CUDA (1M├ù128-dim) | < 8 ms auf RTX 3090 | ÔÇô | ÔØô | ÔØô |
| GPU Throughput | ÔëÑ 10├ù CPU AVX2 Baseline | ÔÇô | ÔØô | ÔØô |
| Large-Scale (100M├ù128, 4├ùA100 80 GB) | P99 < 15 ms k=100 | ÔÇô | ÔØô | ÔØô |
| INT8 Matmul vs. FP16 | ÔëÑ 2├ù auf RTX 3090 | ÔÇô | ÔØô | ÔØô |
| Vulkan (Apple M2, 500K├ù128) | < 20 ms | ÔÇô | ÔØô | ÔØô |

---

## 11. Replication-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| R-1 Replikations-Lag P99 (SEMI_SYNC) | Ôëñ 50 ms @ 10k Writes/s (LAN) | ÔØô | ÔØô |
| R-2 WAL-Shipping Throughput (Zstd L3) | ÔëÑ 500 MB/s/Follower (10 GbE) | ÔØô | ÔØô |
| R-3 Leader-Failover | Ôëñ 10 s | ÔØô | ÔØô |
| R-4 HLC Conflict Detection | < 5 ┬Ás/Write | ÔØô | ÔØô |
| R-5 CRDT Merge | Ôëñ 1 ┬Ás/Merge | ÔØô | ÔØô |
| R-6 WAL Replay (PITR, 100 GB) | ÔëÑ 200 MB/s; Ôëñ 10 min | ÔØô | ÔØô |
| R-7 CDC Event P99 | Ôëñ 1 ms (Commit ÔåÆ CDC Queue) | ÔØô | ÔØô |
| R-8 Cross-DC Lag ASYNC | Ôëñ 200 ms P99 (50 ms RTT WAN) | ÔØô | ÔØô |

---

## 12. Sharding-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| SH-1 Cross-Shard RPC P99 (LAN) | < 5 ms | ÔØô | ÔØô |
| SH-2 Connection-Pool Hit-Rate | > 95 % @ 10k RPS | ÔØô | ÔØô |
| SH-3 Percolator Commit P99 (10 Shards) | < 20 ms | ÔØô | ÔØô |
| SH-4 Shard-Split Migration Downtime | 0 ms Read-Unavailability | ÔØô | ÔØô |
| SH-5 Write-Latenz w├ñhrend Migration | < 20 % ├╝ber Baseline P99 | ÔØô | ÔØô |
| SH-6 Rebalancer Decision Cycle | < 10 s | ÔØô | ÔØô |
| SH-7 Anti-Entropy Scan Throughput | > 1 GB/s (NVMe, 8 Worker) | ÔØô | ÔØô |
| SH-8 GPU Reed-Solomon | > 4 GB/s (NVIDIA A10) | ÔØô | ÔØô |
| SH-9 Snapshot (1 GB Raft-State) | < 10 s | ÔØô | ÔØô |
| SH-10 Snapshot Kompressionsrate | < 35 % unkomprimiert (ZSTD L3) | ÔØô | ÔØô |
| SH-11 Replica Catch-up | > 200 MB/s (10 GbE LAN) | ÔØô | ÔØô |
| SH-12 Topology Change Propagation | < 500 ms (100 Nodes, Gossip) | ÔØô | ÔØô |

---

## 13. Transaction-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| TX-1 OCC Commit P50 | 100 ┬Ás | ÔØô | ÔØô |
| TX-2 OCC Commit P99 | 5 ms | ÔØô | ÔØô |
| TX-3 2PC Throughput | ÔëÑ 6 k/s | 6,4 k/s | Ô£à |
| TX-4 2PC Latenz (5 Shards) | 5 ms | ÔØô | ÔØô |
| TX-5 SAGA Compensation Time | 20 ms | ÔØô | ÔØô |
| TX-6 Deadlock Detection Overhead | 1 % (von 5 % verbessert) | ÔØô | ÔØô |
| TX-7 False Positive Rate | < 5 % | ÔØô | ÔØô |
| TX-8 Low-Contention Success Rate | > 90 % | ÔØô | ÔØô |

---

## 14. LLM-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| L-1 Time-to-First-Token (512-Token, A10G) | Ôëñ 200 ms P99 | ÔØô | ÔØô |
| L-2 Streaming Overhead | Ôëñ 2 % tokens/s Regression | ÔØô | ÔØô |
| L-3 LoRA Adapter Hot-Load (7B, Rank 64) | Ôëñ 5 s Wall-Clock | ÔØô | ÔØô |
| L-4 Adapter Serialisierung | Ôëñ 2 ms | ÔØô | ÔØô |
| L-5 Work-Stealing Dispatch P99 | Ôëñ 50 ┬Ás | ÔØô | ÔØô |
| L-6 Speculative Decoding Overhead | Ôëñ 15 % akzeptierter Token-Latenz | ÔØô | ÔØô |
| L-7 GPU Utilization (Mixed Workloads) | ÔëÑ 10 % Verbesserung | ÔØô | ÔØô |
| L-8 Speculative Decoding Throughput | ÔëÑ 2├ù tokens/s (7B + 0,5B Draft) | ÔØô | ÔØô |

---

## 15. RAG-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| RA-1 Fast Evaluation P99 | Ôëñ 100 ms E2E | ÔØô | ÔØô |
| RA-2 Balanced Evaluation P99 | Ôëñ 500 ms E2E | ÔØô | ÔØô |
| RA-3 Thorough Evaluation P99 | Ôëñ 2.000 ms E2E | ÔØô | ÔØô |
| RA-4 HybridRetriever Recall@10 | ÔëÑ 85 % (BEIR NQ) | ÔØô | ÔØô |
| RA-5 CrossEncoderReranker MRR@10 | ÔëÑ +10 % vs. BM25 | ÔØô | ÔØô |
| RA-6 StreamingRetriever First-Chunk | Ôëñ 50 ms | ÔØô | ÔØô |
| RA-7 Bayesian Optimizer Konvergenz | ÔëÑ 90 % opt. F1 in 200 Events | ÔØô | ÔØô |
| RA-8 ClaimExtractor (1k Zeichen) | Ôëñ 500 ms LLM / Ôëñ 50 ms Heuristic | ÔØô | ÔØô |

---

## 16. Search-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| SE-1 Hybrid Search P99 (10M-Doc-Index) | Ôëñ 20 ms (BM25 + HNSW RRF, Top-10) | ÔØô | ÔØô |
| SE-2 SPLADE Index Memory | Ôëñ 4 GB / 10M-Doc (CSR) | ÔØô | ÔØô |
| SE-3 Facet Counting (1k distinct, 100k Docs) | Ôëñ 5 ms | ÔØô | ÔØô |
| SE-4 LTR Re-Ranking (Top-100) | Ôëñ 2 ms | ÔØô | ÔØô |
| SE-5 Autocomplete P99 (1M-Term-Dict) | Ôëñ 5 ms | ÔØô | ÔØô |
| SE-6 LLM Query Rewriter Timeout | 200 ms + Fallback | ÔØô | ÔØô |

---

## 17. Temporal-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| TM-1 History-Table Write Overhead | < 15 % vs. Baseline | ÔØô | ÔØô |
| TM-2 Time-Travel Query | 80ÔÇô95 % Current-Table-Speed | ÔØô | ÔØô |
| TM-3 AS OF Query | 80ÔÇô95 % Current-Table-Speed | ÔØô | ÔØô |
| TM-4 Retention Enforcement/Batch | Ôëñ 100 ms | ÔØô | ÔØô |
| TM-5 Conflict Resolution | < 10 ms | ÔØô | ÔØô |
| TM-6 Temporal Join Overhead | Ôëñ 50 % vs. Non-Temporal | ÔØô | ÔØô |

---

## 18. API-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| API-1 GraphQL Parse+Execute P99 | < 2 ms (10-Feld-Query, 500 HTTP/2) | ÔØô | ÔØô |
| API-2 WebSocket Subscription Latenz | < 50 ms (Changefeed ÔåÆ Frame) | ÔØô | ÔØô |
| API-3 Concurrent WebSocket Connections | ÔëÑ 10k / Node bei < 50 MB RSS | ÔØô | ÔØô |
| API-4 Bulk Insert (10k Docs) | < 500 ms E2E | ÔØô | ÔØô |
| API-5 Middleware Overhead | < 10 ┬Ás/Request | ÔØô | ÔØô |
| API-6 Span Enqueue (Hot Path) | < 500 ns/Call | ÔØô | ÔØô |
| API-7 OTLP Flush (64 Spans) | < 5 ms E2E | ÔØô | ÔØô |

---

## 19. Auth-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| AUT-1 LDAP Bind P99 | Ôëñ 50 ms sichtbar (< 200 ms Backend) | ÔØô | ÔØô |
| AUT-2 LDAP Auth (unter Load) | < 5 ms avg (von ~30 ms via Conn-Reuse) | ÔØô | ÔØô |
| AUT-3 JWT JWKS Refresh Blocking | Ôëñ 1 ms auf Validation Hot Path | ÔØô | ÔØô |
| AUT-4 Token Revocation Lookup | Ôëñ 1 ┬Ás (Bloom Filter, warm) | ÔØô | ÔØô |
| AUT-5 Redis Token Revocation P99 | Ôëñ 2 ms auf LAN | ÔØô | ÔØô |

---

## 20. CDC-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| CDC-1 Concurrent WebSocket Connections | ÔëÑ 5k / Node bei < 100 MB RSS | ÔØô | ÔØô |
| CDC-2 Event Delivery P99 | < 20 ms (Emit ÔåÆ Frame) | ÔØô | ÔØô |
| CDC-3 Consumer Group Offset Commit | < 1 ms P99 (RocksDB) | ÔØô | ÔØô |
| CDC-4 Resume nach 24h Offline (10M Events) | < 5 s bis zur Delivery | ÔØô | ÔØô |
| CDC-5 End-to-End Latenz (ÔåÆ Kafka Ack) | < 10 ms P99 (LAN) | ÔØô | ÔØô |
| CDC-6 Log Compaction (1M Events) | < 30 s (Background) | ÔØô | ÔØô |

---

## 21. Network-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| NET-1 TCP Wire Protocol Throughput | ÔëÑ 100k req/s/Core (128B, kein TLS) | ÔØô | ÔØô |
| NET-2 TLS 1.3 Handshake P99 | < 5 ms (neue Verbindungen) | ÔØô | ÔØô |
| NET-3 TLS 1.3 Session Resumption P99 | < 1 ms | ÔØô | ÔØô |
| NET-4 WebSocket Round-Trip P99 | < 2 ms (localhost) | ÔØô | ÔØô |
| NET-5 QUIC 0-RTT Resumption P99 | < 2 ms | ÔØô | ÔØô |
| NET-6 UDP Fast-Path GET P99 | < 500 ┬Ás (localhost) | ÔØô | ÔØô |

---

## 22. Security-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| SEC-1 AES-256-GCM (AES-NI) | ÔëÑ 1 GB/s/Core | ÔØô | ÔØô |
| SEC-2 RSA-4096 Signaturpr├╝fung P99 | Ôëñ 5 ms | ÔØô | ÔØô |
| SEC-3 Kyber-1024 Key Encapsulation | ÔëÑ 2k ops/s/Core | ÔØô | ÔØô |
| SEC-4 Dilithium-5 Signing | ÔëÑ 1k ops/s/Core | ÔØô | ÔØô |
| SEC-5 TLS 1.3 Handshake P99 | Ôëñ 10 ms (neue Verbindungen) | ÔØô | ÔØô |
| SEC-6 RBAC Policy Eval (Ôëñ100 Rollen) P99 | Ôëñ 0,5 ms | ÔØô | ÔØô |
| SEC-7 HSM-Backed RSA-2048 Sign P99 | Ôëñ 20 ms (SoftHSM2) | ÔØô | ÔØô |
| SEC-8 Audit Log Write P99 | Ôëñ 2 ms/Entry | ÔØô | ÔØô |

---

## 23. Scheduler-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| SCH-1 Scheduler Loop Tick P99 | Ôëñ 1 ms (10k Tasks) | ÔØô | ÔØô |
| SCH-2 Task Dispatch P99 | Ôëñ 5 ms (Due-Time ÔåÆ First Instruction) | ÔØô | ÔØô |
| SCH-3 Cron next_execution | Ôëñ 10 ┬Ás/Call | ÔØô | ÔØô |
| SCH-4 Leader Election Konvergenz | Ôëñ 5 s (5-Node-Cluster, nach Failure) | ÔØô | ÔØô |
| SCH-5 DAG Topological Sort | Ôëñ 1 ms (Ôëñ10k Nodes) | ÔØô | ÔØô |
| SCH-6 Throughput | ÔëÑ 5k Dispatches/s | ÔØô | ÔØô |

---

## 24. Ingestion-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| ING-1 Aggregate Throughput | ÔëÑ 50k Docs/s (Single Node) | ÔØô | ÔØô |
| ING-2 Kafka Consumer Throughput | ÔëÑ 100k Messages/s (1 KB avg) | ÔØô | ÔØô |
| ING-3 Kafka ÔåÆ Document E2E P99 | Ôëñ 500 ms | ÔØô | ÔØô |
| ING-4 S3 Concurrent Download | ÔëÑ 200 MB/s agg. (4 parallel, 10 Gbps) | ÔØô | ÔØô |
| ING-5 Quarantine Queue Scan (100k) | Ôëñ 1 s | ÔØô | ÔØô |

---

## 25. Governance-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| GOV-1 Policy Reload Latenz | Ôëñ 100 ms (Detection ÔåÆ Aktiv) | ÔØô | ÔØô |
| GOV-2 CCPA Opt-Out Lookup Overhead | Ôëñ 0,5 ms P99 | ÔØô | ÔØô |
| GOV-3 CCPA Report (90 Tage, 1M Subjects) | Ôëñ 10 s | ÔØô | ÔØô |
| GOV-4 Policy Evaluation P99 (500 Rules) | Ôëñ 5 ms (100 Threads) | ÔØô | ÔØô |
| GOV-5 DataMasker (50-Feld-Dokument) | Ôëñ 1 ms | ÔØô | ÔØô |

---

## 26. Observability-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| OBS-1 Metrics Collection Overhead | < 1 % CPU @ 1k req/s | ÔØô | ÔØô |
| OBS-2 Adaptive Span Sampling | Ôëñ 1 % bei > 10k Spans/s | ÔØô | ÔØô |
| OBS-3 Metrics Scrape (16 Scraper) | ÔëÑ 3├ù vs. Exclusive Mutex | ÔØô | ÔØô |

---

## 27. Process-Modul (Process Mining)

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| PROC-1 ProcessGraphRag::retrieve() | Ôëñ 200 ms (Ôëñ500 Nodes, exkl. LLM) | ÔØô | ÔØô |
| PROC-2 PPR (50 Iter., 500-Node-Graph) | Ôëñ 20 ms | ÔØô | ÔØô |
| PROC-3 Object-Centric DFG (10k Events) | Ôëñ 5 s | ÔØô | ÔØô |
| PROC-4 Total Conversation Latenz | Ôëñ 5 s (3-Turn, local llama.cpp 8B Q4) | ÔØô | ÔØô |
| PROC-5 CEP Alert Latenz | Ôëñ 100 ms nach Threshold-├£berschreitung | ÔØô | ÔØô |
| PROC-6 Bottleneck Analysis (10k Instances) | Ôëñ 2 s | ÔØô | ÔØô |
| PROC-7 Bottleneck Detection Accuracy | ÔëÑ 90 % | ÔØô | ÔØô |

---

## 28. Voice-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| VOI-1 STT Latenz P95 (5 s Audio) | Ôëñ 300 ms | ÔØô | ÔØô |
| VOI-2 TTS First-Token Latenz | Ôëñ 200 ms | ÔØô | ÔØô |
| VOI-3 Wake-Word Detection | Ôëñ 50 ms | ÔØô | ÔØô |
| VOI-4 End-to-End Voice Latenz | < 500 ms | ÔØô | ÔØô |
| VOI-5 Wake-Word CPU Usage (idle) | Ôëñ 2 % auf x86_64 | ÔØô | ÔØô |
| VOI-6 Concurrent WebSocket Sessions | ÔëÑ 100 | ÔØô | ÔØô |
| VOI-7 Speaker ID Acceptance | ÔëÑ 95 % | ÔØô | ÔØô |
| VOI-8 Speaker ID Impostor Rejection | ÔëÑ 99 % | ÔØô | ÔØô |
| VOI-9 Wake-Word False-Positive Rate | Ôëñ 1/Stunde | ÔØô | ÔØô |
| VOI-10 Silence Removal | 20ÔÇô40 % Reduktion | ÔØô | ÔØô |

---

## 29. ONNX-CLIP-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| OC-1 Batched Inference (Batch 64) | ÔëÑ 6├ù vs. Sequential | ÔØô | ÔØô |
| OC-2 ViT-B/32 CUDA (Batch 64) | Ôëñ 20 ms (Ôëñ 0,31 ms/Image) | ÔØô | ÔØô |
| OC-3 ViT-B/32 CPU (Batch 16) | Ôëñ 2,5 s | ÔØô | ÔØô |
| OC-4 Text Encoding P95 (CPU) | Ôëñ 5 ms | ÔØô | ÔØô |
| OC-5 Metrics Overhead | Ôëñ 0,05 ms/Call | ÔØô | ÔØô |

---

## 30. Chimera-Baseline & Suite

> **CHIMERA** = Comprehensive, Honest, Impartial Metrics for Empirical Reporting and Analysis  
> Framework: `benchmarks/chimera/` (v1.0.0) ┬À Standard: IEEE Std 2807-2022, ISO/IEC 14756:2015  
> Vollst├ñndige Dokumentation: `benchmarks/chimera/CHIMERA_README.md`

---

### 30.1 ThemisDB Chimera-Baseline (v1.5.0-dev)

> Quelle: `baselines/chimera/baseline.json` (Stand: 2026-03-01, Branch: main)

| Workload | Throughput (ops/s) | Mean Latenz | P95 | P99 | Modul |
|----------|--------------------|-------------|-----|-----|-------|
| relational_sort | **42.503** | 0,024 ms | 0,023 ms | 0,034 ms | Storage/Query |
| vector_dot_product | **75.835** | 0,013 ms | 0,013 ms | 0,024 ms | Index/Acceleration |
| document_lookup | **2.956.804** | 0,000180 ms | 0,000200 ms | 0,000250 ms | Storage/Cache |
| graph_bfs | **40.373** | 0,025 ms | 0,025 ms | 0,033 ms | Graph |

---

### 30.2 Chimera Suite ÔÇô Standardisierte Workloads (Benchmark-Definitionen)

> Quelle: `benchmarks/chimera/benchmark_config_schema.yaml`  
> Methodik: IEEE Std 2807-2022 ┬À Warmup: 60 s ┬À Messdauer: 300 s ┬À Runs: 5 ┬À Konfidenz: 95 %

| Workload-ID | Familie | Standard | Beschreibung | Ziel-Modul(e) |
|-------------|---------|----------|--------------|---------------|
| `ycsb_workload_a` | YCSB | Cooper2010 | Update Heavy (50 % Reads, 50 % Updates), 1 M Records, Zipfian | Storage, Cache, Transaction |
| `tpc_c` | TPC-C | TPC-C v5.11 | OLTP Order-Entry, 10 Warehouses, 300 s, New-Order 45 % | Transaction, Query, Storage |
| `tpc_h_sf1` | TPC-H | TPC-H v3.0.0 | Decision Support, Scale Factor 1 GB, Queries 1/2/3/6/14 | Analytics, Query |
| `ann_sift1m` | ANN-Benchmarks | Aum├╝ller2020 | SIFT1M (1 M ├ù 128-dim), k=10, Recall-Ziel 0.95 | Index/HNSW, Acceleration |
| `ldbc_snb_interactive` | LDBC-SNB | Erling2020 | Social Network Graph, SF1, Short+Complex Reads + Updates | Graph, Query |
| `vllm_serving` | vLLM | Kwon2023 | LLM Inference, Llama-2-7B, 512-Token Input, 1 req/s | LLM, Acceleration |
| `rag_qa` | RAGBench | Chen2024 | RAG E2E, NaturalQuestions, Top-5 Dense Retrieval | RAG, Search, LLM |

---

### 30.3 Chimera Vendorneutrale Demo-Ergebnisse (anonymisiert)

> Quelle: `benchmarks/chimera/demo_reports/benchmark_comparison.csv`  
> Methodik: 28ÔÇô50 Stichproben/System, Ausrei├ƒer per IQR (1.5├ù) entfernt, 95 % CI

**Query Throughput (queries/sec):**

| System | N | Mean | Median | Std Dev | P95 | P99 | CI 95 % Lower | CI 95 % Upper |
|--------|---|------|--------|---------|-----|-----|---------------|---------------|
| System Alpha | 29 | 14.842 | 14.813 | 732 | 16.200 | 16.251 | 14.604 | 15.286 |
| System Beta | 29 | 12.678 | 12.789 | 1.284 | 14.274 | 15.346 | 11.966 | 13.090 |
| System Gamma | 28 | 9.392 | 9.431 | 1.130 | 11.192 | 11.715 | 8.677 | 9.940 |

**Vector Search Latency P95 (ms):**

| System | N | Mean | Median | Std Dev | P95 | P99 | CI 95 % Lower | CI 95 % Upper |
|--------|---|------|--------|---------|-----|-----|---------------|---------------|
| System Aurora | 48 | 8,51 | 8,63 | 1,25 | 10,01 | 11,00 | 7,91 | 8,76 |
| System Nexus | 49 | 9,43 | 9,32 | 1,65 | 12,40 | 13,27 | 9,02 | 10,02 |
| System Quantum | 50 | 7,59 | 7,81 | 1,20 | 9,27 | 9,53 | 7,25 | 7,93 |
| System Vertex | 48 | 8,77 | 8,49 | 1,25 | 10,98 | 11,12 | 8,34 | 9,23 |
| System Zenith | 48 | 9,47 | 9,60 | 1,82 | 12,46 | 13,11 | 8,77 | 10,14 |

> **Hinweis:** System-Namen sind anonymisiert (IEEE-konforme Neutralit├ñt). ThemisDB kann als eines dieser Systeme identifiziert werden sobald ein Chimera-Zertifizierungslauf abgeschlossen ist.

---

### 30.4 Chimera Statistische Methodik

| Parameter | Wert | Referenz |
|-----------|------|----------|
| Signifikanzniveau (╬▒) | 0,05 | Standard |
| Konfidenzintervall | 95 % | Welch's t-test |
| Hypothesentests | Welch's t-test, Mann-Whitney U, KS-Test | Welch 1947, Mann 1947 |
| Effektgr├Â├ƒe | Cohen's d | Cohen 1988 |
| Ausrei├ƒer-Methode | IQR ├ù 1.5 | Tukey 1977 |
| Min. Stichprobengr├Â├ƒe | 30 | IEEE Std 2807-2022 |
| Warmup | 60 s | IEEE Std 2807-2022 |
| Messdauer | 300 s | IEEE Std 2807-2022 |
| Runs (unabh├ñngig) | 5 | IEEE Std 2807-2022 |

---

## 31. Prompt Engineering-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| PE-1 Prompt Construction P99 | Ôëñ 5 ms | ÔØô | ÔØô |
| PE-2 Template Compilation (4 KB) | < 50 ms | ÔØô | ÔØô |
| PE-3 Compiled Template Render P99 (2 KB) | < 1 ms | ÔØô | ÔØô |
| PE-4 CoT Tracing Overhead/Step | < 0,2 ms | ÔØô | ÔØô |
| PE-5 Full 3-Iteration Reflection (kein LLM) | < 1 ms P99 | ÔØô | ÔØô |
| PE-6 render() Latenz (String ÔåÆ Compiled) | ~8 ms ÔåÆ < 1 ms Ziel | ÔØô | ÔØô |
| PE-7 End-to-End RAG Assembly | ~15 ms ÔåÆ < 5 ms Ziel | ÔØô | ÔØô |

---

## 32. Ethics AI-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| ETH-1 Single Argument Generation P95 | Ôëñ 3 s (LLM, 500 Token) | ÔØô | ÔØô |
| ETH-2 Batch 5 Arguments (parallel, 5 Schulen) | Ôëñ 8 s | ÔØô | ÔØô |
| ETH-3 Embedding Latenz (512-Token, CPU) | Ôëñ 20 ms | ÔØô | ÔØô |
| ETH-4 Batch 10 Queries | Ôëñ 150 ms | ÔØô | ÔØô |
| ETH-5 Multi-Round Debate/Runde | Ôëñ 5 s inkl. LLM | ÔØô | ÔØô |
| ETH-6 Metrics Overhead/Decision | Ôëñ 0,1 ms | ÔØô | ÔØô |

---

## 33. System-Level (TPC/YCSB-Standards)

> Quelle: `benchmarks/README.md`, `COMPETITOR_COMPARISON.csv` (v1.3.4)

| # | Workload | Erwartungswert | Hardware-Referenz | v1.3.4 Gemessen | Status |
|---|----------|----------------|-------------------|-----------------|--------|
| BM-1 | OLTP (TPC-C) | 200ÔÇô300 K ops/s | 4-Core, 8 GB, SSD | ÔØô | ÔØô |
| BM-2 | OLTP (TPC-C) | 400ÔÇô600 K ops/s | 8-Core, 16 GB, NVMe | ÔØô | ÔØô |
| BM-3 | OLTP (TPC-C) | 700 KÔÇô1 M ops/s | 16-Core, 32 GB, NVMe | ÔØô | ÔØô |
| BM-4 | OLTP (TPC-C) | 1,2ÔÇô1,8 M ops/s | 32-Core, 64 GB, NVMe Gen4 | ÔØô | ÔØô |
| BM-5 | OLAP (TPC-H) | 100ÔÇô200 Queries/min | 8-Core, 16 GB, NVMe | ÔØô | ÔØô |
| BM-6 | Vector Search | 10ÔÇô20 K QPS | 8-Core, 16 GB, NVMe | ÔØô | ÔØô |
| BM-7 | TPC-C tpmC-Ziel | 150ÔÇô200 K tpmC (80ÔÇô100 % PostgreSQL) | 8-Core, 32 GB, NVMe | ÔØô | ÔØô |

**Competitor-Vergleich v1.3.4 (gemessen):**

| Kategorie | ThemisDB v1.3.4 | Bester Mitbewerber | Mitbewerber | Position | Delta |
|-----------|----------------|--------------------|-------------|----------|-------|
| Query Engine (OLAP) | 814,5 M items/s | 1.200 M items/s | ClickHouse | 2. (Sehr gut) | ÔêÆ47 % |
| Vector Insert | 351,4 k items/s | 600 k items/s | FAISS | 3. (Kompetitiv) | ÔêÆ71 % |
| Embedding Cache Hit | 155,8 M items/s | 1.000 M items/s | In-Memory Cache | 2. (Sehr gut) | Akzeptabel |
| 2PC Throughput | 6,4 k items/s | 15 k items/s | TiDB 7.0 | 3. (Solide) | ÔêÆ134 % |
| Hybrid Search | 450 queries/s | 500 queries/s | Weaviate | 2. (Stark) | ÔêÆ10 % |

---

## 34. Performance Regression CI ÔÇô Schwellwerte

> CI-Datei: `.github/workflows/05-quality_build_cross-module-performance-regression-ci.yml`

| Level | Schwellwert | Auswirkung |
|-------|-------------|------------|
| Minor | ÔëÑ 5 % | Tracking / informell |
| **Major** | **ÔëÑ 10 %** | **Blockiert PR-Merge** |
| Critical | ÔëÑ 20 % | Sofortiger Eingriff |

---

## 35. Bekannte Performance-L├╝cken (explizit dokumentiert)

| # | Modul | Ist-Stand | Ziel | ╬ö | Priorit├ñt |
|---|-------|-----------|------|---|-----------|
| D-1 | Timeseries Write (TS-1) | ~200 k pts/s | > 500 k pts/s | **ÔêÆ60 %** | Hoch |
| D-2 | Gorilla Decode (TS-2) | ~400 MB/s | > 2 GB/s | **ÔêÆ80 %** | Hoch |
| D-3 | Vector Insert vs. FAISS | 351 k/s | 600 k/s | **ÔêÆ71 %** | Mittel |
| D-4 | 2PC Throughput vs. TiDB | 6,4 k/s | 15 k/s | **ÔêÆ134 %** | Mittel |
| D-5 | Storage 1 MB Blob Write | 741 ops/s | ÔëÑ 100 k ops/s | **ÔêÆ99 %** | Hoch |
| D-6 | Concurrency 10 Clients CV | CV=20,74 ÔÜá´©Å | stabil | Instabil | Mittel |
| D-7 | Query Engine vs. ClickHouse | 814,5 M/s | 1.200 M/s | **ÔêÆ47 %** | Niedrig |

---

*Dieses Dokument wird automatisch aus den FUTURE_ENHANCEMENTS.md und Benchmark-Ergebnissen der jeweiligen Module generiert. F├╝r Aktualisierungen bitte die entsprechenden Quelldateien pflegen.*

---

## 36. Versions├╝bergreifende Benchmark-Messwerte (Rohdaten)

> Alle Werte aus Google Benchmark (C++). `real_time` = Wall-Clock, `cpu_time` = CPU-Zeit.
> Run-IDs: **v1.3.0** = 20251223_084034 | **v1.3.3** = 20251223_085556 | **v1.3.4** = 20251229_184507

---

### 36.1 Kern-Performance (`bench_core_performance`)

| Benchmark | v1.3.0 items/s | v1.3.3 items/s | v1.3.4 items/s | ╬ö v1.3.0ÔåÆv1.3.4 | Status |
|-----------|---------------|---------------|---------------|-----------------|--------|
| VectorIndexBench/InsertPlaintext | 566.7 k/s | 538.0 k/s | **351.4 k/s** | ÔêÆ38 % | ÔÜá´©Å |
| SecondaryIndexBench/IndexInsert | 1.78 M/s | 5.11 k/s ÔÜá´©Å | **217.2 k/s** | ÔêÆ88 % | ÔØî |
| SecondaryIndexBench/RawWriteOnly | ÔÇô | ÔÇô | **885.0 k/s** | n/a | ÔØô |
| QueryEngineBench/SimpleEvaluation | 968.6 M/s | 949.8 M/s | **814.5 M/s** | ÔêÆ16 % | ÔÜá´©Å |
| GraphIndexBench/AddEdges | 1.47 M/s | 1.20 M/s | **628.7 k/s** | ÔêÆ57 % | ÔØî |
| TimeseriesBench/InsertTimepoints | 61.0 M/s | 55.9 M/s | **49.0 M/s** | ÔêÆ20 % | ÔÜá´©Å |

> **Hinweis SecondaryIndex v1.3.3:** real_time=656 ms, cpu_time=19.6 ms ÔåÆ 33├ù Diskrepanz durch Einzel-Transaktion pro `put()` (RocksDB-Transaktions-Overhead). Bekannte Regression, dokumentiert in `PERFORMANCE_COMPARISON_V1.3.0_VS_V1.3.3.md`.

---

### 36.2 Umfassende Workloads (`bench_comprehensive`)

| Benchmark | v1.3.3 items/s | v1.3.4 items/s | Ziel | Status |
|-----------|---------------|---------------|------|--------|
| **Vektor-Operationen** | | | | |
| SimpleVectorBench/Insert_RGB_Vectors | 1.33 M/s | **1.22 M/s** | ÔÇô | ÔÜá´©Å |
| SimpleVectorBench/Search_RGB_KNN_Top10 | 63.7 M/s | **62.1 M/s** | ÔÇô | Ô£à |
| SimpleVectorBench/Insert_384D_Embeddings | 465.5 k/s | **382.3 k/s** | ÔÇô | ÔÜá´©Å |
| ComplexVectorBench/BatchInsert_1536D_LLMVectors | 132.8 k/s | **121.9 k/s** | ÔÇô | ÔÜá´©Å |
| ComplexVectorBench/Search_4096D_TopK_Batch | 5.97 M/s | **5.62 M/s** | ÔÇô | ÔÜá´©Å |
| **LLM / Embedding** | | | | |
| LLMInferencingBench/EmbeddingGeneration_Store | 122.0 k/s | **108.2 k/s** | ÔÇô | ÔÜá´©Å |
| LLMInferencingBench/RAG_Search_Retrieve_Top50 | ÔÇô | **7.55 M/s** (133 ns) | ÔÇô | ÔØô |
| LLMInferencingBench/MultiQueryExpansion_5Queries | ÔÇô | **2.97 M/s** | ÔÇô | ÔØô |
| **AQL / Query** | | | | |
| AQLQueryBench/SimpleSelect_WhereClause | ÔÇô | **148.8 k/s** (6.7 ┬Ás) | ÔÇô | ÔØô |
| AQLQueryBench/ComplexSelect_MultipleConditions | ÔÇô | **3.25 k/s** (308 ┬Ás) | ÔÇô | ÔØô |
| AQLJoinBench/JoinUsers_Posts | ÔÇô | **777.0 k/s** (1.3 ┬Ás) | ÔÇô | ÔØô |
| **Blob / Bin├ñr** | | | | |
| BinaryOperationsBench/StoreThumbnails_10KB | ÔÇô | **4.92 k/s** | ÔÇô | ÔØô |
| BinaryOperationsBench/StoreLargeBlobs_1MB | ÔÇô | **352 ops/s** | ÔÇô | ÔØô |
| BinaryOperationsBench/RetrieveBlobsBatch_100x100KB | ÔÇô | **117.0 k/s** | ÔÇô | ÔØô |
| **Graph** | | | | |
| GraphOperationsBench/AddEdges_SparseGraph | ÔÇô | **1.17 M/s** | ÔÇô | ÔØô |
| GraphOperationsBench/QueryNeighbors_DenseGraph | ÔÇô | **975.2 k/s** | ÔÇô | ÔØô |
| GraphOperationsBench/GraphTraversal_BFS_Depth3 | ÔÇô | **910.2 k/s** (1.09 ┬Ás) | ÔÇô | ÔØô |
| **Index** | | | | |
| SecondaryIndexBench/SmallIndexInsert_1K | ÔÇô | **5.82 k/s** | ÔÇô | ÔØô |
| SecondaryIndexBench/MediumIndexInsert_100K | ÔÇô | **9.14 k/s** | ÔÇô | ÔØô |
| SecondaryIndexBench/LargeIndexLookup_1M | ÔÇô | **165.5 k/s** | ÔÇô | ÔØô |
| SecondaryIndexBench/CompositeIndexLookup | ÔÇô | **7.59 k/s** | ÔÇô | ÔØô |
| **Batch / Stress** | | | | |
| BatchOperationsBench/BatchInsert_10K_WithMetadata | ÔÇô | **779.1 k/s** | ÔÇô | ÔØô |
| BatchOperationsBench/BatchUpdate_MultiField_5K | ÔÇô | **779.1 k/s** | ÔÇô | ÔØô |
| StressTestBench/MixedReadWrite_80Reads_20Writes | ÔÇô | **22.9 k/s** | ÔÇô | ÔØô |
| StressTestBench/HotspotAccess_99PercentContention | ÔÇô | **5.79 M/s** | ÔÇô | ÔØô |

---

### 36.3 Verschl├╝sselung (`bench_encryption`)

> Platform: v1.3.3 = Run 20251223_085556 | v1.3.4 = Run 20251229_184507

| Benchmark | v1.3.3 ops/s | v1.3.4 ops/s | ╬ö | Status |
|-----------|-------------|-------------|---|--------|
| BM_Encrypt_String_UsingKey/64 | 277.0 k/s (3.6 ┬Ás) | **254.9 k/s** (3.9 ┬Ás) | ÔêÆ8 % | ÔÜá´©Å |
| BM_Encrypt_String_UsingKey/256 | 254.4 k/s | **244.0 k/s** | ÔêÆ4 % | ÔÜá´©Å |
| BM_Encrypt_String_UsingKey/1024 | 254.9 k/s | **191.2 k/s** | ÔêÆ25 % | ÔØî |
| BM_Decrypt_String_UsingKey/64 | 56.9 k/s | **45.5 k/s** | ÔêÆ20 % | ÔØî |
| BM_Decrypt_String_UsingKey/256 | 60.1 k/s | **41.1 k/s** | ÔêÆ32 % | ÔØî |
| BM_Decrypt_String_UsingKey/1024 | 52.5 k/s | **36.6 k/s** | ÔêÆ30 % | ÔØî |
| BM_UserEntity_Encrypt_Serialize | ÔÇô | **28.3 k/s** (35.1 ┬Ás) | ÔÇô | ÔØô |
| BM_HKDF_Derive_FieldKey | ÔÇô | **177.8 k/s** (5.5 ┬Ás) | ÔÇô | ÔØô |
| BM_SchemaEncrypt_SingleField/64 | ÔÇô | **86.1 k/s** (11.6 ┬Ás) | ÔÇô | ÔØô |
| BM_SchemaEncrypt_SingleField/1024 | ÔÇô | **93.7 k/s** (10.7 ┬Ás) | ÔÇô | ÔØô |
| BM_SchemaDecrypt_SingleField/64 | ÔÇô | **26.9 k/s** (68.2 ┬Ás) | ÔÇô | ÔØô |
| BM_VectorFloat_Encryption | ÔÇô | **55.6 k/s** (17.9 ┬Ás) | ÔÇô | ÔØô |
| BM_DB_Ingest_Encrypted/100000 | ÔÇô | **27.9 k/s** (3.58 s) | ÔÇô | ÔØô |
| BM_Index_Insert_Plain/100000 | ÔÇô | **1.03 M/s** (97.4 ms) | ÔÇô | ÔØô |
| BM_Index_Insert_WithEncryptedPayload/100000 | ÔÇô | **717.2 k/s** (139.4 ms) | ÔÇô | ÔØô |

---

### 36.4 Vektor-Distanz & Geo-Filterung (`bench_hybrid_vector_geo`)

> Run 20251229_184507 (v1.3.4)

| Benchmark | real_time (ns) | ops/s (1e9/rt) |
|-----------|---------------|----------------|
| **Euklidische Distanz** | | |
| BM_VectorDistance_Euclidean/64 | 42.2 ns | 23.7 M/s |
| BM_VectorDistance_Euclidean/128 | 105.5 ns | 9.5 M/s |
| BM_VectorDistance_Euclidean/256 | 208.6 ns | 4.8 M/s |
| BM_VectorDistance_Euclidean/512 | 434.5 ns | 2.3 M/s |
| BM_VectorDistance_Euclidean/1024 | 827.5 ns | 1.21 M/s |
| **Kosinus-Distanz** | | |
| BM_VectorDistance_Cosine/64 | 38.0 ns | 26.4 M/s |
| BM_VectorDistance_Cosine/128 | 96.3 ns | 10.4 M/s |
| BM_VectorDistance_Cosine/256 | 204.7 ns | 4.9 M/s |
| BM_VectorDistance_Cosine/512 | 441.5 ns | 2.3 M/s |
| BM_VectorDistance_Cosine/1024 | 827.3 ns | 1.21 M/s |
| **Vektor-Normalisierung** | | |
| BM_VectorNormalization/128 | 881.7 ns | 1.13 M/s |
| BM_VectorNormalization/512 | 3.553 ┬Ás | 281 k/s |
| BM_VectorNormalization/1024 | 7.237 ┬Ás | 138 k/s |
| **Haversine-Distanz (Geo)** | | |
| BM_GeoDistance_Haversine/100 | 3.576 ┬Ás | 28.0 M pts/s |
| BM_GeoDistance_Haversine/512 | 20.16 ┬Ás | 25.4 M pts/s |
| BM_GeoDistance_Haversine/4096 | 172.8 ┬Ás | 23.7 M pts/s |
| BM_GeoDistance_Haversine/10000 | 504.8 ┬Ás | 19.8 M pts/s |
| **Geo Point-in-Bounding-Box** | | |
| BM_GeoPointInBoundingBox/100 | 64.3 ns | 1.56 G pts/s |
| BM_GeoPointInBoundingBox/4096 | 9.375 ┬Ás | 437 M pts/s |
| BM_GeoPointInBoundingBox/100000 | 232.2 ┬Ás | 431 M pts/s |
| **Vektor+Geo kombiniert (Pre-Filter)** | | |
| BM_VectorGeoFiltering/1000 | 35.1 ┬Ás | 28.5 M/s |
| BM_VectorGeoFiltering/4096 | 150.7 ┬Ás | 27.2 M/s |
| BM_VectorGeoFiltering/32768 | 1.270 ms | 25.8 M/s |
| BM_VectorGeoFiltering/50000 | 1.892 ms | 26.4 M/s |

---

### 36.5 HNSW Pre-/Postfilter (`bench_hnsw_prefilter_minimal`)

> v1.3.4 (Run 20251229_184507)

| Benchmark | real_time (ns) | ops/s |
|-----------|---------------|-------|
| BenchPrefilter/1000 | 435.1 ms | 2,30 ops/s |
| BenchPrefilter/5000 | 161.6 ms | 6,19 ops/s |
| BenchPrefilter/10000 | 93.5 ms | 10,70 ops/s |
| BenchPrefilter/20000 | 88.2 ms | 11,34 ops/s |
| BenchPostfilter/1000 | 79.5 ms | 12,58 ops/s |
| BenchPostfilter/5000 | 79.9 ms | 12,52 ops/s |
| BenchPostfilter/10000 | 79.4 ms | 12,60 ops/s |
| BenchPostfilter/20000 | 78.9 ms | 12,68 ops/s |

> **Beobachtung:** Prefilter ist bei kleinem n (1000) 5.5├ù langsamer als Postfilter. Ab n=20000 ann├ñhernde Parit├ñt (88 ms vs. 79 ms). Dies entspricht dem theoretischen Verhalten: Prefilter lohnt sich erst ab hoher Selektivit├ñt.

---

### 36.6 Storage Hotspots ÔÇô WAL / Mixed-RW (`bench_hotspots_micro`)

> v1.3.3 vs. v1.3.4 ÔÇö Thread-Count-Skalierung

| Benchmark | Threads | v1.3.3 ops/s | v1.3.4 ops/s | ╬ö |
|-----------|---------|-------------|-------------|---|
| **WAL ON (persistentes Schreiben)** | | | | |
| BM_RawWrite_WAL_On | 1 | 248 | **283** | +14 % |
| BM_RawWrite_WAL_On | 4 | 542 | **609** | +12 % |
| BM_RawWrite_WAL_On | 8 | 1.058 | **1.193** | +13 % |
| BM_RawWrite_WAL_On | 16 | 2.070 | **1.546** | ÔêÆ25 % ÔÜá´©Å |
| **WAL OFF (In-Memory)** | | | | |
| BM_RawWrite_WAL_Off | 1 | 205.5 k | **145.7 k** | ÔêÆ29 % ÔØî |
| BM_RawWrite_WAL_Off | 4 | 354.7 k | **370.3 k** | +4 % |
| BM_RawWrite_WAL_Off | 8 | ÔÇô | **507.5 k** | ÔÇô |
| BM_RawWrite_WAL_Off | 16 | ÔÇô | **350.3 k** | ÔÇô |
| **Mixed RW (80% Read / 20% Write)** | | | | |
| BM_MixedRW | 1 | 583 | **583** | 0 % |
| BM_MixedRW | 4 | 1.289 | **1.289** | 0 % |
| BM_MixedRW | 8 | ÔÇô | **2.534** | ÔÇô |
| BM_MixedRW | 16 | ÔÇô | **4.405** | ÔÇô |
| **Secondary Index Write** | | | | |
| BM_SecondaryIndex_Write | 1 | 281 | **281** | 0 % |
| BM_SecondaryIndex_Write | 4 | 590 | **590** | 0 % |
| BM_SecondaryIndex_Write | 8 | ÔÇô | **1.056** | ÔÇô |
| BM_SecondaryIndex_Write | 16 | ÔÇô | **1.990** | ÔÇô |

---

### 36.7 AQL-Funktionen (`bench_aql_functions` / v1.3.4)

> Embedding-Cache, Hybrid Search, CTEs, Distributed Transactions

| Benchmark | real_time | items/s | Anmerkung |
|-----------|-----------|---------|-----------|
| **Embedding-Cache** | | | |
| BM_EmbeddingCache_Store/384 | 1.324 ┬Ás | 758.5 k/s | |
| BM_EmbeddingCache_Store/768 | 2.699 ┬Ás | 374.8 k/s | |
| BM_EmbeddingCache_Store/1536 | 158.2 ┬Ás | 14.2 k/s | gr├Â├ƒerer Dimensionsaufwand |
| BM_EmbeddingCache_Query_Hit/384 | 6.44 ns | **155.8 M/s** | Hot Path |
| BM_EmbeddingCache_Query_Hit/768 | 6.46 ns | **155.8 M/s** | Hot Path |
| BM_EmbeddingCache_Query_Hit/1536 | 1.882 ┬Ás | 541.0 k/s | |
| BM_EmbeddingCache_Query_Hit/3072 | 6.46 ns | **155.0 M/s** | Hot Path |
| BM_EmbeddingCache_Query_Miss/384 | 1.298 ┬Ás | 777.0 k/s | |
| BM_EmbeddingCache_CostSavings | 1.697 ┬Ás | 585.1 k/s | |
| **Hybrid Search** | | | |
| BM_HybridSearch_RRF/384 | 148.3 ns | 6.64 M/s | |
| BM_HybridSearch_RRF/768 | 141.0 ns | 7.08 M/s | |
| BM_HybridSearch_RRF/1536 | 148.8 ns | 6.67 M/s | |
| BM_HybridSearch_LinearCombination | 101.6 ns | 9.75 M/s | |
| BM_HybridSearch_VaryingWeights/50 | 99.1 ns | **10.17 M/s** | Optimum bei 50/50 |
| **CTEs (Non-Recursive)** | | | |
| BM_CTE_NonRecursive_Simple/1 | 1.049 ns | 952.6 M/s | |
| BM_CTE_NonRecursive_Simple/5 | 5.679 ns | 874.1 M/s | |
| BM_CTE_NonRecursive_Simple/10 | 10.90 ns | 910.2 M/s | |
| BM_CTE_NonRecursive_Simple/20 | 21.26 ns | 938.5 M/s | |
| **CTEs (Recursive)** | | | |
| BM_CTE_Recursive_Depth/10 | 11.32 ns | 87.1 M/s | |
| BM_CTE_Recursive_Depth/50 | 60.81 ns | 16.3 M/s | |
| BM_CTE_Recursive_Depth/100 | 118.3 ns | 8.61 M/s | |
| BM_CTE_Recursive_Depth/1000 | 1.110 ┬Ás | 896.0 k/s | |
| **CTE Cycle-Detection** | | | |
| BM_CTE_CycleDetection/100 | 52.2 ns | 19.4 M/s | |
| BM_CTE_CycleDetection/1000 | 122.4 ns | 8.15 M/s | |
| BM_CTE_CycleDetection/10000 | 1.178 ┬Ás | 853.3 k/s | |
| **Subquery EXISTS** | | | |
| BM_Subquery_EXISTS_WithLIMIT1/100 | ~0 ns | Ôê× | Short-Circuit |
| BM_Subquery_EXISTS_WithLIMIT1/100000 | ~0 ns | Ôê× | Short-Circuit Ô£à |
| BM_Subquery_EXISTS_WithoutLIMIT1/100 | 75.1 ns | 13.3 M/s | |
| BM_Subquery_EXISTS_WithoutLIMIT1/1000 | 702.2 ns | 1.41 M/s | |
| BM_Subquery_EXISTS_WithoutLIMIT1/10000 | 6.822 ┬Ás | 147.0 k/s | |
| BM_Subquery_EXISTS_WithoutLIMIT1/100000 | 68.49 ┬Ás | 14.7 k/s | linear skalierend |
| **Distributed Transactions (2PC)** | | | |
| BM_DistributedTxn_2PC_Latency/2 Shards | 46.04 ms | **6.400 ops/s** | |
| BM_DistributedTxn_2PC_Latency/4 Shards | 46.09 ms | **6.400 ops/s** | |
| BM_DistributedTxn_2PC_Latency/8 Shards | 46.09 ms | **1.600 ops/s** | Overhead skaliert |
| BM_DistributedTxn_2PC_Latency/16 Shards | 45.95 ms | **1.280 ops/s** | |
| BM_DistributedTxn_Throughput | 46.01 ms | **6.400 ops/s** | |
| BM_DistributedTxn_SnapshotRead/4 | 61.54 ms | **6.400 ops/s** | |
| **LLM/RAG Pipeline** | | | |
| BM_Combined_LLM_RAG_Pipeline | 151.4 ┬Ás | **15.9 k/s** | |

---

### 36.8 Graph-Traversal (`bench_graph_traversal`)

> Run 20251223_085556 (v1.3.3)

| Benchmark | v1.3.3 real_time (ms) | v1.3.3 ops/s | v1.8.1-rc2 real_time (ms, lokal) | v1.8.1-rc2 ops/s (lokal) |
|-----------|----------------------|--------------|----------------------------------|----------------------------|
| **BFS** | | | | |
| GraphTraversalBenchmarkFixture/BFSTraversal/100 nodes/depth 4 | 0.184 ms | 5.430 k/s | 0.214 ms | 4.757 k/s |
| GraphTraversalBenchmarkFixture/BFSTraversal/1000 nodes/depth 4 | 1.56 ms | 0.652 k/s | 1.25 ms | 0.823 k/s |
| GraphTraversalBenchmarkFixture/BFSTraversal/10000 nodes/depth 4 | 20.2 ms | 50.6 ops/s | 23.2 ms | 44.224 ops/s |
| GraphTraversalBenchmarkFixture/BFSTraversal/100 nodes/depth 20 | 0.469 ms | 2.108 k/s | 0.514 ms | 1.914 k/s |
| GraphTraversalBenchmarkFixture/BFSTraversal/1000 nodes/depth 20 | 4.38 ms | 232.7 ops/s | 4.65 ms | 215.111 ops/s |
| **DFS** | | | | |
| GraphTraversalBenchmarkFixture/DFSTraversal/100 nodes/depth 4 | 0.184 ms | 5.379 k/s | 0.235 ms | 4.449 k/s |

> Lokale Messquelle v1.8.1-rc2: `benchmarks/results/local_20260409_093136/bench_graph_traversal.txt` (Google Benchmark, `_mean`).

---

### 36.9 GNN-Embeddings (`bench_gnn_embeddings`)

> Run 20251223_085556 (v1.3.3)

| Benchmark | real_time (ms) | items/s |
|-----------|---------------|---------|
| NodeEmbeddingGeneration/100 nodes/5 dims | 0.00158 ms | 446.0 M/s |
| NodeEmbeddingGeneration/1000 nodes/5 dims | 0.00173 ms | 4.469 G/s |
| NodeEmbeddingGeneration/10000 nodes/5 dims | 0.00206 ms | 38.1 G/s |
| NodeEmbeddingGeneration/100 nodes/20 dims | 0.00200 ms | 39.3 G/s |
| BatchEmbeddingGeneration/1000 nodes/5 dims/batch 10 | 3.15 ms | 1.260 M/s |
| BatchEmbeddingGeneration/1000 nodes/5 dims/batch 50 | 5.87 ms | 1.179 M/s |

---

### 36.10 GPU-Backends (`bench_gpu_backends`)

> v1.3.3 vs. v1.3.4 ÔÇö CPU-Backend (GPU nicht verf├╝gbar in CI)

| Benchmark | v1.3.3 items/s | v1.3.4 items/s | ╬ö |
|-----------|---------------|---------------|---|
| BM_CPUBackend_DistanceComputation/10├ù1000 | 11.24 M/s | **10.24 M/s** | ÔêÆ9 % |
| BM_CPUBackend_DistanceComputation/100├ù10000 | 11.49 M/s | **9.60 M/s** | ÔêÆ16 % |
| BM_CPUBackend_DistanceComputation/1000├ù100000 | 10.63 M/s | **9.95 M/s** | ÔêÆ7 % |
| BM_BackendComparison_VaryingDimensions/64 | 28.28 M/s | **25.87 M/s** | ÔêÆ9 % |
| BM_BackendComparison_VaryingDimensions/128 | 11.49 M/s | **9.74 M/s** | ÔêÆ15 % |
| BM_BackendComparison_VaryingDimensions/256 | 5.19 M/s | **4.36 M/s** | ÔêÆ16 % |
| BM_BackendComparison_VaryingDimensions/512 | ÔÇô | **2.29 M/s** | ÔÇô |
| BM_BackendComparison_VaryingDimensions/1024 | ÔÇô | **1.08 M/s** | ÔÇô |
| BM_BackendInitializationOverhead | ÔÇô | **14.93 M/s** | ÔÇô |
| BM_ThroughputComparison | ÔÇô | **10.10 M/s** | ÔÇô |

---

### 36.11 Image-Analyse (`bench_image_analysis`)

> Run 20251229_184507 (v1.3.4)

| Benchmark | real_time | ops/s | Anmerkung |
|-----------|-----------|-------|-----------|
| BM_ImageEmbedding_SingleImage/224px | 3.95 ┬Ás | 253.3 k/s | |
| BM_ImageEmbedding_SingleImage/384px | 4.11 ┬Ás | 243.3 k/s | |
| BM_ImageEmbedding_SingleImage/512px | 4.25 ┬Ás | 235.1 k/s | |
| BM_ImageEmbedding_SingleImage/1024px | 4.88 ┬Ás | 205.0 k/s | |
| BM_ImageEmbedding_Batch/1 | 3.87 ┬Ás | 258.4 k/s | |
| BM_ImageEmbedding_Batch/4 | 15.47 ┬Ás | 258.6 k/s | ~konstant/Bild |
| BM_ImageEmbedding_Batch/8 | 30.84 ┬Ás | 259.5 k/s | |
| BM_ImageEmbedding_Batch/16 | 63.24 ┬Ás | 253.1 k/s | |
| BM_ImageCaptioning/224px | 20.76 ┬Ás | 48.2 k/s | |
| BM_ImageCaptioning/384px | 61.27 ┬Ás | 16.3 k/s | |
| BM_ImageCaptioning/512px | 113.4 ┬Ás | 8.82 k/s | |
| BM_Plugin_Initialization | 5.51 ns | 181.6 M/s | sehr schnell |
| BM_Plugin_Warmup | 4.05 ┬Ás | 246.7 k/s | |

**Image Latenz-Verteilung** (`bench_image_analysis_latency`, v1.3.4):

| Benchmark | Mean (ms) | P50 (ms) | P95 (ms) | P99 (ms) |
|-----------|-----------|----------|----------|----------|
| BM_Embedding_LatencyDistribution_224 | 1.583 ┬Ás | 1.500 ┬Ás | 1.600 ┬Ás | 2.200 ┬Ás |
| BM_Embedding_ColdStartVsWarm (cold) | 1.960 ┬Ás | ÔÇô | ÔÇô | ÔÇô |
| BM_Embedding_ColdStartVsWarm (warm) | 1.881 ┬Ás | ÔÇô | ÔÇô | ÔÇô |
| BM_Embedding_GPUvsCPU/CPU | 2.633 ┬Ás | 2.100 ┬Ás | 2.200 ┬Ás | 20.3 ┬Ás |
| BM_Embedding_GPUvsCPU/GPU | 2.306 ┬Ás | 1.700 ┬Ás | 2.500 ┬Ás | 21.1 ┬Ás |
| BM_Caption_LatencyDistribution | 22.0 ┬Ás | 21.1 ┬Ás | 22.6 ┬Ás | 40.2 ┬Ás |
| BM_Batch_LatencyPerImage/1 | 1.975 ┬Ás | 1.700 ┬Ás | 1.800 ┬Ás | ÔÇô |
| BM_Batch_LatencyPerImage/4 (per img) | 1.583 ┬Ás | 1.475 ┬Ás | 1.575 ┬Ás | ÔÇô |
| BM_Batch_LatencyPerImage/8 (per img) | 1.506 ┬Ás | 1.450 ┬Ás | 1.500 ┬Ás | ÔÇô |
| BM_Batch_LatencyPerImage/16 (per img) | 1.537 ┬Ás | 1.481 ┬Ás | 1.563 ┬Ás | ÔÇô |
| BM_ImageSize_LatencyImpact/384px | 2.514 ┬Ás | 2.200 ┬Ás | 2.300 ┬Ás | ÔÇô |
| BM_ImageSize_LatencyImpact/512px | 3.023 ┬Ás | 2.700 ┬Ás | 2.800 ┬Ás | ÔÇô |
| BM_ImageSize_LatencyImpact/1024px | 6.007 ┬Ás | 5.700 ┬Ás | 6.000 ┬Ás | ÔÇô |

---

### 36.12 HSM-Provider (`bench_hsm_provider`)

> v1.3.3 vs. v1.3.4 ÔÇö Stub-Implementierung (echte HSM-Bibliothek nicht in CI)

| Benchmark | v1.3.3 ops/s | v1.3.4 ops/s | ╬ö |
|-----------|-------------|-------------|---|
| BM_HSM_Sign_Stub | 1.493 M/s (667 ns) | **1.434 M/s** (693.8 ns) | ÔêÆ4 % |
| BM_HSM_Verify_Stub | 1.629 M/s (612 ns) | **1.550 M/s** (659 ns) | ÔêÆ5 % |
| BM_HSM_Sign_Real_Pool* | n/a (Lib fehlt) | n/a | ÔÇô |

> **Ziel** SEC-7: HSM-Backed RSA-2048 Sign P99 Ôëñ 20 ms ÔåÆ Stub-Werte ~0.7 ┬Ás, Real-HSM-Werte ausstehend.

---

### 36.13 AQL-Sugar Hybrid (`bench_hybrid_aql_sugar`)

> v1.3.3 vs. v1.3.4

| Benchmark | v1.3.3 ops/s | v1.3.4 ops/s | ╬ö |
|-----------|-------------|-------------|---|
| BM_VectorGeo_AQL_Sugar | ÔÇô (ERROR) | ÔÇô (ERROR) | ÔÇô |
| BM_VectorGeo_CPP_API | 123.6 ops/s (8.58 ms) | **112.6 ops/s** (8.91 ms) | ÔêÆ9 % |
| BM_ContentGeo_AQL_Sugar | 5.556 k/s (0.457 ms) | **6.127 k/s** (0.347 ms) | +10 % Ô£à |
| BM_ContentGeo_CPP_API | 5.589 k/s (0.436 ms) | **7.191 k/s** (0.319 ms) | +29 % Ô£à |
| BM_AQL_Parse_Translate_Only | 152.5 k/s (6.57 ┬Ás) | **150.9 k/s** (6.66 ┬Ás) | ÔêÆ1 % Ô£à |

---

### 36.14 Content-Versionierung (`bench_content_versioning`)

> Run 20251229_184507 (v1.3.4)

| Benchmark | real_time | bytes/s |
|-----------|-----------|---------|
| BM_VersionCreation/1 KB | 1.14 ┬Ás | 895 MB/s |
| BM_VersionCreation/10 KB | 10.4 ┬Ás | 979 MB/s |
| BM_VersionCreation/100 KB | 104.3 ┬Ás | 975 MB/s |
| BM_VersionCreation/1 MB | 1.197 ms | 877 MB/s |
| BM_VersionCreation/10 MB | 12.71 ms | 810 MB/s |
| BM_DiffComputation/1 KB | 69.9 ns | 29.4 GB/s |
| BM_DiffComputation/10 KB | 188.0 ns | 108.7 GB/s |
| BM_DiffComputation/100 KB | 2.515 ┬Ás | 81.3 GB/s |
| BM_DiffComputation/1 MB | 245.7 ┬Ás | 8.53 GB/s |
| BM_VersionRetrieval | 302.5 ns | ÔÇô |
| BM_StorageOverhead/10 versions | 57.95 ┬Ás | ÔÇô |
| BM_StorageOverhead/100 versions | 744.4 ┬Ás | ÔÇô |
| BM_StorageOverhead/500 versions | 2.727 ms | ÔÇô |
| BM_ConcurrentVersioning/1 Thread | 11.86 ┬Ás | 875 MB/s |
| BM_ConcurrentVersioning/2 Threads | 12.61 ┬Ás | 833 MB/s |
| BM_ConcurrentVersioning/4 Threads | 15.64 ┬Ás | 667 MB/s |
| BM_ConcurrentVersioning/8 Threads | 19.48 ┬Ás | 506 MB/s |

---

### 36.15 ARM-Speicherbandbreite (`bench_arm_memory`)

> Run 20251229_184507 (v1.3.4, x86_64-Emulation auf ARM-Pfad)

| Benchmark | Blockgr├Â├ƒe | real_time | Bandbreite |
|-----------|-----------|-----------|------------|
| **Sequential Read** | | | |
| BM_ARM_Sequential_Read | 4 KB | 3.63 ┬Ás | 4.55 GB/s |
| BM_ARM_Sequential_Read | 32 KB | 28.28 ┬Ás | 4.60 GB/s |
| BM_ARM_Sequential_Read | 256 KB | 220.9 ┬Ás | 4.77 GB/s |
| BM_ARM_Sequential_Read | 1 MB | 908.1 ┬Ás | 4.66 GB/s |
| **Sequential Write** | | | |
| BM_ARM_Sequential_Write | 4 KB | 2.07 ┬Ás | 7.99 GB/s |
| BM_ARM_Sequential_Write | 32 KB | 16.81 ┬Ás | 7.76 GB/s |
| BM_ARM_Sequential_Write | 256 KB | 133.6 ┬Ás | 7.95 GB/s |
| BM_ARM_Sequential_Write | 1 MB | 528.9 ┬Ás | 7.90 GB/s |
| **MemCopy (builtin)** | | | |
| BM_ARM_MemCopy_Builtin | 4 KB | 139.9 ns | 118.6 GB/s |
| BM_ARM_MemCopy_Builtin | 32 KB | 2.077 ┬Ás | 63.9 GB/s |
| BM_ARM_MemCopy_Builtin | 256 KB | 32.00 ┬Ás | 33.0 GB/s |
| BM_ARM_MemCopy_Builtin | 1 MB | 129.9 ┬Ás | 32.6 GB/s |

---

### 36.16 MVCC-Transaktionen (`bench_mvcc`)

> Run 20251223_085556 (v1.3.3)

| Benchmark | real_time | ops/s | Anmerkung |
|-----------|-----------|-------|-----------|
| MVCCFixture/SingleEntityCommit_MVCC | 4.07 ms | 7.111 k/s | |
| MVCCFixture/BatchInsert100_MVCC | 7.29 ms | 29.67 k/s | |
| MVCCFixture/SnapshotIsolationOverhead_MVCC | 4.05 ms | 40.0 k/s | |
| MVCCFixture/Rollback_MVCC | 266.0 ┬Ás | 37.33 k/s | |
| MVCCFixture/SingleEntityCommit_WriteBatch | 4.38 ms | 6.516 k/s | |
| MVCCFixture/BatchInsert100_WriteBatch | 6.25 ms | 41.67 k/s | |

---

### 36.17 Lock-Contention (`bench_lock_contention`)

> Run 20251223_085556 (v1.3.3)

| Benchmark | Threads | real_time | ops/s |
|-----------|---------|-----------|-------|
| BM_LockContention_Disjoint | 1 | 4.44 ms | 14.4 k/s |
| BM_LockContention_Disjoint | 4 | 13.5 ms | 18.9 k/s |
| BM_LockContention_Disjoint | 8 | 8.31 ms | 61.6 k/s |
| BM_LockContention_Disjoint | 16 | 66.7 ms | 15.3 k/s ÔÜá´©Å |
| BM_LockContention_Disjoint | 32 | 42.9 ms | 47.8 k/s |
| BM_LockContention_Overlapping | 1 | 14.4 ms | 4.43 k/s |

---

### 36.18 Batch-Insert (`bench_batch_insert`)

> v1.3.4 (Run 20251229_184507)

| Benchmark | real_time | ops/s | Anmerkung |
|-----------|-----------|-------|-----------|
| BatchInsertBenchmark/SingleInserts_100 | 432.9 ms | 533 ops/s | einzelne Inserts |
| BatchInsertBenchmark/BatchInsert_100 | 10.51 ms | 136 ops/s | Batch API |
| BatchInsertBenchmark/SingleInserts_1000 | 15.85 s | 4.571 k/s | |
| BatchInsertBenchmark/BatchInsert_1000 | 277.1 ms | 372 ops/s | |

> **Beobachtung:** Batch-API ist hier **langsamer** als Single-Inserts in Items/s ÔÇö deutet auf Overhead im Batch-Koordinator hin. Bekannte Optimierungsl├╝cke (vgl. ┬º34 D-5).

---

### 36.19 Compression-Benchmark (`bench_compression`)

> Run 20251223_085556 (v1.3.3)

| Benchmark | Blockgr├Â├ƒe | Kompression | real_time | ops/s |
|-----------|-----------|-------------|-----------|-------|
| CompressionFixture/SequentialWrite/Keine/512B | 512 B | ÔÇô | 25.2 ms | 48.0 k/s |
| CompressionFixture/SequentialWrite/LZ4/512B | 512 B | LZ4 | 25.9 ms | 41.8 k/s |
| CompressionFixture/SequentialWrite/Zstd/512B | 512 B | Zstd | 26.2 ms | 42.7 k/s |
| CompressionFixture/SequentialWrite/Keine/4096B | 4096 B | ÔÇô | 33.3 ms | 35.2 k/s |
| CompressionFixture/SequentialWrite/LZ4/4096B | 4096 B | LZ4 | 32.9 ms | 34.7 k/s |
| CompressionFixture/SequentialWrite/Zstd/4096B | 4096 B | Zstd | 32.6 ms | 34.5 k/s |

---

### 36.20 Zusammenfassung: Regression-├£bersicht v1.3.0 ÔåÆ v1.3.4

| Benchmark | v1.3.0 | v1.3.4 | ╬ö | Schwere |
|-----------|--------|--------|---|---------|
| VectorIndexBench/InsertPlaintext | 566.7 k/s | 351.4 k/s | **ÔêÆ38 %** | ÔØî Kritisch |
| SecondaryIndexBench/IndexInsert | 1.78 M/s | 217.2 k/s | **ÔêÆ88 %** | ÔØî Kritisch |
| QueryEngineBench/SimpleEvaluation | 968.6 M/s | 814.5 M/s | ÔêÆ16 % | ÔÜá´©Å Mittel |
| GraphIndexBench/AddEdges | 1.47 M/s | 628.7 k/s | **ÔêÆ57 %** | ÔØî Kritisch |
| TimeseriesBench/InsertTimepoints | 61.0 M/s | 49.0 M/s | **ÔêÆ20 %** | ÔÜá´©Å Mittel |
| BM_Encrypt_String_UsingKey/1024 | 254.9 k/s | 191.2 k/s | ÔêÆ25 % | ÔÜá´©Å Mittel |
| BM_Decrypt_String_UsingKey/256 | 60.1 k/s | 41.1 k/s | ÔêÆ32 % | ÔØî Hoch |
| BM_CPUBackend_DistanceComputation | 11.24 M/s | 10.24 M/s | ÔêÆ9 % | ÔÜá´©Å Gering |
| BM_ContentGeo_CPP_API | 5.59 k/s | 7.19 k/s | **+29 %** | Ô£à Verbesserung |
| BM_ContentGeo_AQL_Sugar | 5.56 k/s | 6.13 k/s | **+10 %** | Ô£à Verbesserung |
| EmbeddingCache_Query_Hit/384 | ÔÇô | 155.8 M/s | n/a (neu) | Ô£à Neu |
| 2PC-Throughput (2 Shards) | ÔÇô | 6.4 k/s | n/a (neu) | Ô£à Neu |

> **Wichtige Relativierung:** Mehrere Regressionen (insb. SecondaryIndex, VectorIndex, Graph) sind auf ge├ñnderte Test-Infrastruktur zur├╝ckzuf├╝hren (per-test temp dirs, einzelne RocksDB-Transaktionen pro `put()`), nicht auf Produktions-Regressions ÔÇö vgl. `PERFORMANCE_COMPARISON_V1.3.0_VS_V1.3.3.md`.


---

### 36.21 Lokale Vergleichsmessung (CMake, v1.8.1-rc2)

> Lauf: `benchmarks/results/local_20260409_093136/` (Windows, msvc-ninja-release, Google Benchmark `_mean`).

#### 36.21.1 Graph Query Optimizer (`bench_graph_query_optimizer`)

| Benchmark | v1.8.1-rc2 real_time | v1.8.1-rc2 items/s |
|-----------|----------------------|--------------------|
| PlanGeneration_ShortestPath/100 | 223 ns | 4.53361 M/s |
| PlanGeneration_KHopNeighborhood/100 | 246 ns | 4.03036 M/s |
| PlanGeneration_WithCache/100 | 225 ns | 4.53361 M/s |
| BFS_Execution/100/2 | 3214 ns | 321.128 k/s |
| BFS_Execution/100/3 | 6038 ns | 160.89 k/s |
| BFS_Execution/100/4 | 13241 ns | 77.037 k/s |

#### 36.21.2 Storage Performance (`bench_storage_performance`)

| Benchmark | v1.8.1-rc2 real_time | v1.8.1-rc2 Throughput |
|-----------|----------------------|-----------------------|
| BM_Allocator_System_Small | 64510 ns | 15.4953 M items/s |
| BM_Allocator_Themis_Small | 7703 ns | 128.493 M items/s |
| BM_Allocator_System_Large | 843667 ns | 119.2 k items/s |
| BM_Allocator_Themis_Large | 41168 ns | 2.49111 M items/s |
| BM_Allocator_Mixed | 52529 ns | 19.6267 M items/s |
| BM_RCU_Read_SingleThread | 109 ns | 919.77 M items/s |

#### 36.21.3 Vector Search (`bench_vector_search`)

| Benchmark | v1.8.1-rc2 real_time |
|-----------|----------------------|
| BM_VectorSearch_efSearch/32/10 | 12.3 ms |
| BM_VectorSearch_efSearch/64/10 | 14.5 ms |
| BM_VectorSearch_efSearch/128/10 | 13.4 ms |
| BM_VectorSearch_efSearch/256/10 | 14.0 ms |
| BM_VectorInsert_Batch100/64 | 4.08 ms |
| BM_VectorInsert_Batch100/128 | 23.2 ms |

> Hinweis: `BM_VectorInsert_Batch100/*` zeigt hohe Varianz (CV bis 139.79 %), daher als vorlaeufige Vergleichswerte behandeln.

#### 36.21.4 Metrics Collector (`bench_metrics_collector`)

| Benchmark | v1.8.1-rc2 real_time | v1.8.1-rc2 items/s |
|-----------|----------------------|--------------------|
| BM_RecordQuery | 2905 ns | 345.126 k/s |
| BM_RecordCacheHit | 963 ns | 1.0276 M/s |
| BM_RecordTSStoreWrite | 3928 ns | 261.692 k/s |
| BM_RecordShardLatency | 1193 ns | 830.39 k/s |
| BM_MixedMetrics | 2368 ns | 415.192 k/s |
| BM_HighVolumeRecording/1000 | 3012030 ns | 333.333 k/s |

> Hinweis: In den lokalen Runs ist der Metrics-Collector gegenueber den historischen v1.3.x-Werten tendenziell langsamer; Ursachenanalyse folgt in separatem Profiling-Run.

#### 36.21.5 Delta lokal_082951 -> lokal_093136 (Auszug)

| KPI (mean) | lokal_082951 | lokal_093136 | Delta |
|-----------|--------------|--------------|-------|
| PlanGeneration_ShortestPath/100 (ns) | 253 | 223 | +11.9 % schneller |
| BFS_Execution/100/2 (ns) | 3725 | 3214 | +13.7 % schneller |
| BM_RecordCacheHit (ns) | 824 | 963 | -16.9 % langsamer |
| BM_RecordQuery (ns) | 2363 | 2905 | -22.9 % langsamer |
| BM_Allocator_Themis_Small (ns) | 6283 | 7703 | -22.6 % langsamer |
| BM_RCU_Read_SingleThread (ns) | 96.9 | 109 | -12.5 % langsamer |
| BM_VectorSearch_efSearch/128/10 (ms) | 10.6 | 13.4 | -26.4 % langsamer |
| BM_VectorInsert_Batch100/64 (ms) | 4.39 | 4.08 | +7.1 % schneller |

> Quelle Delta: Vergleich der `_mean`-Zeilen aus `benchmarks/results/local_20260409_082951/*.txt` und `benchmarks/results/local_20260409_093136/*.txt`.

#### 36.21.6 Profiling-Plan fuer regressionsauffaellige KPIs

| Schritt | Ziel | Befehl/Setup | Erfolgskriterium |
|--------|------|--------------|------------------|
| 1 | Noise reduzieren (mehr Repetitions) | Benchmarks mit `--benchmark_min_time=0.3s --benchmark_repetitions=10` wiederholen | CV bei Kern-KPIs < 10 % |
| 2 | CPU-Frequenz/Thread-Einfluss isolieren | Vergleich 1 Thread vs. Standard-Threading pro betroffenen Benchmark | Delta zwischen Runs < 5 % bei stabilen KPIs |
| 3 | Vector-Insert-Ausreisser lokalisieren | `bench_vector_search` separat 3x ausfuehren, nur `BM_VectorInsert_Batch100/*` auswerten | Ausreisser reproduzierbar oder eliminierbar |
| 4 | Metrics-Hotpath aufteilen | `bench_metrics_collector` fokussiert auf `BM_RecordQuery` und `BM_RecordCacheHit` | Identifizierter dominanter Teilpfad (record vs. export/lock) |
| 5 | Allocator-Einfluss pruefen | `bench_storage_performance` mit identischer Build-Config erneut, Fokus `BM_Allocator_*` und `BM_RCU_Read_SingleThread` | Abweichung zu vorherigem Lauf erklaert (Config/Noise/Regressionskandidat) |

> Empfohlene Priorisierung: zuerst Schritt 1 und 3 (hohe Varianz), danach Schritt 4 und 5 (konstant negative Deltas).

**Kopierfertige PowerShell-Kommandos (lokales Profiling):**

```powershell
$ErrorActionPreference = 'Stop'

$benchDir = 'C:\VCC\themis\build-msvc-ninja-release\cmake\benchmarks'
$binDir = 'C:\VCC\themis\build-msvc-ninja-release\bin'
$ts = Get-Date -Format 'yyyyMMdd_HHmmss'
$outDir = "C:\VCC\themis\benchmarks\results\profiling_$ts"

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$env:PATH = "$binDir;$benchDir;" + $env:PATH
Set-Location $benchDir

# Schritt 1: Noise reduzieren
$commonArgs = @(
	'--benchmark_min_time=0.3s',
	'--benchmark_repetitions=10',
	'--benchmark_report_aggregates_only=true'
)

foreach ($b in @('bench_vector_search','bench_metrics_collector','bench_storage_performance')) {
	& ".\\$b.exe" @commonArgs --benchmark_out="$outDir\\${b}_noise.json" --benchmark_out_format=json `
		| Tee-Object -FilePath "$outDir\\${b}_noise.txt"
}

# Schritt 3: Vector-Insert-Ausreisser (3 Wiederholungen)
for ($i = 1; $i -le 3; $i++) {
	& '.\\bench_vector_search.exe' @commonArgs --benchmark_filter='BM_VectorInsert_Batch100/.*' `
		--benchmark_out="$outDir\\bench_vector_insert_run$i.json" --benchmark_out_format=json `
		| Tee-Object -FilePath "$outDir\\bench_vector_insert_run$i.txt"
}

# Schritt 4: Metrics Hotpath fokussieren
& '.\\bench_metrics_collector.exe' @commonArgs --benchmark_filter='BM_Record(Query|CacheHit).*' `
	--benchmark_out="$outDir\\bench_metrics_hotpath.json" --benchmark_out_format=json `
	| Tee-Object -FilePath "$outDir\\bench_metrics_hotpath.txt"

# Schritt 5: Allocator/RCU fokussieren
& '.\\bench_storage_performance.exe' @commonArgs --benchmark_filter='BM_(Allocator_.*|RCU_Read_SingleThread).*' `
	--benchmark_out="$outDir\\bench_storage_allocator_rcu.json" --benchmark_out_format=json `
	| Tee-Object -FilePath "$outDir\\bench_storage_allocator_rcu.txt"

Write-Host "Profiling-Ergebnisse: $outDir"
```


---

### 36.22 Fortgeschriebener Benchmark-Run (2026-04-09)

> Quelle: `logs/bench_run_20260409_221029/`
> Hinweis: Lauf auf CPU-Only-Umgebung; mehrere GPU-abhaengige Cases liefern erwartbar keinen Messwert.

#### 36.22.1 Gemessene Cases

| Benchmark | Time | CPU | Iterations | Zusatzmetrik | Status |
|---|---:|---:|---:|---|---|
| GraphTraversalBenchmarkFixture/BFSTraversal/1000/4 | 2.60 ms | 2.37 ms | 33 | `nodes_per_sec=422.4k/s`, `items_per_second=422.4/s` | Ô£à |
| ConfigPathResolverBenchFixture/CacheHit_MappedPath | 57,542 ns | 57,199 ns | 11.200 | `cache_hit_rate=99.9188`, Ziel laut Counter `target < 1 us` | ÔØî |

#### 36.22.2 Nicht auswertbare Cases in diesem Lauf

| Benchmark | Ergebnis | Grund | Status |
|---|---|---|---|
| BM_BatchLoading_Throughput/8/128 (`bench_data_transfer`) | `ERROR OCCURRED: CUDA not available` | CUDA/GPU in der Umgebung nicht verfuegbar | ÔØô |
| BM_DataLoader_WithPrefetch/1/8 (`bench_data_transfer`) | `ERROR OCCURRED: CUDA not available` | CUDA/GPU in der Umgebung nicht verfuegbar | ÔØô |
| BM_Cache_HitMiss_Pattern/0 (`bench_data_transfer`) | `ERROR OCCURRED: CUDA not available` | Benchmark ist in diesem Build ebenfalls GPU-gebunden | ÔØô |
| BM_Training_Batch_4x16 (`bench_lora_framework`) | kein valider Zahlen-Output | Basismodell fehlt: `models/default.gguf`; Folge-Warnungen `Training already in progress` | ÔØô |

#### 36.22.3 Kurzbewertung

| Bereich | Bewertung |
|---|---|
| Graph Traversal | Solider CPU-Smoketest mit reproduzierbarem Durchsatz (`422.4k nodes/s`). |
| Config Path Resolver | Hit-Rate sehr gut, aber Latenz klar ueber dem ausgewiesenen Ziel `< 1 us`. |
| Data Transfer | In dieser Umgebung nicht benchmarkbar, da Cases GPU/CUDA voraussetzen. |
| LoRA Framework | Ohne GGUF-Modellartefakt aktuell nur Integrationscheck, kein Performance-Run. |

#### 36.22.4 Empfohlene naechste Messung (fortgeschrieben)

| Prioritaet | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | GPU-Runner verwenden (CUDA/HIP) fuer `bench_data_transfer` | verwertbare Throughput- und Transfer-Latenzwerte |
| 2 | `models/default.gguf` bereitstellen und `bench_lora_framework` erneut laufen lassen | numerische LoRA-Trainingsmetriken statt Fehlerlog |
| 3 | ConfigPathResolver Hot-Path profilen (Locking/String-Normalisierung/Cache-Lookup) | Reduktion der Cache-Hit-Latenz in Richtung `< 1 us` |

---

## 37. Durchgef├╝hrte Performance-Ma├ƒnahmen (mit GitHub-PR)

> Chronologisch absteigend (neueste zuerst). Alle PRs liegen auf dem `develop`-Branch.
> Links: `https://github.com/makr-code/ThemisDB/pull/<Nr>`

---

### 37.1 v1.9.0 ÔÇô Aktuelle Ma├ƒnahmen

| # | Ma├ƒnahme | Modul | PR | Version | Messbare Wirkung |
|---|----------|-------|----|---------|-----------------|
| 1 | **Batch-Prediction, O(1)-Update, Parallel-Auto-Tune, FNV-1a Fit-Cache** ÔÇö `predictBatch()` f├╝r N Serien, inkrementelles ETS/ARIMA/LR-Update, 9 parallele `std::async`-Auto-Tune-Tasks | Analytics / Forecasting | [#4054 (Issue)](https://github.com/makr-code/ThemisDB/issues/4054) | v1.9.0 | Auto-Tune: 9├ù Parallelisierung; Fit-Cache: wiederholte Serien O(1) statt O(n) |
| 2 | **QueryCompiler JIT Hot-Path** ÔÇö JIT-kompilierte Ausf├╝hrungspfade in `executeAql()` verdrahtet, vectorized-execution-Tests registriert | Query | [#4398](https://github.com/makr-code/ThemisDB/pull/4398) | v1.9.0 | AQL Hot-Path: JIT-Pfad aktiv |
| 3 | **Cache Warmup-Logik** ÔÇö `warmupFromLog` max_entries-Grenze korrekt durchgesetzt, Snippet-Boundary-Alignment verbessert | Cache | (direct commit `64a9ae4`) | v1.9.0 | Weniger Overfetch bei Warmup |
| 4 | **AdaLoRA + Multi-Adapter** ÔÇö Importance-basiertes Rank-Pruning, `LoRAAdapterMerger` mit TIES-Merging und Power-Iteration-SVD | Training | [#4405](https://github.com/makr-code/ThemisDB/pull/4405) | v1.9.0 | LoRA Memory-Footprint reduziert, Merge ohne separaten Checkpoint |
| 5 | **DiskANN / MRL-Truncation** ÔÇö Matryoshka Representation Learning f├╝r mehrstufige ANN-Retrieval-Pipeline | Index | [#4399](https://github.com/makr-code/ThemisDB/pull/4399) | v1.9.0 | Ersten Stage mit 64-dim statt 1536-dim ÔåÆ ÔëÑ10├ù weniger FLOPS in Stage 1 |

---

### 37.2 v1.8.0 ÔÇô Ma├ƒnahmen

| # | Ma├ƒnahme | Modul | PR | Version | Messbare Wirkung |
|---|----------|-------|----|---------|-----------------|
| 6 | **SIMD-Vektorisierung AVX-512 + ARM NEON** ÔÇö Aggregations- und Distanz-Kernels mit AVX-512-Intrinsics, ARM NEON-Fallback; CPUID-Check gecacht (static const) | Analytics | [#4317](https://github.com/makr-code/ThemisDB/pull/4317) | v1.8.0 | Benchmark-Ziel: ÔëÑ4 GB/s auf Cortex-A78; AVX-512 check: O(1) statt O(n) |
| 7 | **Predictive Prefetcher (ML-basiertes Zugriffsmuster-Modell)** ÔÇö Erkennt wiederkehrende Zugriffsmuster und l├Âst Prefetch vor dem Cache-Miss aus | Cache / Performance | [#4293](https://github.com/makr-code/ThemisDB/pull/4293) | v1.8.0 | Ziel: Cache-Miss-Rate ÔêÆ20 % bei sequenziellen Workloads |
| 8 | **Intelligent Prefetching System** ÔÇö Zweite Prefetch-Schicht mit konfigurierbarem Lookahead, adaptive Prefetch-Tiefe | Performance | [#4257](https://github.com/makr-code/ThemisDB/pull/4257) | v1.8.0 | Ziel: Prefetch-Overfetch Ôëñ10 % |
| 9 | **Query Compilation & JIT** ÔÇö `AdaptiveQueryCompiler` mit JIT-Codegen-Pfad, Expressions zu nativer Code kompiliert | Query | [#4246](https://github.com/makr-code/ThemisDB/pull/4246) | v1.8.0 | Ziel: AQL-Parse+Execute P99 Ôëñ 2 ms; JIT-Erstcompilierung Ôëñ 50 ms |
| 10 | **Parallel Query Execution (Intra-Query)** ÔÇö Parallele Ausf├╝hrung unabh├ñngiger Query-Teilpl├ñne via Thread-Pool | Query | [#4211](https://github.com/makr-code/ThemisDB/pull/4211) | v1.7.0 | Ziel: multi-core Skalierung f├╝r OLAP-Queries |
| 11 | **Parallel `translateBatchNLToAQL()`** ÔÇö Bounded-Worker-Pool + `std::async`-Semaphor-Throttle f├╝r NLÔåÆAQL-Batch-├£bersetzungen | AQL | [#4221](https://github.com/makr-code/ThemisDB/pull/4221) | v1.7.0 | Batch-Throughput proportional zu Worker-Count |
| 12 | **Write-Optimized Merge (WOM) Tree** ÔÇö LSM-Tree-Optimierungen: Delayed Compaction, Tiered-Merge-Policy, Write-Stall-Pr├ñvention | Storage | [#4204](https://github.com/makr-code/ThemisDB/pull/4204) | v1.8.0 | Ziel: Write-Amplification <1.5├ù; WAL OFF: 507 k ops/s @ 8 Threads |
| 13 | **Write Batching & Coalescing** ÔÇö Transaktions-Batcher mit konfigurierbarem Fenster 1ÔÇô100 ms, adaptive Batch-Gr├Â├ƒe | Transaction | [#4335](https://github.com/makr-code/ThemisDB/pull/4335) | v1.8.0 | Konfigurierbar 1ÔÇô100 ms Batch-Fenster; adaptive ┬▒10 % |
| 14 | **Optimistic Concurrency Control (OCC)** ÔÇö Conflict-Detection-Phase nach Lese-Phase, Retry-Backoff, Deadlock-Watchdog | Transaction | [#4264](https://github.com/makr-code/ThemisDB/pull/4264) | v1.8.0 | OCC Commit P50: 100 ┬Ás, P99: 5 ms; Deadlock-Overhead: 1 % |
| 15 | **Index-Kompression** ÔÇö Delta-, Prefix-, RLE-, Dictionary-, Bloom-Filter-Encoding f├╝r B-Tree/sekund├ñre Indizes | Index | [#4226](https://github.com/makr-code/ThemisDB/pull/4226) | v1.7.0 | Index-Gr├Â├ƒe ÔêÆ40ÔÇô60 % (dokumentiert); Lookup-Latenz unver├ñndert |
| 16 | **Cache Warmup Parallel Bulk-Load** ÔÇö `warmupParallelBulkLoad()` mit konfigurierbaren Worker-Threads | Cache | [#4250](https://github.com/makr-code/ThemisDB/pull/4250) | v1.8.0 | Warmup-Throughput: Ziel ÔëÑ500 k Entries/s |
| 17 | **zlib ÔåÆ ZSTD Migration** ÔÇö StreamWriter-Kompression vollst├ñndig auf ZSTD Level 3 umgestellt | Exporters | [#4252](https://github.com/makr-code/ThemisDB/pull/4252) | v1.8.0 | ZSTD: ÔêÆ30ÔÇô50 % Datenvolumen vs. zlib bei vergleichbarer Latenz |
| 18 | **Wire Protocol Performance** ÔÇö TCP-Framing optimiert, Zero-Copy-Payload-Transfer, Keep-Alive-Pooling | Network | [#4214](https://github.com/makr-code/ThemisDB/pull/4214) | v1.7.0 | Ziel: ÔëÑ100 k req/s/Core (128 B, kein TLS) |
| 19 | **Arrow Zero-Copy IPC + OLAP LRU-Cache** ÔÇö Apache Arrow Record-Batch f├╝r spaltenweisen Zero-Copy-Transfer; OLAP-Ergebnis-Cache mit TTL und LRU-Eviction | Analytics | [#4328](https://github.com/makr-code/ThemisDB/pull/4328) | v1.8.0 | Zero-Copy: kein Memcpy bei OLAP-Ausgabe; LRU: Wiederholte Queries aus Cache |
| 20 | **Memory Pool Allocator (Hot Analytics)** ÔÇö `slab`-basierter Pool f├╝r kurzzeitige Analytics-Allocations auf kritischen Pfaden | Analytics | [#4311](https://github.com/makr-code/ThemisDB/pull/4311) | v1.8.0 | Reduziert Allocator-Contention auf Hot-Paths; jemalloc-freundlich |
| 21 | **SAGA Orchestrator (DAG-Parallelausf├╝hrung)** ÔÇö Parallele Kompensations-Ausf├╝hrung via topologisch sortiertem DAG | Transaction | [#4305](https://github.com/makr-code/ThemisDB/pull/4305) | v1.8.0 | SAGA Compensation Time: 20 ms Ziel; parallelisierte Steps |
| 22 | **Read-Only Transaction Optimization** ÔÇö Skip-Lock-Pfad f├╝r reine Lese-Transaktionen, kein Snapshot-Overhead | Transaction | (direct commit `d5eddfb`) | v1.8.0 | Lese-Transaktionen: kein 2PC-Overhead |
| 23 | **SLO Monitor Latency Percentile Tracking** ÔÇö P50/P95/P99-Histogramm mit konfigurierbaren Schwellwert-Alerts | Cache / Observability | [#4329](https://github.com/makr-code/ThemisDB/pull/4329) | v1.8.0 | Echtzeit-Regression-Erkennung; CI-Gate blockiert bei P99 >20 % ├╝ber Baseline |
| 24 | **DiffEngine::computeDiff() + Cache-Stampede-Fix** ÔÇö O(N)-Changefeed-Scan durch Diff-Cache ersetzt; Cache-Stampede durch Single-Fetch-Lock | Analytics / Cache | [#4325](https://github.com/makr-code/ThemisDB/pull/4325) | v1.8.0 | Changefeed-Scan: O(N) ÔåÆ O(1) f├╝r gecachte Diffs |
| 25 | **Perceptual Hashing Deduplication** ÔÇö pHash-basierte Bild-Deduplizierung mit Hamming-Distance-Index | Content | [#4331](https://github.com/makr-code/ThemisDB/pull/4331) | v1.8.0 | Speichereinsparung durch Dedup; kein Re-Embedding f├╝r Duplikate |
| 26 | **CUDA k>kMaxK Silent-Clamping entfernt** ÔÇö `kMaxK` auf 1024 erh├Âht mit dynamischem Shared Memory; kein silentes Trunkieren mehr | Acceleration | [#4320](https://github.com/makr-code/ThemisDB/pull/4320) | v1.8.0 | CUDA Shared Memory: Ôëñ32 KB bei k=1024 laut Ziel-Spec |
| 27 | **VLLMResourceManager Multi-GPU NVML-Monitoring** ÔÇö Per-GPU Memory/Utilization-Monitoring via NVML; CPU-Snapshot-Cache 200 ms TTL | Acceleration | [#4318](https://github.com/makr-code/ThemisDB/pull/4318) | v1.8.0 | getStats()-Latenz: <2 ms (gecacht) statt NVML-Call auf Hot-Path |
| 28 | **BackendRegistry Thread-Safe Read-Access** ÔÇö Dedizierter Read-Lock-Pfad ohne Writer-Contention | Acceleration | [#4321](https://github.com/makr-code/ThemisDB/pull/4321) | v1.8.0 | Concurrent Registry-Lookups ohne Mutex-Bottleneck |
| 29 | **LLMProcessAnalyzer O(1) LRU-Cache-Eviction unter Lock** ÔÇö `std::list`-basierter LRU statt O(N)-Scan | LLM | [#4322](https://github.com/makr-code/ThemisDB/pull/4322) | v1.8.0 | Eviction: O(N) ÔåÆ O(1) |
| 30 | **LoRA Adapter Hot-Loading** ÔÇö Adapter laden ohne Neustart; `unique_lock` f├╝r thread-sicheres Hot-Swap | LLM / Training | [#4333](https://github.com/makr-code/ThemisDB/pull/4333) | v1.8.0 | Ziel: Ôëñ5 s Wall-Clock f├╝r 7B-Modell, Rank 64, 16-bit |
| 31 | **Logical Replication Parallel Decoding** ÔÇö WAL-Decoder mit parallelisierten Decode-Threads | Replication | (direct commit `02ecdca`) | v1.8.0 | Replication WAL-Shipping Throughput-Ziel: ÔëÑ500 MB/s/Follower |
| 32 | **Distributed Analytics Sharding ÔÇô gecachter Health-State** ÔÇö `getHealthyShardCount()` ohne Network-I/O unter Lock | Sharding | [#4324](https://github.com/makr-code/ThemisDB/pull/4324) | v1.8.0 | Shard-Health-Lookup: O(1) aus Cache statt synchroner RPC |
| 33 | **Lock-Free L1 Cache Read-Path** ÔÇö Migration L1-Lese-Pfad auf `std::atomic` ohne Mutex | Cache | (direct commit `a95475d`) | v1.8.0 | L1 Read Hot-Path: mutex-frei ÔåÆ Ziel ÔëÑ5 M ops/s/Core |
| 34 | **Geo DBSCAN / k-Means GPU** ÔÇö DBSCAN und k-Means mit GPU-Beschleunigung f├╝r gro├ƒe Punkt-Mengen | Geo | [#4298](https://github.com/makr-code/ThemisDB/pull/4298) | v1.8.0 | DBSCAN GPU Speedup: >100├ù vs. CPU (100K Punkte) |
| 35 | **Distributed Ingestion Coordinator** ÔÇö Mehrstufige Ingestion-Pipeline mit Retry-Quarant├ñne und parallelen S3-Downloads | Ingestion | [#4309](https://github.com/makr-code/ThemisDB/pull/4309) | v1.8.0 | S3 concurrent: ÔëÑ200 MB/s agg. (4 parallel, 10 Gbps) |
| 36 | **Incremental View Lock-Free Apply** ÔÇö `applyChanges()` ohne globalen Write-Lock f├╝r inkrementelle Materialized-View-Updates | Analytics | [#4316](https://github.com/makr-code/ThemisDB/pull/4316) | v1.8.0 | IVM Delta-Application: Ôëñ50 ms (10k Rows) |
| 37 | **StreamingWindow konfigurierbare Expiry-Poll-Intervalle** ÔÇö Kein Busy-Wait; konfigurierbare Sleep-Dauer f├╝r Expiry-Worker | Analytics | [#4327](https://github.com/makr-code/ThemisDB/pull/4327) | v1.8.0 | CPU-Idle beim Streaming-Worker signifikant reduziert |

---

### 37.3 v1.7.0 ÔÇô Ma├ƒnahmen

| # | Ma├ƒnahme | Modul | PR | Version | Messbare Wirkung |
|---|----------|-------|----|---------|-----------------|
| 38 | **CUDA ANN-Kernel-Vollimplementierung** ÔÇö Fused-Cosine-Kernel + Shared-Memory Top-K-Helper; HIP/RCCL `mergeTopK` f├╝r Multi-GPU | Acceleration | [#4193](https://github.com/makr-code/ThemisDB/pull/4193) | v1.7.0 | mergeTopK <500 ┬Ás (worldSize=4, k=100, NVLink-3) |
| 39 | **GPU Hardware Support Gaps** ÔÇö HIP Top-K-Heap, CUDA HNSW Bitset, NCCL/RCCL `mergeTopK` | Acceleration | (direct commit `73d8f8a`) | v1.7.0 | Bitset-Optimierung: 8├ù Memory-Reduktion (5 GB ÔåÆ 640 MB) |
| 40 | **TSStore Single-Point Insert Buffering (Gorilla)** ÔÇö In-Memory-Buffer vor Gorilla-Kompressionsflush; kein WAL-Write per Punkt | Timeseries | (direct commit `822b0af`) | v1.7.0 | Ziel: >500 k pts/s (von ~200 k pts/s); Buffer-to-Storage Flush P99 <10 ms |
| 41 | **AdaptiveQueryCompiler Audit-Gaps** ÔÇö L├╝cken in Compiler-Pipeline geschlossen (Issue #86) | Query | (direct commit `2efe683`) | v1.7.0 | Compiler-Regression-Gate: Ôëñ5 % |
| 42 | **HardwareAccelerator v1.8.0** ÔÇö CPU-affinity-basierte NUMA-Zuweisung, GPU-Backend-Selection | Performance | (direct commit `139f96c`) | v1.7.0 | NUMA-lokale Allokation; reduzierten Cross-Socket-Traffic |

---

### 37.4 v1.6.0 und fr├╝her

| # | Ma├ƒnahme | Modul | PR | Version | Messbare Wirkung |
|---|----------|-------|----|---------|-----------------|
| 43 | **GPU-Acceleration Multi-Tenancy** ÔÇö Erste GPU-Backend-Integration, CUDA-Kernel-Grundger├╝st | Acceleration | [#44](https://github.com/makr-code/ThemisDB/pull/44) | fr├╝h | GPU-Backend-Grundlage |
| 44 | **Hardware Acceleration Support** ÔÇö CPU AVX2-Baseline, erste Vektoroperationen | Acceleration | [#30](https://github.com/makr-code/ThemisDB/pull/30) | fr├╝h | CPU AVX2-Baseline f├╝r Benchmarks |
| 45 | **Benchmark-Datenbank-Tests** ÔÇö Erste Google-Benchmark-Targets, Baseline f├╝r sp├ñtere Regression-Tests | Benchmarks | [#54](https://github.com/makr-code/ThemisDB/pull/54) | fr├╝h | Benchmark-Infrastruktur aufgebaut |
| 46 | **Benchmarks-Repository-Erweiterung** ÔÇö Neue Bench-Targets f├╝r Vektor-, Timeseries-, Graph-Operationen | Benchmarks | [#33](https://github.com/makr-code/ThemisDB/pull/33) | fr├╝h | Benchmark-Coverage auf 9 Module erweitert |
| 47 | **Lossless Compression-Methoden (Research)** ÔÇö Evaluierung LZ4 vs. Zstd vs. Snappy ÔåÆ Entscheidung f├╝r Zstd | Storage | [#70](https://github.com/makr-code/ThemisDB/pull/70) | fr├╝h | Grundlage f├╝r PR #4252 (Zstd-Migration) |
| 48 | **OpenCL Erasure Coder** ÔÇö GF(2^8)-Arithmetik-basiertes Reed-Solomon Encode/Decode/BatchEncode | Sharding | (direct commit `dc202ef`) | v1.7.0 | GPU Reed-Solomon: >4 GB/s Ziel (NVIDIA A10) |

---

### 37.5 Offene / Geplante Performance-Ma├ƒnahmen (noch nicht umgesetzt)

| # | Geplante Ma├ƒnahme | Modul | Ziel-Metrik | Ziel-Version |
|---|-------------------|-------|-------------|--------------|
| P-1 | **Gorilla Decode AVX-optimierung** ÔÇö SIMD-Decode-Pfad f├╝r Gorilla-Kompression | Timeseries | >2 GB/s (von ~400 MB/s) | Q3 2026 |
| P-2 | **SecondaryIndex Batch-Transaktionen** ÔÇö Mehrere `put()`-Aufrufe in einer Transaktion b├╝ndeln | Index / Storage | 1.78 M/s wiederherstellen (von 217 k/s) | Q2 2026 |
| P-3 | **CUDA Geospatial Distanz-Kernels** ÔÇö WGS84-Haversine und Point-in-Polygon auf GPU | Geo | GPU Contains 1M Punkte <50 ms (A10G) | Q3 2026 |
| P-4 | **Vector Insert Throughput** ÔÇö HNSW-Build-Parallelisierung, Segment-basiertes Insert | Index | 600 k/s (FAISS-Parit├ñt) | Q3 2026 |
| P-5 | **1 MB Blob Write-Throughput** ÔÇö Async WAL + Background Flush | Storage | ÔëÑ100 k ops/s (von 741 ops/s) | Q2 2026 |
| P-6 | **Concurrent Concurrency-Stabilisierung** ÔÇö CV-Reduktion bei 10-Client-Lasttest | Storage | CV <5 % (von 20.74 %) | Q2 2026 |
| P-7 | **2PC Throughput-Steigerung** ÔÇö Pipelined 2PC (Phase 1+2 ├╝berlappend) | Transaction | 15 k/s (TiDB-Parit├ñt) | Q3 2026 |
| P-8 | **Query Engine vs. ClickHouse** ÔÇö Columnar SIMD Aggregation, Vectorized Scan | Query | 1.2 G items/s | Q4 2026 |
| P-9 | **TLS 1.3 Session Resumption** ÔÇö TLS-Session-Ticket-Cache | Network | <1 ms P99 | Q2 2026 |
| P-10 | **QUIC 0-RTT** ÔÇö QUIC-Transport f├╝r LAN-Kommunikation | Network | <2 ms P99 | Q3 2026 |


---

## 38. Weitere Rohdaten: HTTP-API-Benchmarks (v1.0.x, Dezember 2025)

> Quellen: `benchmarks/results_analysis_reports/scientific_benchmarks_20251204_212220/` und `docker_benchmarks_results_20251209_*/`  
> Plattform: Intel i9-10900K @ 3.70 GHz, 10 physische / 20 logische Cores, 31.3 GB RAM, Linux WSL2 5.15.167.4, Python 3.12 HTTP-Client, ThemisDB v1.0.0, endpoint http://localhost:8765

---

### 38.1 Wissenschaftliche Einzeloperation-Benchmarks (n=500, 5 Iterationen ├á 100 Ops)

> Messmethode: HTTP POST/GET gegen laufende ThemisDB-Instanz; 5 Warmup-Iterationen

| Test | avg (ms) | p50 (ms) | p95 (ms) | p99 (ms) | CV (%) | min (ms) | max (ms) |
|------|----------|----------|----------|----------|--------|----------|----------|
| **INSERT 1 KB** | 1.317 | 1.299 | 1.491 | 1.715 | 6.7 | 1.177 | 1.783 |
| **READ 1 KB** | 1.204 | 1.147 | 1.519 | 1.706 | 12.0 | 1.016 | 1.832 |
| **UPDATE 1 KB** | 1.240 | 1.219 | 1.386 | 1.603 | 6.8 | 1.103 | 1.761 |
| **INSERT 10 KB** | 1.960 | 1.922 | 2.284 | 2.378 | 6.6 | 1.813 | 2.378 |
| **INSERT 100 KB** | 7.913 | 7.889 | 8.847 | 9.369 | 6.5 | 7.075 | 9.369 |
| **INSERT 1 MB** | 61.402 | 60.954 | 65.923 | 68.178 | 2.6 | 60.007 | 68.178 |

> **Beobachtung:** 1 KB INSERT/READ/UPDATE zeigen stabiles Verhalten (CV ~7 %). 1 MB INSERT skaliert fast linear mit der Payload-Gr├Â├ƒe (├ù47 vs 1 KB). Kein Ausrei├ƒer-Verhalten bei Einzel-Clients.

---

### 38.2 Concurrent-Client-Benchmark (HTTP, je 5 Iterationen)

| Concurrent Clients | avg (ms) | p50 (ms) | CV (%) | Anmerkung |
|--------------------|----------|----------|--------|-----------|
| 1 | 1.281 | 1.275 | 1.1 | stabil, keine Contention |
| 5 | 6.800 | 6.742 | 2.5 | linear skalierend |
| 10 | 4.439 ÔÜá´©Å | 13.678 ÔÜá´©Å | 467 % ÔÜá´©Å | **Anomalie**: avg < p50, negative min ÔåÆ Messfehler |
| 25 | 35.464 | 35.754 | 4.1 | stabil, Serialisierungsoverhead |
| 50 | 60.317 | 69.439 | 38.1 % | hohe Varianz, Lock-Contention wahrscheinlich |

> ÔÜá´©Å **10-Client-Anomalie**: CV=467 %, min=-32 ms (Messfehler im HTTP-Timing). Reale Performance ca. 13ÔÇô14 ms p50. Dieser Befund korreliert mit dem bekannten CV >20 % bei 10-Client-Lasttest (┬º37.5 P-6).

---

### 38.3 Docker-Benchmark-Vergleich: ThemisDB vs. Competitors (v1.0.1, 09.12.2025)

> Methodik: Docker-Container, native Client-Bibliotheken, 155 Messpunkte ├╝ber 5 Workloads/Protokolle.  
> Avg-Werte gelten ├╝ber TCP+HTTP+gRPC sofern nicht anders angegeben.

#### Relational Workload (insert / read / update / delete / range_query)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **0.56** | 0.504 | 0.728 | 0.84 | **1786 ops/s** | 568 | 27.8 | Ô£à Schnellste |
| MySQL 8.0 | 0.80 | 0.720 | 1.040 | 1.20 | 1250 ops/s | 592 | 29.0 | +43 % langsamer |
| MariaDB 11 | 0.80 | 0.720 | 1.040 | 1.20 | 1250 ops/s | 592 | 29.0 | +43 % langsamer |
| PostgreSQL 16 | 0.96 | 0.864 | 1.248 | 1.44 | 1042 ops/s | 608 | 29.8 | +71 % langsamer |

> **Hinweis:** Die Latenz-├£berlegenheit (~1.7├ù) entstand nach Einf├╝hrung des direkten RocksDB-Pfads (kein SQL-Parser-Overhead). Gap-Analyse (v1.0.0) stellte noch 44ÔÇô49 % schlechtere Latenz gegen├╝ber PostgreSQL 16 fest ÔÇö nach Optimierungen nun umgekehrt.

#### Dokument-Store Workload (insert / read / update / bulk_insert)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **0.875** | 0.787 | 1.137 | 1.312 | **1143 ops/s** | 600 | 29.4 | Ô£à Schnellste |
| MongoDB | 1.625 | 1.463 | 2.113 | 2.438 | 615 ops/s | 675 | 33.1 | +86 % langsamer |
| CouchDB | 1.750 | 1.575 | 2.275 | 2.625 | 571 ops/s | 687 | 33.8 | +100 % langsamer |

> **Wichtige Gegenprobe** (benchmark_results_simple.json, 20251204): Python HTTP-Client gegen laufende Instanzen auf demselben Rechner ÔÇö dort zeigte ThemisDB **47.56 ms** f├╝r Document Insert (vs. MongoDB 0.87 ms). Diese Abweichung ist auf den HTTP-Overhead des Python-Client-Skripts zur├╝ckzuf├╝hren (unkompilierter Client vs. nativer Client). Die Docker-Messung mit nativem Client ist ma├ƒgeblich.

#### Vektor-Store Workload (search / index / recall / range_search)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **1.05** | 0.945 | 1.365 | 1.575 | **952 ops/s** | 617 | 30.2 | Ô£à Schnellste |
| Qdrant | 2.10 | 1.890 | 2.730 | 3.150 | 476 ops/s | 722 | 35.5 | +100 % langsamer |
| Milvus | 2.25 | 2.025 | 2.925 | 3.375 | 444 ops/s | 737 | 36.2 | +114 % langsamer |
| Weaviate | 2.70 | 2.430 | 3.510 | 4.050 | 370 ops/s | 782 | 38.5 | +157 % langsamer |

#### Graph-Workload (node_insert / edge_insert / traversal / shortest_path)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **1.75** | 1.575 | 2.275 | 2.625 | **571 ops/s** | 687 | 33.8 | Ô£à Schnellste |
| ArangoDB | 4.25 | 3.825 | 5.525 | 6.375 | 235 ops/s | 937 | 46.2 | +143 % langsamer |
| Neo4j | 5.00 | 4.500 | 6.500 | 7.500 | 200 ops/s | 1012 | 50.0 | +186 % langsamer |

#### Geo-Workload (point_insert / radius_search / polygon_search)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **1.312** | 1.181 | 1.706 | 1.969 | **762 ops/s** | 643 | 31.6 | Ô£à Schnellste |
| MongoDB | 2.438 | 2.194 | 3.169 | 3.656 | 410 ops/s | 756 | 37.2 | +86 % langsamer |
| PostgreSQL+PostGIS | 2.438 | 2.194 | 3.169 | 3.656 | 410 ops/s | 756 | 37.2 | +86 % langsamer |
| Elasticsearch | 3.000 | 2.700 | 3.900 | 4.500 | 333 ops/s | 812 | 40.0 | +129 % langsamer |

#### Hybrid-Workload (hybrid_search / multi_modal / polyglot_query)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % |
|-----------|----------|-----|-----|-----|------------|----------|-------|
| **ThemisDB** | **1.40** | 1.260 | 1.820 | 2.100 | **714 ops/s** | 652 | 32.0 |

---

### 38.4 Extended Hybrid-Query-Vergleich (native Clients, 50 Iterationen)

| Szenario | Datenbank | avg (ms) | p50 | p95 | p99 | Bewertung |
|----------|-----------|----------|-----|-----|-----|-----------|
| **Document + Graph** | PostgreSQL + Neo4j | 0.49 | 0.47 | 0.65 | 0.76 | Referenz |
| **Document + Graph** | **ThemisDB** | **0.88** | 0.83 | 1.21 | 1.37 | 1.8├ù langsamer ÔÜá´©Å |
| **Document + Vector** | MongoDB + Qdrant | 0.73 | 0.68 | 1.07 | 1.64 | Referenz |
| **Document + Vector** | **ThemisDB** | **0.88** | 0.81 | 1.31 | 1.40 | 1.2├ù langsamer |
| **OLAP + Document** | ClickHouse + MongoDB | 1.70 | 1.68 | 2.22 | 2.33 | Referenz |
| **OLAP + Document** | **ThemisDB** | **1.06** | 0.95 | 1.51 | 1.96 | Ô£à 1.6├ù schneller |

> ThemisDB schl├ñgt Spezialsysteme (ClickHouse+MongoDB) bei OLAP+Document um 38 %, liegt aber bei Document+Graph hinter dem PostgreSQL+Neo4j-Combo (kein ├£berraschung: kein Transaktionsoverhead zwischen zwei separaten DBs). **Ziel:** Document+Graph Ôëñ 0.6 ms avg (Q3 2026).

---

### 38.5 Acceleration-Modul Baseline (CPU ANN, Referenzwerte f├╝r Regression-Tests)

> Quelle: `benchmarks/baselines/acceleration/baseline.json` (Stand: 2026-01-01, CPU-Backend)

| Benchmark | Dims | n | items/s |
|-----------|------|---|---------|
| BM_CPU_ANN_L2Distance | 64 | 1000 | 2.00 M/s |
| BM_CPU_ANN_L2Distance | 128 | 1000 | 1.10 M/s |
| BM_CPU_ANN_L2Distance | 256 | 1000 | 590 k/s |
| BM_CPU_ANN_L2Distance | 512 | 1000 | 313 k/s |
| BM_CPU_ANN_CosineDistance | 64 | 1000 | 1.67 M/s |
| BM_CPU_ANN_CosineDistance | 128 | 1000 | 910 k/s |
| BM_CPU_ANN_CosineDistance | 256 | 1000 | 476 k/s |
| BM_CPU_ANN_CosineDistance | 512 | 1000 | 250 k/s |
| BM_CPU_ANN_InnerProduct | 64 | 1000 | 2.00 M/s |
| BM_CPU_ANN_InnerProduct | 128 | 1000 | 1.10 M/s |
| BM_CPU_ANN_InnerProduct | 256 | 1000 | 590 k/s |
| BM_CPU_ANN_InnerProduct | 512 | 1000 | 313 k/s |
| BM_CPU_ANN_TopK (k=10) | ÔÇô | 1000 | 20.0 M/s |
| BM_CPU_ANN_TopK (k=50) | ÔÇô | 1000 | 11.1 M/s |
| BM_CPU_ANN_TopK (k=10) | ÔÇô | 5000 | 25.0 M/s |
| BM_CPU_ANN_TopK (k=50) | ÔÇô | 5000 | 12.5 M/s |
| BM_CPU_BatchKNN (128d, k=10) | 128 | 1000 | 1.08 M/s |
| BM_CPU_BatchKNN (256d, k=10) | 256 | 1000 | 570 k/s |
| BM_CPU_BatchKNN (512d, k=10) | 512 | 1000 | 308 k/s |
| BM_CPU_Geo_HaversineDistance | ÔÇô | 1000 | 20.0 M/s |
| BM_CPU_Geo_HaversineDistance | ÔÇô | 10000 | 22.2 M/s |
| BM_CPU_Geo_HaversineDistance | ÔÇô | 100000 | 22.2 M/s |
| BM_CPU_Geo_PointInPolygon | ÔÇô | 1000 | 33.0 M/s |
| BM_CPU_Geo_PointInPolygon | ÔÇô | 10000 | 35.7 M/s |
| BM_CPU_Geo_PointInPolygon | ÔÇô | 100000 | 35.7 M/s |

---

### 38.6 Chimera-Modul Baseline (v1.5.0-dev, Stand: 2026-03-01)

> Quelle: `benchmarks/baselines/chimera/baseline.json`

| Workload | Throughput (ops/s) | avg (ms) | p95 (ms) | p99 (ms) |
|----------|--------------------|----------|----------|----------|
| relational_sort | 42.503 k/s | 0.024 | 0.023 | 0.034 |
| vector_dot_product | 75.835 k/s | 0.013 | 0.013 | 0.024 |
| document_lookup | **2.957 M/s** | 0.00018 | 0.0002 | 0.00025 |
| graph_bfs | 40.373 k/s | 0.025 | 0.025 | 0.033 |

---

### 38.7 Versions-Benchmark-Verlauf (`VERSION_HISTORY.csv`)

| Version | Datum | Query Engine (M items/s) | Vector Insert (k/s) | Index Insert (k/s) | Embedding Cache (items/s) | 2PC (ops/s) | Benchmark-Anzahl | Wichtigste ├änderung |
|---------|-------|--------------------------|---------------------|---------------------|---------------------------|-------------|-----------------|---------------------|
| v1.3.0 | 2025-09-15 | 700 | 280 | 180 | ÔÇô | ÔÇô | 450 | Initial Release |
| v1.3.1 | 2025-09-29 | 750 | 300 | 190 | ÔÇô | ÔÇô | 480 | Query Optimizer Improvements |
| v1.3.2 | 2025-10-31 | 800 | 330 | 210 | ÔÇô | ÔÇô | 520 | SIMD Vectorization + Compression |
| v1.3.3 | 2025-11-30 | 800 | 340 | 215 | ÔÇô | ÔÇô | 780 | Parallelization + Advanced Patterns |
| **v1.3.4** | 2025-12-29 | **814.5** | **351.4** | **217.2** | **155.8 M/s** | **6.4 k** | **1078** | Neu: Cache, 2PC, Hybrid Search |
| **v1.8.1-rc2 (lokal)** | 2026-04-09 | n/a (nicht im lokalen Lauf enthalten) | n/a (nur Search-Latenz lokal) | n/a | **1.0276 M/s** (`BM_RecordCacheHit_mean`) | n/a | **5** (`bench_storage`, `bench_vector`, `bench_graph_traversal`, `bench_graph_query_optimizer`, `bench_metrics`) | Lokaler CMake-Referenzlauf |


---

## 39. API- und Schnittstellen-Performance-Annahmen (aus `src/` extrahiert)

> Quellen: FUTURE_ENHANCEMENTS.md, ROADMAP.md, README.md der jeweiligen Module unter `src/`.  
> Typ-Legende: **[Z]** = Ziel/Target (noch nicht gemessen), **[M]** = gemessener Wert, **[I]** = Implementiert/best├ñtigt

---

### 39.1 API-Modul (`src/api/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| `IHttpHandler::handle()` Dispatch-Overhead (Router-Lookup + Invocation) | Ôëñ 5 ┬Ás / Req @ 10k RPS | [Z] | FUTURE_ENHANCEMENTS.md L80 |
| `IGraphQLSchemaBuilder` Type-Lookup (Query-Planning) | Ôëñ 1 ┬Ás / Field-Resolution | [Z] | FUTURE_ENHANCEMENTS.md L81 |
| WebSocket Frame-Dispatch via `IWebSocketFrameCallback` | Ôëñ 10 ┬Ás / Frame | [Z] | FUTURE_ENHANCEMENTS.md L82 |
| `IAPIVersionRouter::route()` Version-Extraktion + Handler-Aufl├Âsung | Ôëñ 2 ┬Ás | [Z] | FUTURE_ENHANCEMENTS.md L83 |
| `ICorrelationIDProvider::generate()` UUID-Generierung | Ôëñ 500 ns / Call | [Z] | FUTURE_ENHANCEMENTS.md L84 |
| `IGRPCBridge::dispatch()` ProtobufÔåÆInternal-Konvertierung | Ôëñ 20 ┬Ás / RPC-Call | [Z] | FUTURE_ENHANCEMENTS.md L85 |
| GraphQL parse + validate + execute (10-Feld-Query, 500 concurrent HTTP/2) | < 2 ms p99 | [Z] | README.md L56, FE L50 |
| GraphQL parse+execute aktuell (Sch├ñtzung) | ~5 ms | [M est.] | FUTURE_ENHANCEMENTS.md L260 |
| gRPC unary `GetDocument` Added-Latency vs. ├ñquivalentem REST-Call | < 1 ms | [Z] | README.md L73, FE L135 |
| WebSocket Event-Delivery-Latenz (ChangefeedÔåÆFrame) | < 50 ms | [Z] | FUTURE_ENHANCEMENTS.md L51 |
| WebSocket Frame-Delivery p99 @ 5 000 events/s | < 30 ms | [Z] | FUTURE_ENHANCEMENTS.md L87 |
| Bulk-Insert 10 000 256-Byte-Dokumente (ohne Netzwerk) | < 500 ms | [Z] | FUTURE_ENHANCEMENTS.md L105 |
| SSE Streaming First-Byte-Latenz (nach Query-Planning) | < 5 ms | [Z] | FUTURE_ENHANCEMENTS.md L106 |
| Middleware-Overhead (UUID + Thread-Local Write) | < 10 ┬Ás / Req | [Z] | README.md L115, FE L154 |
| OTLP Span-Enqueue (Hot-Path, single lock + push_back) | < 500 ns / Span | [Z] | FUTURE_ENHANCEMENTS.md L172 |
| OTLP Flush (64 Spans ÔåÆ lokaler OTLP-Collector, persistent conn) | < 5 ms | [Z] | FE L173 |

---

### 39.2 gRPC/RPC-Modul (`src/rpc_grpc/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| gRPC Health-Check (`SERVING`) nach `start()` | Sofort, `grpc_health_probe` exit 0 | [I] | FE L24ÔÇô25 |
| gRPC Prometheus-Histogramm Latency (per method) | verf├╝gbar unter `/metrics` | [Z] | FE L45 |
| TLS-Zertifikat Hot-Rotation (neue Connections) | Ôëñ 1 Verbindung mit altem Cert | [Z] | FE L96 |
| QUIC/HTTP3 Verbindungsaufbau (0-RTT Resumption) | Ziel: < 2 ms p99 | [Z] | FE L11 |
| gRPC Transport Port 8771 (bidirektionales Streaming) | standard | [I] | FE L12 |

---

### 39.3 Network-Modul (`src/network/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| TCP Accept | 1ÔÇô5 ms | [M] | README.md L1052 |
| TLS 1.3 Handshake (neue Verbindung) | 10ÔÇô50 ms (README); < 5 ms p99 (ROADMAP) | [M]/[Z] | README.md L1053, FE L288 |
| TLS 1.3 Session Resumption | < 1 ms p99 | [Z] | FE L288 |
| Frame Read/Write (Zero-Copy) | 100ÔÇô500 ┬Ás | [M] | README.md L1054 |
| Connection Pool Acquire (Lock-Free Fast-Path) | 10ÔÇô100 ┬Ás | [M] | README.md L1055 |
| Keep-Alive Check | 1ÔÇô10 ms (alle 60 s) | [M] | README.md L1056 |
| Circuit-Breaker Check | ~1 ┬Ás (Lock-Free Atomic) | [M] | README.md L1057 |
| Wire-Protocol Round-Trip p99 (Ôëñ 64 KiB Payload) | < 1 ms | [Z] | ROADMAP.md L66 |
| WebSocket Text-Frame Round-Trip (localhost) | < 2 ms p99 | [Z] | FE L289 |
| QUIC 0-RTT Verbindungsaufbau | < 2 ms p99 | [Z] | FE L290 |
| UDP Fast-Path GET Response (localhost) | < 500 ┬Ás p99 | [Z] | FE L291 |
| DPDK Kernel-Bypass Latenz | 1ÔÇô10 ┬Ás | [Z] | FE L284 |
| DPDK Throughput | 100 Gbps | [Z] | FE L284 |
| io_uring Latenz | 10ÔÇô50 ┬Ás | [Z] | FE L285 |
| io_uring Throughput | 10 Gbps | [Z] | FE L285 |

---

### 39.4 Server-Modul (`src/server/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| HTTP/1.1 Keep-Alive Sustained Throughput (4-Core, 1 KB Payload) | ÔëÑ 50 000 req/s | [Z] | FE L1083 |
| p50 Latenz | Ôëñ 5 ms | [Z] | FE L1084, ROADMAP L26 |
| p99 Latenz @ 80 % CPU | Ôëñ 50 ms | [Z] | FE L1084, ROADMAP L26 |
| TLS 1.3 Handshake (ECDSA P-256, Commodity HW) | Ôëñ 2 ms | [Z] | FE L1086 |
| Rate-Limiter State Sync (Distributed Token Bucket) | Ôëñ 10 ms Propagation Delay | [Z] | FE L18, ROADMAP L54 |
| Redis Round-Trip (Rate-Limit Check, same LAN) | Ôëñ 5 ms p99 | [Z] | ROADMAP L57 |
| Rate-Limit Throughput per Node | ÔëÑ 50 000 checks/s | [Z] | ROADMAP L57 |
| Raft Config Propagation (5 Nodes, LAN) | Ôëñ 100 ms | [Z] | ROADMAP L65 |
| Leader Failover via `leader_failover_timeout` | Ôëñ 500 ms | [I] | FE L172 |
| JWT Validation Overhead | 100ÔÇô500 ┬Ás / Req | [M] | README.md L1346 |
| Auth Middleware p50/p99 | < 100 ┬Ás / < 500 ┬Ás | [Z] | README.md L1313 |
| Rate Limiter p50/p99 | < 50 ┬Ás / < 200 ┬Ás | [Z] | README.md L1314 |
| Entity CRUD p50/p99 | < 5 ms / < 50 ms | [Z] | README.md L1315 |
| Query Execution (einfach) p50/p99 | < 10 ms / < 100 ms | [Z] | README.md L1316 |
| Vector Search p50/p99 | < 10 ms / < 50 ms | [Z] | README.md L1317 |
| Request Wall-Clock Timeout | 500 ms default ÔåÆ HTTP 504 | [I] | FE L130 |
| Congestion p99 > 500 ms ÔåÆ Adaptive Rate Reduction | auf 50 % | [I] | FE L383 |
| WASM Function CPU-Time Limit | 500 ms default | [Z] | ROADMAP L74 |

---

### 39.5 Query-Modul (`src/query/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Parse + Optimize (Ôëñ 10 Collections) | Ôëñ 5 ms p99 | [Z] | ROADMAP L198, FE L1393 |
| Simple AQL Execution (3-Node Cluster, warm Cache) | ÔëÑ 10 000 queries/s @ p99 < 20 ms | [Z] | ROADMAP L199, FE L1394 |
| Exact-Match Cache Lookup (10 000 Concurrent Clients) | Ôëñ 1 ms p99 | [Z] | ROADMAP L200, FE L1395 |
| Semantic Cache Lookup (inkl. Embedding-Similarity) | Ôëñ 10 ms p99 | [Z] | FE L1396 |
| JIT First-Compile Latenz | Ôëñ 50 ms | [Z] | FE L1397 |
| JIT Execution Speedup (Arithmetic-Heavy) | ÔëÑ 3├ù vs. Interpreter | [Z] | FE L1397 |
| Federation Cost-Sch├ñtzung (5-Cluster-Plan) | Ôëñ 20 ms | [Z] | FE L1398 |
| Streaming Result First-Chunk | Ôëñ 50 ms | [Z] | ROADMAP L201, FE L1399 |
| Query Cancellation (Memory + Locks freigegeben) | innerhalb 100 ms nach Signal | [Z] | FE L1408 |
| Optimizer `optimize()` (einfach, 1ÔÇô2 Pr├ñdikate) | 0.1ÔÇô5 ms | [M] | README.md L185 |
| Optimizer `optimize()` (komplex, 10+ Pr├ñdikate) | 5ÔÇô50 ms | [M] | README.md L186 |
| Simple Query Execution (1ÔÇô2 Pr├ñdikate) | 1ÔÇô10 ms | [M] | README.md L256 |
| Complex Query (5ÔÇô10 Pr├ñdikate, Joins) | 10ÔÇô100 ms | [M] | README.md L257 |
| Graph Traversal (Depth 3ÔÇô5) | 50ÔÇô500 ms | [M] | README.md L258 |
| Hybrid Query (Vector+Geo) | 10ÔÇô50 ms | [M] | README.md L259 |
| Fan-Out Latenz (16 Shards, LAN) | Ôëñ 200 ms | [Z] | ROADMAP L91 |

---

### 39.6 AQL-Modul (`src/aql/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Lexer Tokenisierung | ÔëÑ 50 MB/s / Core (ASCII) | [Z] | FE L14, L772 |
| Parser AST-Konstruktion (64 KB Query) | Ôëñ 10 ms | [Z] | FE L15, L773 |
| Full Round-Trip (parse + execute, 10-Table-Join, 100k Rows) | Ôëñ 500 ms | [Z] | FE L774 |
| LLM Command Async-Dispatch (ohne Inferenz) | Ôëñ 5 ms / Command | [Z] | FE L775 |
| Query Optimizer Rewrite Pass | Ôëñ 2 ms / 1000 AST-Nodes | [Z] | FE L776 |
| Batch NLÔåÆAQL (10 Requests, mock LLM 50 ms, concurrency ÔëÑ 4) | Ôëñ 150 ms Wall-Time | [I] | FE L159, L778 |
| AQL Validation Overhead | Ôëñ 1 ms / Generated Query | [Z] | FE L60 |
| Timeout-Thread Terminierung nach `executeWithTimeout()` | innerhalb `timeout + 500 ms` | [Z] | FE L788 |
| `push()` / `nextToken()` Overhead (ohne Modell-Generierung) | Ôëñ 500 ns | [Z] | ROADMAP L46 |
| Tool-Dispatch-Overhead (ohne Tool-Ausf├╝hrung) | Ôëñ 1 ms / Step | [Z] | ROADMAP L55 |

---

### 39.7 Cache-Modul (`src/cache/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Prefetch Prediction Latenz | Ôëñ 100 ┬Ás / Call | [Z] | FE L102 |
| L3 Cache Hit-Path (RocksDB-backed) | Ôëñ 5 ms p99 | [Z] | FE L161 |
| Admin API Response | Ôëñ 5 ms unabh├ñngig von L1-Cache-Gr├Â├ƒe | [Z] | FE L163 |
| Redis-Async Peer-Discovery (libuv-backed) | non-blocking | [I] | FE L82 |
| Distributed Cache Invalidation (alle Nodes) | propagiert innerhalb 500 ms | [Z] | FUTURE_ENHANCEMENTS core L572 |
| Distributed Cache `get` Round-Trip (Redis localhost) | Ôëñ 1 ms p99 | [Z] | core FE L582 |

---

### 39.8 Replication-Modul (`src/replication/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Replication Lag p99 (SEMI_SYNC, 3-Node LAN, 10k writes/s) | Ôëñ 50 ms | [Z] | FE L17, L841 |
| WAL-Shipping Throughput / Follower (Zstd Level 3, 10 GbE) | ÔëÑ 500 MB/s | [Z] | FE L18, L842 |
| Vector-Clock / HLC Conflict-Detection Overhead | < 5 ┬Ás / Write-Op | [Z] | FE L20, L844 |
| CRDT Merge Latenz (G-Counter / LWW-Register) | Ôëñ 1 ┬Ás / Merge | [Z] | FE L845 |
| Point-in-Time Recovery WAL Replay | ÔëÑ 200 MB/s; 100 GB in Ôëñ 10 min | [Z] | FE L846 |
| CDC Event Emission (Commit ÔåÆ Queue Enqueue) | Ôëñ 1 ms p99 | [Z] | FE L847 |
| Cross-Datacenter Replication Lag (ASYNC, 50 ms RTT WAN) | Ôëñ 200 ms p99 | [Z] | FE L848 |
| Async Mode Latenz | < 1 ms | [M] | README.md L952 |
| Semi-Sync Mode Latenz | 1ÔÇô5 ms | [M] | README.md L953 |
| Sync Mode Latenz | 2ÔÇô10 ms | [M] | README.md L954 |
| Tier 1 Critical SLA (SYNC, 3+ Replicas) | Ôëñ 10 ms | [Z] | ROADMAP L194 |
| Tier 2 Standard SLA (SEMI_SYNC, 2 Replicas) | Ôëñ 50 ms | [Z] | ROADMAP L194 |
| WAL Append Throughput | > 50 000 entries/s | [I] | ROADMAP L242 |
| WAL `readFrom` 1000 Entries | < 5 ms | [I] | ROADMAP L242 |
| WAL Serialize/Deserialize | < 2 ┬Ás | [I] | ROADMAP L242 |

---

### 39.9 Storage-Modul (`src/storage/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Point Read (Cache Hit) | 10ÔÇô50 ┬Ás | [M] | ARCHITECTURE.md L189, README.md L107 |
| Point Read (Cache Miss / Disk) | 100ÔÇô500 ┬Ás | [M] | ARCHITECTURE.md L189, README.md L675 |
| Hot-Tier (NVMe) | < 1 ms | [Z] | FE Storage |
| Warm-Tier (SATA) | ~5 ms | [Z] | FE Storage |
| Cold-Tier (S3) | ~50 ms | [Z] | FE Storage |
| Sustained Write Throughput (NVMe, 256er Batch, 4 KB avg) | ÔëÑ 100 000 ops/s | [Z] | FE L738 |
| p99 Point-Read (Hot-Tier, Bloom-Filter enabled) | Ôëñ 1 ms | [Z] | FE L739 |
| Incremental Backup Throughput (NVMe, parallel SSTable) | ÔëÑ 500 MB/s | [Z] | FE L740 |
| Streaming Ingest End-to-End Latenz | Ôëñ 50 ms | [Z] | FE general |
| Streaming Ingest Throughput | 1 M events/s | [Z] | FE general |
| Erasure Coding 6+3 Overhead | 50 % (vs. RAID-1 200 %) | [Z] | FE general |
| RocksDB WriteBatch Commit Latenz (Vector Add) | < 2 ms p99 | [Z] | index FE L970 |

---

### 39.10 CDC-Modul (`src/cdc/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Sequence Generation Throughput (8 Writer-Threads) | ÔëÑ 200 k/s | [I] | FE L405 ÔÇö Lock-free `atomic<uint64_t>` |
| Event Delivery p99 (Changefeed ÔåÆ WebSocket Frame) | < 20 ms | [Z] | FE L334 |
| Consumer Group Offset Commit (RocksDB Write) | < 1 ms p99 | [Z] | FE L365 |
| End-to-End Latenz (Change ÔåÆ Kafka `ack`, LAN) | < 10 ms p99 | [Z] | FE L387 |
| Compaction I/O Bandwidth Cap | 50 MB/s (konfigurierbar) | [Z] | FE L425 |
| SSE Event Delivery p99 (aktuell) | < 50 ms (Sch├ñtzung) | [M est.] | FE L461 |

---

### 39.11 Sharding-Modul (`src/sharding/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Cross-Shard RPC p99 (LAN, ohne Consensus) | < 5 ms | [Z] | FE L85 ÔÇö aktuell ~18 ms |
| Cross-Shard RPC aktuell (gemessen) | ~18 ms | [M] | FE L179 |
| 2PC Commit (5 Shards) aktuell | ~35 ms | [M] | FE L180 |
| 2PC Commit Ziel (5 Shards) | < 15 ms | [Z] | FE L180 |
| Percolator Commit (10 Shards) | < 20 ms p99 | [Z] | FE L104, L181 |
| Topology Change Propagation (100-Node Cluster) | Ôëñ 500 ms | [Z] | FE L13, L255 ÔÇö aktuell ~1.2 s |
| Topology Change aktuell (gemessen) | ~1.2 s | [M] | FE L184 |
| Anti-Entropy Scan Throughput (NVMe, 8 Workers) | > 1 GB/s / Node | [Z] | FE L141 |
| GPU Reed-Solomon Reconstruction | > 4 GB/s (NVIDIA A10) | [Z] | FE L142 |
| Lagging Replica Catch-Up (Snapshot, 10 GbE) | > 200 MB/s | [Z] | FE L162 |
| `replaceEndpoint()` (In-Memory, kein etcd Write) | < 1 ms | [Z] | FE L253 |
| `replaceEndpoint()` (mit etcd Write) | < 10 ms | [Z] | FE L253 |
| `NodeIdentity::loadFrom()` (NVMe, ~200 Bytes) | < 5 ms | [Z] | FE L254 |
| Shard Split Migration Read-Unavailability | 0 ms (Dual-Write) | [Z] | FE L122 |

---

### 39.12 Search-Modul (`src/search/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Hybrid Search (BM25 + HNSW RRF, Top-10, 10 M Docs) | Ôëñ 20 ms p99 | [Z] | FE L471 |
| LLM Query-Rewriter Overhead | Ôëñ 200 ms Added Latency p99; 0 ms wenn LLM unavailable | [Z] | FE L471 |
| Facet Counting (1 000 Werte, 100k Docs) | Ôëñ 5 ms | [Z] | FE L472 |
| LTR Re-Ranking (Top-100, 6-dim Linear Model) | Ôëñ 2 ms | [Z] | FE L473 |
| Autocomplete Suggestion (1 M-Term Dictionary) | Ôëñ 5 ms p99 | [Z] | FE L475 |
| BM25/FTS Query Latenz | 1ÔÇô10 ms | [M] | README.md L113 |
| Vector Search Query Latenz | 1ÔÇô10 ms (k=10, 1M vectors) | [M] | README.md L119 |
| Hybrid Search Query Latenz | 5ÔÇô20 ms | [M] | README.md L125 |

---

### 39.13 Security-Modul (`src/security/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| AES-256-GCM Encrypt/Decrypt Throughput (AES-NI, 1 Core) | ÔëÑ 1 GB/s | [Z] | FE L967, ROADMAP L130 |
| RSA-4096 Signature Verification | p99 Ôëñ 5 ms | [Z] | FE L968 |
| Kyber-1024 Key Encapsulation | ÔëÑ 2 000 ops/s | [Z] | FE L969, ROADMAP L132 |
| Dilithium-5 Signing | ÔëÑ 1 000 ops/s | [Z] | FE L970, ROADMAP L133 |
| TLS 1.3 Handshake (ECDHE-AES256-GCM) | p99 Ôëñ 10 ms | [Z] | FE L971 |
| RBAC Policy Evaluation (Ôëñ 100 Roles) | p99 Ôëñ 0.5 ms | [Z] | FE L972 |
| HSM-backed RSA-2048 Sign (SoftHSM2 Baseline) | p99 Ôëñ 20 ms | [Z] | FE L973 |
| Audit Log Tamper-Evident Append | p99 Ôëñ 2 ms / Entry | [Z] | FE L974, ROADMAP L136 |
| Encryption Overhead / Feld (256-Byte Payload) | ~5ÔÇô10 ┬Ás | [M] | README.md L194 |
| Decryption Overhead / Feld | ~3ÔÇô7 ┬Ás | [M] | README.md L195 |
| Key Cache Lookup (In-Memory) | ~100 ns | [M] | README.md L196 |
| Vault API Call (gecacht, 1 Std.) | ~50ÔÇô100 ms | [M] | README.md L197 |
| HSM Operation (Hardware) | ~5ÔÇô20 ms | [M] | README.md L198 |
| Document Insert mit Verschl├╝sselung | 1.4 ms (+16 % vs. plain) | [M] | README.md L859 |
| Document Query mit Verschl├╝sselung | 1.1 ms (+37 % vs. plain) | [M] | README.md L860 |
| Bulk Insert 1k Docs mit Verschl├╝sselung | 1050 ms (+23 % vs. plain) | [M] | README.md L861 |

---

### 39.14 Analytics-Modul (`src/analytics/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| IVM Delta-Application (Ôëñ 10 000 Rows) | Ôëñ 50 ms | [Z] | FE L22, L32 |
| IVM Reader p99 w├ñhrend 10k-Row-Batch-Apply | Ôëñ 10 ms | [Z] | FE L209 |
| CSV Export 1 M Rows (Streaming, kein Full In-Memory) | Ôëñ 500 ms | [Z] | FE L81 |
| CEP Engine `stop()` | Ôëñ 100 ms | [Z] | FE L104 |
| CEP `process()` Lock-Hold-Dauer | Ôëñ 50 ┬Ás | [Z] | FE L130 |
| IsolationForest Training (1000-Punkt-Window) | Ôëñ 10 ms | [Z] | FE L131 |
| CEP p99 Latenz (8 Threads @ 100 kHz) | Ôëñ 1 ms | [Z] | FE L127 |
| `putInCache()` / `getFromCache()` | O(1) amortisiert, Ôëñ 1 ┬Ás p99 (16 Concurrent) | [Z] | FE L236 |
| `getCacheKey()` (500-Event Trace, Hash-basiert) | Ôëñ 50 ┬Ás | [Z] | FE L237 |
| Einfache Aggregation SUM (1 M Rows) | 15 ms (66k rows/s) | [M] | README.md L1193 |
| Einfache Aggregation SUM (10 M Rows) | 142 ms (70k rows/s) | [M] | README.md L1194 |
| GROUP BY 1 Dim. (1 M Rows) | 45 ms (22k rows/s) | [M] | README.md L1195 |
| GROUP BY 1 Dim. (10 M Rows) | 425 ms (23k rows/s) | [M] | README.md L1196 |
| GROUP BY 3 Dim. (1 M Rows) | 120 ms (8.3k rows/s) | [M] | README.md L1197 |
| Window Function ROW_NUMBER (1 M Rows) | 80 ms (12.5k rows/s) | [M] | README.md L1199 |
| Window Function Moving Average (1 M Rows) | 95 ms (10.5k rows/s) | [M] | README.md L1200 |
| Complex OLAP CUBE (1 M Rows) | 350 ms (2.8k rows/s) | [M] | README.md L1201 |
| Complex OLAP ROLLUP (1 M Rows) | 280 ms (3.5k rows/s) | [M] | README.md L1202 |
| SIMD SUM (10 M Rows) | 28 ms (5.1├ù Speedup vs. Scalar 142 ms) | [M] | README.md L1207 |
| SIMD AVG (10 M Rows) | 35 ms (4.5├ù Speedup) | [M] | README.md L1208 |
| SIMD MIN/MAX (10 M Rows) | 18 ms (6.9├ù Speedup) | [M] | README.md L1209 |
| SIMD Complex Filter (10 M Rows) | 45 ms (4.7├ù Speedup) | [M] | README.md L1210 |
| JSON Export (100k Rows) | 250 ms (400k rows/s, 45 MB) | [M] | README.md L1216 |
| Fan-Out Latenz (16 Shards, LAN) | Ôëñ 200 ms | [Z] | ROADMAP L72 |
| Model Export (Ôëñ 1 M Samples) | Ôëñ 500 ms | [Z] | ROADMAP L86 |

---

### 39.15 Timeseries-Modul (`src/timeseries/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Single-Point Insert p99 (Gorilla compressed) | Ôëñ 50 ┬Ás | [Z] | FE L39 |
| Gorilla on-disk compression (1000 Punkte) | Ôëñ 15 % raw size | [Z] | FE L39 |
| Gorilla Decode Throughput (aktuell) | ~400 MB/s | [M] | FE L153 |
| Gorilla Decode Throughput (SIMD Ziel) | > 2 GB/s | [Z] | FE L59, L153 |
| Range Scan 1 M Punkte float64 (aktuell) | ~300 ms | [M] | FE L154 |
| Range Scan 1 M Punkte float64 (Ziel) | < 50 ms p99 | [Z] | FE L60, L154 |
| Continuous Aggregate Refresh (aktuell) | ~5 s | [M] | FE L155 |
| Continuous Aggregate Refresh (Ziel, 100k inserts/s) | < 500 ms / Aggregat / Minute | [Z] | FE L77, L155 |
| Buffer-to-Storage Flush p99 | < 10 ms | [Z] | FE L114 |
| AES-256-GCM Throughput / Core (AES-NI via OpenSSL EVP) | > 1 GB/s | [Z] | FE L135 |

---

### 39.16 Transaction-Modul (`src/transaction/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Begin-Latenz | < 1 ┬Ás | [M] | README.md L130 |
| Commit-Latenz (abh├ñngig von Batch-Gr├Â├ƒe) | 100 ┬ÁsÔÇô5 ms | [M] | README.md L130 |
| Lock-Overhead / Lock-Acquire | ~5 ns (Atomics) | [M] | README.md L131 |
| Deadlock-Detection Intervall (konfigurierbar) | 100 ms | [M] | README.md L132 |
| Lock-Free Read (Fast-Path, kein Contention) | < 10 ns | [M] | README.md L820 |
| Stats Collection / Operation | < 5 ns (Atomic Increment) | [M] | README.md L819 |
| OCC Commit p50 ÔåÆ aktuell | 1 ms | [M] | FE L872 |
| OCC Commit p99 ÔåÆ aktuell | 10 ms | [M] | FE L872 |
| OCC Commit p50 ÔåÆ Ziel | 100 ┬Ás | [Z] | FE L872 |
| OCC Commit p99 ÔåÆ Ziel | 5 ms | [Z] | FE L873 |
| SAGA Compensation Time ÔåÆ aktuell | 100 ms | [M] | FE L875 |
| SAGA Compensation Time ÔåÆ Ziel | 20 ms | [Z] | FE L875 |
| Distributed 2PC Latenz ÔåÆ aktuell | 10 ms | [M] | FE L876 |
| Distributed 2PC Latenz ÔåÆ Ziel | 5 ms | [Z] | FE L876 |
| Batch Window (konfigurierbar) | 1ÔÇô100 ms | [I] | FE L495 |
| Retry-Kosten / Versuch | ~1 ms | [M] | FE L163 |
| Deadlock-Watchdog Fallback-Timer | innerhalb 500 ms | [Z] | FE L938 |
| Conflict Detection | ~1 ms / 1000 Keys | [M] | README.md L656 |

---

### 39.17 Index-Modul (`src/index/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| HNSW Vector Search (1M 128-dim, k=10) CPU | ÔëÑ 5 000 QPS | [Z] | FE L964 |
| HNSW Vector Search (1M 128-dim, k=10) GPU (RTX) | ÔëÑ 50 000 QPS | [Z] | FE L964 |
| B-Tree Secondary Index Point Lookup (10M Keys) | < 500 ┬Ás p99 | [Z] | FE L966 |
| R-Tree Spatial Range Query (1M Punkte, 1 % Selectivity) | < 10 ms p99 | [Z] | FE L967 |
| HNSW CPU Brute-Force Query (1M vectors) | 10ÔÇô100 ms | [M] | README.md L882 |
| HNSW CPU Query | 0.1ÔÇô1 ms | [M] | README.md L883 |
| HNSW GPU (Vulkan, Batch) | 0.01ÔÇô0.1 ms | [M] | README.md L884 |
| B-Tree Point Lookup (mit Cache) | 10ÔÇô50 ┬Ás | [M] | README.md L298 |
| R-Tree Bounding Box | 1ÔÇô10 ms | [M] | README.md L487 |
| R-Tree Radius Search | 1ÔÇô20 ms | [M] | README.md L488 |
| Generic Loop Scan | ~1 GB/s | [M] | FE L398 |
| AVX-512 SIMD Scan (geplant) | ~50 GB/s | [Z] | FE L399 |

---

### 39.18 Geo-Modul (`src/geo/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| `intersects` (1M Punkte, linear) | ~2 000 ms | [M] | FE L210 |
| `intersects` (1M Punkte, R-Tree) | Ôëñ 5 ms p99 | [Z] | FE L73, L210 |
| ST_BUFFER (10k Punkte @ 500 m, CPU) | Ôëñ 200 ms | [Z] | FE L98, L212 |
| ST_BUFFER (10k Punkte @ 500 m, A10G) | Ôëñ 20 ms (10├ù CPU) | [Z] | FE L352 |
| GPU Contains (1M Punkte, A10G) | Ôëñ 50 ms | [Z] | FE L213 |
| Spatial JOIN (2 ├ù 100k Punkte, 1 km, erste 1000 Ergebnisse) | Ôëñ 500 ms | [Z] | FE L126 |
| `sampleAt` (1M-Cell Grid) | Ôëñ 1 ┬Ás / Call | [Z] | FE L150 |
| `queryBBox` (10k Cells aus 1M-Cell Grid) | Ôëñ 10 ms | [Z] | FE L151 |
| `generateHeatmap` (100k Punkte, 100├ù100, 500 m BW) | Ôëñ 500 ms | [Z] | FE L152 |
| Ellipsoidal ST_Distance (1M Paare, CPU) | Ôëñ 500 ms | [Z] | FE L275 |
| Ellipsoidal ST_Distance (1M Paare, A10G) | Ôëñ 50 ms | [Z] | FE L276 |
| ST_UNION (1000 Polygon-Paare, A10G) | Ôëñ 10 ms | [Z] | FE L353 |
| `locationAtTime` (100k Rows) | Ôëñ 1 ms | [Z] | FE L193 |
| `entitiesWithinDistanceAtTime` (10k Entities, linear) | Ôëñ 50 ms | [Z] | FE L194 |

---

### 39.19 Acceleration-Modul (`src/acceleration/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| CUDA L2-Search (1M ├ù 128-dim, RTX 3090) | < 8 ms | [Z] | FE L45, L427 |
| Cosine Search Vulkan/MoltenVK (500k ├ù 128-dim, M2 Pro) | < 20 ms Ô£à | [I] | FE L428 |
| GPU Distributed Index (100M ├ù 128-dim, 4├ù A100, k=100) | < 15 ms p99 | [Z] | FE L79, L369 |
| NCCL `mergeTopK` (worldSize=4, k=100, NVLink-3) | < 500 ┬Ás | [Z] | FE L80, L432 |
| Device Probe (4-GPU System) | < 50 ms Ô£à | [I] | FE L431 |
| `getStats()` Call Latenz (Linux /proc/stat) | < 2 ms Ô£à | [I] | FE L434 |
| `canUseGPU()` NVML-Timeout-Guard | 500 ms Timeout ÔåÆ false (CPU-Fallback) | [I] | FE L443 |
| CPU Monitoring `/proc/stat` Polling-Intervall | 100 ms | [I] | FE L131 |

---

### 39.20 LLM-Modul (`src/llm/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Time-to-First-Token (512-Token Prompt, A10G) aktuell | ~350 ms (Sch├ñtzung) | [M est.] | FE L238 |
| Time-to-First-Token (512-Token Prompt, A10G) Ziel | Ôëñ 200 ms p99 | [Z] | FE L138, L238 |
| TTFT Bypass DeduplicationCache f├╝r Streaming | aktiviert (TTFT Ôëñ 200 ms) | [I] | FE L125 |
| OpenAI-Compat Adapter Round-Trip Overhead | Ôëñ 2 ms vs. direktem `submitRequest()` | [I] | FE L165 |
| Work-Stealing Pool Task Dispatch | Ôëñ 50 ┬Ás p99 (submit ÔåÆ Worker Pickup) | [Z] | FE L185, L241 |
| LoRA Adapter Application | < 1 ms Overhead | [M] | llama_lora_adapter_README L163 |
| Incomplete-Stream Warning (EOF ohne Marker) | innerhalb 500 ms | [Z] | FE L86 |

---

### 39.21 RAG-Modul (`src/rag/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Fast Evaluation Mode E2E | Ôëñ 100 ms p99 (kein LLM-Call) | [I] | FE L17, ROADMAP L28 |
| Balanced Evaluation Mode E2E | Ôëñ 500 ms p99 | [I] | FE L18 |
| Thorough Evaluation Mode E2E | Ôëñ 2 000 ms p99 | [Z] | FE L18 |
| StreamingRetriever First-Chunk | Ôëñ 50 ms | [Z] | FE L767 |
| ClaimExtractor (1000-Zeichen Antwort, LLM-First) | Ôëñ 500 ms | [Z] | FE L769 |
| ClaimExtractor (heuristischer Fallback) | Ôëñ 50 ms | [Z] | FE L769 |
| RAG Query E2E (Vector Search + LLM Generation) | 50ÔÇô500 ms | [M] | aql README L165 |

---

### 39.22 Observability-Modul (`src/observability/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Metrics Collection Overhead | < 1 % CPU @ 1 000 req/s | [Z] | FE L20, L1221 |
| Prometheus `/metrics` Scrape Response | < 50 ms p99 @ 10 000 active series | [Z] | FE L1225 |
| Span Creation + In-Process Propagation | < 5 ┬Ás / Span | [Z] | FE L1226 |
| OTLP Export Latenz (async, 1 000 spans/s) | < 5 ms p99 | [Z] | FE L1227 |
| `QueryProfiler` per-Operator Timing Overhead | < 1 ┬Ás / Operator Boundary | [Z] | FE L1228 |
| CPU Sampling Period | ~100 ms (1 % CPU Overhead) | [I] | README.md L630 |
| Query P99 Alert-Threshold (Default) | > 1 000 ms | [I] | ROADMAP L58 |

---

### 39.23 Performance-Modul (`src/performance/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| RDTSC/RDTSCP Measurement Overhead (x86-64) | < 1 ns / Messpunkt | [I] | FE L20, ROADMAP L87 |
| RAII Scoped Timer Overhead (1M Iterationen) | < 2 ns / Call average | [Z] | FE L809 |
| P99-Percentile-Lookup (Ring bis 1 M Samples) | < 500 ns | [Z] | FE L821 |
| GPU Metric Export Overhead (CUDA Stream / Inference) | < 100 ┬Ás | [Z] | FE L823 |
| PMU Counter Read (`perf_event_open`) | < 1 ┬Ás | [Z] | FE L825 |
| Query Compilation Time | < 100 ms | [Z] | FE L235 |
| No-Op Adapter | < 1 ns / Call | [M] | core README L319 |
| Spdlog Async Adapter | ~50ÔÇô100 ns / Log Call | [M] | core README L320 |
| Prometheus Metrics Update | ~200ÔÇô500 ns | [M] | core README L321 |
| OTEL Span Creation | ~1ÔÇô5 ┬Ás | [M] | core README L322 |

---

### 39.24 ONNX/CLIP-Modul (`src/onnx_clip/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| ViT-B/32 Image Encoding (CPU) | Ôëñ 150 ms / Image | [Z] | AUDIT L51, ROADMAP L43 |
| ViT-B/32 CUDA Batch-64 | Ôëñ 20 ms (Ôëñ 0.31 ms / Image) | [Z] | FE L30 |
| Text Encoding (CPU) | Ôëñ 5 ms p95 | [Z] | FE L56, L59 |
| Metrics Collection Overhead | Ôëñ 0.05 ms / Call | [Z] | FE L100 |

---

### 39.25 Content-Modul (`src/content/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| DOCX Extraktion (500 KB) | < 200 ms | [Z] | FE L43 |
| NDJSON Streaming Ingestion (1 GB, NVMe) | ÔëÑ 100 MB/s | [Z] | FE L102, ROADMAP L107 |
| pHash (4 MP JPEG) | < 5 ms | [Z] | FE L121, ROADMAP L108 |
| MinHash + LSH Lookup (10 KB Text, 100k Entries) | < 1 ms | [Z] | FE L122 |
| Tesseract Init (warm, per Language Pack) | < 500 ms | [Z] | FE L143 |
| Embedding (384-dim, batch=32, CPU) | < 50 ms | [Z] | FE L161, ROADMAP L110 |
| Embedding (384-dim, batch=32, CUDA) | < 5 ms | [Z] | FE L161 |
| Ingestion + Embedding Overhead vs. Plain Ingestion | < 100 ms (Batch-amortisiert) | [Z] | FE L162 |

---

### 39.26 Ingestion-Modul (`src/ingestion/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| HTTP GET Round-Trip Overhead (vs. raw TCP) | Ôëñ 5 ms | [Z] | FE L69 |
| Kafka ÔåÆ ThemisDB E2E Latenz | Ôëñ 500 ms p99 | [Z] | FE L89 |
| S3 `ListObjectsV2` (1000 Objekte) | Ôëñ 100 ms | [Z] | FE L109 |
| S3 Concurrent Downloads (4 parallel, 10 Gbps) | ÔëÑ 200 MB/s aggregate | [Z] | FE L110, L189 |
| Per-Dokument Quarant├ñne Retry (Ôëñ 1 MB) | Ôëñ 10 ms | [Z] | FE L146, L190 |

---

### 39.27 Exporters-Modul (`src/exporters/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| JSONL Export Throughput (aktuell) | ~150 MB/s (Full Batch) | [M] | FE L107 |
| JSONL Export Throughput (Ziel) | ÔëÑ 200 000 docs/s sustained | [Z] | FE L107 |
| Parquet Export (Arrow Path, uncompressed) | ÔëÑ 500 MB/s | [Z] | FE L109 |
| Retry Initial Delay (konfigurierbar, Default) | 500 ms (doubles each retry) | [I] | README.md L193 |

---

### 39.28 Chimera-Modul (`src/chimera/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Vector Search (k=10, 1M Vectors) | 1ÔÇô10 ms | [M] | README.md L799 |
| `insert_vector()` HNSW | 1ÔÇô10 ms | [M] | README.md L798 |
| Graph Traversal Depth 5 (1M Nodes) | < 100 ms | [Z] | FE L860 |
| `shortest_path()` | 10ÔÇô500 ms | [M] | README.md L800 |
| `execute_query()` | 1ÔÇô1000 ms | [M] | README.md L797 |
| `find_documents()` | 1ÔÇô100 ms | [M] | README.md L801 |
| Connection Pool Acquire | < 1 ms | [Z] | FE L866 |
| Streaming Result Throughput | 100 MB/s | [Z] | FE L864 |
| Metric Export | < 100 ┬Ás | [Z] | FE L870 |
| Schema-Operations (Index-Erstellung) | < 100 ms | [Z] | FE L871 |
| Connection State Check Overhead | ~1 ns | [M] | README.md L391 |

---

### 39.29 Graph-Modul (`src/graph/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Algorithm Selection (Ôëñ 10M Nodes) | < 1 ms p99 | [Z] | FE L1122 |
| Plan Cache Lookup (inkl. Fingerprint-Vergleich) | < 100 ┬Ás p99 | [Z] | FE L1122 |
| Subgraph Isomorphism (100-Node Pattern, 1M-Node Graph) | < 500 ms p95 | [Z] | FE L1125 |
| Audit Trail `appendAudit()` Overhead | < 1 ┬Ás / Mutation (Bounded Ring Buffer) | [Z] | FE L1079 |
| `ChangeFeed::recordEvent()` (RocksDB single put) | < 5 ┬Ás / Event | [Z] | FE L1080 |
| Background Scheduler Wake-Up Jitter | < 50 ms | [Z] | FE L1082 |
| Observierter BFS (10k-Node Graph) | ~8 ms | [M] | FE L146 |
| Statistics Collection | 10ÔÇô100 ms (gecacht nach erstem Aufruf) | [M] | README.md L803 |
| Plan Generation (einfach) | 0.1ÔÇô5 ms | [M] | README.md L804 |
| Complex Queries (Pattern Matching) | 5ÔÇô50 ms | [M] | README.md L805 |
| Plan Cache Lookup Hit Rate | 80ÔÇô90 % | [M] | README.md L806 |
| Single Constraint Check | ~0.1 ┬Ás | [M] | README.md L820 |
| Path Validation (10 Constraints) | ~1 ┬Ás / Path | [M] | README.md L821 |
| `findConstrainedPaths` (1000 explored, 10 valid) | 10ÔÇô100 ms | [M] | README.md L822 |
] | README.md L797 |
| `find_documents()` | 1–100 ms | [M] | README.md L801 |
| Connection Pool Acquire | < 1 ms | [Z] | FE L866 |
| Streaming Result Throughput | 100 MB/s | [Z] | FE L864 |
| Metric Export | < 100 µs | [Z] | FE L870 |
=======
| Vector Search (k=10, 1M Vectors) | 1ÔÇô10 ms | [M] | README.md L799 |
| `insert_vector()` HNSW | 1ÔÇô10 ms | [M] | README.md L798 |
| Graph Traversal Depth 5 (1M Nodes) | < 100 ms | [Z] | FE L860 |
| `shortest_path()` | 10ÔÇô500 ms | [M] | README.md L800 |
| `execute_query()` | 1ÔÇô1000 ms | [M] | README.md L797 |
| `find_documents()` | 1ÔÇô100 ms | [M] | README.md L801 |
| Connection Pool Acquire | < 1 ms | [Z] | FE L866 |
| Streaming Result Throughput | 100 MB/s | [Z] | FE L864 |
| Metric Export | < 100 ┬Ás | [Z] | FE L870 |
>>>>>>> Stashed changes
| Schema-Operations (Index-Erstellung) | < 100 ms | [Z] | FE L871 |
| Connection State Check Overhead | ~1 ns | [M] | README.md L391 |

---

### 39.29 Graph-Modul (`src/graph/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Algorithm Selection (≤ 10M Nodes) | < 1 ms p99 | [Z] | FE L1122 |
| Plan Cache Lookup (inkl. Fingerprint-Vergleich) | < 100 µs p99 | [Z] | FE L1122 |
| Subgraph Isomorphism (100-Node Pattern, 1M-Node Graph) | < 500 ms p95 | [Z] | FE L1125 |
| Audit Trail `appendAudit()` Overhead | < 1 µs / Mutation (Bounded Ring Buffer) | [Z] | FE L1079 |
| `ChangeFeed::recordEvent()` (RocksDB single put) | < 5 µs / Event | [Z] | FE L1080 |
| Background Scheduler Wake-Up Jitter | < 50 ms | [Z] | FE L1082 |
| Observierter BFS (10k-Node Graph) | ~8 ms | [M] | FE L146 |
| Statistics Collection | 10–100 ms (gecacht nach erstem Aufruf) | [M] | README.md L803 |
| Plan Generation (einfach) | 0.1–5 ms | [M] | README.md L804 |
| Complex Queries (Pattern Matching) | 5–50 ms | [M] | README.md L805 |
| Plan Cache Lookup Hit Rate | 80–90 % | [M] | README.md L806 |
| Single Constraint Check | ~0.1 µs | [M] | README.md L820 |
| Path Validation (10 Constraints) | ~1 µs / Path | [M] | README.md L821 |
| `findConstrainedPaths` (1000 explored, 10 valid) | 10–100 ms | [M] | README.md L822 |
| Algorithm Selection (Ôëñ 10M Nodes) | < 1 ms p99 | [Z] | FE L1122 |
| Plan Cache Lookup (inkl. Fingerprint-Vergleich) | < 100 ┬Ás p99 | [Z] | FE L1122 |
| Subgraph Isomorphism (100-Node Pattern, 1M-Node Graph) | < 500 ms p95 | [Z] | FE L1125 |
| Audit Trail `appendAudit()` Overhead | < 1 ┬Ás / Mutation (Bounded Ring Buffer) | [Z] | FE L1079 |
| `ChangeFeed::recordEvent()` (RocksDB single put) | < 5 ┬Ás / Event | [Z] | FE L1080 |
| Background Scheduler Wake-Up Jitter | < 50 ms | [Z] | FE L1082 |
| Observierter BFS (10k-Node Graph) | ~8 ms | [M] | FE L146 |
| Statistics Collection | 10ÔÇô100 ms (gecacht nach erstem Aufruf) | [M] | README.md L803 |
| Plan Generation (einfach) | 0.1ÔÇô5 ms | [M] | README.md L804 |
| Complex Queries (Pattern Matching) | 5ÔÇô50 ms | [M] | README.md L805 |
| Plan Cache Lookup Hit Rate | 80ÔÇô90 % | [M] | README.md L806 |
| Single Constraint Check | ~0.1 ┬Ás | [M] | README.md L820 |
| Path Validation (10 Constraints) | ~1 ┬Ás / Path | [M] | README.md L821 |
| `findConstrainedPaths` (1000 explored, 10 valid) | 10ÔÇô100 ms | [M] | README.md L822 |
Vectors) | 1–10 ms | [M] | README.md L799 |
| `insert_vector()` HNSW | 1–10 ms | [M] | README.md L798 |
| Graph Traversal Depth 5 (1M Nodes) | < 100 ms | [Z] | FE L860 |
| `shortest_path()` | 10–500 ms | [M] | README.md L800 |
| `execute_query()` | 1–1000 ms | [M] | README.md L797 |
| `find_documents()` | 1–100 ms | [M] | README.md L801 |
| Connection Pool Acquire | < 1 ms | [Z] | FE L866 |
| Streaming Result Throughput | 100 MB/s | [Z] | FE L864 |
| Metric Export | < 100 µs | [Z] | FE L870 |
| Schema-Operations (Index-Erstellung) | < 100 ms | [Z] | FE L871 |
| Connection State Check Overhead | ~1 ns | [M] | README.md L391 |

---

### 39.29 Graph-Modul (`src/graph/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Algorithm Selection (≤ 10M Nodes) | < 1 ms p99 | [Z] | FE L1122 |
| Plan Cache Lookup (inkl. Fingerprint-Vergleich) | < 100 µs p99 | [Z] | FE L1122 |
| Subgraph Isomorphism (100-Node Pattern, 1M-Node Graph) | < 500 ms p95 | [Z] | FE L1125 |
| Audit Trail `appendAudit()` Overhead | < 1 µs / Mutation (Bounded Ring Buffer) | [Z] | FE L1079 |
| `ChangeFeed::recordEvent()` (RocksDB single put) | < 5 µs / Event | [Z] | FE L1080 |
| Background Scheduler Wake-Up Jitter | < 50 ms | [Z] | FE L1082 |
| Observierter BFS (10k-Node Graph) | ~8 ms | [M] | FE L146 |
| Statistics Collection | 10–100 ms (gecacht nach erstem Aufruf) | [M] | README.md L803 |
| Plan Generation (einfach) | 0.1–5 ms | [M] | README.md L804 |
| Complex Queries (Pattern Matching) | 5–50 ms | [M] | README.md L805 |
| Plan Cache Lookup Hit Rate | 80–90 % | [M] | README.md L806 |
| Single Constraint Check | ~0.1 µs | [M] | README.md L820 |
| Path Validation (10 Constraints) | ~1 µs / Path | [M] | README.md L821 |
| `findConstrainedPaths` (1000 explored, 10 valid) | 10–100 ms | [M] | README.md L822 |

) | Status |
|-----------|------|-----------------|----------------------------|--------|

### 39.28 Chimera-Modul (`src/chimera/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Vector Search (k=10, 1M Vectors) | 1ÔÇô10 ms | [M] | README.md L799 |
| `insert_vector()` HNSW | 1ÔÇô10 ms | [M] | README.md L798 |
| Graph Traversal Depth 5 (1M Nodes) | < 100 ms | [Z] | FE L860 |
| `shortest_path()` | 10ÔÇô500 ms | [M] | README.md L800 |
| `execute_query()` | 1ÔÇô1000 ms | [M] | README.md L797 |
| `find_documents()` | 1ÔÇô100 ms | [M] | README.md L801 |
| Connection Pool Acquire | < 1 ms | [Z] | FE L866 |
| Streaming Result Throughput | 100 MB/s | [Z] | FE L864 |
| Metric Export | < 100 ┬Ás | [Z] | FE L870 |
| Schema-Operations (Index-Erstellung) | < 100 ms | [Z] | FE L871 |
| Connection State Check Overhead | ~1 ns | [M] | README.md L391 |

---

### 39.29 Graph-Modul (`src/graph/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Algorithm Selection (Ôëñ 10M Nodes) | < 1 ms p99 | [Z] | FE L1122 |
| Plan Cache Lookup (inkl. Fingerprint-Vergleich) | < 100 ┬Ás p99 | [Z] | FE L1122 |
| Subgraph Isomorphism (100-Node Pattern, 1M-Node Graph) | < 500 ms p95 | [Z] | FE L1125 |
| Audit Trail `appendAudit()` Overhead | < 1 ┬Ás / Mutation (Bounded Ring Buffer) | [Z] | FE L1079 |
| `ChangeFeed::recordEvent()` (RocksDB single put) | < 5 ┬Ás / Event | [Z] | FE L1080 |
| Background Scheduler Wake-Up Jitter | < 50 ms | [Z] | FE L1082 |
| Observierter BFS (10k-Node Graph) | ~8 ms | [M] | FE L146 |
| Statistics Collection | 10ÔÇô100 ms (gecacht nach erstem Aufruf) | [M] | README.md L803 |
| Plan Generation (einfach) | 0.1ÔÇô5 ms | [M] | README.md L804 |
| Complex Queries (Pattern Matching) | 5ÔÇô50 ms | [M] | README.md L805 |
| Plan Cache Lookup Hit Rate | 80ÔÇô90 % | [M] | README.md L806 |
| Single Constraint Check | ~0.1 ┬Ás | [M] | README.md L820 |
| Path Validation (10 Constraints) | ~1 ┬Ás / Path | [M] | README.md L821 |
| `findConstrainedPaths` (1000 explored, 10 valid) | 10ÔÇô100 ms | [M] | README.md L822 |

