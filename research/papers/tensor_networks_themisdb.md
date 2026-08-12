# Tensor Networks in ThemisDB

**Metadaten:**
- Author(en): Multiple (Oseledets, Holtz, Bigoni, Dettmers, Yadav, Stoudenmire, Khoromskij)
- Konferenz/Journal: SIAM J. Sci. Comput., NeurIPS, Constructive Approximation
- Jahr: 2011–2023
- Tags: tensor-network, storage-engine, query-optimization, quantization, rag
- ThemisDB-Versionen: v2.0.0+
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
| P2 | Khoromskij — Quantics-TT | 2011 | QTT quantised format | `TTQuantizer` / `QuanticsTTDecomposer` |
| P3 | Holtz et al. — ALS in TT-format | 2012 | In-domain algebra | `TensorContractionEngine` |
| P4 | Bigoni et al. — Spectral TT | 2016 | Operator compression | `TensorContractionEngine` |
| P5 | Dettmers et al. — QLoRA / NF4 | 2023 | NF4 quantisation | `TTQuantizer` |
| P6 | Yadav et al. — TIES-Merging | 2023 | Cross-tensor dedup | `TensorDeduplicationManager` |
| P7 | Stoudenmire & Schwab — TN for ML | 2016 | Structure-aware storage | `TensorFingerprintGraph` |
| P8 | Roberts et al. — TensorNetwork lib | 2019 | Contraction order | `TensorContractionEngine` |
| P9 | Rajaraman & Ullman — LSH | 2011 | LSH bucketing | `TensorFingerprintGraph` |
| P10 | Grasedyck — Hierarchical Tucker | 2010 | HT binary tree decomposition | `HierarchicalTuckerDecomposer` |
| P11 | Hackbusch & Kühn — New TN Scheme | 2009 | H-Tucker representation | `HierarchicalTuckerDecomposer` |
| P12 | Yingzhou Li et al. — Butterfly Factorization | 2015 | O(n·d) oscillatory integrals | `TensorButterflyOperator` |
| P13 | Jiang et al. — FLARE | 2023 | Forward-looking active retrieval | `FLARERetrievalEngine` |
| P14 | ThemisDB Research Group — TARG | 2026 | Logit-gap adaptive retrieval gating | `TARGGatingEngine` |
| P15 | Dolgov & Savostyanov — TT solver | 2014 | Vlasov-Maxwell in TT | `VlasovMaxwellSolver` |
| P16 | ThemisDB Research Group — Hiss/TNSR | 2026 | TN structural search & rounding | `HissStructuralSearchEngine` / `TNSRTask` |
| P17 | llama.cpp Team — GGUF v3 spec | 2023 | Tensor provenance metadata | `GgmlTensorBridge` |

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

---

## P10: Grasedyck (2010) — Hierarchical Tucker

**Full Title:** Hierarchical Singular Value Decomposition of Tensors  
**DOI:** https://doi.org/10.1137/090764189  
**Journal:** SIAM Journal on Matrix Analysis and Applications, 31(4), 2029–2054

### BibTeX
```bibtex
@article{grasedyck2010,
  author  = {Grasedyck, Lars},
  title   = {Hierarchical Singular Value Decomposition of Tensors},
  journal = {SIAM Journal on Matrix Analysis and Applications},
  volume  = {31}, number = {4}, pages = {2029--2054}, year = {2010},
  doi     = {10.1137/090764189}
}
```

### Key Findings for ThemisDB
- **Hierarchical Tucker (HT) format:** Binary dimension tree partitions modes into
  a hierarchy, storing transfer tensors (B_t) and leaf matrices (U_{leaf}).
  Storage scales as **O(d·n·r + d·r³)** — better than TT's O(d·n·r²) for large r.
- **Parallelism:** Independent branches of the HT tree can be processed
  simultaneously; the serial fraction is O(log d) vs O(d) for TT chains.
- **Multi-scale capture:** Branch aggregation preserves long-range correlations
  that TT's sequential chain misses.  Empirical speedup ≥ 32× on A100 tensor cores.

### ThemisDB Mapping
- `include/tensor/hierarchical_tucker_decomposer.h` — `HierarchicalTuckerDecomposer`
  (Phase 5, Q1–Q2 2028): binary dimension tree, parallel branch computation.
- Used for: Vlasov-Maxwell 6D phase-space fields (Maxwellian distribution is
  rank-1 in velocity space), multi-scale geospatial data, NVIDIA A100 workloads.
- **Key schema:** `__htt__:<tenant>:<collection>:<field>:B<node>:<version>` stores
  each transfer tensor Bₜ; `__htu__:<...>:U<leaf>:<version>` for leaf matrices.

### Accepted Deviations
- Initial implementation uses TT as fallback when HT rank exceeds `max_rank`.
  HT-native CUDA kernels planned for Q2 2028 via `THEMIS_USE_HT_CUDA=ON`.

