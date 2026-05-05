# Tensor Module — Future Enhancements

<!-- Status: current | generated: 2026-05-05 -->

## tensor — Tensor-Train ANN Index Module

### Scope

First-class Tensor-Train-based ANN index module parallel to `src/index/`.
Provides compressed storage and compressed-domain queries for high-dimensional
data where structured compressibility (κ ≥ 2×) makes TT strictly better than
flat HNSW/FAISS.

---

### Enhancement: Phase 2 — hnswlib Integration in HnswTTBridge

**Target:** Q4 2026

**Affected files:**
- `src/tensor/hnsw_tt_bridge.cpp` → `HnswTTBridge::HnswLayer`
- `include/tensor/hnsw_tt_bridge.h` → `HnswTTConfig::sketch_dim`

**Design Constraints:**
- Link `hnswlib::HierarchicalNSW<float>` under `THEMIS_HNSW_ENABLED` guard
- Sketch vectors: `min(n_1_of_G_0, sketch_dim)` floats (first-core row-mean)
- `M=16`, `ef_construction=200` defaults; overridable via `HnswTTConfig`
- Thread-safe: exclusive lock during graph mutation; shared during search

**Required Interfaces:**
- `HnswLayer::insert(id, sketch)` → `hnsw_index->addPoint(sketch.data(), id)`
- `HnswLayer::search(query, ef)` → `hnsw_index->searchKnn(query.data(), ef)`
- `HnswLayer::remove(id)` → `hnsw_index->markDelete(id)`

**Implementation Notes:**
- `HnswLayer` must own a `std::unique_ptr<hnswlib::SpaceInterface<float>>` for cosine/L2
- Initialize lazily on first `insert()` using the first sketch's dimension

**Test Strategy:**
- Unit: insert 10K 4096-dim flat vectors, recall@10 ≥ 0.95 vs. brute-force
- Performance: search latency ≤ 2ms for n=100K, dim=4096, ef=50

**Performance Targets:**
- HNSW-layer search: ≤ 2ms for 100K sketches at ef=50
- TT re-rank: ≤ 5ms for 200 candidates at d=6, r=32

**Security / Reliability:**
- Tenant sketch isolation: HNSW graph is per-index (no cross-tenant leakage)
- Validate sketch dimension consistency on insert; reject mismatched dims

---

### Enhancement: Phase 2 — RocksDB Persistence

**Target:** Q4 2026

**Affected files:**
- `src/tensor/tensor_index.cpp` (`FlatTensorIndex::save/load`)
- `src/tensor/hnsw_tt_bridge.cpp` (`HnswTTBridge::save/load`)
- `src/tensor/tensor_index_manager.cpp` (`dropTenantIndexes`)

**Design Constraints:**
- Key schema: `__ttidx__:<tenant>:<collection>:<field>:<id>:G<k>`
- Use `TensorNetworkStorageEngine::put/get` for core serialization
- HNSW graph persisted to separate `__ttidx_hnsw__:<...>` key

**Required Interfaces:**
- `TensorNetworkStorageEngine::put(key, core_data)`
- `TensorNetworkStorageEngine::get(key) → std::optional<core_data>`
- `RocksDBWrapper::deleteRange(from, to)` for tenant cleanup

**Implementation Notes:**
- `save()` must use WriteBatch for atomicity across all cores
- `load()` must iterate the prefix and reconstruct the TTTrain chain in order
- Version field in key allows schema migration

**Test Strategy:**
- Write 1K tensors, crash-recover, verify all read back correctly
- Tenant drop: verify no cross-tenant keys remain after `dropTenantIndexes()`

**Performance Targets:**
- save(): ≤ 50ms for 1K rank-32 tensors at d=6
- load(): ≤ 200ms cold-start for same set

---

### Enhancement: Phase 3 — Zero-Copy GGML Bridge

**Target:** Q1 2027

**Affected files:**
- `src/tensor/tensor_index_manager.cpp` (`ggmlCorePtrs`)
- `include/storage/ggml_tensor_bridge.h`
- `src/storage/ggml_tensor_bridge.cpp` (new)

**Design Constraints:**
- Pin TT-core pages via `mmap(MAP_SHARED)` or `cudaHostRegister`
- Return `GgmlCoreDescriptor { ggml_tensor*, size_t core_idx }` per core
- Must not invalidate pointers until `TensorIndexManager::releaseGgmlPtrs()`
- Only available under `THEMIS_ENABLE_GGML_BRIDGE` compile flag

