<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Graph Module

**Last Audit:** 2026-03-12  
**Auditor:** Copilot  
**Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 6 |
| Test Coverage | ⚠️ >80% target per ROADMAP; GPU paths not yet covered |
| Open TODOs | 1 (ANN/GNN integration) |
| Open Stubs | 0 |
| Security Issues | 0 open — query injection via path constraints fixed (#1832, 2026-02-28) |

## Build System

The graph module is registered in the top-level `CMakeLists.txt` as a static library target (`themis_graph`). All six source files are listed explicitly; no glob-based inclusion. The module links against the AQL engine, RocksDB, and the internal thread-pool library.

## Source Files Audited

| File | Responsibility | Status |
|------|---------------|--------|
| `distributed_graph.cpp` | Cross-shard edge traversal and result merging | ✅ Reviewed |
| `gpu_traversal.cpp` | GPU-accelerated BFS/DFS (Vulkan/CUDA stubs, partially implemented) | ⚠️ Incomplete — issue #1829 |
| `graph_query_optimizer.cpp` | Cost-based query planning, rule rewriting, cardinality estimation | ✅ Reviewed |
| `parallel_traversal.cpp` | Parallel BFS/DFS with work-stealing, timeout enforcement | ✅ Reviewed |
| `path_constraints.cpp` | Path constraint parsing, validation, and filter evaluation | ✅ Reviewed — security fix applied 2026-02-28 |
| `scheduled_edge_refresh.cpp` | Periodic semantic edge scoring, vector similarity, temporal decay | ✅ Reviewed |

## Test Coverage

- Unit tests exist under `tests/graph/` covering: BFS/DFS correctness, VF2 isomorphism, path constraint acceptance/rejection, optimizer plan shapes, scheduled refresh scheduling logic.
- Coverage target: >80% line coverage per ROADMAP.
- **Gap:** `gpu_traversal.cpp` GPU code paths are not covered due to hardware dependency; CPU fallback paths are tested.
- Integration tests cover distributed traversal across 2-shard configurations.

## Findings

### Resolved
- **Race condition in parallel frontier sharing** — Fixed in v1.3.0; thread-local frontier queues prevent concurrent mutation.
- **VF2 duplicate candidate mappings** — Fixed in v1.2.0; symmetry pruning added.
- **Incorrect BFS shortest-path with negative weights** — Fixed in v1.1.0; negative-weight edges now rejected at ingestion.

### Open

#### ⚠️ [#1829] GPU-accelerated BFS/DFS not yet production-ready
- `gpu_traversal.cpp` contains partial Vulkan/CUDA implementation; GPU paths are disabled by feature flag.
- CPU fallback is fully functional and tested.
- **Action:** Complete GPU kernel implementation and add hardware-in-the-loop tests before enabling.

#### ⚠️ [#1830] Unit test coverage gaps
- Several edge-case branches in `graph_query_optimizer.cpp` (multi-hop cost estimation for star topologies) lack explicit test cases.
- **Action:** Add parametrised tests covering star, clique, and chain graph shapes.

#### ✅ [#1832] Security — Query injection via path constraints — RESOLVED
- `isValidIdentifier()` and `isValidFieldName()` added to `path_constraints.cpp` (commit `23f569828d`, 2026-02-28).
- All six input entry points (`addForbiddenNode`, `addRequiredNode`, `addForbiddenEdge`, `addRequiredEdge`, `addEdgePropertyConstraint`, `addNodePropertyConstraint`) now validate inputs.
- `findConstrainedPaths()` validates start/end node IDs and clamps `max_results` to `MAX_RESULTS_LIMIT` (10 000).
- Integer overflow in `validatePath()` (negative `MIN_LENGTH`/`MAX_LENGTH`) fixed.
- 12 security regression tests added to `tests/test_graph_query_optimizer.cpp`.

#### 📝 Open TODO — ANN/GNN integration
- One TODO comment in `scheduled_edge_refresh.cpp` marks the planned integration point for ANN-based graph neural network embeddings.
- No production code impact; tracked as a future enhancement.

## Compliance

| Requirement | Status |
|-------------|--------|
| No raw string interpolation into AQL | ✅ Enforced |
| Query timeout limits present | ✅ Enforced |
| Cross-shard mTLS | ✅ Enforced |
| EXPLAIN endpoint access-controlled | ✅ Enforced |
| Shard topology not exposed to clients | ✅ Enforced |
| GPU VRAM isolation (when enabled) | ⚠️ Pending — GPU feature not yet production-enabled |
