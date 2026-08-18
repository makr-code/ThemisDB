/**
 * @file BATCH_A9_DETAILED_FIXES.md
 * @brief Batch A-9 — Detailed Code Fixes for Priority Files
 *
 * Specific code changes required in each priority file to fix CRITICAL gaps.
 * This document provides exact line-by-line fixes for CUDA safety hardening.
 *
 * @version 1.0
 * @date 2026-08-18
 */

# Batch A-9 Detailed Code Fixes

## Overview

This document specifies the exact code changes needed to fix 16+ CRITICAL GPU safety gaps across 5 priority files. Total changes: ~250 LOC additions/modifications.

---

## 1. src/gpu/unified_memory.cpp

**CRITICAL Issues: 4**
- Line 13: gpu_memory_leak
- Line 45: data_race
- Line 80: gpu_memory_leak
- Line 161: use_after_free_gpu

### Fix 1.1: Add comprehensive error logging

**Location:** After line 22 (after includes)

**ADD:**
```cpp
#include "themis/gpu/gpu_error.h"

namespace {

// Helper function for detailed error reporting
[[nodiscard]] inline bool checkCudaError(cudaError_t err, const char* operation, const char* file, int line) {
    if (err != cudaSuccess) {
        THEMIS_ERROR("CUDA {} failed at {}:{}: {}", 
                     operation, file, line, cudaGetErrorString(err));
        return false;
    }
    return true;
}

[[nodiscard]] inline bool checkHipError(hipError_t err, const char* operation, const char* file, int line) {
    if (err != hipSuccess) {
        THEMIS_ERROR("HIP {} failed at {}:{}: {}", 
                     operation, file, line, hipGetErrorString(err));
        return false;
    }
    return true;
}

}  // namespace
```

### Fix 1.2: Enhanced allocate() with error logging

**Location:** Lines 90-130 (entire allocate function)

**REPLACE with:**
```cpp
void *GPUUnifiedMemoryAllocator::allocate(size_t bytes, const std::string &tag, const std::string &tenant_id) {
    if (bytes == 0) {
        return nullptr;
    }

    void *ptr = nullptr;

#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaMallocManaged(&ptr, bytes, cudaMemAttachGlobal);
    if (!checkCudaError(err, "cudaMallocManaged", __FILE__, __LINE__)) {
        THEMIS_WARN("GPU unified memory allocation failed for {} bytes (tag: {})", bytes, tag);
        ptr = nullptr;
    }
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipMallocManaged(&ptr, bytes, hipMemAttachGlobal);
    if (!checkHipError(err, "hipMallocManaged", __FILE__, __LINE__)) {
        THEMIS_WARN("HIP unified memory allocation failed for {} bytes (tag: {})", bytes, tag);
        ptr = nullptr;
    }
#else
    ptr = std::malloc(bytes);
#endif

    if (!ptr) {
        THEMIS_ERROR("Memory allocation failed: {} bytes, tag={}", bytes, tag);
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    AllocationRecord rec;
    rec.ptr       = ptr;
    rec.bytes     = bytes;
    rec.tag       = tag;
    rec.tenant_id = tenant_id;
    active_.push_back(rec);

    ++total_allocations_;
    allocated_bytes_ += static_cast<uint64_t>(bytes);
    if (allocated_bytes_ > peak_bytes_) {
        peak_bytes_ = allocated_bytes_;
    }
    if (!tenant_id.empty()) {
        tenant_bytes_[tenant_id] += static_cast<uint64_t>(bytes);
    }
    
    THEMIS_DEBUG("Allocated {} bytes at {:p} (tag: {}, tenant: {})", 
                 bytes, ptr, tag, tenant_id);
    return ptr;
}
```

### Fix 1.3: Enhanced free() with error checking

**Location:** Lines 136-175 (entire free function)

**REPLACE with:**
```cpp
bool GPUUnifiedMemoryAllocator::free(void *ptr) {
    if (!ptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(active_.begin(), active_.end(), 
                          [ptr](const AllocationRecord &r) { return r.ptr == ptr; });
    if (it == active_.end()) {
        THEMIS_WARN("Attempted to free unknown GPU pointer: {:p}", ptr);
        return false;
    }

    const size_t bytes = it->bytes;
    const std::string tag = it->tag;
    const std::string tenant_id = it->tenant_id;

    if (!platformFree(ptr)) {
        THEMIS_ERROR("Failed to free GPU memory at {:p} ({} bytes, tag: {})", 
                     ptr, bytes, tag);
        return false;
    }

    active_.erase(it);
    ++total_frees_;
    if (allocated_bytes_ >= static_cast<uint64_t>(bytes)) {
        allocated_bytes_ -= static_cast<uint64_t>(bytes);
    } else {
        allocated_bytes_ = 0;
    }
    if (!tenant_id.empty()) {
        auto tit = tenant_bytes_.find(tenant_id);
        if (tit != tenant_bytes_.end()) {
            if (tit->second >= static_cast<uint64_t>(bytes)) {
                tit->second -= static_cast<uint64_t>(bytes);
            } else {
                tit->second = 0;
            }
        }
    }

    THEMIS_DEBUG("Freed {} bytes from {:p} (tag: {}, tenant: {})", 
                 bytes, ptr, tag, tenant_id);
    return true;
}
```

