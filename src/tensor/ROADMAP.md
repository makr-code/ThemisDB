> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Tensor Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

> **Scientific Basis:** This roadmap is aligned with the architectural model described in
> *"Advanced Architectural Paradigms for Multi-Model AI Databases: Integrating Tensor Graph
> Networks and Zero-Copy Inference in ThemisDB"* (2026).  Every phase maps directly to
> concepts from that paper.  See `docs/research/best_practices/tensor_train_storage.md`
> and `docs/research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md` for further background.

## Current Status

Experimental — Phase 1 skeleton complete (2026-05-05).  Core interfaces
(`ITensorIndex`, `TensorIndexManager`, `HnswTTBridge`) and a linear-scan
reference implementation (`FlatTensorIndex`) are in place.
RocksDB persistence and hnswlib integration are Phase 2 targets.

## Completed ✅

- [x] `ITensorIndex` interface with add/search/norm/innerProduct/save/load
- [x] `FlatTensorIndex` — Phase-1 linear-scan reference implementation
- [x] `TensorIndexManager` — lifecycle registry, routing, tenant isolation
- [x] `HnswTTBridge` header + skeleton with two-layer architecture
- [x] TT inner-product sweep (Holtz 2012) in O(d·r²)
- [x] First-core sketch extraction for HNSW navigation layer
- [x] Module docs: README, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT
- [x] Research docs: boundary analysis, arXiv draft, AdaLoRA bridge
- [x] STUB_INVENTORY #150–#157 registered (STUB/SIMULATION NOTEs in all stubs)

## In Progress 🚧

- [~] Stub entries registered in `src/STUB_INVENTORY.md` (TTI-01..02, TIM-01..02, HTB-01..03)

## Planned Features 📋

### Short-term (Phase 2, Q4 2026)

- [ ] Wire `HnswTTBridge::HnswLayer` to hnswlib `HierarchicalNSW<float>` (Target: Q4 2026)
  - Sketches from first TT-core (dim = min(n₁, `sketch_dim`))
  - Build-time: no full vector needed; only first-core required for HNSW graph
  - Navigation accuracy: recall@10 ≥ 0.95 vs. brute-force on 1 M 4096-dim vectors
  - Re-rank on TT-domain cosine at O(d·r²); throughput target ≥ 741 ops/s (paper §Similarity Search)

- [ ] RocksDB persistence for `FlatTensorIndex` and `HnswTTBridge` (Target: Q4 2026)
  - Key schema: `__ttidx__:<tenant>:<collection>:<field>:<id>:G<k>:<version>`
  - save() writes all TT-cores atomically via WriteBatch
  - load() rebuilds in-memory index and HNSW graph

- [ ] `TensorIndexManager::dropTenantIndexes()` — RocksDB prefix-delete (Target: Q4 2026)
  - Use `DeleteRange(__ttmgr__:<tenant_id>:, __ttmgr__:<tenant_id+1>:)`

- [ ] `TensorIndexManager` wired to `TensorNetworkStorageEngine` for core persistence (Target: Q4 2026)

- [ ] **Rank-adaptive RocksDB compaction filter** (Target: Q4 2026)
  - Paper §RocksDB Integration: background compaction re-compresses TT-cores when
    local data entropy decreases
  - `TensorCompactionFilter::Filter()` invokes `TensorTrainDecomposer::recompress()`
    with tolerance `ε` read from column-family option `tensor.recompress_epsilon`
  - AC: storage reduction ≥ 20% on a Maxwellian-shaped embedding corpus without
    exceeding prescribed `ε` accuracy loss

### Medium-term (Phase 3, Q1–Q2 2027)

- [ ] `TensorIndexManager::ggmlCorePtrs()` — null-pointer mmap handshake (Target: Q1 2027)
  - Pin TT-core pages via `mmap(MAP_SHARED)` or `cudaHostRegister`
  - Return `GgmlCoreDescriptor { ggml_tensor*, size_t core_idx }` per core
  - Zero-copy path: DB → VRAM without intermediate CPU buffer
  - Apple Silicon: unified memory pool; no PCIe transfer
  - AC: TTFT reduction ≥ 3× vs. classical JSON-RAG path (paper §Zero-Copy RAG table)

