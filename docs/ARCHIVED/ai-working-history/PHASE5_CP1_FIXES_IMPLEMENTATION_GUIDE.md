# Phase 5 Critical Findings — Required Fixes & Implementation Guide

## Fix #1: Exception-Safe Destructor for VectorIndexManager

**File:** `src/index/vector_index.cpp`  
**Current (Lines 91-93):**
```cpp
VectorIndexManager::~VectorIndexManager() {
    shutdown();
}
```

**Problem:** `shutdown()` is not `noexcept` and can throw exceptions

**Fixed Code:**
```cpp
VectorIndexManager::~VectorIndexManager() noexcept {
    try {
        shutdown();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception in VectorIndexManager::shutdown(): {}", e.what());
        // Do NOT re-throw; destructors must be noexcept
    } catch (...) {
        THEMIS_ERROR("Unknown exception in VectorIndexManager::shutdown()");
        // Do NOT re-throw; destructors must be noexcept
    }
}
```

**Also Required:** Update `shutdown()` signature

**File:** `include/index/vector_index.h`  
**Current:** (Find the method declaration)
```cpp
Status shutdown();  // Current signature
```

**Fixed:** Mark as `noexcept` in the implementation (no change to header needed if it's in .cpp)

---

## Fix #2: Exception-Safe Raw `delete` Operations

**File:** `src/index/vector_index.cpp`  
**Current (Lines 294-301 in shutdown()):**
```cpp
// Release the HNSW index to avoid memory leaks
#ifdef THEMIS_HNSW_ENABLED
if (hnswIndex_) {
    delete static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
    hnswIndex_ = nullptr;
    useHnsw_ = false;
}
if (hnswSpace_) {
    delete static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_);
    hnswSpace_ = nullptr;
}
#endif
```

**Problem:** If `delete` throws (e.g., destructor error), `nullptr` assignment is skipped, leaving dangling pointer

**Fixed Code:**
```cpp
// Release the HNSW index safely
#ifdef THEMIS_HNSW_ENABLED
if (hnswIndex_) {
    try {
        std::unique_ptr<hnswlib::HierarchicalNSW<float>> temp(
            static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_)
        );
        hnswIndex_ = nullptr;
        useHnsw_ = false;
        // unique_ptr dtor called; manages cleanup with exception suppression
    } catch (const std::exception& e) {
        THEMIS_WARN("Exception cleaning up HNSW index: {}", e.what());
        hnswIndex_ = nullptr;  // Force cleanup despite exception
    }
}
if (hnswSpace_) {
    try {
        std::unique_ptr<hnswlib::SpaceInterface<float>> temp(
            static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_)
        );
        hnswSpace_ = nullptr;
        // unique_ptr dtor called; manages cleanup
    } catch (const std::exception& e) {
        THEMIS_WARN("Exception cleaning up HNSW space: {}", e.what());
        hnswSpace_ = nullptr;  // Force cleanup despite exception
    }
}
#endif
```

**Also Required:** Same fix at lines 2378-2384 in `loadIndex()` method

---

## Fix #3: Exception-Safe GPU Vector Index Destructor

**File:** `src/index/gpu_vector_index.cpp`  
**Current (Line 1053):**
```cpp
GPUVectorIndex::~GPUVectorIndex() = default;
```

**Problem:** Default destructor calls pImpl dtor which calls shutdown() that can throw

**Fixed Code:**
```cpp
GPUVectorIndex::~GPUVectorIndex() noexcept {
    try {
        if (pImpl) {
            // pImpl::shutdown() may throw; we catch and suppress
            pImpl.reset();  // Calls ~Impl() which calls shutdown()
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception in GPUVectorIndex cleanup: {}", e.what());
    } catch (...) {
        THEMIS_ERROR("Unknown exception in GPUVectorIndex cleanup");
    }
}
```

**Alternative: If pImpl doesn't have public shutdown()**
```cpp
GPUVectorIndex::~GPUVectorIndex() noexcept {
    try {
        // Allow pImpl destructor to run; it will call shutdown()
        // If shutdown() throws, catch it here
    } catch (...) {
        THEMIS_ERROR("Exception in GPUVectorIndex cleanup");
    }
}
```

---

## Fix #4: Iterator Invalidation in rebuildOversubPartitions()

**File:** `src/index/gpu_vector_index.cpp`  
**Current (Lines 103-130):**
```cpp
void rebuildOversubPartitions() {
    if (!oversubManager || vectorData.empty() || oversubBulkLoading_) return;

    // Remove all existing partitions.
    for (size_t pid : oversubManager->getAllPartitionIds()) {
        oversubManager->removePartition(pid);
    }

    const size_t dim   = static_cast<size_t>(dimension);
    const size_t psize = (config.oversubscription_partition_vectors > 0)
                             ? config.oversubscription_partition_vectors
                             : static_cast<size_t>(65536);
    const size_t total = vectorData.size();  // PROBLEM: size not refreshed in loop

    for (size_t start = 0; start < total; start += psize) {
        const size_t end = std::min(start + psize, total);
        const size_t n   = end - start;

        std::vector<float> flat;
        flat.reserve(n * dim);
        for (size_t i = start; i < end; ++i) {
            flat.insert(flat.end(), vectorData[i].begin(), vectorData[i].end());  // UNSAFE
        }
        const std::string tag = "vecs[" + std::to_string(start) + "," +
                                std::to_string(end) + ")";
        oversubManager->addPartition(flat, n, dim, tag);  // Could throw/modify
    }
}
```

**Problems:**
1. `total` size computed once; if vectorData resizes, loop bounds become invalid
2. `vectorData[i]` index access without bounds checking
3. No synchronization between vectorData mutations and partition rebuild

**Fixed Code:**
```cpp
void rebuildOversubPartitions() {
    if (!oversubManager || vectorData.empty() || oversubBulkLoading_) return;

    // Remove all existing partitions.
    for (size_t pid : oversubManager->getAllPartitionIds()) {
        oversubManager->removePartition(pid);
    }

    const size_t dim   = static_cast<size_t>(dimension);
    const size_t psize = (config.oversubscription_partition_vectors > 0)
                             ? config.oversubscription_partition_vectors
                             : static_cast<size_t>(65536);

    // Key fix: Refresh size before entering loop and on each iteration
    size_t vectorDataSize = vectorData.size();
    if (vectorDataSize == 0) return;

    for (size_t start = 0; start < vectorDataSize; start += psize) {
        // CRITICAL: Re-check size to detect concurrent modifications
        if (vectorData.size() != vectorDataSize) {
            THEMIS_WARN("vectorData size changed during partition rebuild "
                       "(was {}, now {}); aborting rebuild to prevent corruption",
                       vectorDataSize, vectorData.size());
            break;  // Defer rebuild to next call
        }

        const size_t end = std::min(start + psize, vectorDataSize);
        const size_t n   = end - start;

        std::vector<float> flat;
        flat.reserve(n * dim);
        for (size_t i = start; i < end; ++i) {
            // Additional safety: bounds check before access
            if (i >= vectorData.size()) {
                THEMIS_WARN("vectorData index {} out of bounds (size={}); "
                           "stopping partition rebuild", i, vectorData.size());
                start = vectorDataSize;  // Terminate outer loop
                break;
            }
            flat.insert(flat.end(), vectorData[i].begin(), vectorData[i].end());
        }
        
        const std::string tag = "vecs[" + std::to_string(start) + "," +
                                std::to_string(end) + ")";
        try {
            oversubManager->addPartition(flat, n, dim, tag);
        } catch (const std::exception& e) {
            THEMIS_WARN("Exception adding partition [{}:{}): {}", start, end, e.what());
            // Continue with next partition rather than failing entire rebuild
        }
    }
}
```

**Recommendation:** Add synchronization (mutex) around vectorData mutations if called from concurrent contexts:
```cpp
// In header or appropriate place
std::shared_mutex vectorData_mutex_;

// In rebuildOversubPartitions():
std::shared_lock<std::shared_mutex> lock(vectorData_mutex_);  // Read lock
size_t vectorDataSize = vectorData.size();
if (vectorDataSize == 0) return;

for (size_t start = 0; start < vectorDataSize; start += psize) {
    // ... rest of code
}
```

---

## Fix #5: Create Required Test Files

### Test 1: `tests/index/test_index_destructor_safety.cpp`

```cpp
#include <gtest/gtest.h>
#include "index/vector_index.h"
#include "index/gpu_vector_index.h"

class IndexDestructorSafetyTests : public ::testing::Test {
protected:
    // Test infrastructure
};

// Test 1: VectorIndexManager destructor doesn't throw
TEST_F(IndexDestructorSafetyTests, VectorIndexManagerDestructorNoThrow) {
    EXPECT_NO_THROW({
        VectorIndexManager mgr(/* db ref */);
        // Destructor called here; must not throw
    });
}

// Test 2: GPUVectorIndex destructor doesn't throw
TEST_F(IndexDestructorSafetyTests, GPUVectorIndexDestructorNoThrow) {
    EXPECT_NO_THROW({
        GPUVectorIndex::Config cfg;
        GPUVectorIndex idx(cfg);
        idx.initialize(128);  // Initialize some state
        // Destructor called here; must not throw
    });
}

// Test 3: Destructor after HNSW initialization
TEST_F(IndexDestructorSafetyTests, DestructorAfterHnswInit) {
    EXPECT_NO_THROW({
        VectorIndexManager mgr(/* db */);
        // Initialize HNSW index
        // ... initialization code ...
        // Destructor called; must not throw even with HNSW state
    });
}

// Test 4: Destructor with pending operations
TEST_F(IndexDestructorSafetyTests, DestructorWithPendingOps) {
    EXPECT_NO_THROW({
        GPUVectorIndex idx;
        idx.initialize(64);
        // Add some vectors
        // ... add operations ...
        // Destructor called; must not throw
    });
}

// Test 5: Multiple construct/destruct cycles
TEST_F(IndexDestructorSafetyTests, RepeatedConstructDestruct) {
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW({
            GPUVectorIndex idx;
            idx.initialize(32);
            // Immediate destruct
        });
    }
}
```

### Test 2: `tests/index/test_index_iterator_validity.cpp`

```cpp
#include <gtest/gtest.h>
#include "index/gpu_vector_index.h"

class IndexIteratorValidityTests : public ::testing::Test {
protected:
    // Test infrastructure
};

// Test 1: Vector access within bounds
TEST_F(IndexIteratorValidityTests, VectorAccessInBounds) {
    // ... add N vectors
    // ... verify all indices are accessible
}

// Test 2: Concurrent add doesn't invalidate iterators during rebuild
TEST_F(IndexIteratorValidityTests, ConcurrentAddDuringRebuild) {
    // ... add vectors
    // ... trigger rebuild
    // ... concurrent add from another thread
    // ... verify no crash/segfault
}

// Test 3: Vector removal doesn't leave gaps in iteration
TEST_F(IndexIteratorValidityTests, RemovalDuringIteration) {
    // ... add 100 vectors
    // ... start rebuild
    // ... remove 10 vectors concurrently
    // ... verify rebuild completes safely
}

// Test 4: Bounds checking in partition rebuild
TEST_F(IndexIteratorValidityTests, PartitionRebuildBounds) {
    // ... add 10000 vectors with small partition size
    // ... trigger rebuild
    // ... verify all partitions are correctly bounded
}

// Test 5: Size change detection
TEST_F(IndexIteratorValidityTests, SizeChangeDetection) {
    // ... add vectors
    // ... trigger rebuild
    // ... modify vectorData size during rebuild
    // ... verify rebuild detects change and aborts safely
}
```

### Test 3: `tests/index/test_index_gpu_memory_safety.cpp`

```cpp
#include <gtest/gtest.h>
#include "index/gpu_vector_index.h"

class IndexGPUMemorySafetyTests : public ::testing::Test {
protected:
    // Test infrastructure
};

// Test 1: No use-after-free on destructor
TEST_F(IndexGPUMemorySafetyTests, NoUseAfterFreeOnDestroy) {
    EXPECT_NO_THROW({
        GPUVectorIndex idx;
        idx.initialize(128);
        // Destructor should safely clean up all pointers
    });
}

// Test 2: HNSW cleanup doesn't leak memory
TEST_F(IndexGPUMemorySafetyTests, HnswCleanupNoLeak) {
    // ... create index with HNSW
    // ... run with AddressSanitizer
    // ... verify no memory leaks reported
}

// Test 3: Exception during cleanup doesn't leak
TEST_F(IndexGPUMemorySafetyTests, ExceptionDuringCleanup) {
    // ... create index
    // ... mock exception in cleanup path
    // ... verify memory is cleaned up despite exception
}

// Test 4: Multiple load/unload cycles
TEST_F(IndexGPUMemorySafetyTests, LoadUnloadCycles) {
    // ... load index 10 times
    // ... unload 10 times
    // ... verify no dangling pointers
}

// Test 5: GPU memory oversubscription cleanup
TEST_F(IndexGPUMemorySafetyTests, OversubscriptionCleanup) {
    // ... enable GPU memory oversubscription
    // ... add vectors larger than VRAM
    // ... verify cleanup properly releases all partitions
}
```

---

## Fix #6: Add CMake Presets

**File:** `CMakePresets.json`  
**Action:** Add these configuration blocks to the `configurePresets` array:

```json
{
  "name": "develop-strict",
  "displayName": "Development: Strict Warnings & Error Checks",
  "description": "Debug build with strict compiler warnings treated as errors. Use for CI/CD gate validation.",
  "inherits": "linux-debug",
  "cacheVariables": {
    "CMAKE_CXX_FLAGS": "-Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion",
    "THEMIS_STRICT_WARNINGS": "ON",
    "THEMIS_ENABLE_COMPILER_CACHE": "ON"
  }
},
{
  "name": "develop-asan",
  "displayName": "Development: AddressSanitizer (ASan)",
  "description": "Debug build with AddressSanitizer enabled for memory error detection.",
  "inherits": "linux-debug",
  "cacheVariables": {
    "THEMIS_ENABLE_ASAN": "ON",
    "CMAKE_CXX_FLAGS": "-fsanitize=address -fno-omit-frame-pointer"
  }
},
{
  "name": "develop-tsan",
  "displayName": "Development: ThreadSanitizer (TSan)",
  "description": "Debug build with ThreadSanitizer enabled for data race detection.",
  "inherits": "linux-debug",
  "cacheVariables": {
    "THEMIS_ENABLE_TSAN": "ON",
    "CMAKE_CXX_FLAGS": "-fsanitize=thread -fno-omit-frame-pointer"
  }
}
```

Also add corresponding buildPresets:
```json
{
  "name": "develop-strict",
  "configurePreset": "develop-strict",
  "inherits": "default-build"
},
{
  "name": "develop-asan",
  "configurePreset": "develop-asan",
  "inherits": "default-build"
},
{
  "name": "develop-tsan",
  "configurePreset": "develop-tsan",
  "inherits": "default-build"
}
```

---

## Validation Commands

After implementing all fixes:

```bash
# Test 1: Build with strict warnings
cmake --preset develop-strict
cmake --build build-strict --target index_tests -j 16

# Test 2: Run destructor safety tests
ctest -R "IndexDestructorSafetyTests" --verbose

# Test 3: Build with AddressSanitizer
cmake --preset develop-asan
cmake --build build-asan --target index_tests -j 4
ctest -R "IndexGPUMemorySafetyTests" --verbose

# Test 4: Build with ThreadSanitizer (run twice)
cmake --preset develop-tsan
cmake --build build-tsan --target index_tests -j 4
ctest -R "IndexIteratorValidityTests" --verbose
ctest -R "IndexIteratorValidityTests" --verbose

# Test 5: Run all index tests
ctest -L index --verbose --output-on-failure
```

---

## Sign-Off Checklist

After all fixes:

- [ ] All 6 findings have corresponding code changes
- [ ] No `noexcept` violation warnings in build output
- [ ] No AddressSanitizer errors in test run
- [ ] No ThreadSanitizer race conditions (2+ consecutive runs)
- [ ] All new test files added and passing
- [ ] Code review approval on PR
- [ ] CI/CD gates GREEN (develop-strict, develop-asan, develop-tsan)

