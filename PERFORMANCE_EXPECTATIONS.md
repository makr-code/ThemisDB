# ThemisDB – Performance-Erwartungswerte & Messergebnisse

> Stand: 2026-04-02 | Quellen: `FUTURE_ENHANCEMENTS.md` je Modul, `benchmarks/results_analysis_reports/`, `benchmarks/baselines/`, `benchmarks/VERSION_HISTORY.csv`

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
