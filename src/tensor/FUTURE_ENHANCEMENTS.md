# Tensor Module — Future Enhancements

<!-- Status: current | generated: 2026-05-05 | updated: 2026-05-05 aligned with paper -->

> **Scientific Basis:** All enhancements below are grounded in
> *"Advanced Architectural Paradigms for Multi-Model AI Databases: Integrating Tensor Graph
> Networks and Zero-Copy Inference in ThemisDB"* (2026).  Section references are noted
> inline.

## tensor — Tensor-Train ANN Index Module

### Scope

First-class Tensor-Train-based ANN index module parallel to `src/index/`.
Provides compressed storage and compressed-domain queries for high-dimensional
data where structured compressibility (κ ≥ 2×) makes TT strictly better than
flat HNSW/FAISS.  Planned extensions cover Hierarchical Tucker (HT), Quantics
TT (QTT), adaptive structural rounding (Hiss/TNSR), multi-modal UTR encoding,
and physics-informed spectral solvers.

---

### Enhancement: Phase 2 — hnswlib Integration in HnswTTBridge

**Target:** Q4 2026  
**Paper §:** Similarity Search in the Latent Space

**Affected files:**
- `src/tensor/hnsw_tt_bridge.cpp` → `HnswTTBridge::HnswLayer`
- `include/tensor/hnsw_tt_bridge.h` → `HnswTTConfig::sketch_dim`

**Design Constraints:**
- Link `hnswlib::HierarchicalNSW<float>` under `THEMIS_HNSW_ENABLED` guard
- Sketch vectors: `min(n_1_of_G_0, sketch_dim)` floats (first-core row-mean)
- `M=16`, `ef_construction=200` defaults; overridable via `HnswTTConfig`
- Thread-safe: exclusive lock during graph mutation; shared during search
- Paper accuracy target: 91% hybrid-TT recall vs. 73% dense baseline

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
- Throughput: ≥ 741 ops/s for large-blob queries (paper §Similarity Search table)

**Performance Targets:**
- HNSW-layer search: ≤ 2ms for 100K sketches at ef=50
- TT re-rank: ≤ 5ms for 200 candidates at d=6, r=32

**Security / Reliability:**
- Tenant sketch isolation: HNSW graph is per-index (no cross-tenant leakage)
- Validate sketch dimension consistency on insert; reject mismatched dims

---

### Enhancement: Phase 2 — RocksDB Persistence

**Target:** Q4 2026  
**Paper §:** RocksDB Integration and the LSM-Tree Paradigm

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

### Enhancement: Phase 2 — Rank-Adaptive RocksDB Compaction Filter

**Target:** Q4 2026  
**Paper §:** RocksDB Integration — "Background compaction processes... can automatically
'round' tensor cores to lower ranks"

**Affected files:**
- `src/storage/tensor_train_decomposer.cpp` — `recompress()` method
- `src/storage/tensor_network_storage_engine.cpp` — `TensorCompactionFilter`
- `include/storage/tensor_network_storage_engine.h`

**Design Constraints:**
- `TensorCompactionFilter` inherits `rocksdb::CompactionFilter`
- `FilterV2()` de-serializes TT-core key, invokes `TensorTrainDecomposer::recompress(ε)`
- `ε` configured via column-family option `tensor.recompress_epsilon` (default 1e-4)
- Only applied when SSTable entropy metric < threshold to avoid unnecessary work

**Required Interfaces:**
- `TensorTrainDecomposer::recompress(TTTrain&, epsilon) → TTTrain` — round to lower rank
- `TensorEntropyEstimator::estimate(SSTableMetadata) → float`

**Implementation Notes:**
- Filter is opt-in: disabled by default; enabled via `setCompactionFilter()`
- STUB/SIMULATION NOTE required: rank selection in `recompress()` uses heuristic
  until LAPACK SVD (THEMIS_USE_LAPACK_SVD) is available

**Test Strategy:**
- Build a column family with 1K Maxwellian-shaped tensors; compact with ε=1e-3;
  verify storage reduction ≥ 20% and max cosine error ≤ ε

**Performance Targets:**
- Compaction overhead: ≤ 2× vs. standard compaction on same SSTable size

**Security / Reliability:**
- Compaction must never increase rank (only lower or keep equal)
- Failed recompression leaves original data unchanged (copy-on-success semantics)

