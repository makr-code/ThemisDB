# Batch A-5: Circular Lock Ordering — EXECUTION PLAN

**Date**: 2026-08-15  
**Target**: Fix 20 HIGH-severity circular_lock_ordering gaps  
**Status**: READY FOR EXECUTION  
**Estimated Effort**: 2-3 days  

---

## Executive Summary

**Objective**: Eliminate all 20 HIGH-severity circular lock ordering deadlock risks by establishing and enforcing a canonical lock hierarchy.

**Gaps Found**: 20 total (vs. 11 estimated in specification)
- `scheduled_edge_refresh.cpp`: 14 issues  
- `tensor_deduplication_manager.cpp`: 5 issues  
- `tensor_fingerprint_graph.cpp`: 1 issue  

**Risk Level**: CRITICAL (deadlock = system hang)  
**Validation**: ThreadSanitizer + ASan + manual code review  

---

## Implementation Strategy

### Step 1: Root Cause Analysis (COMPLETED)

**Finding**: Lock acquisition order is inconsistent across modules.

**Pattern 1**: stats_mutex_ → policy_mutex_
- Lines affected: 157, 162, 173, 178, 183, 448, 497, 560 (scheduled_edge_refresh.cpp)
- Issue: stats_mutex_ (Tier 3) acquired BEFORE policy_mutex_ (Tier 2)
- Fix: Reverse order or restructure

**Pattern 2**: cycle_mutex_ → cv_mutex_  
- Lines affected: 148 (scheduled_edge_refresh.cpp)
- Issue: cv_mutex_ should be lowest priority (always acquired last)
- Fix: Restructure to acquire cv_mutex_ before cycle_mutex_

**Pattern 3**: policy_mutex_ → cv_mutex_
- Lines affected: 113, 168, 199, 236 (scheduled_edge_refresh.cpp)
- Issue: policy_mutex_ (Tier 2) acquired before cv_mutex_ (Tier 3) - actually CORRECT
- Note: May be false positive or needs documentation

**Pattern 4**: policy_mutex_ → cycle_mutex_
- Lines affected: 312 (scheduled_edge_refresh.cpp)
- Issue: Mixed ordering with other operations
- Fix: Document or restructure if conflict

**Pattern 5**: rw_mutex_ → journal_hooks_mutex_
- Lines affected: 151, 329, 1268, 1368, 1383 (tensor_deduplication_manager.cpp)
- Issue: rw_mutex_ (Tier 2) acquired before journal_hooks_mutex_ (Tier 3) - actually CORRECT
- Note: Need to verify no callbacks try to acquire higher-tier locks

**Pattern 6**: mutex_ → hook_mutex_
- Lines affected: 263 (tensor_fingerprint_graph.cpp)
- Issue: General mutex (Tier 2) before hook_mutex_ (Tier 3) - actually CORRECT
- Note: Verify hook implementation

---

### Step 2: Canonical Lock Hierarchy (TO IMPLEMENT)

```
Tier 1 (Global): Acquire FIRST
├── global_state_mutex
├── system_coordinator_lock
└── registry_mutex

Tier 2 (Module): Acquire SECOND  
├── policy_mutex_
├── rw_mutex_
├── cycle_mutex_ (for cycle tracking)
├── module_state_lock
└── configuration_lock

Tier 3 (Local): Acquire LAST
├── cv_mutex_ (condition variable)
├── stats_mutex_
├── journal_hooks_mutex_
├── buffer_mutex_
├── hook_mutex_
├── cache_mutex_
└── event_queue_mutex
```

**Rule**: ALWAYS acquire in order Tier 1 → Tier 2 → Tier 3.  
**Violation**: Two threads acquire same pair of locks in opposite order → DEADLOCK.

---

### Step 3: File-Specific Fixes

#### File 1: `src/graph/scheduled_edge_refresh.cpp`

**Current Lock Patterns**:
```cpp
// schedulerLoop (lines 392-419)
std::lock_guard<std::mutex> lock(policy_mutex_);       // Tier 2
// ...
std::unique_lock<std::mutex> lk(cv_mutex_);            // Tier 3 ✓ CORRECT ORDER
cv_.wait_for(lk, interval, [...]);

// runRefreshCycle (lines 420+)
std::lock_guard<std::mutex> lock(policy_mutex_);       // Tier 2
// ...
std::lock_guard<std::mutex> lock(stats_mutex_);        // Tier 3 ✗ WRONG if reversed elsewhere
```

**Remediation**:

1. **Lines 157-183**: Fix stats_mutex_ → policy_mutex_ (reverse acquisition)
   
   ```cpp
   // BEFORE (WRONG):
   {
       std::lock_guard<std::mutex> lock(stats_mutex_);
       changefeed_ = std::move(changefeed);
   }
   
   // AFTER (CORRECT):
   {
       // Hold policy_mutex_ while updating interdependent state
       // or restructure to avoid cross-lock dependency
       std::lock_guard<std::mutex> lock(policy_mutex_);
       {
           std::lock_guard<std::mutex> stats_lock(stats_mutex_);
           changefeed_ = std::move(changefeed);
       }
   }
   ```

2. **Line 148**: cycle_mutex_ → cv_mutex_ reversal
   
   ```cpp
   // BEFORE: spin-wait without proper CV
   // AFTER: Use cv_mutex_ before cycle_mutex_ in condition variable scenario
   ```

3. **Lines 113, 168, 199, 236**: Verify policy_mutex_ → cv_mutex_ ordering (should be OK)
   - These are in correct Tier 2 → Tier 3 order
   - Add documentation comment: "Canonical order: policy_mutex (Tier 2) before cv_mutex (Tier 3)"

