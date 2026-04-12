# ThemisDB Changelog

All notable changes to ThemisDB are documented in this file.

## [2.0.0] - Q4 2026 (Milestone-Stand 11.04.2026)

### Added - Module Ports & Interfaces

- **CDC Interface Set**: `ICDCReplayController`, `ICDCFilterPipeline` und `ICDCBatchCommitCoordinator` ueber PR #4477.
- **Query v2.0.0 Port**: Port des Query-Scopes fuer Issue #3528 ueber Ersatz-PR #4569; ersetzt die konfliktbehaftete Alt-PR #4507.
- **Storage v2.0.0 Port**: Port des Storage-Scope fuer Issue #3536 ueber Ersatz-PR #4570; ersetzt die konfliktbehaftete Alt-PR #4515.

### Notes

- Zugeordnete Sub-Issues: #3508, #3528, #3536.
- Superseded-Mapping: #4507 -> #4569, #4515 -> #4570.

## [1.10.0] - Q3 2026 (Milestone-Stand 11.04.2026)

### Added - Server Security

- **MQTT Client TLS Support**: TLS-aktivierbarer MQTT-Client-Pfad im Server-Modul ueber PR #4512.

## [1.9.1] - Q3 2026 (Milestone-Stand 11.04.2026)

### Added - Build & Tests

- **Auth Focused Test Targets**: fehlende Testtargets in den Build integriert ueber PR #4474.

## [1.9.0] - Q3 2026 (Milestone-Stand 11.04.2026)

### Added - Compliance & Adapter Layer

- **Chimera Adapter Interfaces**: Streaming Result Sets, Prepared Statements und Connection-Pool-Adapter-Schnittstellen ueber PR #4478.
- **Governance Compliance Evaluators**: ISO 27001- und HIPAA-Regelwerke ueber PR #4484.

## [1.2.0] - Q2 2026 (In Progress)

### Added - Enterprise Features

#### AI/ML Features
- **FAISS Advanced (IVF+PQ)**: Production-scale vector search with 10-100x memory reduction
  - Multiple index types: IVF_PQ, IVF_FLAT, HNSW_FLAT
  - GPU acceleration support
  - 2-10x search speedup on large datasets (> 1M vectors)
  - Save/load functionality for persistent indexes
- **Embedding Cache**: Semantic caching for 70-90% API cost reduction
  - Fuzzy matching via vector similarity (configurable threshold)
  - TTL-based expiration (default 1 hour)
  - 100-1000x faster than API calls on cache hit
  - Cost savings tracking
- **Hybrid Search**: BM25 + Vector search with Reciprocal Rank Fusion (RRF)
  - 70-90% better recall than single-method search
  - Configurable BM25/Vector weights
  - Optimized for RAG workflows

#### IoT/Timescale Features
- **Hypertables**: TimescaleDB-compatible time-series storage
  - Automatic time-based partitioning (1 chunk = 1 RocksDB CF)
  - TTL-based retention (uses v1.1.0 TTL feature)
  - ZSTD compression for old chunks (> 7 days)
  - 5x storage reduction
- **Time-Series Aggregates**: SIMD-accelerated analytics
  - 5-10x faster aggregations (AVX2/AVX512 vectorization)
  - 12 aggregate functions: SUM, AVG, MIN, MAX, COUNT, STDDEV, VARIANCE, FIRST, LAST, P50, P95, P99
  - Resample, rolling window, and time bucketing operations
  - Zero-copy processing for efficiency

### Performance
- FAISS IVF+PQ: 10-100x memory reduction, 2-10x search speedup
- Embedding Cache: 70-90% API cost savings, 100-1000x latency improvement
- Time-Series Aggregates: 5-10x faster via SIMD
- Hybrid Search: 70-90% better recall
- Hypertables: 5x storage reduction

### Dependencies
- No new dependencies (uses existing RocksDB, TBB, Arrow, FAISS)

