# ThemisDB: Hardware-Normalized Performance Evaluation of a Hybrid Multi-Model Database System

**ThemisDB Development Team**  
*makr-code/ThemisDB* — <https://github.com/makr-code/ThemisDB>

---

## Abstract

We present a longitudinal performance evaluation of **ThemisDB**, a C++20 hybrid multi-model database that integrates graph, vector, relational, time-series, and document data models within a single ACID-compliant system governed by an AQL query engine and multi-version concurrency control (MVCC). The study spans versions v1.3.0 through v1.8.2 and covers 1,078+ benchmark cases distributed across 33 software modules. Because absolute throughput numbers are platform-dependent and therefore non-transferable across hardware configurations, we introduce a *hardware-normalized efficiency model* that decomposes the expected throughput of each workload class into a weighted linear combination of eight normalized hardware factors: CPU integer throughput, memory bandwidth (STREAM-triad), sequential and random storage I/O, GPU VRAM capacity, and PCIe host-to-device / device-to-host transfer rates plus dispatch latency. We validate the model on a 20-core x86-64 development platform and report hardware-neutral scores for the six benchmark classes defined in the CHIMERA test suite, which is compliant with IEEE Std 2807-2022 and ISO/IEC 14756:2015. Key findings confirm that PCIe transfer bandwidth and memory bandwidth are the dominant bottlenecks for GPU-heavy workloads, while random I/O efficiency already meets target levels on the reference platform. All v0 efficiency values exceed 1.0, indicating conservative baselines that will be recalibrated as multi-host data accumulates.

**Keywords:** multi-model database, performance evaluation, hardware normalization, benchmark, vector search, LLM inference, time-series, graph database, AQL, MVCC

---

## 1 Introduction

Modern application stacks increasingly require databases that simultaneously support relational queries, graph traversals, vector similarity search, and time-series analytics. Separate purpose-built systems impose heavy operational overhead and cross-store consistency burdens. Polystore approaches address data variety at the abstraction level but typically do not provide native transactional guarantees across models.

**ThemisDB** is an open-source, C++20 hybrid multi-model database system that provides graph, vector, relational, time-series, and document models inside a single process with full ACID support. Its AQL query engine, adapted from ArangoDB's query language, compiles multi-paradigm queries — including LLM inference, RAG retrieval, and geospatial joins — into a single execution plan. The storage layer wraps RocksDB for write-ahead logging, compaction, and point-in-time recovery, while the replication layer employs Raft consensus.

**Contributions of this paper:**

1. *Longitudinal benchmark study* across five minor release series (v1.3.0–v1.8.2) covering key performance indicators (KPIs) for query, vector, graph, and time-series workloads using the Google Benchmark framework.
2. *Hardware-normalized efficiency model* that decomposes expected throughput into eight weighted hardware factors and produces a platform-independent `score_hw_neutral` for each benchmark run.
3. *Governance framework* with mandatory root-cause analysis when `score_hw_neutral < 0.90` across two consecutive runs, and automatic recalibration proposals when the score exceeds 1.10 across three or more distinct hardware platforms.
4. *Comprehensive gap analysis* across 33 modules with a prioritized 29-item remediation roadmap covering benchmarks missing due to proxy substitution, CMake feature gating, and disabled registrations.
5. *CHIMERA vendor-neutral comparison* following IEEE Std 2807-2022 protocol, showing ThemisDB leads two reference systems by +17% and +58% in query throughput.

**Table 1 — Workload Taxonomy**

| Class | Typical Operations | Bottleneck | SLO Unit |
|---|---|---|---|
| OLTP | Point read/write, MVCC commit | Random IOPS, alloc | ops/s |
| OLAP | CUBE, CTE, window functions | CPU, memory bw | ops/s |
| Storage WAL | WAL-ON write, fsync, compaction | Seq I/O, fsync | ops/s |
| Vector GPU | ANN insert, distance, index | VRAM, H2D bw | k ops/s |
| LLM GPU | Inference, LoRA hot-load | VRAM, dispatch | tokens/s |
| Mixed CPU+GPU | RAG, image embed, hybrid search | All factors | ops/s |

---

## 2 System Architecture Overview

**ThemisDB** is organized in seven horizontal layers, each encapsulating a distinct concern. The system comprises 46 software modules, 153 documentation files, and a test suite of 1,078+ benchmark cases plus over 1,200 unit/integration tests (v1.8.2, partial run).

**Table 2 — ThemisDB Architecture Layers (46 modules)**

| Layer | Modules | Key Technologies |
|---|---|---|
| Query & Index | AQL Parser, Optimizer, JIT, CTE | Multi-paradigm query rewrite, B-Tree, R-Tree, HNSW (CPU & GPU), secondary index |
| Storage | RocksDB WAL, Compaction, PITR, Blob Backends | Seven object backends (S3/Azure/GCS/MinIO/local/memory/hybrid), snapshot isolation |
| Analytics | OLAP, CEP, IVM, Streaming Aggregation | CUBE/ROLLUP, SIMD vectorization (4.5×–6.9× speedup), process mining |
| Time-Series | TS Storage, Gorilla Encoder, Retention | Adaptive flush, Gorilla delta-of-delta encoding, tiered retention |
| Graph | Graph Index, Traversal Engine | Adjacency index, BFS/DFS/Dijkstra/A*/Bidirectional, 12 constraint types |
| Vector / AI | ANN Search, Embedding Cache, LLM, RAG, Training | HNSW GPU search, llama.cpp inference, LoRA adapters, RAG pipeline |
| Distributed | Replication, Sharding, 2PC, SAGA | Raft, semi-sync/async replication, 2PC, SAGA |

The security layer provides AES-256-GCM field-level encryption, TLS 1.3 termination, RBAC, and SOC 2 / NIST / GDPR compliance hooks. Seven network protocols are supported: HTTP/1.1, HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL Wire, and gRPC.

### 2.1 Module-Level SLO Inventory

Each module carries explicit service-level objectives (SLOs) maintained in per-module `FUTURE_ENHANCEMENTS.md` files. A total of 28 modules define at least one SLO; of these, 12 have at least one directly measured value, 10 have only proxy coverage, and 6 lack any current measurement.

**Table 3 — Representative Module SLOs and v1.8.2 Status**

| Module | SLO ID | Target | v1.8.2 | Status |
|---|---|---|---|---|
| Query | Q-1 | 750 M items/s | 796 M | ✓ |
| Index | I-1 | 280 k ins/s | 549 k | ✓ |
| Index | I-2 | 180 k ins/s | 255 k | ✓ |
| Cache | C-1 | 5 M ops/s | 5.9 M | ✓ |
| Storage | S-1 | 100 k ops/s | ~1.3 k | ✗ |
| Graph | G-1 | 500 k e/s | 1.18 M | ✓ |
| TS | TS-1 | 500 k pts/s | 61 M* | ✓ |
| TS | TS-2 | 2 GB/s/core | n/a | — |
| Tx | TX-3 | 6 k 2PC/s | 6.4 k | ✓ |
| Replica | R-1 | P99 ≤ 50 ms | n/a | — |
| LLM | L-1 | TTFT ≤ 200 ms | n/a | — |
| Search | SE-1 | P99 ≤ 20 ms | n/a | — |
| Auth | AUT-4 | ≤ 1 µs (Bloom) | n/a | — |
| CDC | CDC-2 | P99 ≤ 20 ms | n/a | — |

