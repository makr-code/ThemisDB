# Batch A-5: Circular Lock Ordering Remediation Plan

**Date**: 2026-08-15  
**Scope**: Fix 11 HIGH-severity circular_lock_ordering gaps across ThemisDB  
**Target Completion**: 2026-08-20  
**Status**: INITIATION  

---

## Executive Summary

**Objective**: Eliminate deadlock risks by establishing canonical lock hierarchies and enforcing consistent lock acquisition ordering across all modules.

**Gap Analysis**:
- **Total Gaps**: 11 HIGH-severity circular_lock_ordering
- **Confidence**: MEDIUM (requires ThreadSanitizer verification)
- **Risk Level**: CRITICAL (deadlock = system hang)
- **Distribution**:
  - `scheduled_edge_refresh.cpp`: 14 issues
  - `tensor_deduplication_manager.cpp`: 5 issues
  - `tensor_fingerprint_graph.cpp`: 1 issue
  - **Total identified**: 20 (exceeds initial 11 estimate)

---

## Canonical Lock Hierarchy (Tier System)

### Tier 1: Global Synchronization (Acquire First)
- `global_config_mutex_`
- `system_state_mutex_`
- `global_registry_mutex_`

### Tier 2: Subsystem/Module Level (Acquire Second)
- `module_mutex_`
- `subsystem_rwlock_`
- `policy_mutex_` (module policy)
- `rw_mutex_` (reader-writer protection)

### Tier 3: Local Resource Level (Acquire Last)
- `cv_mutex_` (condition variable)
- `cycle_mutex_` (cycle detection)
- `stats_mutex_` (statistics)
- `journal_hooks_mutex_` (journal integration)
- `hook_mutex_` (general hooks)
- `buffer_mutex_` (buffer access)

**Rule**: ALWAYS acquire locks in Tier 1 → Tier 2 → Tier 3 order. Never reverse.

---

## File-by-File Remediation Plan

### Issue Group 1: scheduled_edge_refresh.cpp (14 issues)

**Current Lock Order Issues**:
- Line 113: `policy_mutex_` then `cv_mutex_` ✓ CORRECT
- Line 148: `cycle_mutex_` then `cv_mutex_` ✗ INCORRECT (Tier 3 → Tier 3)
- Line 157: `stats_mutex_` then `policy_mutex_` ✗ INCORRECT (Tier 3 → Tier 2)
- Line 162: `stats_mutex_` then `policy_mutex_` ✗ INCORRECT (Tier 3 → Tier 2)
- Line 168: `policy_mutex_` then `cv_mutex_` ✓ CORRECT
- Line 173: `stats_mutex_` then `policy_mutex_` ✗ INCORRECT
- Line 178: `stats_mutex_` then `policy_mutex_` ✗ INCORRECT
- Line 183: `stats_mutex_` then `policy_mutex_` ✗ INCORRECT
- Line 199: `policy_mutex_` then `cv_mutex_` ✓ CORRECT
- Line 236: `policy_mutex_` then `cv_mutex_` ✓ CORRECT
- Line 312: `policy_mutex_` then `cycle_mutex_` ✓ CORRECT
- Line 448: `stats_mutex_` then `policy_mutex_` ✗ INCORRECT
- Line 497: `stats_mutex_` then `policy_mutex_` ✗ INCORRECT
- Line 560: `stats_mutex_` then `policy_mutex_` ✗ INCORRECT

**Remediation Strategy**:
1. For INCORRECT Tier 3 → Tier 2 (stats_mutex_ → policy_mutex_):
   - Restructure to: acquire `policy_mutex_` first
   - Update stats WHILE holding `policy_mutex_`
   - or: Separate stats updates from policy updates with distinct phases

2. For INCORRECT Tier 3 → Tier 3 (cycle_mutex_ → cv_mutex_):
   - Establish ordering: acquire in consistent order everywhere
   - Or: Use single condition variable with multiple conditions

**Affected Lines**: 148, 157, 162, 173, 178, 183, 448, 497, 560

### Issue Group 2: tensor_deduplication_manager.cpp (5 issues)

**Current Lock Order Issues**:
- Line 151: `rw_mutex_` then `journal_hooks_mutex_` ✗ INCORRECT (Tier 2 → Tier 3)
- Line 329: `rw_mutex_` then `journal_hooks_mutex_` ✗ INCORRECT
- Line 1268: `rw_mutex_` then `journal_hooks_mutex_` ✗ INCORRECT
- Line 1368: `rw_mutex_` then `journal_hooks_mutex_` ✗ INCORRECT
- Line 1383: `rw_mutex_` then `journal_hooks_mutex_` ✗ INCORRECT

**Root Cause**: Code acquires `rw_mutex_` (Tier 2) then needs to call journal hooks which try to acquire `journal_hooks_mutex_` (Tier 3). This is backwards.

**Remediation Strategy**:
1. Wrap journal hook invocations OUTSIDE the `rw_mutex_` scope
2. Defer journal operations until after releasing `rw_mutex_`
3. Or: Add pre-journal-check phase before acquiring `rw_mutex_`

**Affected Lines**: 151, 329, 1268, 1368, 1383

### Issue Group 3: tensor_fingerprint_graph.cpp (1 issue)

**Current Lock Order Issue**:
- Line 263: `mutex_` then `hook_mutex_` ✗ INCORRECT

**Remediation Strategy**:
1. Release `mutex_` before invoking hook operations
2. Or: Mark `hook_mutex_` as Tier 2 (same level) and enforce consistent ordering

---

## Implementation Approach

