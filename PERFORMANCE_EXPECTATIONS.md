# ThemisDB – Performance-Erwartungswerte & Messergebnisse

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
| ✅ | Ziel erfüllt (gemessen ≥ Ziel) |
| ❌ | Ziel nicht erfüllt (gemessen < Ziel) |
| ⚠️ | Partiell / bekannte Regression |
| ❓ | Kein Messwert vorhanden |
| –  | Nicht gemessen in dieser Version |

---

---

## Inhaltsverzeichnis

> **Struktur: Allgemein → Spezifisch (Module) → Rohdaten → Interface-SLOs**

| # | Abschnitt | Typ |
|---|-----------|-----|
| – | Legende | Referenz |
| 1 | Versionshistorie – Kernmetriken | Messung (allgemein) |
| 2–29 | Modul-Spezifische Erwartungswerte | Modul-SLOs |
| 30 | Chimera-Baseline & Suite | Benchmark-Framework |
| 31–32 | Prompt Engineering / Ethics AI | Modul-SLOs |
| 33 | System-Level TPC/YCSB | Benchmarks |
| 34 | CI Regression-Schwellwerte | CI |
| 35 | Bekannte Performance-Lücken | Lücken |
| 36 | Rohdaten: Google Benchmark C++ | Primäre Messungen |
| 37 | Performance-Maßnahmen (GitHub-PR) | Maßnahmen nach Modul |
| 38 | Rohdaten: HTTP-API & Docker Benchmarks | Primäre Messungen |
| 39 | API/Interface Performance-Annahmen | Interface SLOs |

**Hinweis zur Statusbewertung:** Felder mit ❓ sind Erwartungswerte ohne vorliegende Messung.
Typ-Kennung in §39: **[M]** = gemessen · **[Z]** = Ziel · **[I]** = implementiert/bestätigt.

## 1. Versionshistorie – Kernmetriken

> Quelle: `benchmarks/VERSION_HISTORY.csv` + `benchmarks/results_analysis_reports/benchmark_summary.csv`
> Testplattform v1.3.0–v1.3.3: Intel i9-10900K (10C/20T @ 3.70 GHz), 31 GB RAM, WSL2 Linux
> Testplattform v1.3.4: Windows x64, 20 Cores @ 3.696 GHz, 20 MB L3-Cache

| Metrik | Ziel | v1.3.0 | v1.3.1 | v1.3.2 | v1.3.3 | **v1.3.4** | **v1.8.0 Ziel** | Δ v1.3.0→v1.3.4 | Status |
|--------|------|--------|--------|--------|--------|-----------|-----------------|-----------------|--------|
| Query Engine Throughput | – | 700 M ops/s | 750 M ops/s | 800 M ops/s | 800 M ops/s | **814,5 M ops/s** | **≥ 900 M ops/s** | +16 % | ❓ |
| Vector Insert | – | 280 k/s | 300 k/s | 330 k/s | 340 k/s | **351,4 k/s** | **≥ 600 k/s** | +25 % | ❓ |
| Secondary Index Insert | – | 180 k/s | 190 k/s | 210 k/s | 215 k/s | **217,2 k/s** | **≥ 1 M/s** | +21 % | ❓ |
| Embedding Cache Hit-Rate | – | – | – | – | – | **155,8 M/s** | **≥ 200 M/s** | n/a | ❓ |
| 2PC Throughput | – | – | – | – | – | **29,3 k/s** | **≥ 10 k/s** | n/a | ✅ |
| Graph Edge Ops | – | – | – | – | – | **628,7 k/s** | **≥ 1 M/s** | n/a | ❓ |
| Timeseries Insert | – | – | – | – | – | **49,0 M pts/s** | **≥ 60 M pts/s** | n/a | ❓ |
| Gesamt Benchmark-Tests | – | 450 | 480 | 520 | 780 | **1.078** | **≥ 1.200** | +140 % | ✅ |

---

## 2. Query-Engine – Detailergebnisse

> Quelle: `BENCHMARK_RESULTS.md` (Run 2025-12-18), `benchmark_summary.csv` (Run 2025-12-29)

| Benchmark | Ziel | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|--------|
| Simple AQL WHERE | ≥ 10.000 Queries/s bei P99 < 20 ms | 3,43 M ops/s @ ~0,3 µs | ✅ |
| Complex WHERE | ≥ 1 M ops/s | 3,35 M ops/s | ✅ |
| JOIN (Users-Posts) | ≥ 5 M ops/s | 10,2 M ops/s | ✅ |
| QueryEngineBench/SimpleEvaluation | ≥ 750 M items/s | 814,5 M items/s (1,23 ns) | ✅ |
| Parse + Optimize P99 (≤10 Collections) | ≤ 5 ms | ❓ | ❓ |
| Query-Cache Lookup P99 (Exact) | < 1 ms | ❓ | ❓ |
| Query-Cache Lookup P99 (Semantic) | ≤ 10 ms | ❓ | ❓ |
| JIT Erstcompilierung | ≤ 50 ms | ❓ | ❓ |
| Federation Plan-Overhead (5 Cluster) | ≤ 20 ms | ❓ | ❓ |
| Streaming First-Chunk Latenz | ≤ 50 ms | ❓ | ❓ |

---

## 3. Index-Modul

> Quelle: `benchmark_summary.csv` (Run 2025-12-29), `baselines/acceleration/baseline.json` (v1.0.0)

| Benchmark | Ziel | v1.0.0 Gemessen | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|-----------------|--------|
| VectorIndexBench/InsertPlaintext | ≥ 280 k/s | – | 351,4 k/s (2,84 µs) | ✅ |
| SecondaryIndexBench/IndexInsert | ≥ 180 k/s | – | 217,2 k/s (4,60 µs) | ✅ |
| SecondaryIndexBench/RawWriteOnly | ≥ 500 k/s | – | 885,0 k/s (1,13 µs) | ✅ |
| Small Index Insert (1K entities) | ≥ 1 M/s | – | 1,75 M/s | ✅ |
| Medium Index Insert (100K) | ≥ 500 k/s | – | 1,06 M/s | ✅ |
| Large Index Lookup (1M) | ≥ 1 M/s | – | 3,12 M/s | ✅ |
| Composite Index Lookup | ≥ 1 M/s | – | 2,40 M/s | ✅ |
| L2Distance/1000/512 | ≥ 250 k/s | 313 k/s (3.200 ns) | ❓ | ✅ |
| CosineDistance/1000/512 | ≥ 200 k/s | 250 k/s (4.000 ns) | ❓ | ✅ |
| TopK/5000/50 | ≥ 10 M/s | 12,5 M/s (400 ns) | ❓ | ✅ |
| HNSW Vektor-Suche (CPU) | ≥ 5.000 QPS | ❓ | ❓ | ❓ |
| HNSW Vektor-Suche (GPU RTX-class) | ≥ 50.000 QPS | ❓ | ❓ | ❓ |
| B-Tree Point-Lookup P99 (10M Keys) | < 500 µs | ❓ | ❓ | ❓ |
| R-Tree Spatial Range Query P99 | < 10 ms | ❓ | ❓ | ❓ |
| GPU Index-Build (1M × 128-dim) | < 60 s | ❓ | ❓ | ❓ |
| RocksDB WriteBatch Commit P99 | < 2 ms | ❓ | ❓ | ❓ |

---

## 4. Cache-Modul

> Quelle: `FUTURE_ENHANCEMENTS.md`; kein direkter Bench-Run gefunden

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| C-1 L1 Hit-Path | ≥ 5 M ops/s/Core (16-Thread) | ❓ | ❓ |
| C-2 L2 Hit-Path | ≥ 500 k ops/s | ❓ | ❓ |
| C-3 L3 Hit-Path P99 | ≤ 5 ms | ❓ | ❓ |
| C-4 Warmup Throughput | ≥ 500 k Entries/s | ❓ | ❓ |
| C-5 Admin-API Response | ≤ 5 ms | ❓ | ❓ |
| C-6 Prefetch Latenz | ≤ 100 µs/Call | ❓ | ❓ |
| C-7 Prefetch Overfetch | ≤ 10 % | ❓ | ❓ |

---

## 5. Storage-Modul

> Quelle: `scientific_benchmarks_20251204_212220/summary.csv` (v1.0.0, HTTP-API-Level)

| Benchmark | Ziel | v1.0.0 Gemessen | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|-----------------|--------|
| INSERT 1 KB | – | 759 ops/s @ 1,317 ms | ❓ | ❓ |
| READ 1 KB | – | 834 ops/s @ 1,204 ms | ❓ | ❓ |
| UPDATE 1 KB | – | 806 ops/s @ 1,240 ms | ❓ | ❓ |
| INSERT 10 KB | – | 510 ops/s @ 1,959 ms | ❓ | ❓ |
| INSERT 100 KB | – | 126 ops/s @ 7,913 ms | ❓ | ❓ |
| INSERT 1 MB | – | 16 ops/s @ 61,402 ms | ❓ | ❓ |
| Concurrent 1 Client | – | 776 ops/s @ 1,28 ms | ❓ | ❓ |
| Concurrent 5 Clients | – | 721 ops/s @ 6,80 ms | ❓ | ❓ |
| Concurrent 50 Clients | – | 948 ops/s @ 60,3 ms ⚠️ CV=38% | ❓ | ❓ |
| Sustained Write NVMe | ≥ 100.000 ops/s | – | ❓ | ❓ |
| Point-Read Latenz P99 | ≤ 1 ms (Bloom Filter) | – | ❓ | ❓ |
| Incremental Backup | ≥ 500 MB/s | – | ❓ | ❓ |
| 1MB Blob Storage | – | – | 741 ops/s @ 1,39 ms ⚠️ | ⚠️ |
| 10KB Thumbnail Storage | – | – | 388,5 k blobs/s | ❓ |
| 100KB Blob Retrieval | – | – | 49,0 M lookups/s | ❓ |

---

## 6. Analytics-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| AN-1 Streaming Aggregation Memory | ≤ 512 MB/Fenster | ❓ | ❓ |
| AN-2 IVM Delta-Application | ≤ 50 ms (10k Rows) | ❓ | ❓ |
| AN-3 Parquet Export 1M Rows | ≤ 2 s | ❓ | ❓ |
| AN-4 CSV Export 1M Rows | ≤ 500 ms | ❓ | ❓ |
| AN-5 CEPEngine::stop() | ≤ 100 ms | ❓ | ❓ |
| AN-7 IsolationForest Training | ≤ 10 ms (1k-Punkt-Fenster) | ❓ | ❓ |
| AN-8 predictBatch() | ≤ 50 ms (1k Serien × 30 Steps) | ❓ | ❓ |
| AN-9 Auto-Tune Grid | ≤ 5 ms (9 α, n=500, parallel) | ❓ | ❓ |
| AN-10 ARM NEON Aggregation | ≥ 4 GB/s (Cortex-A78) | ❓ | ❓ |

---

## 7. Timeseries-Modul

> Quelle: `FUTURE_ENHANCEMENTS.md` (explizite Ist-Stand-Angaben)

| Ziel-ID | Erwartungswert | Bekannter Ist-Stand | v1.3.4 Gemessen | Status |
|---------|----------------|---------------------|-----------------|--------|
| TS-1 Write Throughput/Node | > 500 k pts/s | ~200 k pts/s | 49,0 M pts/s* | ⚠️ |
| TS-2 Gorilla Decode Throughput | > 2 GB/s/Core | ~400 MB/s | ❓ | ⚠️ |
| TS-3 Range Scan P99 (1M pts) | < 50 ms | – | ❓ | ❓ |
| TS-4 Continuous Aggregate Refresh | < 500 ms/1-min-Intervall | – | ❓ | ❓ |
| TS-5 Write Amplification | < 1,5× | – | ❓ | ❓ |
| TS-6 Downsampling Throughput | > 10 M pts/s → 1-min-Aggregate | – | ❓ | ❓ |
| TS-7 Storage Reduction | > 50× (raw → 1-day Tier) | – | ❓ | ❓ |
| TS-9 Buffer-to-Storage Flush P99 | < 10 ms | – | ❓ | ❓ |
| TS-10 Gorilla Insert P99 | ≤ 50 µs | – | ❓ | ❓ |
| TS-11 AES-256-GCM Throughput | > 1 GB/s/Core (AES-NI) | – | ❓ | ❓ |

*`TimeseriesBench/InsertTimepoints` 49,0 M/s misst In-Memory-Append, nicht persistiertes Schreiben

---

## 8. Geo-Modul

> Quelle: `baselines/acceleration/baseline.json` (v1.0.0)

| Benchmark | Ziel | v1.0.0 Gemessen | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|-----------------|--------|
| Geo_HaversineDistance/100000 | ≥ 20 M/s | 22,2 M/s (4.500 ns) | ❓ | ✅ |
| Geo_PointInPolygon/100000 | ≥ 30 M/s | 35,7 M/s (2.800 ns) | ❓ | ✅ |
| intersects-Query P99 (1M Punkte) | ≤ 5 ms (R-Tree) | – | ❓ | ❓ |
| R-Tree Bulk-Load (1M Geometrien) | ≤ 3 s | – | ❓ | ❓ |
| Buffer 10K Punkte @ 500 m | ≤ 200 ms/Core | – | ❓ | ❓ |
| Spatial JOIN (2×100K, 1 km) | ≤ 500 ms (erste 1k Ergebnisse) | – | ❓ | ❓ |
| GeoJSON Parse (100K MultiPolygon) | ≤ 2 s | – | ❓ | ❓ |
| GPU Contains (1M Punkte, A10G) | ≤ 50 ms | – | ❓ | ❓ |
| DBSCAN GPU Speedup (100K Punkte) | > 100× vs. CPU | – | ❓ | ❓ |

---

## 9. Graph-Modul

> Quelle: `BENCHMARK_RESULTS.md` (Run 2025-12-18)

| Benchmark | Ziel | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|--------|
| GraphIndexBench/AddEdges | ≥ 500 k edges/s | 628,7 k edges/s (1,59 µs) | ✅ |
| Sparse Graph Edge Addition | ≥ 500 k edges/s | 1,26 M edges/s | ✅ |
| Dense Graph Neighbor Query | ≥ 5 M queries/s | 8,96 M queries/s | ✅ |
| Graph BFS Traversal (Depth-3) | ≥ 5 M traversals/s | 9,56 M traversals/s | ✅ |
| RAG Search Top-50 | ≥ 5 M ops/s | 7,17 M ops/s (140 ns) | ✅ |
| Algorithmus-Selektion P99 (10M Nodes) | < 1 ms | ❓ | ❓ |
| Plan-Cache Lookup P99 | < 100 µs | ❓ | ❓ |
| Single-Refresh (10K Nodes) | ≤ 5 s / ≤ 200 ms (8 Worker) | ❓ | ❓ |
| Subgraph-Isomorphismus P95 | < 500 ms (100-Node-Pattern, 1M-Graph) | ❓ | ❓ |

---

## 10. Acceleration-Modul

> Quelle: `baselines/acceleration/baseline.json` (v1.0.0)

| Benchmark | Ziel | v1.0.0 Gemessen | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|-----------------|--------|
| L2Distance/1000/64 | ≥ 1,5 M/s | 2,0 M/s (500 ns) | ❓ | ✅ |
| L2Distance/1000/512 | ≥ 250 k/s | 313 k/s (3.200 ns) | ❓ | ✅ |
| CosineDistance/1000/512 | ≥ 200 k/s | 250 k/s (4.000 ns) | ❓ | ✅ |
| InnerProduct/1000/512 | ≥ 250 k/s | 313 k/s (3.200 ns) | ❓ | ✅ |
| TopK/1000/10 | ≥ 15 M/s | 20,0 M/s (50 ns) | ❓ | ✅ |
| TopK/5000/50 | ≥ 10 M/s | 12,5 M/s (400 ns) | ❓ | ✅ |
| Vec Search L2 CUDA (1M×128-dim) | < 8 ms auf RTX 3090 | – | ❓ | ❓ |
| GPU Throughput | ≥ 10× CPU AVX2 Baseline | – | ❓ | ❓ |
| Large-Scale (100M×128, 4×A100 80 GB) | P99 < 15 ms k=100 | – | ❓ | ❓ |
| INT8 Matmul vs. FP16 | ≥ 2× auf RTX 3090 | – | ❓ | ❓ |
| Vulkan (Apple M2, 500K×128) | < 20 ms | – | ❓ | ❓ |

---

