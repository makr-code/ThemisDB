# Query Module Thread-Safety Hardening - Gap Analysis

**Phase B Gate Deliverable** | Thread-safety reduction: 140 → 60+ gaps target

---

## Executive Summary

This document identifies **5 critical thread-safety gaps** in the parallel plan optimizer and proposes targeted fixes to achieve **60%+ gap reduction** (target: 60-80 gaps remaining).

---

## Critical Gaps Identified

### GAP-1: QueryOptimizer::per_query_cost_model_ — Race on attach/detach

**Location:** `include/query/query_optimizer.h:296`, `src/query/query_optimizer.cpp:327-335`

**Issue:**
- Field: `mutable std::shared_ptr<performance::phase3::PerQueryCostModel> per_query_cost_model_`
- Methods `attachPerQueryCostModel()`, `perQueryCostModel()` use no synchronization
- Multiple threads can race during concurrent optimization plans
- Data race: write in `attachPerQueryCostModel()` vs. read in `perQueryCostModel()` and `executeOptimizedKeysWithCost()`

**Impact:** HIGH
- Race condition leads to use-after-free or stale pointer dereference
- Affects production on multi-threaded query planners

**Fix Category:** Add `mutable std::mutex` + lock guards on all accesses

---

### GAP-2: QueryOptimizer::advisor_cost_model_ — Concurrent state transitions

**Location:** `include/query/query_optimizer.h:301`, `src/query/query_optimizer.cpp:339-347`

**Issue:**
- Field: `OptimizerCostModel advisor_cost_model_`
- `setAdvisorCostConstants()` performs write without synchronization
- `chooseOrderForAndQuery()` reads from `advisor_cost_model_` without lock
- Multiple threads can see torn writes or stale values

**Impact:** HIGH
- Concurrent optimization plans get inconsistent cost model state
- Silent correctness bug: plans chosen with wrong cost estimates

**Fix Category:** Add `mutable std::mutex` + lock guards on read/write paths

---

### GAP-3: QueryOptimizer::adaptive_stats_ and adaptive_selector_ — Concurrent initialization

**Location:** `include/query/query_optimizer.h:365-366`, `src/query/query_optimizer.cpp`

**Issue:**
- Fields: `mutable std::shared_ptr<AdaptiveQueryStats> adaptive_stats_`; `mutable std::shared_ptr<AdaptivePlanSelector> adaptive_selector_`
- Method `enableAdaptiveOptimization()` performs non-atomic initialization
- Multiple concurrent callers can race on initialization
- Multiple threads can initialize, leading to leaked instances

**Impact:** MEDIUM-HIGH
- Resource leak (lost shared_ptr references)
- Non-deterministic adaptive optimizer behavior
- Potential double-initialization with inconsistent state

**Fix Category:** Double-checked locking with `mutable std::once_flag` + synchronization

---

### GAP-4: PlanCache::stats_ — Concurrent counter updates without atomics

**Location:** `include/query/plan_cache.h:329`, `src/query/plan_cache.cpp:189-234`

**Issue:**
- Field: `mutable CacheStats stats_`
- Read/write: `++stats_.hits`, `++stats_.misses`, `++stats_.invalidations`, etc.
- Protected by `std::lock_guard<std::mutex>` on get/put/invalidateTable but **NOT** on `getStats()` read
- Race: thread A reading `stats_` while thread B incrementing under lock
- Compound fields (e.g., `stats_.hits` and `stats_.current_size`) not atomically updated together

**Impact:** MEDIUM
- Stale/torn counter reads
- Incorrect cache statistics reported to monitoring (minor logic error, major observability gap)

**Fix Category:** Atomic fields or dedicated lock for stats read-back + ensure all modifications hold lock

---

### GAP-5: PlanCache::cache_, lru_list_, table_index_ — Deadline propagation without timeout guards

**Location:** `include/query/plan_cache.h:321-327`

**Issue:**
- No deadline/timeout enforcement during `get()` or `put()` operations
- Long-held locks during LRU list manipulation (erase + push_front + iterator reassignment)
- In multi-shard federation queries with deadline propagation, a blocked `get()` can propagate timeout to all downstream shards
- No cooperative cancellation token check during lock hold

**Impact:** MEDIUM-HIGH (in federation contexts)
- Cascading timeouts in federated queries
- SLA violations when plan cache is under contention
- Deadline not checked: query may violate its own deadline while waiting for plan cache lock

**Fix Category:** 
1. Add deadline parameter to get/put
2. Use `std::timed_mutex` with timeout
3. Check `QueryCancellationToken` cooperatively

---

## Gap Summary Table