---

### Enhancement: Phase 3 — Zero-Copy GGML Bridge

**Target:** Q1 2027  
**Paper §:** Zero-Copy Inference — Null-Pointer Transfer Mechanism

**Affected files:**
- `src/tensor/tensor_index_manager.cpp` (`ggmlCorePtrs`)
- `include/storage/ggml_tensor_bridge.h`
- `src/storage/ggml_tensor_bridge.cpp` (new)

**Design Constraints:**
- Pin TT-core pages via `mmap(MAP_SHARED)` or `cudaHostRegister`
- Return `GgmlCoreDescriptor { ggml_tensor*, size_t core_idx }` per core
- Must not invalidate pointers until `TensorIndexManager::releaseGgmlPtrs()`
- Only available under `THEMIS_ENABLE_GGML_BRIDGE` compile flag
- Apple Silicon path: unified memory pool; pointer is directly usable by Metal/ANE

**Required Interfaces:**
- `ggml_new_tensor_1d(ctx, GGML_TYPE_TT, size)` — llama.cpp ggml extension
- `ggml_map_custom1` for TT-contraction op registration

**Implementation Notes:**
- `GGML_TYPE_TT` is a new `ggml_type` enum value that ThemisDB adds to ggml;
  inference kernels handle TT tensors as native O(d·r²) contraction objects
- The bridge owns a ref-count; `releaseGgmlPtrs()` unregisters mmap region
- Paper transfer latency table: disk-to-RAM 0ms (mmap), RAM-to-VRAM 0ms (unified),
  serialization 0ms, tokenization 0ms → total TTFT 40–90ms vs. 150–400ms standard

**Test Strategy:**
- Unit: verify pointer addresses match internal core data storage
- Integration: load dummy TT-model, inject to ggml graph, verify output matches
  decompressed inference
- TTFT benchmark: ≥ 3× improvement on a standard RAG task

**Performance Targets:**
- `ggmlCorePtrs()` overhead: ≤ 1ms (near-zero after first call)
- TTFT improvement: ≥ 3× vs. classical JSON-RAG path

**Security / Reliability:**
- mmap fence prevents cross-tenant pointer sharing
- Address range is invalidated on index drop

---

### Enhancement: Phase 3 — GGUF v3 Metadata Provenance

**Target:** Q1 2027  
**Paper §:** GGUF Metadata and File Stability

**Affected files:**
- `src/storage/ggml_tensor_bridge.cpp`
- `include/storage/gguf_metadata.h` (new)

**Design Constraints:**
- GGUF version 3 key-value metadata store attached to each TT-core tensor
- Mandatory provenance fields: `source.filename`, `source.page`, `source.line`,
  `source.doc_id`, `source.tenant_id`, `source.ingest_timestamp`
- All fields must round-trip through save/load without loss

**Required Interfaces:**
- `GGUFMetadata::attach(ggml_tensor*, ProvenanceRecord)`
- `GGUFMetadata::retrieve(ggml_tensor*) → std::optional<ProvenanceRecord>`

**Test Strategy:**
- Write tensor with provenance; reload; assert all fields identical
- Regulated-industry traceability test: citation accuracy on 500 structured docs

**Security / Reliability:**
- Provenance metadata signed via HMAC(tenant_key, record) to prevent tampering

---

### Enhancement: Phase 3 — TensorAwareQueryOptimizer TENSOR_CONTRACTION Node

**Target:** Q1 2027  
**Paper §:** AQL and Compressed Domain Computing

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

### Enhancement: Phase 3 — AQL Operators CONTRACT, PROJECT, DECOMPOSE

**Target:** Q2 2027  
**Paper §:** AQL and Compressed Domain Computing — "structure-oriented system"

**Affected files:**
- `src/query/aql_translator.cpp`
- `src/aql/aql_lexer.cpp` / `aql_parser.cpp`
- `include/query/tensor_contraction_engine.h`

**Design Constraints:**
- `CONTRACT(expr_a, expr_b, mode_list)` — multi-mode tensor contraction;
  returns a `TTTrain` result expression
- `PROJECT(expr, mode_index)` — marginalizes over given mode (velocity marginal, etc.)
- `DECOMPOSE(field, shape, rank, epsilon)` — on-the-fly TT decompose of a scalar field
  stored as a blob; result is a TTTrain usable in subsequent expressions