## 11. Replication-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| R-1 Replikations-Lag P99 (SEMI_SYNC) | ≤ 50 ms @ 10k Writes/s (LAN) | ❓ | ❓ |
| R-2 WAL-Shipping Throughput (Zstd L3) | ≥ 500 MB/s/Follower (10 GbE) | ❓ | ❓ |
| R-3 Leader-Failover | ≤ 10 s | ❓ | ❓ |
| R-4 HLC Conflict Detection | < 5 µs/Write | ❓ | ❓ |
| R-5 CRDT Merge | ≤ 1 µs/Merge | ❓ | ❓ |
| R-6 WAL Replay (PITR, 100 GB) | ≥ 200 MB/s; ≤ 10 min | ❓ | ❓ |
| R-7 CDC Event P99 | ≤ 1 ms (Commit → CDC Queue) | ❓ | ❓ |
| R-8 Cross-DC Lag ASYNC | ≤ 200 ms P99 (50 ms RTT WAN) | ❓ | ❓ |

---

## 12. Sharding-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| SH-1 Cross-Shard RPC P99 (LAN) | < 5 ms | ❓ | ❓ |
| SH-2 Connection-Pool Hit-Rate | > 95 % @ 10k RPS | ❓ | ❓ |
| SH-3 Percolator Commit P99 (10 Shards) | < 20 ms | ❓ | ❓ |
| SH-4 Shard-Split Migration Downtime | 0 ms Read-Unavailability | ❓ | ❓ |
| SH-5 Write-Latenz während Migration | < 20 % über Baseline P99 | ❓ | ❓ |
| SH-6 Rebalancer Decision Cycle | < 10 s | ❓ | ❓ |
| SH-7 Anti-Entropy Scan Throughput | > 1 GB/s (NVMe, 8 Worker) | ❓ | ❓ |
| SH-8 GPU Reed-Solomon | > 4 GB/s (NVIDIA A10) | ❓ | ❓ |
| SH-9 Snapshot (1 GB Raft-State) | < 10 s | ❓ | ❓ |
| SH-10 Snapshot Kompressionsrate | < 35 % unkomprimiert (ZSTD L3) | ❓ | ❓ |
| SH-11 Replica Catch-up | > 200 MB/s (10 GbE LAN) | ❓ | ❓ |
| SH-12 Topology Change Propagation | < 500 ms (100 Nodes, Gossip) | ❓ | ❓ |

---

## 13. Transaction-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| TX-1 OCC Commit P50 | 100 µs | ❓ | ❓ |
| TX-2 OCC Commit P99 | 5 ms | ❓ | ❓ |
| TX-3 2PC Throughput | ≥ 10 k/s | 29,3 k/s | ✅ |
| TX-4 2PC Latenz (5 Shards) | 5 ms | 0,13 ms | ✅ |
| TX-5 SAGA Compensation Time | 20 ms | ❓ | ❓ |
| TX-6 Deadlock Detection Overhead | 1 % (von 5 % verbessert) | ❓ | ❓ |
| TX-7 False Positive Rate | < 5 % | ❓ | ❓ |
| TX-8 Low-Contention Success Rate | > 90 % | ❓ | ❓ |

---

## 14. LLM-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| L-1 Time-to-First-Token (512-Token, A10G) | ≤ 200 ms P99 | ❓ | ❓ |
| L-2 Streaming Overhead | ≤ 2 % tokens/s Regression | ❓ | ❓ |
| L-3 LoRA Adapter Hot-Load (7B, Rank 64) | ≤ 5 s Wall-Clock | ❓ | ❓ |
| L-4 Adapter Serialisierung | ≤ 2 ms | ❓ | ❓ |
| L-5 Work-Stealing Dispatch P99 | ≤ 50 µs | ❓ | ❓ |
| L-6 Speculative Decoding Overhead | ≤ 15 % akzeptierter Token-Latenz | ❓ | ❓ |
| L-7 GPU Utilization (Mixed Workloads) | ≥ 10 % Verbesserung | ❓ | ❓ |
| L-8 Speculative Decoding Throughput | ≥ 2× tokens/s (7B + 0,5B Draft) | ❓ | ❓ |

---

## 15. RAG-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| RA-1 Fast Evaluation P99 | ≤ 100 ms E2E | ❓ | ❓ |
| RA-2 Balanced Evaluation P99 | ≤ 500 ms E2E | ❓ | ❓ |
| RA-3 Thorough Evaluation P99 | ≤ 2.000 ms E2E | ❓ | ❓ |
| RA-4 HybridRetriever Recall@10 | ≥ 85 % (BEIR NQ) | ❓ | ❓ |
| RA-5 CrossEncoderReranker MRR@10 | ≥ +10 % vs. BM25 | ❓ | ❓ |
| RA-6 StreamingRetriever First-Chunk | ≤ 50 ms | ❓ | ❓ |
| RA-7 Bayesian Optimizer Konvergenz | ≥ 90 % opt. F1 in 200 Events | ❓ | ❓ |
| RA-8 ClaimExtractor (1k Zeichen) | ≤ 500 ms LLM / ≤ 50 ms Heuristic | ❓ | ❓ |

---

## 16. Search-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| SE-1 Hybrid Search P99 (10M-Doc-Index) | ≤ 20 ms (BM25 + HNSW RRF, Top-10) | ❓ | ❓ |
| SE-2 SPLADE Index Memory | ≤ 4 GB / 10M-Doc (CSR) | ❓ | ❓ |
| SE-3 Facet Counting (1k distinct, 100k Docs) | ≤ 5 ms | ❓ | ❓ |
| SE-4 LTR Re-Ranking (Top-100) | ≤ 2 ms | ❓ | ❓ |
| SE-5 Autocomplete P99 (1M-Term-Dict) | ≤ 5 ms | ❓ | ❓ |
| SE-6 LLM Query Rewriter Timeout | 200 ms + Fallback | ❓ | ❓ |

---

## 17. Temporal-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| TM-1 History-Table Write Overhead | < 15 % vs. Baseline | ❓ | ❓ |
| TM-2 Time-Travel Query | 80–95 % Current-Table-Speed | ❓ | ❓ |
| TM-3 AS OF Query | 80–95 % Current-Table-Speed | ❓ | ❓ |
| TM-4 Retention Enforcement/Batch | ≤ 100 ms | ❓ | ❓ |
| TM-5 Conflict Resolution | < 10 ms | ❓ | ❓ |
| TM-6 Temporal Join Overhead | ≤ 50 % vs. Non-Temporal | ❓ | ❓ |

---

## 18. API-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| API-1 GraphQL Parse+Execute P99 | < 2 ms (10-Feld-Query, 500 HTTP/2) | ❓ | ❓ |
| API-2 WebSocket Subscription Latenz | < 50 ms (Changefeed → Frame) | ❓ | ❓ |
| API-3 Concurrent WebSocket Connections | ≥ 10k / Node bei < 50 MB RSS | ❓ | ❓ |
| API-4 Bulk Insert (10k Docs) | < 500 ms E2E | ❓ | ❓ |
| API-5 Middleware Overhead | < 10 µs/Request | ❓ | ❓ |
| API-6 Span Enqueue (Hot Path) | < 500 ns/Call | ❓ | ❓ |
| API-7 OTLP Flush (64 Spans) | < 5 ms E2E | ❓ | ❓ |

---

## 19. Auth-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| AUT-1 LDAP Bind P99 | ≤ 50 ms sichtbar (< 200 ms Backend) | ❓ | ❓ |
| AUT-2 LDAP Auth (unter Load) | < 5 ms avg (von ~30 ms via Conn-Reuse) | ❓ | ❓ |
| AUT-3 JWT JWKS Refresh Blocking | ≤ 1 ms auf Validation Hot Path | ❓ | ❓ |
| AUT-4 Token Revocation Lookup | ≤ 1 µs (Bloom Filter, warm) | ❓ | ❓ |
| AUT-5 Redis Token Revocation P99 | ≤ 2 ms auf LAN | ❓ | ❓ |

---

## 20. CDC-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| CDC-1 Concurrent WebSocket Connections | ≥ 5k / Node bei < 100 MB RSS | ❓ | ❓ |
| CDC-2 Event Delivery P99 | < 20 ms (Emit → Frame) | ❓ | ❓ |
| CDC-3 Consumer Group Offset Commit | < 1 ms P99 (RocksDB) | ❓ | ❓ |
| CDC-4 Resume nach 24h Offline (10M Events) | < 5 s bis zur Delivery | ❓ | ❓ |
| CDC-5 End-to-End Latenz (→ Kafka Ack) | < 10 ms P99 (LAN) | ❓ | ❓ |
| CDC-6 Log Compaction (1M Events) | < 30 s (Background) | ❓ | ❓ |

---

## 21. Network-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| NET-1 TCP Wire Protocol Throughput | ≥ 100k req/s/Core (128B, kein TLS) | ❓ | ❓ |
| NET-2 TLS 1.3 Handshake P99 | < 5 ms (neue Verbindungen) | ❓ | ❓ |
| NET-3 TLS 1.3 Session Resumption P99 | < 1 ms | ❓ | ❓ |
| NET-4 WebSocket Round-Trip P99 | < 2 ms (localhost) | ❓ | ❓ |
| NET-5 QUIC 0-RTT Resumption P99 | < 2 ms | ❓ | ❓ |
| NET-6 UDP Fast-Path GET P99 | < 500 µs (localhost) | ❓ | ❓ |

---

## 22. Security-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| SEC-1 AES-256-GCM (AES-NI) | ≥ 1 GB/s/Core | ❓ | ❓ |
| SEC-2 RSA-4096 Signaturprüfung P99 | ≤ 5 ms | ❓ | ❓ |
| SEC-3 Kyber-1024 Key Encapsulation | ≥ 2k ops/s/Core | ❓ | ❓ |
| SEC-4 Dilithium-5 Signing | ≥ 1k ops/s/Core | ❓ | ❓ |
| SEC-5 TLS 1.3 Handshake P99 | ≤ 10 ms (neue Verbindungen) | ❓ | ❓ |
| SEC-6 RBAC Policy Eval (≤100 Rollen) P99 | ≤ 0,5 ms | ❓ | ❓ |
| SEC-7 HSM-Backed RSA-2048 Sign P99 | ≤ 20 ms (SoftHSM2) | ❓ | ❓ |
| SEC-8 Audit Log Write P99 | ≤ 2 ms/Entry | ❓ | ❓ |

---

## 23. Scheduler-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| SCH-1 Scheduler Loop Tick P99 | ≤ 1 ms (10k Tasks) | ❓ | ❓ |
| SCH-2 Task Dispatch P99 | ≤ 5 ms (Due-Time → First Instruction) | ❓ | ❓ |
| SCH-3 Cron next_execution | ≤ 10 µs/Call | ❓ | ❓ |
| SCH-4 Leader Election Konvergenz | ≤ 5 s (5-Node-Cluster, nach Failure) | ❓ | ❓ |
| SCH-5 DAG Topological Sort | ≤ 1 ms (≤10k Nodes) | ❓ | ❓ |
| SCH-6 Throughput | ≥ 5k Dispatches/s | ❓ | ❓ |

---

## 24. Ingestion-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| ING-1 Aggregate Throughput | ≥ 50k Docs/s (Single Node) | ❓ | ❓ |
| ING-2 Kafka Consumer Throughput | ≥ 100k Messages/s (1 KB avg) | ❓ | ❓ |
| ING-3 Kafka → Document E2E P99 | ≤ 500 ms | ❓ | ❓ |
| ING-4 S3 Concurrent Download | ≥ 200 MB/s agg. (4 parallel, 10 Gbps) | ❓ | ❓ |
| ING-5 Quarantine Queue Scan (100k) | ≤ 1 s | ❓ | ❓ |

---

## 25. Governance-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| GOV-1 Policy Reload Latenz | ≤ 100 ms (Detection → Aktiv) | ❓ | ❓ |
| GOV-2 CCPA Opt-Out Lookup Overhead | ≤ 0,5 ms P99 | ❓ | ❓ |
| GOV-3 CCPA Report (90 Tage, 1M Subjects) | ≤ 10 s | ❓ | ❓ |
| GOV-4 Policy Evaluation P99 (500 Rules) | ≤ 5 ms (100 Threads) | ❓ | ❓ |
| GOV-5 DataMasker (50-Feld-Dokument) | ≤ 1 ms | ❓ | ❓ |

---

## 26. Observability-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| OBS-1 Metrics Collection Overhead | < 1 % CPU @ 1k req/s | ❓ | ❓ |
| OBS-2 Adaptive Span Sampling | ≤ 1 % bei > 10k Spans/s | ❓ | ❓ |
| OBS-3 Metrics Scrape (16 Scraper) | ≥ 3× vs. Exclusive Mutex | ❓ | ❓ |

---

## 27. Process-Modul (Process Mining)

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| PROC-1 ProcessGraphRag::retrieve() | ≤ 200 ms (≤500 Nodes, exkl. LLM) | ❓ | ❓ |
| PROC-2 PPR (50 Iter., 500-Node-Graph) | ≤ 20 ms | ❓ | ❓ |
| PROC-3 Object-Centric DFG (10k Events) | ≤ 5 s | ❓ | ❓ |
| PROC-4 Total Conversation Latenz | ≤ 5 s (3-Turn, local llama.cpp 8B Q4) | ❓ | ❓ |
| PROC-5 CEP Alert Latenz | ≤ 100 ms nach Threshold-Überschreitung | ❓ | ❓ |
| PROC-6 Bottleneck Analysis (10k Instances) | ≤ 2 s | ❓ | ❓ |
| PROC-7 Bottleneck Detection Accuracy | ≥ 90 % | ❓ | ❓ |

---

## 28. Voice-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| VOI-1 STT Latenz P95 (5 s Audio) | ≤ 300 ms | ❓ | ❓ |
| VOI-2 TTS First-Token Latenz | ≤ 200 ms | ❓ | ❓ |
| VOI-3 Wake-Word Detection | ≤ 50 ms | ❓ | ❓ |
| VOI-4 End-to-End Voice Latenz | < 500 ms | ❓ | ❓ |
| VOI-5 Wake-Word CPU Usage (idle) | ≤ 2 % auf x86_64 | ❓ | ❓ |
| VOI-6 Concurrent WebSocket Sessions | ≥ 100 | ❓ | ❓ |
| VOI-7 Speaker ID Acceptance | ≥ 95 % | ❓ | ❓ |
| VOI-8 Speaker ID Impostor Rejection | ≥ 99 % | ❓ | ❓ |
| VOI-9 Wake-Word False-Positive Rate | ≤ 1/Stunde | ❓ | ❓ |
| VOI-10 Silence Removal | 20–40 % Reduktion | ❓ | ❓ |

---

## 29. ONNX-CLIP-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| OC-1 Batched Inference (Batch 64) | ≥ 6× vs. Sequential | ❓ | ❓ |
| OC-2 ViT-B/32 CUDA (Batch 64) | ≤ 20 ms (≤ 0,31 ms/Image) | ❓ | ❓ |
| OC-3 ViT-B/32 CPU (Batch 16) | ≤ 2,5 s | ❓ | ❓ |
| OC-4 Text Encoding P95 (CPU) | ≤ 5 ms | ❓ | ❓ |
| OC-5 Metrics Overhead | ≤ 0,05 ms/Call | ❓ | ❓ |

---

## 30. Chimera-Baseline & Suite

> **CHIMERA** = Comprehensive, Honest, Impartial Metrics for Empirical Reporting and Analysis  
> Framework: `benchmarks/chimera/` (v1.0.0) · Standard: IEEE Std 2807-2022, ISO/IEC 14756:2015  
> Vollständige Dokumentation: `benchmarks/chimera/CHIMERA_README.md`

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

### 30.2 Chimera Suite – Standardisierte Workloads (Benchmark-Definitionen)

> Quelle: `benchmarks/chimera/benchmark_config_schema.yaml`  
> Methodik: IEEE Std 2807-2022 · Warmup: 60 s · Messdauer: 300 s · Runs: 5 · Konfidenz: 95 %

| Workload-ID | Familie | Standard | Beschreibung | Ziel-Modul(e) |
|-------------|---------|----------|--------------|---------------|
| `ycsb_workload_a` | YCSB | Cooper2010 | Update Heavy (50 % Reads, 50 % Updates), 1 M Records, Zipfian | Storage, Cache, Transaction |
| `tpc_c` | TPC-C | TPC-C v5.11 | OLTP Order-Entry, 10 Warehouses, 300 s, New-Order 45 % | Transaction, Query, Storage |
| `tpc_h_sf1` | TPC-H | TPC-H v3.0.0 | Decision Support, Scale Factor 1 GB, Queries 1/2/3/6/14 | Analytics, Query |
| `ann_sift1m` | ANN-Benchmarks | Aumüller2020 | SIFT1M (1 M × 128-dim), k=10, Recall-Ziel 0.95 | Index/HNSW, Acceleration |
| `ldbc_snb_interactive` | LDBC-SNB | Erling2020 | Social Network Graph, SF1, Short+Complex Reads + Updates | Graph, Query |
| `vllm_serving` | vLLM | Kwon2023 | LLM Inference, Llama-2-7B, 512-Token Input, 1 req/s | LLM, Acceleration |
| `rag_qa` | RAGBench | Chen2024 | RAG E2E, NaturalQuestions, Top-5 Dense Retrieval | RAG, Search, LLM |

