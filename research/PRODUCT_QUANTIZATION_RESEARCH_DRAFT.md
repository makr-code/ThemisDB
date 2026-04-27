# Product Quantization in ThemisDB: Compression-Accuracy-Throughput Trade-offs

**Status**: Draft  
**Version**: 0.3  
**Last Updated**: 2026-04-27  
**Target Venue**: arXiv (cs.DB / cs.IR) → VLDB 2027  
**Companion to**: `GPU_VECTOR_INDEXING_RESEARCH_DRAFT.md`, `THEMISDB_SYSTEM_PAPER_ARXIV_2026.md` §III.B

---

## Abstract

Product Quantization (PQ) is the primary compression mechanism in ThemisDB's vector
index layer. It enables retrieval over corpora that exceed GPU VRAM capacity by trading
a controlled fraction of recall for 32–192× memory reduction. This paper presents a
systematic evaluation of ThemisDB's three implemented quantization variants — Standard PQ,
Residual PQ, and Binary Quantization — across a three-dimensional parameter space
(subcode count m, corpus scale, embedding dimension), and identifies the
compression-recall-latency Pareto frontier for each variant. We define four workloads
(W-PQ-1: standard compression baseline; W-PQ-2: residual PQ accuracy; W-PQ-3: binary
maximum compression; W-PQ-4: cross-variant comparison), supply a repository-grounded
evidence registry (E1–E9), pre-register expected operating points (H1–H5), and provide
result table schemas. The study informs production tuning decisions and prioritises the
OPQ and SIMD-ADC enhancement roadmap. Empirical execution is deferred pending benchmark
runs on standardised corpora.

---

## I. Introduction

### A. Motivation

At scale, full-precision fp32 vectors dominate memory budgets. For a 10 M-document
corpus at 768 dimensions:
- fp32 storage: 10 M × 768 × 4 B = **30 GB** (GPU VRAM exhausted for most GPUs)
- IVF+PQ (m=32): 10 M × 32 B + centroids ≈ **320 MB** (96× compression)

The 96× reduction makes the difference between requiring a multi-GPU cluster and fitting
on a single 8 GB consumer GPU. However, the compression introduces ADC approximation
error. For ThemisDB's legal and compliance use cases, controlled recall degradation is
acceptable only if it remains within a pre-audited quality floor (Recall@10 ≥ 0.88 for
compliance queries; ≥ 0.82 for exploratory discovery).

ThemisDB already implements three quantization variants:
- **Standard PQ** (`product_quantizer.cpp`): Jégou et al. [1] ADC baseline.
- **Residual PQ** (`residual_quantizer.cpp`): multi-stage residual quantisation for
  higher accuracy at moderate compression.
- **Binary Quantization** (`binary_quantizer.cpp`): 1-bit encoding for maximum
  compression; suitable for pre-filtering before re-ranking.

### B. Contributions

1. A systematic three-way comparison of PQ, Residual PQ, and Binary Quantization
   across m ∈ {8, 16, 32, 64}, dim ∈ {384, 768, 1536}, corpus ∈ {1 M, 10 M, 100 M}.
2. A calibrated Recall@10–memory–latency Pareto frontier for each variant.
3. Pre-registered operating points and a production tuning policy table.
4. A prioritised OPQ and SIMD-ADC enhancement roadmap grounded in measured gaps.

---

## II. Related Work

**Product Quantization**: Jégou et al. [1] introduced PQ with asymmetric distance
computation (ADC) as the de-facto ANN compression baseline. The key insight is that
distances to centroids are pre-computed into lookup tables of size `m × k` (k=256), and
inner products reduce to m byte lookups — cache-friendly and SIMD-acceleratable.

**Residual Quantization**: Chen et al. [2] showed that multi-stage residual quantization
outperforms standard PQ for equal code lengths at the cost of higher training complexity.
`ResidualQuantizer` implements 2-stage residual quantisation.

**Optimised PQ (OPQ)**: Ge et al. [3] showed that pre-rotating embedding space to
minimise quantisation error improves Recall@k by 3–7 pp. OPQ is identified as a high-
value enhancement for ThemisDB Phase 2.

**Binary Quantization**: Faiss [4] binary quantisers (Hamming-distance based) achieve
the smallest per-vector memory footprint at the cost of the largest recall degradation.
Binary quantisation is most effective as a *pre-filter* before re-ranking with full
precision.

**SIMD-ADC acceleration**: André et al. [5] demonstrated that AVX2/AVX-512 SIMD
vectorisation of the ADC lookup step yields 3–6× throughput improvement on CPU. This is
the primary CPU-side optimisation opportunity in ThemisDB.

---

## III. System Model

### A. Quantizer Implementation Inventory