- All operators pipeline without materializing intermediate full tensors

**Required Interfaces:**
- `TensorContractionEngine::contract(TTTrain&, TTTrain&, modes) → TTTrain`
- `TensorContractionEngine::project(TTTrain&, mode) → TTTrain`
- `TensorTrainDecomposer::decomposeOnTheFly(blob_key, shape, rank, epsilon) → TTTrain`

**Test Strategy:**
- AQL: `RETURN CONTRACT(a.grid_tt, b.operator_tt, [2,3])` plan + correctness
- AQL: `RETURN PROJECT(f_tt, [3,4,5])` charge density vs. reference
- AQL: `RETURN DECOMPOSE(doc.raw_embedding, [64,64], 8, 0.01)` similarity search

**Performance Targets:**
- CONTRACT/PROJECT: throughput ≥ 10K ops/s for rank-16 4D tensors

---

### Enhancement: Phase 3 — Tensor Butterfly Algorithm (Oscillatory Integral Operators)

**Target:** Q2 2027  
**Paper §:** Operator Compression and Linear Complexity Transforms

**Affected files:**
- `include/tensor/tensor_butterfly_operator.h` (new)
- `src/tensor/tensor_butterfly_operator.cpp` (new)

**Design Constraints:**
- Represents a $2d$-mode discretized integral operator as multilevel interpolative
  decompositions (Tensor Butterfly)
- `apply(data_tt: TTTrain) → TTTrain` in O(n·d) time vs O(n·d·log n) for FFT
- Operator graphs pre-built for: Radon transform, Fourier transform,
  Green's function (wave equation)
- `contractOperator(data_tt, butterfly_op) → TTTrain` — "marriage" of data and operator graph

**Required Interfaces:**
- `TensorButterflyOperator::build(operator_type, grid_shape, precision)`
- `TensorButterflyOperator::apply(TTTrain&) → TTTrain`

**Test Strategy:**
- Unit: apply discrete Fourier butterfly to a sine wave represented as TT; compare
  to reference FFT result; relative error ≤ 1e-6
- Scaling: verify O(n·d) CPU time by timing at n ∈ {32, 64, 128, 256} on d=4 grid

**Performance Targets:**
- CPU time scales as O(n·d) (not O(n·d·log n))
- Memory: O(n·d) parameter storage vs O(n^d) for dense operator

---

### Enhancement: Phase 3 — FLARE Mid-Generation Retrieval Integration

**Target:** Q1 2027  
**Paper §:** Forward-Looking Active Retrieval (FLARE)

**Affected files:**
- `include/rag/flare_retriever.h` (new)
- `src/rag/flare_retriever.cpp` (new)
- `src/llm/llama_wrapper.cpp` — token-level log-prob hook

**Design Constraints:**
- Monitor per-token log-probability during generation
- When confidence < threshold: generate pseudo-sentence as retrieval query;
  trigger `ThemisRagClient::retrieve()` against TT-indexed corpus
- Re-generate uncertain tokens with retrieved context injected via zero-copy bridge
- Paper: each ThemisDB retrieval step ≤ 90ms total TTFT

**Required Interfaces:**
- `FLARERetriever::step(partial_output, threshold) → {context, regenerated}`
- `LlamaWrapper::tokenLogProbs() → std::vector<float>` — per-token probability hook

**Test Strategy:**
- Factuality: FLARE on TriviaQA/HotpotQA; compare hallucination rate vs. standard RAG
- Latency: each retrieval round-trip ≤ 90ms end-to-end

**Security / Reliability:**
- Pseudo-sentence queries sanitized before passing to AQL translator
- Retrieved context scoped to caller's tenant_id (isolation mandatory)

---

### Enhancement: Phase 3 — TARG Logit-Gap Gating

**Target:** Q2 2027  
**Paper §:** Training-free Adaptive Retrieval Gating (TARG)

**Affected files:**
- `include/rag/targ_gate.h` (new)
- `src/rag/targ_gate.cpp` (new)

**Design Constraints:**
- Single-shot draft (no context) to read model prefix logits
- Uncertainty score = top-1 logit − top-2 logit (logit gap)
- Retrieve only when gap < configurable threshold
- Paper: eliminates 70–90% of unnecessary retrieval calls; accuracy near Always-RAG

