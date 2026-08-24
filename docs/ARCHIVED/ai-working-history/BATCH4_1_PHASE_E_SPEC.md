# Phase E Specification — Smart Pointer & Exception Safety (R14-R15)

**Phase:** E (Smart Pointer Usage & Exception Safety)  
**Status:** ⏳ Queued for implementation after Phase D  
**Target Date:** Aug 19, 2026  
**Estimated Duration:** 2-3 hours  
**Agent:** themisdb-implementer  

---

## Executive Summary

Phase E addresses **2 CRITICAL gaps** related to smart pointer misuse and exception safety. These are safety issues that can cause:
- **Memory leaks** (objects not deleted when ownership unclear)
- **Use-after-free** (accessing deleted objects)
- **Double-delete** (multiple deletions of same object)
- **Exception safety violations** (resources not cleaned up during exception)

Each fix clarifies ownership semantics using proper smart pointer types and exception-safe patterns.

---

## Gap Details & Fixes

### R14: socket_timeout_manager.cpp:202 — Fix Smart Pointer Misuse

**File:** `src/network/socket_timeout_manager.cpp`  
**Location:** Line 202  
**Issue:** Smart pointer used incorrectly; unclear ownership semantics  
**Severity:** CRITICAL (potential memory leak or use-after-free)

**Current Code Pattern (Example):**
```cpp
// Ambiguous ownership — who owns the socket?
std::unique_ptr<Socket> socket = createSocket();
// Later: does this call delete? unclear intent
someFunction(socket.get());  // Passes raw pointer
```

**Fix Strategy:**
1. Identify ownership model at line 202
2. Choose correct smart pointer type:
   - `std::unique_ptr<T>` — exclusive ownership
   - `std::shared_ptr<T>` — shared ownership
3. If passing to function, pass by reference or use `std::move` semantics
4. Document ownership in function header/comments
5. Ensure exception-safe construction/destruction

**Implementation Guide:**
- Add: Clear comment documenting ownership model
- Use: `std::unique_ptr<SocketTimeoutManager>` if exclusive
- Or use: `std::shared_ptr<SocketTimeoutManager>` if shared
- Pass: By const reference (`const T&`) if only borrowing
- Or pass: By rvalue reference (`T&&`) if transferring ownership
- Document: "Acquires ownership of socket" or "Borrows socket reference"

**Acceptance Criteria:**
- [ ] Smart pointer type matches ownership semantics
- [ ] No raw pointers in public API
- [ ] Exception-safe construction/destruction
- [ ] AddressSanitizer: 0 memory leaks
- [ ] Use-after-free detection: 0 alerts
- [ ] Test: Ownership transfers work correctly

**Related Fixes:**
- R06 (socket_timeout_manager.cpp:71) — Destructor in same file; ensure cleanup works

---

### R15: wire_protocol_zero_copy.cpp:220 — Fix Exception Safety in Zero-Copy Path

**File:** `src/network/wire_protocol_zero_copy.cpp`  
**Location:** Line 220  
**Issue:** No exception safety guarantee in zero-copy buffer handling  
**Severity:** CRITICAL (exception in destructor or cleanup path)

**Current Code Pattern:**
```cpp
// Non-exception-safe cleanup — if exception during copy, buffer leaks
void* buffer = allocateBuffer(size);
memcpy(buffer, data, size);  // May throw
releaseBuffer(buffer);       // Never runs if exception above
```

**Fix Strategy:**
1. Wrap buffer allocation/deallocation in RAII class
2. Or use `std::unique_ptr` with custom deleter
3. Ensure destructor is `noexcept` (no exceptions during unwinding)
4. All cleanup happens even if exception thrown

**Implementation Guide:**
- Use: `std::unique_ptr<Buffer> buf(allocateBuffer(size));` (RAII)
- Or define: `class BufferGuard { ~BufferGuard() noexcept { cleanup(); } };`
- Document: "Exception-safe: buffer automatically released on scope exit"
- Test: Verify cleanup occurs even when exception thrown in loop

**Acceptance Criteria:**
- [ ] RAII wrapper used for buffer lifecycle
- [ ] Destructor marked `noexcept`
- [ ] Exception safety guarantee: strong or no-throw
- [ ] AddressSanitizer: 0 resource leaks on exception
- [ ] Test: Exception thrown during copy, buffer still cleaned up

