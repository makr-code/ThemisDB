<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/index/ -->

# Index Module — Public Header Architecture
**Version:** 1.7.0
**Module Path:** `include/index/`
**Implementation:** `../../src/index/`

---

## Overview

The Index module provides the most extensive public header surface in ThemisDB: vector indexes (HNSW, ANN, ScaNN, DiskANN), full-text inverted index, spatial index, graph indexes (property graph, temporal graph, process graph), learned indexes, tiered index management, GPU/multi-GPU vector indexing, quantisation (binary, product, residual), and rotary embeddings.

## Design Principles

- **ANN Abstraction** — `IAnnIndex` (`ann_index.h`) is the universal ANN interface; ScaNN, DiskANN, GPU, and multi-GPU backends all implement it.
- **Tiered Management** — `TieredIndexManager` (`tiered_index_manager.h`) routes queries across index tiers by cost model.
- **Learned Indexes** — `LearnedIndex` and `LearnedQuantizer` (`learned_index.h`, `learned_quantizer.h`) enable ML-based index structures.
- **GPU-First** — `GPUVectorIndex`, `MultiGPUVectorIndex`, and `CudaHnswTraversalEngine` provide GPU-native index paths.

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `ann_index.h` | `IAnnIndex`, `AnnSearchResult`, `ScaNN`, `DiskAnnAdapter` | Universal ANN interface + ScaNN/DiskANN backends |
| `vector_index.h` | `VectorIndex` | Core dense vector index |
| `vector_index_manager.h` | `VectorIndexManager` | Lifecycle management for vector indexes |
| `advanced_vector_index.h` | `AdvancedVectorIndex` | Extended vector index with hybrid search |
| `gpu_vector_index.h` | `GPUVectorIndex` | Single-GPU vector index |
| `multi_gpu_vector_index.h` | `MultiGPUVectorIndex` | Multi-GPU sharded vector index |
| `distributed_vector_index.h` | `DistributedVectorIndex`, `DistributedVectorIndexConfig`, `DistributedShardStats` | Distributed vector index across nodes |
| `hnsw_layer_optimizer.h` | — | HNSW layer parameter optimisation |
| `hnsw_parameter_tuner.h` | — | Auto-tuning for HNSW ef/M parameters |
| `hnsw_production_defaults.h` | — | Validated production defaults for HNSW |
| `cuda_hnsw_graph_traversal.h` | `CudaHnswTraversalEngine`, `CudaHnswConfig`, `HnswTraversalResult`, `HnswLayerGraph` | CUDA-accelerated HNSW traversal |
| `inverted_index.h` | — | Full-text inverted index |
| `spatial_index.h` | — | Geospatial spatial index |
| `graph_index.h` | `GraphIndexManager` | Property graph index management |
| `property_graph.h` | — | Property graph definition |
| `temporal_graph.h` | — | Bi-temporal graph index |
| `process_graph.h` | — | Process/workflow graph index |
| `index_manager.h` | — | Unified index manager |
| `tiered_index_manager.h` | — | Tiered index routing by cost model |
| `index_compression.h` | — | Index compression codecs (5 techniques, v1.7.0) |
| `adaptive_index.h` | `AdaptiveIndexManager` | Adaptive index selection |
| `secondary_index.h` | `SecondaryIndex` | Secondary index for non-vector attributes |
| `secondary_index_metadata_cache.h` | — | Metadata cache for secondary indexes |
| `learned_index.h` | — | ML-based learned index |
| `learned_quantizer.h` | — | ML-based quantiser |
| `binary_quantizer.h` | `BinaryQuantizer` | Binary quantisation for vector compression |
| `product_quantizer.h` | — | Product quantisation |
| `residual_quantizer.h` | — | Residual quantisation |
| `rotary_embeddings.h` | — | Rotary positional embeddings (RoPE) |
| `rotary_embeddings_gpu.h` | — | GPU RoPE computation |
| `lora_rope.h` | — | LoRA-adapted RoPE |
| `learnable_rope.h` | — | Learnable RoPE for fine-tuning |
| `gnn_embeddings.h` | `GNNEmbeddingManager` | GNN embedding management |
| `approximate_radius_search.h` | `ApproximateRadiusSearch` | Approximate radius (range) search |
| `multi_vector_search.h` | — | Multi-vector hybrid search |
| `graph_analytics.h` | `GraphAnalytics` | Graph analytics over indexed graphs |
| `graph_auto_buffer.h` | `GraphAutoBuffer`, `GraphAutoBufferConfig` | Auto-buffering for graph traversals |
| `vector_auto_buffer.h` | — | Auto-buffering for vector queries |
| `gpu_memory_oversubscription.h` | `GPUMemoryOversubscriptionManager` | GPU memory oversubscription handling |
| `workload_replay.h` | — | Index workload replay for benchmarking |
| `edge_types.h` | `EdgeTypeRegistry`, `EdgeTypeInfo` | Typed edge registry for graph indexes |

## References

- Implementation details: `../../src/index/`
- GPU index guide: `../../src/index/ARCHITECTURE.md`
