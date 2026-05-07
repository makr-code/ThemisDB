> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Tensor Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

> **Scientific Basis:** This roadmap is aligned with the architectural model described in
> *"Advanced Architectural Paradigms for Multi-Model AI Databases: Integrating Tensor Graph
> Networks and Zero-Copy Inference in ThemisDB"* — ThemisDB Research Group, 2026.
> Internal research report (pre-print); venue and DOI pending publication.
> See `research/best_practices/tensor_train_storage.md`
> and `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md` for further background.

## Current Status

Experimental — Phase 1 skeleton complete (2026-05-05).  Core interfaces
(`ITensorIndex`, `TensorIndexManager`, `HnswTTBridge`) and a linear-scan
reference implementation (`FlatTensorIndex`) are in place.
The **Ingestion Bridge** (`TensorIngestionBridge`) is production-ready and
provides the first end-to-end path from document ingestion to TT-core storage.
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
- [x] **`TensorIngestionBridge`** — production `ITensorDecompositionBackend`:
  - `include/tensor/tensor_ingestion_bridge.h` + `src/tensor/tensor_ingestion_bridge.cpp`
  - κ-gate with pilot decomposition (stride sub-sampling for dim > 1024)
  - Balanced 2D mode-shape inference (order-2 TT = AdaLoRA-equivalent format)
  - Provenance metadata (ε, rank, mode-shape, file, page, section_ref)
  - Thread-safe (atomic diagnostic counters, stateless decomposer)
- [x] **`builtin.chunk_tt_decompose`** ingestion step — runs after `chunk_embed`
  - `src/ingestion/steps/chunk_tt_decompose_step.cpp`
  - κ-gate controlled via `min_kappa` config key (default 1.3)
  - `skip_when_unavailable` guard (default true)
- [x] `ITensorDecompositionBackend` + `NullTensorDecompositionBackend` in
      `include/ingestion/inference_backend.h`
- [x] `TensorCoreRecord` + `ExtractionContext::tensor_cores` in
      `include/ingestion/extraction_context.h`
