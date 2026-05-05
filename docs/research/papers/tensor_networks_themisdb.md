# Tensor Networks in ThemisDB

**Metadaten:**
- Author(en): Multiple (Oseledets, Holtz, Bigoni, Dettmers, Yadav, Stoudenmire, Khoromskij)
- Konferenz/Journal: SIAM J. Sci. Comput., NeurIPS, Constructive Approximation
- Jahr: 2011–2023
- Status: [x] Partially Implemented (Phase 1 complete, Phases 2–4 planned Q4 2026 – Q2 2027)

---

## 📋 Overview

This document collects all scientific papers that form the basis of ThemisDB's
**Tensor-Native Storage Engine** (Phase 8 of `src/storage/`) and the
**Tensor Algebra Query Engine** (Phase 9 of `src/query/`).  Each paper entry
includes its DOI, a BibTeX citation, and a direct mapping to the ThemisDB
component it influences.

---

## 📚 Paper Index

| # | Paper | Year | Key Concept | ThemisDB Component |
|---|-------|------|-------------|-------------------|
| P1 | Oseledets — TT Decomposition | 2011 | TT-SVD algorithm | `TensorTrainDecomposer` |
| P2 | Khoromskij — Quantics-TT | 2011 | QTT quantised format | `TTQuantizer` |
| P3 | Holtz et al. — ALS in TT-format | 2012 | In-domain algebra | `TensorContractionEngine` |
| P4 | Bigoni et al. — Spectral TT | 2016 | Operator compression | `TensorContractionEngine` |
| P5 | Dettmers et al. — QLoRA / NF4 | 2023 | NF4 quantisation | `TTQuantizer` |
| P6 | Yadav et al. — TIES-Merging | 2023 | Cross-tensor dedup | `TensorDeduplicationManager` |
| P7 | Stoudenmire & Schwab — TN for ML | 2016 | Structure-aware storage | `TensorFingerprintGraph` |
| P8 | Roberts et al. — TensorNetwork lib | 2019 | Contraction order | `TensorContractionEngine` |
| P9 | Rajaraman & Ullman — LSH | 2011 | LSH bucketing | `TensorFingerprintGraph` |

---

## P1: Oseledets (2011) — TT-SVD

**Full Title:** Tensor-Train Decomposition  
**DOI:** https://doi.org/10.1137/090752142  
**Journal:** SIAM Journal on Scientific Computing, 33(5), 2295–2317

### BibTeX
```bibtex
@article{oseledets2011,
  author  = {Oseledets, Ivan V.},
  title   = {Tensor-Train Decomposition},
  journal = {SIAM Journal on Scientific Computing},
  volume  = {33},
  number  = {5},
  pages   = {2295--2317},
  year    = {2011},
  doi     = {10.1137/090752142}
}
```

### Key Findings for ThemisDB
- **Algorithm 1 (TT-SVD):** Iterative unfolding + truncated SVD produces a
  TT-chain with guaranteed error bound: `‖T - T_approx‖_F ≤ ε · ‖T‖_F`.
- **Algorithm 2 (TT-Rounding):** Recompression via right-to-left QR +
  left-to-right SVD, used after TT algebraic operations to prevent rank growth.
- **Storage complexity:** O(d · n · r²) vs O(n^d) for dense tensors.

### ThemisDB Mapping
- `include/storage/tensor_train_decomposer.h` — `TensorTrainDecomposer::decompose()`
  implements Algorithm 1 with per-step threshold `δ = ε / √(d-1) · ‖T‖_F`.
- `TensorTrainDecomposer::round()` implements Algorithm 2.
- **Key schema:** `__ttn__:<tenant>:<collection>:<field>:G<k>:<version>` stores
  each core Gk separately in RocksDB.

### Accepted Deviations
- The internal SVD uses Golub-Reinsch bidiagonalisation with 30 QR iterations
  instead of LAPACK `dgesdd`.  For ill-conditioned matrices, convergence may be
  limited (see STUB_INVENTORY.md, `tensor_train_decomposer.cpp`).
- LAPACK integration planned for Q3 2026 via `THEMIS_USE_LAPACK_SVD=ON`.

