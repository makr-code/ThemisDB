# Tensor Module — README

> **Module path:** `src/tensor/`
> **Namespace:** `themis::tensor`
> **Status:** 🟡 Experimental — Phase 1 complete, Phase 2 in progress (Q4 2026)

**Related docs:**
- Public API: [`../../include/tensor/README.md`](../../include/tensor/README.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Security: [`SECURITY.md`](SECURITY.md)
- Changelog: [`CHANGELOG.md`](CHANGELOG.md)

## Overview

The `tensor` module is a **first-class, standalone index module** parallel to
`src/index/` (HNSW, FAISS, ScaNN, DiskANN).  It provides Tensor-Train (TT)
and Hierarchical Tucker (HT) compressed approximate-nearest-neighbour (ANN)
indexing for ThemisDB, with a clean Separation of Concerns (SOC) boundary:

| Module        | Strength                                      | Use when                             |
|---------------|-----------------------------------------------|--------------------------------------|
| `src/index`   | Sub-ms queries, float32 vectors, n ≥ 1 M      | dim ≤ 4096, κ < 2×, standard ANN    |
| `src/tensor`  | Structured compressibility, zero-copy GGML    | dim > 4096, κ ≥ 2×, LLM / science  |

Boundary analysis: `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`

## Main Components

### Phase 1 — Core TT Index (✅ Complete)

| File | Description |
|------|-------------|
| `include/tensor/tensor_index.h` | `ITensorIndex` interface — all TT-based index backends |
| `src/tensor/tensor_index.cpp` | `FlatTensorIndex` — Phase-1 linear scan reference implementation |
| `include/tensor/tensor_index_manager.h` | `TensorIndexManager` — lifecycle manager, routing, GGML bridge entry-point |
| `src/tensor/tensor_index_manager.cpp` | Manager implementation |
| `include/tensor/tensor_ingestion_bridge.h` | `TensorIngestionBridge` — `ITensorDecompositionBackend` for the ingestion pipeline |
| `src/tensor/tensor_ingestion_bridge.cpp` | `TensorIngestionBridge` implementation |
| `include/tensor/tensor_core_bridge.h` | `TensorCoreStorageBridge` — `ITensorCoreBridge` persisting TT-cores to `ITensorStorageBackend` |
| `src/tensor/tensor_core_bridge.cpp` | `TensorCoreStorageBridge` implementation |

### Phase 2 — HNSW Hybrid Bridge (🟡 In Progress Q4 2026)

| File | Description |
|------|-------------|
| `include/tensor/hnsw_tt_bridge.h` | `HnswTTBridge` — hybrid HNSW navigation + TT re-ranking (HYBRID regime) |
| `src/tensor/hnsw_tt_bridge.cpp` | HNSW+TT bridge implementation (HNSW layer is a linear scan stub until Q4 2026) |

### Phase 3 — Zero-Copy GGML Bridge & Adapter Sovereignty (🟡 Experimental)

| File | Description |
|------|-------------|
| `include/tensor/tensor_mmap_bridge.h` | `TensorMmapBridge` — RAII mmap-pinned TT-core pages for GGML injection |
| `src/tensor/tensor_mmap_bridge.cpp` | mmap bridge implementation (STUB #176: MAP_ANONYMOUS until Q1 2027) |
| `include/tensor/adapter_repository.h` | `AdapterRepository` — LoRA/PEFT adapters stored as TT graphs |
| `src/tensor/adapter_repository.cpp` | Adapter repository implementation |
| `include/tensor/tensor_butterfly_operator.h` | `TensorButterflyOperator` — oscillatory integral operators in TT format |
| `src/tensor/tensor_butterfly_operator.cpp` | Butterfly operator (FOURIER/WHT complete; RADON/GREENS_FUNCTION stubs) |

### Phase 4 — Adapter Fingerprint Graph (🟡 Experimental)

| File | Description |
|------|-------------|
| `include/tensor/tensor_fingerprint_graph.h` | `TensorFingerprintGraph` — fast approximate adapter similarity |
| `src/tensor/tensor_fingerprint_graph.cpp` | Fingerprint graph implementation (STUB #174: column-mean cosine approx) |

### Phase 5 — Hierarchical Tucker Index (🟡 Experimental Q1 2028)

| File | Description |
|------|-------------|
| `include/tensor/ht_train.h` | `HTTrain`, `HTNode`, `HTContractionEngine` — HT decomposition types |
| `include/tensor/ht_index.h` | `IHierarchicalTuckerIndex`, `FlatHTIndex` — HT linear-scan index |
| `src/tensor/ht_index.cpp` | `FlatHTIndex` implementation |

### Phase 6 — Adaptive Structural Rounding (🟡 Experimental Q2-Q3 2028)

| File | Description |
|------|-------------|
| `include/tensor/hiss_structural_search.h` | `HissStructuralSearchEngine`, `HissReshaper`, `TemplateCatalog` |
| `src/tensor/hiss_structural_search.cpp` | Hiss structural search implementation |
| `include/tensor/tnsr_task.h` | `TNSRTask` — background TensorNetworkStructuralRounding |
| `src/tensor/tnsr_task.cpp` | TNSR task implementation (STUB #252: topology changes not persisted) |

### Phase 7 — Multi-Modal UTR Encoding (🟡 Experimental Q3-Q4 2028)

| File | Description |
|------|-------------|
| `include/tensor/utr_converter.h` | `UTRConverter` — geospatial/tabular/image/document → TT/HT |
| `src/tensor/utr_converter.cpp` | UTR converter implementation |
| `include/tensor/hyper_index_builder.h` | `HyperIndexBuilder` — tabular co-occurrence TT index |
| `src/tensor/hyper_index_builder.cpp` | HyperIndex builder implementation |

## Dependencies

- `src/storage/tensor_train_decomposer` — TT-SVD core (Oseledets 2011)
- `src/storage/tensor_network_storage_engine` — RocksDB persistence (Phase 2); `ITensorStorageBackend` interface
- `src/storage/tensor_router` — routing decisions (HNSW/TT/HYBRID thresholds)
- `src/index` — HNSW navigation layer for HYBRID mode (Phase 2)
- `src/utils/logger` — THEMIS_WARN macros
- `include/ingestion/inference_backend.h` — `ITensorDecompositionBackend` interface (ingestion SoC boundary)
- `include/ingestion/ingestion_sinks.h` — `ITensorCoreBridge` interface (ingestion SoC boundary)

## Quick Start

### Insert and search TT vectors

```cpp
#include "tensor/tensor_index_manager.h"

// Create manager
auto mgr = themis::tensor::TensorIndexManager::create(db);

// Check routing decision
auto route = mgr->routeFor("tenant1", "llm_weights", "attention_k", 4096, 1'000'000);
// route == TENSOR_TRAIN (κ ≈ 4.5 for dim=4096)

// Create TT-index and insert flat vector
auto* idx = mgr->createIndex("tenant1", "llm_weights", "attention_k");
idx->addFlat(42, attention_vec.data(), 4096);

// Search without decompression (TT-domain cosine)
auto results = idx->searchFlat(query.data(), 4096, 10);
```

### Persist to disk

```cpp
// Configure data directory (call once at startup)
mgr->setDataDir("/var/themisdb/tensor");

// After inserts — persist all in-memory indexes
mgr->flushAll();
```

### Ingest document embeddings into TT-core storage

```cpp
#include "tensor/tensor_ingestion_bridge.h"
#include "tensor/tensor_core_bridge.h"
#include "ingestion/builtin_step_factories.h"

// Production bootstrap: inject RocksDB backend
TensorCoreStorageBridge::setDefaultBackendFactory([&] {
    return std::make_shared<RocksDBTensorBackend>(db_handle);
});

auto decompose_bridge = std::make_shared<TensorIngestionBridge>(
    /*epsilon=*/0.01, /*max_rank=*/64, /*min_kappa=*/1.3);
auto storage_bridge = std::make_shared<TensorCoreStorageBridge>();

auto step1 = ingestion::builtin::createChunkTtDecomposeStep(decompose_bridge);
auto step2 = ingestion::builtin::createTensorCoreBridgeStep(storage_bridge);
// Register steps in WorkflowEngine after chunk_embed
```

## Runtime Behavior

### Routing Thresholds

The `TensorRouter` applies the following heuristic to decide whether a field
should use TT, HYBRID, or HNSW indexing:

| κ (compressibility) | dim | Decision |
|---------------------|-----|----------|
| κ ≥ 1.7 and dim ≥ 256 | any | `TENSOR_TRAIN` |
| κ ≥ 1.3 | any | `HYBRID` |
| κ < 1.3 | any | `HNSW` → use `src/index` |

### Thread Safety

- All `TensorIndexManager` public methods are thread-safe.
- Reads (`search`, `searchFlat`, `innerProduct`, `norm`, `stats`) on any
  `ITensorIndex` can run concurrently.
- Writes (`add`, `addFlat`, `remove`) are **not internally serialised** for
  `FlatTensorIndex` or `HnswTTBridge`; callers must provide external locking
  when inserting from multiple threads concurrently.
- `TensorMmapBridge` instances are NOT thread-safe; single inference-thread use.
- `TensorFingerprintGraph` and `AdapterRepository` are fully thread-safe.

### Performance Expectations

| Operation | Complexity |
|-----------|------------|
| `FlatTensorIndex::search()` | O(n · d · r²) — linear scan (Phase 1) |
| `HnswTTBridge::search()` | O(C · d · r²), C = `rerank_candidates` << n |
| `ITensorIndex::innerProduct()` | O(d · r³) — Holtz 2012 TT inner-product |
| `HTContractionEngine::innerProduct()` | O(d·n·r² + d·r⁴) — Grasedyck 2010 |
| `TensorIngestionBridge::shouldDecompose()` | O(√dim) — Rademacher pilot projection |

For larger datasets, Phase-2 HNSW integration reduces search from O(n) to
O(log n) navigation overhead + O(C·d·r²) TT re-rank.

### Error Cases and Limits

| Situation | Return value |
|-----------|--------------|
| `createIndex()` when route == HNSW | `nullptr` |
| `getIndex()` when not found | `nullptr` |
| `mapCores()` when vector ID missing | `nullptr` (unique_ptr) |
| `add()` with duplicate ID | `false` |
| `addFlat()` with `dim == 0` | `false` |
| `remove()` when ID not found | `false` |
| `save()` / `load()` (Phase-1 stubs) | `false` |
| `makeKey()` with empty/slash argument | `std::invalid_argument` |
| `HyperIndexBuilder::fromSchema()` with < 2 columns | `std::invalid_argument` |
| `TensorButterflyOperator::build(RADON, …)` without bridge | `std::logic_error` |

## Stubs

| ID      | Location                                    | Description                                     | Target |
|---------|---------------------------------------------|-------------------------------------------------|--------|
| TTI-01  | `FlatTensorIndex::save()`                   | File persistence not implemented                | Phase 2 Q4 2026 |
| TTI-02  | `FlatTensorIndex::load()`                   | File load not implemented                       | Phase 2 Q4 2026 |
| TIM-01  | `TensorIndexManager::ggmlCorePtrs()`        | Deprecated raw-pointer variant                  | Phase 3 Q1 2027 |
| TIM-02  | `dropTenantIndexes()` RocksDB delete        | In-memory only; no RocksDB key deletion         | Phase 2 Q4 2026 |
| HTB-01  | `HnswTTBridge` HNSW layer                   | Linear scan fallback until hnswlib wired        | Phase 2 Q4 2026 |
| HTB-02  | `HnswTTBridge::save()`                      | Not implemented                                 | Phase 2 Q4 2026 |
| HTB-03  | `HnswTTBridge::load()`                      | Not implemented                                 | Phase 2 Q4 2026 |
| #160    | `TensorCoreStorageBridge`                   | `InMemoryTensorBackend` until RocksDB wired     | Phase 2 Q4 2026 |
| #172    | `AdapterRepository::loadAdapter()`          | Heap-copy cores; no mmap                        | Phase 3 Q1 2027 |
| #174    | `TensorFingerprintGraph`                    | Column-mean cosine; not full TT inner-product   | Phase 4 Q3 2027 |
| #176    | `TensorMmapBridge`                          | `MAP_ANONYMOUS` + memcpy; no `MAP_SHARED` on SST | Phase 3 Q1 2027 |
| #178    | `HTTrain::toTTTrain()`                      | Full reconstruction for TT compatibility        | Phase 5 Q2 2028 |
| #252    | `TNSRTask`                                  | Topology mutations not persisted without bridge | Phase 6 Q3 2028 |
| #254    | `HissReshaper::exposeQuantics()`            | Residual-factor fallback; not pure-binary QTT   | Phase 6 Q2 2028 |
| #257    | `UTRConverter::fromDocument()`              | FNV-1a hash embedding fallback                  | Phase 7 Q4 2028 |
| #258    | `UTRConverter::fromImage()`                 | Raw-pixel TT; no semantic encoder               | Phase 7 Q4 2028 |

For a complete stub inventory see [`AUDIT.md`](AUDIT.md).

## Troubleshooting

### `createIndex()` returns `nullptr`

The routing decision was `HNSW`. Check `routeFor()` — if κ < 1.3 the data is not
compressible enough for TT. Use the standard `src/index` (`IndexManager`) path instead.

### `save()` / `load()` always return `false`

These are Phase-1 stubs. Use `TensorIndexManager::setDataDir()` + `flushAll()` for
file-based persistence until the RocksDB backend is complete (Target: Q4 2026).

### Ingestion step skips all embeddings (`kappaSkipCount` == `decomposeCount`)

The κ-gate threshold is too high for your data. Lower `min_kappa` via
`TensorIngestionBridge::setMinKappa()` (default: 1.3). Inspect
`TensorIngestionBridge::decomposeCount()` and `kappaSkipCount()` for diagnostics.

### `HnswTTBridge` is not faster than `FlatTensorIndex`

The hnswlib navigation layer is a linear-scan stub until Phase 2 (Q4 2026).
`HnswTTBridge` in Phase 1 has the same O(n) complexity as `FlatTensorIndex`
plus TT re-ranking overhead.

### `mlock()` fails silently in CI / containers

Expected in unprivileged environments (`RLIMIT_MEMLOCK == 0`). Check
`TensorMmapBridge::isLocked()`. Data pointers remain valid; only swapability
is affected. No action required.

### `TensorButterflyOperator::build(RADON, …)` throws `std::logic_error`

RADON and GREENS_FUNCTION are not implemented (STUB #171). Install a custom
backend via `setRadonTransformFn()` or wait for Phase 3 Q3 2027.

## Installation

Headers are provided by the regular ThemisDB build. Include the module via:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
target_link_libraries(your_target PRIVATE themis_tensor)
```

Include individual headers as needed:

```cpp
#include "tensor/tensor_index_manager.h"    // primary entry point
#include "tensor/hnsw_tt_bridge.h"          // HYBRID mode
#include "tensor/tensor_ingestion_bridge.h" // ingestion pipeline
```

## Usage

See [Quick Start](#quick-start) for common usage patterns.
For the complete public API reference see [`../../include/tensor/README.md`](../../include/tensor/README.md).

## Research References

- Oseledets 2011 — TT-SVD (DOI:10.1137/090752142)
- Holtz et al. 2012 — TT-rounding (DOI:10.1137/100818893)
- Grasedyck 2010 — HT decomposition (SIAM)
- Malkov & Yashunin 2020 — HNSW (DOI:10.1109/TPAMI.2018.2889473)
- Hu et al. 2022 — LoRA (ICLR)
- Michielssen & Boag 1996, Candes et al. 2009 — Butterfly algorithm
- `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`
- `research/TENSOR_NETWORK_DATABASE_ARXIV_DRAFT.md`
- `research/ADALORA_TT_BRIDGE_RESEARCH.md`