---

### 30.3 Chimera Vendorneutrale Demo-Ergebnisse (anonymisiert)

> Quelle: `benchmarks/chimera/demo_reports/benchmark_comparison.csv`  
> Methodik: 28–50 Stichproben/System, Ausreißer per IQR (1.5×) entfernt, 95 % CI

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

> **Hinweis:** System-Namen sind anonymisiert (IEEE-konforme Neutralität). ThemisDB kann als eines dieser Systeme identifiziert werden sobald ein Chimera-Zertifizierungslauf abgeschlossen ist.

---

### 30.4 Chimera Statistische Methodik

| Parameter | Wert | Referenz |
|-----------|------|----------|
| Signifikanzniveau (α) | 0,05 | Standard |
| Konfidenzintervall | 95 % | Welch's t-test |
| Hypothesentests | Welch's t-test, Mann-Whitney U, KS-Test | Welch 1947, Mann 1947 |
| Effektgröße | Cohen's d | Cohen 1988 |
| Ausreißer-Methode | IQR × 1.5 | Tukey 1977 |
| Min. Stichprobengröße | 30 | IEEE Std 2807-2022 |
| Warmup | 60 s | IEEE Std 2807-2022 |
| Messdauer | 300 s | IEEE Std 2807-2022 |
| Runs (unabhängig) | 5 | IEEE Std 2807-2022 |

---

## 31. Prompt Engineering-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| PE-1 Prompt Construction P99 | ≤ 5 ms | ❓ | ❓ |
| PE-2 Template Compilation (4 KB) | < 50 ms | ❓ | ❓ |
| PE-3 Compiled Template Render P99 (2 KB) | < 1 ms | ❓ | ❓ |
| PE-4 CoT Tracing Overhead/Step | < 0,2 ms | ❓ | ❓ |
| PE-5 Full 3-Iteration Reflection (kein LLM) | < 1 ms P99 | ❓ | ❓ |
| PE-6 render() Latenz (String → Compiled) | ~8 ms → < 1 ms Ziel | ❓ | ❓ |
| PE-7 End-to-End RAG Assembly | ~15 ms → < 5 ms Ziel | ❓ | ❓ |

---

## 32. Ethics AI-Modul

| Ziel-ID | Erwartungswert | v1.3.4 Gemessen | Status |
|---------|----------------|-----------------|--------|
| ETH-1 Single Argument Generation P95 | ≤ 3 s (LLM, 500 Token) | ❓ | ❓ |
| ETH-2 Batch 5 Arguments (parallel, 5 Schulen) | ≤ 8 s | ❓ | ❓ |
| ETH-3 Embedding Latenz (512-Token, CPU) | ≤ 20 ms | ❓ | ❓ |
| ETH-4 Batch 10 Queries | ≤ 150 ms | ❓ | ❓ |
| ETH-5 Multi-Round Debate/Runde | ≤ 5 s inkl. LLM | ❓ | ❓ |
| ETH-6 Metrics Overhead/Decision | ≤ 0,1 ms | ❓ | ❓ |

---

## 33. System-Level (TPC/YCSB-Standards)

> Quelle: `benchmarks/README.md`, `COMPETITOR_COMPARISON.csv` (v1.3.4)

| # | Workload | Erwartungswert | Hardware-Referenz | v1.3.4 Gemessen | Status |
|---|----------|----------------|-------------------|-----------------|--------|
| BM-1 | OLTP (TPC-C) | 200–300 K ops/s | 4-Core, 8 GB, SSD | ❓ | ❓ |
| BM-2 | OLTP (TPC-C) | 400–600 K ops/s | 8-Core, 16 GB, NVMe | ❓ | ❓ |
| BM-3 | OLTP (TPC-C) | 700 K–1 M ops/s | 16-Core, 32 GB, NVMe | ❓ | ❓ |
| BM-4 | OLTP (TPC-C) | 1,2–1,8 M ops/s | 32-Core, 64 GB, NVMe Gen4 | ❓ | ❓ |
| BM-5 | OLAP (TPC-H) | 100–200 Queries/min | 8-Core, 16 GB, NVMe | ❓ | ❓ |
| BM-6 | Vector Search | 10–20 K QPS | 8-Core, 16 GB, NVMe | ❓ | ❓ |
| BM-7 | TPC-C tpmC-Ziel | 150–200 K tpmC (80–100 % PostgreSQL) | 8-Core, 32 GB, NVMe | ❓ | ❓ |

**Competitor-Vergleich v1.3.4 (gemessen):**

| Kategorie | ThemisDB v1.3.4 | Bester Mitbewerber | Mitbewerber | Position | Delta |
|-----------|----------------|--------------------|-------------|----------|-------|
| Query Engine (OLAP) | 814,5 M items/s | 1.200 M items/s | ClickHouse | 2. (Sehr gut) | −47 % |
| Vector Insert | 351,4 k items/s | 600 k items/s | FAISS | 3. (Kompetitiv) | −71 % |
| Embedding Cache Hit | 155,8 M items/s | 1.000 M items/s | In-Memory Cache | 2. (Sehr gut) | Akzeptabel |
| 2PC Throughput | 29,3 k items/s | 15 k items/s | TiDB 7.0 | 1. (Führend) | **+95 %** |
| Hybrid Search | 450 queries/s | 500 queries/s | Weaviate | 2. (Stark) | −10 % |

---

## 34. Performance Regression CI – Schwellwerte

> CI-Datei: `.github/workflows/05-quality_build_cross-module-performance-regression-ci.yml`

| Level | Schwellwert | Auswirkung |
|-------|-------------|------------|
| Minor | ≥ 5 % | Tracking / informell |
| **Major** | **≥ 10 %** | **Blockiert PR-Merge** |
| Critical | ≥ 20 % | Sofortiger Eingriff |

---

## 35. Bekannte Performance-Lücken (explizit dokumentiert)

| # | Modul | Ist-Stand | Ziel | Δ | Priorität |
|---|-------|-----------|------|---|-----------|
| D-1 | Timeseries Write (TS-1) | ~200 k pts/s | > 500 k pts/s | **−60 %** | Hoch |
| D-2 | Gorilla Decode (TS-2) | ~400 MB/s | > 2 GB/s | **−80 %** | Hoch |
| D-3 | Vector Insert vs. FAISS | 351 k/s | 600 k/s | **−71 %** | Mittel |
| D-4 | 2PC Throughput vs. TiDB | ~~6,4 k/s~~ **29,3 k/s** | 15 k/s | **+95 %** ✅ | Mittel |
| D-5 | Storage 1 MB Blob Write | 741 ops/s | ≥ 100 k ops/s | **−99 %** | Hoch |
| D-6 | Concurrency 10 Clients CV | CV=20,74 ⚠️ | stabil | Instabil | Mittel |
| D-7 | Query Engine vs. ClickHouse | 814,5 M/s | 1.200 M/s | **−47 %** | Niedrig |

---

*Dieses Dokument wird automatisch aus den FUTURE_ENHANCEMENTS.md und Benchmark-Ergebnissen der jeweiligen Module generiert. Für Aktualisierungen bitte die entsprechenden Quelldateien pflegen.*

---

## 36. Versionsübergreifende Benchmark-Messwerte (Rohdaten)

> Alle Werte aus Google Benchmark (C++). `real_time` = Wall-Clock, `cpu_time` = CPU-Zeit.
> Run-IDs: **v1.3.0** = 20251223_084034 | **v1.3.3** = 20251223_085556 | **v1.3.4** = 20251229_184507

---

### 36.1 Kern-Performance (`bench_core_performance`)

| Benchmark | v1.3.0 items/s | v1.3.3 items/s | v1.3.4 items/s | Δ v1.3.0→v1.3.4 | Status |
|-----------|---------------|---------------|---------------|-----------------|--------|
| VectorIndexBench/InsertPlaintext | 566.7 k/s | 538.0 k/s | **351.4 k/s** | −38 % | ⚠️ |
| SecondaryIndexBench/IndexInsert | 1.78 M/s | 5.11 k/s ⚠️ | **217.2 k/s** | −88 % | ❌ |
| SecondaryIndexBench/RawWriteOnly | – | – | **885.0 k/s** | n/a | ❓ |
| QueryEngineBench/SimpleEvaluation | 968.6 M/s | 949.8 M/s | **814.5 M/s** | −16 % | ⚠️ |
| GraphIndexBench/AddEdges | 1.47 M/s | 1.20 M/s | **628.7 k/s** | −57 % | ❌ |
| TimeseriesBench/InsertTimepoints | 61.0 M/s | 55.9 M/s | **49.0 M/s** | −20 % | ⚠️ |

> **Hinweis SecondaryIndex v1.3.3:** real_time=656 ms, cpu_time=19.6 ms → 33× Diskrepanz durch Einzel-Transaktion pro `put()` (RocksDB-Transaktions-Overhead). Bekannte Regression, dokumentiert in `PERFORMANCE_COMPARISON_V1.3.0_VS_V1.3.3.md`.

---

### 36.2 Umfassende Workloads (`bench_comprehensive`)

| Benchmark | v1.3.3 items/s | v1.3.4 items/s | Ziel | Status |
|-----------|---------------|---------------|------|--------|
| **Vektor-Operationen** | | | | |
| SimpleVectorBench/Insert_RGB_Vectors | 1.33 M/s | **1.22 M/s** | – | ⚠️ |
| SimpleVectorBench/Search_RGB_KNN_Top10 | 63.7 M/s | **62.1 M/s** | – | ✅ |
| SimpleVectorBench/Insert_384D_Embeddings | 465.5 k/s | **382.3 k/s** | – | ⚠️ |
| ComplexVectorBench/BatchInsert_1536D_LLMVectors | 132.8 k/s | **121.9 k/s** | – | ⚠️ |
| ComplexVectorBench/Search_4096D_TopK_Batch | 5.97 M/s | **5.62 M/s** | – | ⚠️ |
| **LLM / Embedding** | | | | |
| LLMInferencingBench/EmbeddingGeneration_Store | 122.0 k/s | **108.2 k/s** | – | ⚠️ |
| LLMInferencingBench/RAG_Search_Retrieve_Top50 | – | **7.55 M/s** (133 ns) | – | ❓ |
| LLMInferencingBench/MultiQueryExpansion_5Queries | – | **2.97 M/s** | – | ❓ |
| **AQL / Query** | | | | |
| AQLQueryBench/SimpleSelect_WhereClause | – | **148.8 k/s** (6.7 µs) | – | ❓ |
| AQLQueryBench/ComplexSelect_MultipleConditions | – | **3.25 k/s** (308 µs) | – | ❓ |
| AQLJoinBench/JoinUsers_Posts | – | **777.0 k/s** (1.3 µs) | – | ❓ |
| **Blob / Binär** | | | | |
| BinaryOperationsBench/StoreThumbnails_10KB | – | **4.92 k/s** | – | ❓ |
| BinaryOperationsBench/StoreLargeBlobs_1MB | – | **352 ops/s** | – | ❓ |
| BinaryOperationsBench/RetrieveBlobsBatch_100x100KB | – | **117.0 k/s** | – | ❓ |
| **Graph** | | | | |
| GraphOperationsBench/AddEdges_SparseGraph | – | **1.17 M/s** | – | ❓ |
| GraphOperationsBench/QueryNeighbors_DenseGraph | – | **975.2 k/s** | – | ❓ |
| GraphOperationsBench/GraphTraversal_BFS_Depth3 | – | **910.2 k/s** (1.09 µs) | – | ❓ |
| **Index** | | | | |
| SecondaryIndexBench/SmallIndexInsert_1K | – | **5.82 k/s** | – | ❓ |
| SecondaryIndexBench/MediumIndexInsert_100K | – | **9.14 k/s** | – | ❓ |
| SecondaryIndexBench/LargeIndexLookup_1M | – | **165.5 k/s** | – | ❓ |
| SecondaryIndexBench/CompositeIndexLookup | – | **7.59 k/s** | – | ❓ |
| **Batch / Stress** | | | | |
| BatchOperationsBench/BatchInsert_10K_WithMetadata | – | **779.1 k/s** | – | ❓ |
| BatchOperationsBench/BatchUpdate_MultiField_5K | – | **779.1 k/s** | – | ❓ |
| StressTestBench/MixedReadWrite_80Reads_20Writes | – | **22.9 k/s** | – | ❓ |
| StressTestBench/HotspotAccess_99PercentContention | – | **5.79 M/s** | – | ❓ |

---

### 36.3 Verschlüsselung (`bench_encryption`)

> Platform: v1.3.3 = Run 20251223_085556 | v1.3.4 = Run 20251229_184507

| Benchmark | v1.3.3 ops/s | v1.3.4 ops/s | Δ | Status |
|-----------|-------------|-------------|---|--------|
| BM_Encrypt_String_UsingKey/64 | 277.0 k/s (3.6 µs) | **254.9 k/s** (3.9 µs) | −8 % | ⚠️ |
| BM_Encrypt_String_UsingKey/256 | 254.4 k/s | **244.0 k/s** | −4 % | ⚠️ |
| BM_Encrypt_String_UsingKey/1024 | 254.9 k/s | **191.2 k/s** | −25 % | ❌ |
| BM_Decrypt_String_UsingKey/64 | 56.9 k/s | **45.5 k/s** | −20 % | ❌ |
| BM_Decrypt_String_UsingKey/256 | 60.1 k/s | **41.1 k/s** | −32 % | ❌ |
| BM_Decrypt_String_UsingKey/1024 | 52.5 k/s | **36.6 k/s** | −30 % | ❌ |
| BM_UserEntity_Encrypt_Serialize | – | **28.3 k/s** (35.1 µs) | – | ❓ |
| BM_HKDF_Derive_FieldKey | – | **177.8 k/s** (5.5 µs) | – | ❓ |
| BM_SchemaEncrypt_SingleField/64 | – | **86.1 k/s** (11.6 µs) | – | ❓ |
| BM_SchemaEncrypt_SingleField/1024 | – | **93.7 k/s** (10.7 µs) | – | ❓ |
| BM_SchemaDecrypt_SingleField/64 | – | **26.9 k/s** (68.2 µs) | – | ❓ |
| BM_VectorFloat_Encryption | – | **55.6 k/s** (17.9 µs) | – | ❓ |
| BM_DB_Ingest_Encrypted/100000 | – | **27.9 k/s** (3.58 s) | – | ❓ |
| BM_Index_Insert_Plain/100000 | – | **1.03 M/s** (97.4 ms) | – | ❓ |
| BM_Index_Insert_WithEncryptedPayload/100000 | – | **717.2 k/s** (139.4 ms) | – | ❓ |

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
| BM_VectorNormalization/512 | 3.553 µs | 281 k/s |
| BM_VectorNormalization/1024 | 7.237 µs | 138 k/s |
| **Haversine-Distanz (Geo)** | | |
| BM_GeoDistance_Haversine/100 | 3.576 µs | 28.0 M pts/s |
| BM_GeoDistance_Haversine/512 | 20.16 µs | 25.4 M pts/s |
| BM_GeoDistance_Haversine/4096 | 172.8 µs | 23.7 M pts/s |
| BM_GeoDistance_Haversine/10000 | 504.8 µs | 19.8 M pts/s |
| **Geo Point-in-Bounding-Box** | | |
| BM_GeoPointInBoundingBox/100 | 64.3 ns | 1.56 G pts/s |
| BM_GeoPointInBoundingBox/4096 | 9.375 µs | 437 M pts/s |
| BM_GeoPointInBoundingBox/100000 | 232.2 µs | 431 M pts/s |
| **Vektor+Geo kombiniert (Pre-Filter)** | | |
| BM_VectorGeoFiltering/1000 | 35.1 µs | 28.5 M/s |
| BM_VectorGeoFiltering/4096 | 150.7 µs | 27.2 M/s |
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

> **Beobachtung:** Prefilter ist bei kleinem n (1000) 5.5× langsamer als Postfilter. Ab n=20000 annähernde Parität (88 ms vs. 79 ms). Dies entspricht dem theoretischen Verhalten: Prefilter lohnt sich erst ab hoher Selektivität.

---

### 36.6 Storage Hotspots – WAL / Mixed-RW (`bench_hotspots_micro`)

> v1.3.3 vs. v1.3.4 — Thread-Count-Skalierung

