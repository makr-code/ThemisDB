# Tensor-Train Storage: Implementation Best Practices

**Metadaten:**
- Source: Oseledets (2011); Holtz et al. (2012); Dettmers et al. (2023)
- Tags: [tensor-network, compression, storage, query, llm-weights, scientific-computing]
- ThemisDB-Versionen: [v2.2.0+]
- Status: [x] Fully Adopted (Phase 1), [ ] Phase 2–4 pending

---

## 📋 Summary

Tensor-Train (TT) decomposition is the primary storage format for
multi-dimensional float tensors in ThemisDB.  It reduces storage cost from
O(nᵈ) to O(d · n · r²) where r is the TT-rank, achieving 10–100× compression
for typical LLM attention matrices and scientific simulation tensors while
preserving a user-configurable reconstruction error bound ε.

This document codifies the key implementation guidelines derived from the
foundational papers, adapted for ThemisDB's C++ / RocksDB architecture.

---

## 🎯 Core Principles

### 1. Always bound the reconstruction error explicitly
The TT-SVD algorithm (Oseledets 2011, Algorithm 1) distributes the total error
budget evenly across d-1 unfolding steps.  The per-step truncation threshold is:

```
δₖ = ε · ‖T‖_F / √(d-1)
```

**Rule:** Never use absolute SVD truncation thresholds.  Always compute `δₖ`
from the *global* Frobenius norm of the input tensor, not from the norm of the
current unfolding.  This guarantees the global error bound
`‖T - T_approx‖_F ≤ ε · ‖T‖_F` (Oseledets 2011, Theorem 2.1).

```cpp
// Correct:
double delta = cfg.eps * norm / std::sqrt(static_cast<double>(d - 1));

// Incorrect (error budget not distributed properly):
double delta = cfg.eps;
```

### 2. Apply TT-Rounding after every binary operation
TT-rank grows multiplicatively during Hadamard products and additively during
sums.  Always apply `TensorTrainDecomposer::round()` (Algorithm 2, Oseledets
2011) after any binary TT-operation to prevent rank explosion.

```cpp
// After hadamard product:
auto result = hadamardProduct(a, b);
result = decomposer.round(result, {.eps = eps, .max_rank = max_rank});
```

**Performance target:** TT-rounding of a rank-32 6D tensor (n=10) should
complete in ≤ 50 ms CPU (right-to-left QR sweep + left-to-right SVD).

### 3. Use the transfer-matrix algorithm for inner products
**Never reconstruct both tensors to compute inner products or cosine
similarity.**  The transfer-matrix (zipper) algorithm (Holtz et al. 2012,
Eq. 2.6) computes `⟨A, B⟩` with complexity O(d · n · r³) without
ever materialising the full tensors.

```cpp
// Correct (O(d·n·r³)):
double sim = TensorContractionEngine::cosineSimilarity(a, b);

// Incorrect (O(n^d)):
auto da = a.reconstruct();
auto db = b.reconstruct();
double sim = dotProduct(da, db) / (norm(da) * norm(db));
```

### 4. Choose quantisation type by data distribution
| Distribution | Recommended Type | Expected Compression |
|---|---|---|
| Uniform | INT8 | ~4× vs float32 |
| Normal N(0, σ²) | NF4 | ~8× vs float32 |
| Sparse | INT8 + TT-rank cap | ~4–10× |
| Mixed / unknown | INT8 | ~4× |

**Rule:** Use `QuantizationType::NF4` for LLM attention weights and embedding
matrices (validated to follow near-normal distributions by Dettmers 2023).
Use `QuantizationType::INT8` for general-purpose tensors.

### 5. Set max_rank to cap worst-case memory
Even with aggressive `eps`, pathological tensors (e.g., random noise) may
produce high TT-ranks.  Always set a `max_rank` cap in production to bound
worst-case core sizes.

**Recommended defaults:**
- General-purpose: `max_rank = 64`
- LLM attention (known low-rank): `max_rank = 32`
- Scientific simulation: `max_rank = 128` (higher tolerance for higher error)
- Delta tensors in `TensorDeduplicationManager`: `max_rank = 16`

### 6. Store cores separately in RocksDB for partial reads
The key schema `__ttn__:<tenant>:<collection>:<field>:G<k>:<version>` stores
each core Gk under its own key.  This enables:
- **Partial reads:** Load only the first few cores for approximate metadata.
- **Parallel loading:** Multiple cores fetched concurrently on multi-threaded
  query paths.
- **Incremental versioning:** Only updated cores need to be re-written.

**Anti-pattern:** Do NOT serialise the entire TTTrain into a single RocksDB
value.  Large values (> 100 KB) degrade RocksDB block cache efficiency.