- [ ] `GgmlTensorBridge` — `GGML_TYPE_TT` registration (Target: Q1 2027)
  - New `ggml_type` enum value; `ggml_map_custom1` op for O(d·r²) contraction
  - GGUF v3 metadata: attach provenance key-values (filename, page, line) per core
    (paper §GGUF Metadata; required for regulated industries)
  - AC: inference output within ±ε of decompressed reference

- [ ] **FLARE mid-generation retrieval integration** (Target: Q1 2027)
  - Paper §FLARE: monitor per-token log-probability; trigger re-retrieval when
    confidence < threshold
  - `ThemisRagClient::flareStep(partial_output, threshold)` → new TT retrieval call
  - AC: factuality improvement measurable on TriviaQA/HotpotQA; TTFT per step ≤ 90ms
    (paper table: ThemisDB accelerates each FLARE round-trip)

- [ ] **TARG logit-gap gating** (Target: Q2 2027)
  - Paper §TARG: single-shot draft measures top-1/top-2 logit gap; triggers retrieval
    only when gap < threshold; eliminates 70-90% of unnecessary retrieval calls
  - `TARGRetrieval::shouldRetrieve(logits) → bool`
  - AC: ≤ 1.5× latency vs. Never-RAG baseline while matching Always-RAG accuracy
    on MMLU / NQ benchmarks

- [ ] `TensorAwareQueryOptimizer` plan-node `TENSOR_CONTRACTION` routing (Target: Q1 2027)
  - Detect TT-stored operands in AQL plan
  - Route to `TensorContractionEngine` instead of reconstructing flat vectors

- [ ] **AQL tensor-aware operators: CONTRACT, PROJECT, DECOMPOSE** (Target: Q2 2027)
  - Paper §AQL: structure-oriented query language; topology is the primary object
  - `CONTRACT(a, b, mode_list)` — multi-mode tensor contraction, returns TTTrain
  - `PROJECT(t, mode)` — marginalization by contracting over specified modes
  - `DECOMPOSE(field, shape, rank, epsilon)` — on-the-fly TT decomposition within query
  - AC: AQL EXPLAIN shows operator nodes; integration test verifies math vs. NumPy

- [ ] **Tensor Butterfly algorithm for oscillatory integral operators** (Target: Q2 2027)
  - Paper §Operator Compression: O(n·d) vs O(n·d·log n) FFT for Radon/Fourier transforms
  - `TensorButterflyOperator` pre-defined as a multilevel tensor network (TN graph)
  - Operator "marriage" with data graphs via `contractOperator(data_tt, butterfly_op)`
  - AC: CPU time scales as O(n·d); validated against reference FFT on Gaussian test case

- [ ] **Adapter sovereignty — LoRA/PEFT store as TT graphs** (Target: Q2 2027)
  - Paper §Adapter Sovereignty: thousands of domain-specific LoRAs stored as TT graphs
  - `AdapterRepository::loadAdapter(domain, base_model_id)` mmap-injects adapter cores
    into ggml graph just-in-time; no model reload required
  - AC: adapter switch latency ≤ 50ms; correctness: perplexity within 0.5% of full load

### Long-term (Phase 4, Q2–Q4 2027)

- [ ] `AdaLoRA ↔ TT Bridge` Phase 3 — `findSimilarAdapters()` via TensorFingerprintGraph (Target: Q3 2027)
- [ ] Distributed TT shard layout (one shard per TT-core index) (Target: Q4 2027)
  - Compatible with RAID-Sharding 2.0 tensor-based reconstruction
    (paper §RAID-Sharding 2.0: node failure = missing tensor index; rebuilt via
    tensor completion algorithms)
- [ ] CUDA cuSOLVER backend for on-GPU TT-SVD during `addFlat()` (Target: Q4 2027)
  - Paper §GPU Tensor Cores: HT decomposition on A100 tensor cores achieves 32× vs
    standard CUDA; implement HT contraction path alongside TT

### Phase 5: Hierarchical Tucker (HT) + Quantics TT (QTT) (Target: Q1–Q2 2028)

- [ ] **`HierarchicalTuckerDecomposer`** — binary-tree tensor factorization (Target: Q1 2028)
  - Paper §HT: storage O(d·n·r + d·r³); parallelizable tree branches
  - `HTTrain` type: binary tree of `HTNode { U_left, U_right, B_transfer }`
  - `IHierarchicalTuckerIndex` parallel to `ITensorIndex`; tree-branch ops parallelized
    via thread pool
  - AC: storage ≤ O(d·n·r + d·r³); contraction ≤ O(d·n·r² + d·r⁴)

