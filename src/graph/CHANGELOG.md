<!-- Status: current | validated: 2026-04-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Graph Module

All notable changes to the Graph module are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- GPU-accelerated BFS/DFS kernels (issue #1829)

## [1.5.0] — 2026-04-09
### Added
- `include/graph/graph_embedding.h` — `IGraphEmbeddingProvider` abstract interface and concrete `GraphEmbedding` implementation.
  Supports Node2Vec, DeepWalk, GNN_SAGE, GNN_GCN, LINE algorithms. Features: configurable embedding dimensions (2–4096), random-walk generation, skip-gram negative-sampling optimiser, link prediction (cosine-similarity ranked candidates), node classification via user-supplied `NodeClassifierFn` callback. Static helpers: `cosineSimilarity`, `dotProduct`, `negativeEuclidean`. (Issue #1830)
- `src/graph/graph_embedding.cpp` — Full Node2Vec / DeepWalk implementation with biased random walks and skip-gram optimiser.
- `include/graph/graph_query_rewriter.h` — `IGraphQueryRewriter` abstract interface and concrete `GraphQueryRewriter` implementation.
  Implements 6 rewrite rules: `PREDICATE_PUSHDOWN`, `EDGE_TYPE_FILTER_PUSHDOWN`, `COMMON_SUBEXPRESSION_ELIMINATION`, `JOIN_REORDERING`, `QUERY_DECOMPOSITION`, `MATERIALIZED_VIEW_UTILIZATION`. Provides `RewriterStats`, dry-run `explainRewrite()`, and view registration.
- `src/graph/graph_query_rewriter.cpp` — Full rule implementation with deterministic application order and stats tracking.
- `include/graph/multi_layer_graph.h` — `IMultiLayerGraph` abstract interface and concrete `MultiLayerGraph` implementation.
  Supports multiple named edge-type layers (DIRECTED, UNDIRECTED, BIDIRECTIONAL), cross-layer Dijkstra `shortestPath()`, BFS `reachableFrom()`, `isReachable()`, and multi-layer `pageRank()` with AVG/SUM/MAX/MIN/COUNT aggregation.
- `src/graph/multi_layer_graph.cpp` — In-memory implementation backed by adjacency maps.
- `tests/graph/test_graph_future_interfaces.cpp` — 42 focused tests covering all three new interfaces (GraphFutureInterfacesFocusedTests).

### Changed
- `include/graph/ROADMAP.md` updated: Phase 4 GNN/Extended GPU marked complete; v1.5.0 current status with 9 public headers.

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
