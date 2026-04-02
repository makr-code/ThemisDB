# ThemisDB – Performance-Erwartungswerte & Messergebnisse

> Stand: 2026-04-02 | Quellen: `FUTURE_ENHANCEMENTS.md` je Modul, `benchmarks/results_analysis_reports/`, `benchmarks/baselines/`, `benchmarks/VERSION_HISTORY.csv`
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

## 1. Versionshistorie – Kernmetriken

> Quelle: `benchmarks/VERSION_HISTORY.csv` + `benchmarks/results_analysis_reports/benchmark_summary.csv`
> Testplattform v1.3.0–v1.3.3: Intel i9-10900K (10C/20T @ 3.70 GHz), 31 GB RAM, WSL2 Linux
> Testplattform v1.3.4: Windows x64, 20 Cores @ 3.696 GHz, 20 MB L3-Cache

| Metrik | Ziel | v1.3.0 | v1.3.1 | v1.3.2 | v1.3.3 | **v1.3.4** | Δ v1.3.0→v1.3.4 | Status |
|--------|------|--------|--------|--------|--------|-----------|-----------------|--------|
| Query Engine Throughput | – | 700 M ops/s | 750 M ops/s | 800 M ops/s | 800 M ops/s | **814,5 M ops/s** | +16 % | ❓ |
| Vector Insert | – | 280 k/s | 300 k/s | 330 k/s | 340 k/s | **351,4 k/s** | +25 % | ❓ |
| Secondary Index Insert | – | 180 k/s | 190 k/s | 210 k/s | 215 k/s | **217,2 k/s** | +21 % | ❓ |
| Embedding Cache Hit-Rate | – | – | – | – | – | **155,8 M/s** | n/a | ❓ |
| 2PC Throughput | – | – | – | – | – | **6,4 k/s** | n/a | ❓ |
| Graph Edge Ops | – | – | – | – | – | **628,7 k/s** | n/a | ❓ |
| Timeseries Insert | – | – | – | – | – | **49,0 M pts/s** | n/a | ❓ |
| Gesamt Benchmark-Tests | – | 450 | 480 | 520 | 780 | **1.078** | +140 % | ✅ |

---

## 2. Query-Engine – Detailergebnisse

> Quelle: `BENCHMARK_RESULTS.md` (Run 2025-12-18), `benchmark_summary.csv` (Run 2025-12-29)

| Benchmark | Ziel | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|--------|
| Simple AQL WHERE | ≥ 10.000 Queries/s bei P99 < 20 ms | 3,43 M ops/s @ ~0,3 µs | ✅ |
| Complex WHERE | – | 3,35 M ops/s | ❓ |
| JOIN (Users-Posts) | – | 10,2 M ops/s | ❓ |
| QueryEngineBench/SimpleEvaluation | – | 814,5 M items/s (1,23 ns) | ❓ |
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
| VectorIndexBench/InsertPlaintext | – | – | 351,4 k/s (2,84 µs) | ❓ |
| SecondaryIndexBench/IndexInsert | – | – | 217,2 k/s (4,60 µs) | ❓ |
| SecondaryIndexBench/RawWriteOnly | – | – | 885,0 k/s (1,13 µs) | ❓ |
| Small Index Insert (1K entities) | – | – | 1,75 M/s | ❓ |
| Medium Index Insert (100K) | – | – | 1,06 M/s | ❓ |
| Large Index Lookup (1M) | – | – | 3,12 M/s | ❓ |
| Composite Index Lookup | – | – | 2,40 M/s | ❓ |
| L2Distance/1000/512 | – | 313 k/s (3.200 ns) | ❓ | ❓ |
| CosineDistance/1000/512 | – | 250 k/s (4.000 ns) | ❓ | ❓ |
| TopK/5000/50 | – | 12,5 M/s (400 ns) | ❓ | ❓ |
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
| Geo_HaversineDistance/100000 | – | 22,2 M/s (4.500 ns) | ❓ | ❓ |
| Geo_PointInPolygon/100000 | – | 35,7 M/s (2.800 ns) | ❓ | ❓ |
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
| GraphIndexBench/AddEdges | – | 628,7 k edges/s (1,59 µs) | ❓ |
| Sparse Graph Edge Addition | – | 1,26 M edges/s | ❓ |
| Dense Graph Neighbor Query | – | 8,96 M queries/s | ❓ |
| Graph BFS Traversal (Depth-3) | – | 9,56 M traversals/s | ❓ |
| RAG Search Top-50 | – | 7,17 M ops/s (140 ns) | ❓ |
| Algorithmus-Selektion P99 (10M Nodes) | < 1 ms | ❓ | ❓ |
| Plan-Cache Lookup P99 | < 100 µs | ❓ | ❓ |
| Single-Refresh (10K Nodes) | ≤ 5 s / ≤ 200 ms (8 Worker) | ❓ | ❓ |
| Subgraph-Isomorphismus P95 | < 500 ms (100-Node-Pattern, 1M-Graph) | ❓ | ❓ |

