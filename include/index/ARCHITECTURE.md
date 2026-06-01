> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/index/ARCHITECTURE.md -->

# Index Module — Public Header Architecture

**Module Path:** `include/index/`  
**Implementation:** `../../src/index/`  
**Canonical architecture doc:** [`../../src/index/ARCHITECTURE.md`](../../src/index/ARCHITECTURE.md)

---

## 1. Overview

`include/index/` defines the **public vector indexes (HNSW, ANN, GPU), graph indexes, secondary/spatial/temporal indexes, product quantisation, learned indexes, and multi-GPU support API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/index/ARCHITECTURE.md`](../../src/index/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 ANN and Vector Indexes

| Header | Public Type | Purpose |
|--------|------------|---------|
| `ann_index.h` | `ANNIndex` | Primary ANN index entry point |
| `vector_index.h` | `VectorIndex` | Dense vector index |
| `vector_index_manager.h` | `VectorIndexManager` | Multi-index lifecycle manager |
| `advanced_vector_index.h` | `AdvancedVectorIndex` | Extended ANN configuration surface |
| `distributed_vector_index.h` | `DistributedVectorIndex` | Sharded distributed vector index |
| `multi_vector_search.h` | `MultiVectorSearch` | Multi-vector query execution |
| `approximate_radius_search.h` | `ApproximateRadiusSearch` | Radius-bounded ANN search |
### 2.2 HNSW Tuning

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hnsw_layer_optimizer.h` | `HNSWLayerOptimizer` | HNSW layer parameter optimizer |
| `hnsw_parameter_tuner.h` | `HNSWParameterTuner` | Automated HNSW parameter tuning |
| `hnsw_production_defaults.h` | `HNSWProductionDefaults` | Production-safe HNSW default configuration |
| `cuda_hnsw_graph_traversal.h` | `CUDAHNSWGraphTraversal` | CUDA-accelerated HNSW traversal |
### 2.3 GPU Vector Indexes

| Header | Public Type | Purpose |
|--------|------------|---------|
| `gpu_vector_index.h` | `GPUVectorIndex` | GPU-resident vector index |
| `gpu_vector_index_vulkan.h` | `GPUVectorIndexVulkan` | Vulkan-backed GPU vector index |
| `multi_gpu_vector_index.h` | `MultiGPUVectorIndex` | Multi-GPU partitioned vector index |
| `gpu_memory_oversubscription.h` | `GPUMemoryOversubscription` | GPU memory oversubscription manager |
### 2.4 Quantisation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `binary_quantizer.h` | `BinaryQuantizer` | Binary/hamming vector quantisation |
| `product_quantizer.h` | `ProductQuantizer` | Product quantisation (PQ) codec |
| `residual_quantizer.h` | `ResidualQuantizer` | Residual quantisation (RQ) codec |
| `index_compression.h` | `IndexCompression` | Index-level compression utilities |
| `learned_quantizer.h` | `LearnedQuantizer` | Data-driven learned quantisation |
| `matryoshka_truncation.h` | `MatryoshkaTruncation` | Matryoshka representation truncation |
### 2.5 Graph and Secondary Indexes

| Header | Public Type | Purpose |
|--------|------------|---------|
| `graph_index.h` | `GraphIndex` | Property graph index |
| `property_graph.h` | `PropertyGraph` | Property graph container |
| `graph_analytics.h` | `GraphAnalytics` | Graph analytics over index |
| `secondary_index.h` | `SecondaryIndex` | Secondary B-tree/hash index |
| `secondary_index_metadata_cache.h` | `SecondaryIndexMetadataCache` | Cached secondary index metadata |
| `spatial_index.h` | `SpatialIndex` | R-tree spatial index |
| `temporal_graph.h` | `TemporalGraph` | Bi-temporal graph index |
| `adaptive_index.h` | `AdaptiveIndex` | Workload-adaptive index selection |
| `tiered_index_manager.h` | `TieredIndexManager` | Hot/warm/cold tiered index manager |
| `index_manager.h` | `IndexManager` | Unified index lifecycle manager |
| `inverted_index.h` | `InvertedIndex` | Full-text inverted index |
### 2.6 Embeddings and Learned Structures

| Header | Public Type | Purpose |
|--------|------------|---------|
| `gnn_embeddings.h` | `GNNEmbeddings` | GNN-based graph node embeddings |
| `rotary_embeddings.h` | `RotaryEmbeddings` | RoPE rotary position embeddings |
| `rotary_embeddings_gpu.h` | `RotaryEmbeddingsGPU` | GPU-accelerated RoPE embeddings |
| `learnable_rope.h` | `LearnableRoPE` | Learnable RoPE variant |
| `lora_rope.h` | `LoRARoPE` | LoRA-adapted RoPE embeddings |
| `learned_index.h` | `LearnedIndex` | Recursive model index (RMI) |
| `process_graph.h` | `ProcessGraph` | Process-mining graph index |
### 2.7 Buffers and Workload

| Header | Public Type | Purpose |
|--------|------------|---------|
| `vector_auto_buffer.h` | `VectorAutoBuffer` | Auto-resizing vector buffer |
| `graph_auto_buffer.h` | `GraphAutoBuffer` | Auto-resizing graph buffer |
| `workload_replay.h` | `WorkloadReplay` | Index workload replay for tuning |
| `edge_types.h` | `EdgeTypes` | Graph edge type definitions |

---

## 3. Namespace Layout

All public types reside in the `themis::index` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/index/` expose the **stable public API**; internal types live in `src/index/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **ANN Frontdoor**.
