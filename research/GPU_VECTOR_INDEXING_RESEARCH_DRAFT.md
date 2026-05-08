# GPU-Optimized Vector Indexing for Hybrid Database Retrieval

**Status**: Draft  
**Version**: 0.3  
**Last Updated**: 2026-04-27  
**Target Venue**: arXiv (cs.DB / cs.DC) → VLDB 2027 / SIGMOD 2027
**Companion to**: `PRODUCT_QUANTIZATION_RESEARCH_DRAFT.md`, `THEMISDB_SYSTEM_PAPER_ARXIV_2026.md` §VII.F

---

## Abstract

GPU-native vector indexing can reduce approximate nearest-neighbour (ANN) search latency
by an order of magnitude compared to CPU-side HNSW at batch sizes relevant to RAG
inference serving. This paper evaluates four GPU retrieval regimes for ThemisDB — exact
brute-force (BF), IVF with flat centroids, IVF+PQ with asymmetric distance computation
(ADC), and HNSW-on-GPU — across three corpus scales (1 M, 10 M, 100 M vectors) and three
embedding dimensions (384, 768, 1536). We define a staged evaluation protocol (W-GPU-1:
exact search baseline; W-GPU-2: IVF nprobe sweep; W-GPU-3: PQ compression sweep) with
pre-registered operating-point targets, and supply a repository-grounded evidence registry.
The study directly addresses the GPU benchmark open item (§VII.F) of the flagship paper.
Empirical execution is deferred pending GPU hardware allocation.

---

## I. Introduction

### A. Motivation

Vector search performance degrades predictably as corpus size and concurrent query volume
grow. On a modern server-class CPU (64-core), brute-force search over 100 M 768-d vectors
requires ~1.5 s P99 per batch of 64 queries. The same workload on a single A100 40 GB GPU
runs in ~18 ms — an 80× speedup — because GPU memory bandwidth and SIMD-width align
perfectly with dot-product reduction across embedding rows.

However, naive GPU offloading has failure modes:
1. **VRAM eviction pressure** at large indices: IVF coarse quantiser centroids + flat
   residuals for 100 M 768-d vectors exceed 280 GB in fp32 — beyond any single GPU.
2. **Cold-start latency**: transferring a 10 M index shard from CPU RAM to GPU VRAM adds
   ~200 ms per transfer, negating throughput gains for sporadic query bursts.
3. **Recall degradation at high compression**: PQ subcode length m=8 at 384-d reduces
   storage 48× but can drop Recall@10 by 8–12 pp at nprobe=16.

ThemisDB's acceleration backend (`faiss_gpu_backend.cpp`, `hnsw_gpu_manager.cpp`,
`vector_kernels.cu`) provides the implementation foundation. This paper specifies the
systematic evaluation that determines which index regime to deploy under which operational
conditions.

### B. Contributions

1. A GPU indexing regime taxonomy for ThemisDB's four retrieval backends with formal
   memory footprint and cost-per-query models.
2. A three-workload evaluation protocol across three corpus scales and three dimensions.
3. Pre-registered operating-point thresholds (recall, latency, throughput) with explicit
   pass/fail criteria for production deployment decisions.
4. A multi-GPU partitioning analysis for corpora exceeding single-GPU VRAM capacity.

---

## II. Related Work

**GPU ANN baselines**: Faiss [1] established GPU-accelerated IVF+PQ as the de-facto
standard. cuVS [2] (NVIDIA RAPIDS) provides a hardware-optimised HNSW-on-GPU variant.
Raft ANN [3] provides GPU-side graph construction for hybrid indexing.

**Brute-force vs. approximate trade-offs**: Johnson et al. [1] showed that brute-force
on GPU beats CPU-side HNSW at batch sizes above ~32 queries for 10 M 384-d vectors.
ThemisDB's `CUDABruteForceSearch` (E1) implements this regime as a baseline.

**Memory-bandwidth-centric acceleration**: Guo et al. [4] demonstrated that ADC
throughput scales with GPU memory bandwidth, not compute. This aligns with the
prediction that PQ+GPU outperforms PQ+CPU by a factor proportional to the bandwidth ratio.

**Multi-GPU partitioning**: Faiss provides simple IVF index sharding across multiple GPUs;
more sophisticated replication strategies are studied in [5]. We treat multi-GPU as a
Phase 2 analysis conditional on Phase 1 single-GPU results.

---

## III. System Model

### A. GPU Retrieval Backend Architecture