### 7. Fingerprint before similarity query, not after
`TensorFingerprintGraph` must compute the MinHash fingerprint from TT-core
norms *before* executing the LSH lookup.  The fingerprint computation is O(d·n)
and should be cached at insert time, not recomputed on every query.

### 8. Delta-encode similar tensors, not identical ones
`TensorDeduplicationManager` is designed for *structurally similar* tensors
(similarity ≥ 0.999), not exact duplicates.  For exact duplicates (hash-equal
core bytes), use RocksDB's built-in block deduplication instead of delta
encoding — it has zero CPU overhead.

---

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/storage/` — `TensorTrainDecomposer`, `TTQuantizer`,
  `TensorNetworkStorageEngine`
- `src/query/` — `TensorContractionEngine`, AQL tensor functions
- `src/graph/` — `TensorFingerprintGraph`, `TensorDeduplicationManager`

### What Was Adopted?

1. **TT-SVD (Oseledets 2011, Algorithm 1)** — core decomposition in
   `TensorTrainDecomposer::decompose()`.
2. **TT-Rounding (Oseledets 2011, Algorithm 2)** — implemented in
   `TensorTrainDecomposer::round()`.
3. **Transfer-matrix inner product (Holtz 2012)** — in
   `TensorTrainDecomposer::innerProduct()` and
   `TensorContractionEngine::innerProduct()`.
4. **NF4 lookup table (Dettmers 2023)** — `TTQuantizer::kNF4Table[16]`.
5. **MinHash + LSH banding (Rajaraman & Ullman 2011)** — in
   `TensorFingerprintGraph::computeFingerprint()` and
   `insertIntoBuckets()`.
6. **Delta encoding (TIES-Merging concept, Yadav 2023)** — in
   `TensorDeduplicationManager::computeDelta()`.

### Deviations & Rationale

1. **Internal SVD vs LAPACK:** The decomposer uses a self-contained Golub-Reinsch
   bidiagonalisation to eliminate the LAPACK build dependency.  For production
   deployments, LAPACK `dgesdd` should be used (enable via
   `THEMIS_USE_LAPACK_SVD=ON`).  See STUB_INVENTORY.md entry for
   `tensor_train_decomposer.cpp`.

2. **Core-norm MinHash instead of value MinHash:** Computing MinHash from TT-core
   norms (rather than individual element values) reduces fingerprint computation
   time from O(n^d) to O(d·r²) and is still highly discriminative for structural
   similarity.  Exact cosine similarity verification is planned for Phase 4.

3. **Jaccard approximation in graph edges:** The fingerprint graph uses MinHash
   Jaccard as the similarity metric for edge creation (O(h) per pair), not exact
   TT-cosine similarity (O(d·n·r³) per pair).  This avoids a quadratic number of
   expensive TT inner products during bulk inserts.

---

## ⚠️ Trade-offs & Limitations

> Full quantitative analysis: [`research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`](../HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md)

### 1. Reconstruction error accumulates across algebraic operations

After k TT operations (each introducing ε_i), the total error reaches
√(Σ εᵢ²) with TT-rounding after each step, or up to k·ε_max without rounding.

**Quantitative bound:** For k=5 operations, ε=0.01:
- With rounding: ε_total ≤ 0.022 — safe for Recall@10 ≥ 0.97
- Without rounding: ε_total ≤ 0.05 — Recall degradation possible

**Rule:** Maximum k = Δ_min / ε operations without rounding, where Δ_min is  
the minimum pairwise distance of nearest-neighbour candidates (typically 0.025  
for LLM embeddings → k_max = 2).

*Mitigation:* Always apply `round()` after each binary TT operation.  
Planned: `TensorContractionEngine` auto-round after each chained operation (Q4 2026).

---

### 2. High-rank tensors (random noise) compress poorly

TT-SVD is effective only when the tensor has structured (low-rank) correlations.  
For truly random tensors: TT-rank = O(n^{d/2}), giving no compression.

**Compressibility indicator κ** (analytically derived):

```
κ = d·log(n) / log(TT-params)  =  2·log(n_pilot) / (2·log(r_pilot) + log(n_pilot))

