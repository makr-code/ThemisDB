# Sprint 2 Quick-Win Execution Plan

**Sprint:** Phase 3 Sprint 2 (2026-07-02 onwards)  
**Duration:** 7 days (parallel batch execution)  
**Deliverable:** 1 PR with 8 quick-wins (Batches 1–2)  
**Success Gate:** 326 tests PASS, zero regressions

---

## Overview

Sprint 2 executes **two parallel quick-win batches** from the Sprint 1 analysis:

1. **QW-P3-2: Scope & Lifetime** (3 findings, 4 hours)
2. **QW-P3-1: Cache & Concurrency** (5 findings, 8 hours)

Both batches run **in parallel** with recommended agent assignment:
- **Agent 1:** themisdb-implementer (Batch 1: Scope & Lifetime)
- **Agent 2:** themisdb-implementer (Batch 2: Cache & Concurrency)
- **Sync:** gap-verifier (Quality validation after each batch)
- **Gate:** themisdb-reviewer (Pre-merge code review)

---

## Batch 1: QW-P3-2 — Scope & Lifetime (3 hrs, 3 findings)

### Findings to Fix

| ID | Category | Issue | File | Estimate |
|----|----------|-------|------|----------|
| QW-008 | Scope | Iterator invalidation after erase | rotate_completion.cpp | 45 min |
| QW-009 | Lifetime | Resource cleanup in exception paths | explain_plan.cpp | 60 min |
| QW-010 | Scope | RAII guard missing in query execution | graph_executor.cpp | 60 min |

### Fix Patterns (from Sprint 1 Playbooks)

**Pattern 1: Iterator Safety**
```cpp
// BEFORE: Unsafe erase inside loop
for (auto it = items.begin(); it != items.end(); ++it) {
  if (shouldRemove(*it)) {
    items.erase(it);  // ❌ Invalidates iterator
  }
}

// AFTER: Safe erase
for (auto it = items.begin(); it != items.end(); ) {
  if (shouldRemove(*it)) {
    it = items.erase(it);  // ✅ Advances iterator safely
  } else {
    ++it;
  }
}
```

**Pattern 2: Exception-Safe Cleanup**
```cpp
// BEFORE: Resource leaked on exception
void processQuery() {
  auto* cache = new QueryCache();
  validateQuery();  // May throw
  executeQuery(cache);
  delete cache;
}

// AFTER: RAII cleanup
void processQuery() {
  auto cache = std::make_unique<QueryCache>();
  validateQuery();
  executeQuery(cache.get());
  // Auto-cleanup on scope exit or exception
}
```

**Pattern 3: RAII Guard Pattern**
```cpp
// BEFORE: Manual state management
void executeInParallel() {
  startParallelMode();
  executeQueries();
  if (error) {
    endParallelMode();  // ❌ Missed path
    return;
  }
  endParallelMode();
}

// AFTER: RAII guard ensures cleanup
void executeInParallel() {
  RAIIGuard<ParallelMode> guard;  // Enters on construct, exits on destroy
  executeQueries();
}
```

### Implementation Steps

1. **Identify affected code** (suggested start points):
   - `src/graph/rotate_completion.cpp:145–165` (iterator erase loop)
   - `src/graph/explain_plan.cpp:203–234` (exception cleanup)
   - `src/graph/graph_executor.cpp:578–615` (parallel mode guard)

2. **Apply fix pattern** to each finding

3. **Write unit tests**:
   - Iterator invalidation edge cases
   - Exception propagation from cleanup paths
   - Guard initialization/finalization

4. **Run validation**:
   ```bash
   cmake --preset community-release --target themis_graph
   ctest --preset community-release -R "test_graph_rotate|test_graph_explain|test_graph_executor" -VV
   ```

5. **Verify no regressions** (full test suite on Batch 1 complete):
   ```bash
   ctest --preset community-release -R "test_graph" --verbose
   ```

---

## Batch 2: QW-P3-1 — Cache & Concurrency (8 hrs, 5 findings)

### Findings to Fix

