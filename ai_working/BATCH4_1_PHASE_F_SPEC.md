# Phase F Specification — Connection Leak Prevention (R17-R19)

**Phase:** F (Connection Lifecycle & Resource Cleanup)  
**Status:** ⏳ Queued for implementation after Phase E  
**Target Date:** Aug 20, 2026  
**Estimated Duration:** 3-4 hours  
**Agent:** themisdb-implementer  

---

## Executive Summary

Phase F addresses the final **3 CRITICAL gaps** related to connection leaks and resource management. These are operational issues that can cause:
- **Connection exhaustion** (max connection limit reached)
- **File descriptor leaks** (system FD limit hit)
- **Memory exhaustion** (accumulated connection objects)
- **Production outage** (unable to accept new connections)

Each fix ensures connections are properly closed/cleaned up in all paths (success, error, cancellation).

---

## Gap Details & Fixes

### R17: raft_load_balancer.cpp:288 — Close Connection on Load Balancer Failure

**File:** `src/network/raft_load_balancer.cpp`  
**Location:** Line 288  
**Issue:** Connection not closed when load balancer operation fails  
**Severity:** CRITICAL (connection leak on error)

**Current Code Pattern (Example):**
```cpp
// Missing cleanup on error — connection hangs open
auto conn = connectToNode(endpoint);
if (!conn.isHealthy()) {
    return error;  // ← Connection leaked here!
}
```

**Fix Strategy:**
1. Identify all error paths at line 288
2. Add explicit close/cleanup on each error path
3. Or use RAII guard to auto-close on scope exit
4. Ensure single close (idempotent)
5. Log connection cleanup for diagnostics

**Implementation Guide:**
- Use: `ConnectionGuard guard(conn);` (RAII pattern)
- Or add: `conn.close()` before each `return error;`
- Or use: `std::unique_ptr<Connection>` with custom deleter
- Document: "Connections automatically closed on error"
- Log: "Connection closed: node={}, reason={}" (debug level)

**Acceptance Criteria:**
- [ ] All error paths close connection
- [ ] Connection close is idempotent (safe to call multiple times)
- [ ] AddressSanitizer: 0 resource leaks
- [ ] File descriptor count: 0 growth after error
- [ ] Test: Verify connection closed on each error condition

**Related Fixes:**
- R18, R19 (raft_load_balancer.cpp:290, other locations) — Connection leaks in same file
- R03 (raft_load_balancer.cpp formatting) — Already done in Phase A

---

### R18: raft_load_balancer.cpp:290 — Close Connection on Rebalance Timeout

**File:** `src/network/raft_load_balancer.cpp`  
**Location:** Line 290  
**Issue:** Connection not closed when rebalancing operation times out  
**Severity:** CRITICAL (connection leak on timeout)

**Current Code Pattern:**
```cpp
// Missing cleanup on timeout — connection hangs open
auto deadline = now() + timeout;
while (now() < deadline) {
    if (rebalance_complete) break;
}
return result;  // ← Connection may still be open
```

**Fix Strategy:**
1. Add explicit connection close on timeout/exit
2. Ensure close happens even if loop breaks normally
3. Use RAII guard to guarantee cleanup
4. Verify close works with timeout-induced cancellation

**Implementation Guide:**
- Use: `std::unique_ptr<Connection, decltype(&closeConnection)> guard(conn, &closeConnection);`
- Or: `RAII_Connection guard(conn);` (custom wrapper)
- Or add: `conn.close();` before final return
- Coordinate: With Phase C timeout fixes to ensure cleanup
- Log: "Rebalance connection closed: success={}, timeout={}" (debug)

**Acceptance Criteria:**
- [ ] Connection closed on all exits (success, timeout, error)
- [ ] RAII guard used (or explicit cleanup guaranteed)
- [ ] AddressSanitizer: 0 resource leaks
- [ ] Test: Timeout scenario, verify connection closed

**Related Fixes:**
- R17 (raft_load_balancer.cpp:288) — Connection leak in same file
- R19 (raft_load_balancer.cpp) — May be other leak location in same file
- R10 (service_mesh.cpp:243) — Timeout in Phase C; coordinate patterns

---

### R19: wire_protocol_zero_copy.cpp → Connection Leak Location [TBD]

**File:** `src/network/wire_protocol_zero_copy.cpp` or other  
**Location:** [Based on gap analysis, likely line in 100-230 range]  
**Issue:** Connection not properly closed/released in zero-copy path  
**Severity:** CRITICAL (connection leak)

**Current Code Pattern:**
```cpp
// Missing cleanup in zero-copy buffer lifecycle
auto buffer = acquireBuffer();
// ... processing ...
// Missing: releaseBuffer() or connection.close()
```

**Fix Strategy:**
1. Identify specific line from phase tracking (Phase A/B already touched file)
2. Locate resource acquisition/release pair
3. Ensure all exit paths release resource
4. Add explicit close on error/cancellation
5. Use RAII if manual cleanup incomplete

**Acceptance Criteria:**
- [ ] Resource release guaranteed on all paths
- [ ] RAII pattern used or explicit cleanup present
- [ ] AddressSanitizer: 0 resource leaks
- [ ] Test: Verify resource released in success/error/timeout paths

---

## Implementation Workflow

### Step 1: Inspect Locations (30 min)

For each fix (R17-R19):
1. Open file at specified line
2. Identify resource lifecycle (acquire → use → release)
3. Trace all exit paths (success, error, exception, timeout)
4. Check if resource released on each path

### Step 2: Add Resource Cleanup (45 min)