κ ≥ 1.7  →  LIFT (TT clearly beneficial)
1.3 ≤ κ < 1.7  →  HYBRID (TT-shadow + HNSW search)
κ < 1.3  →  KEEP (HNSW or FAISS IVF-PQ)
```

**Storage break-even vs HNSW:** TT uses less memory than HNSW when r_eff < 2√d.

| Embedding dim d | r_break (memory parity) |
|----------------|------------------------|
| 768            | r ≈ 56                 |
| 1536           | r ≈ 79                 |
| 4096           | r ≈ 128                |

Typical r_eff for LLM weights: 8–32 → TT wins for d ≥ 256.

*Mitigation:* `TensorNetworkStorageEngine` `min_compression_ratio` guard + κ in `TensorRouter`.  
Updated `TensorRoutingPolicy` thresholds: `min_lift_compression_ratio = 4.0`, `max_lift_rank = 48`.

---

### 3. MinHash false positives for tensors with similar norms but different structures

Core-norm MinHash false-positive rate at Jaccard=0.3 with 128 hash functions,  
32 bands: P_fp ≈ 23% — too high for production deduplication without exact verification.

**Two-stage filter (recommended):**
1. LSH bucket → Jaccard ≥ 0.7 (raises precision significantly)
2. Exact TT-cosine similarity ≥ 0.999 (final dedup decision)

**Verification overhead:** At 1000 candidates/insert and r=32, d=6, n=64:  
~20 ms/insert — acceptable up to ~50 inserts/s.

*Mitigation:* `max_candidates` cap in `FingerprintGraphConfig`.  
Planned: Singular-value-based fingerprinting for better structural discrimination (Q1 2027).  
Planned: Two-stage filter implementation (Q2 2027).

---

### 4. NF4 is suboptimal for non-normal distributions

NF4's quantile table assumes N(0,1). Quantisation error by distribution type:

| Distribution | NF4 MSE/σ² | INT8 MSE/σ² | Recommended |
|-------------|------------|------------|-------------|
| Normal N(0,1) | 0.0012 | 0.0014 | **NF4** |
| Uniform [-1,1] | 0.0028 | 0.0009 | **INT8** |
| Bimodal N(±1, 0.3) | 0.0041 | 0.0012 | **INT8** |
| Laplace | 0.0015 | 0.0018 | **NF4** |
| Heavy-tail (Cauchy) | 0.0180 | 0.0035 | **INT8** |

**Auto-selection rule via excess kurtosis:**
```
|excess_kurtosis| < 1.0  →  NF4
excess_kurtosis > 3.0   →  INT8  (heavy tail)
excess_kurtosis < -1.0  →  INT8  (platykurtic / uniform)
```

*Mitigation:* Inspect data distribution before selecting `QuantizationType`.  
Planned: `TTQuantizer::autoQuantize()` with kurtosis-based selection (Q4 2026).

---

## 🔬 Validation

- [x] TT-SVD error bound verified in `tests/storage/test_tensor_train_decomposer.cpp`
  (TTD-02, TTD-04)
- [x] Transfer-matrix inner product verified against dense dot product
  (TTD-06, TCE-04)
- [x] INT8 and NF4 quantisation round-trip error within spec (TTD-13, TTD-14)
- [x] LSH fingerprint graph insert/query cycle verified (TFG-01..TFG-20)
- [ ] LAPACK SVD parity test (planned Q3 2026)
- [ ] GPU TT-SVD benchmark ≤ 80ms for 10⁶-element 6D tensor (planned Q3 2026)
- [ ] End-to-end storage compression benchmark ≥ 10× for LLM attention matrices
  (planned Q4 2026)

## 📚 Related

- [**HNSW/FAISS/TT Boundary Analysis**](../HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md) — quantitative decision boundaries
- [**arXiv Draft**](../TENSOR_NETWORK_DATABASE_ARXIV_DRAFT.md) — full paper (VLDB 2027 target)
- [Paper references: tensor_networks_themisdb.md](../papers/tensor_networks_themisdb.md)
- [src/storage/ROADMAP.md — Phase 8](../../src/storage/ROADMAP.md)
- [src/query/ROADMAP.md — Phase 9](../../src/query/ROADMAP.md)
- [src/graph/ROADMAP.md — Phase 8](../../src/graph/ROADMAP.md)

---

## 🆕 Best Practices: Hierarchical Tucker (HT)

### HT-1. Prefer HT over TT for d ≥ 5 and GPU workloads
For high-dimensional data (d ≥ 5) with multi-scale structure and GPU execution,
Hierarchical Tucker [Grasedyck 2010] provides better parallelism and compression:
- Storage: O(d·n·r + d·r³) vs TT's O(d·n·r²) for large r.
- Parallel branches process independently; serial fraction O(log d) vs O(d) for TT.
- A100 tensor cores: empirical 32× speedup for HT over standard CUDA cores.

**Routing rule in `TensorRouter`:**
```cpp
if (d_modes >= 5 && category == SIMULATION) return Format::HT;
if (d_modes >= 5 && gpu_acceleration_available) return Format::HT;
// otherwise:
return Format::TT;
```

### HT-2. Use balanced binary dimension tree
Partition modes into a balanced binary tree.  For d=6 (plasma 6D):
```
tree: ((x,y), (z, vx)), ((vy, vz))
```
Unbalanced trees degrade to near-TT performance.  `HierarchicalTuckerDecomposer::buildTree()`
constructs balanced trees automatically via mode-size sorting.

### HT-3. Apply HT-rounding after every HT-algebraic operation
Similar to TT-rounding, HT ranks grow during binary operations.
`HierarchicalTuckerDecomposer::round()` (planned Q1 2028) must be called after
every HT hadamard, addition, or slice.

**Target:** HT-rounding for a rank-32 6D tensor (n=64) in ≤ 200ms CPU,
≤ 20ms CUDA (A100).

---

## 🆕 Best Practices: Quantics-TT (QTT)

### QTT-1. Use QTT for oscillatory integral operators (OIOs)
For OIOs (Fourier integrals, Green's functions, wave equation operators), standard
TT-SVD may require ranks O(n) to resolve oscillations.  QTT [Khoromskij 2011]
factorises each dimension via binary representation, achieving O(log n) ranks:

```
n=1024 grid → 10 binary modes of size 2
QTT storage: O(d · log(n) · r²)  vs  TT: O(d · n · r²)
```

**Rule:** Use `QuanticsTTDecomposer` when:
- The target function is oscillatory (Maxwell kernel, Green's function).
- Grid size n ≥ 64 per dimension.
- Measured TT-rank exceeds 32 after standard TT-SVD.

### QTT-2. Combine with Tensor Butterfly for O(n·d) operator application
Pre-store OIO operator as a Tensor Butterfly network (`TensorButterflyOperator`).
Application complexity drops to O(n·d) vs O(n·d·log n) for FFT.

```
AQL: SELECT TENSOR_APPLY_BUTTERFLY(simulation_field, 'maxwell_green_op')
     WHERE collection = 'plasma_snapshots'
