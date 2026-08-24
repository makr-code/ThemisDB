# Phase D Specification — Memcpy Bounds Validation (R12-R13)

**Phase:** D (Memcpy & Buffer Overflow Prevention)  
**Status:** ⏳ Queued for implementation after Phase C  
**Target Date:** Aug 19, 2026  
**Estimated Duration:** 2-3 hours  
**Agent:** themisdb-implementer  

---

## Executive Summary

Phase D addresses **2 CRITICAL gaps** related to unchecked `memcpy` operations. These are high-risk security/safety issues that can cause:
- **Buffer overflow** (write beyond allocated buffer)
- **Memory corruption** (overwrite adjacent objects)
- **Security vulnerability** (potential code execution)
- **Undefined behavior** (crash or exploit)

Each fix adds bounds checking before `memcpy` with proper error handling.

---

## Gap Details & Fixes

### R12: connection_compression.cpp:114 — Add Bounds Check to Memcpy

**File:** `src/network/connection_compression.cpp`  
**Location:** Line 114  
**Issue:** Unchecked `memcpy` operation without destination buffer size validation  
**Severity:** CRITICAL (buffer overflow risk)

**Current Code Pattern (Example):**
```cpp
// Missing bounds check — may write beyond buffer!
char buffer[256];
memcpy(buffer, source_data, source_len);  // ← If source_len > 256, OVERFLOW
```

**Fix Strategy:**
1. Identify source and destination buffer sizes
2. Add bounds check before memcpy: `if (source_len > dest_size) return error;`
3. Use `memcpy_s` (MSVC) or safe memcpy wrapper if available
4. Return error code on size mismatch
5. Log warning (debug level) if truncation occurs

**Implementation Guide:**
- Add: `if (data_len > buffer_size) { /* error handling */ }`
- Use: `std::memcpy` (standard) with preceding bounds check
- Or use: `memcpy_s` if platform supports it (MSVC 14.0+)
- Return: `BufferOverflowError` or `TruncationError`
- Document: "Copies min(data_len, buffer_size) bytes to prevent overflow"

**Acceptance Criteria:**
- [ ] Bounds check added before memcpy
- [ ] Error handling: Returns error if source > destination
- [ ] No silent truncation (must report error)
- [ ] AddressSanitizer: 0 buffer overflow alerts
- [ ] UBSan: 0 undefined behavior alerts
- [ ] Test: Buffer overflow attempt properly rejected

**Related Fixes:**
- R13 (io_uring_batcher.cpp:251) — Also unchecked memcpy; use same pattern

---

### R13: io_uring_batcher.cpp:251 — Add Bounds Check to Memcpy

**File:** `src/network/io_uring_batcher.cpp`  
**Location:** Line 251  
**Issue:** Unchecked `memcpy` operation in I/O uring batch processing  
**Severity:** CRITICAL (buffer overflow risk)

**Current Code Pattern:**
```cpp
// Missing bounds check in batch processing loop — can overflow per-batch buffer
for (const auto& request : batch) {
    memcpy(batch_buffer + offset, request.data, request.len);  // ← No offset/size check
    offset += request.len;
}
```