- [ ] **`QuanticsTensorTrainDecomposer`** — QTT for multi-scale data (Target: Q1 2028)
  - Paper §QTT: each physical dimension further factorized in binary (log₂ n factors);
    logarithmic scaling relative to grid size
  - Enables exact representation of oscillatory integral operators (OIOs) and
    multi-scale Green's functions for wave equations
  - `QTTConfig::bit_depth` controls per-dimension binary factorization
  - AC: compression ratio ≥ 10× vs. standard TT on Maxwellian velocity distribution
    test data; reconstruction error ≤ ε = 1e-8

- [ ] **HT GPU tensor core acceleration** (Target: Q2 2028)
  - Paper §GPU Tensor Cores: HT on A100 tensor cores achieves 32× vs. standard CUDA
  - `HTContractionKernel.cu` — warp-level tree-branch parallelism
  - AC: 32× speedup vs. CUDA baseline; validated on d=8, n=32, r=32 test tensor

- [ ] **FPGA bi-directional contraction for edge TT-transformers** (Target: Q2 2028)
  - Paper §FPGA: bi-directional contraction flows; 4× energy reduction for TT-transformers
  - `FPGAContractionDriver` — thin wrapper over Alveo OpenCL kernel
  - AC: energy cost ≤ 25% of GPU baseline; validated on Alveo U55C

### Phase 6: Hiss Framework — Adaptive Structural Rounding (Target: Q2–Q3 2028)

- [ ] **`HissStructuralSearchEngine`** — TN-SS with index reshaping (Target: Q2 2028)
  - Paper §Hiss: global stochastic sub-network sampling + local hierarchical refinement;
    entropy-guided index clustering
  - `HissConfig { num_samples, entropy_threshold, max_reshape_depth }`
  - `HissStructuralSearchEngine::search(data_tensor, config) → TensorNetworkGraph`
  - AC: compression ratio 2.5×–100× better than fixed TT/HT on test corpus;
    structures from one data instance transfer to similar instances within 10% perf

- [ ] **Targeted index reshaping to expose QTT/latent-rank structures** (Target: Q2 2028)
  - Hiss reshapes native indices to reveal Quantics formats invisible in original layout
  - `HissReshaper::exposeQuantics(train, grid_sizes) → QTTrain`
  - AC: QTT compression ≥ 2.5× better than plain TT on same data after reshaping

- [ ] **`TensorNetworkStructuralRounding` (TNSR) — background task** (Target: Q3 2028)
  - Paper §TNSR: generalizes round to arbitrary existing tree networks; adjusts bond
    dimensions AND reconfigures topology as background maintenance
  - `TNSRTask` runs in RocksDB compaction thread pool; uses same `ε` tolerance as
    rank-adaptive compaction filter
  - `TNSRReport { bytes_saved, rank_delta, topology_changes }` written to metrics
  - AC: storage decrease ≥ 15% over 24h on a live index with ongoing inserts, without
    measurable accuracy regression (cosine sim δ < 0.001 vs. pre-TNSR)

- [ ] **Domain template graphs** — reuse structure across similar datasets (Target: Q3 2028)
  - Paper §Hiss: optimized structure for one instance maintains ≤10% perf on similar data
  - `TemplateCatalog::register(domain_tag, tn_graph_template)`
  - Automatic template selection by `TensorRouter` based on `domain_tag` metadata
  - AC: first-time search on new dataset using template within 10% of Hiss-optimized recall

### Phase 7: Unified Tensor Representation (UTR) — Multi-Modal Interoperability (Target: Q3–Q4 2028)

- [ ] **`UTRConverter` — heterogeneous-to-tensor-native pipeline** (Target: Q3 2028)
  - Paper §UTR: geospatial, relational, visual, and textual data unified via TT/HT encoding
  - `UTRConverter::fromGeospatial(grid, resolution) → TTTrain`  — preserves topological neighborhoods
  - `UTRConverter::fromTabular(table, schema) → HyperIndexTensor`  — relational Hyper-Index with latent joins
  - `UTRConverter::fromImage(pixels, h, w, c) → TTTrain`  — 3D/4D core network for structural similarity
  - `UTRConverter::fromDocument(text, structure_hint) → HTTrain`  — hierarchical paragraph cores
  - AC: round-trip encode/decode with normalized RMSE ≤ ε per data type

