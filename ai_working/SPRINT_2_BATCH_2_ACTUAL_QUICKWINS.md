# Sprint 2 Batch 2 (Phase 7+): Actual Quick-Win Implementation Plan

**Date:** 2026-07-02  
**Status:** EXECUTION PHASE  
**Target:** Implement QW-024 through QW-028 (5 quick-wins, 8 hours)

---

## Executive Summary

Mapping Phase 3 distributed consistency + error handling patterns to existing src/graph files:
- **5 Quick-Wins** (not 6; QW-029 deferred to batch 7)
- **8 Hours** execution (5.5 hrs + 2.5 hrs)
- **3 Files Modified:** distributed_graph.cpp, graph_query_optimizer.cpp, parallel_traversal.cpp
- **Target:** 326 tests PASS, 0 warnings

---

## Quick-Win Scope

### Batch 5: Distributed Consistency (4 items, 5.5 hours)

| QW-ID | Pattern | File(s) | Scope | Est. Time |
|-------|---------|---------|-------|-----------|
| QW-024 | DC-001: Shard-Aware Routing | distributed_graph.cpp | Primary-shard optimization + async parallelization | 90 min |
| QW-025 | DC-002: Cross-Shard Merging | distributed_graph.cpp | K-way merge for sorted results, dedup | 100 min |
| QW-026 | DC-003: Cache Invalidation | graph_query_optimizer.cpp | Broadcast invalidation hook + ACK protocol | 75 min |
| QW-027 | PA-001/DC variant: Query Affinity | distributed_graph.cpp or parallel_traversal.cpp | Shard affinity pre-computation | 70 min |

### Batch 6: Error Handling (1 item, 2.5 hours)

| QW-ID | Pattern | File(s) | Scope | Est. Time |
|-------|---------|---------|-------|-----------|
| QW-028 | ER-001 + ER-002 + ER-003: Error Mapping + Validation + Silent Failure Detection | graph_query_optimizer.cpp | Exhaustive error switch + input validation + logging | 150 min |

---

## Implementation Strategy

### Phase 1: Code Analysis (30 min)
1. Examine distributed_graph.cpp DistributedGraphExecutor::queryAcrossShards()
2. Locate merge logic in DistributedGraphExecutor 
3. Find cache modification points in graph_query_optimizer.cpp
4. Identify error handling patterns in graph_query_optimizer.cpp::executeQuery()

### Phase 2: Implementation (5.5 hours)

**QW-024: Shard-Aware Routing**
- [ ] Add PrimaryShard selection (hash-based)
- [ ] Convert sequential loop to async + futures
- [ ] Add merge callback for all-shards-ready
- [ ] Measure latency improvement
- [ ] Add unit test: primary + async secondaries

**QW-025: Cross-Shard Result Merging**
- [ ] Implement k-way merge using priority queue or heap
- [ ] Add dedup by entity_id
- [ ] Preserve rank ordering
- [ ] Add integration test: correctness + latency

**QW-026: Cache Invalidation**
- [ ] Add BroadcastInvalidation method to cache_manager (or optimizer)
- [ ] Integrate with cache write paths
- [ ] Add ACK collection or eventual consistency flag
- [ ] Add multi-shard consistency test

**QW-027: Query Affinity Pre-computation**
- [ ] Cache shard affinity decisions to avoid re-hashing
- [ ] Add affinity_cache or routing_table
- [ ] Add test: affinity correctness + cache hit rate

**QW-028: Error Handling (Combined)**
- [ ] Add exhaustive error switch with -Wswitch compilation gate
- [ ] Add input validation for query size + parameters
- [ ] Add logging for silent failures with context
- [ ] Add default error handler with fallback
- [ ] Add unit tests: error coverage + boundary conditions

### Phase 3: Validation (1 hour)
- [ ] Build: `cmake --build --preset community-release --target themis_graph --parallel 16`
- [ ] Test: `ctest --preset community-release -R "test_graph" --output-on-failure`
- [ ] Gate: All 326 tests PASS, 0 new warnings
- [ ] Benchmark: Latency before/after QW-024/025

---

## File Modification Map

| File | QW | Changes |
|------|-----|---------|
| src/graph/distributed_graph.cpp | QW-024, QW-025, QW-027 | Async routing, k-way merge, affinity cache |
| src/graph/graph_query_optimizer.cpp | QW-026, QW-028 | Cache invalidation hook, error mapping |
| src/graph/parallel_traversal.cpp | (optional) QW-027 assist | Thread pool affinity (if needed) |

---

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Move semantics change behavior | MEDIUM | Review all calling code; existing test suite must PASS 100% |
| Cache sync protocol races | MEDIUM | Multi-shard integration tests + consistency verification |
| Result ordering changed by merge | MEDIUM | Shadow-run new merge; verify determinism |
| Error handling regression | MEDIUM | Exhaustive switch + coverage tests |

---

## Success Criteria

- ✅ All 5 quick-wins implemented and tested
- ✅ 326 tests PASS with 0 new warnings
- ✅ No performance regression (latency ≤ baseline)
- ✅ Shard-aware routing shows measurable latency improvement (target: +15-30% throughput)
- ✅ K-way merge shows O(n log k) complexity improvement
- ✅ Cache invalidation consistency verified across 4+ shards
- ✅ Error handling covers 100% of error codes + boundary conditions
- ✅ Code follows C++17 RAII + exception safety standards
- ✅ Public API documentation updated for modified functions

---

## Commit Strategy

**Single batch commit** (aligned with user workflow preference):
```
Commit message: "Sprint 2 Batch 2: QW-024-028 distributed consistency & error handling

- QW-024: Shard-aware routing with async primaries
- QW-025: K-way merge for cross-shard results (O(n log k))
- QW-026: Cache invalidation broadcast with ACK protocol
- QW-027: Query affinity pre-computation with caching
- QW-028: Exhaustive error mapping + input validation + silent failure detection

Performance: +15-30% throughput (shard-aware routing)
Tests: 326 PASS, 0 warnings"
```

---

## Rollback Plan

If any quick-win fails:
1. Revert to pre-change commit
2. Fix the failing quick-win in isolation (new PR)
3. Re-baseline tests before retry

```bash
git revert HEAD~1  # Revert batch commit
git reset --soft HEAD~1  # Or: git checkout <base_commit> -- <file>
```

---

## Timeline

- **Now**: Phase 1 analysis (30 min)
- **+30 min**: QW-024 implementation (90 min)
- **+120 min**: QW-025 implementation (100 min)
- **+220 min**: QW-026 implementation (75 min)
- **+295 min**: QW-027 implementation (70 min)
- **+365 min**: QW-028 implementation (150 min)
- **+515 min**: Validation (60 min)
- **Total: ~8.5 hours** (7 hours implementation + 1.5 hours validation/testing)

---

## Dependencies & Blocking Issues

- None identified
- Build environment: `cmake --preset community-release` (can proceed without vcpkg if system packages installed)
- Test environment: 326 graph module tests available
- No blocking PRs or branch conflicts

---

## Next Steps After Sprint 2 Batch 2

1. **Merge to develop** after validation
2. **Sprint 3 (Q2-6-7)**: Memory & Performance (8 quick-wins, QW-016-023, 12 hours)
3. **Sprint 4 (Q2-8-9)**: Further optimization + stability
4. **Target v2.5-rc1** by 2026-08-27

---

**Status:** ✅ Planning complete. Ready to begin Phase 1 (analysis) immediately.