| ID | Category | Issue | File | Estimate |
|----|----------|-------|------|----------|
| QW-011 | Cache | Missing read lock during validation | query_cache.cpp | 75 min |
| QW-012 | Concurrency | Race condition in cache invalidation | query_cache.cpp | 90 min |
| QW-013 | Cache | Cache coherency: duplicate entries | graph_executor.cpp | 60 min |
| QW-014 | Concurrency | Atomic increment without CAS loop | result_buffer.cpp | 75 min |
| QW-015 | Cache | Result cloning inefficiency | query_cache.cpp | 60 min |

### Fix Patterns (from Sprint 1 Playbooks)

**Pattern 1: Read-Write Locking**
```cpp
// BEFORE: No synchronization during read
if (cache_.count(key)) {
  return cache_[key];  // ❌ Unsync'd read
}

// AFTER: Read lock for validation
{
  std::shared_lock<std::shared_mutex> lock(mutex_);
  if (cache_.count(key)) {
    return cache_[key];  // ✅ Protected read
  }
}
```

**Pattern 2: Cache Invalidation with Atomics**
```cpp
// BEFORE: Non-atomic check-then-act
if (!cache_valid_) {
  rebuildCache();  // ❌ Race: another thread may invalidate first
}

// AFTER: Atomic CAS loop
bool expected = false;
while (!cache_valid_.compare_exchange_strong(expected, true)) {
  expected = false;
  std::this_thread::yield();
}
// Now safe to proceed
```

**Pattern 3: Duplicate Prevention**
```cpp
// BEFORE: Silent duplicates in cache
cache_[key] = value;  // ❌ May overwrite different entry in hash collision

// AFTER: Verify uniqueness before insert
{
  std::unique_lock<std::shared_mutex> lock(mutex_);
  if (cache_.find(key) == cache_.end()) {
    cache_[key] = value;  // ✅ Inserted only if not present
  } else {
    // Log duplicate; return existing
  }
}
```

**Pattern 4: Atomic Counter Safety**
```cpp
// BEFORE: Unsafe increment
result_count_++;  // ❌ Non-atomic

// AFTER: Atomic increment (already atomic, but ensure correct usage)
result_count_.fetch_add(1, std::memory_order_release);
```

**Pattern 5: Move Semantics to Avoid Cloning**
```cpp
// BEFORE: Unnecessary copy/clone
QueryResult result = cache_[key];  // ❌ Deep copy
return result;

// AFTER: Move semantics
QueryResult result = std::move(cache_[key]);
return result;  // Or return directly if temporary
```

### Implementation Steps

1. **Identify affected code**:
   - `src/graph/query_cache.cpp:87–112` (read lock)
   - `src/graph/query_cache.cpp:234–267` (invalidation race)
   - `src/graph/graph_executor.cpp:156–189` (duplicate entries)
   - `src/graph/result_buffer.cpp:445–478` (atomic counter)
   - `src/graph/query_cache.cpp:301–325` (result cloning)

2. **Apply fix patterns** to each finding

3. **Write concurrency tests**:
   - Thread safety (multiple readers + writer)
   - Cache invalidation under load
   - Duplicate detection
   - Atomic counter correctness

4. **Run validation**:
   ```bash
   cmake --preset community-release --target themis_graph
   ctest --preset community-release -R "test_graph_cache|test_graph_concurrency" -VV
   ```

5. **Run race detector** (optional but recommended):
   ```bash
   ctest --preset community-release -R "test_graph_cache" --timeout 300 -VV
   ```

---

## Execution Timeline

| Day | Phase | Batch 1 (Scope) | Batch 2 (Cache) | Sync Point |
|-----|-------|-----------------|-----------------|-----------|
| Day 1–2 | Setup | Build + baseline | Build + baseline | Full test suite |
| Day 2–3 | Impl | Identify + fix iterator | Identify + fix locking | — |
| Day 3–4 | Impl | Fix exception cleanup | Fix race conditions | — |
| Day 4–5 | Test | Unit tests + validation | Unit tests + validation | Manual review |
| Day 5–6 | Review | gap-verifier check | gap-verifier check | Fix findings |
| Day 6–7 | Merge | Merge to develop | Merge to develop | Full test suite |