4. **Line 312**: policy_mutex_ → cycle_mutex_ conflict with other code paths
   - Verify if `cycle_mutex_` is actually Tier 2 or Tier 3
   - If Tier 2, ensure NEVER acquired after Tier 3 locks
   - Restructure scheduler logic if needed

---

#### File 2: `src/tensor/tensor_deduplication_manager.cpp`

**Current Pattern**: rw_mutex_ → journal_hooks_mutex_  
**Classification**: Tier 2 → Tier 3 (technically CORRECT)

**Verification Needed**:
1. Check journal_hooks_mutex_ callback implementation
2. Ensure callbacks DO NOT try to acquire Tier 2 locks
3. If they do: restructure to queue callbacks and invoke outside locks

**Remediation**:

1. **Lines 151, 329, 1268, 1368, 1383**: Defer journal operations
   
   ```cpp
   // BEFORE (potential deadlock if callback acquires higher-tier lock):
   {
       std::unique_lock<std::mutex> lock(rw_mutex_);
       // ... modify state ...
       invoke_journal_hooks();  // May acquire policy_mutex_ → deadlock!
   }
   
   // AFTER (safe):
   std::vector<JournalCallback> deferred;
   {
       std::unique_lock<std::mutex> lock(rw_mutex_);
       // ... modify state ...
       deferred = collect_pending_journal_ops();
   }  // lock released
   // Now safe to invoke callbacks
   for (auto& cb : deferred) {
       cb();
   }
   ```

---

#### File 3: `src/tensor/tensor_fingerprint_graph.cpp`

**Current Pattern**: mutex_ → hook_mutex_  
**Classification**: Tier 2 → Tier 3 (CORRECT)

**Verification**: Ensure hook implementation doesn't acquire higher-tier locks.

---

### Step 4: Implementation (TO DO)

#### Phase A: Code Analysis (30 min)
- [ ] Read all lock acquisition sites in affected files
- [ ] Create lock dependency graph
- [ ] Identify all circular paths
- [ ] Mark safe vs. unsafe patterns

#### Phase B: Fixes (2 hours per file × 3 = 6 hours)

**scheduled_edge_refresh.cpp**:
1. Restructure setChangefeed/setANNIndex/setCEPEventCallback
2. Review schedulerLoop CV usage
3. Verify cycle_mutex_ tier assignment
4. Add lock hierarchy documentation

**tensor_deduplication_manager.cpp**:
1. Extract journal operations from locked sections
2. Create deferred callback queue
3. Invoke callbacks outside locks
4. Add verification tests

**tensor_fingerprint_graph.cpp**:
1. Verify hook_mutex_ usage
2. Document lock hierarchy
3. Add thread-safety annotations

#### Phase C: Validation (2 hours)
- [ ] Compile without errors/warnings
- [ ] Run existing tests: `ctest -L graph,tensor -j 4`
- [ ] Run ThreadSanitizer: `cmake --preset community-asan && ctest -L graph,tensor -j 1`
- [ ] Run 2nd validation pass to confirm no race conditions

#### Phase D: Review & Merge (1 hour)
- [ ] Code review checklist
- [ ] Commit with detailed message
- [ ] Update CHANGELOG.md

---

## Validation & Testing

### Test Execution

```bash
# Configure ASan build
cmake --preset community-asan

# Build graph/tensor modules
cmake --build build-community-asan --target graph_tests tensor_tests -j 16

# Run tests with ThreadSanitizer
cd build-community-asan
ctest -L "graph|tensor" --output-on-failure -j 1 -VV

# Stress test (high concurrency)
ctest -R "concurrent|thread|stress" -j 8 --timeout 60
```

### Success Criteria

- [ ] All 20 issues identified
- [ ] Root cause documented for each
- [ ] All fixes implemented
- [ ] No NEW compiler warnings
- [ ] All existing tests PASS (no regression)
- [ ] ThreadSanitizer: 0 data races
- [ ] ThreadSanitizer: ≥2 consecutive PASS runs
- [ ] Lock contention < 5ms avg (performance baseline)
- [ ] Code review APPROVED

---

## Timeline

| Date | Task | Owner | Status |
|------|------|-------|--------|
| 2026-08-15 | Create execution plan | AI Agent | ✓ DONE |
| 2026-08-16 | Phase A (analysis) + Phase B (fixes) | AI Agent | → NEXT |
| 2026-08-17 | Phase C (validation) + Phase D (review) | AI Agent | → PENDING |
| 2026-08-18 | ThreadSanitizer 2nd pass + final merge | AI Agent | → PENDING |
| 2026-08-19 | Ready for Batch A-6 gate | - | → PENDING |

---

## Code Review Checklist

- [ ] All lock acquisitions follow Tier 1 → 2 → 3 order
- [ ] No nested lock acquisitions in reverse order
- [ ] RAII guards (lock_guard, unique_lock) used everywhere
- [ ] No try_lock() in circular dependency paths
- [ ] Thread-safety comments added at each lock site
- [ ] Deferred callback pattern used where applicable
- [ ] No silent lock failures (all guards checked)
- [ ] Deadlock risk comments resolved/documented
- [ ] No new compiler warnings (-Wall -Wextra -Wpedantic)

---

## References

- gap_index_phase1_verification_report.md: circular_lock_ordering (11 reported)
- scan_graph_impact_fixed.json: 20 actual issues found
- ARCHITECTURE.md: Thread-safety requirements
- `.github/instructions/cpp-best-practices.instructions.md`

---

## Owner

- **Prepared by**: AI Implementation Agent  
- **Date**: 2026-08-15  
- **Status**: Ready for Phase A (analysis & implementation)
- **Approval**: Pending code review

