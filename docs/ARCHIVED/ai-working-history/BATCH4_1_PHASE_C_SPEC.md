# Phase C Specification — Timeout Enforcement (R09-R11, R16)

**Phase:** C (Timeout Enforcement & Deadlock Prevention)  
**Status:** ⏳ Queued for implementation after Phase B  
**Target Date:** Aug 18, 2026  
**Estimated Duration:** 3-4 hours  
**Agent:** themisdb-implementer  

---

## Executive Summary

Phase C addresses **4 CRITICAL gaps** related to missing or incomplete timeout operations. These are high-risk items that can cause:
- **Deadlocks** (operations hang indefinitely)
- **Resource exhaustion** (connections accumulate)
- **Production incidents** (customer-facing impact)

Each fix adds explicit timeout enforcement with proper cancellation/error handling.

---

## Gap Details & Fixes

### R09: wire_protocol_zero_copy.cpp:112 — Add Timeout to I/O Operation

**File:** `src/network/wire_protocol_zero_copy.cpp`  
**Location:** Line 112  
**Issue:** No timeout on I/O operation (zerocopy read/write)  
**Severity:** CRITICAL (deadlock risk)

**Current Code Pattern (Example):**
```cpp
// Missing timeout — will block forever on slow/hung connection
auto result = readZeroCopy(buffer, size);  // ← NO TIMEOUT
```

**Fix Strategy:**
1. Add timeout parameter to readZeroCopy operation
2. Implement cancellation token or std::chrono::duration parameter
3. Return error (timeout_error) if operation exceeds limit
4. Log timeout occurrence (diagnostic)

**Implementation Guide:**
- Use project's timeout pattern from `src/network/socket_timeout_manager.hpp` (if available)
- Or add: `std::optional<TimeoutDuration> timeout = std::chrono::milliseconds(5000);`
- Return: `TimeoutError` on exceeded duration
- Test: Verify timeout triggers at correct interval

**Acceptance Criteria:**
- [ ] Timeout parameter added to readZeroCopy signature
- [ ] Timeout enforced (operation stops after N ms)
- [ ] Error handling: TimeoutError returned cleanly
- [ ] No resource leaks on timeout
- [ ] ThreadSanitizer: 0 deadlocks
- [ ] Test: Timeout triggers within ±100ms of expected value

**Related Fixes:**
- R10 (service_mesh.cpp:243) — Also timeout enforcement
- R04 (wire_protocol_performance.cpp:232) — Related wire protocol timeout

---

### R10: service_mesh.cpp:243 — Add Timeout to Service Mesh Connection

**File:** `src/network/service_mesh.cpp`  
**Location:** Line 243  
**Issue:** No timeout on mesh connection establishment/operation  
**Severity:** CRITICAL (deadlock risk)

**Current Code Pattern:**
```cpp
// Missing timeout — will block forever on unresponsive mesh node
auto connection = connectToServiceMesh(endpoint);  // ← NO TIMEOUT
```

**Fix Strategy:**
1. Add timeout to connectToServiceMesh or related operation
2. Implement exponential backoff with timeout ceiling
3. Return connection_timeout error if exceeded
4. Emit diagnostic event (optional, for observability)

**Implementation Guide:**
- Add parameter: `std::chrono::milliseconds connect_timeout = std::chrono::milliseconds(10000);`
- Use `std::condition_variable::wait_for()` or similar timeout mechanism
- Return error on timeout
- Log: "Service mesh connection timeout: endpoint={}, timeout_ms={}" (debug level)

**Acceptance Criteria:**
- [ ] Timeout parameter added
- [ ] Timeout enforced on connection
- [ ] Error handling: connection_timeout error on exceed
- [ ] No dangling connections on timeout
- [ ] ThreadSanitizer: 0 deadlocks
- [ ] Test: Timeout triggers and connection cleaned up

**Related Fixes:**
- R07, R08 (service_mesh.cpp:175, 194) — Destructors in same file; ensure cleanup on timeout
- R09 (wire_protocol_zero_copy.cpp:112) — Similar timeout pattern

---