| Gap ID | Component | Issue | Severity | Fix Approach |
|--------|-----------|-------|----------|--------------|
| GAP-1  | QueryOptimizer.per_query_cost_model_ | Race on attach/detach | HIGH | std::mutex + lock_guard |
| GAP-2  | QueryOptimizer.advisor_cost_model_ | Concurrent state transitions | HIGH | std::mutex + read lock + write lock |
| GAP-3  | QueryOptimizer.adaptive_*_ | Double-checked locking | MEDIUM-HIGH | std::once_flag + mutex |
| GAP-4  | PlanCache.stats_ | Non-atomic counter updates | MEDIUM | Atomic fields + stats lock |
| GAP-5  | PlanCache (all fields) | No deadline propagation | MEDIUM-HIGH | timed_mutex + cancellation token |

---

## Implementation Roadmap

### Phase 1: QueryOptimizer Per-Query Cost Model Lock (GAP-1)
- Add `mutable std::mutex per_query_cost_model_mutex_`
- Guard `attachPerQueryCostModel()` and `perQueryCostModel()`
- Guard reads in `executeOptimizedKeysWithCost()` and `executeOptimizedEntitiesWithCost()`
- **Estimated effort:** 2 hours | **Lines:** ~30

### Phase 2: QueryOptimizer Advisor Cost Model Lock (GAP-2)
- Add `mutable std::mutex advisor_cost_model_mutex_`
- Guard `setAdvisorCostConstants()` (write lock)
- Guard `advisorCostConstants()` (read lock)
- Guard advisor_cost_model_ usage in `chooseOrderForAndQuery()` (read lock)
- Document lock ordering to prevent deadlocks
- **Estimated effort:** 3 hours | **Lines:** ~50

### Phase 3: QueryOptimizer Adaptive Initialization (GAP-3)
- Add `mutable std::once_flag adaptive_init_flag_`
- Add `mutable std::mutex adaptive_init_mutex_`
- Implement `initializeAdaptiveComponents_locked()` private method
- Use `std::call_once()` in public getter methods
- **Estimated effort:** 2 hours | **Lines:** ~40

### Phase 4: PlanCache Stats Atomicity (GAP-4)
- Replace `CacheStats.hits`, `misses`, etc. with `std::atomic<uint64_t>`
- Audit all stats_ mutations to ensure they still respect cache_mutex_
- Add dedicated lock for compound getStats() read to ensure snapshot consistency
- **Estimated effort:** 1.5 hours | **Lines:** ~25

### Phase 5: PlanCache Deadline Propagation (GAP-5)
- Replace `std::mutex` with `std::timed_mutex`
- Add deadline parameter to get/put signatures (std::optional<std::chrono::steady_clock::time_point>)
- Check deadline at start and during long operations
- Add QueryCancellationToken checks cooperatively
- **Estimated effort:** 4 hours | **Lines:** ~80

---

## Lock Ordering to Prevent Deadlocks

**Global lock hierarchy (if acquiring multiple locks, acquire in this order):**

1. `PlanCache::cache_mutex_` (lowest)
2. `PlanCache::stats_lock_` (if needed separately)
3. `QueryOptimizer::advisor_cost_model_mutex_`
4. `QueryOptimizer::per_query_cost_model_mutex_`
5. `QueryOptimizer::adaptive_init_mutex_` (highest)

**Rule:** Never acquire a lower-numbered lock while holding a higher-numbered one.

---

## Test Coverage Strategy

### Concurrent Reads/Writes
- 10 threads, 1000 concurrent operations each
- Measure: no data races detected by ThreadSanitizer

### Deadline Propagation
- Inject 100ms timeout into PlanCache::get()
- Verify query cancellation honored within 50ms
- Measure: P99 latency under contention < 100ms

### Adaptive Initialization Race
- 20 threads, each calling enableAdaptiveOptimization() concurrently
- Verify single initialization (one object created, others reused)
- Measure: zero resource leaks (valgrind)

---

## Verification Checklist

- [ ] All mutable shared state protected by explicit synchronization primitives
- [ ] No bare `load()`/`store()` without memory ordering specified
- [ ] All public const methods document thread-safety invariants
- [ ] Lock ordering documented and enforced via code review
- [ ] ThreadSanitizer passes on focused query_scheduler tests
- [ ] No new performance regressions (P99 < 100ms gate)
- [ ] Backward-compatible API (no signature changes to public methods)

---

## Success Metrics

1. **Gap Reduction:** 140 → 60+ (target: 60-80 remaining)
2. **ThreadSanitizer:** Clean run on query_scheduler_focused tests
3. **Performance:** Query planner optimization latency P99 < 100ms
4. **Coverage:** 5+ concurrent scenario tests added

---