| Benchmark | Threads | v1.3.3 ops/s | v1.3.4 ops/s | Δ |
|-----------|---------|-------------|-------------|---|
| **WAL ON (persistentes Schreiben)** | | | | |
| BM_RawWrite_WAL_On | 1 | 248 | **283** | +14 % |
| BM_RawWrite_WAL_On | 4 | 542 | **609** | +12 % |
| BM_RawWrite_WAL_On | 8 | 1.058 | **1.193** | +13 % |
| BM_RawWrite_WAL_On | 16 | 2.070 | **1.546** | −25 % ⚠️ |
| **WAL OFF (In-Memory)** | | | | |
| BM_RawWrite_WAL_Off | 1 | 205.5 k | **145.7 k** | −29 % ❌ |
| BM_RawWrite_WAL_Off | 4 | 354.7 k | **370.3 k** | +4 % |
| BM_RawWrite_WAL_Off | 8 | – | **507.5 k** | – |
| BM_RawWrite_WAL_Off | 16 | – | **350.3 k** | – |
| **Mixed RW (80% Read / 20% Write)** | | | | |
| BM_MixedRW | 1 | 583 | **583** | 0 % |
| BM_MixedRW | 4 | 1.289 | **1.289** | 0 % |
| BM_MixedRW | 8 | – | **2.534** | – |
| BM_MixedRW | 16 | – | **4.405** | – |
| **Secondary Index Write** | | | | |
| BM_SecondaryIndex_Write | 1 | 281 | **281** | 0 % |
| BM_SecondaryIndex_Write | 4 | 590 | **590** | 0 % |
| BM_SecondaryIndex_Write | 8 | – | **1.056** | – |
| BM_SecondaryIndex_Write | 16 | – | **1.990** | – |

---

### 36.7 AQL-Funktionen (`bench_aql_functions` / v1.3.4)

> Embedding-Cache, Hybrid Search, CTEs, Distributed Transactions

| Benchmark | real_time | items/s | Anmerkung |
|-----------|-----------|---------|-----------|
| **Embedding-Cache** | | | |
| BM_EmbeddingCache_Store/384 | 1.324 µs | 758.5 k/s | |
| BM_EmbeddingCache_Store/768 | 2.699 µs | 374.8 k/s | |
| BM_EmbeddingCache_Store/1536 | 158.2 µs | 14.2 k/s | größerer Dimensionsaufwand |
| BM_EmbeddingCache_Query_Hit/384 | 6.44 ns | **155.8 M/s** | Hot Path |
| BM_EmbeddingCache_Query_Hit/768 | 6.46 ns | **155.8 M/s** | Hot Path |
| BM_EmbeddingCache_Query_Hit/1536 | 1.882 µs | 541.0 k/s | |
| BM_EmbeddingCache_Query_Hit/3072 | 6.46 ns | **155.0 M/s** | Hot Path |
| BM_EmbeddingCache_Query_Miss/384 | 1.298 µs | 777.0 k/s | |
| BM_EmbeddingCache_CostSavings | 1.697 µs | 585.1 k/s | |
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
| BM_CTE_Recursive_Depth/1000 | 1.110 µs | 896.0 k/s | |
| **CTE Cycle-Detection** | | | |
| BM_CTE_CycleDetection/100 | 52.2 ns | 19.4 M/s | |
| BM_CTE_CycleDetection/1000 | 122.4 ns | 8.15 M/s | |
| BM_CTE_CycleDetection/10000 | 1.178 µs | 853.3 k/s | |
| **Subquery EXISTS** | | | |
| BM_Subquery_EXISTS_WithLIMIT1/100 | ~0 ns | ∞ | Short-Circuit |
| BM_Subquery_EXISTS_WithLIMIT1/100000 | ~0 ns | ∞ | Short-Circuit ✅ |
| BM_Subquery_EXISTS_WithoutLIMIT1/100 | 75.1 ns | 13.3 M/s | |
| BM_Subquery_EXISTS_WithoutLIMIT1/1000 | 702.2 ns | 1.41 M/s | |
| BM_Subquery_EXISTS_WithoutLIMIT1/10000 | 6.822 µs | 147.0 k/s | |
| BM_Subquery_EXISTS_WithoutLIMIT1/100000 | 68.49 µs | 14.7 k/s | linear skalierend |
| **Distributed Transactions (2PC)** | | | |
| BM_DistributedTxn_2PC_Latency/2 Shards | 0.07 ms | **29.300 ops/s** | PERF-D4: thread pool |
| BM_DistributedTxn_2PC_Latency/4 Shards | 0.10 ms | **29.300 ops/s** | PERF-D4: thread pool |
| BM_DistributedTxn_2PC_Latency/8 Shards | 0.13 ms | **29.300 ops/s** | PERF-D4: thread pool |
| BM_DistributedTxn_2PC_Latency/16 Shards | 0.15 ms | **29.300 ops/s** | PERF-D4: thread pool |
| BM_DistributedTxn_Throughput | 0.10 ms | **29.300 ops/s** | PERF-D4: batch window |
| BM_DistributedTxn_SnapshotRead/4 | 61.54 ms | **6.400 ops/s** | |
| **LLM/RAG Pipeline** | | | |
| BM_Combined_LLM_RAG_Pipeline | 151.4 µs | **15.9 k/s** | |

---

### 36.8 Graph-Traversal (`bench_graph_traversal`)

> Run 20251223_085556 (v1.3.3)

| Benchmark | real_time (ms) | ops/s |
|-----------|---------------|-------|
| **BFS** | | |
| GraphTraversalBenchmarkFixture/BFSTraversal/100 nodes/depth 4 | 0.184 ms | 5.430 k/s |
| GraphTraversalBenchmarkFixture/BFSTraversal/1000 nodes/depth 4 | 1.56 ms | 0.652 k/s |
| GraphTraversalBenchmarkFixture/BFSTraversal/10000 nodes/depth 4 | 20.2 ms | 50.6 ops/s |
| GraphTraversalBenchmarkFixture/BFSTraversal/100 nodes/depth 20 | 0.469 ms | 2.108 k/s |
| GraphTraversalBenchmarkFixture/BFSTraversal/1000 nodes/depth 20 | 4.38 ms | 232.7 ops/s |
| **DFS** | | |
| GraphTraversalBenchmarkFixture/DFSTraversal/100 nodes/depth 4 | 0.184 ms | 5.379 k/s |

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

> v1.3.3 vs. v1.3.4 — CPU-Backend (GPU nicht verfügbar in CI)

| Benchmark | v1.3.3 items/s | v1.3.4 items/s | Δ |
|-----------|---------------|---------------|---|
| BM_CPUBackend_DistanceComputation/10×1000 | 11.24 M/s | **10.24 M/s** | −9 % |
| BM_CPUBackend_DistanceComputation/100×10000 | 11.49 M/s | **9.60 M/s** | −16 % |
| BM_CPUBackend_DistanceComputation/1000×100000 | 10.63 M/s | **9.95 M/s** | −7 % |
| BM_BackendComparison_VaryingDimensions/64 | 28.28 M/s | **25.87 M/s** | −9 % |
| BM_BackendComparison_VaryingDimensions/128 | 11.49 M/s | **9.74 M/s** | −15 % |
| BM_BackendComparison_VaryingDimensions/256 | 5.19 M/s | **4.36 M/s** | −16 % |
| BM_BackendComparison_VaryingDimensions/512 | – | **2.29 M/s** | – |
| BM_BackendComparison_VaryingDimensions/1024 | – | **1.08 M/s** | – |
| BM_BackendInitializationOverhead | – | **14.93 M/s** | – |
| BM_ThroughputComparison | – | **10.10 M/s** | – |

---

### 36.11 Image-Analyse (`bench_image_analysis`)

> Run 20251229_184507 (v1.3.4)

| Benchmark | real_time | ops/s | Anmerkung |
|-----------|-----------|-------|-----------|
| BM_ImageEmbedding_SingleImage/224px | 3.95 µs | 253.3 k/s | |
| BM_ImageEmbedding_SingleImage/384px | 4.11 µs | 243.3 k/s | |
| BM_ImageEmbedding_SingleImage/512px | 4.25 µs | 235.1 k/s | |
| BM_ImageEmbedding_SingleImage/1024px | 4.88 µs | 205.0 k/s | |
| BM_ImageEmbedding_Batch/1 | 3.87 µs | 258.4 k/s | |
| BM_ImageEmbedding_Batch/4 | 15.47 µs | 258.6 k/s | ~konstant/Bild |
| BM_ImageEmbedding_Batch/8 | 30.84 µs | 259.5 k/s | |
| BM_ImageEmbedding_Batch/16 | 63.24 µs | 253.1 k/s | |
| BM_ImageCaptioning/224px | 20.76 µs | 48.2 k/s | |
| BM_ImageCaptioning/384px | 61.27 µs | 16.3 k/s | |
| BM_ImageCaptioning/512px | 113.4 µs | 8.82 k/s | |
| BM_Plugin_Initialization | 5.51 ns | 181.6 M/s | sehr schnell |
| BM_Plugin_Warmup | 4.05 µs | 246.7 k/s | |

**Image Latenz-Verteilung** (`bench_image_analysis_latency`, v1.3.4):

| Benchmark | Mean (ms) | P50 (ms) | P95 (ms) | P99 (ms) |
|-----------|-----------|----------|----------|----------|
| BM_Embedding_LatencyDistribution_224 | 1.583 µs | 1.500 µs | 1.600 µs | 2.200 µs |
| BM_Embedding_ColdStartVsWarm (cold) | 1.960 µs | – | – | – |
| BM_Embedding_ColdStartVsWarm (warm) | 1.881 µs | – | – | – |
| BM_Embedding_GPUvsCPU/CPU | 2.633 µs | 2.100 µs | 2.200 µs | 20.3 µs |
| BM_Embedding_GPUvsCPU/GPU | 2.306 µs | 1.700 µs | 2.500 µs | 21.1 µs |
| BM_Caption_LatencyDistribution | 22.0 µs | 21.1 µs | 22.6 µs | 40.2 µs |
| BM_Batch_LatencyPerImage/1 | 1.975 µs | 1.700 µs | 1.800 µs | – |
| BM_Batch_LatencyPerImage/4 (per img) | 1.583 µs | 1.475 µs | 1.575 µs | – |
| BM_Batch_LatencyPerImage/8 (per img) | 1.506 µs | 1.450 µs | 1.500 µs | – |
| BM_Batch_LatencyPerImage/16 (per img) | 1.537 µs | 1.481 µs | 1.563 µs | – |
| BM_ImageSize_LatencyImpact/384px | 2.514 µs | 2.200 µs | 2.300 µs | – |
| BM_ImageSize_LatencyImpact/512px | 3.023 µs | 2.700 µs | 2.800 µs | – |
| BM_ImageSize_LatencyImpact/1024px | 6.007 µs | 5.700 µs | 6.000 µs | – |

---

### 36.12 HSM-Provider (`bench_hsm_provider`)

> v1.3.3 vs. v1.3.4 — Stub-Implementierung (echte HSM-Bibliothek nicht in CI)

| Benchmark | v1.3.3 ops/s | v1.3.4 ops/s | Δ |
|-----------|-------------|-------------|---|
| BM_HSM_Sign_Stub | 1.493 M/s (667 ns) | **1.434 M/s** (693.8 ns) | −4 % |
| BM_HSM_Verify_Stub | 1.629 M/s (612 ns) | **1.550 M/s** (659 ns) | −5 % |
| BM_HSM_Sign_Real_Pool* | n/a (Lib fehlt) | n/a | – |

> **Ziel** SEC-7: HSM-Backed RSA-2048 Sign P99 ≤ 20 ms → Stub-Werte ~0.7 µs, Real-HSM-Werte ausstehend.

---

### 36.13 AQL-Sugar Hybrid (`bench_hybrid_aql_sugar`)

> v1.3.3 vs. v1.3.4

| Benchmark | v1.3.3 ops/s | v1.3.4 ops/s | Δ |
|-----------|-------------|-------------|---|
| BM_VectorGeo_AQL_Sugar | – (ERROR) | – (ERROR) | – |
| BM_VectorGeo_CPP_API | 123.6 ops/s (8.58 ms) | **112.6 ops/s** (8.91 ms) | −9 % |
| BM_ContentGeo_AQL_Sugar | 5.556 k/s (0.457 ms) | **6.127 k/s** (0.347 ms) | +10 % ✅ |
| BM_ContentGeo_CPP_API | 5.589 k/s (0.436 ms) | **7.191 k/s** (0.319 ms) | +29 % ✅ |
| BM_AQL_Parse_Translate_Only | 152.5 k/s (6.57 µs) | **150.9 k/s** (6.66 µs) | −1 % ✅ |

---

### 36.14 Content-Versionierung (`bench_content_versioning`)

> Run 20251229_184507 (v1.3.4)

| Benchmark | real_time | bytes/s |
|-----------|-----------|---------|
| BM_VersionCreation/1 KB | 1.14 µs | 895 MB/s |
| BM_VersionCreation/10 KB | 10.4 µs | 979 MB/s |
| BM_VersionCreation/100 KB | 104.3 µs | 975 MB/s |
| BM_VersionCreation/1 MB | 1.197 ms | 877 MB/s |
| BM_VersionCreation/10 MB | 12.71 ms | 810 MB/s |
| BM_DiffComputation/1 KB | 69.9 ns | 29.4 GB/s |
| BM_DiffComputation/10 KB | 188.0 ns | 108.7 GB/s |
| BM_DiffComputation/100 KB | 2.515 µs | 81.3 GB/s |
| BM_DiffComputation/1 MB | 245.7 µs | 8.53 GB/s |
| BM_VersionRetrieval | 302.5 ns | – |
| BM_StorageOverhead/10 versions | 57.95 µs | – |
| BM_StorageOverhead/100 versions | 744.4 µs | – |
| BM_StorageOverhead/500 versions | 2.727 ms | – |
| BM_ConcurrentVersioning/1 Thread | 11.86 µs | 875 MB/s |
| BM_ConcurrentVersioning/2 Threads | 12.61 µs | 833 MB/s |
| BM_ConcurrentVersioning/4 Threads | 15.64 µs | 667 MB/s |
| BM_ConcurrentVersioning/8 Threads | 19.48 µs | 506 MB/s |

---

### 36.15 ARM-Speicherbandbreite (`bench_arm_memory`)

> Run 20251229_184507 (v1.3.4, x86_64-Emulation auf ARM-Pfad)

| Benchmark | Blockgröße | real_time | Bandbreite |
|-----------|-----------|-----------|------------|
| **Sequential Read** | | | |
| BM_ARM_Sequential_Read | 4 KB | 3.63 µs | 4.55 GB/s |
| BM_ARM_Sequential_Read | 32 KB | 28.28 µs | 4.60 GB/s |
| BM_ARM_Sequential_Read | 256 KB | 220.9 µs | 4.77 GB/s |
| BM_ARM_Sequential_Read | 1 MB | 908.1 µs | 4.66 GB/s |
| **Sequential Write** | | | |
| BM_ARM_Sequential_Write | 4 KB | 2.07 µs | 7.99 GB/s |
| BM_ARM_Sequential_Write | 32 KB | 16.81 µs | 7.76 GB/s |
| BM_ARM_Sequential_Write | 256 KB | 133.6 µs | 7.95 GB/s |
| BM_ARM_Sequential_Write | 1 MB | 528.9 µs | 7.90 GB/s |
| **MemCopy (builtin)** | | | |
| BM_ARM_MemCopy_Builtin | 4 KB | 139.9 ns | 118.6 GB/s |
| BM_ARM_MemCopy_Builtin | 32 KB | 2.077 µs | 63.9 GB/s |
| BM_ARM_MemCopy_Builtin | 256 KB | 32.00 µs | 33.0 GB/s |
| BM_ARM_MemCopy_Builtin | 1 MB | 129.9 µs | 32.6 GB/s |

---

### 36.16 MVCC-Transaktionen (`bench_mvcc`)

> Run 20251223_085556 (v1.3.3)

| Benchmark | real_time | ops/s | Anmerkung |
|-----------|-----------|-------|-----------|
| MVCCFixture/SingleEntityCommit_MVCC | 4.07 ms | 7.111 k/s | |
| MVCCFixture/BatchInsert100_MVCC | 7.29 ms | 29.67 k/s | |
| MVCCFixture/SnapshotIsolationOverhead_MVCC | 4.05 ms | 40.0 k/s | |
| MVCCFixture/Rollback_MVCC | 266.0 µs | 37.33 k/s | |
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
| BM_LockContention_Disjoint | 16 | 66.7 ms | 15.3 k/s ⚠️ |
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

> **Beobachtung:** Batch-API ist hier **langsamer** als Single-Inserts in Items/s — deutet auf Overhead im Batch-Koordinator hin. Bekannte Optimierungslücke (vgl. §34 D-5).

---

### 36.19 Compression-Benchmark (`bench_compression`)

> Run 20251223_085556 (v1.3.3)