### Fix 1.4: Enhanced prefetch() with error logging

**Location:** Lines 181-197 (prefetch function)

**REPLACE with:**
```cpp
bool GPUUnifiedMemoryAllocator::prefetch(const void *ptr, size_t bytes, 
                                         [[maybe_unused]] int device_id) {
    if (!ptr || bytes == 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    ++prefetch_calls_;

#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaMemPrefetchAsync(ptr, bytes, device_id, nullptr);
    if (!checkCudaError(err, "cudaMemPrefetchAsync", __FILE__, __LINE__)) {
        THEMIS_WARN("GPU prefetch failed for {} bytes to device {}", bytes, device_id);
        return false;
    }
    return true;
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipMemPrefetchAsync(ptr, bytes, device_id, nullptr);
    if (!checkHipError(err, "hipMemPrefetchAsync", __FILE__, __LINE__)) {
        THEMIS_WARN("HIP prefetch failed for {} bytes to device {}", bytes, device_id);
        return false;
    }
    return true;
#else
    static_cast<void>(device_id);
    return true;
#endif
}
```

### Fix 1.5: Enhanced advise() with error logging

**Location:** Lines 203-260 (advise function)

**REPLACE CUDA path:**
```cpp
#ifdef THEMIS_ENABLE_CUDA
    cudaMemoryAdvise cuda_advice;
    switch (advice) {
        case MemAdvice::SET_PREFERRED_LOCATION:
            cuda_advice = cudaMemAdviseSetPreferredLocation;
            break;
        case MemAdvice::SET_ACCESSED_BY:
            cuda_advice = cudaMemAdviseSetAccessedBy;
            break;
        case MemAdvice::SET_READ_MOSTLY:
            cuda_advice = cudaMemAdviseSetReadMostly;
            break;
        case MemAdvice::UNSET_PREFERRED_LOCATION:
            cuda_advice = cudaMemAdviseUnsetPreferredLocation;
            break;
        case MemAdvice::UNSET_ACCESSED_BY:
            cuda_advice = cudaMemAdviseUnsetAccessedBy;
            break;
        case MemAdvice::UNSET_READ_MOSTLY:
            cuda_advice = cudaMemAdviseUnsetReadMostly;
            break;
        default:
            return false;
    }
    
    cudaError_t err = cudaMemAdvise(ptr, bytes, cuda_advice, device_id);
    if (!checkCudaError(err, "cudaMemAdvise", __FILE__, __LINE__)) {
        THEMIS_WARN("GPU memory advise failed: {} bytes, advice={}", bytes, static_cast<int>(advice));
        return false;
    }
    return true;
```

---

## 2. src/gpu/query_accelerator.cpp

**CRITICAL Issues: 8**
- Lines 787-788: use_after_free_gpu (__half d_a, d_b)
- Lines 825-827: use_after_free_gpu (__nv_bfloat16 d_a, d_b, d_c)
- Lines 916-917: use_after_free_gpu (HIP version d_a, d_b)
- Line 1100: gpu_memory_leak (cuVS allocation)

### Fix 2.1: Verify RAII wrappers are used (Lines 807-850)

**Current code already uses:**
- `make_unique_gpu<T>(n)` for allocations
- `CHECKED_CUDA()` for error checking

**Status:** ✅ Already fixed in current version

### Fix 2.2: Add FP32 dotProduct error handling (Lines 802-832)