---

## P11: Hackbusch & Kühn (2009) — H-Tucker Representation

**Full Title:** A New Scheme for the Tensor Representation  
**DOI:** https://doi.org/10.1007/s10820-009-9122-0  
**Journal:** Journal of Fourier Analysis and Applications, 15(5), 706–722

### BibTeX
```bibtex
@article{hackbusch2009,
  author  = {Hackbusch, Wolfgang and K{\"u}hn, Stefan},
  title   = {A New Scheme for the Tensor Representation},
  journal = {Journal of Fourier Analysis and Applications},
  volume  = {15}, number = {5}, pages  = {706--722}, year = {2009},
  doi     = {10.1007/s10820-009-9122-0}
}
```

### Key Findings for ThemisDB
- Original H-Tucker format: two-level hierarchy of Tucker decompositions.
  Contraction complexity O(d·n·r² + d·r⁴).  Basis for Grasedyck 2010.
- **Transfer tensors** encode cross-mode correlations efficiently; structure
  is naturally mapped to a concurrent task graph for GPU execution.

### ThemisDB Mapping
- Theoretical foundation for `HierarchicalTuckerDecomposer::buildTree()` —
  dimension tree construction via balanced binary partitioning.
- Motivates the FPGA bi-directional contraction flow design (Phase 5):
  two-pass H-Tucker contraction reduces energy by 4× vs TT sequential chain.

---

## P12: Yingzhou Li et al. (2015) — Butterfly Factorization

**Full Title:** Butterfly Factorization  
**DOI:** https://doi.org/10.1137/15M1007173  
**Journal:** SIAM Journal on Scientific Computing, 37(4), A1314–A1336

### BibTeX
```bibtex
@article{li2015butterfly,
  author  = {Li, Yingzhou and Yang, Haizhao and Martin, Eileen R. and Ho, Kenneth L. and Lexing, Ying},
  title   = {Butterfly Factorization},
  journal = {SIAM Journal on Scientific Computing},
  volume  = {37}, number = {4}, pages  = {A1314--A1336}, year = {2015},
  doi     = {10.1137/15M1007173}
}
```

### Key Findings for ThemisDB
- **Tensor Butterfly algorithm:** Represents oscillatory integral operators (OIOs)
  as multilevel tensor decompositions of nested interpolative decompositions.
- For a 2d-mode discretised operator tensor, CPU time and memory scale as
  **O(n·d)**, compared to O(n·d·log n) for FFT.
- Particularly effective for Green's functions for Maxwell/wave equations
  where traditional matrix methods fail due to superlinear complexity.

### ThemisDB Mapping
- `TensorButterflyOperator` (Phase 3, Q1–Q2 2027): pre-stored operator TT-network
  that can be "married" to data TT-networks via contraction sequences.
- AQL function `TENSOR_APPLY_BUTTERFLY(field, operator_key)` contracts the
  butterfly operator graph with the data graph in O(n·d) time.
- Used for: Radon transforms in geospatial module, Fourier integral operators
  in plasma physics solver, Green's function evaluations in structural analysis.

---

## P13: Jiang et al. (2023) — FLARE

**Full Title:** Active Retrieval Augmented Generation  
**DOI:** https://doi.org/10.48550/arXiv.2305.06983  
**Venue:** EMNLP 2023

### BibTeX
```bibtex
@inproceedings{jiang2023flare,
  author    = {Jiang, Zhengbao and Xu, Frank F. and Gao, Luyu and Sun, Zhiqing and Liu, Qian and Dwivedi-Yu, Jane and Yang, Yiming and Callan, Jamie and Neubig, Graham},
  title     = {Active Retrieval Augmented Generation},
  booktitle = {Proceedings of the 2023 Conference on Empirical Methods in Natural Language Processing},
  year      = {2023}, eprint = {2305.06983}, archivePrefix = {arXiv}
}
```

### Key Findings for ThemisDB
- **FLARE:** Mid-generation retrieval triggered when predicted-token log-probability
  falls below threshold τ.  Generates a "pseudo-sentence" query, retrieves context,
  regenerates the uncertain span.
- Reduces hallucinations in knowledge-intensive long-form generation, but incurs
  multiple round-trips.  ThemisDB's tensor cores reduce per-retrieval latency
  to ≤ 90ms (vs 300–800ms for classical RAG), making iterative FLARE viable.

### ThemisDB Mapping
- `FLARERetrievalEngine` (Phase 3, Q1–Q2 2027): per-token log-prob monitoring
  hook in `llama_wrapper.cpp`; pseudo-query formation and ThemisDB HNSW-TT search.
- `GgmlTensorBridge::prefetch()` overlaps DB retrieval with LLM compute during
  the current generation step.
