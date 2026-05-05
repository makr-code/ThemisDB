# Tensor Module — README

> **Module path:** `src/tensor/`
> **Namespace:** `themis::tensor`
> **Status:** 🟡 Experimental — Phase 1 (Q3 2026)

## Overview

The `tensor` module is a **first-class, standalone index module** parallel to
`src/index/` (HNSW, FAISS, ScaNN, DiskANN).  It provides Tensor-Train (TT)
compressed approximate-nearest-neighbour (ANN) indexing for ThemisDB, with a
clean Separation of Concerns (SOC) boundary:

| Module        | Strength                                      | Use when                             |
|---------------|-----------------------------------------------|--------------------------------------|
| `src/index`   | Sub-ms queries, float32 vectors, n ≥ 1 M      | dim ≤ 4096, κ < 2×, standard ANN    |
| `src/tensor`  | Structured compressibility, zero-copy GGML    | dim > 4096, κ ≥ 2×, LLM / science  |

Boundary analysis: `docs/research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`

## Files

| File | Description |
|------|-------------|
| `include/tensor/tensor_index.h` | `ITensorIndex` interface — all TT-based index backends |
| `include/tensor/tensor_index_manager.h` | Lifecycle manager, routing, GGML bridge entry-point |
| `include/tensor/hnsw_tt_bridge.h` | Hybrid HNSW navigation + TT re-ranking (HYBRID regime) |
| **`include/tensor/tensor_ingestion_bridge.h`** | **`TensorIngestionBridge`** — `ITensorDecompositionBackend` for the ingestion pipeline |
| **`include/tensor/tensor_core_bridge.h`** | **`TensorCoreStorageBridge`** — `ITensorCoreBridge` persisting TT-cores to `ITensorStorageBackend` |
| `src/tensor/tensor_index.cpp` | `FlatTensorIndex` (Phase-1 linear scan reference impl) |
| `src/tensor/tensor_index_manager.cpp` | Manager implementation |
| `src/tensor/hnsw_tt_bridge.cpp` | HNSW+TT bridge implementation |
| **`src/tensor/tensor_ingestion_bridge.cpp`** | `TensorIngestionBridge` implementation |
| **`src/tensor/tensor_core_bridge.cpp`** | `TensorCoreStorageBridge` implementation |

## Dependencies

- `src/storage/tensor_train_decomposer` — TT-SVD core
- `src/storage/tensor_network_storage_engine` — RocksDB persistence (Phase 2); `ITensorStorageBackend` interface
- `src/storage/tensor_router` — routing decisions
- `src/index` — HNSW navigation layer for HYBRID mode (Phase 2)
- `src/utils/logger` — THEMIS_WARN macros
- `include/ingestion/inference_backend.h` — `ITensorDecompositionBackend` interface (ingestion SoC boundary)
- `include/ingestion/ingestion_sinks.h` — `ITensorCoreBridge` interface (ingestion SoC boundary)

## Quick Start

```cpp
// Create manager
auto mgr = themis::tensor::TensorIndexManager::create(db);

// Check routing decision
auto route = mgr->routeFor("tenant1", "llm_weights", "attention_k", 4096, 1e6);
// route == TENSOR_TRAIN (κ ≈ 4.5 for dim=4096)

// Create TT-index and insert flat vector
auto* idx = mgr->createIndex("tenant1", "llm_weights", "attention_k");
idx->addFlat(42, attention_vec.data(), 4096);

// Search without decompression (TT-domain cosine)
auto results = idx->searchFlat(query.data(), 4096, 10);
```

## Stubs

| ID      | Location                               | Status        |
|---------|----------------------------------------|---------------|
| TTI-01  | `FlatTensorIndex::save()`              | Phase 2 Q4 2026 |
| TTI-02  | `FlatTensorIndex::load()`              | Phase 2 Q4 2026 |
| TIM-01  | `TensorIndexManager::ggmlCorePtrs()`   | Phase 3 Q1 2027 |
| TIM-02  | `dropTenantIndexes()` RocksDB delete   | Phase 2 Q4 2026 |
| HTB-01  | HNSW layer (linear scan fallback)      | Phase 2 Q4 2026 |
| HTB-02  | `HnswTTBridge::save()`                 | Phase 2 Q4 2026 |
| HTB-03  | `HnswTTBridge::load()`                 | Phase 2 Q4 2026 |

## Research References

- Oseledets 2011 — TT-SVD (DOI:10.1137/090752142)
- Holtz et al. 2012 — TT-rounding (DOI:10.1137/100818893)
- Malkov & Yashunin 2020 — HNSW (DOI:10.1109/TPAMI.2018.2889473)
- `docs/research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`
- `docs/research/TENSOR_NETWORK_DATABASE_ARXIV_DRAFT.md`
