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
| S1 High | ⚠️ 2 (bidirectional BFS hangs; VF2 exponential) |
| Traversal timeout enforcement | 🔴 **Bidirectional BFS has no timeout** |

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

### S1 — High (from source code analysis of `graph_query_optimizer.cpp`)

#### GQ-1 · `graph_query_optimizer.cpp` · `executeBidirectional()` — No timeout check inside BFS loop

Unlike `executeBFS()` and `executeDFS()`, the bidirectional BFS implementation has **no
timeout check** inside its `while` loop:

```cpp
while (!forward_queue.empty() || !backward_queue.empty()) {
    // ← No: if (timedOut()) return Err(ERR_QUERY_TIMEOUT, ...)
    if (meeting_point.has_value()) {
        break;
    }
}
```

On a dense or cyclic graph (`max_depth = INT_MAX`, default `constraints.timeout_ms = 0`),
this loop runs indefinitely, permanently blocking the handling thread.

**Fix required:** Add `if (constraints.timeout_ms > 0 && timedOut()) { return Err(...); }`
at the top of the while loop, consistent with `executeBFS` and `executeDFS`.

---

#### GQ-2 · `graph_query_optimizer.cpp` · `executeSubgraphIsomorphism()` — VF2 exponential without hard resource limit

The VF2 backtracking is O(|V|^|pattern|). The only stopping conditions are `max_results`
and `timeout_ms`, both of which are `0` in the default `QueryConstraints{}`. A 5-vertex
pattern on a 1,000-node graph explores up to 10^15 candidate pairs:

```cpp
std::function<void(size_t)> backtrack = [&](size_t depth) {
    if (timedOut()) { ... return; }  // only if timeout_ms > 0
    for (const auto& dv : data_vertices) {
        backtrack(depth + 1);  // O(|V|^|pattern|) blow-up when timeout = 0
    }
};
backtrack(0);  // called with default QueryConstraints{}
```

**Fix required:** Enforce a hard maximum on pattern size (e.g., 8 vertices) at the API
entry point, and set a non-zero default `timeout_ms` for isomorphism queries even when
the caller does not specify one.

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
