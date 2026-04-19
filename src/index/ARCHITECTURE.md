> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Index Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/index/`

---

## 1. Overview

The Index module provides ThemisDB's complete indexing infrastructure across all five data
models: vector (HNSW, IVF+PQ, FAISS), relational (B-tree, composite, sparse), graph
(adjacency lists, property graphs, temporal graphs), spatial (R-tree, Z-order), and
document (inverted index, full-text).

It is the single source of truth for index structures; all query execution, storage, and
graph traversal modules read from and write to index structures managed here.

---

## 2. Design Principles

- **Unified IndexManager** – a single `IndexManager` provides access to all specialized
  sub-managers (vector, secondary, graph, spatial), breaking circular dependencies via
  dependency injection.
- **Adaptive Indexing** – `adaptive_index.cpp` monitors query patterns and recommends
  or automatically creates indexes as workloads evolve.
- **GPU Acceleration** – vector indexes have GPU-accelerated search paths (CUDA, Vulkan,
  HIP) via `gpu_vector_index.cpp` and `multi_gpu_vector_index.cpp`.
- **Quantization** – multiple quantization schemes (Product, Binary, Residual, Learned)
  reduce VRAM and memory footprint for large vector collections.
- **Workload Auto-Tuning** – `hnsw_parameter_tuner.cpp` selects HNSW construction
  parameters based on workload classification.
- **RocksDB Persistence** – all index updates use atomic WriteBatch for ACID durability.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `index_manager.cpp` | Unified façade: access all specialized index managers |
| `vector_index.cpp` | VectorIndexManager: HNSW + GPU, multi-metric, quantization |
| `hnsw_production_defaults.cpp` | Optimized HNSW configs per use case (RAG, search, etc.) |
| `hnsw_parameter_tuner.cpp` | Workload-driven parameter selection |
| `hnsw_layer_optimizer.cpp` | HNSW layer structure optimization |
| `advanced_vector_index.cpp` | IVF+PQ, FAISS integration |
| `gpu_vector_index.cpp` | CUDA GPU-accelerated HNSW |
| `gpu_vector_index_vulkan.cpp` | Vulkan GPU-accelerated HNSW |
| `multi_gpu_vector_index.cpp` | Multi-GPU sharded vector index |
| `approximate_radius_search.cpp` | Approximate k-NN with radius constraint |
| `multi_vector_search.cpp` | Multi-vector (ColBERT-style) search |
| `product_quantizer.cpp` | Product Quantization (PQ) |
| `binary_quantizer.cpp` | Binary Quantization |
| `residual_quantizer.cpp` | Residual Quantization |
| `learned_quantizer.cpp` | Neural learned quantization |
| `secondary_index.cpp` | B-tree, range, sparse, composite indexes |
| `inverted_index.cpp` | Inverted index for full-text / token search |
| `graph_index.cpp` | Graph adjacency list index |
| `property_graph.cpp` | Property graph (vertices + edges with properties) |
| `temporal_graph.cpp` | Temporal graph (versioned edges with timestamps) |
| `graph_analytics.cpp` | PageRank, centrality, community detection |
| `graph_auto_buffer.cpp` | Auto-buffered graph writes |
| `spatial_index.cpp` | R-tree + Z-order spatial index |
| `adaptive_index.cpp` | Query pattern monitoring + auto-index recommendation |
| `workload_replay.cpp` | Replay recorded workloads for index tuning |
| `gnn_embeddings.cpp` | GNN-computed graph node embeddings |
| `rotary_embeddings.cpp` / `.cu` / `_gpu_cpu.cpp` / `_hip.cpp` | RoPE embeddings (CPU/CUDA/HIP) |
| `learnable_rope.cpp` / `lora_rope.cpp` | Learnable RoPE with LoRA adaptation |
| `edge_types.cpp` | Edge type registry and schema |
| `process_graph.cpp` | Process mining graph representation |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              Query Engine / Storage / Graph Module              │
│   index_manager->getVectorIndexManager()->search(...)           │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     IndexManager (façade)                        │
│  getVectorIndexManager()  → VectorIndexManager                  │
│  getSecondaryIndexManager() → SecondaryIndexManager             │
│  getGraphIndexManager()   → GraphIndexManager                   │
│  getSpatialIndexManager() → SpatialIndexManager                 │
└──────────┬──────────────┬──────────────┬──────────────┬─────────┘
           │              │              │              │
┌──────────▼──┐  ┌────────▼──────┐  ┌───▼────────┐  ┌─▼──────────┐
│   Vector    │  │  Secondary    │  │   Graph    │  │  Spatial   │
│   Index     │  │   Index       │  │   Index    │  │   Index    │
│  (HNSW+GPU) │  │ (B-tree etc.) │  │ (adj list) │  │ (R-tree)   │
└──────────┬──┘  └───────────────┘  └────────────┘  └────────────┘
           │
    ┌──────┴────────────────┐
    │   GPU Acceleration     │
    │  CUDA / Vulkan / HIP   │
    └───────────────────────┘
```