| Variant | Header | Implementation | Code Length | Stages | Training |
|---|---|---|---|---|---|
| Standard PQ | `include/index/product_quantizer.h` | `src/index/product_quantizer.cpp` | m × nbits | 1 | k-means per subspace |
| Residual PQ | `include/index/residual_quantizer.h` | `src/index/residual_quantizer.cpp` | m × nbits × stages | 2 | sequential k-means |
| Binary | `include/index/binary_quantizer.h` | `src/index/binary_quantizer.cpp` | dim bits | 1 | sign quantisation |

### B. Memory Footprint Model

```
bytes_per_vector(PQ, m, nbits)          = m × (nbits / 8)   # standard
bytes_per_vector(ResidualPQ, m, stages) = m × stages         # nbits=8 assumed
bytes_per_vector(Binary, dim)           = ceil(dim / 8)

compression_ratio(variant) = (dim × 4) / bytes_per_vector(variant)
```

Expected compression ratios:

| Variant | m | dim=768 | Bytes/Vec | Compression |
|---|---|---|---|---|
| PQ | 8 | 768 | 8 B | 384× |
| PQ | 32 | 768 | 32 B | 96× |
| PQ | 64 | 768 | 64 B | 48× |
| Residual PQ (2-stage) | 32 | 768 | 64 B | 48× |
| Binary | — | 768 | 96 B | 32× |

### C. ADC Query Path

```
Query embedding q (float[dim])
    ↓
ProductQuantizer::computeDistanceTables(q)
    → distance_tables[m][k]  (m × 256 float values)
    ↓
For each compressed code c[m]:
    score += distance_tables[i][c[i]]  for i in {0..m-1}
    ↓
Top-k merge
```

SIMD-ADC replaces the inner loop with AVX2 8-lane float gather, improving throughput
from ~300 M to ~1 000 M code-distance ops/s on Skylake Xeon (per [5]).

---

## IV. Experimental Methodology

### A. Workloads

| Workload | Quantizer | Corpus | Primary Goal |
|---|---|---|---|
| W-PQ-1 | Standard PQ (m sweep) | 1 M, 10 M, 100 M | Recall–memory–latency Pareto |
| W-PQ-2 | Residual PQ vs. PQ (same bytes) | 10 M | Accuracy gain from residual stages |
| W-PQ-3 | Binary quantisation | 1 M, 10 M | Maximum compression ceiling |
| W-PQ-4 | Cross-variant at iso-recall | 10 M | Latency comparison at Recall@10 = 0.90 |

### B. Parameter Sweep

**W-PQ-1 (Standard PQ)**:
- m ∈ {8, 16, 32, 64}
- nbits = 8 (standard)
- nprobe ∈ {16, 32, 64} (for IVF+PQ)
- dim ∈ {384, 768, 1536}
- corpus ∈ {1 M, 10 M, 100 M}
- 30 repetitions per cell

**W-PQ-2 (Residual PQ)**:
- stages ∈ {1 (= std PQ), 2, 3}
- m ∈ {16, 32}
- corpus = 10 M, dim = 768
- 30 repetitions per cell

### C. Metrics

| Metric | Definition | Production Floor |
|---|---|---|
| Recall@10 | Ground truth overlap | ≥ 0.88 (compliance) |
| Memory/vec | Bytes per compressed vector | ≤ 64 B for 100 M corpus |
| P50/P95/P99 query latency | End-to-end batch latency | P99 ≤ 50 ms (768-d, batch=64) |
| Codebook training time | Offline build cost | ≤ 600 s (10 M 768-d) |
| Recall@10 degradation vs. exact | Gap from BF ground truth | ≤ −0.10 pp at m=32 |

### D. Statistical Analysis Plan

