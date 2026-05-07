> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: S1 fixed 2026-05-04 | validated: 2026-04-21 (source code analysis of graph_query_optimizer.cpp) -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Graph Module

**Last Audit:** 2026-04-21
**Auditor:** Copilot
**Status:** ✅ S1 fixed 2026-05-04 — 0 S1 open

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 8 |
| Test Coverage | ⚠️ >80% target per ROADMAP; GPU paths not yet covered |
| S0 Critical | ✅ None from graph_query_optimizer.cpp |
| S1 High | ✅ 0 (GQ-1 and GQ-2 fixed 2026-05-04) |
| Traversal timeout enforcement | ✅ **Bidirectional BFS timeout added** |

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

✅ **Fixed 2026-05-04** — A `bidiTimedOut()` lambda is now evaluated at the top of every `while` loop iteration. When `constraints.timeout_ms > 0` the clock-based check fires; when it is `0` a 30-second default cap applies. On expiry, `recordExecution()` is called and `ERR_QUERY_TIMEOUT` is returned, consistent with `executeBFS` and `executeDFS`.

~~Unlike `executeBFS()` and `executeDFS()`, the bidirectional BFS implementation had no timeout check inside its `while` loop.~~

---

#### GQ-2 · `graph_query_optimizer.cpp` · `executeSubgraphIsomorphism()` — VF2 exponential without hard resource limit

✅ **Fixed 2026-05-04** — A hard iteration counter `vf2_iteration_count` is incremented for every candidate pair evaluated. Once it exceeds `VF2_MAX_CANDIDATE_PAIRS = 10'000'000`, `early_terminated` is set to `true` and the backtrack function returns immediately. The post-`backtrack(0)` error path now checks `vf2_limit_exceeded` independently of `timeout_ms`, returning `ERR_QUERY_TIMEOUT` with a clear message about the candidate-pair limit.

~~The VF2 backtracking is O(|V|^|pattern|) with no hard upper bound when `timeout_ms = 0`.~~

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
