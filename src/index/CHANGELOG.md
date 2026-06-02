> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Index Module

All notable changes to the index module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified index benchmark symbols from vector, GPU-vector, rebuild, spatial, quantization, and radius-search benchmark suites.
- Process-graph multi-model query and critical-path traversal now reduce avoidable allocations/lookups (hashed edge-type filtering, DFS stack/path pre-reserve, and single-lookup duration/adjacency access).
- Process-graph JSON parsing now routes through a shared typed parser helper in query/join/aggregate/geo/multi-model flows, replacing silent catch-all suppression with contextual debug diagnostics while preserving empty-object fallback behavior.
- Graph-index edge encryption parsing, edge weight/type decode paths, and temporal field parsing now emit contextual `THEMIS_DEBUG` diagnostics on parse/decode failures instead of silently swallowing exceptions while preserving existing fallback behavior.
- Tiered index migration results now expose diagnostic codes plus source/target path context, and successful migrations refresh lifecycle metadata (`last_access`, `access_count`) to avoid stale follow-up state after tier changes.

## [1.8.0] - 2026-03-24

### Added
- Matryoshka truncation support and focused benchmark/test coverage for advanced vector retrieval paths.

## [1.7.0] - 2026-03-01

### Added
- index compression surfaces and GPU memory oversubscription support.

## [1.6.0] - 2026-02-01

### Added
- multi-GPU/distributed vector and tiered migration capabilities.

### Changed
- incremental reindexing and advisor behavior refinement.

### Fixed
- backend-specific stability issues in mixed hardware paths.

## [1.5.0] - 2025-09-01

### Added
- full-text indexing and adaptive advisor support.

### Changed
- HNSW and composite index tuning behavior.

### Fixed
- high-concurrency vector connectivity/correctness issues.

## [1.4.0] - 2025-03-01

### Added
- spatial indexing and additional quantization pathways.

### Changed
- HNSW filtered-search and PQ behavior refinement.

### Fixed
- quantization assignment correctness issues.

## [1.3.0] - 2024-09-01

### Added
- GPU acceleration surfaces for vector operations.

### Changed
- tenant/key isolation and persistence behavior hardening.

### Fixed
- backend resource lifecycle and synchronization issues.

## [1.0.0] - 2024-01-01

### Added
- foundational HNSW vector indexing and persistent lookup support.