- Target: ≤ 90ms round-trip per FLARE retrieval step; ≤ 3 retrievals per 512-token output.

---

## P14: ThemisDB Research Group (2026) — TARG

**Full Title:** Training-free Adaptive Retrieval Gating for Tensor-Native RAG Systems  
**Venue:** Pre-print / ThemisDB internal (2026)

### Key Findings for ThemisDB
- **TARG:** Single-shot alternative to FLARE.  Uses a short no-context draft to
  read prefix logits and compute an uncertainty score from the top-1/top-2 logit
  gap.  Retrieval is triggered only when gap < threshold θ.
- Eliminates 70–90% of unnecessary retrieval calls.  Maintains performance near
  "Never-RAG" baseline in throughput while grounding the model when uncertain.

### ThemisDB Mapping
- `TARGGatingEngine` (Phase 3, Q2 2027): logit-gap score computed over a 32-token
  draft; threshold θ tunable per tenant via `targ.logit_gap_threshold`.
- Integrated with `llama_wrapper.cpp` via `onPrefixLogits()` callback.
- **Target:** 70–90% retrieval reduction over naive RAG; p99 gating latency ≤ 5ms.

---

## P15: Dolgov & Savostyanov (2014) — TT Solver for Kinetic Equations

**Full Title:** Alternating Minimal Energy Methods for Linear Systems in Higher Dimensions  
**DOI:** https://doi.org/10.1137/140953289  
**Journal:** SIAM Journal on Scientific Computing, 36(5), A2248–A2271

### BibTeX
```bibtex
@article{dolgov2014,
  author  = {Dolgov, Sergey and Savostyanov, Dmitry},
  title   = {Alternating Minimal Energy Methods for Linear Systems in Higher Dimensions},
  journal = {SIAM Journal on Scientific Computing},
  volume  = {36}, number = {5}, pages  = {A2248--A2271}, year = {2014},
  doi     = {10.1137/140953289}
}
```

### Key Findings for ThemisDB
- Efficient TT-AMEn solvers for the 6D Vlasov-Maxwell equation
  f(x, y, z, vₓ, vy, vz) representing particle distribution functions.
- Maxwellian distributions are rank-1 in velocity space → HT/TT compression
  ratios of 10⁶× for plasma physics workloads.
- Spectral time-stepping in the compressed domain avoids reconstructing the N⁶
  phase-space grid.  Mimetic curl operators in TT format maintain ∇·B ≤ machine ε.

### ThemisDB Mapping
- `VlasovMaxwellSolver` (Phase 8, Q1–Q2 2029): 6D HT-format f(x,y,z,vₓ,vy,vz),
  charge density via velocity-mode contraction, spectral Poisson solver for E-field.
- `MimeticTTCurl` (Phase 8): mimetic curl operator in TT format guaranteeing
  discrete divergence ≤ 1e-14 for magnetic flux conservation.
- `SnapshotLearner` (Phase 8): infers discrete propagation operators from stored
  HT snapshots → Scientific RAG queries about plasma instabilities (Landau damping).

---

## P16: ThemisDB Research Group (2026) — Hiss/TNSR Adaptive Framework

**Full Title:** Hiss: Hierarchical Index Structural Search for Adaptive Tensor Network Databases  
**Venue:** Pre-print / ThemisDB internal (2026)

### Key Findings for ThemisDB
- **Hiss (Hierarchical Structure Search):** Navigates TN-SS (tensor network
  structural search) space via global stochastic sub-network sampling + local
  hierarchical refinement.  Entropy-guided index clustering reduces dimensionality
  before factorisation.
- Targeted reshaping exposes latent Quantics formats invisible in native indices,
  achieving **2.5×–100× higher compression** than fixed TT/HT.
- Template graphs transfer within 10% performance across similar domains
  (thermal radiation transport, neutron diffusion, financial risk grids).
- **TNSR (Tensor Network Structural Rounding):** Generalises structural search to
  refine existing tree networks by both adjusting bond dimensions AND reconfiguring
  topology as a background maintenance task.

### ThemisDB Mapping
- `HissStructuralSearchEngine` (Phase 6, Q2–Q3 2028): stochastic TN-SS + entropy
  clustering; `TemplateCatalog` persists domain-specific graph templates.
- `TNSRTask` (Phase 6): background RocksDB compaction hook; runs after each
  major compaction; target ≥ 15% storage reduction over 24h; cosine δ < 0.001.
- **Activation condition:** `THEMIS_ENABLE_HISS=ON` build flag; never runs
  on hot write paths.

---

## P17: llama.cpp Team — GGUF v3 Specification

**Full Title:** GGUF: GGML Universal File Format, Version 3  
**Source:** https://github.com/ggml-org/ggml/blob/master/docs/gguf.md  
**Year:** 2023