**Required Interfaces:**
- `TARGGate::shouldRetrieve(logits: std::vector<float>, threshold: float) → bool`
- `TARGGate::computeGap(logits) → float`

**Test Strategy:**
- MMLU benchmark: TARG accuracy within 2% of Always-RAG; latency ≤ 1.5× Never-RAG
- NQ (Natural Questions): retrieval call rate ≤ 30% of Always-RAG baseline

**Performance Targets:**
- `shouldRetrieve()`: ≤ 0.1ms per call (must not delay token generation)

---

### Enhancement: Phase 3 — Adapter Sovereignty (LoRA/PEFT as TT Graphs)

**Target:** Q2 2027  
**Paper §:** Adapter Sovereignty and PEFT Storage Paradigms

**Affected files:**
- `include/tensor/adapter_repository.h` (new)
- `src/tensor/adapter_repository.cpp` (new)
- `src/tensor/tensor_index_manager.cpp` — `loadAdapter()` hook

**Design Constraints:**
- Thousands of domain-specific LoRA adapters (legal, medical, scientific) stored as
  TT graph objects in ThemisDB
- `loadAdapter(domain, base_model_id)` maps adapter TT-cores into ggml graph JIT
  via zero-copy mmap — no model reload required
- Paper: "single base model switches between identities instantly"
- Static PEFT (LoRA weight update Δ = A·B) coexists with dynamic TT adapter injection

**Required Interfaces:**
- `AdapterRepository::store(domain, adapter_tt: TTTrain)` — persist adapter as TT graph
- `AdapterRepository::loadAdapter(domain, base_model_id) → GgmlCoreDescriptor`
- `AdapterRepository::listDomains() → std::vector<std::string>`

**Test Strategy:**
- Store legal and medical adapters; switch between them; verify perplexity within 0.5%
  of full LoRA load on respective domain benchmarks
- Adapter switch latency ≤ 50ms end-to-end

**Performance Targets:**
- Adapter switch: ≤ 50ms (mmap page-pin for hot adapters expected ≤ 5ms)
- Memory: no duplication of base model weights during adapter switch

**Security / Reliability:**
- Adapter key scoped to tenant_id; no cross-tenant adapter access
- HMAC provenance on adapter metadata (GGUF v3, same as core provenance)

---

### Enhancement: Phase 4 — CUDA TT-SVD in addFlat()

**Target:** Q4 2027  
**Paper §:** CUDA and FPGA Acceleration

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

### Enhancement: Phase 4 — Distributed TT Shard Layout + RAID-Sharding 2.0

**Target:** Q4 2027  
**Paper §:** Multi-Version Concurrency Control and Distributed Resilience — RAID-Sharding 2.0

**Affected files:**
- `src/sharding/adaptive_shard_router.cpp`
- `src/tensor/tensor_index_manager.cpp`

**Design Constraints:**
- Shard k stores `G_k` cores for all vectors in that tenant/collection
- Enables all-reduce contraction across shards without assembling full TT chain
- RAID-Sharding 2.0: node failure = missing index in global tensor graph;
  reconstruction via tensor completion algorithms (not Reed-Solomon parity)
- `RemoteExecutor` performs the contraction step for remote-shard cores

**Required Interfaces:**
- `TensorCompletionSolver::reconstruct(partial_cores, missing_shard_indices) → TTTrain`
- `ShardRouter::assignTTCore(core_index, num_shards) → shard_id`

**Implementation Notes:**
- Shard key: `__ttidx__:<tenant>:<collection>:<field>:shard<k>:<id>`
- Witness nodes participate in quorum but hold no TT-core data

**Performance Targets:**
- Distributed inner-product for rank-32 6D TT: ≤ 20ms across 6 shards at 1 Gbps
- Tensor completion reconstruction after single-node failure: ≤ 500ms for 10K vectors

---

### Enhancement: Phase 5 — Hierarchical Tucker (HT) Decomposition

**Target:** Q1 2028  
**Paper §:** HT storage O(d·n·r + d·r³); contraction O(d·n·r² + d·r⁴); high parallelizability

**Affected files:**
- `include/tensor/ht_train.h` (new) — `HTNode`, `HTTrain`
- `include/storage/hierarchical_tucker_decomposer.h` (new)
- `src/storage/hierarchical_tucker_decomposer.cpp` (new)
- `include/tensor/ht_index.h` (new) — `IHierarchicalTuckerIndex`
- `src/tensor/ht_index.cpp` (new)