---

## P2: Khoromskij (2011) — Quantics-TT (QTT)

**Full Title:** O(d log n)-quantics approximation of n^d tensors in high-dimensional numerical modeling  
**DOI:** https://doi.org/10.1007/s00365-011-9131-1  
**Journal:** Constructive Approximation, 34(2), 257–280

### BibTeX
```bibtex
@article{khoromskij2011,
  author  = {Khoromskij, Boris N.},
  title   = {{$O(d \log n)$}-Quantics Approximation of {$n^d$} Tensors in High-Dimensional Numerical Modeling},
  journal = {Constructive Approximation},
  volume  = {34},
  number  = {2},
  pages   = {257--280},
  year    = {2011},
  doi     = {10.1007/s00365-011-9131-1}
}
```

### Key Findings for ThemisDB
- Quantics-TT (QTT) achieves O(d log n) storage for smooth functions on
  n^d grids, relevant for LLM attention matrices and embedding tables.
- Provides theoretical motivation for `TTQuantizer`: quantisation of TT-core
  elements is equivalent to a secondary quantics-like encoding step.

### ThemisDB Mapping
- `include/storage/tt_quantizer.h` — `TTQuantizer` (INT8, NF4 modes).
- The NF4 lookup table uses the quantile-derived fixed-point set from Dettmers
  2023 (P5) rather than QTT grid points, as NF4 is better suited for
  normally-distributed LLM weights.

---

## P3: Holtz, Rohwedder, Schneider (2012) — ALS in TT-Format

**Full Title:** The alternating linear scheme for tensor optimization in the Tensor-Train format  
**DOI:** https://doi.org/10.1137/100818893  
**Journal:** SIAM Journal on Scientific Computing, 34(2), A683–A713

### BibTeX
```bibtex
@article{holtz2012,
  author  = {Holtz, Sebastian and Rohwedder, Thorsten and Schneider, Reinhold},
  title   = {The Alternating Linear Scheme for Tensor Optimization in the Tensor-Train Format},
  journal = {SIAM Journal on Scientific Computing},
  volume  = {34},
  number  = {2},
  pages   = {A683--A713},
  year    = {2012},
  doi     = {10.1137/100818893}
}
```

### Key Findings for ThemisDB
- Section 2.3: Inner products and Frobenius norms of TT-trains can be computed
  via the **transfer-matrix** (zipper) algorithm with complexity O(d · n · r³).
- This enables "Computing in the Compressed Domain" — AQL tensor operations
  without decompression.

### ThemisDB Mapping
- `include/query/tensor_contraction_engine.h` — `TensorContractionEngine`:
  - `innerProduct()` uses the transfer-matrix algorithm (Holtz 2012, Eq. 2.6).
  - `frobeniusNorm()` = sqrt(innerProduct(T, T)).
  - `cosineSimilarity()` = innerProduct(A, B) / (‖A‖ · ‖B‖).
- AQL function `TENSOR_SIMILARITY` directly delegates to `cosineSimilarity()`.

---

## P4: Bigoni, Engsig-Karup, Marzouk (2016) — Spectral TT

**Full Title:** Spectral tensor-train decomposition  
**DOI:** https://doi.org/10.1137/15M1036881  
**Journal:** SIAM Journal on Scientific Computing, 38(4), A2405–A2439

### BibTeX
```bibtex
@article{bigoni2016,
  author  = {Bigoni, Daniele and Engsig-Karup, Allan P. and Marzouk, Youssef M.},
  title   = {Spectral Tensor-Train Decomposition},
  journal = {SIAM Journal on Scientific Computing},
  volume  = {38},
  number  = {4},
  pages   = {A2405--A2439},
  year    = {2016},
  doi     = {10.1137/15M1036881}
}
```

### Key Findings for ThemisDB
- Motivates operator compression: mathematical operators (Laplacian, Fourier,
  Maxwell field operators) can be expressed as low-rank TT-trains, allowing
  their application directly in the compressed domain.
- For ThemisDB: similarity search and aggregation queries can be treated as
  operator contractions, bypassing the I/O bottleneck of decompression.

