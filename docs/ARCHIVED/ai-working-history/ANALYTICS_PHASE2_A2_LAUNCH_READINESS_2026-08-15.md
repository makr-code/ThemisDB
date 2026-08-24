# Analytics Module Phase 2 A-2 — Launch Readiness Brief
**Date:** 2026-08-15 17:12 UTC  
**Status:** 🟢 READY TO LAUNCH (after Index Phase 2 merge)  
**Target Start:** 2026-08-23 (after Index A-2/A-3 validation)

---

## Scope: db_connection_leak Remediation

**Gap Count:** 20 HIGH-severity gaps  
**Pattern:** Database connection resource leaks in streaming_window.cpp and distributed_analytics.cpp  
**Fix Strategy:** RAII ConnectionGuard wrapper + error path cleanup

### Gap Distribution
| File | Count | Pattern | Fix Type |
|------|-------|---------|----------|
| streaming_window.cpp | 15 | conn leak in window aggregation | RAII + try-catch |
| distributed_analytics.cpp | 5 | conn leak in distributed compute | RAII + cleanup path |

### Implementation Pattern

```cpp
// Before (leak):
Connection* conn = db_->getConnection();
if (!conn) return false;
// ... might throw or return early without cleanup
if (condition) return false;  // ← leak! conn never released
result = conn->query(...);
db_->releaseConnection(conn);

// After (RAII):
auto connGuard = ConnectionGuard::acquire(db_);
if (!connGuard) return false;
if (condition) return false;  // ← safe! guard destructor releases
auto result = connGuard->query(...);
// connGuard destructor automatically releases connection
```

### Validation Gates
- ✅ ASan: 0 connection leaks
- ✅ TSan: 0 use-after-free
- ✅ Existing tests: 100% pass
- ✅ No performance regression on streaming paths

### Estimated Effort
- **Implementation:** 4-6 hours (20 gaps × 15min average)
- **Testing:** 2-3 hours
- **Validation:** 1-2 hours
- **Total:** 7-11 hours (can overlap with Index Phase 2 validation)

### Prerequisites
- ✅ ConnectionGuard class exists or needs to be created
- ✅ Error codes in range [7400-7499] (Analytics uses these)
- ✅ DiagnosticEmitter integration for leak diagnostics
- ⚠️ Dependency: Index Phase 2 must merge before Analytics Phase 2 (no conflicts)

### Timeline
**Week 2 (Aug 26-30):**
- Aug 23-25: Index Phase 2 validation & merge
- Aug 26: Analytics Phase 2 A-2 launch
- Aug 28-29: Validation & commit
- Aug 30: Sign-off & move to Phase 2 Batch B

---

## Launch Checklist

- [ ] Index Phase 2 A-2/A-3 merged to develop
- [ ] ConnectionGuard class implemented (or confirmed existing)
- [ ] Test infrastructure for connection leaks ready
- [ ] Analytics module CMake presets in place (develop-asan, develop-tsan)
- [ ] Parallel Analytics Phase 2 B (pointer_arithmetic, 14 gaps) ready to queue
- [ ] User approval for Analytics Phase 2 launch timing

---

## Cross-Module Coordination

**Parallel Work During Analytics Phase 2:**
- Index Phase 3: HIGH-severity batches (11 lock + 34 connection gaps)
- LLM Phase 2 kickoff (942 CRITICAL gaps, lower-priority pattern fixes)

**Risk:** Connection leak remediation affects streaming_window.cpp which is also touched in Index Phase 3 (deadlock fixes). Coordinate merge order: Index Phase 2 → Analytics Phase 2 A-2 → Index Phase 3.

---

## Deliverable

**Expected Output:**
- Single larger batch commit: "analytics: Phase 2 A-2 remediation (20 HIGH gaps)"
- PHASE2_A2_COMPLETION_SUMMARY.md (fix patterns, validation results)
- Test coverage report (connection leak regression tests)

**Ready:** YES, awaiting Index Phase 2 merge + user confirmation
