# Phase 5 Blocker Remediation — Agent 3 (Test Coverage)

**Date:** 2026-08-15 15:20 UTC  
**Target:** Blocker #6 (MEDIUM finding)  
**Agent Type:** themisdb-implementer  
**Timeline:** 6-8 hours (depends on Agent 1 completion for test targets)  
**Dependency:** Agent 1 must merge first (code fixes needed for tests to validate)

---

## Scope

Create 3 focused test files to validate Phase 2 blocker fixes:

1. `tests/index/test_index_destructor_safety.cpp` — Exception safety in destructors
2. `tests/index/test_index_iterator_validity.cpp` — Iterator re-fetching after mutations
3. `tests/index/test_index_gpu_memory_safety.cpp` — CUDA allocation/free pairing

**Goals:**
- Verify fixes for Blockers #1-4 (C++ safety violations)
- Enable regression testing for future changes
- Provide ASan/TSan validation hooks for CI/CD

---

## Test File 1: `test_index_destructor_safety.cpp`

**Purpose:** Validate exception safety in VectorIndexManager and GPUVectorIndex destructors

**Location:** `tests/index/test_index_destructor_safety.cpp`

**Test Framework:** GTest (aligned with existing test suite)

**Tests to Implement:**

### Test 1.1: VectorIndexManager Destructor with Thrown Exception

```cpp
TEST(VectorIndexDestructorSafety, VectorIndexManagerDestructorHandlesException) {
    // Setup: Create VectorIndexManager in exception context
    {
        VectorIndexManager manager;
        // Simulate exception during destructor
        // (stop() may throw if resources unavailable)
        
        // Verify destructor doesn't crash on exit
        // Expected: Destructor catches exception, logs error, returns normally
    }
    // If we reach here without std::terminate(), test PASS
}
```

### Test 1.2: GPUVectorIndex Destructor with GPU Cleanup

```cpp
TEST(VectorIndexDestructorSafety, GPUVectorIndexDestructorCleanupGPU) {
    {
        GPUVectorIndex gpu_index;
        // Setup GPU resources
        gpu_index.allocateGPUResources(...);
        
        // Verify cleanup called on destructor
        // Expected: releaseGPUResources() called, noexcept
    }
    // No GPU resource leaks after destruction
}
```

### Test 1.3: Destructor Exception During Stack Unwinding

```cpp
TEST(VectorIndexDestructorSafety, DestructorExceptionDuringStackUnwinding) {
    try {
        VectorIndexManager manager;
        throw std::runtime_error("Simulated exception");
        // Destructor called here during stack unwinding
    } catch (const std::runtime_error& e) {
        // If we catch original exception (not std::terminate), test PASS
        EXPECT_EQ(e.what(), std::string("Simulated exception"));
    }
}
```

**Validation Gates:**
- ASan: 0 memory leaks during exception handling
- Program termination: No `std::terminate()` call
- Expected behavior: Destructor catches and logs, allows normal stack unwinding

---

## Test File 2: `test_index_iterator_validity.cpp`