## [1.1.0] - Q1 2026

### Added - Optimization Release

#### RocksDB Advanced Features
- **TTL Support**: Auto-expiring data via `DBWithTTL::Open()` for time-series compliance
- **Incremental Backups**: 80-90% storage reduction vs full backups
  - `BackupEngine` with `share_table_files=true`
  - `createIncrementalBackup()`, `restoreFromBackup()`, `getBackupCount()` methods
- **Statistics Export**: JSON bridge to OpenTelemetry
  - `exportStatisticsJSON()` and `getStatistic()` methods
  - 13 key metrics: BYTES_WRITTEN, BYTES_READ, BLOCK_CACHE_HIT, etc.

#### TBB Parallelization
- **Parallel Sort**: Replaced 23 `std::sort` calls with `tbb::parallel_sort`
  - 2-4x speedup for large result sets
  - Applied to fulltext, spatial, vector, and hybrid search results
- **Concurrent Hash Maps**: Lock-free caches with 2-3x throughput
  - Converted `PromptManager` from `std::unordered_map + std::mutex` to `tbb::concurrent_hash_map`
  - Lock-free reads for high concurrency

#### Arrow Parquet Export
- **Data Lake Integration**: `exportToParquet()` with type inference
  - Support for compression codecs: snappy, gzip, zstd
  - 90% storage reduction vs RocksDB for cold data archival
  - `exportCollectionToParquet()` for full collection exports

#### vLLM Co-Location
- **CUDA Streams**: Low-priority `cudaStreamNonBlocking` for GPU sharing
  - Priority range detection and configuration
  - vLLM-aware CUDA backend
- **Resource Manager**: `VLLMResourceManager` with NVML monitoring
  - Adaptive GPU usage (< 80% threshold with `canUseGPU()`)
  - CPU/RAM allocation recommendations (`getRecommendedThreadCount()`)
  - 50 cores/200GB RAM for ThemisDB, 14 cores/56GB RAM for vLLM
- **Docker Compose**: Complete stack for ThemisDB + vLLM deployment
  - `docker-compose-vllm.yml` with resource limits
  - 4x NVIDIA A100 GPUs (shared GPU 0, rest for vLLM)

#### Build System
- **mimalloc Integration**: Drop-in allocator via `mimalloc-override.h`
  - 20-40% memory throughput boost
  - Zero code changes (automatic override)
- **Build Variants**: 4 optimized configurations
  - Standard (OLTP): 16 dependencies
  - OLAP: 17 dependencies (+ DuckDB)
  - Embedded: 12 dependencies (lightweight)
  - vLLM Co-Location: 16 dependencies + CUDA (AI/ML workloads)

### Performance
- RocksDB: 80-90% backup reduction, auto-cleanup, real-time stats
- TBB: 2-4x sort speedup, 2-3x cache throughput
- Arrow: 90% storage reduction (Parquet vs RocksDB)
- vLLM: 15-27% RAG latency reduction
- mimalloc: 20-40% memory throughput boost

### Dependencies
- Added: mimalloc v2.x
- Total: 16 dependencies (from 15)
- Overhead: +6%

### Breaking Changes
None - Fully backward compatible with v1.0.x

## [1.0.1] - Previous Release

See previous changelog for v1.0.1 and earlier releases.

---

## Version Scheme

ThemisDB follows semantic versioning: MAJOR.MINOR.PATCH

- **MAJOR**: Incompatible API changes
- **MINOR**: New functionality (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

## Links

- [v2.0.0 Release Notes](RELEASE_NOTES_v2.0.0.md)
- [v1.9.0 Release Notes](RELEASE_NOTES_v1.9.0.md)
- [v1.2.0 Release Notes](v1.2.0.md)
- [v1.1.0 Release Notes](v1.1.0.md)
- [GitHub Repository](https://github.com/makr-code/ThemisDB)
- [Documentation](https://makr-code.github.io/ThemisDB/)
