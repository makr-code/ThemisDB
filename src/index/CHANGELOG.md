<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Index Module

All notable changes to the Index module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Multi-GPU distributed vector index improvements (issue #1878)
- Multi-tenancy isolation hardening (issue #1872)
- Extended integration test suite (issue #1883)
- Performance benchmark suite (issue #1884)
- Full security audit (issue #1885)
- API stability guarantees documentation (issue #1887)

## [1.8.0] — 2026-03-24
### Added
- **Matryoshka Representation Learning (MRL) truncation** (Issue: #1876 follow-up):
  `MatryoshkaTruncation` stateless helper for prefix-truncation and optional L2
  normalisation of MRL embeddings; `MatryoshkaTruncatedIndex` IAnnIndex decorator
  that applies truncation transparently to any ANN backend (ScaNN, DiskAnnAdapter,
  HNSW, etc.), enabling multi-stage retrieval pipelines and compact low-dimensional
  pre-filter indexes from a single full-dimensional embedding.
  Files: `include/index/matryoshka_truncation.h`, `src/index/matryoshka_truncation.cpp`.
  Standard granularity constants `kMRL_64/128/256/512/768/1024/1536` for
  OpenAI text-embedding-3, Nomic Embed v1.5, BGE-M3 compatibility.
  Tests: `tests/index/test_matryoshka_truncation.cpp` — 25 focused tests (v1.8.0).
  CI: `.github/workflows/matryoshka-truncation-ci.yml`.

## [1.7.0] — 2026-03-xx
### Added
- **Index Compression** (issue #176): `IndexCompressionCodec` with five techniques:
  delta encoding, prefix compression, Bloom filters, dictionary encoding, and
  run-length encoding.  `SecondaryIndexManager::Config` gains `enable_compression`,
  `compression_algorithm` (NONE/LZ4/ZSTD/SNAPPY), `compression_level`, and
  per-technique enable flags.
  Files: `include/index/index_compression.h`, `src/index/index_compression.cpp`.
- GPU memory oversubscription manager for VRAM-bounded large-scale vector search

## [1.6.0] — 2026-03-xx
### Added
- Multi-GPU distributed vector index with cross-device shard rebalancing
- Tiered index migration: hot (VRAM), warm (DRAM), cold (NVMe) tiers with automatic promotion/demotion
- DiskANN algorithm support for billion-scale approximate nearest-neighbour search
- ScaNN integration for asymmetric distance computation and quantisation
### Changed
- HNSW incremental re-indexing now supports online node deletion without full rebuild
- Adaptive index advisor extended with workload replay for plan validation
### Fixed
- GPU build failure on HIP targets when ROCm < 5.4 (issue #1878, partially resolved)
- R-tree Z-order curve producing incorrect MBR splits for high-dimensional spatial data

## [1.5.0] — 2025-09-01
### Added
- Full-text inverted index with BM25 scoring and positional posting lists
- Graph indexing integration for adjacency structure acceleration
- Adaptive index advisor with workload sampling and index recommendation engine
### Changed
- HNSW ef_construction and M parameters now auto-tuned based on dataset size
- B-tree index now supports composite keys up to 16 columns
### Fixed
- HNSW graph connectivity broken after concurrent inserts at high parallelism

## [1.4.0] — 2025-03-01
### Added
- R-tree spatial index with Z-order (Morton) curve space-filling for range queries
- Binary Quantization (BQ) and Residual Quantization (RQ) for compact vector storage
- Secondary index framework: B-tree and range index for non-vector columns
### Changed
- Product Quantization (PQ) codebook training now parallelised across CPU cores
- HNSW search now supports filtered ANN (pre-filter and post-filter modes)
### Fixed
- PQ encoding producing incorrect sub-vector assignments for high-dimensional inputs

## [1.3.0] — 2024-09-01
### Added
- GPU-accelerated vector search via Vulkan compute shaders (L2, Cosine, Dot product)
- CUDA kernel support for HNSW distance computation on NVIDIA devices
- HIP backend for AMD GPU support
- VRAM secure clear on index eviction to prevent cross-tenant data leakage
### Changed
- IndexManager layer enforces tenant key prefix scoping: `tenant:<id>:<index_name>`
- RocksDB key prefix isolation extended to all secondary index families
### Fixed
- Vulkan descriptor set leak on index rebuild
- CUDA stream synchronisation race during concurrent multi-index queries

## [1.2.0] — 2024-06-01
### Added
- Product Quantization (PQ) for compressed HNSW index storage
- Tenant key prefix scoping in RocksDB (`tenant:<id>:<index_name>` format)
- Index registry isolation at IndexManager layer
### Changed
- HNSW layer graph now stored in RocksDB with bloom-filter-accelerated neighbour lookups
### Fixed
- HNSW ef_search returning fewer results than requested when index is small

## [1.1.0] — 2024-03-01
### Added
- HNSW incremental re-indexing (delta inserts without full rebuild)
- Cosine and Dot product distance metrics in addition to L2
### Changed
- Vector dimension validation enforced at insert time
### Fixed
- L2 distance overflow for high-magnitude float32 vectors

## [1.0.0] — 2024-01-01
### Added
- Initial HNSW vector index implementation with L2 distance
- Basic approximate nearest-neighbour search API
- RocksDB-backed persistent index storage