### ThemisDB Mapping
- `TensorContractionEngine::hadamardProduct()` — element-wise operator
  application via Kronecker product of cores.
- Future: `TENSOR_APPLY_OPERATOR(field, operator_tt)` AQL function (Phase 3, Q1 2027).

---

## P5: Dettmers, Pagnoni, Holtzman, Zettlemoyer (2023) — QLoRA

**Full Title:** QLoRA: Efficient Finetuning of Quantized LLMs  
**DOI:** https://doi.org/10.48550/arXiv.2305.14314  
**Venue:** NeurIPS 2023

### BibTeX
```bibtex
@inproceedings{dettmers2023qlora,
  author    = {Dettmers, Tim and Pagnoni, Artidoro and Holtzman, Ari and Zettlemoyer, Luke},
  title     = {{QLoRA}: Efficient Finetuning of Quantized {LLM}s},
  booktitle = {Advances in Neural Information Processing Systems},
  year      = {2023},
  eprint    = {2305.14314},
  archivePrefix = {arXiv}
}
```

### Key Findings for ThemisDB
- **NF4 (Normal Float 4-bit):** 16-level lookup table derived from quantiles of
  N(0,1).  Optimal for weight matrices with approximately normal distributions
  (LLM attention, FFN weights).  Claims ≈8× compression vs float32.
- **Block-wise quantisation:** Per-block scaling prevents quantisation error
  accumulation.  ThemisDB maps "block" to "TT-core".

### ThemisDB Mapping
- `TTQuantizer::kNF4Table[16]` — Dettmers Table 1, reproduced exactly.
- `TTQuantizer::quantizeNF4()` — per-core scaling + NF4 index lookup.
- `TTQuantizer::dequantizeNF4()` — inverse lookup + scale/mean correction.
- **Target:** ≥8× compression for attention weight tensors stored in
  `TensorNetworkStorageEngine` with `QuantizationType::NF4`.

---

## P6: Yadav et al. (2023) — TIES-Merging

**Full Title:** TIES-Merging: Resolving Interference When Merging Models  
**DOI:** https://doi.org/10.48550/arXiv.2306.01708  
**Venue:** NeurIPS 2023

### BibTeX
```bibtex
@inproceedings{yadav2023ties,
  author    = {Yadav, Prateek and Tam, Derek and Choshen, Leshem and Raffel, Colin and Bansal, Mohit},
  title     = {{TIES-Merging}: Resolving Interference When Merging Models},
  booktitle = {Advances in Neural Information Processing Systems},
  year      = {2023},
  eprint    = {2306.01708},
  archivePrefix = {arXiv}
}
```

### Key Findings for ThemisDB
- Model weights can be merged by identifying shared parameter subspaces
  (intersection of delta tensors).  This implies that different LLM variants
  share large portions of their weight tensors at the TT-core level.
- **Implication for ThemisDB:** "Single-Instance-Storage" at TT-core granularity
  can reduce LLM weight repository storage by ≥40% for shared Transformer blocks.

### ThemisDB Mapping
- `TensorDeduplicationManager::computeDelta()` — stores residual TT-chain
  (analogous to TIES delta vectors).
- `TensorDeduplicationManager::store()` — checks `TensorFingerprintGraph` for
  similar canonical tensors before deciding on delta encoding.

---

## P7: Stoudenmire & Schwab (2016) — TN for Supervised Learning

**Full Title:** Supervised Learning with Tensor Networks  
**DOI:** https://doi.org/10.48550/arXiv.1605.05775  
**Venue:** NeurIPS 2016

### BibTeX
```bibtex
@inproceedings{stoudenmire2016,
  author    = {Stoudenmire, Edwin M. and Schwab, David J.},
  title     = {Supervised Learning with Tensor Networks},
  booktitle = {Advances in Neural Information Processing Systems},
  year      = {2016},
  eprint    = {1605.05775},
  archivePrefix = {arXiv}
}
```

### Key Findings for ThemisDB
- Tensor networks naturally capture correlation structure in high-dimensional data.
- TT-cores encode correlations locally (between adjacent modes), making them
  ideal fingerprints: two tensors sharing similar local correlations will have
  similar core norms → similar fingerprints.