**REPLACE try/catch block (around line 806):**
```cpp
try {
    auto d_a = themis::gpu::make_unique_gpu<float>(n);
    auto d_b = themis::gpu::make_unique_gpu<float>(n);
    
    // Copy vectors to device with comprehensive error checking
    try {
        CHECKED_CUDA(cudaMemcpy(d_a.get(), a.data(), n * sizeof(float), 
                               cudaMemcpyHostToDevice));
        CHECKED_CUDA(cudaMemcpy(d_b.get(), b.data(), n * sizeof(float), 
                               cudaMemcpyHostToDevice));
    } catch (const std::exception& e) {
        THEMIS_ERROR("FP32 cudaMemcpy failed: {}", e.what());
        gpu_done = false;
        throw;  // Re-throw to trigger CPU fallback
    }
    
    // Enforce SLA: kernel must complete within 5 seconds
    KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
    
    float dot_result = 0.0f;
    cublasStatus_t cublas_err = cublasSdot(blas.get(), n, d_a.get(), 1, d_b.get(), 1, &dot_result);
    
    if (cublas_err != CUBLAS_STATUS_SUCCESS) {
        THEMIS_ERROR("cublasSdot failed with status {}", static_cast<int>(cublas_err));
        gpu_done = false;
        throw std::runtime_error("cublasSdot failed");
    }
    
    THEMIS_GPU_QUERY_ACCEL_SYNC();
    
    // Check SLA deadline
    if (!kernel_guard.checkTimeoutDeadline()) {
        result.value = static_cast<double>(dot_result);
        gpu_done     = true;
    } else {
        THEMIS_WARN("dotProduct kernel exceeded SLA, degrading to CPU");
        gpu_done = false;
    }
    // d_a, d_b automatically freed on scope exit via unique_gpu_ptr destructor
} catch (const std::exception& e) {
    // Allocation failure or CUDA error → fall through to CPU
    THEMIS_DEBUG("GPU dotProduct path failed: {}", e.what());
    gpu_done = false;
}
```

### Fix 2.3: Add FP16 dotProduct error handling (Lines 832-876)

**REPLACE entire FP16 section:**
```cpp
else if (config_.precision_mode == PrecisionMode::FP16) {
    std::vector<__half> ha(n), hb(n);
    for (int i = 0; i < n; ++i) {
        ha[i] = __float2half(a[i]);
        hb[i] = __float2half(b[i]);
    }
    try {
        auto d_a = themis::gpu::make_unique_gpu<__half>(n);
        auto d_b = themis::gpu::make_unique_gpu<__half>(n);
        auto d_c = themis::gpu::make_unique_gpu<float>(1);
        
        try {
            // Copy quantized vectors to device with error checking
            CHECKED_CUDA(cudaMemcpy(d_a.get(), ha.data(), n * sizeof(__half), 
                                   cudaMemcpyHostToDevice));
            CHECKED_CUDA(cudaMemcpy(d_b.get(), hb.data(), n * sizeof(__half), 
                                   cudaMemcpyHostToDevice));
        } catch (const std::exception& e) {
            THEMIS_ERROR("FP16 cudaMemcpy failed: {}", e.what());
            gpu_done = false;
            throw;
        }
        
        KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
        
        const float alpha = 1.0f, beta = 0.0f;
        cublasStatus_t cublas_err = cublasGemmEx(
            blas.get(), CUBLAS_OP_N, CUBLAS_OP_N, 1, 1, n, &alpha,
            d_b.get(), CUDA_R_16F, 1, d_a.get(), CUDA_R_16F, n,
            &beta, d_c.get(), CUDA_R_32F, 1, CUBLAS_COMPUTE_32F,
            CUBLAS_GEMM_DEFAULT);
        
        if (cublas_err != CUBLAS_STATUS_SUCCESS) {
            THEMIS_ERROR("cublasGemmEx (FP16) failed with status {}", 
                        static_cast<int>(cublas_err));
            gpu_done = false;
            throw std::runtime_error("cublasGemmEx failed");
        }
        
        float c_host = 0.0f;
        try {
            CHECKED_CUDA(cudaMemcpy(&c_host, d_c.get(), sizeof(float), 
                                   cudaMemcpyDeviceToHost));
        } catch (const std::exception& e) {
            THEMIS_ERROR("FP16 result cudaMemcpy failed: {}", e.what());
            gpu_done = false;
            throw;
        }
        
        THEMIS_GPU_QUERY_ACCEL_SYNC();
        
        if (!kernel_guard.checkTimeoutDeadline()) {
            result.value = static_cast<double>(c_host);
            gpu_done     = true;
        } else {
            THEMIS_WARN("dotProduct (FP16) kernel exceeded SLA, degrading to CPU");
            gpu_done = false;
        }
    } catch (const std::exception& e) {
        THEMIS_DEBUG("GPU dotProduct (FP16) failed: {}", e.what());
        gpu_done = false;
    }
}
```

### Fix 2.4: Similar fixes for BF16 and HIP versions

**Pattern:** Apply same error checking and logging as FP32/FP16 sections for:
- BF16 path (around line 876+)
- HIP FP16 path (around line 880+)
- HIP BF16 path (if present)

---

## 3. src/gpu/memory_pool.cpp

**CRITICAL Issues: 1** (+ 8 HIGH)
- Line 85: iterator_invalidation during concurrent access