### Phase 1: Code Analysis (COMPLETED)
- [x] Identify all 11+ circular_lock_ordering gaps
- [x] Map lock hierarchy tiers
- [x] Classify issues by type (forward, reverse, same-tier inconsistency)

### Phase 2: Implement Fixes (IN PROGRESS)

#### For scheduled_edge_refresh.cpp:
1. Extract common lock ordering pattern
2. Create helper function: `ScopedPolicyAndStats` RAII guard
3. Replace inline lock sequences with helper
4. Add thread-safety annotations

#### For tensor_deduplication_manager.cpp:
1. Split lock scopes: dedup logic separate from journal logic
2. Create callback queue to defer journal operations
3. Update journal hooks to run OUTSIDE `rw_mutex_` scope

#### For tensor_fingerprint_graph.cpp:
1. Add scope guard for hook operations
2. Document lock hierarchy in class header

### Phase 3: Validation (PENDING)

#### ThreadSanitizer Verification
```bash
cmake --preset community-asan
cmake --build build-community-asan --target index_tests -j 16
cd build-community-asan
ctest -L index --output-on-failure -j 1 -VV
```

#### Code Review Checklist
- [ ] All lock acquisitions follow Tier 1 → 2 → 3 order
- [ ] Thread-safety annotations added (clang: `[[clang::acquire_capability]]`, etc.)
- [ ] RAII guards used for all critical sections
- [ ] No nested lock operations on same tier without strict ordering
- [ ] Deadlock risk comments removed/resolved
- [ ] Test pass rate = 100% (no regression)

---

## Code Patterns

### Pattern 1: Tier-aware RAII Guard

```cpp
// DO: Acquire in canonical order
{
    std::unique_lock<std::mutex> policy_lock(policy_mutex_);      // Tier 2
    // ... policy work ...
    {
        std::unique_lock<std::mutex> stats_lock(stats_mutex_);    // Tier 3 (nested OK)
        // ... stats work ...
    }  // stats_lock released
}  // policy_lock released

// DON'T: Acquire in wrong order
{
    std::unique_lock<std::mutex> stats_lock(stats_mutex_);      // Tier 3
    // ... stats work ...
    {
        std::unique_lock<std::mutex> policy_lock(policy_mutex_); // Tier 2 (WRONG!)
        // This can deadlock if another thread acquires in correct order
    }
}
```

### Pattern 2: Defer External Callbacks

```cpp
// DO: Defer callbacks until after releasing locks
std::vector<Callback> deferred_callbacks;
{
    std::unique_lock<std::mutex> lock(rw_mutex_);
    // ... modify state ...
    // Collect callbacks to run later
    deferred_callbacks = collect_pending_callbacks();
}  // lock released
// NOW invoke callbacks outside lock
for (auto& cb : deferred_callbacks) {
    cb();  // May safely acquire journal_hooks_mutex_
}

// DON'T: Invoke callbacks while holding lock
{
    std::unique_lock<std::mutex> lock(rw_mutex_);
    // ... modify state ...
    invoke_hooks();  // ← May acquire other locks → deadlock risk
}
```

### Pattern 3: Thread-Safety Annotations

```cpp
class MyClass {
private:
    mutable std::mutex policy_mutex_;
    std::mutex stats_mutex_;
    int policy_state_ GUARDED_BY(policy_mutex_);
    int stats_ GUARDED_BY(stats_mutex_);

public:
    [[nodiscard]] bool update_policy(int new_val)
        ACQUIRE_SHARED(policy_mutex_) ACQUIRE_SHARED(stats_mutex_) {
        // Compiler checks lock acquisition order
        std::shared_lock<std::mutex> p(policy_mutex_);  // Tier 2
        std::shared_lock<std::mutex> s(stats_mutex_);   // Tier 3
        policy_state_ = new_val;
        stats_++;
        return true;
    }
};
```

---

## Success Criteria

- [ ] All 11+ issues identified and categorized
- [ ] Tier hierarchy defined and documented
- [ ] 100% of fixes implement canonical lock ordering
- [ ] ThreadSanitizer: ≥2 consecutive PASS runs
- [ ] No new compiler warnings
- [ ] All existing tests PASS
- [ ] Code review APPROVED
- [ ] Commit message includes issue tracking

---

## Testing Strategy

### Unit Tests
- Existing tests should PASS without modification
- Add stress test: 100+ iterations with varied thread counts
- Add race detection: Run with ThreadSanitizer enabled

### Integration Tests
- Run full index test suite
- Monitor for unexpected deadlocks during test execution
- Measure lock contention baseline (< 5ms avg wait time)

### Regression Tests
- Before/after performance comparison
- Before/after thread contention metrics
- Lock hold time telemetry

---

## Timeline

| Date | Milestone | Status |
|------|-----------|--------|
| 2026-08-15 | Batch A-5 PLAN document created | ✓ COMPLETE |
| 2026-08-16 | Code analysis + fixes implemented | → NEXT |
| 2026-08-17 | ThreadSanitizer validation | → PENDING |
| 2026-08-18 | Code review + final validation | → PENDING |
| 2026-08-19 | Merge to main | → PENDING |
| 2026-08-20 | Ready for Batch A-6 | → PENDING |

---

## References

- gap_index_phase1_verification_report.md: 11 circular_lock_ordering gaps
- scan_graph_impact_fixed.json: Detailed lock order conflicts
- ARCHITECTURE.md: Thread-safety requirements
- C++ Best Practices: `.github/instructions/cpp-best-practices.instructions.md`

---

## Owner & Approval

- **Prepared by**: AI Implementation Agent
- **Date**: 2026-08-15
- **Status**: Ready for Phase 2 implementation

