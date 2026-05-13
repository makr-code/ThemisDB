# Tensor Module — Public Headers

<!-- status: current | validated: 2026-05-13 | commit: HEAD -->

**Module Path:** `include/tensor/`
**Implementation:** `../../src/tensor/`
**Namespace:** `themis::tensor`
**Status:** 🟡 Experimental — Phase 1 complete; Phase 2 in progress (Q4 2026)

Public C++ header files for the ThemisDB tensor module. All headers are
`#pragma once` guarded. This module provides Tensor-Train (TT) and Hierarchical
Tucker (HT) compressed approximate-nearest-neighbour (ANN) indexing, zero-copy
GGML bridge, LoRA adapter sovereignty, multi-modal data encoding, and adaptive
structural rounding.

**See also:**
- Implementation overview: [`../../src/tensor/README.md`](../../src/tensor/README.md)
- Architecture: [`../../src/tensor/ARCHITECTURE.md`](../../src/tensor/ARCHITECTURE.md)
- Roadmap: [`../../src/tensor/ROADMAP.md`](../../src/tensor/ROADMAP.md)
- Future enhancements: [`../../src/tensor/FUTURE_ENHANCEMENTS.md`](../../src/tensor/FUTURE_ENHANCEMENTS.md)

---

## Header Overview