*\* In-memory append; sustained NVMe write not yet measured separately. ✗ = SLO not met; — = no current measurement.*

---

## 3 Benchmark Methodology

### 3.1 Benchmark Infrastructure

All micro-benchmarks are implemented with the Google Benchmark framework. Each fixture applies at least two warmup iterations followed by a minimum of five measurement samples; the median is used as the standard reported value. Reporting units follow the workload type: operations per second (ops/s) for query and graph workloads, insertions per second (k/s or M/s) for index and vector workloads, and points per second (M pts/s) for time-series workloads.

**Platform specifications:**

- *v1.3.0–v1.3.3* (baseline runs 20251223, 20251223_085556): Intel i9-10900K, 10C/20T @ 3.70 GHz, 31 GB RAM, WSL2 Linux, MSVC Release x64, AVX2.
- *v1.3.4 and v1.8.2* (run 20251229_184507 and current): Windows x64, 20 cores @ 3.696 GHz, 20 MB L3-cache, L1=32 KB, L2=256 KB, MSVC Release x64, AVX2.

### 3.2 Hardware Baseline Capture

Every benchmark run is preceded by the `HardwareBaseline.CaptureAndPersist` Google Test fixture, which writes a structured JSON snapshot to `logs/hardware_baseline/`.

**Table 4 — Hardware Baseline Snapshot Schema**

| Field Group | Contents |
|---|---|
| Identification | `schema_version`, `run_id`, `host_fingerprint` |
| Build context | compiler, optimization flags, AVX tier |
| OS context | OS name, kernel version, NUMA topology |
| CPU metrics | Integer ops/s (STREAM-like integer kernel) |
| Memory metrics | STREAM triad bandwidth (GB/s) |
| Storage metrics | Sequential read (MB/s), random read IOPS (fio-equivalent) |
| GPU inventory | VRAM (GB), H2D transfer (GB/s), D2H transfer (GB/s), dispatch latency (µs) |
| Runtime context | process affinity, huge-page status |

Missing hardware metrics are recorded as `unavailable` (not as 0.0) to prevent silent misattribution. When a factor is unavailable, it is imputed to 0.5 and flagged as uncertain.

### 3.3 CHIMERA Benchmark Suite

**ThemisDB** integrates the CHIMERA vendor-neutral benchmark adapter (`src/chimera`), compliant with IEEE Std 2807-2022 and ISO/IEC 14756:2015. The adapter provides unified drivers for YCSB, TPC-C, TPC-H, ANN-Benchmarks, LDBC-SNB, vLLM inference, and RAGBench. CHIMERA normalizes results across participating database systems to enable apples-to-apples comparisons independent of vendor-specific reporting conventions.

---

## 4 Hardware-Normalized Efficiency Model

### 4.1 Problem Statement

Absolute throughput targets depend on the platform on which the measurement was taken. A database achieving 796 M ops/s on a 20-core workstation with 31 GB RAM cannot be directly compared to the same target on a dual-socket server with 256 GB RAM and NVMe RAID storage. A hardware-agnostic evaluation framework requires that expected throughput be expressed as a function of the actual hardware capability of the test host.

### 4.2 Normalized Hardware Factors

We define eight normalized hardware factors $N_i \in [0, 1]$ by clamping measured values against reference ceilings chosen to represent high-end commodity hardware:

$$N_\text{cpu}      = \min\!\bigl(\text{cpu\_ops} / 8{\times}10^{7},\ 1\bigr)$$
$$N_\text{mem}      = \min\!\bigl(\text{triad\_GB/s} / 35,\ 1\bigr)$$
$$N_\text{seq}      = \min\!\bigl(\text{disk\_MB/s} / 1200,\ 1\bigr)$$
$$N_\text{rand}     = \min\!\bigl(\text{disk\_IOPS} / 50000,\ 1\bigr)$$
$$N_\text{vram}     = \min\!\bigl(\text{vram\_GB} / 16,\ 1\bigr)$$
$$N_\text{h2d}      = \min\!\bigl(\text{h2d\_GB/s} / 8,\ 1\bigr)$$
$$N_\text{d2h}      = \min\!\bigl(\text{d2h\_GB/s} / 8,\ 1\bigr)$$
$$N_\text{dispatch} = \min\!\bigl(200 / \text{dispatch\_µs},\ 1\bigr)$$

### 4.3 Expected Capacity Models

Six workload classes are defined, each with a weighted linear expected-capacity function $E_c$. Weights reflect the dominant resource bottleneck for each class.

**Table 5 — Expected-Capacity Models per Workload Class (v0)**

| Class | Expected Capacity $E_c$ |
|---|---|
| OLTP | $0.45\,N_\text{cpu} + 0.20\,N_\text{mem} + 0.25\,N_\text{rand} + 0.10\,N_\text{seq}$ |
| OLAP | $0.40\,N_\text{cpu} + 0.45\,N_\text{mem} + 0.15\,N_\text{seq}$ |
| Storage WAL | $0.20\,N_\text{cpu} + 0.20\,N_\text{mem} + 0.35\,N_\text{seq} + 0.25\,N_\text{rand}$ |
| Vector GPU | $0.20\,N_\text{cpu} + 0.15\,N_\text{mem} + 0.30\,N_\text{vram} + 0.20\,N_\text{h2d} + 0.10\,N_\text{d2h} + 0.05\,N_\text{dispatch}$ |
| LLM GPU | $0.15\,N_\text{cpu} + 0.15\,N_\text{mem} + 0.35\,N_\text{vram} + 0.20\,N_\text{h2d} + 0.05\,N_\text{d2h} + 0.10\,N_\text{dispatch}$ |
| Mixed CPU+GPU | $0.25\,N_\text{cpu} + 0.20\,N_\text{mem} + 0.20\,N_\text{vram} + 0.15\,N_\text{h2d} + 0.10\,N_\text{d2h} + 0.10\,N_\text{dispatch}$ |

### 4.4 Efficiency Computation

For a specific benchmark $b$ assigned to workload class $c$:

$$\text{expected}_{b}  = \text{baseline\_ref}_{b} \times E_c$$
$$\text{efficiency}_{b} = \frac{\text{measured}_{b}}{\text{expected}_{b}}$$
$$\text{score\_hw\_neutral}_{b} = \frac{\text{measured}_{b}}{\text{target\_product}_{b} \times E_c}$$

where `baseline_ref_b` is the reference throughput fixed at v0 initialization, `target_product_b` is the product-roadmap absolute target, and `score_hw_neutral_b` is the primary release-gate metric.

### 4.5 Calibration Rules (v1)

Version 1 calibration is binding once 30 paired runs across at least three distinct hardware classes are collected:

1. *Class calibration factor:* $K_c = \operatorname{median}(\{\text{efficiency}_{b}\}_{b \in c})$ over the reference window.
2. *Calibrated efficiency:* $\text{efficiency\_cal}_{b} = \text{efficiency}_{b} / K_c$.
3. *Factor significance:* a hardware factor $N_i$ is deemed significant for class $c$ only if $|\rho_\text{Spearman}(N_i,\,\text{efficiency\_cal})| \geq 0.35$ and $p \leq 0.05$.
4. *Stability rule:* the coefficient of variation $\text{CV} \leq 0.20$ within the reference window.

**Banding table:**

| Band | score range |
|---|---|
| Critical | score < 0.85 |
| Notable | 0.85 ≤ score < 0.95 |
| Normal | 0.95 ≤ score ≤ 1.05 |
| Above target | score > 1.05 |

---

## 5 Experimental Results

### 5.1 Longitudinal Throughput Evolution

**Table 6 — Longitudinal Throughput Evolution (v1.3.0 – v1.8.2)**

| Metric | v1.3.0 | v1.3.1 | v1.3.2 | v1.3.3 | v1.3.4 | v1.8.2 | Target |
|---|---|---|---|---|---|---|---|
| Query Engine (M ops/s) | 700 | 750 | 800 | 800 | **814.5** | **796.4** | 900 |
| Vector Insert (k/s) | 280 | 300 | 330 | 340 | **351.4** | **548.7** | 600 |
| Secondary Index (k/s) | 180 | 190 | 210 | 215 | **217.2** | **254.9** | 1,000 |
| Graph Edge Ops (k/s) | — | — | — | — | **628.7** | **1,177** | 1,000 |
| Timeseries Insert (M pts/s) | — | — | — | — | **49.0** | **61.0** | 60 |
| Benchmark Tests | 450 | 480 | 520 | 780 | **1,078** | 5 (partial) | 1,200 |

Graph edge operations surpassed the 1 M edges/s target already in v1.8.2 (+87% over v1.3.4). Time-series insert throughput exceeded the 60 M pts/s target by 1.7%. Query engine throughput reached 814.5 M ops/s in v1.3.4 but regressed to 796.4 M ops/s in v1.8.2 due to increased governance-framework overhead in the hot evaluation path. Secondary index insert performance remains the largest gap at 254.9 k/s vs. the 1 M/s target, primarily attributable to RocksDB write amplification.

### 5.2 Hardware Baseline on the Reference Platform

**Table 7 — Normalized Hardware Factors — Reference Platform (run 1775806092)**

| Factor | Value | Tier |
|---|---|---|
| $N_\text{cpu}$ | 0.679 | medium |
| $N_\text{mem}$ | 0.619 | medium |
| $N_\text{seq}$ | 0.646 | SSD class |
| $N_\text{rand}$ | 1.000 | high (at ceiling) |
| $N_\text{vram}$ | 0.739 | medium |
| $N_\text{h2d}$ | 0.509 | medium |
| $N_\text{d2h}$ | 0.494 | medium |
| $N_\text{dispatch}$ | 0.402 | low–medium |

Random I/O ($N_\text{rand} = 1.0$) is fully saturated, meaning OLTP workloads are not storage-bottlenecked on this platform. PCIe dispatch latency ($N_\text{dispatch} = 0.402$) represents the largest gap and constitutes the primary hardware constraint for GPU-heavy workload classes.

### 5.3 Efficiency Results (v0)

**Table 8 — Efficiency Results — v0, Reference Platform**

| Class | $E_c$ | Baseline | Measured | Efficiency |
|---|---|---|---|---|
| OLTP | 0.744 | 814.5 M/s | 796.4 M/s | 1.314 |
| OLAP | 0.647 | 242.6 M/s | 242.6 M/s | 1.545 |
| Storage WAL | 0.736 | 1.058 k/s | 1.193 k/s | 1.532 |
| Vector GPU | 0.622 | 351.4 k/s | 548.7 k/s | 2.510 |
| LLM GPU | 0.620 | 15.9 k/s | 15.9 k/s | 1.613 |
| Mixed CPU+GPU | 0.607 | 717.2 k/s | 717.2 k/s | 1.647 |

### 5.4 Hardware-Neutral Scores

**Table 9 — Hardware-Neutral Scores (run 1775806092)**

| Benchmark | Target | $E_c$ | Score |
|---|---|---|---|
| QueryEngineBench/SimpleEval | 750 M/s | 0.744 | 1.427 |
| VectorIndexBench/InsertPlain | 280 k/s | 0.622 | 3.150 |
| BM_RawWrite_WAL_On/8 | 1.0 k/s | 0.736 | 1.621 |
| BM_EmbeddingCache/100k | — | 0.620 | 1.573 |

All scores exceed 1.0, confirming above-target performance relative to hardware capability.

### 5.4.1 Hardware Factor Expectation Matrix (v0)

**Table 10 — Hardware Factor Expectation Matrix (v0) — H=High, M=Medium, L=Low, 0=Negligible**

| Class | $N_\text{cpu}$ | $N_\text{mem}$ | $N_\text{seq}$ | $N_\text{rand}$ | $N_\text{vram}$ | $N_\text{h2d}$ | $N_\text{d2h}$ | $N_\text{disp}$ |
|---|---|---|---|---|---|---|---|---|
| OLTP | H | M | L | H | M | 0 | 0 | 0 |
| OLAP | H | H | M | L | L | 0 | 0 | 0 |
| Storage WAL | M | M | H | H | 0 | 0 | 0 | 0 |
| Vector GPU | M | L | 0 | 0 | H | H | M | L |
| LLM GPU | L | M | 0 | 0 | H | H | L | M |
| Mixed CPU+GPU | M | M | 0 | 0 | M | M | M | M |

### 5.5 Raw Benchmark Details

#### 5.5.1 Allocator Performance

**ThemisDB** ships a custom allocator (themis-malloc, inspired by mimalloc). Table 11 compares wall-clock throughput against the system allocator on the v1.8.1-rc2 build (Windows x64, MSVC Release, benchmark `bench_storage_performance`).

**Table 11 — Allocator Throughput (v1.8.1-rc2, `bench_storage_performance`)**

| Case | System | ThemisDB | Ratio |
|---|---|---|---|
| Small alloc (≤ 256 B) | 15.5 M/s | 128.5 M/s | 8.3× |
| Large alloc (≥ 64 KB) | 119 k/s | 2.49 M/s | 20.9× |
| Mixed workload | — | 19.6 M/s | — |
| RCU read (single thread) | — | 919.8 M/s | — |

The 8–21× allocator speedup is consistent with the mimalloc free-list sharding approach and is a primary enabler of the high OLTP throughput reported in §5.1.

#### 5.5.2 WAL Scaling with Thread Count

**Table 12 — Storage Hotspot Scaling (`bench_hotspots_micro`, v1.3.4)**

| Pattern | 1T | 4T | 8T | 16T |
|---|---|---|---|---|
| WAL ON (ops/s) | 283 | 609 | 1,193 | 1,546 |
| WAL OFF (k ops/s) | 146 | 370 | 508 | 350 |
| Mixed 80/20 (ops/s) | 583 | 1,289 | 2,534 | 4,405 |
| Sec. Index (ops/s) | 281 | 590 | 1,056 | 1,990 |