**Design Constraints:**
- Binary tree of `HTNode { U_left, U_right, B_transfer }` tensors
- Left/right children can be contracted in parallel (independent subtrees)
- Storage: O(d·n·r + d·r³) validated by benchmark on 8D tensor
- `HTContractionEngine` parallelizes tree branches via `std::async`/thread pool

**Required Interfaces:**
- `HierarchicalTuckerDecomposer::decompose(tensor, shape, rank, epsilon) → HTTrain`
- `IHierarchicalTuckerIndex::add(id, HTTrain)` / `search(query, k)`
- `HTContractionEngine::innerProduct(HTTrain&, HTTrain&) → float`

**Test Strategy:**
- Storage benchmark: 8D tensor d=8, n=32, r=16; verify ≤ O(d·n·r + d·r³) bytes
- Contraction time: tree-parallel path ≥ 2× faster than sequential TT on same data
- Accuracy: reconstruction error ≤ ε vs. original dense tensor

**Performance Targets:**
- HT GPU tensor core acceleration: 32× vs. standard CUDA baseline (paper §Hardware)
- Memory: 30–50% less than equivalent TT representation for multi-scale data

---

### Enhancement: Phase 5 — Quantics Tensor Train (QTT)

**Target:** Q1 2028  
**Paper §:** QTT "logarithmic scaling relative to grid size... highly oscillatory integral operators"

**Affected files:**
- `include/storage/quantics_tt_decomposer.h` (new)
- `src/storage/quantics_tt_decomposer.cpp` (new)

**Design Constraints:**
- Each physical dimension further factorized via binary representation
  (grid size n → log₂(n) QTT dimensions)
- `QTTConfig::bit_depth` per dimension; defaults to `ceil(log2(n))`
- Supports prime-number factorizations for non-power-of-2 grids
- Compatible with `ITensorIndex::add()` via `QTTrain → TTTrain` flattening

**Required Interfaces:**
- `QuanticsTTDecomposer::decompose(tensor, grid_sizes, bit_depths, epsilon) → QTTrain`
- `QTTrain::toTTTrain() → TTTrain` — flatten for existing index compatibility