| Header | Key Types | Phase | Status |
|--------|-----------|-------|--------|
| [`tensor_index.h`](#tensor_indexh) | `ITensorIndex`, `TensorSearchResult`, `TensorIndexStats` | 1 | ✅ Production API |
| [`tensor_index_manager.h`](#tensor_index_managerh) | `TensorIndexManager`, `IndexHandle` | 1 | ✅ Production API |
| [`hnsw_tt_bridge.h`](#hnsw_tt_bridgeh) | `HnswTTBridge`, `HnswTTConfig` | 2 | 🟡 Skeleton (Q4 2026) |
| [`tensor_ingestion_bridge.h`](#tensor_ingestion_bridgeh) | `TensorIngestionBridge` | 1 | ✅ Production Ready |
| [`tensor_core_bridge.h`](#tensor_core_bridgeh) | `TensorCoreStorageBridge` | 1 | ✅ Production Ready |
| [`tensor_mmap_bridge.h`](#tensor_mmap_bridgeh) | `TensorMmapBridge`, `MmapCoreSlice` | 3 | 🟡 Stub #176 (Q1 2027) |
| [`adapter_repository.h`](#adapter_repositoryh) | `AdapterRepository`, `GgmlCoreDescriptor` | 3 | 🟡 Experimental |
| [`tensor_fingerprint_graph.h`](#tensor_fingerprint_graphh) | `TensorFingerprintGraph`, `SimilarityResult` | 4-prep | 🟡 Experimental |
| [`tensor_butterfly_operator.h`](#tensor_butterfly_operatorh) | `TensorButterflyOperator`, `ButterflyConfig` | 3 | 🟡 FOURIER only |
| [`hiss_structural_search.h`](#hiss_structural_searchh) | `HissStructuralSearchEngine`, `HissReshaper`, `TemplateCatalog` | 6 | 🟡 Experimental |
| [`tnsr_task.h`](#tnsr_taskh) | `TNSRTask`, `TNSRConfig`, `TNSRReport` | 6 | 🟡 Experimental |
| [`encoder_interface.h`](#encoder_interfaceh) | `ITextEncoder`, `IImageEncoder`, `EncoderQuality` | 7 | 🟡 Experimental |
| [`utr_converter.h`](#utr_converterh) | `UTRConverter`, `RasterGrid`, `UTRConfig` | 7 | 🟡 Experimental |
| [`hyper_index_builder.h`](#hyper_index_builderh) | `HyperIndexBuilder`, `HyperIndexTensor`, `ColumnSchema` | 7 | 🟡 Experimental |
| [`ht_train.h`](#ht_trainh) | `HTTrain`, `HTNode`, `HTContractionEngine` | 5 | 🟡 Experimental |
| [`ht_index.h`](#ht_indexh) | `IHierarchicalTuckerIndex`, `FlatHTIndex` | 5 | 🟡 Experimental |

---

## Module Use-Case Map

| Use Case | Entry-Point Header | Phase |
|----------|--------------------|-------|
| Insert / search TT-compressed vectors | `tensor_index.h` + `tensor_index_manager.h` | 1 |
| Hybrid HNSW-navigation + TT re-ranking | `hnsw_tt_bridge.h` | 2 |
| Ingest document embeddings → TT-cores | `tensor_ingestion_bridge.h` | 1 |
| Persist TT-cores from ingestion pipeline | `tensor_core_bridge.h` | 1 |
| Zero-copy mmap bridge for GGML/llama.cpp | `tensor_mmap_bridge.h` | 3 |
| Store/retrieve LoRA/PEFT adapters as TT | `adapter_repository.h` | 3 |
| Fast approximate adapter similarity | `tensor_fingerprint_graph.h` | 4-prep |
| Oscillatory integral operators on TT data | `tensor_butterfly_operator.h` | 3 |
| Adaptive structural topology search | `hiss_structural_search.h` | 6 |
| Background rank/topology compression | `tnsr_task.h` | 6 |
| Multi-modal data → TT/HT encoding | `utr_converter.h` | 7 |
| Tabular cross-column latent index | `hyper_index_builder.h` | 7 |
| Hierarchical Tucker tensor operations | `ht_train.h` + `ht_index.h` | 5 |

---

## Headers

### tensor_index.h

**Purpose:** Uniform abstract interface for all Tensor-Train-based ANN index backends.
Primary entry point for consumers that only need to insert and query vectors.

**Key Types:**

| Type | Description |
|------|-------------|
| `ITensorIndex` | Pure-virtual interface for all TT-based index backends |
| `TensorSearchResult` | One k-NN result: `id`, `distance`, `tt_norm` |
| `TensorIndexStats` | Runtime diagnostics: vector count, rank, compression ratio, search latency |

**Key API:**
```cpp
#include "tensor/tensor_index.h"

// Obtained via TensorIndexManager::createIndex()
ITensorIndex* idx = mgr->createIndex("tenant1", "llm_weights", "attention_k",
                                     /*dim=*/4096, /*max_rank=*/32);

// Insert — pre-compressed TT train
bool ok = idx->add(42, tt_train);

// Insert — flat float32 vector (compressed internally)
bool ok = idx->addFlat(42, vec.data(), 4096);

// Search — returns up to k results sorted by ascending distance
auto results = idx->searchFlat(query.data(), 4096, /*k=*/10);
for (auto& r : results) {
    // r.id, r.distance, r.tt_norm
}

// Compressed-domain arithmetic (no decompression)
auto ip   = idx->innerProduct(id_a, id_b);   // O(d · r³)
auto norm = idx->norm(id);                    // O(d · r³)

// Diagnostics
TensorIndexStats s = idx->stats();
// s.num_vectors, s.avg_tt_rank, s.avg_compress_ratio, s.avg_search_ms
```

**Thread Safety:**
Concurrent reads (`search`, `searchFlat`, `innerProduct`, `norm`, `get`, `stats`) are safe for all
implementations (`FlatTensorIndex`, `HnswTTBridge`).
Writes (`add`, `addFlat`, `remove`) are **not internally serialised** — callers must use external
locking when inserting from multiple threads concurrently.

**Persistence:**
`save(path)` and `load(path)` use the key schema `__ttidx__:<index_name>:<id>:<core_k>`.
Both are **stubs** in Phase 1 (see STUB IDs TTI-01, TTI-02 in `src/tensor/README.md`).

**Error Cases:**
- `add()` returns `false` if `id` already exists or `train` is structurally invalid (empty cores).
- `addFlat()` returns `false` if `dim == 0` or the decomposer returns an empty train.
- `remove()` returns `false` if `id` is not found.
- `search()` returns an empty vector if the index is empty.
- `innerProduct()` / `norm()` return `std::nullopt` if either ID is not found.

---

### tensor_index_manager.h

**Purpose:** Module-level lifecycle manager for `ITensorIndex` instances. Analogous to
`IndexManager` in `src/index/`. Owns all TT-based indexes for the process lifetime,
routes incoming vectors, and bridges to the zero-copy GGML path.

**Key Types:**

| Type | Description |
|------|-------------|
| `TensorIndexManager` | Registry, routing, lifecycle, mmap bridge |
| `IndexHandle` | Lightweight descriptor: tenant/collection/field + route + `ITensorIndex*` |

**Key API:**
```cpp
#include "tensor/tensor_index_manager.h"

// Factory — requires a live RocksDB instance
auto mgr = TensorIndexManager::create(db);

// Optional: configure file-based persistence directory
mgr->setDataDir("/var/themisdb/tensor");

// Routing decision (delegates to TensorRouter)
// Returns TENSOR_TRAIN, HNSW, or HYBRID
auto route = mgr->routeFor("tenant1", "llm_weights", "attention_k",
                            /*dim=*/4096, /*num_vectors=*/1'000'000);

// Create (or open) a TT index
ITensorIndex* idx = mgr->createIndex("tenant1", "llm_weights", "attention_k",
                                     /*dim=*/4096, /*max_rank=*/32, /*epsilon=*/0.01);
// idx == nullptr if route == HNSW

// Retrieve without creating
ITensorIndex* idx2 = mgr->getIndex("tenant1", "llm_weights", "attention_k");

// Drop and delete persisted data
bool dropped = mgr->dropIndex("tenant1", "llm_weights", "attention_k");

// Drop all indexes for a tenant
mgr->dropTenantIndexes("tenant1");

// Introspection
auto handles = mgr->listIndexes();
auto stats   = mgr->aggregateStats();

// Zero-copy mmap bridge (Phase 3)
auto bridge = mgr->mapCores("tenant1", "llm_weights", "attention_k", /*id=*/42);
if (bridge) {
    for (const auto& s : bridge->slices()) {
        // s.data, s.num_elems — valid while bridge is alive
    }
} // destructor calls munlock + munmap

// Persist all in-memory indexes
size_t saved = mgr->flushAll();
```

**Multi-tenancy:**
All keys are namespaced as `__ttmgr__:<tenant_id>:<collection>:<field>`.

**Thread Safety:**
All public methods are thread-safe. Reads use a `shared_mutex`; writes (`createIndex`,
`dropIndex`, `dropTenantIndexes`) acquire exclusive ownership.

**Error Cases / Limits:**
- `createIndex()` returns `nullptr` when the routing decision is HNSW.
- `getIndex()` returns `nullptr` if the index was never created.
- `mapCores()` returns `nullptr` if the vector ID does not exist (STUB #176: uses
  MAP_ANONYMOUS + memcpy; true MAP_SHARED on SST files deferred to Q1 2027).
- `ggmlCorePtrs()` is deprecated; prefer `mapCores()` for new code.

---

### hnsw_tt_bridge.h

**Purpose:** Hybrid two-layer index combining HNSW graph navigation over first-core
sketches with full TT-domain re-ranking. Optimal for moderate compressibility
κ ∈ [2×, 6×), dim ∈ [1024, 16384], n ≥ 500 K.

**Key Types:**

| Type | Description |
|------|-------------|
| `HnswTTBridge` | Implements `ITensorIndex`; managed by `TensorIndexManager` |
| `HnswTTConfig` | Tuning parameters for HNSW layer and TT re-ranker |

**Configuration (`HnswTTConfig`):**
```cpp
HnswTTConfig cfg;
cfg.M               = 16;    // HNSW neighbours per node
cfg.ef_construction = 200;   // ef during build
cfg.ef_search       = 50;    // ef during search (candidate pool)
cfg.sketch_dim      = 64;    // First-core sketch dim for HNSW (0 = full first-core)
cfg.rerank_candidates = 200; // Candidates passed to TT re-ranker
cfg.max_tt_rank     = 32;    // Max TT-rank for compression
cfg.epsilon         = 0.01;  // Relative reconstruction error bound
cfg.kappa_min       = 2.0;   // Min compressibility for HYBRID route
cfg.kappa_max       = 6.0;   // Max compressibility (above → pure TT)
```

**Stubs (Phase 2):** `save()`, `load()`, and the full hnswlib integration are stubs.
The HNSW layer falls back to a linear scan until hnswlib is wired (Target: Q4 2026).

**Error Cases:**
- `add()` returns `false` if the train is invalid.
- `search()` on an empty index returns an empty vector.
- `save()` / `load()` always return `false` until stub HTB-02/HTB-03 is resolved.

---

### tensor_ingestion_bridge.h

**Purpose:** Concrete `ITensorDecompositionBackend` that integrates the ingestion
pipeline with the TT storage layer. Bridges `ingestion/` and `storage/tensor/`
without a direct dependency from ingestion headers to storage headers.

**Key Type:** `TensorIngestionBridge`

**Key API:**
```cpp
#include "tensor/tensor_ingestion_bridge.h"
#include "ingestion/builtin_step_factories.h"

// Construction (default epsilon=0.01, no rank cap, min_kappa=1.3)
auto bridge = std::make_shared<themis::tensor::TensorIngestionBridge>();
bridge->setEpsilon(0.01);    // override reconstruction error tolerance
bridge->setMaxRank(64);      // override bond-dimension cap (0 = no cap)
bridge->setMinKappa(1.3);    // override κ-gate threshold

// Wire into ingestion pipeline
auto step = ingestion::builtin::createChunkTtDecomposeStep(bridge);

// κ-gate check (used internally; exposed for testing)
bool worth_it = bridge->shouldDecompose(embedding, /*min_kappa=*/1.3);

// Diagnostics
long long total   = bridge->decomposeCount();
long long skipped = bridge->kappaSkipCount();
```

**κ-gate:** `shouldDecompose()` runs a pilot TT-decomposition at `epsilon/5`
(capped at 0.05) to estimate compressibility. For dim > 1024 a Rademacher random
projection to 1024 elements is used (JL guarantee: κ deviation ≤ 5%).

**Thread Safety:** Fully thread-safe. Atomic counters; stateless decomposer.

**Error Cases:**
- `decompose()` returns a `TensorCoreRecord` with empty `serialized_train` on
  decomposer error; the caller should log the warning.
- `shouldDecompose()` returns `false` when κ < `min_kappa` (embedding not worth compressing).

---

### tensor_core_bridge.h

**Purpose:** Production `ITensorCoreBridge` that persists TT-core records to
an injectable `ITensorStorageBackend`. Bridges the ingestion sink step and
the tensor storage layer.

**Key Schema:** `__ttcore__:<tenant>:<source_file_id>:<chunk_id>`

**Key API:**
```cpp
#include "tensor/tensor_core_bridge.h"

// Inject a production RocksDB backend (do once at server bootstrap)
TensorCoreStorageBridge::setDefaultBackendFactory([&] {
    return std::make_shared<RocksDBTensorBackend>(db_handle);
});

// Construction — uses factory if set; falls back to InMemoryTensorBackend
auto bridge = std::make_shared<TensorCoreStorageBridge>();

// Write a TT-core record (upsert)
auto result = bridge->write(record, "tenant1");
if (!result) { /* handle error */ }

// Read raw bytes (for verification / testing)
auto raw = bridge->getRaw("tenant1", source_file_id, chunk_id);

// Build the storage key
std::string key = TensorCoreStorageBridge::makeKey("tenant1", src_id, chunk_id);

// Diagnostics
std::size_t n = bridge->writeCount();
```

**Thread Safety:** Delegated to the injected `ITensorStorageBackend`.

**Error Cases:**
- `write()` returns an error if `tenant_id` is empty, contains `'/'`, or `'\0'`.
- `write()` returns an error on backend write failure.
- RocksDB backend is a stub until wired (STUB #160, Target: Q4 2026).
- `makeKey()` throws `std::invalid_argument` if any argument is empty or contains `'/'`.

---

### tensor_mmap_bridge.h

**Purpose:** RAII owner of mmap-pinned TT-core pages for zero-copy injection into
GGML/llama.cpp inference graphs. Phase 3 — obtained exclusively via
`TensorIndexManager::mapCores()`.

**Key Types:**

| Type | Description |
|------|-------------|
| `TensorMmapBridge` | RAII owner; destructor calls `munlock()` + `munmap()` |
| `MmapCoreSlice` | Lightweight view of one pinned TT-core: `data`, `bytes`, `core_idx`, `num_elems` |

**Key API:**
```cpp
#include "tensor/tensor_mmap_bridge.h"

// Obtained from TensorIndexManager::mapCores()
auto bridge = mgr->mapCores("tenant1", "collection", "field", /*id=*/42);
if (!bridge) { return {}; }  // vector not found

for (const auto& s : bridge->slices()) {
    // s.data[0 .. s.num_elems-1] — valid while bridge is alive
}

bridge->totalBytes();    // total bytes pinned
bridge->isLocked();      // true if mlock() succeeded for at least one region
bridge->lockedRegions(); // number of successfully locked regions
bridge->release();       // explicit early release (also called by destructor)
// After release(), all slice pointers are INVALID

// SST page-map injection (STUB #270 — production zero-copy Q1 2027)
TensorMmapBridge::setSstMapFn(
    [](std::size_t bytes,      // required byte length for this core region
       std::size_t core_idx    // index of the TT-core being mapped (0-based)
    ) -> void* {
        // Return a MAP_SHARED pointer to the SST page; nullptr → fallback to MAP_ANONYMOUS+memcpy
        return nullptr;
    });
TensorMmapBridge::clearSstMapFn();
```

**STUB #176:** Current implementation uses `MAP_ANONYMOUS | MAP_PRIVATE` + `memcpy`.
True zero-copy (`MAP_SHARED` on RocksDB SST pages) is deferred to Q1 2027.

**Thread Safety:** Instances are NOT thread-safe. Single inference-thread ownership assumed.

**Error Cases / Limits:**
- `mlock()` silently fails in unprivileged CI containers (`RLIMIT_MEMLOCK == 0`);
  `isLocked()` returns `false` but data pointers remain valid.
- Do NOT hold slice pointers beyond the bridge's lifetime.
- Not copyable; movable.

---

### adapter_repository.h

**Purpose:** Persistent store for LoRA/PEFT adapters as TT graphs ("Adapter Sovereignty").
Supports store, load, list, remove, and fingerprint-based similarity search.

**Key Types:**

| Type | Description |
|------|-------------|
| `AdapterRepository` | Tenant-scoped adapter store backed by `ITensorStorageBackend` |
| `GgmlCoreDescriptor` | Descriptor returned by `loadAdapter()`; contains TT weights and provenance |
| `AdapterMetadata` | Optional provenance (author, created_at, description, achieved_eps, max_rank) |

**Key Schema:** `__adapters__:<tenant_id>:<domain>:<base_model_id>`

**Key API:**
```cpp
#include "tensor/adapter_repository.h"

auto backend = std::make_shared<InMemoryTensorBackend>();
AdapterRepository repo(backend, "tenant1");

// Store a legal LoRA adapter
AdapterMetadata meta;
meta.author       = "team-legal";
meta.description  = "German legal domain adapter for LLaMA-3-8B";
meta.achieved_eps = 0.008f;
bool ok = repo.store("legal", "llama3-8b", adapter_train, meta);

// Load for inference (STUB #172: heap copy; MAP_SHARED in Q1 2027)
auto desc = repo.loadAdapter("legal", "llama3-8b");
if (desc.valid) {
    // desc.train — TT-format adapter weights (all cores)
    // inject into llama.cpp ggml graph
}

// Enumerate
auto domains  = repo.listDomains();   // sorted, deduplicated
auto adapters = repo.listAdapters();  // all (domain, base_model_id) pairs

// Remove
bool removed = repo.remove("legal", "llama3-8b");

// Adapter similarity (requires a TensorFingerprintGraph to be wired)
auto graph = std::make_shared<TensorFingerprintGraph>();
repo.setFingerprintGraph(graph);
auto similar = repo.findSimilarAdapters("legal", "llama3-8b", /*k=*/5);

// Diagnostics
auto stats = repo.stats();
// stats.total_adapters, stats.load_hits, stats.load_misses
```

**Thread Safety:** All public methods are thread-safe. Reads use `shared_mutex`; writes take exclusive ownership.

**Error Cases:**
- `store()` returns `false` if serialisation or backend write fails.
- `loadAdapter()` returns `GgmlCoreDescriptor{valid=false}` if the adapter is not found.
- `findSimilarAdapters()` returns empty if `setFingerprintGraph()` was not called.

---

### tensor_fingerprint_graph.h

**Purpose:** Fast approximate similarity lookup between TT-encoded LoRA adapters using
column-mean fingerprints of the first TT-core (G₀). Phase 4 preparation.

**Key Types:**

| Type | Description |
|------|-------------|
| `TensorFingerprintGraph` | Fingerprint registry + cosine-similarity search |
| `FingerprintEntry` | Per-adapter record: key, domain, base_model_id, tenant_id, fingerprint vector |
| `SimilarityResult` | One similarity hit: key, domain, base_model_id, score ∈ [−1, 1] |

**STUB #174:** Similarity is approximate (fingerprint cosine, not full TT inner-product).
For adapters where G₀ energy < 60% of total Frobenius norm, rankings may differ by up to 15%
from the exact result. Full TT inner-product similarity deferred to Q3 2027.

**Key API:**
```cpp
#include "tensor/tensor_fingerprint_graph.h"

TensorFingerprintGraph graph;

// Register an adapter
graph.addAdapter("__adapters__:t1:legal:llama3",
                 adapter_tt, "legal", "llama3", "t1");

// Find similar adapters
auto results = graph.findSimilar("__adapters__:t1:legal:llama3", /*k=*/5);
for (auto& r : results) {
    // r.adapter_key, r.domain, r.base_model_id, r.score
}

// Raw fingerprint query (without storing first)
auto results2 = graph.findSimilarByFingerprint(fingerprint_vec, 5, "t1");

// Remove
graph.removeAdapter("__adapters__:t1:legal:llama3");

// Inspection
auto entry  = graph.entry("__adapters__:t1:legal:llama3");
auto keys   = graph.adapterKeys();
auto stats  = graph.stats();
```

**Thread Safety:** All public methods are thread-safe via `shared_mutex`.

---

### tensor_butterfly_operator.h

**Purpose:** Tensor butterfly algorithm for oscillatory integral operators (Radon,
Fourier, Green's functions) in TT-format. Applies O(n·d·log n·r²) transforms
to TT-format data without decompression.

**Key Types:**

| Type | Description |
|------|-------------|
| `TensorButterflyOperator` | Immutable butterfly operator in TT-network format |
| `ButterflyConfig` | Build parameters: `type`, `grid_shape`, `precision` |
| `OperatorType` | `FOURIER` (Walsh-Hadamard proxy), `RADON` (stub), `GREENS_FUNCTION` (stub) |

**Key API:**
```cpp
#include "tensor/tensor_butterfly_operator.h"

// Build a FOURIER operator (Walsh-Hadamard Transform, grid must be powers of 2)
auto op = TensorButterflyOperator::build(
    OperatorType::FOURIER, {8, 8}, /*precision=*/1e-6f);

// Apply to a TT-format data tensor
TTTrain freq = op.apply(data_tt);

// Free-function style alias
TTTrain freq2 = TensorButterflyOperator::contractOperator(data_tt, op);

// Introspection
op.type();       // OperatorType::FOURIER
op.gridShape();  // {8, 8}
op.precision();  // 1e-6f
op.describe();   // human-readable string for AQL EXPLAIN

// Inject custom FOURIER backend (STUB #267)
TensorButterflyOperator::setFourierTransformFn(
    [](std::vector<float>& fiber) { /* real FFT */ });
TensorButterflyOperator::clearFourierTransformFn();
```

**STUB #171:** `RADON` and `GREENS_FUNCTION` are not yet implemented.
`build()` throws `std::logic_error` for these types until a bridge function is installed
via `setRadonTransformFn()` / `setGreensTransformFn()`.

**Error Cases:**
- `build()` throws `std::invalid_argument` if `grid_shape` is empty or any mode is not a power of 2 (FOURIER).
- `apply()` throws `std::invalid_argument` if data shape is incompatible with the operator grid.

---

### hiss_structural_search.h

**Purpose:** Tensor-network graph primitives for Hiss/TNSR Phase-6 structural search.
Provides entropy-guided topology search, pure-binary quantics-based reshape with
reversible index mapping, and a domain template catalog.

**Key Types:**

| Type | Description |
|------|-------------|
| `HissStructuralSearchEngine` | Builds optimised `TensorNetworkGraph` from a TT train |
| `TensorNetworkGraph` | Mutable graph: `addNode()`, `addEdge()`, `rerouteEdge()`, `neighbors()` |
| `TensorGraphNode` | Node descriptor: mode_index, rank_left, rank_right, entropy_score |
| `TensorGraphEdge` | Edge descriptor: from, to, weight, topology |
| `HissConfig` | Tuning: `num_samples`, `entropy_threshold`, `max_reshape_depth`, `diversity_budget` |
| `HissReshaper` | Pure-binary quantics reshape via `exposeQuantics()`; optional custom backend via `setQuanticsFn()` |
| `QTTrain` | Quantics TT: bit_depths, grid_sizes, padded_grid_sizes, quantics_mode_sizes, original_element_count, **mapping** |
| `QTTMappingDescriptor` | Reversible physical↔QTT flat-index map: `physicalToQTT()`, `qttToPhysical()` |
| `TemplateCatalog` | Thread-safe domain→graph registry for TN template reuse |

**Key API:**
```cpp
#include "tensor/hiss_structural_search.h"

// Structural topology search
HissStructuralSearchEngine engine;
HissConfig cfg;
cfg.num_samples       = 64;
cfg.entropy_threshold = 0.35;
TensorNetworkGraph tng = engine.search(train, cfg);

// Quantics reshape — pure-binary padded QTT (built-in path)
QTTrain qtt = HissReshaper::exposeQuantics(train, {8, 8});
// qtt.original_element_count — number of valid (non-padding) elements
// qtt.mapping                — reversible physical↔QTT index map

// Reversible index mapping
size_t physical_idx = 42;
size_t qtt_idx      = qtt.mapping.physicalToQTT(physical_idx);
auto   back         = qtt.mapping.qttToPhysical(qtt_idx); // == 42
// Padding slots: qttToPhysical() returns std::nullopt for zero-padded QTT indices

// Optional custom quantics backend
HissReshaper::setQuanticsFn([](const TTTrain& t, const std::vector<size_t>& gs) {
    // Custom pure-binary QTT encode; populate QTTrain::mapping as needed.
    return /* ... */;
});

// Template catalog
TemplateCatalog catalog;
catalog.registerTemplate("legal", tng);
auto tmpl = catalog.lookup("legal");
```

---

### tnsr_task.h

**Purpose:** Background maintenance task that performs TensorNetworkStructuralRounding (TNSR):
rank reduction via recompression + optional topology mutation via `HissStructuralSearchEngine`.

**Key Types:**

| Type | Description |
|------|-------------|
| `TNSRTask` | Background TNSR runner; cancellable |
| `TNSRConfig` | Tuning: `epsilon`, `max_topology_changes_per_run`, `min_bytes_saved_to_commit`, `run_frequency_hours` |
| `TNSRReport` | Run summary: `bytes_saved`, `rank_delta`, `topology_changes`, `keys_rewritten`, `error_count`, `duration_ms` |

**Key API:**
```cpp
#include "tensor/tnsr_task.h"

TNSRTask task(engine, decomposer);

TNSRConfig cfg;
cfg.epsilon                   = 0.01;
cfg.max_topology_changes_per_run = 4;
cfg.min_bytes_saved_to_commit    = 64;

// Run on a subset of keys (blocking)
TNSRReport report = task.run(key_range, cfg);
// report.bytes_saved, report.rank_delta, report.keys_rewritten

// Cancellation (from another thread)
task.requestCancel();
task.clearCancel();

// Inject topology re-serialization backend (STUB #252)
TNSRTask::setRerouteSerializeFn(
    [](auto& eng, const auto& key, const auto& tng, const auto& train) -> bool {
        // Persist mutated topology to storage
        return true;
    });
TNSRTask::clearRerouteSerializeFn();
```

**STUB #252:** Topology mutations via `rerouteEdge()` are counted but NOT persisted
to storage unless a `RerouteSerializeFn` is installed. Only bond-dimension reductions
(rank_delta > 0) are durable in this release. Full topology-guided re-serialisation
is deferred to Q3 2028.

**Acceptance Criteria (Phase 6):**
- AC-storage: live index storage decreases ≥ 15% over 24 h
- AC-accuracy: cosine δ < 0.001 before vs. after TNSR
- AC-overhead: ≤ 5% additional CPU vs. baseline compaction
- AC-frequency: at most once per `run_frequency_hours` per index

**Thread Safety:** Do not call `run()` from multiple threads concurrently on the same engine.
`requestCancel()` / `clearCancel()` are safe to call from any thread.

---

### encoder_interface.h

**Purpose:** Abstract encoder interfaces enabling pluggable text and image encoding
backends for `UTRConverter::fromDocument()` and `UTRConverter::fromImage()`.
Implementing these interfaces replaces the built-in lexical / patch-statistics
fallbacks without changing any caller code.

**Key Types:**

| Type | Description |
|------|-------------|
| `EncoderQuality` | Quality tier: `SEMANTIC` (learned), `LEXICAL` (statistical), `HASH` (hash-projection fallback) |
| `ITextEncoder` | Abstract interface for text segment encoders; `encode(segment, embed_dim)` + `isAvailable()` + `quality()` + `description()` |
| `IImageEncoder` | Abstract interface for image-to-TTTrain encoders; same lifecycle methods |

**Key API:**
```cpp
#include "tensor/encoder_interface.h"
#include "tensor/utr_converter.h"

// Implement ITextEncoder for a custom semantic backend
class MySentenceEncoder final : public themis::tensor::ITextEncoder {
public:
    std::vector<float> encode(const std::string& seg, std::size_t dim) const override {
        // ... call quantised BERT, sentence-transformers, etc.
    }
    bool isAvailable() const noexcept override { return model_loaded_; }
    themis::tensor::EncoderQuality quality() const noexcept override {
        return themis::tensor::EncoderQuality::SEMANTIC;
    }
    std::string_view description() const noexcept override { return "BERT-base Q4_0"; }
private:
    bool model_loaded_ = true;
};

// Register the encoder — takes priority over EmbedFn bridge and built-in fallback
themis::tensor::UTRConverter::setTextEncoder(std::make_shared<MySentenceEncoder>());

// Remove the encoder; reverts to EmbedFn bridge or built-in lexical encoder
themis::tensor::UTRConverter::clearTextEncoder();
```

**Encoder Priority Chain (for both `fromDocument` and `fromImage`):**
1. Registered `ITextEncoder` / `IImageEncoder` (if `isAvailable()` returns true)
2. Injected `EmbedFn` / `ImageEmbedFn` bridge function (STUB #257 / #258)
3. Built-in lexical / patch-statistics encoder (degraded mode)

**Error Handling:**
- If a registered encoder is unavailable (`isAvailable() == false`), the system
  silently falls back to the next tier without throwing.
- If an encoder returns a vector of the wrong size (dimension mismatch), a
  `std::runtime_error` is thrown immediately (**fail-closed**).
- If an `IImageEncoder` returns an empty `TTTrain`, a `std::runtime_error` is thrown.

---

### utr_converter.h

**Purpose:** Unified Tensor Representation (UTR) pipeline — converts geospatial, tabular,
image, and document data into TT/HT objects directly indexable by `TensorIndexManager`.

**Key Types:**

| Type | Description |
|------|-------------|
| `UTRConverter` | Stateless converter (all methods `static`) |
| `UTRConfig` | Shared config: `eps`, `max_rank`, `embed_dim`, `max_segments`, `bucket_count` |
| `RasterGrid` | Geospatial raster input: rows, cols, lat_min, lon_min, cell_size_deg, values |
| `DocumentStructureHint` | `NONE`, `PARAGRAPHS`, `SENTENCES` |

**Key API:**
```cpp
#include "tensor/utr_converter.h"

UTRConfig cfg;
cfg.eps       = 0.01;
cfg.max_rank  = 16;
cfg.embed_dim = 64;

// Geospatial raster → TTTrain (Hilbert-curve reorder + TT-SVD)
RasterGrid grid;
grid.rows = grid.cols = 16;
grid.values = { /* 256 floats */ };
TTTrain geo_tt = UTRConverter::fromGeospatial(grid, cfg);

// Tabular data → HyperIndexTensor (TT-encoded co-occurrence tensor)
HyperIndexTensor ht = UTRConverter::fromTabular("tenant1", schema, rows, cfg);

// Image → TTTrain (patch-statistics + TT-decomposition)
TTTrain img_tt = UTRConverter::fromImage(pixels, h, w, c, cfg);

// Document → HTTrain (segment embeddings + HT-decomposition)
HTTrain doc_ht = UTRConverter::fromDocument(text,
    DocumentStructureHint::PARAGRAPHS, cfg);

// Register a learned encoder (preferred API — see encoder_interface.h)
UTRConverter::setTextEncoder(std::make_shared<MySentenceEncoder>());
UTRConverter::clearTextEncoder();

UTRConverter::setImageEncoder(std::make_shared<MyPatchEncoder>());
UTRConverter::clearImageEncoder();

// Inject raw bridge functions (legacy API — STUB #257 / #258)
UTRConverter::setEmbedFn(
    [](const std::string& seg, size_t dim) -> std::vector<float> {
        return /* real sentence encoder output */;
    });
UTRConverter::clearEmbedFn();

UTRConverter::setImageEmbedFn(
    [](const auto& pixels, size_t h, size_t w, size_t c,
       const UTRConfig& cfg) -> TTTrain { return /* custom */; });
UTRConverter::clearImageEmbedFn();
```

**Precision Contracts:**
- Geospatial: coordinate round-trip loss ≤ 1e-7 degrees.
- All outputs: dense round-trip RMSE ≤ configured `eps`.
- No tenant data mixing.

**Error Cases:**
- `fromGeospatial()` throws `std::invalid_argument` if grid is empty or `cell_size_deg ≤ 0`.
- `fromTabular()` throws `std::invalid_argument` if schema has < 2 columns or rows is empty.
- `fromImage()` throws `std::invalid_argument` if h, w, or c is 0, or `pixels.size() != h*w*c`.
- `fromDocument()` throws `std::invalid_argument` if text is empty.
- Custom `EmbedFn` must return exactly `embed_dim` elements or `fromDocument()` throws `std::runtime_error`.

---

### hyper_index_builder.h

**Purpose:** Builds a TT-encoded co-occurrence tensor from tabular data to expose
latent cross-column relationships invisible to the relational query planner.

**Key Types:**

| Type | Description |
|------|-------------|
| `HyperIndexBuilder` | Stateless builder (all methods `static`) |
| `HyperIndexTensor` | TT-encoded result: `tt_train`, `schema`, `bucket_count`, `total_rows` |
| `ColumnSchema` | Column descriptor: `name`, `type` (`NUMERIC`, `CATEGORY`, `BOOLEAN`), range/categories |
| `TableRow` | One row: `numeric_values`, `category_values`, `bool_values` |
| `HyperIndexConfig` | Build params: `bucket_count`, `eps`, `max_rank` |
| `ColumnType` | `NUMERIC`, `CATEGORY`, `BOOLEAN` |

**Key API:**
```cpp
#include "tensor/hyper_index_builder.h"

std::vector<ColumnSchema> schema = {
    {"income",  ColumnType::NUMERIC,   0.0, 200000.0},
    {"region",  ColumnType::CATEGORY,  0.0, 0.0, {"north", "south", "east", "west"}},
    {"is_risk", ColumnType::BOOLEAN},
};

HyperIndexConfig cfg;
cfg.bucket_count = 8;
cfg.eps          = 0.05;

HyperIndexTensor result = HyperIndexBuilder::fromSchema("tenant1", schema, rows, cfg);

// Query a marginal (contract out all modes except the pinned ones)
double count = result.contract({{0, 3}, {2, 1}});
// → estimated count of rows where income is in bucket 3 AND is_risk == true

// Inject FK-aware bucket assignment (optional)
HyperIndexBuilder::setBucketAssignmentFn(
    [](const std::string& tid, const auto& schema, const auto& row,
       size_t row_idx, const auto& default_buckets) {
        return default_buckets; // or custom assignment
    });
HyperIndexBuilder::clearBucketAssignmentFn();
```

**Error Cases:**
- `fromSchema()` throws `std::invalid_argument` if schema has < 2 columns or rows is empty.

---

### ht_train.h

**Purpose:** Hierarchical Tucker (HT) tensor decomposition types and compressed-domain
arithmetic engine. Used by `IHierarchicalTuckerIndex` and `UTRConverter::fromDocument()`.

**Key Types:**

| Type | Description |
|------|-------------|
| `HTTrain` | Complete HT decomposition: `root`, `shape`, `max_rank`, `achieved_eps`, `original_norm` |
| `HTNode` | One tree node: leaf (basis matrix U) or internal (transfer tensor B) |
| `HTContractionEngine` | Compressed-domain arithmetic: `innerProduct`, `frobeniusNorm`, `cosineSimilarity` |

**Storage Complexity:** O(d·n·r + d·r³) floats, where d = number of modes, n = mode size, r = HT rank.
**Inner-product Complexity:** O(d·n·r² + d·r⁴) — Grasedyck 2010 §4.

**Key API:**
```cpp
#include "tensor/ht_train.h"

// Obtained from UTRConverter::fromDocument() or HierarchicalTuckerDecomposer
HTTrain ht = UTRConverter::fromDocument(text);

// Compressed-domain arithmetic (no decompression)
double ip   = HTContractionEngine::innerProduct(ht_a, ht_b);
double norm = HTContractionEngine::frobeniusNorm(ht);
double cos  = HTContractionEngine::cosineSimilarity(ht_a, ht_b);

// Introspection
ht.order();            // number of modes d
ht.totalParams();      // total float parameters in the HT tree
ht.compressionRatio(); // (∏ n_k) / totalParams; > 1 means compressed
ht.achieved_eps;       // relative reconstruction error ‖T − T̃‖_F / ‖T‖_F

// Compatibility bridge (STUB #178 — full reconstruction + TT redecomposition)
storage::TTTrain tt = ht.toTTTrain();  // slow; for compatibility only

// Dense reconstruction (slow; for correctness testing)
auto dense = ht.reconstruct();

// Persistence
auto bytes = ht.serialize();
auto ht2   = HTTrain::deserialize(bytes);  // std::nullopt on format error

// Deep copy (no implicit copy — move-only by default)
HTTrain copy = ht.clone();
```

**Error Cases:**
- `toTTTrain()` is O(∏ n_k) — full reconstruction; not suitable for production-scale tensors.
- `deserialize()` returns `std::nullopt` on format errors.
- `HTTrain` is move-only; use `clone()` for deep copy.

---

### ht_index.h

**Purpose:** Abstract interface and linear-scan implementation for Hierarchical Tucker (HT)
vector indexes. Mirrors `ITensorIndex` / `FlatTensorIndex` for the HT format.

**Key Types:**

| Type | Description |
|------|-------------|
| `IHierarchicalTuckerIndex` | Abstract interface: `add`, `remove`, `search`, `serialize`, `deserialize` |
| `FlatHTIndex` | Linear-scan implementation; exact cosine similarity; suitable for n ≤ ~10 000 |
| `HTSearchResult` | One k-NN result: `id` (string), `similarity` ∈ [−1, 1] |

**Key API:**
```cpp
#include "tensor/ht_index.h"

FlatHTIndex idx;

// Insert (replaces existing entry with same id)
idx.add("doc-42", std::move(ht_train));

// k-NN search (exact cosine similarity on all entries)
auto results = idx.search(query_ht, /*k=*/10);
for (auto& r : results) {
    // r.id, r.similarity
}

// Retrieve by id
auto entry = idx.get("doc-42");  // std::optional<const HTTrain*>

// Remove
bool removed = idx.remove("doc-42");

// Persistence
auto bytes  = idx.serialize();
bool loaded = idx.deserialize(bytes);  // false on format error
```

**Thread Safety:**
Concurrent reads (`search`, `get`) are safe. Writes (`add`, `remove`) are serialised
by an internal mutex.

**Limits:** `FlatHTIndex` is O(n) per query. For n > 10 000, an HNSW-backed HT index
(`HnswHTBridge`) is planned for Phase 5 (Q2 2028).

---

## Configuration Quick Reference

| Parameter | Header | Default | Description |
|-----------|--------|---------|-------------|
| `HnswTTConfig::M` | `hnsw_tt_bridge.h` | 16 | HNSW neighbours per node |
| `HnswTTConfig::ef_construction` | `hnsw_tt_bridge.h` | 200 | HNSW build quality |
| `HnswTTConfig::ef_search` | `hnsw_tt_bridge.h` | 50 | HNSW search candidate pool |
| `HnswTTConfig::sketch_dim` | `hnsw_tt_bridge.h` | 64 | First-core sketch dimension |
| `HnswTTConfig::rerank_candidates` | `hnsw_tt_bridge.h` | 200 | TT re-rank pool size |
| `HnswTTConfig::max_tt_rank` | `hnsw_tt_bridge.h` | 32 | Max TT bond dimension |
| `HnswTTConfig::epsilon` | `hnsw_tt_bridge.h` | 0.01 | Relative reconstruction error |
| `HnswTTConfig::kappa_min` | `hnsw_tt_bridge.h` | 2.0 | Min κ for HYBRID route |
| `HnswTTConfig::kappa_max` | `hnsw_tt_bridge.h` | 6.0 | Max κ (above → pure TT) |
| `TensorIngestionBridge::setEpsilon()` | `tensor_ingestion_bridge.h` | 0.01 | TT-SVD error tolerance |
| `TensorIngestionBridge::setMaxRank()` | `tensor_ingestion_bridge.h` | 0 (no cap) | Bond-dimension cap |
| `TensorIngestionBridge::setMinKappa()` | `tensor_ingestion_bridge.h` | 1.3 | κ-gate threshold |
| `HyperIndexConfig::bucket_count` | `hyper_index_builder.h` | 8 | Buckets per column dimension |
| `HyperIndexConfig::eps` | `hyper_index_builder.h` | 0.05 | TT reconstruction error |
| `HyperIndexConfig::max_rank` | `hyper_index_builder.h` | 16 | Hard cap on TT-rank |
| `TNSRConfig::epsilon` | `tnsr_task.h` | 0.01 | TNSR recompression tolerance |
| `TNSRConfig::max_topology_changes_per_run` | `tnsr_task.h` | 4 | Max rerouteEdge calls per run |
| `TNSRConfig::min_bytes_saved_to_commit` | `tnsr_task.h` | 64 | Write-back threshold (bytes) |
| `UTRConfig::eps` | `utr_converter.h` | 0.01 | TT/HT reconstruction error |
| `UTRConfig::max_rank` | `utr_converter.h` | 16 | Hard cap on TT/HT rank |
| `UTRConfig::embed_dim` | `utr_converter.h` | 64 | Embedding dimension for documents |
| `HissConfig::num_samples` | `hiss_structural_search.h` | 64 | Stochastic sub-network samples |
| `HissConfig::entropy_threshold` | `hiss_structural_search.h` | 0.35 | Entropy skip threshold |

---

## Runtime Behavior, Errors, and Limits

### Routing Thresholds (TensorRouter)

| κ value | dim | Recommended route |
|---------|-----|-------------------|
| κ ≥ 1.7 and dim ≥ 256 | any | `TENSOR_TRAIN` |
| κ ≥ 1.3 | any | `HYBRID` |
| κ < 1.3 | any | `HNSW` (use `src/index` path) |

### Performance Expectations

| Operation | Complexity | Notes |
|-----------|------------|-------|
| `ITensorIndex::search()` (pure TT) | O(n · d · r²) | Linear scan in Phase 1; n = number of stored vectors |
| `HnswTTBridge::search()` | O(C · d · r²) where C = rerank_candidates | C << n (total vectors) |
| `ITensorIndex::innerProduct()` | O(d · r³) | Holtz 2012 TT inner-product |
| `HTContractionEngine::innerProduct()` | O(d·n_k·r² + d·r⁴) | n_k = mode size; Grasedyck 2010 §4 |
| `TensorButterflyOperator::apply()` | O(n_k · d · log₂(n_k) · r²) | n_k = mode size; butterfly factorisation |
| `HissStructuralSearchEngine::search()` | O(num_samples · d) | Entropy-guided sampling |
| `TNSRTask::run()` | O(keys · d · r²) | Per-key recompression + topology analysis |
| `TensorIngestionBridge::shouldDecompose()` | O(√dim) pilot | Rademacher projection for dim > 1024 |

### Memory Limits

- `FlatHTIndex`: exact cosine scan; recommended limit ~10 000 entries.
- `FlatTensorIndex`: linear scan; O(n · d · r²) per query (n = total vectors).
- `TensorMmapBridge`: mlock() subject to system `RLIMIT_MEMLOCK`; unprivileged containers
  may have 0 limit — bridge still works but pages are not locked.

### Common Error Cases

| Situation | Return value |
|-----------|--------------|
| `createIndex()` when route == HNSW | `nullptr` |
| `getIndex()` when not found | `nullptr` |
| `mapCores()` when vector ID missing | `nullptr` (`unique_ptr`) |
| `add()` with duplicate ID | `false` |
| `addFlat()` with `dim == 0` | `false` |
| `remove()` when ID not found | `false` |
| `save()` / `load()` (Phase-1 stubs) | `false` |
| `HnswTTBridge::save()` / `load()` (Phase-2 stubs) | `false` |
| `makeKey()` with empty/slash argument | `std::invalid_argument` |
| `HyperIndexBuilder::fromSchema()` with < 2 columns | `std::invalid_argument` |
| `TensorButterflyOperator::build(RADON, ...)` without bridge | `std::logic_error` |
| `TensorMmapBridge` slice after `release()` | undefined behaviour — do not use |
| `HTTrain` copy construction | deleted — use `clone()` |

---

## Stub Inventory

| Stub ID | Location | Description | Target |
|---------|----------|-------------|--------|
| TTI-01 | `tensor_index.h` / `tensor_index.cpp` | `FlatTensorIndex::save()` | Phase 2 Q4 2026 |
| TTI-02 | `tensor_index.h` / `tensor_index.cpp` | `FlatTensorIndex::load()` | Phase 2 Q4 2026 |
| TIM-01 | `tensor_index_manager.h` | `ggmlCorePtrs()` deprecated; `mapCores()` preferred | Phase 3 Q1 2027 |
| TIM-02 | `tensor_index_manager.cpp` | `dropTenantIndexes()` RocksDB delete | Phase 2 Q4 2026 |
| HTB-01 | `hnsw_tt_bridge.cpp` | HNSW layer (linear scan fallback) | Phase 2 Q4 2026 |
| HTB-02 | `hnsw_tt_bridge.h` | `HnswTTBridge::save()` | Phase 2 Q4 2026 |
| HTB-03 | `hnsw_tt_bridge.h` | `HnswTTBridge::load()` | Phase 2 Q4 2026 |
| #160 | `tensor_core_bridge.h` | RocksDB backend not wired; uses `InMemoryTensorBackend` | Phase 2 Q4 2026 |
| #172 | `adapter_repository.h` | `loadAdapter()` heap-copies cores (no mmap) | Phase 3 Q1 2027 |
| #174 | `tensor_fingerprint_graph.h` | Fingerprint cosine, not full TT inner-product | Phase 4 Q3 2027 |
| #176 | `tensor_mmap_bridge.h` | `MAP_ANONYMOUS` + memcpy; no `MAP_SHARED` on SST pages | Phase 3 Q1 2027 |
| #178 | `ht_train.h` | `toTTTrain()` full reconstruction path | Phase 5 Q2 2028 |
| #252 | `tnsr_task.h` | Topology mutations not persisted without `RerouteSerializeFn` | Phase 6 Q3 2028 |
| #254 | `hiss_structural_search.h` | `exposeQuantics()` residual-factor fallback | Phase 6 Q2 2028 |
| #257 | `utr_converter.h` | `fromDocument()` built-in lexical encoder (FNV-hash + bigrams + L2 norm); superseded by registered `ITextEncoder` | Phase 7 — pluggable via `setTextEncoder()` |
| #258 | `utr_converter.h` | `fromImage()` built-in patch-statistics encoder; superseded by registered `IImageEncoder` | Phase 7 — pluggable via `setImageEncoder()` |
| #265 | `adapter_repository.h` | `setMmapLoadFn` bridge not wired | Phase 3 Q1 2027 |
| #266 | `adapter_repository.h` | `setExactSimilarityFn` bridge not wired | Phase 4 Q3 2027 |
| #267 | `tensor_butterfly_operator.h` | `setFourierTransformFn` bridge | Phase 3 Q2 2027 |
| #268 | `tensor_butterfly_operator.h` | RADON/GREENS_FUNCTION not implemented | Phase 3 Q3 2027 |
| #269 | `tensor_core_bridge.h` | `setDefaultBackendFactory` STUB bridge | Phase 2 Q4 2026 |
| #270 | `tensor_mmap_bridge.h` | `setSstMapFn` SST page-map bridge | Phase 3 Q1 2027 |

---

## Usage Examples

### Example 1 — Insert and search TT-compressed vectors (Phase 1)

```cpp
#include "tensor/tensor_index_manager.h"

auto mgr = TensorIndexManager::create(db);

// Let the router decide
auto route = mgr->routeFor("tenant1", "llm_weights", "attention_k", 4096, 1'000'000);
// route == TensorRouter::Route::TENSOR_TRAIN for κ ≈ 4.5, dim=4096

// Create the index
auto* idx = mgr->createIndex("tenant1", "llm_weights", "attention_k");
if (!idx) { /* route was HNSW — use src/index path */ }

// Insert 1000 flat vectors
for (int i = 0; i < 1000; ++i) {
    idx->addFlat(i, vectors[i].data(), 4096);
}

// k-NN query (10 nearest)
auto results = idx->searchFlat(query.data(), 4096, 10);
for (auto& r : results) {
    std::cout << "id=" << r.id << " distance=" << r.distance << "\n";
}

// Persist to disk
mgr->setDataDir("/var/themisdb/tensor");
mgr->flushAll();
```

### Example 2 — Zero-copy GGML injection (Phase 3)

```cpp
#include "tensor/tensor_index_manager.h"
#include "tensor/tensor_mmap_bridge.h"

auto bridge = mgr->mapCores("tenant1", "llm_weights", "attention_k", /*id=*/42);
if (!bridge) { /* vector not found */ return; }

// Pass pinned core pointers directly to llama.cpp ggml graph
for (const auto& s : bridge->slices()) {
    inject_to_ggml(s.data, s.num_elems, s.core_idx);
}
// bridge destructor: munlock + munmap called automatically
```

### Example 3 — Ingestion pipeline with TT-core persistence

```cpp
#include "tensor/tensor_ingestion_bridge.h"
#include "tensor/tensor_core_bridge.h"
#include "ingestion/builtin_step_factories.h"

// Set up RocksDB backend (production bootstrap)
TensorCoreStorageBridge::setDefaultBackendFactory([&] {
    return std::make_shared<RocksDBTensorBackend>(db_handle);
});

// Wire ingestion bridge
auto decompose_bridge = std::make_shared<TensorIngestionBridge>(
    /*epsilon=*/0.01, /*max_rank=*/64, /*min_kappa=*/1.3);
auto storage_bridge = std::make_shared<TensorCoreStorageBridge>();

auto decompose_step = ingestion::builtin::createChunkTtDecomposeStep(decompose_bridge);
auto storage_step   = ingestion::builtin::createTensorCoreBridgeStep(storage_bridge);

// Register steps in WorkflowEngine profile AFTER chunk_embed
engine.registerStep(decompose_step);
engine.registerStep(storage_step);
```

### Example 4 — LoRA adapter sovereignty

```cpp
#include "tensor/adapter_repository.h"
#include "tensor/tensor_fingerprint_graph.h"

auto backend = std::make_shared<RocksDBTensorBackend>(db);
AdapterRepository repo(backend, "tenant1");
auto graph = std::make_shared<TensorFingerprintGraph>();
repo.setFingerprintGraph(graph);

// Store a legal domain adapter
repo.store("legal", "llama3-8b", adapter_tt, {.author = "team-legal"});

// Find similar adapters for cross-domain transfer
auto similar = repo.findSimilarAdapters("legal", "llama3-8b", 5);
for (auto& s : similar) {
    std::cout << s.domain << "/" << s.base_model_id
              << " sim=" << s.score << "\n";
}
```

### Example 5 — Multi-modal encoding with UTR

```cpp
#include "tensor/utr_converter.h"

UTRConfig cfg{.eps = 0.01, .max_rank = 16};

// Geospatial raster → TT index
RasterGrid grid{ .rows=16, .cols=16, .values=raster };
TTTrain geo_tt = UTRConverter::fromGeospatial(grid, cfg);
mgr->createIndex("t1", "flood_risk", "zone_a")->add(1, geo_tt);

// Document → HT index
HTTrain doc_ht = UTRConverter::fromDocument(legal_text,
    DocumentStructureHint::SENTENCES, cfg);
ht_idx.add("doc-123", std::move(doc_ht));
```

---

## Troubleshooting

### `createIndex()` returns `nullptr`

The routing decision was `HNSW`. Use the standard `src/index` path instead.
Check `routeFor()` — if κ < 1.3, your data is not compressible enough for TT.

### `addFlat()` returns `false` consistently

- Verify `dim` matches the vector length.
- Confirm `TensorTrainDecomposer` is configured with a non-trivial epsilon
  (very large epsilon can produce empty trains for some inputs).
- Check that no duplicate IDs are being inserted.

### `mapCores()` always returns `nullptr`

The requested vector ID was not inserted or has been removed. Verify the ID
was returned by a successful `addFlat()` / `add()` call.

### `mlock()` fails in CI / containers

Expected in unprivileged environments. `isLocked()` returns `false` but all
data pointers from `slices()` remain valid — only swapability is affected.
No action required.

### `save()` / `load()` always return `false`

These are Phase-1 stubs. Use `TensorIndexManager::flushAll()` with a configured
`setDataDir()` for file-based persistence until RocksDB integration is complete
(Target: Phase 2, Q4 2026).

### `TensorButterflyOperator::build(RADON, ...)` throws `std::logic_error`

RADON and GREENS_FUNCTION operators are not yet implemented (STUB #171).
Either install a custom backend via `setRadonTransformFn()` / `setGreensTransformFn()`,
or wait for Phase 3 Q3 2027.

### Ingestion step `chunk_tt_decompose` skips all embeddings

The κ-gate threshold may be too high for your data.
Lower `min_kappa` via `setMinKappa()` or the step config key `min_kappa`.
Check `TensorIngestionBridge::kappaSkipCount()` vs. `decomposeCount()`.

### `HTTrain` copy compilation error

`HTTrain` is move-only (no implicit copy). Use `HTTrain::clone()` for a deep copy.

---

## Installation

Headers are provided by the regular ThemisDB build. Include the module via:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

Link the tensor implementation object:

```cmake
target_link_libraries(your_target PRIVATE themis_tensor)
```

Include individual headers as needed:

```cpp
#include "tensor/tensor_index_manager.h"    // primary entry point
#include "tensor/hnsw_tt_bridge.h"          // HYBRID mode
#include "tensor/tensor_ingestion_bridge.h" // ingestion pipeline
```

---

## See Also

- **Implementation overview:** [`../../src/tensor/README.md`](../../src/tensor/README.md)
- **Architecture:** [`../../src/tensor/ARCHITECTURE.md`](../../src/tensor/ARCHITECTURE.md)
- **Roadmap:** [`../../src/tensor/ROADMAP.md`](../../src/tensor/ROADMAP.md)
- **Future enhancements:** [`../../src/tensor/FUTURE_ENHANCEMENTS.md`](../../src/tensor/FUTURE_ENHANCEMENTS.md)
- **Storage layer:** [`../storage/README.md`](../storage/README.md)
- **Ingestion layer:** [`../ingestion/README.md`](../ingestion/README.md)
- **Boundary analysis:** `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`
- **Scientific background:** Oseledets 2011 (TT-SVD, DOI:10.1137/090752142), Holtz et al. 2012, Grasedyck 2010 (HT)