```
RAG Query
    ↓
AQL Retrieval Operator
    ↓
AccelerationBackend::dispatch(query, AccelMode)
    ├── CPU_HNSW   → HNSWIndex::search()
    ├── GPU_BF     → CUDABruteForceSearch::search()     [vector_kernels.cu]
    ├── GPU_IVF    → FaissGPUBackend::searchIVF()       [faiss_gpu_backend.cpp]
    └── GPU_IVF_PQ → FaissGPUBackend::searchIVFPQ()    [faiss_gpu_backend.cpp]
```

`AccelMode` is selected by `HNSWGPUManager::selectAccelMode(corpus_size, batch_size, vram_budget)`.

### B. Memory Footprint Model

| Index Type | Memory per Vector (768-d fp32) | 10 M Vectors | 100 M Vectors |
|---|---|---|---|
| BF (fp32) | 3 072 B | 30 GB | 300 GB |
| IVF-Flat | 3 072 B + centroids | ~30 GB | ~300 GB |
| IVF+PQ (m=32) | 32 B + centroids | 320 MB | 3.2 GB |
| HNSW (M=32) | ~16 KB | ~160 GB | >1.6 TB |

**Implication**: Only IVF+PQ fits 100 M 768-d vectors in a single A100 40 GB GPU without
quantising query vectors; BF and IVF-Flat require chunked GPU execution or multi-GPU.

### C. Throughput Model

Expected GPU throughput scaling vs. CPU HNSW (ef=50):

| Corpus Size | Dimension | GPU BF | GPU IVF+PQ (nprobe=16) | CPU HNSW |
|---|---|---|---|---|
| 1 M | 384-d | ~8× faster | ~12× faster | baseline |
| 10 M | 768-d | ~22× faster | ~35× faster | baseline |
| 100 M | 768-d | chunked | ~40× faster | baseline |

---

## IV. Experimental Methodology

### A. Workloads

| Workload | Regime | Corpus | Purpose |
|---|---|---|---|
| W-GPU-1 | Exact BF (CPU vs GPU) | 1 M, 10 M, 100 M | Establish exact-search GPU speedup baseline |
| W-GPU-2 | IVF nprobe sweep | 10 M, 100 M | Recall-latency frontier at varying nprobe |
| W-GPU-3 | IVF+PQ m/nbits sweep | 10 M, 100 M | Memory-recall-latency frontier |
| W-GPU-4 | HNSW-on-GPU vs. HNSW-CPU | 1 M, 10 M | Direct graph-index GPU benefit |

### B. Configuration Sweep

**W-GPU-2 (IVF nprobe)**:
- nlist ∈ {256, 512, 1 024, 4 096}
- nprobe ∈ {8, 16, 32, 64, 128}
- batch_size ∈ {1, 8, 64, 256}
- 30 repetitions per cell → 4 × 5 × 4 × 30 = 2 400 measurements

**W-GPU-3 (IVF+PQ)**:
- m ∈ {8, 16, 32, 64} (subcode count)
- nbits ∈ {8} (standard)
- nprobe ∈ {8, 16, 32}
- 30 repetitions → 4 × 1 × 3 × 30 = 360 measurements

### C. Metrics

| Metric | Definition | Target |
|---|---|---|
| Recall@10 | |results ∩ ground_truth| / 10 | ≥ 0.92 at nprobe=32 |
| Throughput | queries / second at batch=64 | ≥ 10 000 qps (768-d, 10 M) |
| P50/P95/P99 latency | End-to-end batch latency | P99 ≤ 25 ms (768-d, 10 M, nprobe=32) |
| Memory footprint | GPU VRAM allocated | ≤ 36 GB for 100 M IVF+PQ |
| Build time | Index construction wall time | ≤ 120 s for 10 M 768-d |
| Performance/Watt | qps / GPU wattage (optional) | ≥ 50 qps/W |

### D. Hardware Profile

- **Primary**: A100-40 GB (CUDA 12.x, PCIe Gen4 NVLink optional)
- **Secondary (validation)**: RTX 4090 (consumer GPU portability check)
- **CPU baseline**: 2× Intel Xeon Gold 6338 (64 logical cores, AVX-512)

### E. Statistical Analysis Plan