**Test Strategy:**
- Maxwellian velocity distribution d=6, n=64: QTT compression ratio ≥ 10× vs. TT
- OIO (Green's function): QTT vs. FFT on wave equation; error ≤ 1e-8
- Round-trip: QTTrain → TTTrain → QTTrain with no rank inflation

**Performance Targets:**
- Compression ratio: ≥ 10× over plain TT on Maxwellian test data
- Decomposition time: ≤ 50ms for n=64 6D tensor

---

### Enhancement: Phase 5 — FPGA Bi-Directional Contraction for Edge Devices

**Target:** Q2 2028  
**Paper §:** FPGA Accelerators — "4.0x energy reduction, TT-compressed transformers on edge"

**Affected files:**
- `include/acceleration/fpga_contraction_driver.h` (new)
- `src/acceleration/fpga_contraction_driver.cpp` (new)

**Design Constraints:**
- OpenCL kernel on Alveo U55C; bi-directional contraction flow for TT-transformer layers
- Custom dataflows and explicit memory control suited to "tiny and sequential" contractions
- `FPGAContractionDriver` wraps OpenCL queue; falls back to CPU when FPGA unavailable

**Required Interfaces:**
- `FPGAContractionDriver::contract(TTTrain&, query_tt) → float`
- `FPGAContractionDriver::isAvailable() → bool`

**Performance Targets:**
- Energy: ≤ 25% of equivalent GPU run (4× energy reduction vs. A100)
- Latency: ≤ 10ms per TT inner-product on Alveo U55C at d=6, r=32

---

### Enhancement: Phase 6 — Hiss Structural Search Engine

**Target:** Q2 2028  
**Paper §:** The Hiss Framework — TN-SS with index reshaping

**Affected files:**
- `include/tensor/hiss_structural_search.h` (new)
- `src/tensor/hiss_structural_search.cpp` (new)

**Design Constraints:**
- Combines global stochastic sub-network sampling with local hierarchical refinement
- Entropy-guided index clustering to reduce dimensionality before search
- `HissConfig { num_samples, entropy_threshold, max_reshape_depth, diversity_budget }`
- Result: `TensorNetworkGraph` — an optimized TN topology for the input data class

**Required Interfaces:**
- `HissStructuralSearchEngine::search(data_tensor, config) → TensorNetworkGraph`
- `HissReshaper::exposeQuantics(train, grid_sizes) → QTTrain`
- `TemplateCatalog::registerTemplate(domain_tag, graph_template)`
- `TemplateCatalog::lookup(domain_tag) → std::optional<TensorNetworkGraph>`

**Implementation Notes:**
- STUB/SIMULATION NOTE required on initial sampling kernel until full stochastic
  search is implemented
- Template transfer: structures from instance A applied to instance B within 10% perf
  loss (paper §Hiss)

**Test Strategy:**
- Compression ratio 2.5×–100× vs. fixed TT/HT on domain-specific test corpus
- Template transfer: new dataset from same domain achieves ≥ 90% recall of Hiss-optimized

**Performance Targets:**
- Structure search: ≤ 10s for a 10K-sample corpus (offline; not on hot path)

---

### Enhancement: Phase 6 — TNSR Background Maintenance Task

**Target:** Q3 2028  
**Paper §:** TNSR — "generalizes the search to refine arbitrary existing tree networks by
both adjusting bond dimensions and reconfiguring the topology"

**Affected files:**
- `include/tensor/tnsr_task.h` (new)
- `src/tensor/tnsr_task.cpp` (new)
- `src/storage/tensor_network_storage_engine.cpp` — schedule TNSR task

**Design Constraints:**
- Runs in RocksDB compaction thread pool (reuses existing threading infrastructure)
- Adjusts both bond dimensions AND network topology (unlike rank-adaptive compaction
  which only changes ranks)
- `TNSRConfig { epsilon, max_topology_changes_per_run, min_bytes_saved_to_commit }`
- `TNSRReport { bytes_saved, rank_delta, topology_changes }` emitted to metrics

**Required Interfaces:**
- `TNSRTask::run(index_key_range, config) → TNSRReport`
- `TensorNetworkGraph::rerouteEdge(node_a, node_b, new_topology)`

**Test Strategy:**
- Live index with 10K vectors over 24h: storage decrease ≥ 15%
- Accuracy regression: cosine similarity δ < 0.001 before vs. after TNSR

**Performance Targets:**
- TNSR overhead: ≤ 5% CPU increase over baseline compaction
- Run frequency: at most once per 6h per index (configurable)

---

### Enhancement: Phase 7 — Unified Tensor Representation (UTR)

**Target:** Q3–Q4 2028  
**Paper §:** UTR for Multi-Modal Semantic Interoperability

**Affected files:**
- `include/tensor/utr_converter.h` (new)
- `src/tensor/utr_converter.cpp` (new)
- `include/tensor/hyper_index_builder.h` (new)

**Design Constraints:**
- `UTRConverter` produces tensor-native representations for all major data types
- Common `TensorAddOptions::data_type` enum: `GEOSPATIAL`, `RELATIONAL`, `IMAGE`,
  `DOCUMENT`, `SCIENTIFIC`, `CUSTOM`
- All UTR outputs are valid `TTTrain` or `HTTrain` objects indexable by `TensorIndexManager`

**Required Interfaces:**
- `UTRConverter::fromGeospatial(grid: RasterGrid, resolution) → TTTrain`
- `UTRConverter::fromTabular(table, schema) → HyperIndexTensor`
- `UTRConverter::fromImage(pixels, h, w, c) → TTTrain`
- `UTRConverter::fromDocument(text, structure_hint) → HTTrain`

**Test Strategy:**
- Geospatial: spatial correlation flood_risk × population via TT contraction within 0.1%
  of raster baseline
- Relational: TPC-H Q18 latent join detected without explicit JOIN hint
- Document: recall@5 ≥ 3× flat-chunk baseline on government-document benchmark
- Image: structural similarity search on CIFAR-10 within 2% of dense CLIP baseline

**Performance Targets:**
- `fromTabular()`: ≤ 2s for a 1M-row table with 20 columns
- `fromDocument()`: ≤ 100ms per document page

**Security / Reliability:**
- Geospatial data: no coordinate precision loss > 1e-7 degrees after UTR round-trip
- Relational data: Hyper-Index must not expose values from other tenants' rows

---

### Enhancement: Phase 8 — Physics-Informed Vlasov-Maxwell Solver

**Target:** Q1 2029  
**Paper §:** Physics-Informed Kinetic Solvers and Scientific AI Agents

**Affected files:**
- `include/tensor/vlasov_maxwell_solver.h` (new)
- `src/tensor/vlasov_maxwell_solver.cpp` (new)
- `include/tensor/mimetic_tt_curl.h` (new)

**Design Constraints:**
- Distribution function f(x,y,z,vₓ,vy,vz) stored in HT format (rank-1 in velocity for
  Maxwellian; paper: near-lossless at 1000× compression)
- Time-stepping via spectral splitting entirely in compressed space; 6D grid never
  reconstructed
- Mimetic curl operator in TT format: ∇·B ≤ machine epsilon (1e-14)
- Charge density: contraction over velocity modes → source term for spectral Poisson

**Required Interfaces:**
- `VlasovMaxwellSolver::step(f_ht: HTTrain, dt) → HTTrain`
- `MimeticTTCurl::apply(B_tt: TTTrain, dx) → TTTrain`
- `SnapshotLearner::inferOperator(snapshots: std::vector<HTTrain>) → DiscreteOperatorTN`

**Test Strategy:**
- Landau damping: damping rate matches analytic dispersion relation within 1% over
  100 time steps without full-grid reconstruction
- ∇·B test: apply mimetic curl; assert ∇·B ≤ 1e-14 on uniform grid
- Snapshot learning: infer operator from 50 training snapshots; predict step 51
  within ε on held-out simulation

**Performance Targets:**
- Single time step: ≤ 1s for n=32 per dimension (N⁶ = 1.07B equivalent grid points)
- Storage: ≤ 1 MB for d=6, r=10 HT representation vs. 8.59 GB dense (paper benchmark)

---

## Summary Table

| Enhancement | Phase | Target | Stub IDs | Priority |
|-------------|-------|--------|----------|----------|
| hnswlib integration | 2 | Q4 2026 | HTB-01 | 🔴 Critical |
| RocksDB persistence | 2 | Q4 2026 | TTI-01, TTI-02, TIM-02, HTB-02, HTB-03 | 🔴 Critical |
| Rank-adaptive compaction filter | 2 | Q4 2026 | — | 🟠 High |
| Zero-Copy GGML bridge | 3 | Q1 2027 | TIM-01 | 🟠 High |
| GGUF v3 metadata provenance | 3 | Q1 2027 | — | 🟠 High |
| TENSOR_CONTRACTION plan-node | 3 | Q1 2027 | — | 🟠 High |
| FLARE mid-generation retrieval | 3 | Q1 2027 | — | 🟠 High |
| AQL CONTRACT/PROJECT/DECOMPOSE | 3 | Q2 2027 | — | 🟡 Medium |
| Tensor Butterfly operator | 3 | Q2 2027 | — | 🟡 Medium |
| TARG logit-gap gating | 3 | Q2 2027 | — | 🟡 Medium |
| Adapter sovereignty (LoRA as TT) | 3 | Q2 2027 | — | 🟡 Medium |
| CUDA TT-SVD | 4 | Q4 2027 | — | 🟡 Medium |
| Distributed shard layout + RAID-Sharding 2.0 | 4 | Q4 2027 | — | 🟡 Medium |
| HT decomposition | 5 | Q1 2028 | — | 🟡 Medium |
| QTT decomposition | 5 | Q1 2028 | — | 🟡 Medium |
| HT GPU tensor core kernel | 5 | Q2 2028 | — | 🟢 Low |
| FPGA bi-directional contraction | 5 | Q2 2028 | — | 🟢 Low |
| Hiss structural search engine | 6 | Q2 2028 | — | 🟢 Low |
| TNSR background task | 6 | Q3 2028 | — | 🟢 Low |
| UTR multi-modal converter | 7 | Q3–Q4 2028 | — | 🟢 Low |
| Vlasov-Maxwell spectral solver | 8 | Q1 2029 | — | 🟢 Low |
| Mimetic TT curl operators | 8 | Q1 2029 | — | 🟢 Low |
| Scientific RAG snapshot learning | 8 | Q2 2029 | — | 🟢 Low |