### R11: wire_protocol_zero_copy.cpp:160 — Add Timeout to Write Operation

**File:** `src/network/wire_protocol_zero_copy.cpp`  
**Location:** Line 160  
**Issue:** No timeout on zerocopy write operation  
**Severity:** CRITICAL (deadlock risk)

**Current Code Pattern:**
```cpp
// Missing timeout — will block forever on full/stuck send buffer
auto result = writeZeroCopy(data, size);  // ← NO TIMEOUT
```

**Fix Strategy:**
1. Add timeout to writeZeroCopy operation (complementary to R09 readZeroCopy)
2. Use same timeout pattern as R09 for consistency
3. Return timeout_error if send buffer never drains
4. Optional: Implement send buffer pressure backpressure

**Implementation Guide:**
- Use consistent timeout signature with R09
- Add: `std::chrono::milliseconds write_timeout = std::chrono::milliseconds(5000);`
- Return: `TimeoutError` on exceeded duration
- Coordinate with R09 so both read/write timeouts match

**Acceptance Criteria:**
- [ ] Timeout parameter added to writeZeroCopy
- [ ] Timeout enforced (write stops after N ms)
- [ ] Error handling: TimeoutError returned
- [ ] Send buffer state valid after timeout (not corrupted)
- [ ] ThreadSanitizer: 0 deadlocks
- [ ] Test: Timeout triggers, buffer integrity maintained

**Related Fixes:**
- R09 (wire_protocol_zero_copy.cpp:112) — Read timeout in same file; coordinate style/timeout values
- R04 (wire_protocol_performance.cpp:232) — Wire protocol timeout pattern reference

---

### R16: wire_protocol_performance.cpp:232 — Enforcement Timeout in Performance Path

**File:** `src/network/wire_protocol_performance.cpp`  
**Location:** Line 232  
**Issue:** No timeout on performance-critical wire protocol operation  
**Severity:** CRITICAL (deadlock risk in high-throughput path)

**Current Code Pattern:**
```cpp
// Missing timeout in hot path — can hang entire protocol processing
auto packet = processHighThroughputPacket(data);  // ← NO TIMEOUT
```

**Fix Strategy:**
1. Add timeout to high-throughput operation
2. **Must not add significant overhead** (performance-critical path)
3. Use fast timeout check (e.g., `std::chrono::steady_clock` elapsed)
4. Fall back to default timeout if processing exceeds budget