### ThemisDB Mapping
- `TensorFingerprintGraph::computeFingerprint()` — core-norm-based MinHash
  exploits this local correlation structure.
- The 128-element MinHash signature captures both structural (rank, order) and
  value (core norms) similarity simultaneously.

---

## P8: Roberts et al. (2019) — TensorNetwork Library

**Full Title:** TensorNetwork: A Library for Physics and Machine Learning  
**DOI:** https://doi.org/10.48550/arXiv.1905.01330  
**Venue:** arXiv preprint

### BibTeX
```bibtex
@article{roberts2019tensornetwork,
  author  = {Roberts, Chase and Milsted, Adam and Ganahl, Martin and Zalcman, Adam and Fontaine, Bruce and Zou, Yijian and Hidary, Jack and Vidal, Guifre and Leichenauer, Stefan},
  title   = {{TensorNetwork}: A Library for Physics and Machine Learning},
  year    = {2019},
  eprint  = {1905.01330},
  archivePrefix = {arXiv}
}
```

### Key Findings for ThemisDB
- Optimal contraction ordering is NP-hard in general, but for TT (linear)
  topologies it is O(d) — sequential left-to-right or right-to-left sweeps.
- Intermediate result sizes are bounded by the TT-rank, avoiding memory blowup.

### ThemisDB Mapping
- `TensorContractionEngine` uses sequential left-to-right contraction
  (transfer-matrix sweep) matching the TT topology — no contraction order
  optimisation required.

---

## P9: Rajaraman & Ullman (2011) — LSH for Nearest Neighbours

**Full Citation:** Rajaraman, A. & Ullman, J. D. (2011). Mining of Massive Datasets.
Cambridge University Press. Chapter 3: Finding Similar Items.

### Key Findings for ThemisDB
- MinHash + LSH banding: `b` bands of `r` rows each yield a similarity threshold
  of approximately `(1/b)^{1/r}` for the S-curve.
- For `b=32, r=4` (default): threshold ≈ `(1/32)^{0.25} ≈ 0.42` — tensors with
  Jaccard similarity > 0.42 have > 50% probability of sharing a bucket.

### ThemisDB Mapping
- `TensorFingerprintGraph::insertIntoBuckets()` — b=32 bands, r=4 rows/band.
- Exact verification via MinHash Jaccard approximation filters false positives
  before adding graph edges.
- Future: replace MinHash Jaccard with exact TT-domain cosine similarity for
  verification (Phase 4, Q2 2027).

---

## 🔗 Component-to-Paper Mapping

```
TensorTrainDecomposer          ← P1 (TT-SVD), P3 (TT-rounding)
TTQuantizer                    ← P2 (QTT motivation), P5 (NF4 table)
TensorNetworkStorageEngine     ← P1 (key schema), P5 (quantisation)
TensorContractionEngine        ← P3 (transfer matrix), P4 (operator compression), P8 (contraction order)
AQL: TENSOR_*                  ← P3, P4
TensorFingerprintGraph         ← P7 (core-norm fingerprint), P9 (LSH banding)
TensorDeduplicationManager     ← P6 (TIES delta), P7
```

---

## ⚠️ Open Research Questions

1. **Rank selection heuristics:** The per-step threshold `δ = ε/√(d-1)·‖T‖` may
   be overly conservative for tensors with unevenly distributed singular values.
   Adaptive per-mode thresholds (Grasedyck 2010) could improve compression.

2. **Maxwell / PDE operators:** Bigoni et al. (2016) show that Maxwell curl
   operators have low TT-rank, but the specific ranks for finite-element
   discretisations on ThemisDB-typical grids need empirical validation.

3. **LSH false-positive rate:** The current MinHash is computed from core norms
   (structural fingerprint).  For tensors with similar norms but different
   structures, false positives are expected.  Exact TT-cosine verification
   is planned for Phase 4.

---

**Last Updated:** 2026-05-05  
**Next Review:** 2026-08-01 (after Phase 2 implementation)