---

## Success Criteria

✅ **Code Quality:**
- All 8 quick-wins implemented per playbook patterns
- No compiler warnings
- No ASAN/valgrind issues
- No code style violations

✅ **Testing:**
- 326 existing tests PASS
- New tests added for each fix
- No flaky tests
- No regressions vs. Phase 2.4 baseline

✅ **Documentation:**
- Each fix includes inline comments (why, not just what)
- API changes documented (if any)
- Test cases documented

---

## Recommended Agent Prompts

### Batch 1 — themisdb-implementer agent

```
Sprint 2 Quick-Win Batch 1: Scope & Lifetime (3 findings)
Target: rotate_completion.cpp, explain_plan.cpp, graph_executor.cpp

Findings to fix (from PHASE_3_FINDING_ANALYSIS.md):
- QW-008: Iterator invalidation after erase (rotate_completion.cpp)
- QW-009: Resource cleanup in exception paths (explain_plan.cpp)
- QW-010: RAII guard missing in query execution (graph_executor.cpp)

Fix patterns from playbooks:
1. Iterator Safety: Use erase() return value to advance iterator
2. Exception-Safe Cleanup: Convert raw new/delete to std::unique_ptr
3. RAII Guard: Implement scope guard for parallel mode

Timeline: 4 hours total
Success criteria: 326 tests PASS, unit tests for each fix, no regressions

Build/test: cmake --preset community-release && ctest --preset community-release -R "test_graph"
```

### Batch 2 — themisdb-implementer agent (parallel)

```
Sprint 2 Quick-Win Batch 2: Cache & Concurrency (5 findings)
Target: query_cache.cpp, graph_executor.cpp, result_buffer.cpp

Findings to fix (from PHASE_3_FINDING_ANALYSIS.md):
- QW-011: Missing read lock during validation (query_cache.cpp)
- QW-012: Race condition in cache invalidation (query_cache.cpp)
- QW-013: Cache coherency: duplicate entries (graph_executor.cpp)
- QW-014: Atomic increment without CAS loop (result_buffer.cpp)
- QW-015: Result cloning inefficiency (query_cache.cpp)

Fix patterns from playbooks:
1. Read-Write Locking: Use std::shared_lock for read-only access
2. Atomic CAS: Implement compare_exchange_strong loop for invalidation
3. Duplicate Prevention: Check-then-insert with unique_lock
4. Atomic Operations: Ensure correct memory_order semantics
5. Move Semantics: Eliminate unnecessary copies

Timeline: 8 hours total
Success criteria: 326 tests PASS, concurrency tests for race conditions, no TLS issues

Build/test: cmake --preset community-release && ctest --preset community-release -R "test_graph_cache"
```

---

## Post-Sprint Review

After both batches complete and all tests PASS:

1. **gap-verifier agent** performs quality check:
   - Code review against playbook patterns
   - False-positive detection
   - Risk assessment

2. **themisdb-reviewer agent** performs pre-merge review:
   - Architecture conformance
   - API documentation
   - Test coverage

3. **Merge decision:**
   - If all gates PASS → merge 1 PR (8 quick-wins) to `develop`
   - If gaps found → fix in follow-up batch

4. **Proceed to Sprint 3:**
   - Repeat for QW-P3-3 (Memory & RAII) and QW-P3-4 (Performance & Algo)

---

## Files to Reference

- `ai_working/PHASE_3_FINDING_ANALYSIS.md` — Full details of each finding + fix patterns
- `ai_working/SPRINT_1_COMPLETION_SUMMARY.md` — Executive summary
- `src/graph/rotate_completion.cpp` — Batch 1 target 1
- `src/graph/explain_plan.cpp` — Batch 1 target 2
- `src/graph/graph_executor.cpp` — Batch 1 target 3 + Batch 2 target 2
- `src/graph/query_cache.cpp` — Batch 2 targets 1, 2, 5
- `src/graph/result_buffer.cpp` — Batch 2 target 4

---

**Next Step:** Launch themisdb-implementer agents for Batch 1 & 2 in parallel mode.

