<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Graph Module (Public Headers)

All notable changes to the Graph module public headers are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/graph/CHANGELOG.md`.

## [Unreleased]
- GPU-accelerated BFS/DFS kernel headers (Issue #1829)
- ANN/GNN graph embedding interface headers (Issue #1830)

## [1.4.0] — 2026-03-22
### Added
- `scheduled_edge_refresh.h`: `ScheduledGraphEdgeRefreshEngine` with `setANNIndex()` and `setCEPEventCallback()` (v1.4.0 API extensions)

## [1.3.0] — 2026-03-xx
### Added
- `scheduled_edge_refresh.h`: `RefreshPolicy`, `EdgeScore`, `RefreshStats`, `RefreshAuditEntry`
- `distributed_graph.h`: `DistributedGraphManager`, `ShardGraphExecutor`, `LocalShardGraphExecutor`
### Changed
- `graph_query_optimizer.h`: cardinality estimation for multi-hop path planning
- `parallel_traversal.h`: work-stealing thread pool refactor

## [1.2.0] — 2025-09-01
### Added
- `path_constraints.h`: `PathConstraints` for node/edge filter expressions

## [1.0.0] — 2024-01-01
### Added
- `gpu_traversal.h`: `GPUGraphTraversal` CUDA BFS/DFS
- `graph_query_optimizer.h`: `GraphQueryOptimizer`
- `parallel_traversal.h`: `ParallelTraversal` BFS/DFS