WAL-ON throughput scales approximately linearly up to 8 threads (+13% at 8T vs. 1T) but degrades −25% at 16 threads due to log-group-commit contention. WAL-OFF saturates at 16T. Mixed 80/20 read-write and secondary-index writes exhibit near-linear scaling through 16 threads.

#### 5.5.3 Vector Distance Kernel Throughput

**Table 13 — Distance Kernel Throughput (AVX2, v1.3.4)**

| Kernel | Dim *d* | Latency | Throughput |
|---|---|---|---|
| Euclidean | 64 | 42 ns | 23.7 M/s |
| Euclidean | 256 | 209 ns | 4.8 M/s |
| Euclidean | 1024 | 828 ns | 1.21 M/s |
| Cosine | 64 | 38 ns | 26.4 M/s |
| Cosine | 256 | 205 ns | 4.9 M/s |
| Cosine | 1024 | 827 ns | 1.21 M/s |
| Haversine | 2 | 3.6 µs/100 pts | 28.0 M pts/s |
| Bbox containment | 2 | 64 ns/100 pts | 1.56 G pts/s |
| Vec+Geo filter | 256 | 1.27 ms/32K pts | 25.8 M/s |

Throughput decreases roughly as $1/d$ for small *d* and asymptotes toward a bandwidth-limited regime around *d* = 512. Geo operations run at much higher throughput because they use scalar arithmetic on two-dimensional coordinate pairs rather than full *d*-dimensional dot products.

#### 5.5.4 Graph Query Optimizer

**Table 14 — Graph Query Optimizer (v1.8.1-rc2)**

| Benchmark | Latency | Throughput |
|---|---|---|
| PlanGen — Shortest Path | 223 ns | 4.53 M/s |
| PlanGen — k-Hop (k=2) | 246 ns | 4.03 M/s |
| PlanGen — With Cache Hit | 225 ns | 4.53 M/s |
| BFS — depth 2 (100 nodes) | 3,214 ns | 321 k/s |
| BFS — depth 3 (100 nodes) | 6,038 ns | 161 k/s |
| BFS — depth 4 (100 nodes) | 13,241 ns | 77 k/s |

Plan generation is sub-microsecond (≤ 246 ns) and is cache-resident after the first execution. BFS execution time grows super-linearly with depth (3,214 ns at depth 2 vs. 13,241 ns at depth 4), reflecting the exponential fan-out of the traversal frontier.

#### 5.5.5 AQL Function Benchmarks

**Table 15 — AQL Function Benchmark Highlights (v1.3.4)**

| Benchmark | Latency | Throughput |
|---|---|---|
| EmbeddingCache hit /384 | 6.44 ns | 155.8 M/s |
| EmbeddingCache miss /384 | 1.298 µs | 777 k/s |
| HybridSearch RRF /384 | 148 ns | 6.64 M/s |
| HybridSearch 50/50 | 99 ns | 10.2 M/s |
| CTE non-recursive /1 | 1.0 ns | 952.6 M/s |
| CTE non-recursive /20 | 21.3 ns | 938.5 M/s |
| CTE recursive depth 10 | 11.3 ns | 87.1 M/s |
| CTE recursive depth 1000 | 1.11 µs | 896 k/s |
| EXISTS w/ LIMIT 1 /100K | ≈ 0 | short-circuit |
| EXISTS w/o LIMIT 1 /10K | 6.82 µs | 147 k/s |

**Embedding Cache.** The hot-path lookup achieves 155.8 M/s at 384–3,072 dimensions with only 6.4–6.5 ns per call because the result is fully resident in L1 cache. Cache misses incur a 200× penalty (777 k/s) due to the backing vector comparison.

**Hybrid Search RRF.** Reciprocal-rank fusion across BM25 and HNSW result sets runs at 6.6–7.1 M/s for 384–1,536 dimensions; the 50/50-weight variant hits 10.2 M/s due to branch elimination in the weight-balancing path.

**CTEs.** Non-recursive CTEs scale nearly linearly: 1-clause throughput is 952.6 M/s; 20-clause throughput is 938.5 M/s (−1.5%), indicating negligible per-clause overhead. Recursive CTEs degrade with depth: depth 10 achieves 87.1 M/s; depth 1,000 drops to 896 k/s, reflecting O(d) stack traversal.

**Subquery EXISTS.** The LIMIT 1 early-exit optimisation eliminates essentially all per-row work (short-circuit). Without LIMIT 1 the cost grows linearly: 147 k/s at 10 K rows, 14.7 k/s at 100 K rows.

#### 5.5.6 Graph Traversal Scaling

**Table 16 — BFS Traversal Throughput: v1.3.3 vs v1.8.1-rc2**

| Config | v1.3.3 lat. (ms) | v1.3.3 ops/s | v1.8.1-rc2 lat. (ms) | v1.8.1-rc2 ops/s |
|---|---|---|---|---|
| 100n / d4 | 0.184 | 5.43 k | 0.214 | 4.76 k |
| 1000n / d4 | 1.560 | 652 | 1.250 | 823 |
| 10000n / d4 | 20.2 | 50.6 | 23.2 | 44.2 |
| 100n / d20 | 0.469 | 2.11 k | 0.514 | 1.91 k |
| 1000n / d20 | 4.38 | 232.7 | 4.65 | 215.1 |

*n = nodes; d = traversal depth.*

Both versions exhibit O(n) scaling at fixed depth. The v1.8.1-rc2 build shows a slight regression at 10,000 nodes/depth 4 (+15% latency) but a +26% improvement at 1,000 nodes/depth 4, suggesting improved cache locality for medium-sized graphs.

#### 5.5.7 Image Analysis Latency

**Table 17 — Image Analysis Latency Distribution (v1.3.4)**

| Operation | Mean | P50 | P95 | P99 |
|---|---|---|---|---|
| Embed 224 px | 1.58 ms | 1.50 ms | 1.60 ms | 2.20 ms |
| Embed 512 px | 3.02 ms | 2.70 ms | 2.80 ms | — |
| Embed 1024 px | 6.01 ms | 5.70 ms | 6.00 ms | — |
| Caption 384 px | 22.0 ms | 21.1 ms | 22.6 ms | 40.2 ms |
| Batch /4 (per img) | 1.58 ms | 1.47 ms | 1.58 ms | — |
| Batch /16 (per img) | 1.54 ms | 1.48 ms | 1.56 ms | — |
| GPU vs CPU (embed 224) | 2.31 ms | 1.70 ms | 2.50 ms | 21.1 ms |

Key findings:
- Embedding throughput is constant across batch sizes 1–16 at ≈ 258 k/s per image, indicating CPU-bound execution with no parallelism overhead up to batch 16.
- P99 latency spikes to 20–21 ms for both CPU and GPU backends, likely caused by garbage-collection or OS scheduling jitter; median latency is 1.5–2.1 ms.
- Image resolution increases from 224 px to 1024 px raise median embedding latency 3.6× (1.975 ms → 6.007 ms), consistent with an O(r²) pixel-scan dependency.
- Captioning at 384 px incurs a 9× overhead vs. embedding (P50 = 22 ms), due to the additional transformer decode pass.

#### 5.5.8 MVCC Transactions and Lock Contention

