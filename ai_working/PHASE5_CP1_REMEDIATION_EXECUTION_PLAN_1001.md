# Phase 5 CP-1 Blocker Remediation — Execution Plan

**Date:** 2026-08-15 13:42 UTC  
**Status:** 🔴 BLOCKER REMEDIATION IN PROGRESS  
**Deadline:** 2026-08-22 (7 days)  
**Target Completion:** 2026-08-20 (5 days, with buffer)  
**ETA:** 2026-08-20 18:00 UTC

---

## Findings Summary

| # | Finding | Severity | File(s) | Lines | Status |
|---|---------|----------|---------|-------|--------|
| 1 | Exception-in-destructor (VectorIndexManager) | CRITICAL | vector_index.cpp | 91-93 | FIXING |
| 2 | Unsafe raw `delete` (RAII) | CRITICAL | vector_index.cpp | 294-301, 2378-2384 | FIXING |
| 3 | GPU Vector Index destructor | CRITICAL | gpu_vector_index.cpp | 1053, 214-216, 298+ | FIXING |
| 4 | Iterator invalidation (concurrent) | HIGH | gpu_vector_index.cpp | 103-130 | FIXING |
| 5 | Missing CMake presets | HIGH | CMakePresets.json | N/A | FIXING |
| 6 | Missing test coverage | MEDIUM | tests/index/ | N/A | CREATING |

---

## Remediation Execution Plan

### Phase 1: Critical Destructors (Findings #1 & #3)
**Duration:** 2-3 hours  
**Files:**
- `include/index/vector_index.h`
- `src/index/vector_index.cpp`
- `src/index/gpu_vector_index.cpp`

**Tasks:**
1. Add `noexcept` to VectorIndexManager destructor
2. Wrap all exception-throwing operations in try-catch
3. Add `noexcept` to Impl destructors in gpu_vector_index.cpp
4. Ensure all cleanup paths are exception-safe

### Phase 2: Memory Safety (Finding #2)
**Duration:** 2-3 hours  
**Files:**
- `src/index/vector_index.cpp`

**Tasks:**
1. Replace raw `delete` with `std::unique_ptr` / `std::make_unique`
2. Ensure RAII compliance
3. Verify exception safety of all cleanup paths

### Phase 3: Iterator Safety (Finding #4)
**Duration:** 1-2 hours  
**Files:**
- `src/index/gpu_vector_index.cpp`

**Tasks:**
1. Fix concurrent access patterns
2. Add bounds checks and defensive guards
3. Add thread safety verification

### Phase 4: CMake Configuration (Finding #5)
**Duration:** 1-2 hours  
**Files:**
- `CMakePresets.json`

**Tasks:**
1. Add `develop-strict` preset
2. Add `develop-asan` preset
3. Add `develop-tsan` preset
4. Verify compilation with all presets

### Phase 5: Test Coverage (Finding #6)
**Duration:** 4-6 hours  
**Files:**
- `tests/index/test_index_destructor_safety.cpp` (new)
- `tests/index/test_index_iterator_validity.cpp` (new)
- `tests/index/test_index_gpu_memory_safety.cpp` (new)

**Tasks:**
1. Create destructor safety test
2. Create iterator validity test
3. Create GPU memory safety test
4. Ensure ASan/TSan test compatibility

### Phase 6: Validation & Testing
**Duration:** 2-3 hours

**Tasks:**
1. Build with all presets
2. Run ASan validation (no leaks)
3. Run TSan validation (no races)
4. Run all new tests (must pass)
5. Verify no regressions in existing tests

---

## Success Criteria

- [ ] All 6 findings addressed with code fixes
- [ ] All destructors marked `noexcept` with exception handling
- [ ] All `delete` replaced with RAII (std::unique_ptr)
- [ ] Iterator invalidation patterns fixed
- [ ] CMake presets added and working
- [ ] 3 test files created and passing
- [ ] ASan/UBSan/TSan validation gates all PASS
- [ ] No new compiler warnings
- [ ] All existing tests still pass

---

## Execution Status

Proceeding with systematic implementation...

