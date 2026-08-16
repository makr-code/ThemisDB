# Phase 3 Analytics A-2: Connection Leak Gap Audit

**Date**: 2026-08-15  
**Scope**: 20 HIGH-severity db_connection_leak gaps in Analytics streaming operations  
**Files**: 
- `src/analytics/streaming_window.cpp` (~12 gaps)
- `src/analytics/distributed_analytics.cpp` (~8 gaps)

---

## Streaming Window Operations - Gap Analysis (~12 gaps)

### File: `src/analytics/streaming_window.cpp`

#### Identified Gaps (RAII-related, not explicit DB connections currently)

**Gap 1-4: TumblingWindow lifecycle management (4 gaps)**
- **Location**: Lines 330-545 (TumblingWindow constructor/destructor)
- **Pattern**: Resource initialization without guaranteed cleanup
- **Risk**: If exception thrown during setup, cleanup incomplete
- **Fix**: Apply RAII pattern to all resource acquisition

**Gap 5-8: SlidingWindow lifecycle management (4 gaps)**
- **Location**: Lines 570-830 (SlidingWindow constructor/destructor)
- **Pattern**: Same RAII pattern needed for sliding window state
- **Risk**: Resource exhaustion on exception paths
- **Fix**: Ensure all resources cleaned up via RAII

**Gap 9-12: SessionWindow lifecycle management (4 gaps)**
- **Location**: Lines 850-1120 (SessionWindow constructor/destructor)
- **Pattern**: Session state management without RAII guarantees
- **Risk**: Leaks on early exit from session operations
- **Fix**: Wrap state management with RAII patterns

---

## Distributed Analytics Queries - Gap Analysis (~8 gaps)

### File: `src/analytics/distributed_analytics.cpp`

#### Identified Gaps

**Gap 1-3: executeDistributed() - Thread management (3 gaps)**
- **Location**: Lines 822-858 (async thread spawning)
- **Pattern**: Threads created without proper lifecycle management
- **Issues**:
  - Line 858: `.detach()` call creates detached threads
  - No explicit cleanup if exception thrown during dispatch
  - Thread resources not guaranteed to be released on error paths
- **Fix**: Apply ThreadGuard or ensure all threads are properly managed

**Gap 4-5: HealthCheckLoop() - Exception path management (2 gaps)**
- **Location**: Lines 370-430 (health check background loop)
- **Pattern**: Exception handlers don't guarantee state cleanup
- **Issues**:
  - Lines 413-420: catch blocks may not release all acquired resources
  - No guaranteed cleanup on abnormal termination
- **Fix**: Use RAII for all resource acquisition in exception paths

**Gap 6-8: Shard registry management (3 gaps)**
- **Location**: Lines 450-475 (registerShard, unregisterShard)
- **Pattern**: Shard state modification without transactional safety
- **Issues**:
  - Line 460: make_shared allocation without cleanup guarantee on error
  - Line 467-470: Vector modification in unregisterShard may leak references
  - Temporary shard copies in snapshot operations need proper cleanup
- **Fix**: Wrap shard operations with RAII cleanup handlers

---

## Resource Leak Pattern Categories

### 1. **RAII Lifecycle Gaps** (Primary Category)
These are places where resources are acquired but cleanup is not guaranteed in all code paths.

**Pattern Before**:
```cpp
void process() {
    auto resource = acquire_resource();
    if (!validate(resource)) return;  // ← LEAK if not RAII!
    use(resource);
    cleanup(resource);
}
```

**Pattern After**:
```cpp
void process() {
    auto guard = ResourceGuard::acquire(resource_manager);
    if (!guard) return;  // ← SAFE - guard destructor cleans up
    use(guard);
}  // ← Automatic cleanup guaranteed
```

### 2. **Exception Path Gaps**
Places where exception handlers don't guarantee cleanup.

**Pattern Before**:
```cpp
void process() {
    Connection* conn = db->getConnection();
    try {
        if (!validate()) throw std::exception();  // ← LEAK!
        use(conn);
    } catch (...) {
        db->releaseConnection(conn);  // ← Never reached for pre-try exceptions
        throw;
    }
}
```