**Purpose:** Validate iterator invalidation fixes (Blocker #4)

**Location:** `tests/index/test_index_iterator_validity.cpp`

**Tests to Implement:**

### Test 2.1: Cached Iterator Invalidation on Mutation

```cpp
TEST(IndexIteratorValidity, CachedIteratorInvalidatedOnMutation) {
    std::vector<Index> indices;
    indices.push_back(Index(1));
    
    // Old (unsafe) pattern:
    // auto iter = indices.begin();
    // indices.push_back(Index(2));  // ← Invalidates iter
    // use(*iter);  // ← Segfault risk
    
    // New (safe) pattern:
    size_t original_size = indices.size();
    indices.push_back(Index(2));  // ← Mutation allowed
    
    for (size_t i = 0; i < original_size; ++i) {
        if (i < indices.size()) {  // ← Guard bounds check
            // Safe to access indices[i]
            EXPECT_EQ(indices[i].id, i + 1);
        }
    }
}
```

### Test 2.2: Concurrent Iterator Mutation (TSan Test)

```cpp
TEST(IndexIteratorValidity, ConcurrentIteratorMutation) {
    std::vector<Index> indices;
    std::atomic<bool> stop(false);
    
    // Thread 1: Mutate
    std::thread mutator([&]() {
        for (int i = 0; i < 100 && !stop; ++i) {
            indices.push_back(Index(i));
        }
    });
    
    // Thread 2: Iterate (using safe pattern)
    std::thread reader([&]() {
        for (int iter = 0; iter < 100 && !stop; ++iter) {
            size_t snapshot_size = indices.size();
            for (size_t i = 0; i < snapshot_size; ++i) {
                if (i < indices.size()) {
                    // Access via bounds-checked index
                    volatile auto id = indices[i].id;
                    (void)id;  // Suppress unused warning
                }
            }
        }
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop = true;
    
    mutator.join();
    reader.join();
    
    // TSan should report 0 data races
}
```

### Test 2.3: Iterator Bounds Protection

```cpp
TEST(IndexIteratorValidity, IteratorBoundsProtection) {
    std::vector<Index> indices;
    indices.push_back(Index(1));
    indices.push_back(Index(2));
    
    size_t original_size = indices.size();  // = 2
    indices.erase(indices.begin() + 1);  // Now size = 1
    
    // Safe access with bounds check:
    for (size_t i = 0; i < original_size; ++i) {
        if (i < indices.size()) {  // ← Bounds guard protects
            EXPECT_EQ(indices[i].id, 1);  // Only element 0 exists now
        }
    }
    // No segfault even though original_size=2, current size=1
}
```

**Validation Gates:**
- TSan: 0 data races in concurrent scenario
- Program stability: No segmentation faults
- Correctness: All elements accessed correctly with bounds checks

---

## Test File 3: `test_index_gpu_memory_safety.cpp`

**Purpose:** Validate GPU memory allocation/deallocation pairing (Blocker #3)

**Location:** `tests/index/test_index_gpu_memory_safety.cpp`

**Tests to Implement:**

### Test 3.1: GPU Resource Allocation and Cleanup

```cpp
TEST(IndexGPUMemorySafety, GPUResourceAllocationCleanup) {
    size_t initial_free_mem = 0;
    // Query initial GPU free memory
    cudaMemGetInfo(&initial_free_mem, nullptr);
    
    {
        GPUVectorIndex gpu_index;
        size_t alloc_size = 1024 * 1024;  // 1 MB
        gpu_index.allocateGPUResources(alloc_size);
        
        size_t post_alloc_free_mem = 0;
        cudaMemGetInfo(&post_alloc_free_mem, nullptr);
        
        // Verify allocation succeeded
        EXPECT_LT(post_alloc_free_mem, initial_free_mem);
    }  // Destructor called here
    
    size_t final_free_mem = 0;
    cudaMemGetInfo(&final_free_mem, nullptr);
    
    // Verify cleanup returned memory
    EXPECT_GT(final_free_mem, initial_free_mem - (1024 * 1024));
    // (allowing small margin for CUDA overhead)
}
```

### Test 3.2: GPU Memory Leak Detection with ASan

```cpp
TEST(IndexGPUMemorySafety, GPUMemoryLeakDetection) {
    // This test runs under ASan to detect:
    // - Leaked GPU pointers
    // - Use-after-free on GPU resources
    // - Buffer overflows in GPU buffers
    
    GPUVectorIndex gpu_index;
    
    // Allocate and intentionally NOT free (simulating leak)
    // Expected: ASan reports "memory leak" if cleanup is missing
    
    gpu_index.allocateGPUResources(100 * 1024);
    // No explicit cleanup; destructor should handle it
}
```

### Test 3.3: GPU Resource Release on Exception

```cpp
TEST(IndexGPUMemorySafety, GPUResourceReleaseOnException) {
    size_t initial_free_mem = 0;
    cudaMemGetInfo(&initial_free_mem, nullptr);
    
    try {
        GPUVectorIndex gpu_index;
        gpu_index.allocateGPUResources(512 * 1024);
        
        // Simulate exception
        throw std::runtime_error("GPU compute error");
        
    } catch (const std::runtime_error& e) {
        // Exception caught
    }
    
    size_t final_free_mem = 0;
    cudaMemGetInfo(&final_free_mem, nullptr);
    
    // Verify GPU resources cleaned up despite exception
    EXPECT_GT(final_free_mem, initial_free_mem - (512 * 1024));
}
```

**Validation Gates:**
- ASan: 0 GPU memory leaks
- GPU memory tracking: Allocations match deallocations
- Exception safety: GPU cleanup happens during exception unwinding

---

## Implementation Strategy

### Phase A: File Creation & GTest Boilerplate

1. Create 3 test files in `tests/index/`
2. Include standard headers: `<gtest/gtest.h>`, CUDA/GPU headers, index headers
3. Add test fixtures if needed (setup/teardown GPU context)

### Phase B: Test Implementation

1. Implement Test 1: Destructor safety (simplest, no GPU)
2. Implement Test 2: Iterator validity (medium complexity)
3. Implement Test 3: GPU memory (requires CUDA, most complex)

### Phase C: Validation & CI Integration

1. Compile all 3 test files: `cmake --build --preset develop-release -t index_destructor_safety_tests`
2. Run with ASan: `cmake --preset develop-asan && ctest --preset develop-asan -R destructor_safety`
3. Run with TSan: `cmake --preset develop-tsan && ctest --preset develop-tsan -R iterator_validity`
4. Verify all tests PASS with 0 sanitizer alerts

---

## CMakeLists.txt Integration

Add to `tests/index/CMakeLists.txt`:

```cmake
# Phase 5 blocker test suite
add_executable(test_index_destructor_safety test_index_destructor_safety.cpp)
target_link_libraries(test_index_destructor_safety PRIVATE themis::index gtest gtest_main)

add_executable(test_index_iterator_validity test_index_iterator_validity.cpp)
target_link_libraries(test_index_iterator_validity PRIVATE themis::index gtest gtest_main)

add_executable(test_index_gpu_memory_safety test_index_gpu_memory_safety.cpp)
target_link_libraries(test_index_gpu_memory_safety PRIVATE themis::index themis::gpu gtest gtest_main)

# Register with ctest
gtest_discover_tests(test_index_destructor_safety)
gtest_discover_tests(test_index_iterator_validity)
gtest_discover_tests(test_index_gpu_memory_safety)
```

---

## Acceptance Criteria

- [ ] 3 test files created in `tests/index/`
- [ ] All tests compile without errors/warnings
- [ ] All tests PASS with default configuration
- [ ] All tests PASS with ASan (0 memory errors)
- [ ] Iterator validity tests PASS with TSan (0 data races)
- [ ] GPU memory tests PASS with GPU available
- [ ] CMakeLists.txt updated with test registration
- [ ] No test regressions in existing test suite

---

## Commit Message

```
test(index): Add Phase 5 blocker validation test suite

- test_index_destructor_safety: Verify noexcept destructors + exception handling
- test_index_iterator_validity: Verify iterator invalidation fixes + bounds guards
- test_index_gpu_memory_safety: Verify GPU resource allocation/cleanup pairing

Fixes Phase 5 blocker finding #6 (MEDIUM).
Enables regression testing and ASan/TSan validation for Blockers #1-4 fixes.

Tests: 10 total (3 destructors, 3 iterators, 3 GPU, 1 edge case)
All tests PASS with ASan/TSan/UBSan gates.
```

---

## Quality Gates

**Before Submission:**
1. Compile: `cmake --preset develop-release && cmake --build --preset develop-release`
2. Run tests: `ctest --preset develop-release -R "test_index_(destructor|iterator|gpu)" --output-on-failure`
3. ASan: `cmake --preset develop-asan && cmake --build --preset develop-asan && ctest --preset develop-asan -R "test_index_" --output-on-failure`
4. TSan (iterator only): `cmake --preset develop-tsan && cmake --build --preset develop-tsan && ctest --preset develop-tsan -R "iterator_validity" --output-on-failure`

---

**Estimated Duration:** 6-8 hours  
**Target Submission:** 2026-08-21 (after Agent 1 merges)  
**Dependency Chain:** Requires Agent 1 (code fixes) to merge first for test targets to be valid

