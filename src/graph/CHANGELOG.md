<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Graph Module

All notable changes to the Graph module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- GPU-accelerated BFS/DFS kernels (issue #1829)
- ANN/GNN integration for graph embeddings
- Extended unit test coverage (issue #1830)

## [1.3.0] — 2026-03-xx
### Added
- EXPLAIN HTTP endpoint for graph query plan inspection
- Scheduled semantic edge refresh with vector similarity scoring and temporal decay
- Distributed graph query support across shards with shard-isolated execution
### Changed
- Cost-based query optimizer extended with cardinality estimation for multi-hop paths
- Parallel BFS/DFS refactored to use work-stealing thread pool
### Fixed
- Edge weight overflow in large-graph traversals with high-cardinality nodes
- Race condition in parallel traversal when visiting shared frontier nodes

## [1.2.0] — 2025-09-01
### Added
- Subgraph isomorphism via VF2 algorithm with pruning heuristics
- Path constraint validation for node/edge filter expressions
- Cost-based query optimizer with rule-based rewriting pass
### Changed
- BFS/DFS parallelism level now configurable via `graph.traversal.parallelism`
### Fixed
- VF2 matching producing duplicate candidate mappings on symmetric graphs

## [1.1.0] — 2025-03-01
### Added
- Parallel BFS and DFS traversal using thread-local frontier queues
- Distributed graph queries with cross-shard edge resolution
- Query timeout enforcement to prevent unbounded traversals
### Changed
- Internal adjacency representation migrated from CSR to hybrid CSR/hash-map for dynamic graphs
### Fixed
- Incorrect shortest-path result when negative-weight edges present in BFS mode

## [1.0.0] — 2024-01-01
### Added
- Initial implementation of property graph traversal engine
- Basic BFS/DFS with depth and hop-count limits
- AQL graph pattern matching integration
- Edge and node property filtering