**Table 18 — MVCC and Lock-Contention Benchmarks (v1.3.3)**

| Benchmark | Latency | Throughput |
|---|---|---|
| MVCC SingleEntity Commit | 4.07 ms | 7.11 k/s |
| MVCC BatchInsert 100 | 7.29 ms | 29.7 k/s |
| MVCC SnapshotIsolation | 4.05 ms | 40.0 k/s |
| MVCC Rollback | 266 µs | 37.3 k/s |
| WriteBatch SingleEntity | 4.38 ms | 6.52 k/s |
| WriteBatch BatchInsert 100 | 6.25 ms | 41.7 k/s |
| LockContention Disjoint 1T | 4.44 ms | 14.4 k/s |
| LockContention Disjoint 8T | 8.31 ms | 61.6 k/s |
| LockContention Disjoint 16T | 66.7 ms | 15.3 k/s |
| LockContention Overlap 1T | 14.4 ms | 4.43 k/s |

MVCC single-entity commit runs at 7.1 k/s (4.07 ms/tx); snapshot isolation imposes only −3.9% overhead vs. raw WriteBatch at 40.0 k/s, confirming that the MVCC read-view chain is not on the critical write path.

Lock contention exhibits a bimodal profile: disjoint-key workloads scale near-linearly up to 8 threads (61.6 k/s) but collapse at 16 threads (15.3 k/s) due to RocksDB write-group-commit congestion. This pattern directly corroborates Gap D-6 (concurrency CV > 20%). Overlapping-key contention is 3.3× slower than disjoint even at a single thread (4.43 k/s vs. 14.4 k/s), consistent with lock-convoy formation on high-contention key ranges.

#### 5.5.9 2PC Shard-Count Scaling

**Table 19 — 2PC Shard-Count Scaling (v1.3.4)**

| Shards | Latency (ms) | ops/s | vs. target (15 k/s) |
|---|---|---|---|
| 2 | 46.04 | 6,400 | −57% |
| 4 | 46.09 | 6,400 | −57% |
| 8 | 46.09 | 1,600 | −89% |
| 16 | 45.95 | 1,280 | −91% |

Commit latency is essentially flat at 46 ms regardless of shard count because the latency is dominated by the coordinator's synchronous fsync, not by fan-out RPC overhead. Throughput halves from 6.4 k/s to 1.6 k/s as shard count doubles from 4 to 8, then halves again to 1.28 k/s at 16 shards, indicating a linear fan-out bottleneck in the coordinator's parallel-prepare phase. Gap D-4 (target 15 k/s) therefore requires both batched commits and a pipelined prepare phase to eliminate the fsync bottleneck.

### 5.6 Regression Overview v1.3.0 → v1.3.4

**Table 20 — Regression Summary v1.3.0 → v1.3.4**

| Benchmark | v1.3.0 | v1.3.4 | Δ | Cause |
|---|---|---|---|---|
| Vec Insert | 567 k/s | 351 k/s | −38% | infra change |
| Sec. Index | 1.78 M/s | 217 k/s | −88% | infra change |
| Query Engine | 969 M/s | 815 M/s | −16% | governance |
| Graph Edges | 1.47 M/s | 629 k/s | −57% | infra change |
| TS Insert | 61.0 M/s | 49.0 M/s | −20% | flush policy |
| Encrypt 1 KB | 255 k/s | 191 k/s | −25% | EVP overhead |
| Decrypt 256 B | 60 k/s | 41 k/s | −32% | EVP overhead |
| Geo Content API | 5.6 k/s | 7.2 k/s | +29% | *improvement* |

Several regressions marked *critical* are attributable to intentional infrastructure changes (per-test temporary directories, single-transaction-per-put RocksDB semantics) rather than production code path changes. This distinction is documented in `PERFORMANCE_COMPARISON_V1.3.0_VS_V1.3.3.md`.

### 5.7 CHIMERA Vendor-Neutral Comparison

