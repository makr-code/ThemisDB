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

1. **Reconstruction error accumulates across algebraic operations:**
   After multiple TT operations (each introducing ε_k error), the total error
   can reach √(Σ ε_k²) without TT-rounding between steps.
   *Mitigation:* Always apply `round()` with a tight eps after each operation.

2. **High-rank tensors (random noise) compress poorly:**
   TT-SVD is effective only when the tensor has low TT-rank (structured data).
   For truly random tensors, TT-rank = min(n_k, r_{k-1} · n_k) = O(n^{d/2}),
   giving no compression.
   *Mitigation:* The `min_compression_ratio` guard in `TensorNetworkStorageEngine`
   falls back to raw storage when TT compression is not beneficial.

3. **MinHash false positives for tensors with similar norms but different structures:**
   Two tensors with the same core-norm profile but different core values will
   hash to the same LSH bucket.  Exact verification prevents false edges, but
   CPU cost increases linearly with the number of candidates.
   *Mitigation:* `max_candidates` cap in `FingerprintGraphConfig`.

4. **NF4 is suboptimal for non-normal distributions:**
   The NF4 lookup table is derived from N(0,1) quantiles.  For bimodal or
   heavy-tailed distributions, INT8 achieves lower quantisation error.
   *Mitigation:* Callers should inspect the data distribution before selecting
   `QuantizationType`.

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

- [Paper: tensor_networks_themisdb.md](../papers/tensor_networks_themisdb.md)
- [src/storage/ROADMAP.md — Phase 8](../../src/storage/ROADMAP.md)
- [src/query/ROADMAP.md — Phase 9](../../src/query/ROADMAP.md)
- [src/graph/ROADMAP.md — Phase 8](../../src/graph/ROADMAP.md)

---

**Last Updated:** 2026-05-05  
**Next Review:** 2026-09-01 (after Phase 2 completion)