- [ ] **Geospatial TT-cores preserving topological proximity** (Target: Q3 2028)
  - Paper §Geospatial: n-dimensional grids factorized; spatial reasoning by core contraction
  - `GeoTTIndex::spatialContraction(flood_risk_tt, population_tt) → correlation_score`
  - AC: spatial correlation result within 0.1% of raster-based baseline

- [ ] **Relational Hyper-Index with latent join discovery** (Target: Q4 2028)
  - Paper §Relational: Hyper-Index tensor extracts cross-table relationships invisible to
    the relational engine
  - `HyperIndexBuilder::fromSchema(tables, fk_graph) → HyperIndexTensor`
  - AC: latent join for standard TPC-H Q18 detected without explicit JOIN hint

- [ ] **Hierarchical document tensor — child-to-parent retrieval** (Target: Q4 2028)
  - Paper §Documents: 3–5× better accuracy on structured data via structural context retention
  - `DocumentHTIndex::retrieveFragment(query_tt, k) → { fragment, parent_context }`
  - AC: recall@5 on government-document benchmark ≥ 3× vs. flat chunk retrieval

### Phase 8: Physics-Informed Scientific Solvers (Target: Q1–Q2 2029)

- [ ] **Spectral TT solver for Vlasov-Maxwell system** (Target: Q1 2029)
  - Paper §Physics: distribution function f(x,y,z,vₓ,vy,vz) in HT format;
    time-stepping entirely in compressed space; N⁶ grid never reconstructed
  - `VlasovMaxwellSolver::step(f_ht, dt) → f_ht_next`
  - Charge density via velocity-mode contraction; spectral Poisson solver for fields
  - AC: Landau damping instability detected without full-grid reconstruction;
    relative error vs. reference PIC simulation ≤ 1%

- [ ] **Mimetic finite-difference curl operators in TT format** (Target: Q1 2029)
  - Paper §Mimetic: zero discrete divergence ∇·B ≈ machine epsilon; O(d·n·r²) vs dense curl
  - `MimeticTTCurl::apply(B_tt, dx) → curl_tt`
  - AC: ∇·B ≤ 1e-14 on uniform grid; CPU time ≤ O(d·n·r²) measured

- [ ] **Scientific RAG — snapshot learning from TT simulation states** (Target: Q2 2029)
  - Paper §Scientific RAG: discrete operators inferred from stored tensor snapshots;
    predict future dynamics or physical instabilities
  - `SnapshotLearner::inferOperator(snapshot_series: [HTTrain]) → DiscreteOperatorTN`
  - AC: operator predicts next snapshot within ε on held-out test sequence

## Implementation Phases

### Phase 1: Interface & Reference (Status: ✅ Complete — 2026-05-05)

- [x] `ITensorIndex` — interface definition
- [x] `FlatTensorIndex` — linear-scan impl
- [x] `TensorIndexManager` — lifecycle / routing
- [x] `HnswTTBridge` — header + skeleton
- [x] TT arithmetic: inner-product (O(d·r²)), norm, cosine similarity

### Phase 2: Production Core (Target: Q4 2026)

- [ ] hnswlib integration in `HnswTTBridge::HnswLayer`
- [ ] RocksDB persistence (save/load for all backends)
- [ ] Tenant prefix-delete in `dropTenantIndexes()`
- [ ] `TensorIndexManager` → `TensorNetworkStorageEngine` wire-up
- [ ] Rank-adaptive RocksDB compaction filter (`TensorCompactionFilter`)
- [ ] Phase-2 tests: TTI-10..20, HTB-10..20

### Phase 3: Zero-Copy Inference + Advanced RAG (Target: Q1–Q2 2027)

- [ ] `ggmlCorePtrs()` — null-pointer mmap handshake (TIM-01)
- [ ] `GgmlTensorBridge` — `GGML_TYPE_TT` + GGUF v3 metadata provenance
- [ ] `TensorAwareQueryOptimizer` TENSOR_CONTRACTION plan-node
- [ ] AQL operators: CONTRACT, PROJECT, DECOMPOSE
- [ ] Tensor Butterfly operator for O(n·d) oscillatory integral transforms
- [ ] FLARE mid-generation retrieval (TTFT per step ≤ 90ms)
- [ ] TARG logit-gap gating (70–90% unnecessary retrieval calls eliminated)
- [ ] Adapter sovereignty — LoRA/PEFT as TT graphs (adapter switch ≤ 50ms)

