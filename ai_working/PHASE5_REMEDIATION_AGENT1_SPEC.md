# Phase 5 Blocker Remediation — Agent 1 (C++ Code Fixes)

**Date:** 2026-08-15 15:20 UTC  
**Target:** Blockers #1, #2, #3, #4 (CRITICAL + HIGH findings)  
**Agent Type:** themisdb-implementer  
**Timeline:** 12-16 hours (target 2026-08-20 18:00 UTC)  
**Batch Strategy:** Single comprehensive commit with all 4 fixes

---

## Scope

Fix 4 critical C++ safety violations in the Index module:
- Exception-in-destructor (VectorIndexManager)
- Unsafe raw `delete` (2 locations, RAII violation)
- GPU Vector Index destructor incomplete cleanup
- Iterator invalidation (concurrent access)

---

## Files to Modify

1. `include/index/vector_index.h` — VectorIndexManager class declaration
2. `src/index/vector_index.cpp` — VectorIndexManager destructor + delete sites (2 locations)
3. `src/index/gpu_vector_index.h` — GPUVectorIndex class declaration
4. `src/index/gpu_vector_index.cpp` — GPUVectorIndex destructor + iterator pattern

---

## Detailed Fix Specifications

### Fix #1: Exception-in-Destructor (VectorIndexManager)

**File:** `src/index/vector_index.cpp` (lines 91-93)

**Before:**
```cpp
~VectorIndexManager() {
    // Destructor body may throw
    stop();  // ← May throw exceptions
}
```

**After:**
```cpp
~VectorIndexManager() noexcept {
    try {
        stop();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Destructor exception (ignored): {}", e.what());
        // Cleanup continues despite exception
    }
}
```

**Also Update Header:** `include/index/vector_index.h` — declare destructor as `noexcept`

**Validation:**
- Mark class destructor `noexcept` in declaration
- Wrap stop() in try-catch at implementation site
- Verify with clang-tidy: `readability-noexcept-function-check`

---

### Fix #2: Unsafe Raw `delete` → std::unique_ptr (2 locations)

**File:** `src/index/vector_index.cpp`

**Location A (lines 294-301):**
```cpp
// Before:
MyType* ptr = new MyType();
// ... use ptr ...
delete ptr;
ptr = nullptr;

// After:
auto ptr = std::make_unique<MyType>();
// ... use ptr ...
// Automatic cleanup via RAII
```

**Location B (lines 2378-2384):**
Same pattern — replace with std::make_unique

**Search/Replace Strategy:**
```bash
# Find all manual delete patterns in vector_index.cpp
grep -n "new.*MyType\|delete.*ptr" src/index/vector_index.cpp
```

**Update Member Variables:**
- Change type from `MyType*` to `std::unique_ptr<MyType>` in class header
- Update all read sites to use `.get()` or `*ptr` syntax

**Validation:**
- Compile without errors/warnings
- Verify ASan finds 0 memory leaks
- Run existing tests for vector_index

---

### Fix #3: GPU Vector Index Destructor (Incomplete Cleanup)

**File:** `src/index/gpu_vector_index.cpp` (line 1053)

**Before:**
```cpp
~GPUVectorIndex() = default;  // ← Default destructor, no cleanup
```

**After:**
```cpp
~GPUVectorIndex() noexcept {
    try {
        releaseGPUResources();  // Explicit cleanup with error handling
    } catch (const std::exception& e) {
        THEMIS_WARN("GPU cleanup failed (ignored): {}", e.what());
    }
}
```

**Also Update Header:** `include/index/gpu_vector_index.h`
- Change from `= default` to explicit declaration
- Mark `noexcept`

**Implementation Checklist:**
- Verify `releaseGPUResources()` exists; if not, create it
- Call CUDA cleanup routines (cudaFree, cudaStreamDestroy, etc.)
- Catch and log errors without propagating

**Validation:**
- Compile with CUDA support
- Verify GPU memory tracking (no leaks with deviceQuery)

---

### Fix #4: Iterator Invalidation (Concurrent Access)

**File:** `src/index/gpu_vector_index.cpp` (lines 103-130)

**Before:**
```cpp
std::vector<Index> indices;
auto iter = indices.begin();  // ← Cache iterator
// ... concurrent mutation could invalidate iter ...
for (auto& idx : indices) {  // ← May dereference invalid iter
    use(*iter);  // ← Segmentation fault risk
}
```

**After:**
```cpp
// Use size caching + index-based access instead:
std::vector<Index> indices;
size_t original_size = indices.size();
for (size_t i = 0; i < original_size; ++i) {
    if (i < indices.size()) {  // ← Guard bounds check
        use(indices[i]);  // ← Safe index access
    }
}
```

**Search/Replace Strategy:**
```bash
# Find cached iterators in gpu_vector_index.cpp
grep -n "auto.*iter.*\.begin()\|for.*iter.*;" src/index/gpu_vector_index.cpp
```

**Refactoring Approach:**
1. Replace iterator caching with size snapshot
2. Use index-based loop instead of iterator
3. Add bounds guard before each access
4. Document thread-safety model if concurrent access expected

**Validation:**
- TSan: 0 data races on iteration
- Compile without errors
- Existing iteration tests pass

---

## Acceptance Criteria

- [ ] All 4 fixes implemented in single commit
- [ ] Destructors marked `noexcept` with try-catch
- [ ] All `new`/`delete` replaced with `std::unique_ptr`/`std::make_unique`
- [ ] Iterator invalidation patterns refactored
- [ ] Compiles without warnings (gcc/clang)
- [ ] ASan: 0 memory leaks/errors
- [ ] UBSan: 0 undefined behavior
- [ ] TSan: 0 data races
- [ ] Existing tests pass (no regressions)
- [ ] Code adheres to C++ Core Guidelines

---

## Commit Message

```
fix(index): Phase 5 blocker resolution — destructors, RAII, iterator safety

- Exception-in-destructor: VectorIndexManager destructor now noexcept
  with exception wrapping (std::terminate crash fix)
- RAII violation: Replace 2x manual delete with std::unique_ptr
  (memory leak + undefined behavior fix)
- GPU cleanup: GPUVectorIndex destructor explicit noexcept cleanup
  (GPU resource exhaustion fix)
- Iterator invalidation: Replace cached iterators with size-cached
  index-based loop + bounds guards (race condition / segfault fix)

Fixes Phase 5 blocker findings #1-4 (CRITICAL + HIGH).
Validated: ASan/UBSan/TSan gates + existing test regression pass.

Timeline impact: Unblocks CP-1 validation (2026-08-28).
```

---

## Quality Gates

**Before Submission:**
1. Run local build: `cmake --preset develop-release && cmake --build --preset develop-release`
2. Run ASan: `cmake --preset develop-asan && cmake --build --preset develop-asan && ctest --preset develop-asan`
3. Run TSan: `cmake --preset develop-tsan && cmake --build --preset develop-tsan && ctest --preset develop-tsan`
4. Run UBSan: `cmake --preset develop-ubsan && cmake --build --preset develop-ubsan`
5. Verify: All existing tests pass with no new warnings

**Submission Checklist:**
- [ ] All 4 fixes committed together
- [ ] CI/CD gates pass (ASan/TSan/UBSan 0 alerts)
- [ ] Code style: clang-format compliant
- [ ] Doxygen: no new docs needed (internal changes only)
- [ ] Tests: no regressions

---

**Estimated Duration:** 8-12 hours  
**Target Submission:** 2026-08-20 15:00 UTC (3 hours before 18:00 deadline)