```

---

## 🆕 Best Practices: Hiss/TNSR Adaptive Framework

### Hiss-1. Enable TNSR only after a stable base decomposition exists
TNSR re-optimises the network topology.  Running it on freshly inserted data with
unstable correlations wastes CPU.  Recommended trigger: ≥ 1000 inserts since last
TNSR sweep OR ≥ 10% data entropy change (measured via pilot SVD sample).

```cpp
// In TensorCompactionFilter::Filter():
if (stats.inserts_since_tnsr < 1000 && stats.entropy_delta < 0.10) return Keep;
return TNSRTask::schedule(key, value);
```

### Hiss-2. Store domain template graphs in TemplateCatalog
After Hiss converges on an optimal structure for a domain, persist the template:
```
Key: __hiss_template__:<domain>:<version>
Value: serialised HissTemplateGraph (topology + bond dimensions)
```
Re-use templates for similar new collections: compression within 10% of domain
optimum without re-running full structural search.

### Hiss-3. Gate TNSR behind a build flag in production
TNSR is computationally expensive.  Never run on hot write paths.

```cpp
// STUB/SIMULATION NOTE:
// Purpose: TNSR is disabled by default in production builds
// Activation: THEMIS_ENABLE_HISS=ON cmake flag
// Production Delta: Without HISS, TT/HT structures are static after first decomposition
// Removal Plan: Enable by default after Q3 2028 validation campaign
```

---

## 🆕 Best Practices: GGUF v3 Provenance Metadata

### GGUF-1. Always emit provenance KV pairs per tensor block
When exporting TT-cores to GGUF v3 via `GgmlTensorBridge::writeGgufHeader()`,
the following keys are **mandatory** for regulated-industry deployments:

```
source.filename     string   — original document filename
source.page         uint32   — page number (1-based) or 0 for non-documents
source.line         uint32   — line number or 0
source.tenant_id    string   — ThemisDB tenant identifier
source.epsilon      float32  — reconstruction error bound used during decomposition
source.format       string   — "TT" | "HT" | "QTT"
source.rank_max     uint32   — maximum TT/HT rank used
```

### GGUF-2. Account for reversed dimension order in GGUF
GGUF stores dimensions in reverse PyTorch order.  A TT-core `Gₖ ∈ ℝ^{r_prev × n_k × r_next}`
must be stored with GGUF dimensions `[r_next, n_k, r_prev]`.  `GgmlTensorBridge`
handles this automatically via `transposeGgufDimensions()`.

### GGUF-3. Register GGML_TYPE_TT before any inference call
The custom type must be registered before `ggml_tensor` operations:
```cpp
// In GgmlTensorBridge constructor (or llama.cpp init hook):
ggml_register_type(GGML_TYPE_TT, {
    .type_size = sizeof(TTTrain*),  // pointer to managed TTTrain
    .blck_size = 1,
    .type_name = "TT",
    .to_float  = &GgmlTensorBridge::contractToFloat,
    .from_float = nullptr,  // TT created via ThemisDB decomposition only
});
```

---

**Last Updated:** 2026-05-05  
**Next Review:** 2026-09-01 (after Phase 2 completion)
