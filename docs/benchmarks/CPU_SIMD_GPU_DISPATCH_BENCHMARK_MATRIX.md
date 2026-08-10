# CPU SIMD vs GPU Dispatch — Benchmark Matrix
## ANN · Tensor · Graph Paths · Dynamic Tensor Updates

> **Issue:** [#5466](https://github.com/makr-code/ThemisDB/issues/5466)  
> **Status:** `[~]` In Progress — Benchmark plan defined; baselines pending hardware run  
> **Target:** Q3 2026  
> **Owner:** Acceleration / Tensor teams  
> **Relates to:** `docs/ci-cd/gpu-benchmark-matrix-runner.md`, `benchmarks/ROADMAP.md §Planned Features`

---

## Table of Contents

1. [Goals & Scope](#1-goals--scope)
2. [Methodology](#2-methodology)
3. [Benchmark Matrix — ANN Frontdoor](#3-benchmark-matrix--ann-frontdoor)
4. [Benchmark Matrix — Tensor Mid-Layer](#4-benchmark-matrix--tensor-mid-layer)
5. [Benchmark Matrix — Graph Paths](#5-benchmark-matrix--graph-paths)
6. [Benchmark Matrix — Dynamic Tensor Updates](#6-benchmark-matrix--dynamic-tensor-updates)
7. [Cross-Cutting Scenarios](#7-cross-cutting-scenarios)
8. [Metrics Reference](#8-metrics-reference)
9. [Break-Even Thresholds & Classification](#9-break-even-thresholds--classification)
10. [Benchmark Binary Plan](#10-benchmark-binary-plan)
11. [CTest Smoke Coverage](#11-ctest-smoke-coverage)
12. [Performance Baselines (Target)](#12-performance-baselines-target)
13. [Acceleration Classification](#13-acceleration-classification)
14. [Known Gaps & Follow-Up](#14-known-gaps--follow-up)

---

## 1. Goals & Scope

This document defines the evidence-based benchmark matrix for determining where GPU
acceleration yields a measurable advantage over CPU SIMD or CPU-local execution in
ThemisDB's core query and update paths.

### Objectives

- Measure GPU vs CPU SIMD vs CPU scalar throughput and latency across all primary execution
  paths (ANN, Tensor, Graph).
- Establish **break-even thresholds**: minimum batch size / tensor rank / graph frontier
  at which GPU dispatch outweighs host↔device transfer cost.
- Classify workloads as **acceleration-friendly** or **acceleration-hostile** with
  empirical evidence.
- Provide inputs usable by the query planner (`src/query/`) and lifecycle decisions
  (`src/distributed_tensor/`).
- Cover both **query-time** (retrieval) and **update-time** (dynamic tensor maintenance)
  tensor workloads.

### Out of Scope

- LLM / LoRA training acceleration (covered by `benchmarks/bench_gpu_training_cycle.cpp`).
- End-to-end RAG pipeline latency (covered by `benchmarks/bench_rag_pipeline.cpp`).
- Multi-GPU scaling (covered by `benchmarks/bench_multi_gpu_scaling.cpp`).

---

## 2. Methodology

### Execution Configuration

| Parameter | Value |
|---|---|
| Warmup iterations | 5 |
| Measurement iterations | ≥ 30 (statistical requirement per `benchmarks/ROADMAP.md §Phase 5`) |
| Timer | `benchmark::kMicrosecond` (Google Benchmark) |
| Percentiles reported | P50, P95, P99 |
| Output format | JSON (`--benchmark_format=json`) |
| Baseline freeze | Compiler, flags, preset, hardware, OS locked per run |

### Execution Backends

| Backend ID | Description | CMake Flag |
|---|---|---|
| `cpu-scalar` | Single-threaded reference (no SIMD) | `-DTHEMIS_SIMD_LEVEL=none` |
| `cpu-simd` | AVX2 / NEON SIMD (auto-selected by target arch) | default |
| `cpu-avx512` | AVX-512 on supported Intel/AMD CPUs | `-DTHEMIS_SIMD_LEVEL=avx512` |
| `gpu-cuda` | NVIDIA CUDA (sm_80 / sm_89 / sm_90) | `-DTHEMIS_ENABLE_CUDA=ON` |
| `gpu-hip` | AMD HIP / ROCm (gfx1100 / gfx90a) | `-DTHEMIS_ENABLE_HIP=ON` |
| `gpu-vulkan` | Vulkan Compute | `-DTHEMIS_ENABLE_VULKAN=ON` |

### Sweep Parameters

Sweeps are parameterised via `BENCHMARK_REGISTER_F(...)->ArgsProduct(...)` following
the pattern established in `benchmarks/bench_cuda_vs_cpu.cpp`.

---

## 3. Benchmark Matrix — ANN Frontdoor

**Binary:** `bench_ann_cpu_gpu_dispatch`  
**Source:** `benchmarks/bench_ann_cpu_gpu_dispatch.cpp`  
**Existing reference:** `benchmarks/bench_cuda_vs_cpu.cpp`, `benchmarks/bench_acceleration_dispatch.cpp`

### 3.1 HNSW CPU vs GPU-Assisted Vector Search

| Scenario ID | Description | Sweep Parameter | Range |
|---|---|---|---|
| `ANN-S1` | L2 distance: CPU scalar vs SIMD vs GPU | `num_vectors` | 256, 1K, 4K, 16K, 64K |
| `ANN-S2` | Cosine distance: CPU scalar vs SIMD vs GPU | `num_vectors` | 256, 1K, 4K, 16K, 64K |
| `ANN-S3` | Inner product: CPU scalar vs SIMD vs GPU | `num_vectors` | 256, 1K, 4K, 16K, 64K |
| `ANN-S4` | Top-K selection: CPU vs GPU | `num_vectors`, `k` | vectors: 1K–64K; k: 1, 10, 50, 100 |
| `ANN-S5` | Batch query throughput | `batch_size` | 1, 8, 32, 128, 512, 1024 |

**Dimension sweep** (applied to `ANN-S1..S3`): `dim` ∈ {64, 128, 256, 512, 1024, 1536}

**Fixed parameters:** `num_queries = 1` (single-query latency mode), then `num_queries` swept
alongside `batch_size` for throughput mode.

### 3.2 Batch Size Sensitivity

| Scenario ID | Description | Primary Variable | Fixed |
|---|---|---|---|
| `ANN-B1` | Query batch vs GPU transfer amortisation | `batch_size` 1–2048 | dim=128, n=16K |
| `ANN-B2` | GPU break-even batch size at dim=128 | `batch_size` 1–512 | n=16K |
| `ANN-B3` | GPU break-even batch size at dim=512 | `batch_size` 1–512 | n=16K |
| `ANN-B4` | Large-N effect on break-even | `num_vectors` 4K–512K | batch=1, dim=128 |

### 3.3 Top-K Size Sensitivity

| Scenario ID | Description | Primary Variable |
|---|---|---|
| `ANN-K1` | Top-K latency CPU vs GPU, fixed corpus | `k` ∈ {1, 5, 10, 50, 100, 500} |
| `ANN-K2` | Top-K accuracy impact (recall@k) — CPU vs GPU path | `k` ∈ {10, 100} |

### 3.4 Quantised vs Non-Quantised Artifacts

| Scenario ID | Description |
|---|---|
| `ANN-Q1` | FP32 vs INT8 quantised vectors: CPU SIMD path |
| `ANN-Q2` | FP32 vs INT8 quantised vectors: GPU path |
| `ANN-Q3` | Quantised → dequantise on device vs host |

---

## 4. Benchmark Matrix — Tensor Mid-Layer

**Binary:** `bench_tensor_cpu_gpu_dispatch` *(implemented — Phase 1: TEN-S1..S4, TEN-G1..G4, TEN-M1..M2)*  
**Existing reference:** `benchmarks/bench_tensor_integration_baseline.cpp`,
`benchmarks/bench_tensor_fingerprint.cpp`

### 4.1 CPU SIMD Tensor Similarity

| Scenario ID | Description | Sweep |
|---|---|---|
| `TEN-S1` | Cosine similarity: CPU scalar vs AVX2 vs AVX-512 | rank ∈ {4, 8, 16, 32, 64} |
| `TEN-S2` | Frobenius norm: CPU vs GPU | rank × dim |
| `TEN-S3` | Dot-product contraction: CPU SIMD vs GPU | tensor shape 64×64–1024×1024 |
| `TEN-S4` | Batch similarity scoring: CPU vs GPU | batch 1–512 |

### 4.2 GPU Contraction / Refinement

| Scenario ID | Description | Sweep |
|---|---|---|
| `TEN-G1` | GEMM contraction: CPU BLAS vs cuBLAS vs rocBLAS | matrix dims 256–4096 |
| `TEN-G2` | Shard relevance scoring: CPU vs GPU | shard count 4–256 |
| `TEN-G3` | Summary generation (mean-pool): CPU vs GPU | tensor count 16–1024 |
| `TEN-G4` | Residual/error tracking: CPU vs GPU accumulation | rank 4–64 |

### 4.3 mmap-Backed Artifact Access vs Device Upload

| Scenario ID | Description | Notes |
|---|---|---|
| `TEN-M1` | mmap read latency: cold vs warm page cache | Isolate I/O from compute |
| `TEN-M2` | mmap → CPU SIMD pipeline: end-to-end | Baseline for GPU comparison |
| `TEN-M3` | mmap → cudaMemcpy → GPU compute: end-to-end | Full transfer cost included |
| `TEN-M4` | mmap-warm vs device-resident latency gap | Quantify when GPU resident wins |
| `TEN-M5` | SSD random-access influence on mmap path | PCIe NVMe vs SATA SSD |

---

## 5. Benchmark Matrix — Graph Paths

**Binary:** `bench_graph_cpu_gpu_dispatch` *(planned)*  
**Existing reference:** `benchmarks/bench_graph_traversal.cpp`,
`benchmarks/bench_graph_query_optimizer.cpp`

### 5.1 Bounded Graph Kernels on GPU

| Scenario ID | Description | Sweep |
|---|---|---|
| `GRP-G1` | Batch BFS: CPU traversal vs GPU kernel | graph nodes 1K–1M |
| `GRP-G2` | Batch shortest-path: CPU Dijkstra vs GPU BFS | node count, edge density |
| `GRP-G3` | Frontier expansion: CPU vs GPU parallelism | frontier width 1–10K |
| `GRP-G4` | GPU kernel overhead at small frontier (< 64 nodes) | frontier 1–64 |

### 5.2 Frontier Expansion vs CPU Traversal

| Scenario ID | Description | Key Metric |
|---|---|---|
| `GRP-F1` | Frontier width break-even (GPU efficient at width ≥ N) | frontier node count |
| `GRP-F2` | Depth sensitivity: BFS depth vs GPU utility | BFS depth 1–20 |
| `GRP-F3` | Edge-density sensitivity: sparse vs dense graphs | edges/node 2–32 |

### 5.3 Neighborhood Exploration vs CPU Metadata-Local Traversal

| Scenario ID | Description |
|---|---|
| `GRP-N1` | k-hop neighborhood: CPU cache-local vs GPU parallel |
| `GRP-N2` | Metadata join during traversal: CPU vs GPU data-parallel |
| `GRP-N3` | Predicate-filtered traversal: GPU selectivity benefit |

### 5.4 Synchronisation-Heavy Paths vs CPU Execution

| Scenario ID | Description |
|---|---|
| `GRP-X1` | Mutex-gated graph update vs GPU atomic path |
| `GRP-X2` | Barrier-synchronised frontier merge: CPU vs GPU |
| `GRP-X3` | Cross-shard graph kernel: transfer + synchronisation overhead |

---

## 6. Benchmark Matrix — Dynamic Tensor Updates

> **Added per issue extension:** Covers commit-path overhead, tensor update worker
> paths, planner impact, and CPU/GPU break-even for tensor maintenance.

**Binary:** `bench_tensor_update_dispatch` *(planned)*  
**Binary:** `bench_tensor_commit_overhead` *(planned)*

### 6.1 Commit Path Overhead

| Scenario ID | Description | Baseline |
|---|---|---|
| `UPD-C1` | Baseline RocksDB transaction (no tensor delta) | Reference latency |
| `UPD-C2` | RocksDB transaction + tensor delta logging | Delta = UPD-C2 − UPD-C1 |
| `UPD-C3` | RocksDB transaction + tensor delta logging + manifest invalidation | Full commit cost |
| `UPD-C4` | Commit path latency vs tensor delta size | delta size 64 B–64 KiB |

### 6.2 Tensor Update Worker Paths

| Scenario ID | Description | Sweep |
|---|---|---|
| `UPD-W1` | Small delta patch path: CPU vs GPU | delta size 1–256 elements |
| `UPD-W2` | Partial refit path: CPU SIMD vs GPU | refit fraction 10%–50% |
| `UPD-W3` | Full snapshot rebuild: CPU vs GPU | tensor rank 4–64 |
| `UPD-W4` | Rank growth sensitivity: rebuild latency vs rank | rank 4, 8, 16, 32, 64 |
| `UPD-W5` | Residual / approximation error tracking overhead | rank sweep |

### 6.3 Planner Impact

| Scenario ID | Description |
|---|---|
| `PLN-P1` | Summary-first routing latency vs direct exact fetch |
| `PLN-P2` | Fallback frequency to exact graph path (recall quality) |
| `PLN-P3` | Fan-out reduction under summary routing |
| `PLN-P4` | False-negative risk in tensor routing vs quality threshold |

### 6.4 CPU/GPU Break-Even for Tensor Maintenance

| Scenario ID | Description | Sweep |
|---|---|---|
| `UPD-BE1` | Batch size sweep: patch vs partial refit vs rebuild | batch 1–512 |
| `UPD-BE2` | Tensor density sweep: sparse vs dense update windows | density 1%–100% |
| `UPD-BE3` | Rank sweep: at which rank does GPU win? | rank 4–64 |
| `UPD-BE4` | Host↔device transfer overhead fraction of update cost | tensor size 1 KiB–16 MiB |
| `UPD-BE5` | CPU SIMD vs GPU for bounded update windows | window 16–4096 elements |

---

## 7. Cross-Cutting Scenarios

| Scenario ID | Description | Relevant Paths |
|---|---|---|
| `CRS-T1` | Host↔device transfer cost vs compute: latency breakdown | All GPU paths |
| `CRS-T2` | Pinned (page-locked) vs pageable memory transfer overhead | ANN, Tensor |
| `CRS-T3` | SSD / mmap I/O influence on GPU pipeline latency | Tensor, ANN |
| `CRS-T4` | Cross-shard transfer amplification: shards 1–32 | Graph, Tensor |
| `CRS-Q1` | Quantised INT8 artifact: CPU SIMD vs GPU decode + compute | ANN, Tensor |
| `CRS-Q2` | Mixed FP16/FP32 precision: GPU benefit vs CPU parity | Tensor |
| `CRS-M1` | VRAM pressure: GPU performance degradation near VRAM limit | Tensor, ANN |
| `CRS-M2` | CPU NUMA effect on SIMD throughput (cross-socket) | ANN, Tensor |

---

## 8. Metrics Reference

### Primary Metrics

| Metric | Unit | Description |
|---|---|---|
| `latency_p50` | µs | Median single-operation latency |
| `latency_p95` | µs | 95th-percentile latency (tail) |
| `latency_p99` | µs | 99th-percentile latency (worst case) |
| `throughput` | ops/s | Sustained operations per second at steady state |
| `items_per_second` | items/s | Google Benchmark `SetItemsProcessed` counter |

### Transfer and Memory Metrics

| Metric | Unit | Description |
|---|---|---|
| `h2d_latency` | µs | Host-to-device transfer latency |
| `d2h_latency` | µs | Device-to-host transfer latency |
| `h2d_bandwidth` | GB/s | Effective host-to-device bandwidth |
| `vram_peak_mb` | MiB | Peak VRAM usage during benchmark |
| `cpu_mem_peak_mb` | MiB | Peak host RAM usage |

### Quality Metrics (ANN paths)

| Metric | Unit | Description |
|---|---|---|
| `recall_at_k` | [0, 1] | Fraction of true top-K results returned |
| `approximation_error` | L2 | Mean distance error vs exact result |
| `false_negative_rate` | [0, 1] | Fraction of exact results missed by GPU path |

### Update-Path Metrics

| Metric | Unit | Description |
|---|---|---|
| `commit_latency` | µs | End-to-end commit path duration (UPD-C* scenarios) |
| `update_throughput` | tensors/s | Sustained update throughput (patch / refit / rebuild) |
| `rebuild_latency` | ms | Full snapshot rebuild duration |
| `routing_quality` | [0, 1] | Fraction of queries correctly routed by planner |
| `exact_fallback_rate` | [0, 1] | Fraction of queries falling back to exact graph path |

### Derived Break-Even Metrics

| Metric | Description |
|---|---|
| `gpu_breakeven_batch` | Minimum batch size where GPU P50 latency ≤ CPU P50 latency |
| `gpu_breakeven_n` | Minimum vector/node count where GPU wins on throughput |
| `transfer_overhead_fraction` | `(h2d_latency + d2h_latency) / total_gpu_latency` |
| `speedup_ratio` | `cpu_simd_latency / gpu_latency` (> 1 = GPU wins) |

---

## 9. Break-Even Thresholds & Classification

The following thresholds are **targets** derived from existing data in
`benchmarks/bench_cuda_vs_cpu.cpp` and the acceleration roadmap. They must be
validated with hardware runs before becoming canonical.

### ANN Paths — Preliminary Break-Even Estimates

| Metric | CPU SIMD wins | GPU wins | Notes |
|---|---|---|---|
| `num_vectors` at dim=128 | < 4K | ≥ 4K | Based on existing `BM_CUDA_ANN_L2Distance` pattern |
| `batch_size` at n=16K, dim=128 | < 32 | ≥ 32 | Transfer amortisation threshold |
| `dim` at batch=1, n=16K | < 256 | ≥ 256 | Compute intensity threshold |
| Top-K `k` | always CPU for k ≤ 5 | GPU at k ≥ 50 | Selection dominates at small k |

### Tensor Mid-Layer — Preliminary Break-Even Estimates

| Metric | CPU SIMD wins | GPU wins |
|---|---|---|
| Tensor batch | < 16 tensors | ≥ 16 tensors |
| GEMM dim | < 256×256 | ≥ 256×256 |
| Shard count | < 32 shards | ≥ 32 shards |
| mmap-warm artifact | Always CPU (no transfer) | GPU only if device-resident |

### Graph Paths — Preliminary Break-Even Estimates

| Metric | CPU wins | GPU wins |
|---|---|---|
| Graph nodes | < 10K | ≥ 10K |
| BFS frontier width | < 64 nodes | ≥ 64 nodes |
| Metadata join during traversal | Always CPU | GPU: predicate selectivity < 30% |

### Dynamic Tensor Updates — Preliminary Break-Even Estimates

| Metric | CPU SIMD wins | GPU wins |
|---|---|---|
| Delta patch elements | < 256 | ≥ 256 |
| Tensor rank | < rank 16 | ≥ rank 16 |
| Rebuild: tensor count | < 8 | ≥ 8 |

---

## 10. Benchmark Binary Plan

The following benchmark binaries are planned as part of this matrix.
Existing binaries are referenced; new binaries are marked *(planned)*.

| Binary | Source File | Status | Covers |
|---|---|---|---|
| `bench_cuda_vs_cpu` | `benchmarks/bench_cuda_vs_cpu.cpp` | ✅ Existing | ANN-S1..S3 (partial) |
| `bench_acceleration_dispatch` | `benchmarks/bench_acceleration_dispatch.cpp` | ✅ Existing | ANN-S1..S4 (CPU) |
| `bench_tensor_integration_baseline` | `benchmarks/bench_tensor_integration_baseline.cpp` | ✅ Existing | TEN-S1..S4 (partial) |
| `bench_graph_traversal` | `benchmarks/bench_graph_traversal.cpp` | ✅ Existing | GRP-F1..F3 (CPU only) |
| `bench_graph_query_optimizer` | `benchmarks/bench_graph_query_optimizer.cpp` | ✅ Existing | GRP-N1..N3 (CPU only) |
| `bench_ann_cpu_gpu_dispatch` | `benchmarks/bench_ann_cpu_gpu_dispatch.cpp` | ✅ Existing | ANN-S1..S5 (phase 1: scalar/SIMD/mixed-path/HNSW), ANN-B1..B4 (initial batch and k sweeps) |
| `bench_tensor_cpu_gpu_dispatch` | `benchmarks/bench_tensor_cpu_gpu_dispatch.cpp` | ✅ Phase 1 | TEN-S1..S4, TEN-G1..G4, TEN-M1..M2 (TEN-M3..M5 planned) |
| `bench_graph_cpu_gpu_dispatch` | `benchmarks/bench_graph_cpu_gpu_dispatch.cpp` | *(planned)* | GRP-G1..G4, GRP-F1..F3, GRP-N1..N3, GRP-X1..X3 |
| `bench_tensor_update_dispatch` | `benchmarks/bench_tensor_update_dispatch.cpp` | *(planned)* | UPD-W1..W5, UPD-BE1..BE5 |
| `bench_tensor_commit_overhead` | `benchmarks/bench_tensor_commit_overhead.cpp` | *(planned)* | UPD-C1..C4 |
| `bench_cross_cutting` | `benchmarks/bench_cross_cutting.cpp` | *(planned)* | CRS-T1..T4, CRS-Q1..Q2, CRS-M1..M2 |

### CMake Registration

New binaries must be registered in `benchmarks/CMakeLists.txt` following the
existing pattern and recorded in `benchmarks/benchmark_target_mapping.json`.

---

## 11. CTest Smoke Coverage

Each planned benchmark binary requires a CTest smoke target that:
1. Builds in CPU-only mode (no GPU required in CI).
2. Runs with `--benchmark_filter=Smoke` for ≤ 5 seconds.
3. Returns exit code 0.
4. Is registered in the CI workflow `acceleration-benchmark-ci`.

| CTest Target | Binary | Smoke Filter | Timeout |
|---|---|---|---|
| `smoke_bench_ann_cpu_gpu_dispatch` | `bench_ann_cpu_gpu_dispatch` | `BM_Smoke_ANN_.*` | 30 s |
| `smoke_bench_tensor_cpu_gpu_dispatch` | `bench_tensor_cpu_gpu_dispatch` | `BM_Smoke_.*` | 30 s |
| `smoke_bench_graph_cpu_gpu_dispatch` | `bench_graph_cpu_gpu_dispatch` | `BM_Smoke_.*` | 30 s |
| `smoke_bench_tensor_update_dispatch` | `bench_tensor_update_dispatch` | `BM_Smoke_.*` | 30 s |
| `smoke_bench_tensor_commit_overhead` | `bench_tensor_commit_overhead` | `BM_Smoke_.*` | 30 s |
| `smoke_bench_cross_cutting` | `bench_cross_cutting` | `BM_Smoke_.*` | 30 s |

Smoke tests must compile and run under the `linux-release` / `windows-release` presets
without any GPU hardware present.

---

## 12. Performance Baselines (Target)

The following are **target** baselines; actual values must be populated after the
first hardware run on the reference GPU runner
(`docs/ci-cd/gpu-benchmark-matrix-runner.md §Runner-Profile`).

### ANN Frontdoor Targets

| Scenario | CPU SIMD Target | GPU Target | Condition |
|---|---|---|---|
| L2 distance, 1K vec, dim=128 | ≤ 500 µs | n/a (no GPU benefit) | batch=1 |
| L2 distance, 16K vec, dim=128 | ≤ 4 ms | ≤ 1 ms | batch=1 |
| L2 distance, 64K vec, dim=512 | ≤ 20 ms | ≤ 4 ms | batch=1 |
| Top-K k=100, 16K vec | ≤ 2 ms | ≤ 500 µs | dim=128 |
| Batch=128, 16K vec, dim=128 | ≥ 80K qps | ≥ 400K qps | throughput |

### Tensor Mid-Layer Targets

| Scenario | CPU SIMD Target | GPU Target | Condition |
|---|---|---|---|
| Cosine similarity, rank=8 | ≤ 50 µs | n/a | batch=1 |
| GEMM 512×512 | ≤ 2 ms | ≤ 200 µs | single op |
| Shard scoring, 64 shards | ≤ 5 ms | ≤ 500 µs | batch=32 |

### Commit Path Targets

| Scenario | Target |
|---|---|
| Baseline RocksDB transaction (UPD-C1) | ≤ 1 ms |
| + tensor delta logging (UPD-C2) | ≤ 2 ms |
| + manifest invalidation (UPD-C3) | ≤ 3 ms |
| Small delta patch, CPU (UPD-W1) | ≤ 100 µs |
| Full snapshot rebuild, rank=16 (UPD-W3) | ≤ 50 ms |

### Routing Quality Targets

| Metric | Target |
|---|---|
| Routing quality (PLN-P1..P4) | ≥ 0.95 |
| Exact fallback rate | ≤ 0.05 |
| False-negative rate | ≤ 0.02 |

---

## 13. Acceleration Classification

Based on preliminary estimates; final classification requires hardware validation.

### Acceleration-Friendly Workloads (GPU wins)

| Workload | Minimum Condition |
|---|---|
| ANN L2 / cosine distance | batch ≥ 32 AND n ≥ 4K AND dim ≥ 128 |
| Top-K selection | k ≥ 50 AND n ≥ 16K |
| GEMM tensor contraction | matrix ≥ 256×256 |
| Shard relevance scoring | shards ≥ 32 AND batch ≥ 16 |
| Tensor delta patch | elements ≥ 256 |
| BFS frontier expansion | frontier ≥ 64 nodes AND graph ≥ 10K nodes |
| Full snapshot rebuild | rank ≥ 16 AND tensor count ≥ 8 |

### Acceleration-Hostile Workloads (CPU wins)

| Workload | Reason |
|---|---|
| Single-query ANN, n < 4K | Transfer overhead dominates |
| ANN with mmap-warm artifacts (no device-resident copy) | mmap page cache faster than H2D |
| Metadata-local graph traversal (k-hop ≤ 3, n < 10K) | CPU cache locality dominates |
| Small delta patch (< 256 elements) | Kernel launch overhead dominates |
| Commit path tensor delta logging | Sequential write, no parallelism |
| Barrier-synchronised graph paths | Sync cost eliminates GPU benefit |

---

## 14. Known Gaps & Follow-Up

| Gap ID | Description | Owner | Target |
|---|---|---|---|
| `GAP-1` | Actual hardware run to validate break-even thresholds against shipped CPU baselines | Acceleration team | 2027 |
| `GAP-2` | Extend `bench_ann_cpu_gpu_dispatch.cpp` with quantised artifact and recall-quality scenarios | ANN team | Q3 2026 |
| `GAP-3` | `bench_tensor_cpu_gpu_dispatch.cpp` implementation | Tensor team | Q3 2026 |
| `GAP-4` | `bench_graph_cpu_gpu_dispatch.cpp` implementation | Graph team | Q3 2026 |
| `GAP-5` | `bench_tensor_update_dispatch.cpp` implementation | Tensor team | Q3 2026 |
| `GAP-6` | `bench_tensor_commit_overhead.cpp` implementation | Storage/Tensor team | Q3 2026 |
| `GAP-7` | Populate performance baselines table after first run | Acceleration team | Q3 2026 |
| `GAP-8` | Integrate baseline JSON into `benchmarks/baselines/` | Acceleration team | Q3 2026 |
| `GAP-9` | Query planner integration: consume break-even thresholds | Query team | Q4 2026 |
| `GAP-10` | Lifecycle decision integration: update worker path selection | Tensor team | Q4 2026 |

---

## Related Documents

- [`docs/ci-cd/gpu-benchmark-matrix-runner.md`](../ci-cd/gpu-benchmark-matrix-runner.md) — GPU CI runner setup
- [`docs/benchmarks/CPU_SIMD_GPU_BENCHMARK_SCENARIO_CATALOG.md`](CPU_SIMD_GPU_BENCHMARK_SCENARIO_CATALOG.md) — Detailed scenario catalog
- [`docs/benchmarks/slo_benchmark_matrix_v190.md`](slo_benchmark_matrix_v190.md) — Wave2 SLO matrix
- [`docs/performance/PERFORMANCE_EXPECTATIONS.md`](../performance/PERFORMANCE_EXPECTATIONS.md) — KPI definitions
- [`benchmarks/ROADMAP.md`](../../benchmarks/ROADMAP.md) — Benchmark module roadmap
- [`benchmarks/bench_cuda_vs_cpu.cpp`](../../benchmarks/bench_cuda_vs_cpu.cpp) — Existing CUDA vs CPU binary
- [`docs/analysis/GPU_ACCELERATION_ADDENDUM.md`](../analysis/GPU_ACCELERATION_ADDENDUM.md) — GPU infrastructure addendum
