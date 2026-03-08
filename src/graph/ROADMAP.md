# Graph Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Core graph query optimization (cost-based algorithm selection, constrained path finding, traversal algorithm selection, adaptive optimization, parallel traversal, structural plan reuse) is functional. Distributed graph query execution across shards is now implemented.

## Completed ✅
- [x] Graph query optimizer with cost-based algorithm selection
- [x] Constrained path finding (min/max length, required/forbidden nodes and edges)
- [x] Traversal algorithm selection: BFS, DFS, Dijkstra, A*, Bidirectional
- [x] Query plan generation with cost estimates
- [x] Query plan explanation and alternative strategy reporting
- [x] Execution statistics tracking for adaptive optimization
- [x] Query plan caching
- [x] Path validation and constraint checking
- [x] Integration with GraphIndexManager for graph operations
- [x] Integration with AQL for graph query execution
- [x] Query plan reuse across structurally similar queries
- [x] Parallel multi-source BFS/DFS for large graphs (Issue: #1808)
- [x] Adaptive cost model: EMA-based per-algorithm learning, enabled by default
- [x] Adaptive plan selection using execution feedback (cost model learning) (Issue: #1812)
- [x] Cost model calibration from real execution feedback (Issue: #2386)
- [x] Property graph schema-aware optimizer hints (Issue: #1819)
- [x] Distributed graph query execution across shards (Issue: #1826)
- [x] Incremental graph query execution on live updates (Issue: #1825)
- [x] Plan cache eviction with size and TTL controls (Issue: #1827)
- [x] Graph query result streaming for large path sets (Issue: #1822)
- [x] Integration with analytics module for graph algorithm reuse (Issue: #1821)
- [x] Parallel multi-source traversal for large fan-out queries — fan_out_threshold + intra-frontier parallelism (Issue: #1811)
- [x] Subgraph isomorphism queries (pattern matching) (Issue: #2390)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] EXPLAIN output in AQL for graph query plans (Issue: #1816)

### Long-term (6-12 months)
- [I] GPU-accelerated BFS/DFS for massive graphs (Issue: #1829)

## Implementation Phases

### Phase 1: Graph Query Optimizer Core (Status: Completed ✅)
- [x] Graph query optimizer with cost-based algorithm selection (`graph/query_optimizer.cpp`)
- [x] Constrained path finding (min/max length, required/forbidden nodes and edges)
- [x] Traversal algorithm selection: BFS, DFS, Dijkstra, A*, Bidirectional
- [x] Query plan generation with cost estimates and explanation output
- [x] Execution statistics tracking for adaptive optimization
- [x] Query plan caching (`graph/plan_cache.cpp`)
- [x] Path validation and constraint checking
- [x] Integration with GraphIndexManager for graph operations
- [x] Integration with AQL for graph query execution

### Phase 2: Parallel Traversal & Adaptive Planning (Status: Completed ✅)
- [x] Parallel multi-source BFS/DFS for large graphs (`graph/parallel_traversal.cpp`, Target: Q2 2026) (Issue: #1833)
- [x] Query plan reuse across structurally similar queries (Target: Q2 2026)
- [x] Adaptive cost model: EMA per algorithm, confidence-weighted blending into cost estimates
- [x] Advanced cost model calibration from real execution feedback (Target: Q3 2026)

### Phase 3: Pattern Matching & Distribution (Status: In Progress 🚧)
- [x] Subgraph isomorphism queries (pattern matching)
- [x] Distributed graph query execution across shards
- [x] Plan cache eviction with size and TTL controls
- [x] Temporal graph query optimization (time-ranged traversals)
- [~] GPU-accelerated BFS/DFS for massive graphs (`graph/gpu_traversal.cpp`, CPU fallback active; real CUDA kernels planned for THEMIS_ENABLE_CUDA)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1830)
- [x] Integration tests (query optimizer, constrained path finding, AQL integration)
- [x] Performance benchmarks (traversal latency vs graph size) (Issue: #1831)
- [I] Security audit (query injection via path constraints) (Issue: #1832)
- [x] Documentation complete
- [x] API stability guaranteed for graph query optimizer and path finder

## Known Issues & Limitations
- Adaptive plan selection using execution feedback is now active; `selectAlgorithm` uses learned EMA costs when confidence > 0, falling back to static depth heuristics otherwise
- Advanced cost model calibration from real execution feedback is implemented: `calibrateFromHistory()` re-seeds EMA models from batch history and computes cost accuracy metrics (`mean_estimated_ms`, `mean_absolute_error_ms`, `cost_ratio`) when `ExecutionStats::estimated_cost_ms` is populated (automatic in all execute* methods)
- Incremental query execution is BFS-only; DFS/Dijkstra/A* incremental modes are planned
- Incremental query execution is not thread-safe (same as the optimizer itself)
- Incremental query HTTP API (`POST /graph/query/incremental`, `DELETE /graph/query/incremental/:handle`, `POST /graph/changes`) is exposed via `GraphApiHandler`; edge mutations via `POST /graph/edge` and `DELETE /graph/edge/:id` automatically notify registered queries on success
- Subgraph isomorphism (pattern matching) is implemented via `executeSubgraphIsomorphism` (VF2-style backtracking)
- Cross-shard edge following (edges whose endpoints reside on different shards) requires caller-side coordination; the current distributed query model executes intra-shard queries in parallel and returns the globally cheapest result

## Breaking Changes
- Distributed graph query introduces shard-aware plan nodes (new plan format, backward-compatible with single-node)
- Subgraph isomorphism query syntax will extend AQL graph traversal syntax (additive)