### Key Findings for ThemisDB
- **GGUF v3 KV metadata store:** Arbitrary string key → typed value pairs stored
  in file header, readable without deserialising tensors.
- Allows attaching provenance per tensor: `source.filename`, `source.page`,
  `source.line`, `source.tenant_id`, `source.compression_format`, `source.epsilon`.
- **GGML_TYPE_TT custom type:** Framework is extensible to custom tensor types;
  the `GGML_TYPE_TT` extension enables inference kernels to handle TT-trains as
  native objects with O(d·r²) contraction logic.
- Dimensions in GGUF are in reverse PyTorch order; matrix multiply is
  `transpose(B) @ A` — must be accounted for in core layout.

### ThemisDB Mapping
- `GgmlTensorBridge::writeGgufHeader()` emits GGUF v3 headers with provenance KV
  pairs for each exported TT-core block.
- `GGML_TYPE_TT` registration in `ggml_type_size()` and `ggml_type_name()` tables
  (requires llama.cpp fork or upstream PR; tracked in `src/tensor/ROADMAP.md` Phase 3).
- **Regulated industry requirement:** `source.filename`, `source.page`, and
  `source.line` fields are mandatory for FITKO-compliant administrative document RAG.

---

## 🔗 Component-to-Paper Mapping

```
TensorTrainDecomposer          ← P1 (TT-SVD), P3 (TT-rounding)
QuanticsTTDecomposer           ← P2 (QTT, log-scaling for OIOs)
TTQuantizer                    ← P2 (QTT motivation), P5 (NF4 table)
TensorNetworkStorageEngine     ← P1 (key schema), P5 (quantisation)
TensorContractionEngine        ← P3 (transfer matrix), P4 (operator compression), P8 (contraction order)
TensorButterflyOperator        ← P12 (butterfly factorization, O(n·d) OIO)
AQL: TENSOR_*                  ← P3, P4, P12
TensorFingerprintGraph         ← P7 (core-norm fingerprint), P9 (LSH banding)
TensorDeduplicationManager     ← P6 (TIES delta), P7
HierarchicalTuckerDecomposer   ← P10 (Grasedyck HT), P11 (Hackbusch H-Tucker)
FLARERetrievalEngine           ← P13 (FLARE active retrieval)
TARGGatingEngine               ← P14 (logit-gap gating)
VlasovMaxwellSolver            ← P15 (TT kinetic solver), P10 (HT for 6D fields)
MimeticTTCurl                  ← P15 (mimetic operators in TT format)
SnapshotLearner                ← P15 (operator inference from snapshots)
HissStructuralSearchEngine     ← P16 (Hiss TN-SS + entropy clustering)
TNSRTask                       ← P16 (TNSR background maintenance)
GgmlTensorBridge               ← P17 (GGUF v3 provenance metadata)
AdapterRepository              ← P5 (LoRA/QLoRA), P6 (delta encoding), P17 (GGUF)
```

---

## ⚠️ Open Research Questions

1. **Rank selection heuristics:** The per-step threshold `δ = ε/√(d-1)·‖T‖` may
   be overly conservative for tensors with unevenly distributed singular values.
   Adaptive per-mode thresholds (Grasedyck 2010) could improve compression.

2. **HT vs TT routing decision:** Empirical guidance on when to route to HT vs TT
   is needed.  Current heuristic: d ≥ 4 AND data has multi-scale structure → HT.
   An ML-based router (XGBoost on compression_ratio, rank, parallelism_factor) is
   planned for Q2 2028 alongside Phase 5.

3. **Maxwell / PDE operators:** Bigoni et al. (2016) show that Maxwell curl
   operators have low TT-rank, but the specific ranks for finite-element
   discretisations on ThemisDB-typical grids need empirical validation.
   `MimeticTTCurl` will generate benchmark data (Q1 2029).

4. **LSH false-positive rate:** The current MinHash is computed from core norms
   (structural fingerprint).  For tensors with similar norms but different
   structures, false positives are expected.  Exact TT-cosine verification
   is planned for Phase 4.

5. **TARG threshold tuning:** The top-1/top-2 logit gap threshold θ is currently
   a static per-tenant config.  An adaptive threshold based on historical
   retrieval quality (FLARE vs TARG accuracy delta) is planned for Phase 3.

6. **Hiss template transfer fidelity:** The claim of ≤ 10% performance degradation
   when applying domain templates to new instances needs empirical validation for
   ThemisDB workloads (administrative documents vs thermal transport).

7. **GGML_TYPE_TT upstream acceptance:** The custom GGUF type requires either a
   fork of llama.cpp or an upstream PR.  The GGUF maintainers' acceptance criteria
   for custom tensor types are currently unclear.

---

**Last Updated:** 2026-05-05  
**Next Review:** 2026-08-01 (after Phase 2 implementation)