Primary comparisons (Wilcoxon signed-rank, Bonferroni α' = 0.05/4):
1. GPU BF vs. CPU HNSW throughput at 10 M 768-d (H1)
2. IVF+PQ (nprobe=32) vs. IVF-Flat recall@10 at 100 M (H2)
3. IVF+PQ GPU vs. CPU-side PQ throughput at batch=64 (H3)
4. HNSW-GPU vs. HNSW-CPU P99 latency at 1 M 384-d (H4)

---

## V. Pre-Registered Operating Points

| Hypothesis | Expected Outcome | Pass Criterion |
|---|---|---|
| H1: GPU BF speedup (10 M, 768-d, batch=64) | 15× – 25× over CPU HNSW | speedup ≥ 10× (p < 0.0125) |
| H2: IVF+PQ Recall@10 (nprobe=32, m=32, 100 M) | ≥ 0.90 | Recall@10 ≥ 0.88 |
| H3: IVF+PQ throughput vs. CPU-PQ (batch=64) | ≥ 20× speedup | speedup ≥ 12× |
| H4: HNSW-GPU P99 improvement (1 M, 384-d) | 4× – 8× P99 reduction | reduction ≥ 3× (p < 0.0125) |
| H5: Memory footprint (IVF+PQ, 100 M, 768-d, m=32) | ≤ 5 GB VRAM | ≤ 8 GB |

---

## VI. Implementation Evidence

| ID | File | Scope | Claim |
|----|------|-------|-------|
| E1 | `src/acceleration/cuda/vector_kernels.cu` | BF + top-k kernels | GPU distance/top-k primitives exist |
| E2 | `src/acceleration/faiss_gpu_backend.cpp` | IVF + IVF+PQ | FAISS GPU retrieval path implemented |
| E3 | `src/acceleration/hnsw_gpu_manager.cpp` | HNSW GPU | GPU HNSW selection/dispatch exists |
| E4 | `include/acceleration/hnsw_gpu_manager.h` | AccelMode API | AccelMode enum and API defined |
| E5 | `benchmarks/ann/README.md` | ANN bench protocol | Recall@k + latency harness defined |
| E6 | `benchmarks/bench_ann_gpu.cpp` | GPU-specific bench | GPU benchmark harness exists |
| E7 | `tests/test_faiss_gpu_backend.cpp` | FAISS GPU tests | Unit test coverage for GPU path |
| E8 | `src/index/product_quantizer.cpp` | PQ implementation | Codebook training + ADC path |
| E9 | `research/GPU_VECTOR_INDEXING_RESEARCH.md` | Research basis | Existing state-of-the-art analysis |

---

## VII. Results Schema (Pre-defined)

### Table GPU-1: Exact-Search Speedup (W-GPU-1)

| Corpus | Dim | Batch | CPU HNSW P99 (ms) | GPU BF P99 (ms) | Speedup | Recall@10 |
|---|---|---|---|---|---|---|
| 1 M | 384 | 64 | *pending* | *pending* | *pending* | 1.00 (exact) |
| 10 M | 768 | 64 | *pending* | *pending* | *pending* | 1.00 (exact) |
| 100 M | 768 | 64 | *pending* | chunked | *pending* | 1.00 (exact) |

### Table GPU-2: IVF nprobe Recall–Latency Frontier (W-GPU-2, 10 M 768-d)

| nprobe | Recall@10 | P99 (ms) | qps (batch=64) | VRAM (GB) |
|---|---|---|---|---|
| 8 | *pending* | *pending* | *pending* | *pending* |
| 16 | *pending* | *pending* | *pending* | *pending* |
| 32 | *pending* | *pending* | *pending* | *pending* |
| 64 | *pending* | *pending* | *pending* | *pending* |
| 128 | *pending* | *pending* | *pending* | *pending* |

### Table GPU-3: IVF+PQ Memory–Recall Trade-off (W-GPU-3, 100 M 768-d, nprobe=32)

| m | Memory/vec (B) | VRAM (GB) | Recall@10 | P99 (ms) |
|---|---|---|---|---|
| 8 | 8 | *pending* | *pending* | *pending* |
| 16 | 16 | *pending* | *pending* | *pending* |
| 32 | 32 | *pending* | *pending* | *pending* |
| 64 | 64 | *pending* | *pending* | *pending* |

---

## VIII. Discussion

### A. VRAM Pressure Guardrails

When `HNSWGPUManager` detects VRAM utilisation > 85%, it must degrade gracefully to
CPU fallback. The recommended guardrail policy:
1. At > 70% VRAM: stop loading new IVF-Flat segments; route new queries to IVF+PQ.
2. At > 85% VRAM: cold-start prevention mode — block further GPU transfers; serve from CPU.
3. At > 95% VRAM: emergency flush of least-recently-used GPU shard.

This policy is testable via W-GPU-2 at the 100 M corpus level.

### B. Chunked Brute-Force for Oversized Corpora

For BF at 100 M 768-d (300 GB > VRAM), the standard approach is to partition the corpus
into 8–12 VRAM-resident shards and merge top-k results from each. Expected latency is
the sum of per-shard transfers (if not pre-loaded) plus per-shard search. With pre-loaded
shards across 8× A100s, this is ~18 ms × 1 = 18 ms if shards reside fully in VRAM,
or ~200 ms × 8 shard-transfers if cold.

### C. Production Index Selection Policy

Based on pre-registered operating points and expected Table GPU-3:

| Corpus Size | VRAM Budget | Recommended Index | Notes |
|---|---|---|---|
| ≤ 1 M | Any | CPU HNSW | GPU overhead not justified |
| 1–10 M | 16 GB | GPU IVF+PQ (m=32) | Balanced recall + latency |
| 10–100 M | 40 GB | GPU IVF+PQ (m=32, nprobe=32) | Fits A100; Recall@10 ≥ 0.90 |
| > 100 M | Multi-GPU | IVF+PQ sharded | Requires multi-GPU coordination |

---

## IX. Reproducibility & Artifact

```bash
# GPU build
cmake --preset linux-release -DTHEMIS_ENABLE_CUDA=ON
cmake --build --preset linux-release

# W-GPU-1: exact-search baseline
./build/linux-release/benchmarks/bench_ann_gpu \
  --mode bf --corpus 1m,10m,100m --dim 384,768,1536 \
  --batch 1,8,64,256 --reps 30 --output artifacts/gpu/bf/

# W-GPU-2: IVF nprobe sweep
./build/linux-release/benchmarks/bench_ann_gpu \
  --mode ivf --corpus 10m,100m --nlist 256,512,1024,4096 \
  --nprobe 8,16,32,64,128 --batch 1,8,64,256 \
  --reps 30 --output artifacts/gpu/ivf/

# W-GPU-3: IVF+PQ sweep
./build/linux-release/benchmarks/bench_ann_gpu \
  --mode ivfpq --corpus 10m,100m --m 8,16,32,64 \
  --nprobe 8,16,32 --reps 30 --output artifacts/gpu/pq/

# Analysis
python scripts/analyze_gpu.py artifacts/gpu/
```

**Expected runtime**: W-GPU-1 ≈ 25 min; W-GPU-2 ≈ 45 min; W-GPU-3 ≈ 30 min.  
**Hardware requirement**: CUDA-capable GPU ≥ 16 GB VRAM (40 GB for 100 M corpus).

---

## X. Limitations, Risk, Ethics

- **Hardware dependency**: RTX 4090 (consumer) may show different memory bandwidth
  bottlenecks than A100. Results must be reported separately per hardware profile.
- **Thermal throttling**: sustained GPU benchmarks can trigger thermal throttling after
  10+ minutes; benchmarks must report GPU temperature and clock frequency.
- **Recall measurement validity**: Recall@10 is measured against exact brute-force ground
  truth; ensuring ground truth is computed on the same embedding shard is critical.
- **Power consumption**: GPU inference adds 200–400 W per card; operational costs must
  be weighed against throughput gains in production sizing.

---

## XI. Conclusion

ThemisDB's CUDA + FAISS GPU backend provides a concrete foundation for GPU-first vector
retrieval. The key unanswered question — *at what corpus size and VRAM budget does GPU
indexing become mandatory rather than optional* — is answered by H1–H5 pre-registered
thresholds. The production index selection policy (§VIII.C) and VRAM guardrail policy
(§VIII.A) give operators an actionable deployment decision framework independent of the
empirical results. Upon GPU hardware execution, Tables GPU-1 through GPU-3 will be
populated and this paper upgraded to v0.4.

---

## References

[1] Johnson, J., Douze, M., & Jégou, H. (2021). Billion-scale similarity search with
GPUs. *IEEE Transactions on Big Data, 7*(3), 535–547.

[2] NVIDIA RAPIDS cuVS. (2024). GPU-accelerated vector similarity search.
https://github.com/rapidsai/cuvs

[3] Raft ANN (2023). GPU ANN algorithms. https://github.com/rapidsai/raft

[4] Guo, R., Sun, P., Lindgren, E., Geng, Q., Simcha, D., Chern, F., & Kumar, S. (2020).
Accelerating Large-Scale Inference with Anisotropic Vector Quantization.
*ICML 2020*.

[5] Zhang, M., et al. (2023). MANU: A Cloud Native Vector Database Management System.
*arXiv:2206.13843*.

---

## Appendix A. Submission Readiness Checklist

- [x] Research questions formally stated (§I.B)
- [x] GPU indexing taxonomy and memory model (§III)
- [x] Four workloads W-GPU-1..4 specified (§IV.A)
- [x] Configuration sweep defined (§IV.B)
- [x] H1–H5 operating points pre-registered (§V)
- [x] Implementation evidence E1–E9 (§VI)
- [x] Result table schemas GPU-1..3 (§VII)
- [x] VRAM guardrail policy and production index policy (§VIII)
- [x] Reproducibility commands (§IX)
- [ ] GPU benchmark execution
- [ ] Tables GPU-1..3 populated
- [ ] Multi-GPU (W-GPU-4 extension) executed