**Implementation Guide:**
- Use lightweight timeout mechanism (not mutex/CV if possible)
- Add: `std::chrono::microseconds perf_budget = std::chrono::microseconds(100);` (example)
- Check: `if (elapsed > perf_budget) return PerformanceTimeoutError;`
- Log timeout occurrence at debug level only (don't spam hot path)

**Acceptance Criteria:**
- [ ] Timeout parameter added
- [ ] Timeout enforced (operation stops after budget)
- [ ] Error handling: PerformanceTimeoutError returned
- [ ] **Performance impact: < 1% overhead** (critical gate)
- [ ] ThreadSanitizer: 0 deadlocks
- [ ] Test: Timeout triggers without degrading throughput

**Related Fixes:**
- R04 (wire_protocol_performance.cpp) — Same file; coordinate timeout values
- R09, R11 (wire_protocol_zero_copy.cpp) — Related protocol patterns

---

## Implementation Workflow

### Step 1: Inspect Locations (30 min)

For each fix (R09, R10, R11, R16):
1. Open file at specified line
2. Identify operation that needs timeout
3. Review existing timeout patterns in codebase
4. Check error handling contract

### Step 2: Add Timeout Parameters (45 min)

1. Add timeout parameter to function signature (or wrapper call)
2. Use project's existing timeout type/pattern if available
3. Document timeout semantics in function comments
4. Use consistent timeout values across related fixes (e.g., R09 & R11 in same file)

### Step 3: Implement Timeout Logic (60 min)

1. Add deadline calculation (start_time + timeout)
2. Add timeout check in operation loop (if applicable)
3. Implement cancellation/error return on timeout
4. Ensure no resource leaks on timeout exit

### Step 4: Error Handling (30 min)

1. Add TimeoutError type to error enum (if not present)
2. Implement error conversion/propagation
3. Add diagnostic logging (debug level)
4. Document timeout behavior in API comments

### Step 5: Validation & Tests (60 min)

1. Compile: `cmake --preset windows-release && cmake --build --preset windows-release`
2. Unit tests: `ctest --preset windows-release -k network`
3. ThreadSanitizer: Run tests with TSan enabled
4. Verify: Timeout triggers at correct time, no deadlocks

### Step 6: Create Commit & Report (15 min)

1. Stage changes: `git add src/network/wire_protocol_*.cpp src/network/service_mesh.cpp`
2. Commit: `git commit -m "Fix Batch 4.1 Phase C: Timeout enforcement (R09-R11, R16)"`
3. Create: `ai_working/BATCH4_1_PHASE_C_COMPLETION_REPORT.md`

---

## Quality Gates

### Per-Fix Validation

- [ ] Compilation succeeds (no syntax errors)
- [ ] No new warnings
- [ ] Timeout parameter documented
- [ ] Error handling complete

### Test Validation

```bash
# After implementing all Phase C fixes:
cmake --preset windows-release
cmake --build --preset windows-release --parallel 8
ctest --preset windows-release -k network --output-on-failure -j 4
```

**Expected:** All tests PASS

### ThreadSanitizer Validation

```bash
# Verify 0 deadlocks detected:
cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=thread"
cmake --build --preset windows-release
ctest --preset windows-release -k network
```

**Expected:** 0 ThreadSanitizer warnings/deadlock reports

### Performance Validation (Critical for R16)

For R16 (wire_protocol_performance.cpp:232), verify:
- [ ] Throughput: ≥ baseline (no regression)
- [ ] Latency: ≤ baseline + 1% (< 1% overhead)
- [ ] Timeout: Triggers within ±100ms of target

---

## Documentation for Phase C Completion

Create: `ai_working/BATCH4_1_PHASE_C_COMPLETION_REPORT.md`

**Contents:**
- Summary of 4 timeout fixes (R09-R11, R16)
- File locations and changes
- Timeout values used (standardization check)
- Compilation results
- Test results (all PASS)
- ThreadSanitizer results (0 deadlocks)
- Performance impact analysis (especially R16)
- Next phase readiness (Phase D)

---

## Risk Assessment

**Risk Level:** MEDIUM

**Risks & Mitigations:**
1. **Incorrect timeout values:** Mitigate with configurable defaults + documentation
2. **Performance overhead (R16):** Mitigate with lightweight timeout checks + benchmarking
3. **Resource cleanup on timeout:** Mitigate with RAII + test coverage
4. **Interaction with Phase B destructors:** Ensure R07/R08 destructors clean up timeout resources

---

## Dependency Notes

- **Phase B Prerequisite:** R07, R08 (service_mesh.cpp destructors) must be complete
- **Related Phase D:** R04 is already in this file (wire_protocol_performance.cpp:232) — may need coordination
- **Rollback:** If timeout causes deadlock, disable temporarily and escalate to maintainer

---

## Timeline & Effort Estimate

| Step | Task | Duration | Resource |
|------|------|----------|----------|
| 1 | Inspect locations | 30 min | agent (direct code review) |
| 2 | Add timeout parameters | 45 min | agent (signature edits) |
| 3 | Implement timeout logic | 60 min | agent (control flow edits) |
| 4 | Error handling | 30 min | agent (error paths) |
| 5 | Validation & tests | 60 min | agent (build + test runner) |
| 6 | Commit & report | 15 min | agent (git + doc) |
| **Total** | Phase C | **3-4 hrs** | themisdb-implementer |

**Start:** After Phase B completion (estimated Aug 18, 10:00 AM UTC)  
**End:** Aug 18, 1-2 PM UTC (with contingency)

---

**Document Status:** Ready for Phase C implementation  
**Approval Status:** Pending Phase B completion  
**Next:** Send Phase C instructions to agent after Phase B passes validation
