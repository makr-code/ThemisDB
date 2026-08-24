# Analytics Module Phase 2: Batch A - Lock Ordering & Connection Leaks Fix

**Status**: Implementation Design  
**Created**: 2026-08-15T09:15Z  
**Target**: Fix circular_lock_ordering (14) + db_connection_leak (20) HIGH severity gaps  

---

## Lock Ordering Strategy

### Lock Hierarchy (Enforced)
All locks in distributed_analytics.h must follow this **strict ordering**:

1. **Tier 1 (Outermost)**: `mutex_` (main registry lock)
   - Protects: `shards_` vector, shard topology, health monitor state
   - Held: Brief lock (acquire → read/modify → release)
   - Never held during: Network I/O, external calls, per-shard operations

2. **Tier 2 (Per-Shard, never held with Tier 1)**:
   - `circuit_breaker_mutex` (per-shard state)
   - `queue_mutex` (per-shard request queue)
   - Held: During per-shard fault detection, queue management
   - Never held with: `mutex_` (would cause circular lock)

### Circular Lock Scenarios (MUST BE FIXED)

**Pattern A - DEADLOCK RISK**:
```cpp
void funcA() {
    std::lock_guard<std::mutex> lock(mutex_);  // Tier 1
    // ... iterate shards, acquire per-shard locks
    std::lock_guard<std::mutex> cb_lock(entry.circuit_breaker_mutex);  // Tier 2
}

void funcB() {
    std::lock_guard<std::mutex> cb_lock(entry.circuit_breaker_mutex);  // Tier 2
    // ... then try to acquire Tier 1
    std::lock_guard<std::mutex> lock(mutex_);  // DEADLOCK!
}
```

**Solution**: 
- Never acquire Tier 1 lock while holding any Tier 2 lock
- If per-shard operations need registry info, snapshot it first under Tier 1
- Release Tier 1 before working on per-shard state

---

## Implementation Approach

### Phase 1: Documentation Comments (Low Risk)
Add comprehensive lock ordering documentation to each function that acquires locks:

```cpp
// LOCK ORDERING DOCUMENTATION:
// This function acquires locks in this order:
//   1. mutex_ (registry lock) — snapshot shard list
//   2. per-shard circuit_breaker_mutex — check/update circuit state
// NEVER acquire mutex_ while holding circuit_breaker_mutex (deadlock risk)
// NEVER hold both locks simultaneously in different call chains
```

### Phase 2: Lock Refactoring (Medium Risk)
Refactor functions that violate lock ordering:
- Split lock acquisition into distinct phases (snapshot → release → process)
- Use scoped guards with explicit unlock boundaries
- Extract per-shard operations into separate functions with clear ownership

### Phase 3: Connection Pool RAII (Low-Medium Risk)
Implement connection pooling with RAII to prevent leaks:
- Use `std::unique_ptr<Connection>` with custom deleter (return to pool)
- Wrap all connection acquisition in scoped guards
- Ensure exception safety: connections returned even if exception thrown

---

## Files Affected

### Tier 1: Must Fix First
- **distributed_analytics.cpp** (primary lock usage)
  - Functions: healthMonitorLoop(), executeDistributed(), 
    updateCircuitBreakerState(), queueRequest(), processQueue()
  - Action: Add lock ordering comments + refactor violation patterns

- **streaming_window.cpp** (connection pooling focus)
  - Functions: ingest(), flush(), idleTimeoutLoop()
  - Action: Implement connection RAII + add guards

### Tier 2: Verify Safe
- **jit_aggregation.cpp** (secondary)
  - Action: Audit lock usage; add documentation comments if present

---

## Acceptance Criteria

### Lock Ordering (14 gaps)
- [ ] All functions acquiring >1 lock have documented lock order in comments
- [ ] No function acquires Tier 1 while holding Tier 2
- [ ] No deadlock detection in CI-deadlock test suite
- [ ] Code review confirms circular dependencies eliminated

### Connection Leaks (20 gaps)
- [ ] All connection allocations wrapped in RAII guards
- [ ] Connection release occurs in normal + exception paths
- [ ] Valgrind/ASan shows no connection leaks (resource pooling tests)
- [ ] New test: test_analytics_resource_pooling_focused.cpp passes

---

## Commit Strategy

Commit in focused batches:

```bash
# Commit 1: Lock ordering documentation + comments
git commit -m "analytics: document lock ordering hierarchy [Batch A-1]

- Add LOCK_ORDERING comments to all mutex-acquiring functions
- Document Tier 1 (mutex_) vs Tier 2 (per-shard) lock levels
- Identify circular lock patterns for refactoring"

# Commit 2: Fix circular locks (refactor to avoid nesting)
git commit -m "analytics: fix circular_lock_ordering violations [Batch A-2]

- Refactor functions to snapshot Tier 1 state, release lock, then process Tier 2
- Ensure mutex_ released before per-shard operations
- Verify no Tier 2 acquired while holding Tier 1"

# Commit 3: Connection pooling RAII implementation
git commit -m "analytics: implement connection pool RAII pattern [Batch A-3]

- Use unique_ptr with custom deleter for connection lifecycle
- Wrap all pool.acquire() calls in scoped guards
- Ensure exception-safe resource cleanup"

# Commit 4: Tests & verification
git commit -m "analytics: add lock ordering + resource pool tests [Batch A-4]

- test_analytics_concurrency_safety_focused.cpp — lock order validation
- test_analytics_resource_pooling_focused.cpp — connection lifecycle
- Verify no deadlocks and no resource leaks"
```

---

## Implementation Checklist

### Documentation Phase
- [ ] Review distributed_analytics.cpp for all mutex operations
- [ ] Review streaming_window.cpp for all mutex operations
- [ ] Review jit_aggregation.cpp for all mutex operations
- [ ] Document lock tier for each mutex
- [ ] Identify lock ordering violations

### Refactoring Phase
- [ ] Fix each circular lock violation (snapshot → release → process pattern)
- [ ] Verify exception safety maintained
- [ ] Add RAII guards to connection operations

### Testing Phase
- [ ] Build: cmake --preset community-release --target themis_analytics
- [ ] Test: ctest --preset community-release -R AnalyticsFocusedTests
- [ ] Verify: No new compiler warnings (C++20 build)
- [ ] Verify: No deadlocks (if CI-deadlock available)

### Documentation Update
- [ ] Update ARCHITECTURE.md with concurrency section
- [ ] Add lock ordering diagram/table
- [ ] Document error taxonomy

---

## Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| Lock refactoring introduces deadlock | Code review + ThreadSanitizer testing |
| Exception path connection leaks | RAII guards + exception safety review |
| Performance regression | Profile before/after; target <5% slowdown |
| Incomplete coverage | Grep audit for all mutex usage patterns |

---

## Next Step

1. **Immediate**: Run audit to find all lock usage patterns
2. **Day 1**: Add documentation comments to all functions
3. **Day 2-3**: Implement lock refactoring fixes
4. **Day 4**: Implement RAII connection pooling
5. **Day 5**: Testing & verification

---

