<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Index Module (Public Headers)

All notable changes to the Index module public headers are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/index/CHANGELOG.md`.

## [Unreleased]
- Multi-GPU distributed vector index improvements (Issue #1878)
- Multi-tenancy isolation hardening (Issue #1872)
- Integration test suite (Issue #1883)
- Performance benchmark suite (Issue #1884)

## [1.7.0] — 2026-03-xx
### Added
- `index_compression.h`: `IndexCompressionCodec` with 5 compression techniques (Issue #176)
- `learnable_rope.h`: Learnable RoPE for fine-tuning use cases
- `lora_rope.h`: LoRA-adapted rotary positional embeddings

## [1.6.0] — 2026-02-01
### Added
- `multi_gpu_vector_index.h`: `MultiGPUVectorIndex` sharded across multiple GPUs
- `distributed_vector_index.h`: `DistributedVectorIndex` across cluster nodes
- `gpu_memory_oversubscription.h`: `GPUMemoryOversubscriptionManager`
- `gnn_embeddings.h`: `GNNEmbeddingManager`
- `workload_replay.h`: Index workload replay for benchmarking

## [1.5.0] — 2026-01-15
### Added
- `cuda_hnsw_graph_traversal.h`: CUDA-accelerated HNSW traversal
- `learned_index.h`, `learned_quantizer.h`: ML-based index and quantiser
- `tiered_index_manager.h`: Tiered index routing by cost model
- `approximate_radius_search.h`: Approximate radius search

## [1.0.0] — 2024-01-01
### Added
- `ann_index.h`, `vector_index.h`, `inverted_index.h`, `spatial_index.h`
- `graph_index.h`, `property_graph.h`, `temporal_graph.h`, `process_graph.h`
- `binary_quantizer.h`, `product_quantizer.h`, `residual_quantizer.h`
- `rotary_embeddings.h`, `rotary_embeddings_gpu.h`
- `hnsw_layer_optimizer.h`, `hnsw_parameter_tuner.h`, `hnsw_production_defaults.h`
- `secondary_index.h`, `adaptive_index.h`, `multi_vector_search.h`
