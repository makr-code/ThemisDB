# Phase 7: Sprint 2 Batch 2 — Distributed Consistency & Error Handling Kickoff

**Date:** 2026-07-02 07:25 UTC  
**Status:** READY FOR IMPLEMENTATION  
**Duration:** ~8 hours (1 day)  
**Deliverable:** 1 commit with 6 quick-wins

---

## Overview

Following completion of EPIC 3.3 (factorization-aware shard placement), Phase 7 advances through Block 3 Phase 3 Sprint 2 Batch 2, focusing on distributed query optimization and error handling hardening.

## Quick-Wins Scope (6 fixes)

### Category 5: Distributed Consistency (4 fixes, 5.5 hours)

| QW-ID | File | Issue | Solution Pattern | Duration |
|-------|------|-------|------------------|----------|
| **QW-024** | `src/distributed/shard_query.cpp` | Sequential query execution blocks parallelism | Route primary query, dispatch async secondary queries with timeout | 90 min |
| **QW-025** | `src/distributed/result_merger.cpp` | O(n²) merge complexity for k-way merges | Implement min-heap k-way merge algorithm; O(n log k) | 100 min |
| **QW-026** | `src/distributed/cross_shard_cache.cpp` | Cache consistency issues across shards | Broadcast invalidation protocol on every modification | 75 min |
| **QW-027** | `src/distributed/distributed_planner.cpp` | Shard affinity decisions made ad-hoc | Pre-compute affinity routing policy at planning time | 70 min |

### Category 6: Error Handling & Robustness (2 fixes, 2.5 hours)

| QW-ID | File | Issue | Solution Pattern | Duration |
|-------|------|-------|------------------|----------|
| **QW-028** | `src/query/query_validator.cpp` | Missing input validation | Add bounds checking for numeric ranges; throw on invalid | 60 min |
| **QW-029** | `src/query/error_mapper.cpp` | Unhandled error codes cause silent failures | Exhaustive switch-case for all ErrorCode enum values | 75 min |

---

## Execution Phases

### Phase 1: File Discovery & Analysis (1 hour)

**Tasks:**
1. Locate target files in repository
2. Understand current implementation (class hierarchy, key functions)
3. Identify modification points (function signatures, call sites)
4. Review test infrastructure for each module

**Deliverable:** File map with modification points documented

### Phase 2: Implementation (6 hours)

**For each quick-win:**

1. **Read existing code** (5–10 min per file)
   - Understand class structure, member variables, public API
   - Identify where change applies (usually 1–3 functions)

2. **Implement fix** (40–80 min per fix)
   - Apply solution pattern from table above
   - Follow C++17 RAII best practices
   - Use `std::shared_ptr`, `std::unique_lock`, `std::optional` where appropriate
   - Add comprehensive error handling

3. **Document changes** (10–15 min per fix)
   - Add Doxygen comments to modified functions
   - Document assumptions and edge cases
   - Update README if approach changes API

**Code Quality Checklist (per fix):**
- ✅ No raw `new`/`delete` (RAII only)
- ✅ Const-correctness enforced
- ✅ Move semantics where appropriate (e.g., result building)
- ✅ Exception-safe (strong or basic guarantee)
- ✅ Doxygen comments complete (@brief, @param, @return, @throws)
- ✅ Zero compiler warnings (clean build)

### Phase 3: Validation (1 hour)

**Build:**
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset community-release
cmake --build --preset community-release --target themis_graph --parallel 16
```

**Test:**
```bash
ctest --preset community-release -R "test_graph" --output-on-failure
```

**Verification:**
- [ ] Build succeeds with 0 new warnings
- [ ] All 326 tests PASS on first run
- [ ] No test timeouts or crashes
- [ ] Performance regression < 1% (measured in ctest output)

---

## Phase 3 Finding Analysis Reference

For detailed root-cause analysis and fix patterns, refer to:
- `ai_working/PHASE_3_FINDING_ANALYSIS.md` — Full categorization
- `ai_working/BLOCK_3_PHASE_3_NEXT_STEPS.md` — Detailed sprint plan

---

## Success Criteria

### Gate 1: File Discovery ✅
- [ ] All 6 target files located
- [ ] Modification points identified
- [ ] Build environment ready

### Gate 2: Implementation ✅
- [ ] All 6 quick-wins implemented
- [ ] Code review checklist passed for each
- [ ] Doxygen comments complete

### Gate 3: Validation ✅
- [ ] Build: 0 new warnings
- [ ] Tests: 326/326 PASS
- [ ] Performance: No regressions detected

---

## Recommended Agent

**Type:** `general-purpose`  
**Rationale:** Distributed consistency + error handling are architecturally coupled; require coordinated decision-making across modules

**Alternative:** Use parallel `explore` agents if files prove independent after discovery phase

---

## Timeline

| Task | Duration | Start | End |
|------|----------|-------|-----|
| Phase 1: Discovery | 1 hour | 08:00 | 09:00 |
| Phase 2: Implementation | 6 hours | 09:00 | 15:00 |
| Phase 3: Validation | 1 hour | 15:00 | 16:00 |
| **Total** | **8 hours** | — | — |

**Target Completion:** 2026-07-03 (EOD)

---

## Rollback Plan

If issues encountered:

1. **Build failures** — Revert to clean state: `git reset --hard origin/develop`
2. **Test failures** — Isolate failing quick-win, debug, or defer to Sprint 3
3. **Performance regression** — Profile hot path, optimize or adjust approach

---

## Notes for Next Phases

### Sprint 3 (Week 2): Memory & Performance
- 8 quick-wins focusing on pooling, cache sizing, hot-path latency
- Recommendation: Parallel explore agents

### Sprint 4 (Weeks 3–4): Hardening
- 6 quick-wins + 35 HIGH severity findings
- Load balancing, resource cleanup, error propagation

### Sprint 5–6 (Weeks 5–6): Release Gate
- 100x stability runs (100% pass rate required)
- L1 conformance audit (4/4 criteria)
- CodeQL security scan
- Release candidate v2.5-rc1

---

**Status:** READY  
**Next Action:** Assign to general-purpose agent or proceed with manual implementation  
**Approval:** User approval via "weiter" comment to proceed