---

## 10. Acceleration-Modul

> Quelle: `baselines/acceleration/baseline.json` (v1.0.0)

| Benchmark | Ziel | v1.0.0 Gemessen | v1.3.4 Gemessen | Status |
|-----------|------|-----------------|-----------------|--------|
| L2Distance/1000/64 | – | 2,0 M/s (500 ns) | ❓ | ❓ |
| L2Distance/1000/512 | – | 313 k/s (3.200 ns) | ❓ | ❓ |
| CosineDistance/1000/512 | – | 250 k/s (4.000 ns) | ❓ | ❓ |
| InnerProduct/1000/512 | – | 313 k/s (3.200 ns) | ❓ | ❓ |
| TopK/1000/10 | – | 20,0 M/s (50 ns) | ❓ | ❓ |
| TopK/5000/50 | – | 12,5 M/s (400 ns) | ❓ | ❓ |
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
| TX-3 2PC Throughput | – | 6,4 k/s | ❓ |
| TX-4 2PC Latenz (5 Shards) | 5 ms | ❓ | ❓ |
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

## 30. Chimera-Baseline

> Quelle: `baselines/chimera/baseline.json` (v1.5.0-dev)

| Workload | Throughput | Mean Latenz | P95 | P99 |
|----------|-----------|-------------|-----|-----|
| relational_sort | 42.503 ops/s | 0,024 ms | 0,023 ms | 0,034 ms |
| vector_dot_product | 75.835 ops/s | 0,013 ms | 0,013 ms | 0,024 ms |
| document_lookup | 2.956.804 ops/s | 0,000180 ms | 0,000200 ms | 0,000250 ms |
| graph_bfs | 40.373 ops/s | 0,025 ms | 0,025 ms | 0,033 ms |

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
| 2PC Throughput | 6,4 k items/s | 15 k items/s | TiDB 7.0 | 3. (Solide) | −134 % |
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
| D-4 | 2PC Throughput vs. TiDB | 6,4 k/s | 15 k/s | **−134 %** | Mittel |
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
| BM_DistributedTxn_2PC_Latency/2 Shards | 46.04 ms | **6.400 ops/s** | |
| BM_DistributedTxn_2PC_Latency/4 Shards | 46.09 ms | **6.400 ops/s** | |
| BM_DistributedTxn_2PC_Latency/8 Shards | 46.09 ms | **1.600 ops/s** | Overhead skaliert |
| BM_DistributedTxn_2PC_Latency/16 Shards | 45.95 ms | **1.280 ops/s** | |
| BM_DistributedTxn_Throughput | 46.01 ms | **6.400 ops/s** | |
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
| 2PC-Throughput (2 Shards) | – | 6.4 k/s | n/a (neu) | ✅ Neu |

> **Wichtige Relativierung:** Mehrere Regressionen (insb. SecondaryIndex, VectorIndex, Graph) sind auf geänderte Test-Infrastruktur zurückzuführen (per-test temp dirs, einzelne RocksDB-Transaktionen pro `put()`), nicht auf Produktions-Regressions — vgl. `PERFORMANCE_COMPARISON_V1.3.0_VS_V1.3.3.md`.