---

## 4. Data Flow

### 4.1 Vector Search (HNSW + GPU)

```
VectorIndexManager::search("embeddings", query_vec, k=10)
    │
    ├─ GPU available? → GPUVectorIndex::search() → CUDA/Vulkan kernel
    │       GPU search: O(log N) HNSW traversal on device
    └─ CPU fallback → HNSW::search() on host
    │
    ▼
Apply quantization reconstruction if PQ/BQ/RQ compressed
    │
    ▼
Return []{pk, distance} sorted by distance (top-k)
```

### 4.2 Vector Insert (HNSW + RocksDB)

```
VectorIndexManager::addVector("embeddings", "doc123", embedding)
    │
    ├─ HNSW::insert(embedding) → update graph layers
    ├─ quantize → store compressed vector in RocksDB
    └─ audit log entry
```

### 4.3 Adaptive Index Recommendation

```
AdaptiveIndex: monitor query patterns
    │
    ├─ detect frequently filtered field → recommend B-tree index
    ├─ detect range queries on timestamp → recommend sorted index
    ├─ detect vector search without index → recommend HNSW
    └─ above threshold → auto-create or alert operator
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Called by** | `src/query/` | Index lookup during query execution |
| **Called by** | `src/graph/` | GraphIndexManager for traversal |
| **Called by** | `src/storage/` | Index maintenance on write |
| **Uses** | `src/gpu/` | VRAM management for GPU vector indexes |
| **Uses** | `src/acceleration/` | GPU compute backends for ANN |
| **Uses** | `src/storage/` | RocksDB WriteBatch for persistence |

---

## 6. Threading & Concurrency Model

- `VectorIndexManager` HNSW search is read-safe for concurrent queries.
- HNSW insert uses a per-layer lock; concurrent inserts serialize per layer.
- `SecondaryIndex` B-tree uses RCU for lock-free reads, copy-on-write for writes.
- `GraphIndexManager` uses a read-write lock on the adjacency list.
- GPU operations are queued to device streams and executed asynchronously.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| HNSW | O(log N) approximate search; ef_search controls speed/recall tradeoff |
| IVF+PQ | Inverted file + Product Quantization: 100× compression, fast coarse search |
| Multi-GPU sharding | `multi_gpu_vector_index.cpp`: horizontal scaling across devices |
| RoPE embeddings | GPU-accelerated rotary position encoding (CUDA/HIP) |
| Adaptive parameters | `hnsw_parameter_tuner.cpp` auto-selects M, ef_construction per workload |

---

## 8. Security Considerations

- Vector operations include audit logging for compliance tracking.
- Embedding data is not logged; only operation metadata (collection, pk, timestamp).
- GPU memory access is scoped per tenant via `src/gpu/GPUMemoryManager`.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `index.hnsw.m` | 16 | HNSW connections per node |
| `index.hnsw.ef_construction` | 200 | HNSW construction beam width |
| `index.hnsw.ef_search` | 100 | HNSW search beam width |
| `index.gpu.enabled` | auto | Use GPU for vector search |
| `index.quantization.type` | none | Quantization: none/pq/bq/rq |
| `index.adaptive.auto_create` | false | Auto-create recommended indexes |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| GPU OOM during search | Fall back to CPU HNSW |
| HNSW graph corruption | Rebuild index from persisted vectors |
| Index out of sync with storage | Trigger background reconciliation |
| Dimension mismatch on insert | Reject with structured error |

---

## 11. Known Limitations & Future Work

- Full-text inverted index is in early development; BM25 ranking is partial.
- DiskANN (disk-based ANN for billion-scale) is planned.
- Multi-vector search (ColBERT) is experimental.
- GNN embeddings require external GNN training; not computed in-process.

---

## 12. References

- `src/index/README.md` — module overview
- `src/index/VECTOR_ADVANCED_FEATURES_README.md` — advanced vector features
- `docs/index_roadmap.md` — roadmap
- `docs/architecture/VECTOR_INDEXING_ARCHITECTURE.md` — vector indexing architecture
- `ARCHITECTURE.md` (root) — full system architecture
