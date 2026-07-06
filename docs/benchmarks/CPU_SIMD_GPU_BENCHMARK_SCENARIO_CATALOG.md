# CPU SIMD vs GPU Dispatch — Benchmark Scenario Catalog
## ANN · Tensor · Graph · Dynamic Tensor Updates

> **Issue:** [#5466](https://github.com/makr-code/ThemisDB/issues/5466)  
> **Status:** `[~]` In Progress  
> **Target:** Q3 2026  
> **Authoritative matrix:** [`CPU_SIMD_GPU_DISPATCH_BENCHMARK_MATRIX.md`](CPU_SIMD_GPU_DISPATCH_BENCHMARK_MATRIX.md)

This catalog provides the per-scenario detail that the benchmark matrix summarises.
Each entry defines: workload, parameters, backends, metrics, acceptance gate, and
break-even classification.

---

## Scenario ID Convention

```
<LAYER>-<TYPE><N>
  LAYER : ANN | TEN | GRP | UPD | PLN | CRS
  TYPE  : S (similarity/search) | B (batch sweep) | K (top-K) | Q (quantised)
          G (GPU kernel) | M (mmap) | F (frontier) | N (neighborhood)
          X (sync-heavy) | C (commit) | W (worker) | BE (break-even) | P (planner)
          T (transfer) | M (memory)
  N     : sequential number within layer+type
```

---

## ANN Frontdoor Scenarios

### ANN-S1 — L2 Distance: CPU Scalar vs SIMD vs GPU

| Field | Value |
|---|---|
| **Description** | Measures pairwise L2 distance between one query and N database vectors at varying dimensionalities and corpus sizes |
| **Binary** | `bench_ann_cpu_gpu_dispatch` |
| **Backends** | `cpu-scalar`, `cpu-simd`, `gpu-cuda`, `gpu-hip` |
| **Fixed params** | `num_queries = 1` |
| **Sweep: num_vectors** | 256, 1K, 4K, 16K, 64K, 256K |
| **Sweep: dim** | 64, 128, 256, 512, 1024, 1536 |
| **Metrics** | `latency_p50`, `latency_p95`, `throughput`, `speedup_ratio` |
| **Transfer measured** | `h2d_latency`, `d2h_latency` separately in `CRS-T1` |
| **Break-even criterion** | GPU P50 ≤ CPU SIMD P50 |
| **Expected GPU threshold** | n ≥ 4K at dim=128 |
| **Reference impl** | `benchmarks/bench_cuda_vs_cpu.cpp::BM_CPU_ANN_L2Distance` / `BM_CUDA_ANN_L2Distance` |

---

### ANN-S2 — Cosine Distance: CPU Scalar vs SIMD vs GPU

| Field | Value |
|---|---|
| **Description** | Cosine similarity for unit-normalised and non-normalised vector sets |
| **Binary** | `bench_ann_cpu_gpu_dispatch` |
| **Backends** | `cpu-scalar`, `cpu-simd`, `gpu-cuda`, `gpu-hip` |
| **Fixed params** | `num_queries = 1` |
| **Sweep: num_vectors** | 256, 1K, 4K, 16K, 64K |
| **Sweep: dim** | 64, 128, 256, 512 |
| **Metrics** | `latency_p50`, `latency_p95`, `speedup_ratio` |
| **Expected GPU threshold** | n ≥ 4K at dim=128 (same pattern as ANN-S1) |
| **Reference impl** | `benchmarks/bench_cuda_vs_cpu.cpp::BM_CPU_ANN_CosineDistance` |

---

### ANN-S3 — Inner Product: CPU Scalar vs SIMD vs GPU

| Field | Value |
|---|---|
| **Description** | Raw dot-product (no normalisation); typical for MIPS (Maximum Inner Product Search) |
| **Binary** | `bench_ann_cpu_gpu_dispatch` |
| **Backends** | `cpu-scalar`, `cpu-simd`, `gpu-cuda`, `gpu-hip` |
| **Sweep: num_vectors** | 256, 1K, 4K, 16K, 64K |
| **Sweep: dim** | 64, 128, 256, 512 |
| **Metrics** | `latency_p50`, `latency_p95`, `speedup_ratio` |
| **Reference impl** | `benchmarks/bench_cuda_vs_cpu.cpp::BM_CPU_ANN_InnerProduct` |

---

### ANN-S4 — Top-K Selection: CPU vs GPU

| Field | Value |
|---|---|
| **Description** | Isolates Top-K heap selection from distance computation using a pre-computed distance matrix |
| **Binary** | `bench_ann_cpu_gpu_dispatch` |
| **Backends** | `cpu-simd`, `gpu-cuda` |
| **Sweep: num_vectors** | 1K, 4K, 16K, 64K |
| **Sweep: k** | 1, 5, 10, 50, 100, 500 |
| **Fixed params** | `num_queries = 1` |
| **Metrics** | `latency_p50`, `latency_p99`, `throughput` |
| **Expected GPU threshold** | k ≥ 50 AND n ≥ 16K |
| **Reference impl** | `benchmarks/bench_cuda_vs_cpu.cpp::BM_CPU_ANN_TopK` |

---

### ANN-S5 — Batch Query Throughput

| Field | Value |
|---|---|
| **Description** | Throughput of concurrent batch queries; measures GPU transfer amortisation |
| **Binary** | `bench_ann_cpu_gpu_dispatch` |
| **Backends** | `cpu-simd`, `gpu-cuda`, `gpu-hip` |
| **Sweep: batch_size** | 1, 8, 32, 128, 512, 1024 |
| **Fixed params** | `num_vectors = 16K`, `dim = 128`, `k = 10` |
| **Metrics** | `throughput`, `latency_p95`, `h2d_bandwidth` |
| **Expected GPU threshold** | batch ≥ 32 |

---

### ANN-B1 — Query Batch vs GPU Transfer Amortisation

| Field | Value |
|---|---|
| **Description** | Sweeps batch size to find the point where GPU H2D transfer cost is amortised |
| **Binary** | `bench_ann_cpu_gpu_dispatch` |
| **Backends** | `cpu-simd`, `gpu-cuda` |
| **Sweep: batch_size** | 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 |
| **Fixed params** | `num_vectors = 16K`, `dim = 128` |
| **Metrics** | `latency_p50`, `h2d_latency`, `transfer_overhead_fraction`, `gpu_breakeven_batch` |
| **Output** | Break-even batch size (canonical input for query planner) |

---

### ANN-B2 / ANN-B3 — GPU Break-Even Batch Size at dim=128 / dim=512

| Field | ANN-B2 | ANN-B3 |
|---|---|---|
| **dim** | 128 | 512 |
| **Sweep: batch_size** | 1–512 | 1–512 |
| **Fixed: num_vectors** | 16K | 16K |
| **Output** | `gpu_breakeven_batch` at dim=128 | `gpu_breakeven_batch` at dim=512 |

---

### ANN-B4 — Large-N Effect on Break-Even

| Field | Value |
|---|---|
| **Description** | How does corpus size influence the GPU break-even batch size? |
| **Sweep: num_vectors** | 4K, 16K, 64K, 256K, 512K |
| **Fixed params** | `batch = 1`, `dim = 128` |
| **Metrics** | `speedup_ratio` per corpus size, `gpu_breakeven_n` |

---

### ANN-K1 / ANN-K2 — Top-K Latency and Recall Sensitivity

| Field | ANN-K1 | ANN-K2 |
|---|---|---|
| **Description** | Latency vs k | Recall@k comparison |
| **Sweep: k** | 1, 5, 10, 50, 100, 500 | 10, 100 |
| **Metric** | `latency_p50`, `speedup_ratio` | `recall_at_k`, `false_negative_rate` |

---

### ANN-Q1 / ANN-Q2 / ANN-Q3 — Quantised vs Non-Quantised Artifacts

| Scenario | Description |
|---|---|
| `ANN-Q1` | FP32 vs INT8: CPU SIMD path latency and recall impact |
| `ANN-Q2` | FP32 vs INT8: GPU path latency and recall impact |
| `ANN-Q3` | Quantised→dequantise on device vs host: which is faster? |

**Common parameters:** `num_vectors = 16K`, `dim = 128`, `k = 10`  
**Metrics:** `latency_p50`, `recall_at_k`, `approximation_error`, `vram_peak_mb`

---

## Tensor Mid-Layer Scenarios

### TEN-S1 — Cosine Similarity: CPU Scalar vs AVX2 vs AVX-512

| Field | Value |
|---|---|
| **Description** | SIMD tier comparison for tensor cosine similarity; no GPU (GPU is not expected to win at small rank) |
| **Binary** | `bench_tensor_cpu_gpu_dispatch` |
| **Backends** | `cpu-scalar`, `cpu-simd`, `cpu-avx512` |
| **Sweep: rank** | 4, 8, 16, 32, 64 |
| **Sweep: batch** | 1, 8, 32, 128 |
| **Metrics** | `latency_p50`, `throughput`, `speedup_ratio` (AVX-512 / AVX2) |
| **GPU included?** | No — GPU included in TEN-S4 batch sweep |

---

### TEN-S2 — Frobenius Norm: CPU vs GPU

| Field | Value |
|---|---|
| **Sweep: rank** | 4, 8, 16, 32, 64 |
| **Sweep: dim** | 64, 128, 256, 512 |
| **Backends** | `cpu-simd`, `gpu-cuda` |
| **Break-even criterion** | GPU P50 ≤ CPU SIMD P50 |

---

### TEN-G1 — GEMM Contraction: CPU BLAS vs cuBLAS vs rocBLAS

| Field | Value |
|---|---|
| **Description** | Dense matrix multiply as proxy for tensor contraction workload |
| **Binary** | `bench_tensor_cpu_gpu_dispatch` |
| **Backends** | `cpu-simd` (OpenBLAS), `gpu-cuda` (cuBLAS), `gpu-hip` (rocBLAS) |
| **Sweep: matrix_dim** | 64, 128, 256, 512, 1024, 2048, 4096 |
| **Metrics** | `latency_p50`, `throughput`, `speedup_ratio`, `vram_peak_mb` |
| **Expected GPU threshold** | matrix ≥ 256×256 |

---

### TEN-G2 — Shard Relevance Scoring: CPU vs GPU

| Field | Value |
|---|---|
| **Sweep: shard_count** | 4, 8, 16, 32, 64, 128, 256 |
| **Sweep: batch** | 1, 8, 32, 128 |
| **Backends** | `cpu-simd`, `gpu-cuda` |
| **Break-even criterion** | GPU wins at shard_count ≥ 32 AND batch ≥ 16 |

---

### TEN-M1..M5 — mmap-Backed Artifact Access vs Device Upload

| Scenario | Description | Key Measurement |
|---|---|---|
| `TEN-M1` | mmap cold vs warm page cache latency | I/O latency baseline |
| `TEN-M2` | mmap → CPU SIMD: end-to-end pipeline | Full CPU-side latency |
| `TEN-M3` | mmap → cudaMemcpy → GPU compute | Full GPU path including H2D |
| `TEN-M4` | mmap-warm vs device-resident latency gap | When does pre-upload pay off? |
| `TEN-M5` | SSD random-access influence (NVMe vs SATA SSD) | Storage I/O sensitivity |

**Metric for TEN-M4:** `transfer_overhead_fraction` — if > 0.5, mmap path is preferred.

---

## Graph Path Scenarios

### GRP-G4 — GPU Kernel Overhead at Small Frontier (< 64 nodes)

| Field | Value |
|---|---|
| **Description** | Quantifies kernel launch and synchronisation overhead at sub-breakeven frontier widths |
| **Binary** | `bench_graph_cpu_gpu_dispatch` |
| **Backends** | `cpu-simd`, `gpu-cuda` |
| **Sweep: frontier_width** | 1, 2, 4, 8, 16, 32, 64 |
| **Fixed params** | `graph_nodes = 100K` |
| **Expected result** | GPU loses at frontier < 64; CPU cache-local wins |
| **Metrics** | `latency_p50`, `speedup_ratio`, overhead decomposition |

---

### GRP-F1 — Frontier Width Break-Even

| Field | Value |
|---|---|
| **Description** | Find minimum frontier width where GPU P50 ≤ CPU SIMD P50 |
| **Sweep: frontier_width** | 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1K, 10K |
| **Output** | `gpu_breakeven_n` for frontier (canonical planner input) |

---

### GRP-X1..X3 — Synchronisation-Heavy Paths

| Scenario | Description | Expected Result |
|---|---|---|
| `GRP-X1` | Mutex-gated graph update vs GPU atomic | CPU wins; GPU atomics have high contention cost |
| `GRP-X2` | Barrier-synchronised frontier merge | CPU wins at all tested scales |
| `GRP-X3` | Cross-shard graph kernel: transfer + sync overhead | Measures `h2d_latency + d2h_latency` fraction |

---

## Dynamic Tensor Update Scenarios

### UPD-C1..C4 — Commit Path Overhead

| Scenario | Measured Component | Expected Latency |
|---|---|---|
| `UPD-C1` | Baseline RocksDB transaction (no tensor) | ≤ 1 ms |
| `UPD-C2` | + tensor delta logging | ≤ 2 ms (≤ 1 ms delta) |
| `UPD-C3` | + manifest invalidation | ≤ 3 ms (≤ 1 ms additional) |
| `UPD-C4` | Sweep delta size 64 B–64 KiB | Latency vs delta size profile |

**Binary:** `bench_tensor_commit_overhead`  
**Backends:** CPU only (commit path is sequential and not GPU-accelerated by design)  
**Primary metric:** `commit_latency` (P50, P95, P99)

---

### UPD-W1..W5 — Tensor Update Worker Paths

| Scenario | Description | Primary Variable | Expected Break-Even |
|---|---|---|---|
| `UPD-W1` | Small delta patch: CPU SIMD vs GPU | delta elements 1–256 | GPU wins at ≥ 256 elements |
| `UPD-W2` | Partial refit: CPU SIMD vs GPU | refit fraction 10%–50% of rank | GPU wins at fraction > 20% of large tensor |
| `UPD-W3` | Full snapshot rebuild | tensor rank 4–64, count 1–32 | GPU wins at rank ≥ 16 AND count ≥ 8 |
| `UPD-W4` | Rank growth: rebuild latency vs rank | rank 4, 8, 16, 32, 64 | Linear CPU, sub-linear GPU |
| `UPD-W5` | Residual / approximation error tracking | rank sweep | CPU only (tracking is lightweight scalar) |

**Binary:** `bench_tensor_update_dispatch`  
**Metrics:** `update_throughput`, `rebuild_latency`, `latency_p95`, `speedup_ratio`

---

### PLN-P1..P4 — Planner Impact Scenarios

| Scenario | Description | Acceptance Gate |
|---|---|---|
| `PLN-P1` | Summary-first routing latency vs direct exact fetch | Summary path ≤ 2× exact fetch latency |
| `PLN-P2` | Fallback frequency to exact graph path | `exact_fallback_rate` ≤ 0.05 |
| `PLN-P3` | Fan-out reduction under summary routing | ≥ 50% fan-out reduction vs exact |
| `PLN-P4` | False-negative risk in tensor routing | `false_negative_rate` ≤ 0.02 |

**Binary:** `bench_tensor_update_dispatch` (planner integration fixture)  
**Metrics:** `routing_quality`, `exact_fallback_rate`, `false_negative_rate`

---

### UPD-BE1..BE5 — CPU/GPU Break-Even for Tensor Maintenance

| Scenario | Sweep Variable | Fixed | Output |
|---|---|---|---|
| `UPD-BE1` | batch_size 1–512 | rank=16, density=100% | `gpu_breakeven_batch` |
| `UPD-BE2` | density 1%–100% | rank=16, batch=32 | Density threshold for GPU |
| `UPD-BE3` | rank 4–64 | batch=32, density=50% | `gpu_breakeven_rank` |
| `UPD-BE4` | tensor_size 1 KiB–16 MiB | — | `transfer_overhead_fraction` |
| `UPD-BE5` | window 16–4096 elements | — | `gpu_breakeven_n` for update window |

---

## Cross-Cutting Scenarios

### CRS-T1 — Host↔Device Transfer Cost vs Compute

| Field | Value |
|---|---|
| **Description** | Decomposes GPU operation latency into H2D, compute, and D2H components |
| **Binary** | `bench_cross_cutting` |
| **Backends** | `gpu-cuda`, `gpu-hip` |
| **Sweep: tensor_size** | 1 KiB, 4 KiB, 16 KiB, 64 KiB, 256 KiB, 1 MiB, 4 MiB, 16 MiB |
| **Metrics** | `h2d_latency`, `d2h_latency`, `h2d_bandwidth`, `transfer_overhead_fraction` |
| **Key output** | Transfer overhead fraction per size class → used by all GPU scenarios |

---

### CRS-T4 — Cross-Shard Transfer Amplification

| Field | Value |
|---|---|
| **Description** | Measures how cross-shard data movement amplifies with shard count |
| **Sweep: shard_count** | 1, 2, 4, 8, 16, 32 |
| **Metrics** | Total transfer volume, latency, `transfer_overhead_fraction` |
| **Expected result** | Linear amplification; CPU wins for shard_count ≤ 4 |

---

### CRS-Q1 / CRS-Q2 — Quantised Artifact Benchmarks

| Scenario | Description |
|---|---|
| `CRS-Q1` | INT8 decode + compute on CPU SIMD vs GPU path; includes recall impact |
| `CRS-Q2` | FP16 vs FP32 mixed precision: GPU tensor cores vs CPU FP32 SIMD |

---

## Scenario Status Summary

| Layer | Total Scenarios | Existing Binary Coverage | New Binary Required |
|---|---|---|---|
| ANN Frontdoor | 18 | Partial (ANN-S1..S3 CPU) | `bench_ann_cpu_gpu_dispatch` |
| Tensor Mid-Layer | 14 | Partial (TEN-S1 CPU, TEN-G1 CPU) | `bench_tensor_cpu_gpu_dispatch` |
| Graph Paths | 13 | Partial (GRP-F1..F3 CPU, GRP-N1..N3 CPU) | `bench_graph_cpu_gpu_dispatch` |
| Dynamic Updates | 14 | None | `bench_tensor_update_dispatch`, `bench_tensor_commit_overhead` |
| Cross-Cutting | 8 | None | `bench_cross_cutting` |
| **Total** | **67** | **~15 (CPU only)** | **5 new binaries** |

---

## Related Documents

- [`CPU_SIMD_GPU_DISPATCH_BENCHMARK_MATRIX.md`](CPU_SIMD_GPU_DISPATCH_BENCHMARK_MATRIX.md) — Full benchmark matrix
- [`slo_benchmark_matrix_v190.md`](slo_benchmark_matrix_v190.md) — Wave2 SLO matrix (reference for metric conventions)
- [`docs/performance/PERFORMANCE_EXPECTATIONS.md`](../performance/PERFORMANCE_EXPECTATIONS.md) — KPI definitions
- [`docs/ci-cd/gpu-benchmark-matrix-runner.md`](../ci-cd/gpu-benchmark-matrix-runner.md) — CI runner setup
- [`benchmarks/ROADMAP.md`](../../benchmarks/ROADMAP.md) — Benchmark module roadmap