### Fix 3.1: Protect iterator operations with RAII

**Pattern:** Wherever pool_.erase() is called, ensure iterator is re-validated after unlocking:

```cpp
// Before: UNSAFE
{
    auto it = pool_.find(key);
    lock.unlock();
    // ... do work ...
    lock.lock();
    pool_.erase(it);  // Iterator may be invalid!
}

// After: SAFE
{
    std::string key_copy = key;  // Copy key for later
    {
        auto it = pool_.find(key);
        if (it != pool_.end()) {
            lock.unlock();
            // ... do work ...
            lock.lock();
            
            // Re-find iterator
            auto it2 = pool_.find(key_copy);
            if (it2 != pool_.end()) {
                pool_.erase(it2);
            }
        }
    }
}
```

---

## 4. src/gpu/gpu_memory_manager_edition.cpp

**CRITICAL Issues: 0** (+ 42 HIGH db_connection_leak)

### Fix 4.1: Wrap all GPU resource creation/destruction

**Pattern:** Ensure all GPU allocations are wrapped with RAII:

```cpp
// Before: UNSAFE
{
    float* d_data = nullptr;
    cudaMalloc(&d_data, size);
    // ... use ...
    cudaFree(d_data);  // May leak on exception
}

// After: SAFE
{
    DeviceMemoryGuard<float> d_data(size / sizeof(float));
    // ... use d_data.get() ...
}  // Auto cleanup
```

---

## 5. src/gpu/time_slice_scheduler.cpp

**CRITICAL Issues: 3**

### Fix 5.1: Add timeout enforcement in dispatch loop

**Location:** dispatch() method, around line 170 (fn(item) call)

**REPLACE:**
```cpp
const auto item_start = std::chrono::steady_clock::now();

// Enforce timeout for each work item
try {
    GPUKernelTimeoutGuard timeout_guard(nullptr, std::chrono::milliseconds(5000));
    fn(item);
    timeout_guard.markCompleted();
    
    if (timeout_guard.didTimeout()) {
        THEMIS_ERROR("Work item {} exceeded timeout for tenant {}", 
                     st.stats.completed + 1, tenant_id);
        ++st.stats.preempted;
        ++total_preempted_;
    }
} catch (const std::exception& e) {
    THEMIS_ERROR("Work item execution failed for tenant {}: {}", 
                 tenant_id, e.what());
    ++st.stats.failed;  // Add failed counter if not present
    // Continue to next item (fail-closed to next tenant)
}

const auto item_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - item_start);
```

---

## Summary of Changes

| File | CRITICAL Fixes | HIGH Fixes | Total Lines |
|------|---|---|---|
| unified_memory.cpp | 4 | 5 | ~80 |
| query_accelerator.cpp | 8 | 21 | ~150 |
| memory_pool.cpp | 1 | 8 | ~30 |
| time_slice_scheduler.cpp | 3 | 5 | ~40 |
| gpu_memory_manager_edition.cpp | 0 | 42 | ~50 |
| **TOTAL** | **16** | **81** | **~350** |

---

## Implementation Checklist

- [ ] unified_memory.cpp error checking (Fixes 1.1-1.5)
- [ ] query_accelerator.cpp error handling (Fixes 2.1-2.4)
- [ ] memory_pool.cpp iterator protection (Fix 3.1)
- [ ] gpu_memory_manager_edition.cpp RAII wrapping (Fix 4.1)
- [ ] time_slice_scheduler.cpp timeout enforcement (Fix 5.1)
- [ ] Build with -Wall -Wextra -Wpedantic (no warnings)
- [ ] Run tests: test_gpu_batch_a9_safety_focused
- [ ] Run ASan/UBSan: verify no leaks or UB
- [ ] Commit with comprehensive message

---

## Testing Validation

```bash
# Build and run batch A-9 tests
cmake --preset community-debug
cmake --build build --target module_gpu_test_gpu_batch_a9_safety_focused_focused

# Verify tests pass
ctest --output-on-failure -R "batch.*a9.*safety"

# Run with ASan for leak detection
cmake --preset develop-asan
cmake --build build --target module_gpu_test_gpu_batch_a9_safety_focused_focused
ASAN_OPTIONS=detect_leaks=1 ./build/tests/gpu/module_gpu_test_gpu_batch_a9_safety_focused_focused

# Verify no compiler warnings
cmake --build build 2>&1 | grep -i "warning" | wc -l
# Should be 0 warnings in GPU module code
```

---

## References

- gpu/MODULE_GAPS.md - Scanner findings
- gpu/gpu_backend_dispatch_contract.h - Error codes
- gpu/gpu_safe_raii.h - Existing RAII patterns
- Wave A Acceptance Report - Prior implementation evidence