| Benchmark | Blockgröße | Kompression | real_time | ops/s |
|-----------|-----------|-------------|-----------|-------|
| CompressionFixture/SequentialWrite/Keine/512B | 512 B | – | 25.2 ms | 48.0 k/s |
| CompressionFixture/SequentialWrite/LZ4/512B | 512 B | LZ4 | 25.9 ms | 41.8 k/s |
| CompressionFixture/SequentialWrite/Zstd/512B | 512 B | Zstd | 26.2 ms | 42.7 k/s |
| CompressionFixture/SequentialWrite/Keine/4096B | 4096 B | – | 33.3 ms | 35.2 k/s |
| CompressionFixture/SequentialWrite/LZ4/4096B | 4096 B | LZ4 | 32.9 ms | 34.7 k/s |
| CompressionFixture/SequentialWrite/Zstd/4096B | 4096 B | Zstd | 32.6 ms | 34.5 k/s |

---

### 36.20 Zusammenfassung: Regression-Übersicht v1.3.0 → v1.3.4

| Benchmark | v1.3.0 | v1.3.4 | Δ | Schwere |
|-----------|--------|--------|---|---------|
| VectorIndexBench/InsertPlaintext | 566.7 k/s | 351.4 k/s | **−38 %** | ❌ Kritisch |
| SecondaryIndexBench/IndexInsert | 1.78 M/s | 217.2 k/s | **−88 %** | ❌ Kritisch |
| QueryEngineBench/SimpleEvaluation | 968.6 M/s | 814.5 M/s | −16 % | ⚠️ Mittel |
| GraphIndexBench/AddEdges | 1.47 M/s | 628.7 k/s | **−57 %** | ❌ Kritisch |
| TimeseriesBench/InsertTimepoints | 61.0 M/s | 49.0 M/s | **−20 %** | ⚠️ Mittel |
| BM_Encrypt_String_UsingKey/1024 | 254.9 k/s | 191.2 k/s | −25 % | ⚠️ Mittel |
| BM_Decrypt_String_UsingKey/256 | 60.1 k/s | 41.1 k/s | −32 % | ❌ Hoch |
| BM_CPUBackend_DistanceComputation | 11.24 M/s | 10.24 M/s | −9 % | ⚠️ Gering |
| BM_ContentGeo_CPP_API | 5.59 k/s | 7.19 k/s | **+29 %** | ✅ Verbesserung |
| BM_ContentGeo_AQL_Sugar | 5.56 k/s | 6.13 k/s | **+10 %** | ✅ Verbesserung |
| EmbeddingCache_Query_Hit/384 | – | 155.8 M/s | n/a (neu) | ✅ Neu |
| 2PC-Throughput (2 Shards) | 6.4 k/s | 29.3 k/s | **+357 %** | ✅ PERF-D4 |

> **Wichtige Relativierung:** Mehrere Regressionen (insb. SecondaryIndex, VectorIndex, Graph) sind auf geänderte Test-Infrastruktur zurückzuführen (per-test temp dirs, einzelne RocksDB-Transaktionen pro `put()`), nicht auf Produktions-Regressions — vgl. `PERFORMANCE_COMPARISON_V1.3.0_VS_V1.3.3.md`.


---

## 37. Durchgeführte Performance-Maßnahmen (mit GitHub-PR)

> Chronologisch absteigend (neueste zuerst). Alle PRs liegen auf dem `develop`-Branch.
> Links: `https://github.com/makr-code/ThemisDB/pull/<Nr>`

---

### 37.1 v1.9.0 – Aktuelle Maßnahmen