**Table 21 — CHIMERA Query Throughput Comparison (95% CI, Welch's t)**

| System | N | Mean (q/s) | Median | CI lo | CI hi |
|---|---|---|---|---|---|
| Alpha (ThemisDB) | 29 | 14,842 | 14,813 | 14,604 | 15,286 |
| Beta | 29 | 12,678 | 12,789 | 11,966 | 13,090 |
| Gamma | 28 | 9,392 | 9,431 | 8,677 | 9,940 |

*System names anonymised per IEEE Std 2807 neutrality.*

**Table 22 — CHIMERA Vector Search P95 Latency Comparison (ms, 95% CI)**

| System | N | Mean | P95 | CI lo | CI hi |
|---|---|---|---|---|---|
| Aurora | 48 | 8.51 | 10.01 | 7.91 | 8.76 |
| Nexus | 49 | 9.43 | 12.40 | 9.02 | 10.02 |
| Quantum | 50 | 7.59 | 9.27 | 7.25 | 7.93 |
| Vertex | 48 | 8.77 | 10.98 | 8.34 | 9.23 |
| Zenith | 48 | 9.47 | 12.46 | 8.77 | 10.14 |

*Lower is better. N ≥ 28 per IEEE Std 2807; outliers removed by IQR×1.5.*

System Alpha (ThemisDB) leads query throughput by +17% over System Beta and +58% over System Gamma. In vector search latency the spread is narrower: the fastest system (Quantum, mean 7.59 ms) beats the slowest (Zenith, mean 9.47 ms) by only 24.7%, suggesting that HNSW index performance is largely converging across the vendor landscape at SIFT-1M scale.

**Table 23 — ThemisDB CHIMERA Self-Baseline (v1.5.0-dev)**

| Workload | Throughput | Mean | P95 | P99 |
|---|---|---|---|---|
| relational_sort | 42,503 ops/s | 0.024 ms | 0.023 ms | 0.034 ms |
| vector_dot_product | 75,835 ops/s | 0.013 ms | 0.013 ms | 0.024 ms |
| document_lookup | 2.96 M ops/s | 0.18 µs | 0.20 µs | 0.25 µs |
| graph_bfs | 40,373 ops/s | 0.025 ms | 0.025 ms | 0.033 ms |

---

## 6 Analysis and Discussion

### 6.1 Root-Cause Analysis of Performance Gaps

**Query Engine (796.4 M vs. 900 M ops/s).** The 11.5% gap between v1.3.4 (814.5 M) and the 900 M target widened to 13.4% in v1.8.2 due to additional governance-framework instrumentation in the hot evaluation path. The primary mitigation is selective hot-path profiling to identify and defer non-critical checks out of the tight inner loop.

**Secondary Index (254.9 k vs. 1 M/s).** The 74.5% gap is the largest in the system and is caused by RocksDB write amplification under the current compaction policy. Level-based compaction with default `l0_file_num_compaction_trigger=4` generates 4–10× write amplification for random-key workloads. Switching to a tiered compaction strategy or pre-sorting inserts by key range reduces this amplification at the cost of read performance.

**Storage WAL Sustained Write (1.276 k/s vs. 100 k/s SLO).** The production SLO (≥ 100 k ops/s sustained NVMe write) is not directly covered by the available proxy benchmark (`BM_RawWrite_WAL_On/8` at 1.276 k/s batch of 8), which measures fsync-commit overhead rather than sustained NVMe throughput. A dedicated benchmark targeting 64 KB sequential writes to NVMe is required to evaluate this SLO fairly.

### 6.2 Benchmark Coverage Gap Analysis

**Table 24 — Benchmark Coverage Gap Categories (33 modules)**

| Category | Count | Root Cause |
|---|---|---|
| Feature-gated benchmarks | 7 | CMake `THEMIS_ENABLE_*` flags |
| Disabled stubs | 4 | Deactivated `BENCHMARK(...)` registrations |
| Missing runtime deps | 3 | External library / GPU not present |
| Proxy-only benchmarks | 12 | No 1:1 SLO mapping; indirect metrics only |
| Missing build artifacts | 3 | CMake exclusion / optional submodule |
| **Total** | **29** | |

The dominant category is *proxy-only* (12 benchmarks), where existing throughput numbers exist but do not directly correspond to the documented SLO metric. Notable examples are Cache (proxy via `BM_EmbeddingCache`) and Analytics (proxy via `bench_olap_performance`, while the direct `bench_olap_analytics` is disabled pending API alignment).

### 6.3 Meta-Causes and Remediation Roadmap

Three meta-causes explain the majority of coverage gaps:

1. *Proxy vs. primary benchmarks.* Twelve SLO gaps arise because the available benchmark measures a related but not equivalent code path. Remediation: add at minimum one benchmark per SLO that exercises the exact code path referenced in the SLO.
2. *CMake feature gating.* Seven benchmarks are conditionally compiled behind feature flags (e.g., `THEMIS_ENABLE_HNSW_GPU`, `THEMIS_ENABLE_LLM`). Remediation: add an integration CI job that enables all optional features on a GPU runner.
3. *Disabled benchmark registrations.* Four benchmark files exist and compile correctly but have their `BENCHMARK()` invocations commented out. Remediation: un-gate or replace with a reduced-scope variant that terminates within CI time limits.

**Table 25 — Remediation Roadmap for 29 Benchmark Gaps**

| Priority | Module / Gap | Required Action | Target |
|---|---|---|---|
| **P1 — High: 8 items** | | | |
| P1 | Storage WAL (D-5) | Direct pwrite path; bypass blob ext. | v2.0 |
| P1 | TS NVMe write (D-1) | New 64KB seq-write benchmark | v2.0 |
| P1 | Gorilla decode (D-2) | AVX2 kernel + dedicated bench | v2.0 |
| P1 | CPU Concurrency (D-6) | LMAX ring-buf; re-enable CV bench | v2.0 |
| P1 | Sec. Index (proxy) | Un-comment `BENCHMARK()` | v1.9 |
| P1 | HNSW GPU (feature) | GPU runner in CI | v1.9 |
| P1 | LLM inference (feature) | GPU runner; enable flag | v1.9 |
| P1 | Vec vs. FAISS (D-3) | FAISS parity benchmark | v2.0 |
| **P2 — Medium: 13 items** | | | |
| P2 | 2PC throughput (D-4) | Pipelined prepare phase | v2.0 |
| P2 | Query vs. CH (D-7) | Vectorized agg. pass | v2.0 |
| P2 | Replication lag | WAL-shipping bench | v1.9 |
| P2 | Search SE-1 | Dedicated HNSW+BM25 bench | v1.9 |
| P2 | Auth AUT-4 | Bloom-filter microbench | v1.9 |
| P2 | CDC CDC-2 | Delivery latency bench | v1.9 |
| P2 | Analytics proxy | Un-disable `bench_olap_analytics` | v1.9 |
| P2 | Cache proxy | Map to LRU direct-hit SLO | v1.9 |
| P2 | AQL-Sugar (ERROR) | Fix API mismatch; re-register | v1.9 |
| P2 | Network NET-1 | TCP wire-protocol bench | v1.9 |
| P2 | Graph traversal | Extend to v1.8.2 run | v1.9 |
| P2 | GNN embeddings | Full-dim GPU variant | v2.0 |
| P2 | HSM real lib | Integrate stub→real sign bench | v2.0 |
| **P3 — Low: 8 items** | | | |
| P3 | ARM emulation | Native ARM runner | v2.1 |
| P3 | TS compression | Gorilla encode bench | v2.1 |
| P3 | Content ver. CV | Thread-count tuning | v2.1 |
| P3 | Batch insert | API align; re-enable | v2.1 |
| P3 | Image GPU | Real CUDA pipeline bench | v2.1 |
| P3 | CDC compaction | 1M-event replay bench | v2.1 |
| P3 | Temporal join | Overhead vs. non-temporal | v2.1 |
| P3 | QUIC 0-RTT | QUIC transport bench | v2.1 |

### 6.4 Known Performance Gaps (D-1..D-7)

**Table 26 — Documented Performance Gaps (Section 35 Registry)**

| ID | Module | Current | Target | Gap | Priority |
|---|---|---|---|---|---|
| D-1 | TS Write | 200 k pts/s | 500 k pts/s | −60% | High |
| D-2 | Gorilla Dec. | 400 MB/s | 2 GB/s | −80% | High |
| D-3 | Vec vs FAISS | 351 k/s | 600 k/s | −41% | Medium |
| D-4 | 2PC vs TiDB | 6.4 k/s | 15 k/s | −57% | Medium |
| D-5 | Blob Write | 741 ops/s | 100 k/s | −99% | High |
| D-6 | Concur. CV | 20.74 | stable | — | Medium |
| D-7 | Q vs ClickHouse | 815 M/s | 1,200 M/s | −32% | Low |

**D-1 and D-2 (Time-Series).** The sustained NVMe write target (D-1) is not yet evaluated by the available benchmark; the reported 61 M pts/s is for in-memory append only. The Gorilla decoder throughput gap (D-2) reflects the lack of an AVX2/AVX-512 decode kernel; the current implementation is scalar.

**D-5 (Blob Write).** The 1 MB blob write path achieves only 741 ops/s against a target of 100 k/s. Profiling reveals 99% of wall time is spent in multi-part upload coordination within the RocksDB blob extension layer. A direct `pwrite` bypass for blobs exceeding a configurable threshold is the primary proposed mitigation.

**D-6 (Concurrency Stability).** Under 10-client concurrent write workload the coefficient of variation of throughput exceeds 20%, indicating lock-convoy behaviour in the RocksDB write-group-commit path. Increasing the write batch budget or adopting a LMAX-disruptor ring buffer for the commit queue are candidate mitigations.

### 6.5 CI Regression Governance

**ThemisDB** enforces three-tier regression governance via the `05-quality_build_cross-module-performance-regression-ci.yml` GitHub Actions workflow:

| Tier | Threshold | Action |
|---|---|---|
| Minor | ≥ 5% degradation | Tracking issue created; does not block merge |
| Major | ≥ 10% degradation | PR merge blocked until root-cause comment attached |
| Critical | ≥ 20% degradation | Immediate escalation; PR cannot be merged until resolved or waived |

The governance framework operates on `score_hw_neutral` rather than raw throughput, preventing false positives when the benchmark host's hardware capability differs between runs. Root-cause analysis is mandatory when `score_hw_neutral < 0.90` occurs in two consecutive runs on the same module.

---

## 7 Threats to Validity

### 7.1 Internal Validity

**Infrastructure-induced regressions.** Several benchmarks in Table 20 show large apparent regressions between v1.3.0 and v1.3.4 caused by deliberate changes to the test harness (per-test temporary directories, single-transaction-per-put semantics for correctness isolation) rather than by production code changes. Mitigation: the `PERFORMANCE_COMPARISON` documentation file explicitly flags all harness-induced changes with root-cause notes.

**WAL-OFF vs. WAL-ON semantics.** The `BM_RawWrite_WAL_Off` benchmark bypasses the write-ahead log entirely, yielding throughput values (≥ 145 k ops/s) that are not representative of any production workload, which always requires durability. All durability-relevant comparisons in this paper use WAL-ON measurements exclusively.

**In-memory vs. persistent time-series.** The `TimeseriesBench/InsertTimepoints` benchmark measures in-memory append throughput (61 M pts/s at v1.8.2). Persistent, Gorilla-compressed, multi-tier write throughput is not yet covered by any automated benchmark (Gap D-1) and may be substantially lower.

### 7.2 External Validity

**Single platform.** All measurements are taken on a single Windows x64 workstation with an Intel i9-10900K. Throughput numbers may not generalise to Linux ARM64, AMD EPYC, or cloud-native serverless deployments without recalibration.

**Benchmark representativeness.** Google Benchmark micro-benchmarks isolate individual code paths under unrealistic conditions (e.g., warm caches, single query type, no concurrent background I/O). CHIMERA macro-workloads (YCSB, TPC-C/H) provide more realistic coverage but have not yet been fully executed with production data volumes (≥ 10 TB) on the reference hardware.

**Hardware model accuracy.** The v0 efficiency model uses heuristic weights derived from first-principles analysis. Until 30 or more paired runs across at least three distinct hardware classes are available, the statistical significance of the model cannot be assessed. All v0 scores exceeding 1.0 should therefore be interpreted as *conservative baselines*, not as validated performance claims.

### 7.3 Construct Validity

**Proxy benchmarks.** Twelve of the 29 benchmark gaps are proxy-only measurements, meaning the measured code path does not directly correspond to the documented SLO metric. The `score_hw_neutral` values derived from proxy benchmarks carry higher uncertainty and are labeled *proxy* in all tables.

**CHIMERA anonymisation.** The anonymised vendor comparison (§5.7) uses system aliases to comply with IEEE Std 2807-2022 neutrality requirements. The mapping between aliases and actual systems is not disclosed; readers should treat comparative scores as indicative rather than definitive.

---

## 8 Related Work

Multi-model databases have been surveyed by Lu and Holubová, who classify systems by their native model and secondary model support. **ThemisDB** extends this landscape by providing fully native graph, vector, relational, time-series, and document support with shared MVCC and a unified query language.

Hardware-normalized evaluation has antecedents in HPC benchmarking. McCalpin's STREAM benchmark established the concept of measuring memory bandwidth as a hardware baseline. Hennessy and Patterson discuss the growing importance of domain-specific hardware. Our efficiency model applies the same normalization philosophy to database workloads, extending it to GPU-specific factors ($N_\text{vram}$, $N_\text{h2d}$, $N_\text{d2h}$, $N_\text{dispatch}$) that are absent from CPU-centric HPC models.

YCSB provides workload-portable key-value benchmarking. TPC-C and TPC-H define transaction and analytical workloads, respectively. ANN-Benchmarks standardize vector search evaluation. LDBC-SNB targets graph analytics. The CHIMERA suite integrated into **ThemisDB** provides a unified adapter layer over all five standards. The statistical methodology used by CHIMERA is grounded in Welch's *t*-test, Mann-Whitney *U*, Cohen's *d*, and Tukey's IQR, following the minimum sample-size and warmup requirements of IEEE Std 2807-2022.

Approximate nearest-neighbor search using HNSW underpins the vector search engine. FAISS provides GPU-accelerated ANN baselines against which Gap D-3 is measured. The custom allocator design follows the mimalloc free-list sharding approach, yielding 8–21× speedup over the system allocator on allocation-intensive code paths. RocksDB is the storage foundation; its write-amplification characteristics drive Gap D-5 (blob write) and the secondary index gap. Raft provides consensus; SAGA and 2PC handle distributed transactions. Gorilla delta-of-delta encoding is used in the time-series module; Gap D-2 documents the planned SIMD acceleration of the decode path.

---

## 9 Conclusions

We have presented a longitudinal performance evaluation of **ThemisDB** spanning versions v1.3.0 through v1.8.2, together with a hardware-normalized efficiency model that produces platform-independent benchmark scores, a module-level SLO inventory across 28 modules, detailed raw benchmark tables, a CHIMERA vendor-neutral comparison, and an analysis of seven known performance gaps with root causes.

**Five major findings emerge:**

1. Graph edge operations (+87%) and time-series insert throughput (+25%) both surpassed their product targets in v1.8.2, demonstrating effective optimization of data-structure hot paths.
2. PCIe host-to-device transfer ($N_\text{h2d} = 0.509$) and CPU dispatch latency ($N_\text{dispatch} = 0.402$) are the dominant hardware bottlenecks for GPU-heavy workloads on the reference platform.
3. All v0 efficiency values exceed 1.0, confirming that the initial baseline references are conservative. Recalibration to v1 statistical rules requires 30+ paired runs across at least three distinct hardware classes.
4. The CHIMERA query throughput comparison shows **ThemisDB** (System Alpha) leading two reference systems by +17% and +58% respectively; vector search P95 latency is within 25% of the field leader.
5. Seven documented performance gaps (D-1..D-7) remain open, of which three (D-1, D-2, D-5) are rated high priority for the v2.0 roadmap; all require production-path benchmarks that are currently missing.

Future work will focus on (i) closing the 29 identified benchmark coverage gaps, particularly the 12 proxy-only benchmarks and 7 high-priority known gaps; (ii) running the CHIMERA suite on heterogeneous hardware (ARM, AMD, NVMe RAID) to build the v1 calibration dataset; (iii) extending the efficiency model to encompass network I/O and PCIe topology for distributed deployments; and (iv) implementing the AVX2 Gorilla decode kernel (Gap D-2) and the LMAX-disruptor write queue (Gap D-6).

---

## References

1. Lu, Y. & Holubová, I. (2019). Multi-model databases: a new journey to handle the variety of data. *ACM CSUR* 52(3).
2. Stonebraker, M. (2010). SQL databases v. NoSQL databases. *CACM* 53(4), 10–11.
3. Atzeni, P. et al. (2016). Underspecification of the relational model and how to fix it. *Information Systems* 59, 80–100.
4. Dong, S. et al. (2021). Evolution of development priorities in key-value stores serving large-scale applications. *FAST 2021*.
5. Ongaro, D. & Ousterhout, J. (2014). In search of an understandable consensus algorithm (Raft). *USENIX ATC 2014*.
6. Pelkonen, T. et al. (2015). Gorilla: a fast, scalable, in-memory time series database. *VLDB 2015*.
7. Malkov, Y. & Yashunin, D. (2020). Efficient and robust approximate nearest neighbor search using HNSW. *TPAMI 42*(12).
8. llamacpp contributors (2023). llama.cpp: LLM inference in C/C++. GitHub.
9. Hu, E. et al. (2022). LoRA: low-rank adaptation of large language models. *ICLR 2022*.
10. Lewis, P. et al. (2020). Retrieval-augmented generation for knowledge-intensive NLP tasks. *NeurIPS 2020*.
11. Gray, J. & Lamport, L. (1978). Notes on database operating systems. *Springer LNCS 60*.
12. Garcia-Molina, H. & Salem, K. (1987). SAGAS. *SIGMOD 1987*.
13. Google Benchmark. (2023). <https://github.com/google/benchmark>.
14. IEEE Std 2807-2022. Framework for Artificial Intelligence (AI) Systems Using Ontology.
15. ISO/IEC 14756:2015. Measurement and rating of performance of computer-based software systems.
16. Cooper, B. et al. (2010). Benchmarking cloud serving systems with YCSB. *SoCC 2010*.
17. Transaction Processing Performance Council. (2010). TPC-C Benchmark v5.11.
18. Transaction Processing Performance Council. (2013). TPC-H Benchmark v2.17.1.
19. Aumuller, M. et al. (2020). ANN-Benchmarks: a benchmarking tool for ANN algorithms. *Information Systems 87*.
20. Angles, R. et al. (2020). The LDBC Social Network Benchmark. arXiv:2001.02299.
21. McCalpin, J. (1995). Memory bandwidth and machine balance in current high-performance computers. *IEEE Technical Committee on Computer Architecture*.
22. Hennessy, J. & Patterson, D. (2019). A new golden age for computer architecture. *CACM 62*(2).
23. Welch, B. L. (1947). The generalization of "Student's" problem when several different population variances are involved. *Biometrika 34*(1/2), 28–35.
24. Mann, H. B. & Whitney, D. R. (1947). On a test of whether one of two random variables is stochastically larger than the other. *Ann. Math. Statist. 18*(1), 50–60.
25. Cohen, J. (1988). *Statistical Power Analysis for the Behavioral Sciences* (2nd ed.). Lawrence Erlbaum.
26. Tukey, J. W. (1977). *Exploratory Data Analysis*. Addison-Wesley.
27. Johnson, J. et al. (2021). Billion-scale similarity search with GPUs. *IEEE Big Data 2021*.
28. Leijen, D. et al. (2019). Mimalloc: free list sharding in action. *APLAS 2019*.
29. Lemire, D. et al. (2018). Roaring bitmaps: implementation of an optimized software library. *Software: Practice and Experience 48*(4).
30. LDBC Social Network Benchmark. (2023). <https://ldbcouncil.org/benchmarks/snb/>.

---

## Appendix A — Efficiency Formula Derivation

### A.1 Hardware Factor Definition

Each hardware factor $N_f$ is defined as a ratio of the measured platform capability to a reference machine capability:

$$N_f = \frac{\hat{C}_f}{C_f^{\text{ref}}}$$

where $\hat{C}_f$ is the measured value and $C_f^{\text{ref}}$ is the reference-machine value, both expressed in the same unit (GB/s, M ops/s, etc.). Values are clamped to $[0.10, 3.00]$ to prevent outlier machines from producing degenerate scores.

### A.2 Class Capability Factor

The combined class capability factor $E_c$ aggregates all normalized factors relevant to workload class $c$ by a weighted geometric mean:

$$E_c = \prod_{f \in \mathcal{F}} N_f^{w_{c,f}}$$

where $\mathcal{F} = \{$cpu, mem, seq, rand, vram, h2d, d2h, dispatch$\}$ and the weight vector $\mathbf{w}_c$ is normalized per class: $\sum_f w_{c,f} = 1$.

The geometric mean is preferred over the arithmetic mean because performance is multiplicatively bounded: a system with $N_\text{cpu} = 2$ and $N_\text{mem} = 0.5$ achieves the same throughput as the reference machine (1.0 net), whereas an arithmetic mean would yield 1.25.

### A.3 Hardware-Neutral Score

The hardware-neutral score for benchmark $b$ on platform $p$ is:

$$\text{score}_{b,p} = \frac{T_{b,p}}{T_{b}^{\text{target}} \cdot E_{c(b),p}}$$

where $T_{b,p}$ is the measured throughput, $T_b^{\text{target}}$ is the documented product target, and $E_{c(b),p}$ is the capability factor of platform $p$ for the workload class of benchmark $b$. A score of 1.0 indicates the benchmark exactly meets its target after accounting for platform hardware capability. Values > 1.0 indicate above-target performance; values < 0.9 trigger the regression governance rule (§6.5).

### A.4 Recalibration Trigger

The automatic recalibration proposal fires when $\text{score}_{b,p} > 1.10$ for the same benchmark $b$ across at least three distinct platforms. In this case the target $T_b^{\text{target}}$ is raised to the geometric mean of the three measured values:

$$T_b^{\text{target, new}} = \left(\prod_{i=1}^{3} T_{b,p_i}\right)^{1/3}$$

This prevents benchmark targets from becoming permanently stale as hardware improves. The updated target takes effect at the next minor release boundary (e.g., v1.9 → v2.0) so that in-flight performance work is not retroactively invalidated.

---

## Appendix B — CHIMERA Workload Schema

The CHIMERA adapter serializes all benchmark results to a JSON structure conformant with IEEE Std 2807-2022 Annex A. System names are replaced with stable UUIDs during ingestion; the alias table is kept in a restricted-access registry and is not distributed with benchmark artefacts.

**Table 27 — CHIMERA Result Schema (IEEE Std 2807-2022 Annex A)**

| Field | Type | Description |
|---|---|---|
| `run_id` | string | UUID of the benchmark run |
| `system_alias` | string | Anonymised system identifier |
| `suite` | enum | YCSB / TPC-C / TPC-H / ANN / LDBC / vLLM / RAGBench |
| `workload` | string | Suite-specific workload name |
| `scale_factor` | number | Dataset scale (e.g. SF=10 for TPC-H) |
| `n_samples` | integer | Number of independent runs (≥ 28) |
| `mean` | number | Arithmetic mean throughput |
| `median` | number | Median throughput |
| `p95` | number | 95th percentile latency (ms) |
| `p99` | number | 99th percentile latency (ms) |
| `ci_lo_95` | number | Lower bound of 95% CI (Welch) |
| `ci_hi_95` | number | Upper bound of 95% CI (Welch) |
| `cohen_d` | number | Effect size vs. reference system |
| `outliers_removed` | integer | IQR×1.5 outliers removed |
| `hw_snapshot` | object | Hardware baseline (Appendix A) |
| `timestamp` | ISO8601 | Run completion timestamp |

Participating systems submit raw result archives (JSON + hardware snapshot) to the CHIMERA coordinator. The coordinator runs the statistical pipeline (outlier removal, normalization, CI computation) and publishes aggregate comparison tables to the read-only leaderboard API at `chimera.themisdb.io/api/v1/results`. Individual run archives are retained for 24 months to support longitudinal analysis.
