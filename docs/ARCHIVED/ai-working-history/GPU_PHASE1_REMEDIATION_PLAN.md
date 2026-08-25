# GPU Module Phase 1 Remediation Plan

## Date: 2026-08-17
## Priority: P0 Critical

### Overview
Remediate critical gaps in three GPU module files:
1. **src/gpu/query_accelerator.cpp** - Unchecked CUDA calls, resource leaks
2. **src/gpu/gpu_memory_manager_edition.cpp** - Resource lifecycle management
3. **src/gpu/unified_memory.cpp** - Error handling, RAII contract violations

---

## File 1: src/gpu/unified_memory.cpp

### Issue 1.1: platformFree Error Handling (Line 41-49)
**Severity**: HIGH (CWE-252: Unchecked error return)
**Location**: Lines 41-49, 149, 317

**Problem**:
- platformFree returns bool but callers don't consistently check it
- Line 149: if (!platformFree(ptr)) { return false; } ✓ (good)
- Line 317: static_cast<void>(platformFree(rec.ptr)); ✗ (ignores error)

**Fix**:
- Add diagnostic emission when platformFree fails
- Ensure all callers handle errors appropriately
- Use GPUBackendDispatchDiagnostics::emitDiagnostic for failure cases

---

## File 2: src/gpu/query_accelerator.cpp

### Issue 2.1: Redundant CUDA Operations (Lines 1147, 1164, 1181-1182)
**Severity**: MEDIUM (redundant work, potential double-copy)
**Location**: IVF-Flat search section

**Problem**:
- Both CHECKED_CUDA(cudaMemcpy(...)) AND raft::copy(...) called for same data
- Line 1147: cudaMemcpy THEN raft::copy (db_dev)
- Line 1164: cudaMemcpy THEN raft::copy (q_dev)
- Lines 1181-1182: cudaMemcpy called, then raft::copy for results

**Fix**:
- Remove redundant raft::copy calls where CHECKED_CUDA(cudaMemcpy) already did the work
- Keep only one path per data transfer
- Ensure exception safety remains intact

### Issue 2.2: Exception-Unsafe Resource Cleanup (Lines 1137-1199)
**Severity**: HIGH (resource leak on exception)
**Location**: GPU path try-catch block

**Problem**:
- raft::device_resources handle created on line 1141
- Exception on any operation after handle creation leaves handle un-destructed
- No explicit cleanup in catch block

**Fix**:
- Rely on RAII: raft::device_resources destructor should clean up
- Add exception-safety verification (ensure no resources leak on throw)
- Document RAII contract in comments

---

## File 3: src/gpu/gpu_memory_manager_edition.cpp

### Issue 3.1: Resource Lifecycle in Allocation Tracking (Lines 29-68)
**Severity**: MEDIUM (potential vector corruption on exception)
**Location**: TryAllocateUnderLock function

**Problem**:
- Line 56: active_allocations_.push_back() can throw
- If push_back fails, internal state is inconsistent (counters incremented but allocation not tracked)

**Fix**:
- Push to vector BEFORE incrementing counters, or
- Use transaction pattern: save old state, push, then commit counters

---

## Implementation Strategy

### Phase 1A: Fix unified_memory.cpp (highest risk)
1. Add diagnostic emission for platformFree failures
2. Update all callers to handle errors
3. Test with memory cleanup scenario

### Phase 1B: Fix query_accelerator.cpp
1. Remove redundant raft::copy calls
2. Verify exception safety with RAII analysis
3. Test with exception injection

### Phase 1C: Harden gpu_memory_manager_edition.cpp
1. Fix allocation tracking order
2. Add exception-safety tests
3. Verify no leaks under stress

### Testing
- Unit tests for each module
- Exception injection tests
- Memory leak detection (valgrind/asan)
- SLA compliance verification

---

## Success Criteria
✓ All use-after-free patterns eliminated
✓ All CUDA calls have error checking
✓ All resource leaks in exception paths closed
✓ No nullptr dereferences
✓ Public API unchanged
✓ All GPU tests pass