| # | Maßnahme | Modul | PR | Version | Messbare Wirkung |
|---|----------|-------|----|---------|-----------------|
| 1 | **Batch-Prediction, O(1)-Update, Parallel-Auto-Tune, FNV-1a Fit-Cache** — `predictBatch()` für N Serien, inkrementelles ETS/ARIMA/LR-Update, 9 parallele `std::async`-Auto-Tune-Tasks | Analytics / Forecasting | [#4054 (Issue)](https://github.com/makr-code/ThemisDB/issues/4054) | v1.9.0 | Auto-Tune: 9× Parallelisierung; Fit-Cache: wiederholte Serien O(1) statt O(n) |
| 2 | **QueryCompiler JIT Hot-Path** — JIT-kompilierte Ausführungspfade in `executeAql()` verdrahtet, vectorized-execution-Tests registriert | Query | [#4398](https://github.com/makr-code/ThemisDB/pull/4398) | v1.9.0 | AQL Hot-Path: JIT-Pfad aktiv |
| 3 | **Cache Warmup-Logik** — `warmupFromLog` max_entries-Grenze korrekt durchgesetzt, Snippet-Boundary-Alignment verbessert | Cache | (direct commit `64a9ae4`) | v1.9.0 | Weniger Overfetch bei Warmup |
| 4 | **AdaLoRA + Multi-Adapter** — Importance-basiertes Rank-Pruning, `LoRAAdapterMerger` mit TIES-Merging und Power-Iteration-SVD | Training | [#4405](https://github.com/makr-code/ThemisDB/pull/4405) | v1.9.0 | LoRA Memory-Footprint reduziert, Merge ohne separaten Checkpoint |
| 5 | **DiskANN / MRL-Truncation** — Matryoshka Representation Learning für mehrstufige ANN-Retrieval-Pipeline | Index | [#4399](https://github.com/makr-code/ThemisDB/pull/4399) | v1.9.0 | Ersten Stage mit 64-dim statt 1536-dim → ≥10× weniger FLOPS in Stage 1 |

---

### 37.2 v1.8.0 – Maßnahmen

| # | Maßnahme | Modul | PR | Version | Messbare Wirkung |
|---|----------|-------|----|---------|-----------------|
| 6 | **SIMD-Vektorisierung AVX-512 + ARM NEON** — Aggregations- und Distanz-Kernels mit AVX-512-Intrinsics, ARM NEON-Fallback; CPUID-Check gecacht (static const) | Analytics | [#4317](https://github.com/makr-code/ThemisDB/pull/4317) | v1.8.0 | Benchmark-Ziel: ≥4 GB/s auf Cortex-A78; AVX-512 check: O(1) statt O(n) |
| 7 | **Predictive Prefetcher (ML-basiertes Zugriffsmuster-Modell)** — Erkennt wiederkehrende Zugriffsmuster und löst Prefetch vor dem Cache-Miss aus | Cache / Performance | [#4293](https://github.com/makr-code/ThemisDB/pull/4293) | v1.8.0 | Ziel: Cache-Miss-Rate −20 % bei sequenziellen Workloads |
| 8 | **Intelligent Prefetching System** — Zweite Prefetch-Schicht mit konfigurierbarem Lookahead, adaptive Prefetch-Tiefe | Performance | [#4257](https://github.com/makr-code/ThemisDB/pull/4257) | v1.8.0 | Ziel: Prefetch-Overfetch ≤10 % |
| 9 | **Query Compilation & JIT** — `AdaptiveQueryCompiler` mit JIT-Codegen-Pfad, Expressions zu nativer Code kompiliert | Query | [#4246](https://github.com/makr-code/ThemisDB/pull/4246) | v1.8.0 | Ziel: AQL-Parse+Execute P99 ≤ 2 ms; JIT-Erstcompilierung ≤ 50 ms |
| 10 | **Parallel Query Execution (Intra-Query)** — Parallele Ausführung unabhängiger Query-Teilpläne via Thread-Pool | Query | [#4211](https://github.com/makr-code/ThemisDB/pull/4211) | v1.7.0 | Ziel: multi-core Skalierung für OLAP-Queries |
| 11 | **Parallel `translateBatchNLToAQL()`** — Bounded-Worker-Pool + `std::async`-Semaphor-Throttle für NL→AQL-Batch-Übersetzungen | AQL | [#4221](https://github.com/makr-code/ThemisDB/pull/4221) | v1.7.0 | Batch-Throughput proportional zu Worker-Count |
| 12 | **Write-Optimized Merge (WOM) Tree** — LSM-Tree-Optimierungen: Delayed Compaction, Tiered-Merge-Policy, Write-Stall-Prävention | Storage | [#4204](https://github.com/makr-code/ThemisDB/pull/4204) | v1.8.0 | Ziel: Write-Amplification <1.5×; WAL OFF: 507 k ops/s @ 8 Threads |
| 13 | **Write Batching & Coalescing** — Transaktions-Batcher mit konfigurierbarem Fenster 1–100 ms, adaptive Batch-Größe | Transaction | [#4335](https://github.com/makr-code/ThemisDB/pull/4335) | v1.8.0 | Konfigurierbar 1–100 ms Batch-Fenster; adaptive ±10 % |
| 14 | **Optimistic Concurrency Control (OCC)** — Conflict-Detection-Phase nach Lese-Phase, Retry-Backoff, Deadlock-Watchdog | Transaction | [#4264](https://github.com/makr-code/ThemisDB/pull/4264) | v1.8.0 | OCC Commit P50: 100 µs, P99: 5 ms; Deadlock-Overhead: 1 % |
| 15 | **Index-Kompression** — Delta-, Prefix-, RLE-, Dictionary-, Bloom-Filter-Encoding für B-Tree/sekundäre Indizes | Index | [#4226](https://github.com/makr-code/ThemisDB/pull/4226) | v1.7.0 | Index-Größe −40–60 % (dokumentiert); Lookup-Latenz unverändert |
| 16 | **Cache Warmup Parallel Bulk-Load** — `warmupParallelBulkLoad()` mit konfigurierbaren Worker-Threads | Cache | [#4250](https://github.com/makr-code/ThemisDB/pull/4250) | v1.8.0 | Warmup-Throughput: Ziel ≥500 k Entries/s |
| 17 | **zlib → ZSTD Migration** — StreamWriter-Kompression vollständig auf ZSTD Level 3 umgestellt | Exporters | [#4252](https://github.com/makr-code/ThemisDB/pull/4252) | v1.8.0 | ZSTD: −30–50 % Datenvolumen vs. zlib bei vergleichbarer Latenz |
| 18 | **Wire Protocol Performance** — TCP-Framing optimiert, Zero-Copy-Payload-Transfer, Keep-Alive-Pooling | Network | [#4214](https://github.com/makr-code/ThemisDB/pull/4214) | v1.7.0 | Ziel: ≥100 k req/s/Core (128 B, kein TLS) |
| 19 | **Arrow Zero-Copy IPC + OLAP LRU-Cache** — Apache Arrow Record-Batch für spaltenweisen Zero-Copy-Transfer; OLAP-Ergebnis-Cache mit TTL und LRU-Eviction | Analytics | [#4328](https://github.com/makr-code/ThemisDB/pull/4328) | v1.8.0 | Zero-Copy: kein Memcpy bei OLAP-Ausgabe; LRU: Wiederholte Queries aus Cache |
| 20 | **Memory Pool Allocator (Hot Analytics)** — `slab`-basierter Pool für kurzzeitige Analytics-Allocations auf kritischen Pfaden | Analytics | [#4311](https://github.com/makr-code/ThemisDB/pull/4311) | v1.8.0 | Reduziert Allocator-Contention auf Hot-Paths; jemalloc-freundlich |
| 21 | **SAGA Orchestrator (DAG-Parallelausführung)** — Parallele Kompensations-Ausführung via topologisch sortiertem DAG | Transaction | [#4305](https://github.com/makr-code/ThemisDB/pull/4305) | v1.8.0 | SAGA Compensation Time: 20 ms Ziel; parallelisierte Steps |
| 22 | **Read-Only Transaction Optimization** — Skip-Lock-Pfad für reine Lese-Transaktionen, kein Snapshot-Overhead | Transaction | (direct commit `d5eddfb`) | v1.8.0 | Lese-Transaktionen: kein 2PC-Overhead |
| 23 | **SLO Monitor Latency Percentile Tracking** — P50/P95/P99-Histogramm mit konfigurierbaren Schwellwert-Alerts | Cache / Observability | [#4329](https://github.com/makr-code/ThemisDB/pull/4329) | v1.8.0 | Echtzeit-Regression-Erkennung; CI-Gate blockiert bei P99 >20 % über Baseline |
| 24 | **DiffEngine::computeDiff() + Cache-Stampede-Fix** — O(N)-Changefeed-Scan durch Diff-Cache ersetzt; Cache-Stampede durch Single-Fetch-Lock | Analytics / Cache | [#4325](https://github.com/makr-code/ThemisDB/pull/4325) | v1.8.0 | Changefeed-Scan: O(N) → O(1) für gecachte Diffs |
| 25 | **Perceptual Hashing Deduplication** — pHash-basierte Bild-Deduplizierung mit Hamming-Distance-Index | Content | [#4331](https://github.com/makr-code/ThemisDB/pull/4331) | v1.8.0 | Speichereinsparung durch Dedup; kein Re-Embedding für Duplikate |
| 26 | **CUDA k>kMaxK Silent-Clamping entfernt** — `kMaxK` auf 1024 erhöht mit dynamischem Shared Memory; kein silentes Trunkieren mehr | Acceleration | [#4320](https://github.com/makr-code/ThemisDB/pull/4320) | v1.8.0 | CUDA Shared Memory: ≤32 KB bei k=1024 laut Ziel-Spec |
| 27 | **VLLMResourceManager Multi-GPU NVML-Monitoring** — Per-GPU Memory/Utilization-Monitoring via NVML; CPU-Snapshot-Cache 200 ms TTL | Acceleration | [#4318](https://github.com/makr-code/ThemisDB/pull/4318) | v1.8.0 | getStats()-Latenz: <2 ms (gecacht) statt NVML-Call auf Hot-Path |
| 28 | **BackendRegistry Thread-Safe Read-Access** — Dedizierter Read-Lock-Pfad ohne Writer-Contention | Acceleration | [#4321](https://github.com/makr-code/ThemisDB/pull/4321) | v1.8.0 | Concurrent Registry-Lookups ohne Mutex-Bottleneck |
| 29 | **LLMProcessAnalyzer O(1) LRU-Cache-Eviction unter Lock** — `std::list`-basierter LRU statt O(N)-Scan | LLM | [#4322](https://github.com/makr-code/ThemisDB/pull/4322) | v1.8.0 | Eviction: O(N) → O(1) |
| 30 | **LoRA Adapter Hot-Loading** — Adapter laden ohne Neustart; `unique_lock` für thread-sicheres Hot-Swap | LLM / Training | [#4333](https://github.com/makr-code/ThemisDB/pull/4333) | v1.8.0 | Ziel: ≤5 s Wall-Clock für 7B-Modell, Rank 64, 16-bit |
| 31 | **Logical Replication Parallel Decoding** — WAL-Decoder mit parallelisierten Decode-Threads | Replication | (direct commit `02ecdca`) | v1.8.0 | Replication WAL-Shipping Throughput-Ziel: ≥500 MB/s/Follower |
| 32 | **Distributed Analytics Sharding – gecachter Health-State** — `getHealthyShardCount()` ohne Network-I/O unter Lock | Sharding | [#4324](https://github.com/makr-code/ThemisDB/pull/4324) | v1.8.0 | Shard-Health-Lookup: O(1) aus Cache statt synchroner RPC |
| 33 | **Lock-Free L1 Cache Read-Path** — Migration L1-Lese-Pfad auf `std::atomic` ohne Mutex | Cache | (direct commit `a95475d`) | v1.8.0 | L1 Read Hot-Path: mutex-frei → Ziel ≥5 M ops/s/Core |
| 34 | **Geo DBSCAN / k-Means GPU** — DBSCAN und k-Means mit GPU-Beschleunigung für große Punkt-Mengen | Geo | [#4298](https://github.com/makr-code/ThemisDB/pull/4298) | v1.8.0 | DBSCAN GPU Speedup: >100× vs. CPU (100K Punkte) |
| 35 | **Distributed Ingestion Coordinator** — Mehrstufige Ingestion-Pipeline mit Retry-Quarantäne und parallelen S3-Downloads | Ingestion | [#4309](https://github.com/makr-code/ThemisDB/pull/4309) | v1.8.0 | S3 concurrent: ≥200 MB/s agg. (4 parallel, 10 Gbps) |
| 36 | **Incremental View Lock-Free Apply** — `applyChanges()` ohne globalen Write-Lock für inkrementelle Materialized-View-Updates | Analytics | [#4316](https://github.com/makr-code/ThemisDB/pull/4316) | v1.8.0 | IVM Delta-Application: ≤50 ms (10k Rows) |
| 37 | **StreamingWindow konfigurierbare Expiry-Poll-Intervalle** — Kein Busy-Wait; konfigurierbare Sleep-Dauer für Expiry-Worker | Analytics | [#4327](https://github.com/makr-code/ThemisDB/pull/4327) | v1.8.0 | CPU-Idle beim Streaming-Worker signifikant reduziert |

---

### 37.3 v1.7.0 – Maßnahmen

| # | Maßnahme | Modul | PR | Version | Messbare Wirkung |
|---|----------|-------|----|---------|-----------------|
| 38 | **CUDA ANN-Kernel-Vollimplementierung** — Fused-Cosine-Kernel + Shared-Memory Top-K-Helper; HIP/RCCL `mergeTopK` für Multi-GPU | Acceleration | [#4193](https://github.com/makr-code/ThemisDB/pull/4193) | v1.7.0 | mergeTopK <500 µs (worldSize=4, k=100, NVLink-3) |
| 39 | **GPU Hardware Support Gaps** — HIP Top-K-Heap, CUDA HNSW Bitset, NCCL/RCCL `mergeTopK` | Acceleration | (direct commit `73d8f8a`) | v1.7.0 | Bitset-Optimierung: 8× Memory-Reduktion (5 GB → 640 MB) |
| 40 | **TSStore Single-Point Insert Buffering (Gorilla)** — In-Memory-Buffer vor Gorilla-Kompressionsflush; kein WAL-Write per Punkt | Timeseries | (direct commit `822b0af`) | v1.7.0 | Ziel: >500 k pts/s (von ~200 k pts/s); Buffer-to-Storage Flush P99 <10 ms |
| 41 | **AdaptiveQueryCompiler Audit-Gaps** — Lücken in Compiler-Pipeline geschlossen (Issue #86) | Query | (direct commit `2efe683`) | v1.7.0 | Compiler-Regression-Gate: ≤5 % |
| 42 | **HardwareAccelerator v1.8.0** — CPU-affinity-basierte NUMA-Zuweisung, GPU-Backend-Selection | Performance | (direct commit `139f96c`) | v1.7.0 | NUMA-lokale Allokation; reduzierten Cross-Socket-Traffic |

---

### 37.4 v1.6.0 und früher

| # | Maßnahme | Modul | PR | Version | Messbare Wirkung |
|---|----------|-------|----|---------|-----------------|
| 43 | **GPU-Acceleration Multi-Tenancy** — Erste GPU-Backend-Integration, CUDA-Kernel-Grundgerüst | Acceleration | [#44](https://github.com/makr-code/ThemisDB/pull/44) | früh | GPU-Backend-Grundlage |
| 44 | **Hardware Acceleration Support** — CPU AVX2-Baseline, erste Vektoroperationen | Acceleration | [#30](https://github.com/makr-code/ThemisDB/pull/30) | früh | CPU AVX2-Baseline für Benchmarks |
| 45 | **Benchmark-Datenbank-Tests** — Erste Google-Benchmark-Targets, Baseline für spätere Regression-Tests | Benchmarks | [#54](https://github.com/makr-code/ThemisDB/pull/54) | früh | Benchmark-Infrastruktur aufgebaut |
| 46 | **Benchmarks-Repository-Erweiterung** — Neue Bench-Targets für Vektor-, Timeseries-, Graph-Operationen | Benchmarks | [#33](https://github.com/makr-code/ThemisDB/pull/33) | früh | Benchmark-Coverage auf 9 Module erweitert |
| 47 | **Lossless Compression-Methoden (Research)** — Evaluierung LZ4 vs. Zstd vs. Snappy → Entscheidung für Zstd | Storage | [#70](https://github.com/makr-code/ThemisDB/pull/70) | früh | Grundlage für PR #4252 (Zstd-Migration) |
| 48 | **OpenCL Erasure Coder** — GF(2^8)-Arithmetik-basiertes Reed-Solomon Encode/Decode/BatchEncode | Sharding | (direct commit `dc202ef`) | v1.7.0 | GPU Reed-Solomon: >4 GB/s Ziel (NVIDIA A10) |

---

### 37.5 Offene / Geplante Performance-Maßnahmen (noch nicht umgesetzt)

| # | Geplante Maßnahme | Modul | Ziel-Metrik | Ziel-Version |
|---|-------------------|-------|-------------|--------------|
| P-1 | **Gorilla Decode AVX-optimierung** — SIMD-Decode-Pfad für Gorilla-Kompression | Timeseries | >2 GB/s (von ~400 MB/s) | Q3 2026 |
| P-2 | **SecondaryIndex Batch-Transaktionen** — Mehrere `put()`-Aufrufe in einer Transaktion bündeln | Index / Storage | 1.78 M/s wiederherstellen (von 217 k/s) | Q2 2026 |
| P-3 | **CUDA Geospatial Distanz-Kernels** — WGS84-Haversine und Point-in-Polygon auf GPU | Geo | GPU Contains 1M Punkte <50 ms (A10G) | Q3 2026 |
| P-4 | **Vector Insert Throughput** — HNSW-Build-Parallelisierung, Segment-basiertes Insert | Index | 600 k/s (FAISS-Parität) | Q3 2026 |
| P-5 | **1 MB Blob Write-Throughput** — Async WAL + Background Flush | Storage | ≥100 k ops/s (von 741 ops/s) | Q2 2026 |
| P-6 | **Concurrent Concurrency-Stabilisierung** — CV-Reduktion bei 10-Client-Lasttest | Storage | CV <5 % (von 20.74 %) | Q2 2026 |
| P-7 | **2PC Throughput-Steigerung** — Batched Prepare + Thread Pool (PERF-D4) | Transaction | ~~15 k/s~~ **29,3 k/s ✅** | Completed |
| P-8 | **Query Engine vs. ClickHouse** — Columnar SIMD Aggregation, Vectorized Scan | Query | 1.2 G items/s | Q4 2026 |
| P-9 | **TLS 1.3 Session Resumption** — TLS-Session-Ticket-Cache | Network | <1 ms P99 | Q2 2026 |
| P-10 | **QUIC 0-RTT** — QUIC-Transport für LAN-Kommunikation | Network | <2 ms P99 | Q3 2026 |


---

## 38. Weitere Rohdaten: HTTP-API-Benchmarks (v1.0.x, Dezember 2025)

> Quellen: `benchmarks/results_analysis_reports/scientific_benchmarks_20251204_212220/` und `docker_benchmarks_results_20251209_*/`  
> Plattform: Intel i9-10900K @ 3.70 GHz, 10 physische / 20 logische Cores, 31.3 GB RAM, Linux WSL2 5.15.167.4, Python 3.12 HTTP-Client, ThemisDB v1.0.0, endpoint http://localhost:8765

---

### 38.1 Wissenschaftliche Einzeloperation-Benchmarks (n=500, 5 Iterationen à 100 Ops)

> Messmethode: HTTP POST/GET gegen laufende ThemisDB-Instanz; 5 Warmup-Iterationen

| Test | avg (ms) | p50 (ms) | p95 (ms) | p99 (ms) | CV (%) | min (ms) | max (ms) |
|------|----------|----------|----------|----------|--------|----------|----------|
| **INSERT 1 KB** | 1.317 | 1.299 | 1.491 | 1.715 | 6.7 | 1.177 | 1.783 |
| **READ 1 KB** | 1.204 | 1.147 | 1.519 | 1.706 | 12.0 | 1.016 | 1.832 |
| **UPDATE 1 KB** | 1.240 | 1.219 | 1.386 | 1.603 | 6.8 | 1.103 | 1.761 |
| **INSERT 10 KB** | 1.960 | 1.922 | 2.284 | 2.378 | 6.6 | 1.813 | 2.378 |
| **INSERT 100 KB** | 7.913 | 7.889 | 8.847 | 9.369 | 6.5 | 7.075 | 9.369 |
| **INSERT 1 MB** | 61.402 | 60.954 | 65.923 | 68.178 | 2.6 | 60.007 | 68.178 |

> **Beobachtung:** 1 KB INSERT/READ/UPDATE zeigen stabiles Verhalten (CV ~7 %). 1 MB INSERT skaliert fast linear mit der Payload-Größe (×47 vs 1 KB). Kein Ausreißer-Verhalten bei Einzel-Clients.

---

### 38.2 Concurrent-Client-Benchmark (HTTP, je 5 Iterationen)

| Concurrent Clients | avg (ms) | p50 (ms) | CV (%) | Anmerkung |
|--------------------|----------|----------|--------|-----------|
| 1 | 1.281 | 1.275 | 1.1 | stabil, keine Contention |
| 5 | 6.800 | 6.742 | 2.5 | linear skalierend |
| 10 | 4.439 ⚠️ | 13.678 ⚠️ | 467 % ⚠️ | **Anomalie**: avg < p50, negative min → Messfehler |
| 25 | 35.464 | 35.754 | 4.1 | stabil, Serialisierungsoverhead |
| 50 | 60.317 | 69.439 | 38.1 % | hohe Varianz, Lock-Contention wahrscheinlich |

> ⚠️ **10-Client-Anomalie**: CV=467 %, min=-32 ms (Messfehler im HTTP-Timing). Reale Performance ca. 13–14 ms p50. Dieser Befund korreliert mit dem bekannten CV >20 % bei 10-Client-Lasttest (§37.5 P-6).

---

### 38.3 Docker-Benchmark-Vergleich: ThemisDB vs. Competitors (v1.0.1, 09.12.2025)

> Methodik: Docker-Container, native Client-Bibliotheken, 155 Messpunkte über 5 Workloads/Protokolle.  
> Avg-Werte gelten über TCP+HTTP+gRPC sofern nicht anders angegeben.

#### Relational Workload (insert / read / update / delete / range_query)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **0.56** | 0.504 | 0.728 | 0.84 | **1786 ops/s** | 568 | 27.8 | ✅ Schnellste |
| MySQL 8.0 | 0.80 | 0.720 | 1.040 | 1.20 | 1250 ops/s | 592 | 29.0 | +43 % langsamer |
| MariaDB 11 | 0.80 | 0.720 | 1.040 | 1.20 | 1250 ops/s | 592 | 29.0 | +43 % langsamer |
| PostgreSQL 16 | 0.96 | 0.864 | 1.248 | 1.44 | 1042 ops/s | 608 | 29.8 | +71 % langsamer |

> **Hinweis:** Die Latenz-Überlegenheit (~1.7×) entstand nach Einführung des direkten RocksDB-Pfads (kein SQL-Parser-Overhead). Gap-Analyse (v1.0.0) stellte noch 44–49 % schlechtere Latenz gegenüber PostgreSQL 16 fest — nach Optimierungen nun umgekehrt.

#### Dokument-Store Workload (insert / read / update / bulk_insert)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **0.875** | 0.787 | 1.137 | 1.312 | **1143 ops/s** | 600 | 29.4 | ✅ Schnellste |
| MongoDB | 1.625 | 1.463 | 2.113 | 2.438 | 615 ops/s | 675 | 33.1 | +86 % langsamer |
| CouchDB | 1.750 | 1.575 | 2.275 | 2.625 | 571 ops/s | 687 | 33.8 | +100 % langsamer |

> **Wichtige Gegenprobe** (benchmark_results_simple.json, 20251204): Python HTTP-Client gegen laufende Instanzen auf demselben Rechner — dort zeigte ThemisDB **47.56 ms** für Document Insert (vs. MongoDB 0.87 ms). Diese Abweichung ist auf den HTTP-Overhead des Python-Client-Skripts zurückzuführen (unkompilierter Client vs. nativer Client). Die Docker-Messung mit nativem Client ist maßgeblich.

#### Vektor-Store Workload (search / index / recall / range_search)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **1.05** | 0.945 | 1.365 | 1.575 | **952 ops/s** | 617 | 30.2 | ✅ Schnellste |
| Qdrant | 2.10 | 1.890 | 2.730 | 3.150 | 476 ops/s | 722 | 35.5 | +100 % langsamer |
| Milvus | 2.25 | 2.025 | 2.925 | 3.375 | 444 ops/s | 737 | 36.2 | +114 % langsamer |
| Weaviate | 2.70 | 2.430 | 3.510 | 4.050 | 370 ops/s | 782 | 38.5 | +157 % langsamer |

#### Graph-Workload (node_insert / edge_insert / traversal / shortest_path)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **1.75** | 1.575 | 2.275 | 2.625 | **571 ops/s** | 687 | 33.8 | ✅ Schnellste |
| ArangoDB | 4.25 | 3.825 | 5.525 | 6.375 | 235 ops/s | 937 | 46.2 | +143 % langsamer |
| Neo4j | 5.00 | 4.500 | 6.500 | 7.500 | 200 ops/s | 1012 | 50.0 | +186 % langsamer |

#### Geo-Workload (point_insert / radius_search / polygon_search)

| Datenbank | avg (ms) | p50 | p95 | p99 | Throughput | Mem (MB) | CPU % | Bewertung |
|-----------|----------|-----|-----|-----|------------|----------|-------|-----------|
| **ThemisDB** | **1.312** | 1.181 | 1.706 | 1.969 | **762 ops/s** | 643 | 31.6 | ✅ Schnellste |
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
| **Document + Graph** | **ThemisDB** | **0.88** | 0.83 | 1.21 | 1.37 | 1.8× langsamer ⚠️ |
| **Document + Vector** | MongoDB + Qdrant | 0.73 | 0.68 | 1.07 | 1.64 | Referenz |
| **Document + Vector** | **ThemisDB** | **0.88** | 0.81 | 1.31 | 1.40 | 1.2× langsamer |
| **OLAP + Document** | ClickHouse + MongoDB | 1.70 | 1.68 | 2.22 | 2.33 | Referenz |
| **OLAP + Document** | **ThemisDB** | **1.06** | 0.95 | 1.51 | 1.96 | ✅ 1.6× schneller |

> ThemisDB schlägt Spezialsysteme (ClickHouse+MongoDB) bei OLAP+Document um 38 %, liegt aber bei Document+Graph hinter dem PostgreSQL+Neo4j-Combo (kein Überraschung: kein Transaktionsoverhead zwischen zwei separaten DBs). **Ziel:** Document+Graph ≤ 0.6 ms avg (Q3 2026).

---

### 38.5 Acceleration-Modul Baseline (CPU ANN, Referenzwerte für Regression-Tests)

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
| BM_CPU_ANN_TopK (k=10) | – | 1000 | 20.0 M/s |
| BM_CPU_ANN_TopK (k=50) | – | 1000 | 11.1 M/s |
| BM_CPU_ANN_TopK (k=10) | – | 5000 | 25.0 M/s |
| BM_CPU_ANN_TopK (k=50) | – | 5000 | 12.5 M/s |
| BM_CPU_BatchKNN (128d, k=10) | 128 | 1000 | 1.08 M/s |
| BM_CPU_BatchKNN (256d, k=10) | 256 | 1000 | 570 k/s |
| BM_CPU_BatchKNN (512d, k=10) | 512 | 1000 | 308 k/s |
| BM_CPU_Geo_HaversineDistance | – | 1000 | 20.0 M/s |
| BM_CPU_Geo_HaversineDistance | – | 10000 | 22.2 M/s |
| BM_CPU_Geo_HaversineDistance | – | 100000 | 22.2 M/s |
| BM_CPU_Geo_PointInPolygon | – | 1000 | 33.0 M/s |
| BM_CPU_Geo_PointInPolygon | – | 10000 | 35.7 M/s |
| BM_CPU_Geo_PointInPolygon | – | 100000 | 35.7 M/s |

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

| Version | Datum | Query Engine (M items/s) | Vector Insert (k/s) | Index Insert (k/s) | Embedding Cache (items/s) | 2PC (ops/s) | Benchmark-Anzahl | Wichtigste Änderung |
|---------|-------|--------------------------|---------------------|---------------------|---------------------------|-------------|-----------------|---------------------|
| v1.3.0 | 2025-09-15 | 700 | 280 | 180 | – | – | 450 | Initial Release |
| v1.3.1 | 2025-09-29 | 750 | 300 | 190 | – | – | 480 | Query Optimizer Improvements |
| v1.3.2 | 2025-10-31 | 800 | 330 | 210 | – | – | 520 | SIMD Vectorization + Compression |
| v1.3.3 | 2025-11-30 | 800 | 340 | 215 | – | – | 780 | Parallelization + Advanced Patterns |
| v1.3.4 | 2025-12-29 | 814.5 | 351.4 | 217.2 | 155.8 M/s | 6.4 k | 1078 | Neu: Cache, 2PC, Hybrid Search |
| **v2.0.0** | 2026-04-09 | **814.5** | **351.4** | **217.2** | **155.8 M/s** | **29.3 k** | **1089** | PERF-D4: 2PC thread pool + batched prepare |


---

## 39. API- und Schnittstellen-Performance-Annahmen (aus `src/` extrahiert)

> Quellen: FUTURE_ENHANCEMENTS.md, ROADMAP.md, README.md der jeweiligen Module unter `src/`.  
> Typ-Legende: **[Z]** = Ziel/Target (noch nicht gemessen), **[M]** = gemessener Wert, **[I]** = Implementiert/bestätigt

---

### 39.1 API-Modul (`src/api/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| `IHttpHandler::handle()` Dispatch-Overhead (Router-Lookup + Invocation) | ≤ 5 µs / Req @ 10k RPS | [Z] | FUTURE_ENHANCEMENTS.md L80 |
| `IGraphQLSchemaBuilder` Type-Lookup (Query-Planning) | ≤ 1 µs / Field-Resolution | [Z] | FUTURE_ENHANCEMENTS.md L81 |
| WebSocket Frame-Dispatch via `IWebSocketFrameCallback` | ≤ 10 µs / Frame | [Z] | FUTURE_ENHANCEMENTS.md L82 |
| `IAPIVersionRouter::route()` Version-Extraktion + Handler-Auflösung | ≤ 2 µs | [Z] | FUTURE_ENHANCEMENTS.md L83 |
| `ICorrelationIDProvider::generate()` UUID-Generierung | ≤ 500 ns / Call | [Z] | FUTURE_ENHANCEMENTS.md L84 |
| `IGRPCBridge::dispatch()` Protobuf→Internal-Konvertierung | ≤ 20 µs / RPC-Call | [Z] | FUTURE_ENHANCEMENTS.md L85 |
| GraphQL parse + validate + execute (10-Feld-Query, 500 concurrent HTTP/2) | < 2 ms p99 | [Z] | README.md L56, FE L50 |
| GraphQL parse+execute aktuell (Schätzung) | ~5 ms | [M est.] | FUTURE_ENHANCEMENTS.md L260 |
| gRPC unary `GetDocument` Added-Latency vs. äquivalentem REST-Call | < 1 ms | [Z] | README.md L73, FE L135 |
| WebSocket Event-Delivery-Latenz (Changefeed→Frame) | < 50 ms | [Z] | FUTURE_ENHANCEMENTS.md L51 |
| WebSocket Frame-Delivery p99 @ 5 000 events/s | < 30 ms | [Z] | FUTURE_ENHANCEMENTS.md L87 |
| Bulk-Insert 10 000 256-Byte-Dokumente (ohne Netzwerk) | < 500 ms | [Z] | FUTURE_ENHANCEMENTS.md L105 |
| SSE Streaming First-Byte-Latenz (nach Query-Planning) | < 5 ms | [Z] | FUTURE_ENHANCEMENTS.md L106 |
| Middleware-Overhead (UUID + Thread-Local Write) | < 10 µs / Req | [Z] | README.md L115, FE L154 |
| OTLP Span-Enqueue (Hot-Path, single lock + push_back) | < 500 ns / Span | [Z] | FUTURE_ENHANCEMENTS.md L172 |
| OTLP Flush (64 Spans → lokaler OTLP-Collector, persistent conn) | < 5 ms | [Z] | FE L173 |

---

### 39.2 gRPC/RPC-Modul (`src/rpc_grpc/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| gRPC Health-Check (`SERVING`) nach `start()` | Sofort, `grpc_health_probe` exit 0 | [I] | FE L24–25 |
| gRPC Prometheus-Histogramm Latency (per method) | verfügbar unter `/metrics` | [Z] | FE L45 |
| TLS-Zertifikat Hot-Rotation (neue Connections) | ≤ 1 Verbindung mit altem Cert | [Z] | FE L96 |
| QUIC/HTTP3 Verbindungsaufbau (0-RTT Resumption) | Ziel: < 2 ms p99 | [Z] | FE L11 |
| gRPC Transport Port 8771 (bidirektionales Streaming) | standard | [I] | FE L12 |

---

### 39.3 Network-Modul (`src/network/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| TCP Accept | 1–5 ms | [M] | README.md L1052 |
| TLS 1.3 Handshake (neue Verbindung) | 10–50 ms (README); < 5 ms p99 (ROADMAP) | [M]/[Z] | README.md L1053, FE L288 |
| TLS 1.3 Session Resumption | < 1 ms p99 | [Z] | FE L288 |
| Frame Read/Write (Zero-Copy) | 100–500 µs | [M] | README.md L1054 |
| Connection Pool Acquire (Lock-Free Fast-Path) | 10–100 µs | [M] | README.md L1055 |
| Keep-Alive Check | 1–10 ms (alle 60 s) | [M] | README.md L1056 |
| Circuit-Breaker Check | ~1 µs (Lock-Free Atomic) | [M] | README.md L1057 |
| Wire-Protocol Round-Trip p99 (≤ 64 KiB Payload) | < 1 ms | [Z] | ROADMAP.md L66 |
| WebSocket Text-Frame Round-Trip (localhost) | < 2 ms p99 | [Z] | FE L289 |
| QUIC 0-RTT Verbindungsaufbau | < 2 ms p99 | [Z] | FE L290 |
| UDP Fast-Path GET Response (localhost) | < 500 µs p99 | [Z] | FE L291 |
| DPDK Kernel-Bypass Latenz | 1–10 µs | [Z] | FE L284 |
| DPDK Throughput | 100 Gbps | [Z] | FE L284 |
| io_uring Latenz | 10–50 µs | [Z] | FE L285 |
| io_uring Throughput | 10 Gbps | [Z] | FE L285 |

---

### 39.4 Server-Modul (`src/server/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| HTTP/1.1 Keep-Alive Sustained Throughput (4-Core, 1 KB Payload) | ≥ 50 000 req/s | [Z] | FE L1083 |
| p50 Latenz | ≤ 5 ms | [Z] | FE L1084, ROADMAP L26 |
| p99 Latenz @ 80 % CPU | ≤ 50 ms | [Z] | FE L1084, ROADMAP L26 |
| TLS 1.3 Handshake (ECDSA P-256, Commodity HW) | ≤ 2 ms | [Z] | FE L1086 |
| Rate-Limiter State Sync (Distributed Token Bucket) | ≤ 10 ms Propagation Delay | [Z] | FE L18, ROADMAP L54 |
| Redis Round-Trip (Rate-Limit Check, same LAN) | ≤ 5 ms p99 | [Z] | ROADMAP L57 |
| Rate-Limit Throughput per Node | ≥ 50 000 checks/s | [Z] | ROADMAP L57 |
| Raft Config Propagation (5 Nodes, LAN) | ≤ 100 ms | [Z] | ROADMAP L65 |
| Leader Failover via `leader_failover_timeout` | ≤ 500 ms | [I] | FE L172 |
| JWT Validation Overhead | 100–500 µs / Req | [M] | README.md L1346 |
| Auth Middleware p50/p99 | < 100 µs / < 500 µs | [Z] | README.md L1313 |
| Rate Limiter p50/p99 | < 50 µs / < 200 µs | [Z] | README.md L1314 |
| Entity CRUD p50/p99 | < 5 ms / < 50 ms | [Z] | README.md L1315 |
| Query Execution (einfach) p50/p99 | < 10 ms / < 100 ms | [Z] | README.md L1316 |
| Vector Search p50/p99 | < 10 ms / < 50 ms | [Z] | README.md L1317 |
| Request Wall-Clock Timeout | 500 ms default → HTTP 504 | [I] | FE L130 |
| Congestion p99 > 500 ms → Adaptive Rate Reduction | auf 50 % | [I] | FE L383 |
| WASM Function CPU-Time Limit | 500 ms default | [Z] | ROADMAP L74 |

---

### 39.5 Query-Modul (`src/query/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Parse + Optimize (≤ 10 Collections) | ≤ 5 ms p99 | [Z] | ROADMAP L198, FE L1393 |
| Simple AQL Execution (3-Node Cluster, warm Cache) | ≥ 10 000 queries/s @ p99 < 20 ms | [Z] | ROADMAP L199, FE L1394 |
| Exact-Match Cache Lookup (10 000 Concurrent Clients) | ≤ 1 ms p99 | [Z] | ROADMAP L200, FE L1395 |
| Semantic Cache Lookup (inkl. Embedding-Similarity) | ≤ 10 ms p99 | [Z] | FE L1396 |
| JIT First-Compile Latenz | ≤ 50 ms | [Z] | FE L1397 |
| JIT Execution Speedup (Arithmetic-Heavy) | ≥ 3× vs. Interpreter | [Z] | FE L1397 |
| Federation Cost-Schätzung (5-Cluster-Plan) | ≤ 20 ms | [Z] | FE L1398 |
| Streaming Result First-Chunk | ≤ 50 ms | [Z] | ROADMAP L201, FE L1399 |
| Query Cancellation (Memory + Locks freigegeben) | innerhalb 100 ms nach Signal | [Z] | FE L1408 |
| Optimizer `optimize()` (einfach, 1–2 Prädikate) | 0.1–5 ms | [M] | README.md L185 |
| Optimizer `optimize()` (komplex, 10+ Prädikate) | 5–50 ms | [M] | README.md L186 |
| Simple Query Execution (1–2 Prädikate) | 1–10 ms | [M] | README.md L256 |
| Complex Query (5–10 Prädikate, Joins) | 10–100 ms | [M] | README.md L257 |
| Graph Traversal (Depth 3–5) | 50–500 ms | [M] | README.md L258 |
| Hybrid Query (Vector+Geo) | 10–50 ms | [M] | README.md L259 |
| Fan-Out Latenz (16 Shards, LAN) | ≤ 200 ms | [Z] | ROADMAP L91 |

---

### 39.6 AQL-Modul (`src/aql/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Lexer Tokenisierung | ≥ 50 MB/s / Core (ASCII) | [Z] | FE L14, L772 |
| Parser AST-Konstruktion (64 KB Query) | ≤ 10 ms | [Z] | FE L15, L773 |
| Full Round-Trip (parse + execute, 10-Table-Join, 100k Rows) | ≤ 500 ms | [Z] | FE L774 |
| LLM Command Async-Dispatch (ohne Inferenz) | ≤ 5 ms / Command | [Z] | FE L775 |
| Query Optimizer Rewrite Pass | ≤ 2 ms / 1000 AST-Nodes | [Z] | FE L776 |
| Batch NL→AQL (10 Requests, mock LLM 50 ms, concurrency ≥ 4) | ≤ 150 ms Wall-Time | [I] | FE L159, L778 |
| AQL Validation Overhead | ≤ 1 ms / Generated Query | [Z] | FE L60 |
| Timeout-Thread Terminierung nach `executeWithTimeout()` | innerhalb `timeout + 500 ms` | [Z] | FE L788 |
| `push()` / `nextToken()` Overhead (ohne Modell-Generierung) | ≤ 500 ns | [Z] | ROADMAP L46 |
| Tool-Dispatch-Overhead (ohne Tool-Ausführung) | ≤ 1 ms / Step | [Z] | ROADMAP L55 |

---

### 39.7 Cache-Modul (`src/cache/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Prefetch Prediction Latenz | ≤ 100 µs / Call | [Z] | FE L102 |
| L3 Cache Hit-Path (RocksDB-backed) | ≤ 5 ms p99 | [Z] | FE L161 |
| Admin API Response | ≤ 5 ms unabhängig von L1-Cache-Größe | [Z] | FE L163 |
| Redis-Async Peer-Discovery (libuv-backed) | non-blocking | [I] | FE L82 |
| Distributed Cache Invalidation (alle Nodes) | propagiert innerhalb 500 ms | [Z] | FUTURE_ENHANCEMENTS core L572 |
| Distributed Cache `get` Round-Trip (Redis localhost) | ≤ 1 ms p99 | [Z] | core FE L582 |

---

### 39.8 Replication-Modul (`src/replication/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Replication Lag p99 (SEMI_SYNC, 3-Node LAN, 10k writes/s) | ≤ 50 ms | [Z] | FE L17, L841 |
| WAL-Shipping Throughput / Follower (Zstd Level 3, 10 GbE) | ≥ 500 MB/s | [Z] | FE L18, L842 |
| Vector-Clock / HLC Conflict-Detection Overhead | < 5 µs / Write-Op | [Z] | FE L20, L844 |
| CRDT Merge Latenz (G-Counter / LWW-Register) | ≤ 1 µs / Merge | [Z] | FE L845 |
| Point-in-Time Recovery WAL Replay | ≥ 200 MB/s; 100 GB in ≤ 10 min | [Z] | FE L846 |
| CDC Event Emission (Commit → Queue Enqueue) | ≤ 1 ms p99 | [Z] | FE L847 |
| Cross-Datacenter Replication Lag (ASYNC, 50 ms RTT WAN) | ≤ 200 ms p99 | [Z] | FE L848 |
| Async Mode Latenz | < 1 ms | [M] | README.md L952 |
| Semi-Sync Mode Latenz | 1–5 ms | [M] | README.md L953 |
| Sync Mode Latenz | 2–10 ms | [M] | README.md L954 |
| Tier 1 Critical SLA (SYNC, 3+ Replicas) | ≤ 10 ms | [Z] | ROADMAP L194 |
| Tier 2 Standard SLA (SEMI_SYNC, 2 Replicas) | ≤ 50 ms | [Z] | ROADMAP L194 |
| WAL Append Throughput | > 50 000 entries/s | [I] | ROADMAP L242 |
| WAL `readFrom` 1000 Entries | < 5 ms | [I] | ROADMAP L242 |
| WAL Serialize/Deserialize | < 2 µs | [I] | ROADMAP L242 |

---

### 39.9 Storage-Modul (`src/storage/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Point Read (Cache Hit) | 10–50 µs | [M] | ARCHITECTURE.md L189, README.md L107 |
| Point Read (Cache Miss / Disk) | 100–500 µs | [M] | ARCHITECTURE.md L189, README.md L675 |
| Hot-Tier (NVMe) | < 1 ms | [Z] | FE Storage |
| Warm-Tier (SATA) | ~5 ms | [Z] | FE Storage |
| Cold-Tier (S3) | ~50 ms | [Z] | FE Storage |
| Sustained Write Throughput (NVMe, 256er Batch, 4 KB avg) | ≥ 100 000 ops/s | [Z] | FE L738 |
| p99 Point-Read (Hot-Tier, Bloom-Filter enabled) | ≤ 1 ms | [Z] | FE L739 |
| Incremental Backup Throughput (NVMe, parallel SSTable) | ≥ 500 MB/s | [Z] | FE L740 |
| Streaming Ingest End-to-End Latenz | ≤ 50 ms | [Z] | FE general |
| Streaming Ingest Throughput | 1 M events/s | [Z] | FE general |
| Erasure Coding 6+3 Overhead | 50 % (vs. RAID-1 200 %) | [Z] | FE general |
| RocksDB WriteBatch Commit Latenz (Vector Add) | < 2 ms p99 | [Z] | index FE L970 |

---

### 39.10 CDC-Modul (`src/cdc/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Sequence Generation Throughput (8 Writer-Threads) | ≥ 200 k/s | [I] | FE L405 — Lock-free `atomic<uint64_t>` |
| Event Delivery p99 (Changefeed → WebSocket Frame) | < 20 ms | [Z] | FE L334 |
| Consumer Group Offset Commit (RocksDB Write) | < 1 ms p99 | [Z] | FE L365 |
| End-to-End Latenz (Change → Kafka `ack`, LAN) | < 10 ms p99 | [Z] | FE L387 |
| Compaction I/O Bandwidth Cap | 50 MB/s (konfigurierbar) | [Z] | FE L425 |
| SSE Event Delivery p99 (aktuell) | < 50 ms (Schätzung) | [M est.] | FE L461 |

---

### 39.11 Sharding-Modul (`src/sharding/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Cross-Shard RPC p99 (LAN, ohne Consensus) | < 5 ms | [Z] | FE L85 — aktuell ~18 ms |
| Cross-Shard RPC aktuell (gemessen) | ~18 ms | [M] | FE L179 |
| 2PC Commit (5 Shards) aktuell | ~35 ms | [M] | FE L180 |
| 2PC Commit Ziel (5 Shards) | < 15 ms | [Z] | FE L180 |
| Percolator Commit (10 Shards) | < 20 ms p99 | [Z] | FE L104, L181 |
| Topology Change Propagation (100-Node Cluster) | ≤ 500 ms | [Z] | FE L13, L255 — aktuell ~1.2 s |
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
| Hybrid Search (BM25 + HNSW RRF, Top-10, 10 M Docs) | ≤ 20 ms p99 | [Z] | FE L471 |
| LLM Query-Rewriter Overhead | ≤ 200 ms Added Latency p99; 0 ms wenn LLM unavailable | [Z] | FE L471 |
| Facet Counting (1 000 Werte, 100k Docs) | ≤ 5 ms | [Z] | FE L472 |
| LTR Re-Ranking (Top-100, 6-dim Linear Model) | ≤ 2 ms | [Z] | FE L473 |
| Autocomplete Suggestion (1 M-Term Dictionary) | ≤ 5 ms p99 | [Z] | FE L475 |
| BM25/FTS Query Latenz | 1–10 ms | [M] | README.md L113 |
| Vector Search Query Latenz | 1–10 ms (k=10, 1M vectors) | [M] | README.md L119 |
| Hybrid Search Query Latenz | 5–20 ms | [M] | README.md L125 |

---

### 39.13 Security-Modul (`src/security/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| AES-256-GCM Encrypt/Decrypt Throughput (AES-NI, 1 Core) | ≥ 1 GB/s | [Z] | FE L967, ROADMAP L130 |
| RSA-4096 Signature Verification | p99 ≤ 5 ms | [Z] | FE L968 |
| Kyber-1024 Key Encapsulation | ≥ 2 000 ops/s | [Z] | FE L969, ROADMAP L132 |
| Dilithium-5 Signing | ≥ 1 000 ops/s | [Z] | FE L970, ROADMAP L133 |
| TLS 1.3 Handshake (ECDHE-AES256-GCM) | p99 ≤ 10 ms | [Z] | FE L971 |
| RBAC Policy Evaluation (≤ 100 Roles) | p99 ≤ 0.5 ms | [Z] | FE L972 |
| HSM-backed RSA-2048 Sign (SoftHSM2 Baseline) | p99 ≤ 20 ms | [Z] | FE L973 |
| Audit Log Tamper-Evident Append | p99 ≤ 2 ms / Entry | [Z] | FE L974, ROADMAP L136 |
| Encryption Overhead / Feld (256-Byte Payload) | ~5–10 µs | [M] | README.md L194 |
| Decryption Overhead / Feld | ~3–7 µs | [M] | README.md L195 |
| Key Cache Lookup (In-Memory) | ~100 ns | [M] | README.md L196 |
| Vault API Call (gecacht, 1 Std.) | ~50–100 ms | [M] | README.md L197 |
| HSM Operation (Hardware) | ~5–20 ms | [M] | README.md L198 |
| Document Insert mit Verschlüsselung | 1.4 ms (+16 % vs. plain) | [M] | README.md L859 |
| Document Query mit Verschlüsselung | 1.1 ms (+37 % vs. plain) | [M] | README.md L860 |
| Bulk Insert 1k Docs mit Verschlüsselung | 1050 ms (+23 % vs. plain) | [M] | README.md L861 |

---

### 39.14 Analytics-Modul (`src/analytics/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| IVM Delta-Application (≤ 10 000 Rows) | ≤ 50 ms | [Z] | FE L22, L32 |
| IVM Reader p99 während 10k-Row-Batch-Apply | ≤ 10 ms | [Z] | FE L209 |
| CSV Export 1 M Rows (Streaming, kein Full In-Memory) | ≤ 500 ms | [Z] | FE L81 |
| CEP Engine `stop()` | ≤ 100 ms | [Z] | FE L104 |
| CEP `process()` Lock-Hold-Dauer | ≤ 50 µs | [Z] | FE L130 |
| IsolationForest Training (1000-Punkt-Window) | ≤ 10 ms | [Z] | FE L131 |
| CEP p99 Latenz (8 Threads @ 100 kHz) | ≤ 1 ms | [Z] | FE L127 |
| `putInCache()` / `getFromCache()` | O(1) amortisiert, ≤ 1 µs p99 (16 Concurrent) | [Z] | FE L236 |
| `getCacheKey()` (500-Event Trace, Hash-basiert) | ≤ 50 µs | [Z] | FE L237 |
| Einfache Aggregation SUM (1 M Rows) | 15 ms (66k rows/s) | [M] | README.md L1193 |
| Einfache Aggregation SUM (10 M Rows) | 142 ms (70k rows/s) | [M] | README.md L1194 |
| GROUP BY 1 Dim. (1 M Rows) | 45 ms (22k rows/s) | [M] | README.md L1195 |
| GROUP BY 1 Dim. (10 M Rows) | 425 ms (23k rows/s) | [M] | README.md L1196 |
| GROUP BY 3 Dim. (1 M Rows) | 120 ms (8.3k rows/s) | [M] | README.md L1197 |
| Window Function ROW_NUMBER (1 M Rows) | 80 ms (12.5k rows/s) | [M] | README.md L1199 |
| Window Function Moving Average (1 M Rows) | 95 ms (10.5k rows/s) | [M] | README.md L1200 |
| Complex OLAP CUBE (1 M Rows) | 350 ms (2.8k rows/s) | [M] | README.md L1201 |
| Complex OLAP ROLLUP (1 M Rows) | 280 ms (3.5k rows/s) | [M] | README.md L1202 |
| SIMD SUM (10 M Rows) | 28 ms (5.1× Speedup vs. Scalar 142 ms) | [M] | README.md L1207 |
| SIMD AVG (10 M Rows) | 35 ms (4.5× Speedup) | [M] | README.md L1208 |
| SIMD MIN/MAX (10 M Rows) | 18 ms (6.9× Speedup) | [M] | README.md L1209 |
| SIMD Complex Filter (10 M Rows) | 45 ms (4.7× Speedup) | [M] | README.md L1210 |
| JSON Export (100k Rows) | 250 ms (400k rows/s, 45 MB) | [M] | README.md L1216 |
| Fan-Out Latenz (16 Shards, LAN) | ≤ 200 ms | [Z] | ROADMAP L72 |
| Model Export (≤ 1 M Samples) | ≤ 500 ms | [Z] | ROADMAP L86 |

---

### 39.15 Timeseries-Modul (`src/timeseries/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Single-Point Insert p99 (Gorilla compressed) | ≤ 50 µs | [Z] | FE L39 |
| Gorilla on-disk compression (1000 Punkte) | ≤ 15 % raw size | [Z] | FE L39 |
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
| Begin-Latenz | < 1 µs | [M] | README.md L130 |
| Commit-Latenz (abhängig von Batch-Größe) | 100 µs–5 ms | [M] | README.md L130 |
| Lock-Overhead / Lock-Acquire | ~5 ns (Atomics) | [M] | README.md L131 |
| Deadlock-Detection Intervall (konfigurierbar) | 100 ms | [M] | README.md L132 |
| Lock-Free Read (Fast-Path, kein Contention) | < 10 ns | [M] | README.md L820 |
| Stats Collection / Operation | < 5 ns (Atomic Increment) | [M] | README.md L819 |
| OCC Commit p50 → aktuell | 1 ms | [M] | FE L872 |
| OCC Commit p99 → aktuell | 10 ms | [M] | FE L872 |
| OCC Commit p50 → Ziel | 100 µs | [Z] | FE L872 |
| OCC Commit p99 → Ziel | 5 ms | [Z] | FE L873 |
| SAGA Compensation Time → aktuell | 100 ms | [M] | FE L875 |
| SAGA Compensation Time → Ziel | 20 ms | [Z] | FE L875 |
| Distributed 2PC Latenz → aktuell | 10 ms | [M] | FE L876 |
| Distributed 2PC Latenz → Ziel | 5 ms | [Z] | FE L876 |
| Batch Window (konfigurierbar) | 1–100 ms | [I] | FE L495 |
| Retry-Kosten / Versuch | ~1 ms | [M] | FE L163 |
| Deadlock-Watchdog Fallback-Timer | innerhalb 500 ms | [Z] | FE L938 |
| Conflict Detection | ~1 ms / 1000 Keys | [M] | README.md L656 |

---

### 39.17 Index-Modul (`src/index/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| HNSW Vector Search (1M 128-dim, k=10) CPU | ≥ 5 000 QPS | [Z] | FE L964 |
| HNSW Vector Search (1M 128-dim, k=10) GPU (RTX) | ≥ 50 000 QPS | [Z] | FE L964 |
| B-Tree Secondary Index Point Lookup (10M Keys) | < 500 µs p99 | [Z] | FE L966 |
| R-Tree Spatial Range Query (1M Punkte, 1 % Selectivity) | < 10 ms p99 | [Z] | FE L967 |
| HNSW CPU Brute-Force Query (1M vectors) | 10–100 ms | [M] | README.md L882 |
| HNSW CPU Query | 0.1–1 ms | [M] | README.md L883 |
| HNSW GPU (Vulkan, Batch) | 0.01–0.1 ms | [M] | README.md L884 |
| B-Tree Point Lookup (mit Cache) | 10–50 µs | [M] | README.md L298 |
| R-Tree Bounding Box | 1–10 ms | [M] | README.md L487 |
| R-Tree Radius Search | 1–20 ms | [M] | README.md L488 |
| Generic Loop Scan | ~1 GB/s | [M] | FE L398 |
| AVX-512 SIMD Scan (geplant) | ~50 GB/s | [Z] | FE L399 |

---

### 39.18 Geo-Modul (`src/geo/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| `intersects` (1M Punkte, linear) | ~2 000 ms | [M] | FE L210 |
| `intersects` (1M Punkte, R-Tree) | ≤ 5 ms p99 | [Z] | FE L73, L210 |
| ST_BUFFER (10k Punkte @ 500 m, CPU) | ≤ 200 ms | [Z] | FE L98, L212 |
| ST_BUFFER (10k Punkte @ 500 m, A10G) | ≤ 20 ms (10× CPU) | [Z] | FE L352 |
| GPU Contains (1M Punkte, A10G) | ≤ 50 ms | [Z] | FE L213 |
| Spatial JOIN (2 × 100k Punkte, 1 km, erste 1000 Ergebnisse) | ≤ 500 ms | [Z] | FE L126 |
| `sampleAt` (1M-Cell Grid) | ≤ 1 µs / Call | [Z] | FE L150 |
| `queryBBox` (10k Cells aus 1M-Cell Grid) | ≤ 10 ms | [Z] | FE L151 |
| `generateHeatmap` (100k Punkte, 100×100, 500 m BW) | ≤ 500 ms | [Z] | FE L152 |
| Ellipsoidal ST_Distance (1M Paare, CPU) | ≤ 500 ms | [Z] | FE L275 |
| Ellipsoidal ST_Distance (1M Paare, A10G) | ≤ 50 ms | [Z] | FE L276 |
| ST_UNION (1000 Polygon-Paare, A10G) | ≤ 10 ms | [Z] | FE L353 |
| `locationAtTime` (100k Rows) | ≤ 1 ms | [Z] | FE L193 |
| `entitiesWithinDistanceAtTime` (10k Entities, linear) | ≤ 50 ms | [Z] | FE L194 |

---

### 39.19 Acceleration-Modul (`src/acceleration/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| CUDA L2-Search (1M × 128-dim, RTX 3090) | < 8 ms | [Z] | FE L45, L427 |
| Cosine Search Vulkan/MoltenVK (500k × 128-dim, M2 Pro) | < 20 ms ✅ | [I] | FE L428 |
| GPU Distributed Index (100M × 128-dim, 4× A100, k=100) | < 15 ms p99 | [Z] | FE L79, L369 |
| NCCL `mergeTopK` (worldSize=4, k=100, NVLink-3) | < 500 µs | [Z] | FE L80, L432 |
| Device Probe (4-GPU System) | < 50 ms ✅ | [I] | FE L431 |
| `getStats()` Call Latenz (Linux /proc/stat) | < 2 ms ✅ | [I] | FE L434 |
| `canUseGPU()` NVML-Timeout-Guard | 500 ms Timeout → false (CPU-Fallback) | [I] | FE L443 |
| CPU Monitoring `/proc/stat` Polling-Intervall | 100 ms | [I] | FE L131 |

---

### 39.20 LLM-Modul (`src/llm/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Time-to-First-Token (512-Token Prompt, A10G) aktuell | ~350 ms (Schätzung) | [M est.] | FE L238 |
| Time-to-First-Token (512-Token Prompt, A10G) Ziel | ≤ 200 ms p99 | [Z] | FE L138, L238 |
| TTFT Bypass DeduplicationCache für Streaming | aktiviert (TTFT ≤ 200 ms) | [I] | FE L125 |
| OpenAI-Compat Adapter Round-Trip Overhead | ≤ 2 ms vs. direktem `submitRequest()` | [I] | FE L165 |
| Work-Stealing Pool Task Dispatch | ≤ 50 µs p99 (submit → Worker Pickup) | [Z] | FE L185, L241 |
| LoRA Adapter Application | < 1 ms Overhead | [M] | llama_lora_adapter_README L163 |
| Incomplete-Stream Warning (EOF ohne Marker) | innerhalb 500 ms | [Z] | FE L86 |

---

### 39.21 RAG-Modul (`src/rag/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Fast Evaluation Mode E2E | ≤ 100 ms p99 (kein LLM-Call) | [I] | FE L17, ROADMAP L28 |
| Balanced Evaluation Mode E2E | ≤ 500 ms p99 | [I] | FE L18 |
| Thorough Evaluation Mode E2E | ≤ 2 000 ms p99 | [Z] | FE L18 |
| StreamingRetriever First-Chunk | ≤ 50 ms | [Z] | FE L767 |
| ClaimExtractor (1000-Zeichen Antwort, LLM-First) | ≤ 500 ms | [Z] | FE L769 |
| ClaimExtractor (heuristischer Fallback) | ≤ 50 ms | [Z] | FE L769 |
| RAG Query E2E (Vector Search + LLM Generation) | 50–500 ms | [M] | aql README L165 |

---

### 39.22 Observability-Modul (`src/observability/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Metrics Collection Overhead | < 1 % CPU @ 1 000 req/s | [Z] | FE L20, L1221 |
| Prometheus `/metrics` Scrape Response | < 50 ms p99 @ 10 000 active series | [Z] | FE L1225 |
| Span Creation + In-Process Propagation | < 5 µs / Span | [Z] | FE L1226 |
| OTLP Export Latenz (async, 1 000 spans/s) | < 5 ms p99 | [Z] | FE L1227 |
| `QueryProfiler` per-Operator Timing Overhead | < 1 µs / Operator Boundary | [Z] | FE L1228 |
| CPU Sampling Period | ~100 ms (1 % CPU Overhead) | [I] | README.md L630 |
| Query P99 Alert-Threshold (Default) | > 1 000 ms | [I] | ROADMAP L58 |

---

### 39.23 Performance-Modul (`src/performance/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| RDTSC/RDTSCP Measurement Overhead (x86-64) | < 1 ns / Messpunkt | [I] | FE L20, ROADMAP L87 |
| RAII Scoped Timer Overhead (1M Iterationen) | < 2 ns / Call average | [Z] | FE L809 |
| P99-Percentile-Lookup (Ring bis 1 M Samples) | < 500 ns | [Z] | FE L821 |
| GPU Metric Export Overhead (CUDA Stream / Inference) | < 100 µs | [Z] | FE L823 |
| PMU Counter Read (`perf_event_open`) | < 1 µs | [Z] | FE L825 |
| Query Compilation Time | < 100 ms | [Z] | FE L235 |
| No-Op Adapter | < 1 ns / Call | [M] | core README L319 |
| Spdlog Async Adapter | ~50–100 ns / Log Call | [M] | core README L320 |
| Prometheus Metrics Update | ~200–500 ns | [M] | core README L321 |
| OTEL Span Creation | ~1–5 µs | [M] | core README L322 |

---

### 39.24 ONNX/CLIP-Modul (`src/onnx_clip/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| ViT-B/32 Image Encoding (CPU) | ≤ 150 ms / Image | [Z] | AUDIT L51, ROADMAP L43 |
| ViT-B/32 CUDA Batch-64 | ≤ 20 ms (≤ 0.31 ms / Image) | [Z] | FE L30 |
| Text Encoding (CPU) | ≤ 5 ms p95 | [Z] | FE L56, L59 |
| Metrics Collection Overhead | ≤ 0.05 ms / Call | [Z] | FE L100 |

---

### 39.25 Content-Modul (`src/content/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| DOCX Extraktion (500 KB) | < 200 ms | [Z] | FE L43 |
| NDJSON Streaming Ingestion (1 GB, NVMe) | ≥ 100 MB/s | [Z] | FE L102, ROADMAP L107 |
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
| HTTP GET Round-Trip Overhead (vs. raw TCP) | ≤ 5 ms | [Z] | FE L69 |
| Kafka → ThemisDB E2E Latenz | ≤ 500 ms p99 | [Z] | FE L89 |
| S3 `ListObjectsV2` (1000 Objekte) | ≤ 100 ms | [Z] | FE L109 |
| S3 Concurrent Downloads (4 parallel, 10 Gbps) | ≥ 200 MB/s aggregate | [Z] | FE L110, L189 |
| Per-Dokument Quarantäne Retry (≤ 1 MB) | ≤ 10 ms | [Z] | FE L146, L190 |

---

### 39.27 Exporters-Modul (`src/exporters/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| JSONL Export Throughput (aktuell) | ~150 MB/s (Full Batch) | [M] | FE L107 |
| JSONL Export Throughput (Ziel) | ≥ 200 000 docs/s sustained | [Z] | FE L107 |
| Parquet Export (Arrow Path, uncompressed) | ≥ 500 MB/s | [Z] | FE L109 |
| Retry Initial Delay (konfigurierbar, Default) | 500 ms (doubles each retry) | [I] | README.md L193 |

---

### 39.28 Chimera-Modul (`src/chimera/`)

| Schnittstelle | Ziel/Messwert | Typ | Quelle |
|---------------|---------------|-----|--------|
| Vector Search (k=10, 1M Vectors) | 1–10 ms | [M] | README.md L799 |
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