1. Use RAII guard if available (preferred)
2. Or add explicit cleanup at each exit
3. Verify cleanup is idempotent
4. Use consistent cleanup pattern across fixes

### Step 3: Test Error Paths (30 min)

1. Identify error conditions (network failure, timeout, invalid input)
2. Add test cases for each path
3. Verify resource released even on error
4. Add stress test: rapid connect/disconnect

### Step 4: Validation & Tests (30 min)

1. Compile: `cmake --preset windows-release && cmake --build --preset windows-release`
2. Tests: `ctest --preset windows-release -k network`
3. AddressSanitizer: Resource leak detection
4. Stress test: 1000 connections opened/closed
5. Verify: FD count stable, no growth

### Step 5: Create Commit & Report (15 min)

1. Stage changes: `git add src/network/*.cpp`
2. Commit: `git commit -m "Fix Batch 4.1 Phase F: Connection leak prevention (R17-R19)"`
3. Create: `ai_working/BATCH4_1_PHASE_F_COMPLETION_REPORT.md`

---

## Quality Gates

### Per-Fix Validation

- [ ] Compilation succeeds
- [ ] All exit paths cleanup resources
- [ ] RAII or explicit cleanup consistent
- [ ] No double-close or use-after-close

### Resource Leak Detection

```bash
# After implementing all Phase F fixes:
cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=address"
cmake --build --preset windows-release
ctest --preset windows-release -k network --output-on-failure
```

**Expected:**
- AddressSanitizer: 0 resource leaks
- File descriptor count: Stable (< 1% growth on 1000 iterations)
- All tests PASS

### Stress Test

```bash
# Rapid connect/disconnect to verify resource cleanup
for i in {1..1000}; do
    # Create connection
    # Use connection
    # Close connection
    # Verify cleanup
done
# Verify: File descriptor count returns to baseline
```

---

## Documentation for Phase F Completion

Create: `ai_working/BATCH4_1_PHASE_F_COMPLETION_REPORT.md`

**Contents:**
- Summary of 3 connection leak fixes (R17-R19)
- File locations and changes
- Resource lifecycle patterns used
- RAII guards or cleanup mechanisms created
- Compilation results
- Test results (all PASS)
- AddressSanitizer results (0 leaks)
- Stress test results (FD count stable)
- Batch 4.1 overall completion summary
- Readiness for final validation

---

## Risk Assessment

**Risk Level:** LOW-MEDIUM

**Risks & Mitigations:**
1. **Performance:** Connection close may block; verify timeout/async close works
2. **Interaction with Phase C timeouts:** Ensure timeout triggers close properly
3. **Backward compatibility:** Resource management changes shouldn't break API
4. **Batching with Phase E:** Both use RAII; ensure consistent patterns

---

## Timeline & Effort Estimate

| Step | Task | Duration | Resource |
|------|------|----------|----------|
| 1 | Inspect locations | 30 min | agent (code review) |
| 2 | Add resource cleanup | 45 min | agent (RAII + explicit) |
| 3 | Test error paths | 30 min | agent (test cases) |
| 4 | Validation & tests | 30 min | agent (build + test + stress) |
| 5 | Commit & report | 15 min | agent (git + doc) |
| **Total** | Phase F | **3-4 hrs** | themisdb-implementer |

**Start:** After Phase E completion (estimated Aug 19-20, 4:00 PM UTC)  
**End:** Aug 20, 7-8 PM UTC (with contingency)

---

## Final Batch 4.1 Validation (After Phase F)

Once all 6 phases (A-F) complete:

1. **Full Compilation**
   ```bash
   rm -rf build
   cmake --preset windows-release
   cmake --build --preset windows-release --parallel 16
   ```
   **Expected:** No errors, no warnings

2. **Full Network Test Suite**
   ```bash
   ctest --preset windows-release -k network -j 4 --output-on-failure
   ```
   **Expected:** All tests PASS (100% pass rate)

3. **Full Sanitizer Suite**
   ```bash
   # ASan
   cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=address"
   cmake --build --preset windows-release
   ctest --preset windows-release -k network --output-on-failure
   # Result: 0 leaks, 0 overflows
   
   # UBSan
   cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=undefined"
   cmake --build --preset windows-release
   ctest --preset windows-release -k network --output-on-failure
   # Result: 0 undefined behavior alerts
   
   # TSan (if available)
   cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=thread"
   cmake --build --preset windows-release
   ctest --preset windows-release -k network --output-on-failure
   # Result: 0 data races, 0 deadlocks
   ```

4. **Release CI Validation**
   ```bash
   # Trigger: release_critical workflow on PR
   # Expected: All jobs PASS, no regressions
   ```

5. **Performance Baseline**
   - Compare throughput/latency vs Phase A baseline
   - Expected: ≥ baseline (no regression)
   - R16 (timeout in perf path): < 1% overhead

---

## Batch 4.1 Completion Criteria

✅ **Batch 4.1 is COMPLETE when:**

1. All 19 CRITICAL fixes implemented (R01-R19)
2. All 6 phases executed and documented
3. All tests PASS (100% pass rate)
4. ASan: 0 leaks, 0 overflows
5. UBSan: 0 undefined behavior
6. TSan: 0 data races, 0 deadlocks
7. Performance: No regression vs baseline
8. Release CI: All workflows GREEN
9. Comprehensive completion report created
10. Network module ready for GA promotion

---

**Document Status:** Ready for Phase F implementation  
**Approval Status:** Pending Phase E completion  
**Next:** Send Phase F instructions to agent after Phase E passes validation
