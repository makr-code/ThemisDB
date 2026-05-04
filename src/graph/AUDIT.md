> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: HIGH FINDINGS | validated: 2026-04-21 (source code analysis of graph_query_optimizer.cpp) -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Graph Module

**Last Audit:** 2026-04-21
**Auditor:** Copilot
**Status:** ⚠️ High — 2×S1 (bidirectional BFS no timeout + VF2 exponential without bounds)

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 8 |
| Test Coverage | ⚠️ >80% target per ROADMAP; GPU paths not yet covered |
| S0 Critical | ✅ None from graph_query_optimizer.cpp |
| S1 High | ✅ 0 (GQ-1 + GQ-2 fixed 2026-05-04) |
| Traversal timeout enforcement | ✅ Bidirectional BFS and VF2 both have per-iteration timeout (fixed 2026-05-04) |

## Build System

The graph module is registered in the top-level `CMakeLists.txt` as a static library target (`themis_graph`). All six source files are listed explicitly; no glob-based inclusion. The module links against the AQL engine, RocksDB, and the internal thread-pool library.

## Source Files Audited

| File | Responsibility | Status |
|------|---------------|--------|
| `distributed_graph.cpp` | Cross-shard edge traversal and result merging | ✅ Reviewed |
| `gpu_traversal.cpp` | GPU-accelerated BFS/DFS (Vulkan/CUDA stubs, partially implemented) | ⚠️ Incomplete — issue #1829 |
| `graph_query_optimizer.cpp` | Cost-based query planning, rule rewriting, cardinality estimation | ✅ Reviewed |
| `graph_query_rewriter.cpp` | Query rewriting and optimisation for graph traversals | ✅ Reviewed |
| `graph_watermark.cpp` | Graph watermarking for provenance and data lineage tracking | ✅ Reviewed |
| `parallel_traversal.cpp` | Parallel BFS/DFS with work-stealing, timeout enforcement | ✅ Reviewed |
| `path_constraints.cpp` | Path constraint parsing, validation, and filter evaluation | ✅ Reviewed — security fix applied 2026-02-28 |
| `scheduled_edge_refresh.cpp` | Periodic semantic edge scoring, vector similarity, temporal decay | ✅ Reviewed |

## Test Coverage

- Unit tests exist under `tests/graph/` covering: BFS/DFS correctness, VF2 isomorphism, path constraint acceptance/rejection, optimizer plan shapes, scheduled refresh scheduling logic.
- Coverage target: >80% line coverage per ROADMAP.
- **Gap:** `gpu_traversal.cpp` GPU code paths are not covered due to hardware dependency; CPU fallback paths are tested.
- Integration tests cover distributed traversal across 2-shard configurations.

## Findings

### S1 — High (all resolved 2026-05-04)

#### ~~GQ-1 · `graph_query_optimizer.cpp` · `executeBidirectional()` — No timeout check inside BFS loop~~

**Fixed 2026-05-04:** Timeout check added at the top of the `while` loop in
`executeBidirectional()`, consistent with `executeBFS()` and `executeDFS()`.
Returns `ERR_QUERY_TIMEOUT` when `constraints.timeout_ms > 0` and the elapsed
time exceeds the limit.

---

#### ~~GQ-2 · `graph_query_optimizer.cpp` · `executeSubgraphIsomorphism()` — VF2 exponential without hard resource limit~~

**Fixed 2026-05-04:**
1. Pattern size is validated against `kMaxPatternVertices = 10` at the entry point;
   larger patterns are rejected with `ERR_INVALID_ARGUMENT`.
2. When the caller passes `timeout_ms == 0` (default), `effective_constraints.timeout_ms`
   is set to 30000 ms (30-second hard cap) so the `timedOut()` helper is always active.

---

### Resolved (from 2026-03-12 audit)
- **Race condition in parallel frontier sharing** — Fixed in v1.3.0; thread-local frontier queues.
- **VF2 duplicate candidate mappings** — Fixed in v1.2.0; symmetry pruning added.
- **Incorrect BFS shortest-path with negative weights** — Fixed in v1.1.0; negative-weight edges rejected at ingestion.

### Open (carried forward)
- **[#1829]** GPU-accelerated BFS/DFS not yet production-ready; GPU paths disabled by feature flag.
- **[#1830]** Unit test coverage gaps for star topology cost estimation.
- **[#1832]** Query injection via path constraints — RESOLVED (2026-02-28).

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
