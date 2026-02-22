# Graph Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Core graph query optimization (cost-based algorithm selection, constrained path finding, traversal algorithm selection, adaptive optimization) is functional. Parallel traversal and distributed graph queries are in progress.

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

## In Progress 🚧
- [I] Parallel multi-source BFS/DFS for large graphs (Target: Q2 2026) (Issue: #1808)
- [!] Query plan reuse across structurally similar queries (Target: Q2 2026) (Issue: #2394)
- [I] Cost model calibration from real execution feedback (Target: Q3 2026) (Issue: #2386)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Parallel multi-source traversal for large fan-out queries (Issue: #1811)
- [I] Adaptive plan selection using execution feedback (cost model learning) (Issue: #1812)
- [!] Subgraph isomorphism queries (pattern matching) (Issue: #2390)
- [I] Incremental graph query execution on live updates (Issue: #1825)
- [I] Plan cache eviction with size and TTL controls (Issue: #1827)
- [I] EXPLAIN output in AQL for graph query plans (Issue: #1816)

### Long-term (6-12 months)
- [I] Distributed graph query execution across shards (Issue: #1826)
- [I] Temporal graph query optimization (time-ranged traversals) (Issue: #1828)
- [I] Property graph schema-aware optimizer hints (Issue: #1819)
- [I] GPU-accelerated BFS/DFS for massive graphs (Issue: #1829)
- [I] Integration with analytics module for graph algorithm reuse (Issue: #1821)
- [I] Graph query result streaming for large path sets (Issue: #1822)

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

### Phase 2: Parallel Traversal & Adaptive Planning (Status: In Progress 🚧)
- [I] Parallel multi-source BFS/DFS for large graphs (`graph/parallel_traversal.cpp`, Target: Q2 2026) (Issue: #1833)
- [~] Query plan reuse across structurally similar queries (Target: Q2 2026)
- [ ] Cost model calibration from real execution feedback (Target: Q3 2026)

### Phase 3: Pattern Matching & Distribution (Status: Planned 📋)
- [ ] Subgraph isomorphism queries (pattern matching)
- [ ] Incremental graph query execution on live updates
- [ ] Distributed graph query execution across shards
- [ ] Plan cache eviction with size and TTL controls
- [ ] Temporal graph query optimization (time-ranged traversals)
- [ ] GPU-accelerated BFS/DFS for massive graphs

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1830)
- [x] Integration tests (query optimizer, constrained path finding, AQL integration)
- [I] Performance benchmarks (traversal latency vs graph size) (Issue: #1831)
- [I] Security audit (query injection via path constraints) (Issue: #1832)
- [x] Documentation complete
- [x] API stability guaranteed for graph query optimizer and path finder

## Known Issues & Limitations
- Parallel multi-source BFS/DFS is not yet implemented; large fan-out queries may be slow
- Cost model is static; adaptive learning from execution history is planned but not active
- Subgraph isomorphism (pattern matching) is not yet available
- Distributed graph queries across shards are not yet supported

## Breaking Changes
- Distributed graph query introduces shard-aware plan nodes (new plan format, backward-compatible with single-node)
- Subgraph isomorphism query syntax will extend AQL graph traversal syntax (additive)
