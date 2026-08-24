# Index Module Phase 3 — A-5 & A-6 Launch Specification
**Date:** 2026-08-15 17:12 UTC  
**Status:** 📋 QUEUED (launches after Phase 2 A-2/A-3 validation, ~2026-08-26)  
**Gap Count:** 45 HIGH-severity gaps

---

## Scope Summary

### Batch A-5: circular_lock_ordering (11 gaps)
**Risk Level:** HIGH (deadlock potential)  
**Pattern:** Locks acquired in inconsistent order across different functions  
**Impact:** Potential thread deadlocks, thread safety violations

**Files Affected:**
- src/index/distributed_graph_index.cpp
- src/index/partitioned_vector_index.cpp
- include/index/*.h (lock declarations)

**Fix Strategy:**
1. Establish canonical 3-tier lock hierarchy:
   - Tier 1 (outermost): global_index_lock
   - Tier 2 (middle): partition_lock
   - Tier 3 (innermost): element_lock
2. Document lock ordering in comments
3. Verify all acquisition sites follow hierarchy
4. Add ThreadSanitizer assertions/checks

**Example Fix:**
```cpp
// Before (risk: inconsistent order):
{
    std::lock_guard<std::mutex> lock1(partLock_);  // Tier 2
    std::lock_guard<std::mutex> lock2(globalLock_); // Tier 1 ← WRONG ORDER!
    // ...
}

// After (safe: consistent order):
{
    std::lock_guard<std::mutex> lock1(globalLock_); // Tier 1
    std::lock_guard<std::mutex> lock2(partLock_);   // Tier 2
    // ...
}
```

**Validation:** ThreadSanitizer (TSan) must report 0 lock ordering issues

---

### Batch A-6: db_connection_leak (34 gaps)
**Risk Level:** HIGH (resource exhaustion)  
**Pattern:** Database connections not released in error paths or exception scenarios  
**Impact:** Connection pool exhaustion, service hangs, OOM

**Files Affected:**
- src/index/streaming_connectivity.cpp (15 gaps)
- src/index/batch_loader.cpp (10 gaps)
- src/index/vector_index.cpp (9 gaps)

**Fix Strategy:**
1. Create/use ConnectionGuard RAII wrapper:
```cpp
class ConnectionGuard {
public:
    static auto acquire(Database* db) -> std::unique_ptr<ConnectionGuard> {
        auto conn = db->getConnection();
        if (!conn) return nullptr;
        return std::make_unique<ConnectionGuard>(db, conn);
    }
    
    ~ConnectionGuard() noexcept {
        if (conn_) {
            try {
                db_->releaseConnection(conn_);
            } catch (...) {
                // Log error, don't throw from destructor
            }
        }
    }
    
private:
    Database* db_;
    Connection* conn_;
};
```

2. Replace all manual connection management with ConnectionGuard
3. Ensure cleanup happens in all error paths (exceptions, early returns)

**Validation:** ASan must report 0 connection leaks

---

## Consolidated Phase 3 Execution Plan

### Timeline
**Week 2 (Aug 26-30):**
- Aug 26-27: Implement A-5 (lock ordering, 11 gaps)
- Aug 27-28: Validate A-5 with ThreadSanitizer (≥2 consecutive PASS)
- Aug 28-29: Implement A-6 (connection leaks, 34 gaps)
- Aug 29-30: Validate A-6 with ASan (0 leaks)

**Week 3 (Sep 2-6):**
- Commit A-5 + A-6 as larger batch (45 gaps)
- Code review & merge to develop
- Move to Phase 3 Batch A-7 (deadlock_risk + generic patterns)

### Execution Model
**Single Agent, Sequential Batches:**
- Agent: themisdb-implementer (index-phase3-high-severity) — launches after Phase 2
- Batch Size: 11 + 34 = 45 gaps per commit (single larger batch)
- Validation: TSan for A-5, ASan for A-6, both required to PASS before commit

### Validation Gates

**Phase 3 A-5 (Lock Ordering):**
```bash
cmake --preset develop-tsan
ctest --preset develop-tsan
# Expected: 0 lock ordering issues, 0 data races
```

**Phase 3 A-6 (Connection Leaks):**
```bash
cmake --preset develop-asan
ctest --preset develop-asan
# Expected: 0 leaks, 0 memory errors
```

**Combined Gate:**
```bash
cmake --preset windows-release  # or linux-release
ctest --preset windows-release --output-on-failure
# Expected: All tests pass, no regressions
```

---

## Cross-Module Dependencies

**Before Phase 3 Launches:**
- ✅ Phase 2 A-2/A-3 must merge to develop (for clean base)
- ✅ Analytics Phase 2 A-2 should NOT modify streaming_connectivity.cpp in parallel
  - Coordinate: Analytics starts AFTER Index Phase 3 A-5/A-6 merge
  - Or: Use feature branches if parallel work needed

**During Phase 3 Execution:**
- 🟡 LLM Phase 2 can proceed in parallel (independent module)
- 📋 Index Phase 4+ can queue (depends on Phase 3 completion)

---

## Success Metrics

| Metric | Target | Validation |
|--------|--------|-----------|
| Deadlock risk (A-5) | 0 new issues | TSan: 0 lock ordering issues |
| Connection leaks (A-6) | 0 new leaks | ASan: 0 memory leaks |
| Test coverage | 100% pass | CTest: all tests PASS |
| Code quality | Review PASS | Peer review: no blockers |
| Performance | ±5% baseline | Benchmark gates: PASS |

---

## Deliverable

**Expected Commit:**
- Message: "index: Phase 3 A-5 + A-6 remediation (45 HIGH gaps)"
- Files: Minimal changes to lock code + connection handling
- Tests: Enhanced with 15+ lock ordering + connection leak test cases

**Summary Report:**
- File: PHASE3_A5_A6_COMPLETION_SUMMARY.md
- Contents: Deadlock prevention patterns, connection RAII patterns, validation results

---

## Prerequisites Checklist

- [ ] Phase 2 A-2/A-3 merged to develop
- [ ] Phase 5 CP-1 checkpoint PASS (no blockers)
- [ ] develop-tsan preset verified (TSan builds successfully)
- [ ] develop-asan preset verified (ASan builds successfully)
- [ ] ConnectionGuard class exists or ready to implement
- [ ] ThreadSanitizer CI/CD integration ready
- [ ] Parallel Analytics work coordination confirmed

---

## Risk Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|-----------|
| TSan false positives on locks | MEDIUM | LOW | Manually verify with helgrind/code review |
| ASan connection leak detection delay | LOW | MEDIUM | Use explicit cleanup verification in tests |
| Cross-module merge conflicts | LOW | MEDIUM | Coordinate with Analytics Phase 2 timing |
| Large batch complexity | LOW | MEDIUM | Break into smaller commits if needed (user can request) |

---

## Ready for Launch

**Status:** ✅ READY (awaiting Phase 2 merge + user approval)

**Next Step:** After Phase 2 A-2/A-3 completes (2026-08-25), launch index-phase3-high-severity agent with Batch A-5 + A-6 work.

**Estimated Duration:** 4-5 days (implementation + validation), target completion 2026-09-01