### Phase 4: Distributed & GPU (Target: Q2–Q4 2027)

- [ ] Per-core shard distribution over Themis-Net + RAID-Sharding 2.0 tensor completion
- [ ] CUDA cuSOLVER TT-SVD during `add()` (≤ 80ms for 10⁶-element 6D tensor)
- [ ] HT contraction on A100 GPU tensor cores (32× vs CUDA baseline)
- [ ] AdaLoRA `findSimilarAdapters()` wire-up

### Phase 5: HT + QTT Formats (Target: Q1–Q2 2028)

- [ ] `HierarchicalTuckerDecomposer` + `IHierarchicalTuckerIndex`
- [ ] `QuanticsTensorTrainDecomposer` for multi-scale / OIO data
- [ ] HT GPU tensor core kernel (`HTContractionKernel.cu`)
- [ ] FPGA bi-directional contraction driver for edge devices

### Phase 6: Hiss Adaptive Structural Rounding (Target: Q2–Q3 2028)

- [ ] `HissStructuralSearchEngine` — TN-SS with entropy-guided clustering
- [ ] Targeted index reshaping to expose QTT latent structures
- [ ] `TensorNetworkStructuralRounding` (TNSR) background maintenance task
- [ ] Domain template graph catalog (`TemplateCatalog`)

### Phase 7: UTR Multi-Modal Interoperability (Target: Q3–Q4 2028)

- [ ] `UTRConverter` — geospatial / relational / visual / document TT encoding
- [ ] Geospatial TT-cores with topological proximity preservation
- [ ] Relational Hyper-Index with latent join discovery
- [ ] Hierarchical document HTTrain child-to-parent retrieval

### Phase 8: Physics-Informed Scientific Solvers (Target: Q1–Q2 2029)

- [ ] Spectral TT Vlasov-Maxwell solver (full 6D phase space in HT)
- [ ] Mimetic TT curl operators (∇·B ≤ machine epsilon)
- [ ] Scientific RAG snapshot learning (`SnapshotLearner`)

## Production Readiness Checklist

- [ ] All stub entries resolved (TTI-01..02, TIM-01..02, HTB-01..03)
- [ ] hnswlib integration tested with recall@10 ≥ 0.95
- [ ] RocksDB persistence tested with crash recovery
- [ ] Rank-adaptive compaction filter tested for correctness + storage reduction
- [ ] GGML bridge validated end-to-end with llama.cpp (GGUF v3 metadata round-trip)
- [ ] FLARE + TARG integration tests on MMLU/NQ benchmarks
- [ ] Security audit: tenant key isolation; mmap fence prevents cross-tenant access
- [ ] Performance benchmarks: search latency vs. HNSW at dim=4096
- [ ] Throughput target ≥ 741 ops/s large-blob search (per paper §Similarity Search)

## Known Issues & Limitations

- `FlatTensorIndex::search()` is O(n·d·r²) — only suitable for n ≤ 50 K
- 1-D TT-decomposition in `addFlat()` loses inter-mode correlations;
  multi-mode reshape requires explicit `shape` parameter
- `HnswTTBridge::HnswLayer` is a linear-scan stub (HTB-01)
- TT inner-product sweep is correct but not SIMD-optimised in Phase 1
- `simpleSVD()` sets U/Vt to identity matrices (STUB_INVENTORY #157);
  reconstruction error can reach ‖T‖_F when THEMIS_USE_LAPACK_SVD is not set
- HT and QTT formats not yet implemented (Phases 5+)
- Hiss/TNSR adaptive rounding not yet integrated (Phase 6)

## Breaking Changes

| Version | Change |
|---------|--------|
| v2.0.0  | `ITensorIndex::add()` signature may gain `TTAddOptions` for rank override |
| Phase 3 | `ggmlCorePtrs()` return type will change to `GgmlCoreDescriptor` struct |
| Phase 5 | New `ITensorIndex` variant `IHierarchicalTuckerIndex` added; routed via `TensorIndexManager::routeHT()` |
| Phase 6 | `TensorRouter::route()` gains `HissConfig` optional parameter for structure-search override |
| Phase 7 | `UTRConverter` adds new field types to `TensorAddOptions`; existing callers unaffected |