Primary comparisons (Wilcoxon signed-rank, Bonferroni α' = 0.05/5 = 0.01):
1. Standard PQ m=32 vs. m=8 Recall@10 at 10 M 768-d (H1)
2. Residual PQ (2-stage, m=32) vs. Standard PQ (m=32) at same bytes (H2)
3. Binary vs. PQ m=8 latency at 10 M (H3)
4. Standard PQ m=32 vs. m=64 latency (H4, expected: within 5%)
5. Cross-corpus scaling of Recall@10 degradation m=32 (H5)

---

## V. Pre-Registered Operating Points

| Hypothesis | Expected Outcome | Pass Criterion |
|---|---|---|
| H1: PQ m=32 vs. m=8 Recall@10 (10 M, 768-d) | m=32 ≥ m=8 + 0.06 | Difference ≥ 0.04 (p < 0.01) |
| H2: Residual PQ vs. PQ (same bytes) Recall@10 | Residual ≥ PQ + 0.03 | Difference ≥ 0.02 (p < 0.01) |
| H3: Binary vs. PQ m=8 P99 latency | Binary ≤ PQ × 0.5 | Binary P99 ≤ PQ P99 × 0.6 |
| H4: PQ m=32 vs. m=64 latency overhead | m=64 ≤ m=32 × 1.05 | Overhead ≤ 10% |
| H5: Recall@10 degradation m=32 across 1M/10M/100M | < 3 pp variance | σ(Recall) ≤ 0.03 |

---

## VI. Implementation Evidence

| ID | File | Scope | Claim |
|----|------|-------|-------|
| E1 | `include/index/product_quantizer.h` | Standard PQ API | PQ training + encode/search interfaces defined |
| E2 | `src/index/product_quantizer.cpp` | Standard PQ impl | Codebook training + ADC implemented |
| E3 | `include/index/residual_quantizer.h` | Residual PQ API | Multi-stage quantisation interface defined |
| E4 | `src/index/residual_quantizer.cpp` | Residual PQ impl | 2-stage residual quantisation implemented |
| E5 | `include/index/binary_quantizer.h` | Binary API | Binary encoding interface defined |
| E6 | `src/index/binary_quantizer.cpp` | Binary impl | Sign quantisation + Hamming search |
| E7 | `tests/test_product_quantizer.cpp` | PQ tests | Correctness test coverage exists |
| E8 | `tests/test_residual_quantizer.cpp` | Residual tests | Residual quantisation test coverage exists |
| E9 | `benchmarks/bench_product_quantization.cpp` | PQ benchmark | Performance measurement harness exists |

---

## VII. Results Schema (Pre-defined)

### Table PQ-1: Recall@10 × m × Corpus (Standard PQ, dim=768)

| m | 1 M | 10 M | 100 M |
|---|---|---|---|
| 8 | *pending* | *pending* | *pending* |
| 16 | *pending* | *pending* | *pending* |
| 32 | *pending* | *pending* | *pending* |
| 64 | *pending* | *pending* | *pending* |
| Exact BF | 1.00 | 1.00 | 1.00 |

### Table PQ-2: Memory–Recall Pareto (m=32, dim=768, 10 M corpus)

| Variant | Bytes/Vec | VRAM (10 M) | Recall@10 | P99 (ms, batch=64) |
|---|---|---|---|---|
| fp32 exact | 3 072 B | 30 GB | 1.00 | *pending* |
| Binary | 96 B | 960 MB | *pending* | *pending* |
| PQ (m=8) | 8 B | 80 MB | *pending* | *pending* |
| PQ (m=32) | 32 B | 320 MB | *pending* | *pending* |
| PQ (m=64) | 64 B | 640 MB | *pending* | *pending* |
| ResidualPQ (m=32, s=2) | 64 B | 640 MB | *pending* | *pending* |

### Table PQ-3: Recall@10 × nprobe (Standard PQ m=32, 100 M, dim=768)

| nprobe | Recall@10 | P99 (ms) | Throughput (qps) |
|---|---|---|---|
| 8 | *pending* | *pending* | *pending* |
| 16 | *pending* | *pending* | *pending* |
| 32 | *pending* | *pending* | *pending* |
| 64 | *pending* | *pending* | *pending* |

---

## VIII. Discussion

### A. Production Tuning Policy

Based on H1–H5 pre-registered ranges and expected Pareto:

| Use Case | Recall Floor | Recommended Variant | Notes |
|---|---|---|---|
| Compliance search (legal documents) | ≥ 0.88 | PQ m=32, nprobe=32 | Baseline for all compliance queries |
| Exploratory discovery | ≥ 0.82 | PQ m=16, nprobe=16 | Lower recall acceptable; faster |
| Pre-filter for re-ranking | ≥ 0.70 | Binary | Binary as candidate selector before exact re-rank |
| High-precision citation | ≥ 0.95 | ResidualPQ (2-stage, m=32) | Residual PQ for legal precedent lookup |

### B. OPQ Enhancement Roadmap

Standard PQ quantises subspaces independently, ignoring cross-subspace correlations.
OPQ learns a rotation matrix R that minimises quantisation distortion:

```
OPQ objective: min_R ||X - PQ(XR)||²   subject to R ∈ O(d)
```

Expected benefit from [3]: +3–7 pp Recall@10 at the same memory footprint. Estimated
implementation effort: ~2 weeks (rotation training loop + pre-encoding transform).
Priority: **High** — justified if compliance queries fail the ≥ 0.88 floor at m=32.

### C. SIMD-ADC Implementation Roadmap

Current `product_quantizer.cpp` ADC loop is scalar. AVX2 vectorisation yields an
expected 3–6× throughput improvement (per [5]) with ~1 week implementation effort.
This directly reduces IVF+PQ query latency, contributing to the P99 ≤ 50 ms target.

### D. Threats to Validity

**Internal validity**: codebook training quality depends on training set size (default:
100 000 samples). Under-training leads to imbalanced centroids and poorer recall.
Mitigation: re-train codebooks if centroid imbalance (max/avg cluster size) > 4×.

**Construct validity**: Recall@10 against brute-force ground truth on the *training
distribution* may not predict recall on distribution-shifted queries. We include a
separate distribution-shift test corpus.

---

## IX. Reproducibility & Artifact

```bash
# Build (PQ benchmarks enabled by default)
cmake --preset linux-release
cmake --build --preset linux-release

# W-PQ-1: Standard PQ m sweep
./build/linux-release/benchmarks/bench_product_quantization \
  --mode standard --m 8,16,32,64 --nbits 8 \
  --corpus 1m,10m,100m --dim 384,768,1536 \
  --nprobe 16,32,64 --reps 30 \
  --output artifacts/pq/standard/

# W-PQ-2: Residual PQ vs. Standard PQ
./build/linux-release/benchmarks/bench_product_quantization \
  --mode residual --stages 1,2,3 --m 16,32 \
  --corpus 10m --dim 768 --reps 30 \
  --output artifacts/pq/residual/

# W-PQ-3: Binary quantization
./build/linux-release/benchmarks/bench_product_quantization \
  --mode binary --corpus 1m,10m --dim 384,768 \
  --reps 30 --output artifacts/pq/binary/

# Analysis
python scripts/analyze_pq.py artifacts/pq/
```

**Expected runtime**: W-PQ-1 ≈ 90 min; W-PQ-2 ≈ 20 min; W-PQ-3 ≈ 15 min.

---

## X. Limitations, Risk, Ethics

- **Recall measurement validity**: ground truth must be computed over the same embedding
  set with exact brute-force; ground-truth corpus changes invalidate prior measurements.
- **Quantiser transferability**: codebooks trained on one corpus domain (e.g.,
  NaturalQuestions) may underperform on domain-shifted corpora (e.g., German legal).
  Domain-specific codebook training is recommended for production.
- **Dataset bias**: compression quality metrics can be overstated on synthetic
  embeddings with uniform cluster structure. All results must be validated on
  corpus-derived embeddings.

---

## XI. Conclusion

ThemisDB provides a strong PQ baseline with residual and binary quantization extensions.
This paper specifies the systematic evaluation needed to identify the
compression-recall-latency Pareto frontier, derive production tuning policies, and
prioritise the OPQ and SIMD-ADC enhancement roadmap. Pre-registered hypotheses H1–H5
allow falsifiable evaluation. Tables PQ-1 through PQ-3 will be populated upon benchmark
execution and this paper upgraded to v0.4.

---

## References

[1] Jégou, H., Douze, M., & Schmid, C. (2011). Product Quantization for Nearest Neighbor
Search. *IEEE Transactions on Pattern Analysis and Machine Intelligence, 33*(1), 117–128.

[2] Chen, J., & Wang, J. (2010). Approximate nearest neighbor search by residual vector
quantization. *Sensors, 10*(12), 11259–11273.

[3] Ge, T., He, K., Ke, Q., & Sun, J. (2013). Optimized Product Quantization.
*IEEE Transactions on Pattern Analysis and Machine Intelligence, 36*(4), 744–755.

[4] Johnson, J., Douze, M., & Jégou, H. (2021). Billion-scale similarity search with
GPUs. *IEEE Transactions on Big Data, 7*(3), 535–547.

[5] André, F., Kermarrec, A.-M., & Le Scouarnec, N. (2015). Cache locality is not enough:
High-Performance Nearest Neighbor Search with Product Quantization Fast Scan.
*PVLDB, 9*(4), 288–299.

---

## Appendix A. Submission Readiness Checklist

- [x] Research questions and contributions stated (§I.B)
- [x] Quantizer inventory and memory model (§III.A–B)
- [x] Four workloads W-PQ-1..4 (§IV.A)
- [x] Parameter sweep defined (§IV.B)
- [x] H1–H5 pre-registered with pass criteria (§V)
- [x] Evidence registry E1–E9 (§VI)
- [x] Result table schemas PQ-1..3 (§VII)
- [x] Production tuning policy (§VIII.A)
- [x] OPQ and SIMD-ADC roadmap (§VIII.B–C)
- [x] Reproducibility commands (§IX)
- [ ] Benchmark execution
- [ ] Tables PQ-1..3 populated
- [ ] OPQ comparison added (Phase 2)