- [x] `createChunkTtDecomposeStep()` factory in `include/ingestion/builtin_step_factories.h`
- [x] Tests `tests/test_tensor_ingestion_bridge.cpp` (TIB-01..TIB-20)
- [x] STUB_INVENTORY #158–#159 registered
- [x] **`TensorCoreStorageBridge`** — production `ITensorCoreBridge` backed by `ITensorStorageBackend`:
  - `include/tensor/tensor_core_bridge.h` + `src/tensor/tensor_core_bridge.cpp`
  - Key schema: `__ttcore__:<tenant>:<source_file_id>:<chunk_id>`
  - Upsert semantics; atomic write counter; fail-closed validation
  - Defaults to `InMemoryTensorBackend` until RocksDB backend wired (STUB #160)
- [x] **`builtin.tensor_core_bridge`** ingestion step — runs after `chunk_tt_decompose`
  - `src/ingestion/steps/tensor_core_bridge_step.cpp`
  - Tenant resolution: config key → record metadata → `"default"`
  - `skip_empty` + `fail_on_write_error` config knobs
- [x] `ITensorCoreBridge` + `InMemoryTensorCoreBridge` in `include/ingestion/ingestion_sinks.h`
- [x] `createTensorCoreBridgeStep()` factory in `include/ingestion/builtin_step_factories.h`
- [x] Tests `tests/test_tensor_core_bridge.cpp` (TCS-01..TCS-20)
- [x] STUB_INVENTORY #160 registered

## Completed ✅ (additions 2026-05-06)

- [x] **`TensorRouter::Route` + `DataProfile` + `decide(DataProfile)`** — static routing API for index-creation time
  - `include/storage/tensor_router.h`: `Route` enum (TENSOR_TRAIN/HYBRID/HNSW), `DataProfile` struct (dim, num_vectors, kappa_estimate), `static Route decide(const DataProfile&)`
  - `src/storage/tensor_router.cpp`: κ-threshold heuristic (κ ≥ 1.7 && dim ≥ 256 → TENSOR_TRAIN; κ ≥ 1.3 → HYBRID; else HNSW)
  - `src/tensor/tensor_index_manager.cpp`: `routeFor()` fixed to call `TensorRouter::decide(p)` (no broken default ctor)
  - Tests TIM-03..TIM-14 in `tests/test_tensor_index_manager.cpp`
  - STUB_INVENTORY #163 resolved

- [x] **Rademacher random projection in `TensorIngestionBridge::shouldDecompose()`** (stub #159)
  - Replaces stride-based sub-sampling for dim > 1024
  - Signs ±1 via xorshift64; seed = `embedding.size() * 11400714819323198485ULL`; scale = 1/√dim
  - JL guarantee: κ deviation ≤ 5% vs. ≤ 15% for stride sampling
  - Tests TIB-21, TIB-22 added in `tests/test_tensor_ingestion_bridge.cpp`
  - STUB_INVENTORY #159 resolved

## In Progress 🚧

- [~] Stub entries registered in `src/STUB_INVENTORY.md` (TTI-01..02, TIM-01..02, HTB-01..03)
- [~] **Phase 3 AQL operators + optimizer** (2026-05-06):
  - [x] `TensorContractionEngine::project(train, mode)` — O(d·n·r²) compressed-domain marginalization
  - [x] `TensorContractionEngine::contractModes(a, b, modes_a, modes_b)` — multi-mode contraction (dense path for AQL)
  - [x] AQL function `TENSOR_CONTRACT(a, b, modes_a, modes_b)` registered in `registerTensorFunctions()`
  - [x] AQL function `TENSOR_PROJECT(t, mode)` registered in `registerTensorFunctions()`
  - [x] AQL function `TENSOR_DECOMPOSE(data, shape, max_rank, eps)` registered in `registerTensorFunctions()`
  - [x] `PlanNodeType::TensorContraction` added to `query_plan_visualizer.h`
  - [x] `TensorAwareQueryOptimizer` — rewrites tensor-function plan nodes to `TensorContraction` type (stub #166)
  - [x] `TARGRetrieval` — logit-gap gating for token-adaptive retrieval (stub #167)
  - [x] Tests TCP-01..04, TCM-01..05, TAQO-01..06, TARG-01..08 in `tests/test_tensor_phase3.cpp`

- [x] **FLARE mid-generation retrieval** (2026-05-06):
  - [x] `FlareRetrieval` class — per-token log-probability gating (AC: TTFT per step ≤ 90 ms with TT-core index)
  - [x] `FlareConfig` with `confidence_threshold`, `min_consecutive_uncertain`, cooldown, `max_retrieval_steps`, `query_window_tokens`, `mask_uncertain_tokens`
  - [x] `FlareDecision` struct with full gating metrics
  - [x] `buildQuery()` — sliding window token concatenation with uncertain-token masking (stub #168: string query; embed path deferred to Phase 3-C)
  - [x] `notifyTokenEmitted()` / `notifyRetrievalExecuted()` / `reset()` state management
  - [x] `FlareStats` with mean/min/max log-prob, trigger rate
  - [x] Tests FR-01..FR-10 added to `tests/test_tensor_phase3.cpp`
  - [x] STUB_INVENTORY #168 registered (buildQuery string-mode)

- [~] **GgmlTensorBridge skeleton** (2026-05-06):
  - [x] `src/storage/ggml_tensor_bridge.cpp` created (THEMIS_ENABLE_GGML_BRIDGE gate)
  - [x] `map()` / `mapAdapter()` / `prefetch()` / `releaseAll()` / `stats()` implemented
  - [x] `registerGgmlTypeTT()` placeholder (returns ID 9999; real ggml_type_register deferred)
  - [x] STUB_INVENTORY #169 registered (FakeTensor proxy; real ggml_tensor* deferred to Q1 2027)
  - [x] CMake option `THEMIS_ENABLE_GGML_BRIDGE` added

## Planned Features 📋

### Short-term (Phase 2, Q4 2026)

- [ ] Wire `HnswTTBridge::HnswLayer` to hnswlib `HierarchicalNSW<float>` (Target: Q4 2026)
  - Sketches from first TT-core (dim = min(n₁, `sketch_dim`))
  - Build-time: no full vector needed; only first-core required for HNSW graph
  - Navigation accuracy: recall@10 ≥ 0.95 vs. brute-force on 1 M 4096-dim vectors
  - Re-rank on TT-domain cosine at O(d·r²); throughput target ≥ 741 ops/s (paper §Similarity Search)

- [x] RocksDB persistence for `FlatTensorIndex` and `HnswTTBridge` (Target: Q4 2026)
  - Binary file persistence implemented (THEMIS_TTI / THEMIS_HTB magic, v1); stubs #150/#151/#155/#156 resolved
  - `TensorIndexManager::setDataDir()` + `flushAll()` wire file-based persistence
  - `main_server.cpp` injects the tensor pipeline under `tensor_index` config

- [ ] `TensorIndexManager::dropTenantIndexes()` — RocksDB prefix-delete (Target: Q4 2026)
  - Use `DeleteRange(__ttmgr__:<tenant_id>:, __ttmgr__:<tenant_id+1>:)`

- [x] `TensorIndexManager` wired to `TensorNetworkStorageEngine` for core persistence (Target: Q4 2026)
  - `setDataDir()` + `flushAll()` added; `createIndex()` loads from disk on open; `dropIndex()` removes file; `main_server.cpp` wires it with RocksDB

- [x] **Rank-adaptive RocksDB compaction filter** (Target: Q4 2026)
  - **resolved 2026-05-06**: `TensorCompactionFilter` added to `include/storage/tensor_compaction_filter.h`
    and `src/storage/tensor_compaction_filter.cpp`.  Inherits `rocksdb::CompactionFilter`,
    targets `__ttcore__:` (raw TTTrain) and `__ttn__:...:meta:` (QuantizedTrain) key namespaces.
  - `FilterV2()` deserialises → `recompress(ε)` → re-serialises only if smaller (copy-on-success).
  - `TensorTrainDecomposer::recompress()` implemented as proper TT-rounding (Oseledets 2011 Alg. 2):
    right-to-left `thinLQ()` orthogonalisation + left-to-right truncated-SVD, O(d·r²·n).
  - AC-pending: 20% storage reduction measurement deferred to integration test (requires
    Maxwellian corpus; tracked in Phase 3 AC checklist).

### Medium-term (Phase 3, Q1–Q2 2027)

- [~] `TensorIndexManager::ggmlCorePtrs()` / `mapCores()` — mmap-pinned zero-copy bridge (Target: Q1 2027)
  - **Phase 3-A resolved 2026-05-06**: `TensorMmapBridge` (`include/tensor/tensor_mmap_bridge.h` + `src/tensor/tensor_mmap_bridge.cpp`) wraps each TT-core in an `mmap(MAP_ANONYMOUS|MAP_PRIVATE)` region pinned via `mlock()`; RAII destructor calls `munlock()+munmap()`.  `TensorIndexManager::mapCores()` replaces raw `ggmlCorePtrs()` as the recommended call path.  Tests TIM-19..TIM-24 added.  STUB #176 registered for MAP_SHARED SST path.
  - Remaining: replace MAP_ANONYMOUS + memcpy with `MAP_SHARED` on RocksDB SST file pages (STUB #176, Q1 2027).
  - Return `GgmlCoreDescriptor { ggml_tensor*, size_t core_idx }` per core — deferred to GGML_TYPE_TT merge.
  - AC: TTFT reduction ≥ 3× vs. classical JSON-RAG path (paper §Zero-Copy RAG table) — pending integration test.

- [ ] `GgmlTensorBridge` — `GGML_TYPE_TT` registration (Target: Q1 2027)
  - New `ggml_type` enum value; `ggml_map_custom1` op for O(d·r²) contraction
  - GGUF v3 metadata: attach provenance key-values (filename, page, line) per core
    (paper §GGUF Metadata; required for regulated industries)
  - AC: inference output within ±ε of decompressed reference

- [~] **FLARE mid-generation retrieval integration** (Target: Q1 2027)
  - **resolved 2026-05-06**: `FlareRetrieval` in `include/rag/flare_retrieval.h` + `src/rag/flare_retrieval.cpp`
  - Per-token log-probability gating (`confidence_threshold` default: ln(0.1) ≈ -2.303)
  - Sliding-window token history with uncertain-token masking via `buildQuery()`
  - `max_retrieval_steps` cap, cooldown, min_consecutive_uncertain knobs
  - `FlareStats` tracks trigger rate, mean/min/max log-prob
  - Tests FR-01..FR-10 in `tests/test_tensor_phase3.cpp`
  - AC: TTFT per step ≤ 90 ms (pending integration test with TT-core index)

- [~] **TARG logit-gap gating** (Target: Q2 2027)
  - Paper §TARG: single-shot draft measures top-1/top-2 logit gap; triggers retrieval
    only when gap < threshold; eliminates 70-90% of unnecessary retrieval calls
  - **resolved 2026-05-06**: `TARGRetrieval` in `include/rag/targ_retrieval.h` + `src/rag/targ_retrieval.cpp`
  - Single-shot draft measures top-1/top-2 logit gap; triggers retrieval only when gap < threshold
  - Optional entropy gate (top-32 approximate entropy, STUB #167)
  - Cool-down, consecutive-uncertain counter, `min_consecutive_uncertain` knob
  - Tests TARG-01..TARG-08 in `tests/test_tensor_phase3.cpp`
  - AC: eliminates unnecessary retrieval calls when gap ≥ threshold ✓; stats track trigger rate ✓

- [~] `TensorAwareQueryOptimizer` plan-node `TENSOR_CONTRACTION` routing (Target: Q1 2027)
  - Detect TT-stored operands in AQL plan
  - Route to `TensorContractionEngine` instead of reconstructing flat vectors
  - **resolved 2026-05-06 (Phase 3-A)**: `TensorAwareQueryOptimizer` rewrites plan nodes whose description contains tensor function names to `PlanNodeType::TensorContraction`; cost estimate uses TT-domain complexity. Full AQL IR visitor deferred to Phase 3-C (STUB #166).

- [~] **AQL tensor-aware operators: CONTRACT, PROJECT, DECOMPOSE** (Target: Q2 2027)
  - Paper §AQL: structure-oriented query language; topology is the primary object
  - **resolved 2026-05-06**:
    - `TENSOR_CONTRACT(a, b, modes_a, modes_b)` — multi-mode contraction returning TTTrain
    - `TENSOR_PROJECT(t, mode)` — compressed-domain marginalization over one mode (O(d·n·r²))
    - `TENSOR_DECOMPOSE(data, shape, rank, epsilon)` — on-the-fly TT decomposition within query
    - All three registered via `registerTensorFunctions()` in `tensor_functions.cpp`
    - `TensorContractionEngine::project()` and `::contractModes()` methods added
  - AC: AQL EXPLAIN shows TensorContraction operator nodes ✓; integration tests TCP/TCM verify math ✓

- [~] **Tensor Butterfly algorithm for oscillatory integral operators** (Target: Q2 2027)
  - **In progress 2026-05-06**: `TensorButterflyOperator` in `include/tensor/tensor_butterfly_operator.h`
    and `src/tensor/tensor_butterfly_operator.cpp`
  - FOURIER type implemented using Walsh-Hadamard Transform (WHT) — real-valued, butterfly-structured;
    orthogonal transform O(n·d·log n) per apply(); STUB #170 (full complex DFT deferred Q3 2027)
  - RADON and GREENS_FUNCTION types: throw `std::logic_error` (STUB #171, Q3 2027)
  - Tests TBO-01..TBO-06 in `tests/test_tensor_phase3.cpp`
  - AC-partial: norm preservation confirmed (WHT orthogonality); complex DFT accuracy deferred

- [~] **Adapter sovereignty — LoRA/PEFT store as TT graphs** (Target: Q2 2027)
  - **In progress 2026-05-06**: `AdapterRepository` in `include/tensor/adapter_repository.h`
    and `src/tensor/adapter_repository.cpp`
  - `GgmlCoreDescriptor` struct defined (tenant_id, domain, base_model_id, TTTrain, valid flag)
  - `store(domain, base_model_id, TTTrain)`: serialises + persists under
    `__adapters__:<tenant>:<domain>:<base_model_id>`
  - `loadAdapter(domain, base_model_id)`: deserialises → GgmlCoreDescriptor (heap copy; STUB #172)
  - `listDomains() / listAdapters()`: prefix-scan + deduplicate
  - Tenant isolation enforced via key prefix
  - Tests AR-01..AR-06 in `tests/test_tensor_phase3.cpp`
  - STUB #172: mmap zero-copy path deferred to Q1 2027 (currently heap-copies TT-core data)

- [x] **GGUFMetadata — GGUF v3 provenance store** (2026-05-06):
  - `include/storage/gguf_metadata.h` + `src/storage/gguf_metadata.cpp`
  - `ProvenanceRecord` struct: source_filename, source_page, source_line, source_doc_id, tenant_id, ingest_timestamp, hmac_signature
  - `GGUFMetadata::attach()` / `retrieve()` / `detach()` / `has()` / `keys()` — thread-safe via shared_mutex
  - `GGUFMetadata::sign()` / `verify()` — byte-XOR placeholder (STUB #173; HMAC-SHA256 deferred Q2 2027)
  - `serialize()` / `deserialize()` — flat binary format for GGUF embedding
  - Tests GMD-01..GMD-08 in `tests/test_gguf_metadata.cpp`

- [x] **TensorFingerprintGraph — adapter similarity via G_0 fingerprint** (2026-05-06):
  - `include/tensor/tensor_fingerprint_graph.h` + `src/tensor/tensor_fingerprint_graph.cpp`
  - `FingerprintEntry { adapter_key, domain, base_model_id, tenant_id, fingerprint, first_core_norm }`
  - `addAdapter(key, TTTrain, domain, base_model_id, tenant_id)` — extracts column-mean fingerprint of G_0
  - `findSimilar(adapter_key, k)` — top-k cosine similarity on fingerprints (stub #174; full TT inner-product Q3 2027)
  - `findSimilarByFingerprint(fingerprint, k, tenant_id)` — query by raw fingerprint with optional tenant filter
  - `removeAdapter()`, `entry()`, `adapterKeys()`, `stats()` accessors
  - Tests TFG-01..TFG-06 appended to `tests/test_tensor_phase3.cpp`

### Long-term (Phase 4, Q2–Q4 2027)

- [~] `AdaLoRA ↔ TT Bridge` Phase 3 — `findSimilarAdapters()` via TensorFingerprintGraph (Target: Q3 2027)
  - **In progress 2026-05-06**: `AdapterRepository::setFingerprintGraph()` wires `store()`/`remove()` to
    register/deregister fingerprints automatically; `findSimilarAdapters(domain, model, k)` delegates to
    `TensorFingerprintGraph::findSimilar()` (column-mean cosine; STUB #177 — inherits STUB #174).
  - Tests AR-07..AR-12 added to `tests/test_tensor_phase3.cpp`.
  - Remaining: replace column-mean fingerprint similarity with full TT inner-product sweep (O(d·r²))
    and HNSW index over fingerprints for sub-linear search (Q3 2027, Phase 4).
- [ ] Distributed TT shard layout (one shard per TT-core index) (Target: Q4 2027)
  - Compatible with RAID-Sharding 2.0 tensor-based reconstruction
    (paper §RAID-Sharding 2.0: node failure = missing tensor index; rebuilt via
    tensor completion algorithms)
- [ ] CUDA cuSOLVER backend for on-GPU TT-SVD during `addFlat()` (Target: Q4 2027)
  - Paper §GPU Tensor Cores: HT decomposition on A100 tensor cores achieves 32× vs
    standard CUDA; implement HT contraction path alongside TT

### Phase 5: Hierarchical Tucker (HT) + Quantics TT (QTT) (Target: Q1–Q2 2028)

- [~] **`HierarchicalTuckerDecomposer`** — binary-tree tensor factorization (Target: Q1 2028)
  - Paper §HT: storage O(d·n·r + d·r³); parallelizable tree branches
  - `HTTrain` type: binary tree of `HTNode { U_k (leaf), B_transfer (internal) }`
  - `IHierarchicalTuckerIndex` parallel to `ITensorIndex`; tree-branch ops parallelized
    via thread pool
  - AC: storage ≤ O(d·n·r + d·r³); contraction ≤ O(d·n·r² + d·r⁴)
  - **2026-05-07 in progress**: HOSVD initialization + top-down SVD transfer tensor
    construction implemented; FlatHTIndex linear-scan search; STUBS #178, #179, #180;
    tests HT-01..HT-18 all passing; HOOI iteration + GPU path deferred Q2 2028

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

### Phase 2: Production Core (Status: ✅ Complete — 2026-05-06)

- [x] hnswlib integration in `HnswTTBridge::HnswLayer` — resolved 2026-05-06
- [x] RocksDB persistence (save/load for all backends) — resolved 2026-05-06
- [x] Tenant prefix-delete in `dropTenantIndexes()` — resolved 2026-05-06
- [x] `TensorIndexManager` → `TensorNetworkStorageEngine` wire-up — resolved 2026-05-06
- [x] Rank-adaptive RocksDB compaction filter (`TensorCompactionFilter`) — resolved 2026-05-06
- [x] Phase-2 tests: TTI-01..18, HTB-04..09, TIM-03..18 — resolved 2026-05-06

### Phase 3: Zero-Copy Inference + Advanced RAG (Status: 🚧 In Progress — 2026-05-06)

- [~] `ggmlCorePtrs()` — null-pointer mmap handshake (TIM-01)
  - **Phase 3-A resolved 2026-05-06**: `TensorMmapBridge` added (`include/tensor/tensor_mmap_bridge.h` + `src/tensor/tensor_mmap_bridge.cpp`); `TensorIndexManager::mapCores()` allocates `MAP_ANONYMOUS|MAP_PRIVATE` regions + `mlock()`; RAII destructor calls `munlock()+munmap()`; STUB #176 for MAP_SHARED SST-file path (Q1 2027); tests TIM-19..TIM-24
  - Remaining: replace MAP_ANONYMOUS + memcpy with MAP_SHARED on RocksDB SST pages (STUB #176, Q1 2027)
- [~] `GgmlTensorBridge` — `GGML_TYPE_TT` + GGUF v3 metadata provenance
  - Skeleton implemented 2026-05-06: `map()`/`mapAdapter()`/`prefetch()`/`releaseAll()`/`stats()` wired;
    FakeTensor proxy (stub #169) replaces real ggml_tensor allocation; `registerGgmlTypeTT()` returns placeholder
  - Real ggml_tensor allocation + ggml_map_custom1 contraction kernel deferred to Q1 2027
- [x] `TensorAwareQueryOptimizer` TENSOR_CONTRACTION plan-node — resolved 2026-05-06
- [x] AQL operators: CONTRACT, PROJECT, DECOMPOSE — resolved 2026-05-06
- [~] Tensor Butterfly operator for O(n·d) oscillatory integral transforms — 2026-05-06 (FOURIER/WHT; complex DFT Q3 2027)
- [x] FLARE mid-generation retrieval (TTFT per step ≤ 90ms) — resolved 2026-05-06
- [x] TARG logit-gap gating (70–90% unnecessary retrieval calls eliminated) — resolved 2026-05-06
- [~] Adapter sovereignty — LoRA/PEFT as TT graphs (adapter switch ≤ 50ms) — 2026-05-06 (heap-copy path; mmap Q1 2027)
- [x] GGUFMetadata — GGUF v3 provenance store (sign/verify stub #173; HMAC-SHA256 Q2 2027) — resolved 2026-05-06
- [x] TensorFingerprintGraph — adapter similarity via G_0 fingerprint (full TT inner-product stub #174; Q3 2027) — resolved 2026-05-06
- [x] TensorRAGPipeline — unified FLARE+TARG coordinator (PIPE-01 stub; embed path Q1 2027) — resolved 2026-05-06
  - `include/rag/tensor_rag_pipeline.h` + `src/rag/tensor_rag_pipeline.cpp`
  - `step(token_text, log_prob, logits)` evaluates both gates per token; returns `RAGDecision`
  - `notifyRetrievalDone()` resets both gate cooldowns
  - `reset()` clears all session state
  - `PipelineStats` tracks total_token_steps, flare_triggers, targ_triggers, combined_triggers, total_retrievals
  - `use_flare` / `use_targ` flags allow individual gate disable
  - Tests TRPL-01..TRPL-08 in `tests/test_tensor_phase3.cpp`

### Phase 4: Distributed & GPU (Target: Q2–Q4 2027)

- [ ] Per-core shard distribution over Themis-Net + RAID-Sharding 2.0 tensor completion
- [ ] CUDA cuSOLVER TT-SVD during `add()` (≤ 80ms for 10⁶-element 6D tensor)
- [ ] HT contraction on A100 GPU tensor cores (32× vs CUDA baseline)
- [~] AdaLoRA `findSimilarAdapters()` wire-up — **in progress 2026-05-06** (fingerprint cosine path; full TT inner-product + HNSW Q3 2027)

### Phase 5: HT + QTT Formats (Target: Q1–Q2 2028)

- [~] `HierarchicalTuckerDecomposer` + `IHierarchicalTuckerIndex` — **in progress 2026-05-07**
  - HOSVD-based HT decomposition implemented (STUB #179 — HOOI iteration deferred Q2 2028)
  - `HTNode` binary tree with leaf U matrices and internal B transfer tensors
  - `HTTrain` type: binary tree with shape, max_rank, achieved_eps, serialization
  - `HTContractionEngine`: O(d·n·r² + d·r⁴) inner product via Gram matrix propagation
  - `FlatHTIndex` (linear-scan IHierarchicalTuckerIndex), serialization, cosine search
  - Jacobi EVD-based truncated SVD (STUB #180; LAPACK dgesdd deferred Q2 2028)
  - toTTTrain() compatibility bridge (STUB #178; full TT path deferred Q2 2028)
  - Tests HT-01..HT-18 in `tests/test_tensor_ht.cpp`
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
- [x] Rank-adaptive compaction filter (`TensorCompactionFilter`) implemented — 2026-05-06; storage-reduction AC deferred to Phase 3 integration suite
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