**Required Interfaces:**
- `ggml_new_tensor_1d(ctx, GGML_TYPE_TT, size)` — llama.cpp ggml extension
- `ggml_map_custom1` for TT-contraction op registration

**Implementation Notes:**
- `GGML_TYPE_TT` is a new `ggml_type` enum value that ThemisDB adds to ggml
- The bridge owns a ref-count; `releaseGgmlPtrs()` unregisters mmap region

**Test Strategy:**
- Unit: verify pointer addresses match internal core data storage
- Integration: load dummy TT-model, inject to ggml graph, verify output matches
  decompressed inference

**Performance Targets:**
- `ggmlCorePtrs()` overhead: ≤ 1ms (should be near-zero after first call)
- TTFT improvement: ≥ 3× vs. classical JSON-RAG path

**Security / Reliability:**
- mmap fence prevents cross-tenant pointer sharing
- Address range is invalidated on index drop

---

### Enhancement: Phase 3 — TensorAwareQueryOptimizer TENSOR_CONTRACTION Node

**Target:** Q1 2027

**Affected files:**
- `src/query/aql_translator.cpp`
- `src/query/query_optimizer.cpp` (new TENSOR_CONTRACTION case)
- `include/query/tensor_rag_cost_model.h`

**Design Constraints:**
- New plan node `TENSOR_CONTRACTION` added to the AQL physical plan AST
- Optimizer detects `TENSOR_SIMILARITY`, `TENSOR_NORM`, `TENSOR_SLICE` calls
  whose operands resolve to TT-stored fields
- Routes to `TensorContractionEngine` instead of flat-vector path

**Required Interfaces:**
- `TensorContractionEngine::innerProduct(TTTrain&, TTTrain&)`
- `TensorRagCostModel::estimateCost(TENSOR_CONTRACTION, ...)` — Phase 2 done

**Implementation Notes:**
- `EXPLAIN` output must show `TENSOR_CONTRACTION` nodes with rank/dim metadata
- Fallback to decompression path if TT-storage not confirmed at plan time

**Test Strategy:**
- AQL integration: `SELECT TENSOR_SIMILARITY(a.vec, b.vec)` plan shows
  `TENSOR_CONTRACTION` node in EXPLAIN
- Correctness: result within ±ε of decompressed reference

---

### Enhancement: Phase 4 — CUDA TT-SVD in addFlat()

**Target:** Q4 2027

**Affected files:**
- `src/storage/tensor_train_decomposer.cpp`
- `src/tensor/tensor_index.cpp`

**Design Constraints:**
- Guard under `THEMIS_ENABLE_CUDA`
- Use cuSOLVER `cusolverDnSgesvd` for SVD steps
- Fall back to CPU LAPACK `dgesvd` if CUDA unavailable

**Required Interfaces:**
- `TensorTrainDecomposer::Config::use_cuda` flag (already declared)
- cuSOLVER handle lifetime managed by `CudaBackend` singleton

**Performance Targets:**
- TT-SVD for 10⁶-element 6D tensor: ≤ 80ms GPU vs. ≤ 500ms CPU

---

### Enhancement: Phase 4 — Distributed TT Shard Layout

**Target:** Q4 2027

**Affected files:**
- `src/sharding/adaptive_shard_router.cpp`
- `src/tensor/tensor_index_manager.cpp`

**Design Constraints:**
- Shard k stores `G_k` cores for all vectors in that tenant/collection
- Enables all-reduce contraction across shards without assembling full TT chain
- Compatible with existing Reed-Solomon parity scheme

**Implementation Notes:**
- Shard key: `__ttidx__:<tenant>:<collection>:<field>:shard<k>:<id>`
- `RemoteExecutor` performs the contraction step for remote-shard cores

**Performance Targets:**
- Distributed inner-product for rank-32 6D TT: ≤ 20ms across 6 shards at 1 Gbps

---

## Summary Table

| Enhancement | Target | Stub IDs | Priority |
|-------------|--------|----------|----------|
| hnswlib integration | Q4 2026 | HTB-01 | 🟠 High |
| RocksDB persistence | Q4 2026 | TTI-01, TTI-02, TIM-02, HTB-02, HTB-03 | 🟠 High |
| Zero-Copy GGML bridge | Q1 2027 | TIM-01 | 🟡 Medium |
| TENSOR_CONTRACTION plan-node | Q1 2027 | — | 🟡 Medium |
| CUDA TT-SVD | Q4 2027 | — | 🟢 Low |
| Distributed shard layout | Q4 2027 | — | 🟢 Low |