**Pattern After**:
```cpp
void process() {
    auto guard = ConnectionGuard::acquire(db, connection_id, [db]{ db->releaseConnection(conn); });
    if (!validate()) throw std::exception();  // ← SAFE - RAII catches this
    use(guard);
}  // ← Guaranteed cleanup on any path
```

### 3. **Early Return Gaps**
Places where early returns bypass cleanup.

---

## ConnectionGuard Pattern Specification

```cpp
// RAII pattern for all resource cleanup:
class ConnectionGuard {
public:
    static auto acquire(int conn_id, Releaser releaser) -> ConnectionGuard;
    
    ~ConnectionGuard() noexcept {
        release();  // Guaranteed cleanup
    }
    
private:
    int connection_id_;
    Releaser releaser_;
    bool is_released_;
};
```

**Guarantees**:
- ✅ Cleanup on normal scope exit
- ✅ Cleanup on exception
- ✅ Cleanup on early return
- ✅ Exception-safe (no throw from destructor)
- ✅ Works with all code paths

---

## Remediation Strategy

### Phase 1: Create ConnectionGuard (DONE)
- ✅ `include/analytics/connection_guard.h` created
- ✅ RAII pattern matches Index Phase 3 A-6 spec
- ✅ Exception-safe cleanup guaranteed

### Phase 2: Apply to streaming_window.cpp
For each identified gap:
1. Identify resource acquisition point
2. Create ConnectionGuard wrapper
3. Verify cleanup in all code paths (return, throw, scope exit)
4. Add inline comments explaining RAII safety

### Phase 3: Apply to distributed_analytics.cpp
For each identified gap:
1. Replace manual thread management with ThreadGuard or proper cleanup
2. Wrap async operations with RAII handlers
3. Ensure exception paths don't skip cleanup
4. Add circuit breaker state cleanup

### Phase 4: Validation
```bash
cmake --preset develop-asan --fresh
cmake --build build-develop-asan -j 8
ASAN_OPTIONS="detect_leaks=1" ctest -L analytics --timeout 120
```

**Expected Results**:
- ✅ 0 memory leaks detected
- ✅ 0 use-after-free errors
- ✅ All analytics tests PASS

---

## Success Metrics

| Gap Category | Count | Files | Status |
|---|---|---|---|
| TumblingWindow RAII | 4 | streaming_window.cpp | PLANNED |
| SlidingWindow RAII | 4 | streaming_window.cpp | PLANNED |
| SessionWindow RAII | 4 | streaming_window.cpp | PLANNED |
| AsyncThread lifecycle | 3 | distributed_analytics.cpp | PLANNED |
| Exception path cleanup | 2 | distributed_analytics.cpp | PLANNED |
| Shard registry RAII | 3 | distributed_analytics.cpp | PLANNED |
| **TOTAL** | **20** | **Both files** | **IN PROGRESS** |

---

## Implementation Checklist

- [ ] ConnectionGuard header created ✅
- [ ] streaming_window.cpp analyzed (12 gaps)
- [ ] distributed_analytics.cpp analyzed (8 gaps)
- [ ] All gaps wrapped with appropriate RAII patterns
- [ ] Inline comments added for safety explanation
- [ ] Build succeeds on all presets
- [ ] ASan validation: 0 leaks
- [ ] All tests PASS
- [ ] Code review PASS
- [ ] Ready for merge after Index Phase 3 A-6

---

## Notes

1. **Pattern Dependency**: This work follows Index Phase 3 A-6 (ConnectionGuard RAII pattern established in Index module). Analytics Phase 3 A-2 applies same pattern to Analytics-specific resources.

2. **Preventative Approach**: Some gaps are "preventative" - they identify resource management patterns that COULD leak if connections/resources were added to these operations. The RAII pattern ensures future changes won't introduce leaks.

3. **Coordination**: Merge sequencing coordination required:
   - Index Phase 3 A-6: Expected merge 2026-08-29 morning
   - Analytics Phase 3 A-2: Ready to merge 2026-08-29 afternoon