**Related Fixes:**
- R11 (wire_protocol_zero_copy.cpp:160) — Timeout in same file; ensure timeout handles exception-safe cleanup
- R09 (wire_protocol_zero_copy.cpp:112) — Related zero-copy operations

---

## Implementation Workflow

### Step 1: Inspect Locations (20 min)

For each fix (R14, R15):
1. Open file at specified line
2. Identify smart pointer usage or resource allocation
3. Determine ownership model (exclusive vs shared)
4. Check for potential memory leaks or use-after-free

### Step 2: Fix Smart Pointer Usage (30 min)

**For R14:**
1. Identify current smart pointer type (if any)
2. Switch to `std::unique_ptr` or `std::shared_ptr` as appropriate
3. Remove raw `new`/`delete` calls
4. Use `std::make_unique` or `std::make_shared` where possible

**For R15:**
1. Identify resource allocation points
2. Wrap in `std::unique_ptr` with custom deleter (if needed)
3. Or create RAII wrapper class
4. Ensure no manual cleanup code (RAII handles it)

### Step 3: Add Documentation & Safety (20 min)

1. Add comment documenting ownership model
2. Ensure exception-safety guarantee stated
3. Mark destructors `noexcept` (if custom)
4. Document any custom deleters or RAII wrappers

### Step 4: Validation & Tests (30 min)

1. Compile: `cmake --preset windows-release && cmake --build --preset windows-release`
2. Tests: `ctest --preset windows-release -k network`
3. AddressSanitizer: Memory leak detection
4. Valgrind/Dr. Memory (optional): Deep inspection
5. Manual test: Force exception, verify cleanup

### Step 5: Create Commit & Report (10 min)

1. Stage changes: `git add src/network/*.cpp`
2. Commit: `git commit -m "Fix Batch 4.1 Phase E: Smart pointer and exception safety (R14-R15)"`
3. Create: `ai_working/BATCH4_1_PHASE_E_COMPLETION_REPORT.md`

---

## Quality Gates

### Per-Fix Validation

- [ ] Compilation succeeds
- [ ] No raw pointers in modified APIs
- [ ] Smart pointer type documented
- [ ] Exception-safe guarantee stated

### Memory Safety Validation

```bash
# After implementing all Phase E fixes:
cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=address"
cmake --build --preset windows-release
ctest --preset windows-release -k network --output-on-failure
```

**Expected:**
- AddressSanitizer: 0 memory leaks
- AddressSanitizer: 0 use-after-free alerts
- All tests PASS

### Exception Safety Test

Create simple test:
```cpp
// Test: Exception during operation, verify cleanup
try {
    auto resource = createResource();
    throwException();  // Should never reach below
} catch (...) {
    // Resource must be cleaned up
    // Verify no leaks reported by ASan
}
```

---

## Documentation for Phase E Completion

Create: `ai_working/BATCH4_1_PHASE_E_COMPLETION_REPORT.md`

**Contents:**
- Summary of 2 smart pointer/exception safety fixes (R14-R15)
- File locations and changes
- Ownership models used (unique_ptr, shared_ptr, or RAII)
- Exception safety guarantees stated
- Compilation results
- Test results (all PASS)
- AddressSanitizer results (0 leaks, 0 use-after-free)
- RAII/wrapper classes created (if any)
- Next phase readiness (Phase F)

---

## Timeline & Effort Estimate

| Step | Task | Duration | Resource |
|------|------|----------|----------|
| 1 | Inspect locations | 20 min | agent (code review) |
| 2 | Fix smart pointers | 30 min | agent (ownership clarity) |
| 3 | Documentation & safety | 20 min | agent (comments + noexcept) |
| 4 | Validation & tests | 30 min | agent (build + test) |
| 5 | Commit & report | 10 min | agent (git + doc) |
| **Total** | Phase E | **2-3 hrs** | themisdb-implementer |

---

## Risk Assessment

**Risk Level:** MEDIUM

**Risks & Mitigations:**
1. **ABI changes:** Smart pointer changes may affect binary compatibility; coordinate with release notes
2. **Performance:** shared_ptr has slight overhead; use unique_ptr when possible
3. **Interaction with Phase F:** Connection management may use shared_ptr; ensure consistency

---

**Document Status:** Ready for Phase E implementation  
**Approval Status:** Pending Phase D completion  
**Next:** Send Phase E instructions to agent after Phase D passes validation