**Fix Strategy:**
1. Add bounds check for total batch size: `if (offset + request.len > BATCH_MAX) return error;`
2. Verify per-request size: `if (request.len > BATCH_ITEM_MAX) return error;`
3. Return error on overflow (don't silently truncate)
4. Track batch fill percentage (diagnostic, optional)

**Implementation Guide:**
- Add: `if (offset + data_len > batch_capacity) return BatchFullError;`
- Add: `if (data_len > item_max_size) return ItemTooLargeError;`
- Use: `std::memcpy` with preceding checks
- Return: Appropriate error enum value
- Log: "Batch overflow prevented: needed={}, available={}" (debug level)

**Acceptance Criteria:**
- [ ] Per-batch size check added
- [ ] Per-item size check added
- [ ] Error handling: Returns error on overflow
- [ ] No silent truncation (must report error)
- [ ] AddressSanitizer: 0 buffer overflow alerts
- [ ] UBSan: 0 undefined behavior alerts
- [ ] Test: Batch overflow attempt properly rejected

**Related Fixes:**
- R12 (connection_compression.cpp:114) — Similar pattern; coordinate error handling

---

## Implementation Workflow

### Step 1: Inspect Locations (20 min)

For each fix (R12, R13):
1. Open file at specified line
2. Identify source and destination buffer sizes
3. Locate memcpy operation (likely nearby)
4. Review any existing bounds checks in same function

### Step 2: Add Bounds Checks (30 min)

1. Add size validation before memcpy
2. Use consistent error pattern (if/return or exception)
3. Choose appropriate error type/enum
4. Document check purpose in comment

### Step 3: Implement Error Handling (20 min)

1. Define error return type (if not present)
2. Return error cleanly without side effects
3. Add optional diagnostic logging
4. Document error conditions in function comments

### Step 4: Validation & Tests (30 min)

1. Compile: `cmake --preset windows-release && cmake --build --preset windows-release`
2. Tests: `ctest --preset windows-release -k network`
3. AddressSanitizer: Run with ASan enabled
4. UBSan: Run with UBSan enabled
5. Verify: Bounds check triggers at correct threshold

### Step 5: Create Commit & Report (10 min)

1. Stage changes: `git add src/network/*.cpp`
2. Commit: `git commit -m "Fix Batch 4.1 Phase D: Memcpy bounds validation (R12-R13)"`
3. Create: `ai_working/BATCH4_1_PHASE_D_COMPLETION_REPORT.md`

---

## Quality Gates

### Per-Fix Validation

- [ ] Compilation succeeds (no syntax errors)
- [ ] No new warnings
- [ ] Bounds check present and correct
- [ ] Error handling complete

### Security Validation

```bash
# After implementing all Phase D fixes:
cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
cmake --build --preset windows-release
ctest --preset windows-release -k network --output-on-failure
```

**Expected:**
- AddressSanitizer: 0 buffer overflow alerts
- UBSan: 0 undefined behavior alerts
- All tests PASS

### Buffer Overflow Test

Create simple test to verify bounds enforcement:
```cpp
// Test: Attempt overflow, expect error
char buffer[100];
// Attempt copy 200 bytes → should be rejected
// Status: PASS if error returned, FAIL if overflow occurs
```

---

## Documentation for Phase D Completion

Create: `ai_working/BATCH4_1_PHASE_D_COMPLETION_REPORT.md`

**Contents:**
- Summary of 2 memcpy bounds fixes (R12-R13)
- File locations and changes
- Bounds check strategies used
- Error types defined/used
- Compilation results
- Test results (all PASS)
- AddressSanitizer results (0 overflow alerts)
- UBSan results (0 undefined behavior alerts)
- Security implications summary
- Next phase readiness (Phase E)

---

## Risk Assessment

**Risk Level:** LOW

**Risks & Mitigations:**
1. **Performance impact:** Bounds checks are O(1), minimal overhead; profile if concerned
2. **Error handling complexity:** Use consistent pattern with Phase C timeouts
3. **API stability:** Error handling should be non-breaking (add to error enum if needed)
4. **Interaction with Phase C:** No dependencies; phases can proceed in any order

---

## Timeline & Effort Estimate

| Step | Task | Duration | Resource |
|------|------|----------|----------|
| 1 | Inspect locations | 20 min | agent (code review) |
| 2 | Add bounds checks | 30 min | agent (edit operations) |
| 3 | Error handling | 20 min | agent (error paths) |
| 4 | Validation & tests | 30 min | agent (build + test) |
| 5 | Commit & report | 10 min | agent (git + doc) |
| **Total** | Phase D | **2-3 hrs** | themisdb-implementer |

**Start:** After Phase C completion (estimated Aug 18-19, 2:00 PM UTC)  
**End:** Aug 19, 5-6 PM UTC (with contingency)

---

## Error Handling Patterns Reference

### Pattern 1: Return Error Code

```cpp
int copyWithBounds(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (src_size > dest_size) {
        return ERR_BUFFER_OVERFLOW;  // Error code
    }
    std::memcpy(dest, src, src_size);
    return 0;  // Success
}
```

### Pattern 2: Throw Exception

```cpp
void copyWithBounds(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (src_size > dest_size) {
        throw std::overflow_error("Destination buffer too small");
    }
    std::memcpy(dest, src, src_size);
}
```

### Pattern 3: std::optional (C++17)

```cpp
std::optional<size_t> copyWithBounds(void* dest, size_t dest_size, 
                                      const void* src, size_t src_size) {
    if (src_size > dest_size) {
        return std::nullopt;  // Indicates failure
    }
    std::memcpy(dest, src, src_size);
    return src_size;  // Indicates success
}
```

**Choose pattern based on existing codebase convention** (likely Pattern 1 or 2 in this repo)

---

## Dependency Notes

- **Phase C Prerequisite:** Should be complete before Phase D (timeout fixes)
- **No interaction with Phases E-F:** Can be done in parallel if needed
- **Related Phase C:** If timeout logic interacts with memcpy, coordinate timing

---

**Document Status:** Ready for Phase D implementation  
**Approval Status:** Pending Phase C completion  
**Next:** Send Phase D instructions to agent after Phase C passes validation
