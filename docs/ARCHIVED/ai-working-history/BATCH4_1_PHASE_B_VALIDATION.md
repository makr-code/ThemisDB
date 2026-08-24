# Phase B Validation Checklist — Missing Destructors (R06-R08)

**Status:** 🟡 Implementation in progress (agent: network-batch4-1-implementer)  
**Target:** Aug 17-18, 2026  
**Quality Gate:** ASan = 0 memory leaks, all tests PASS  

---

## Pre-Validation Setup

These should be verified before agent starts Phase B:

- [x] Phase A complete and committed (b806b401e1)
- [x] Compilation baseline established (Phase A compiles cleanly)
- [x] Network tests baseline established (Phase A tests all PASS)
- [x] ASan baseline established (no pre-existing alerts)
- [x] Repository synchronized (HEAD on develop)

---

## Phase B Implementation Targets

### R06: socket_timeout_manager.cpp:71 — Add Missing Destructor

**File:** `src/network/socket_timeout_manager.cpp`  
**Location:** Class definition at/near line 71  
**Issue Type:** missing_dtor (CRITICAL)

**Fix Details:**
- Locate class that should have virtual destructor
- Add: `virtual ~SocketTimeoutManager() = default;` (or with noexcept)
- Ensure no exception-throwing code in destructor body
- Document ownership/cleanup in class header comment

**Acceptance Criteria:**
- [ ] Virtual destructor added
- [ ] Marked noexcept (no exception paths)
- [ ] Class inheritable or has virtual methods ✓
- [ ] No functional logic changes
- [ ] ASan reports 0 new memory leaks on this class

**Related Fixes:**
- R14 (socket_timeout_manager.cpp:202) — Smart pointer misuse in same file; may interact with destructor

---

### R07: service_mesh.cpp:175 — Add Missing Destructor

**File:** `src/network/service_mesh.cpp`  
**Location:** Class definition at/near line 175  
**Issue Type:** missing_dtor (CRITICAL)

**Fix Details:**
- Locate class that should have virtual destructor
- Add: `virtual ~ServiceMeshComponent() = default;` (or appropriate name)
- Ensure exception-safe cleanup
- Document cleanup contract

**Acceptance Criteria:**
- [ ] Virtual destructor added
- [ ] Marked noexcept
- [ ] ASan reports 0 new memory leaks on this class
- [ ] Related timeouts (R10: line 243) still work correctly

**Related Fixes:**
- R10 (service_mesh.cpp:243) — Timeout enforcement in same file; may interact

---

### R08: service_mesh.cpp:194 — Add Missing Destructor

**File:** `src/network/service_mesh.cpp`  
**Location:** Class definition at/near line 194  
**Issue Type:** missing_dtor (CRITICAL)

**Fix Details:**
- Locate class that should have virtual destructor
- Add: `virtual ~<ClassName>() = default;`
- Ensure exception-safe cleanup
- Document cleanup contract

**Acceptance Criteria:**
- [ ] Virtual destructor added
- [ ] Marked noexcept
- [ ] ASan reports 0 new memory leaks on this class
- [ ] Consistent with R07 destructor style

**Related Fixes:**
- R07 (service_mesh.cpp:175) — Also in service_mesh.cpp; coordinate styles

---

## Per-Fix Validation Steps

After agent implements each fix (R06, R07, R08):

### Code Review Checklist

- [ ] **Destructor Signature:**
  - `virtual ~ClassName() = default;` or with body
  - `noexcept` keyword present (no exceptions)
  - No `= delete;` unless explicitly intended

- [ ] **Logic Preservation:**
  - No functional changes to class behavior
  - Cleanup code (if any) moved to separate method or wrapped safely
  - All existing public methods unchanged

- [ ] **Style Consistency:**
  - Matches project's RAII pattern
  - Consistent with Phase A formatting
  - Doxygen comments (if destructor has body)

### Compilation Validation

```bash
# After each fix:
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset windows-release
cmake --build --preset windows-release 2>&1 | tee build.log
```

**Expected:** Build succeeds, no warnings related to destructors

### Unit Test Validation

```bash
# After each fix:
ctest --preset windows-release -k network --output-on-failure 2>&1 | tee test.log
```

**Expected:** All network tests PASS, no test failures

### Memory Leak Detection (ASan)

```bash
# After all 3 fixes in Phase B:
cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=address"
cmake --build --preset windows-release
ctest --preset windows-release -k network --output-on-failure
```

**Expected:** No memory leak reports from AddressSanitizer

---

## Phase B Completion Criteria

✅ **Phase B is complete when:**

1. All 3 destructors added (R06, R07, R08)
2. Code compiles without errors
3. All network tests PASS
4. ASan memory leak detection = 0 alerts
5. Single commit with message: "Fix Batch 4.1 Phase B: Missing destructors (R06-R08)"
6. Phase B completion report created

---

## Documentation to Create After Phase B

Create: `ai_working/BATCH4_1_PHASE_B_COMPLETION_REPORT.md`

**Contents:**
- Summary of 3 destructors added
- File locations and changes made
- Compilation verification status
- Test results (all PASS)
- ASan memory leak results (0 alerts)
- Next phase readiness (Phase C - Timeouts)

---

## Rollback Plan

If Phase B encounters issues:

1. **Compilation Error:** Revert destructors, check for syntax errors
2. **Test Failure:** Verify destructor doesn't break existing logic
3. **ASan Alert:** Review destructor logic; ensure proper cleanup

**Command to rollback:**
```bash
git revert HEAD --no-edit
```

---

## Phase C Preparation (Ready for After Phase B)

Once Phase B is complete and validated:

**Phase C Scope:** Timeout Enforcement (R09-R11, R16)
- wire_protocol_zero_copy.cpp:112, 160
- service_mesh.cpp:243
- wire_protocol_performance.cpp:232

**Key Difference:** Phase C modifies logic (adds timeout params), not just destructors

---

**Document Status:** Ready for validation  
**Agent Status:** Awaiting Phase B implementation results
