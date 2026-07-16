/**
 * @file unified_memory.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: unified_memory.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 338
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=5, M=2, L=0
 * PR History (last 5): #3009 feat(gpu): Unified memory s... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * GPU Unified Memory Allocator — CPU+GPU shared address space.
 *
 * Real cudaMallocManaged / hipMallocManaged calls are gated behind
 * THEMIS_ENABLE_CUDA and THEMIS_ENABLE_HIP respectively.  When neither is
 * defined (CI / CPU-only builds) the allocator falls back to ordinary
 * malloc/free so that all call sites compile and are fully tested without
 * GPU hardware.
 */

#include "themis/gpu/unified_memory.h"

#include <algorithm>
#include <cstdlib>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace gpu {

// ============================================================================
// isSupported
// ============================================================================

bool GPUUnifiedMemoryAllocator::isSupported() noexcept {
    // C++11 guarantees that initialization of a function-local static is
    // performed exactly once, even under concurrent calls.  Using a single
    // const static avoids the two-variable (cached + result) pattern that is
    // susceptible to a data race on the first concurrent call.
    static const bool result = []() noexcept -> bool {
#ifdef THEMIS_ENABLE_CUDA
        int device_count = 0;
        if (cudaGetDeviceCount(&device_count) == cudaSuccess && device_count > 0) {
            cudaDeviceProp prop{};
            if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
                return prop.unifiedAddressing != 0;
            }
        }
#elif defined(THEMIS_ENABLE_HIP)
        int device_count = 0;
        if (hipGetDeviceCount(&device_count) == hipSuccess && device_count > 0) {
            hipDeviceProp_t prop{};
            if (hipGetDeviceProperties(&prop, 0) == hipSuccess) {
                return prop.unifiedAddressing != 0;
            }
        }
#endif
        return false;
    }();
    return result;
}

// ============================================================================
// allocate
// ============================================================================

void *GPUUnifiedMemoryAllocator::allocate(size_t bytes, const std::string &tag, const std::string &tenant_id) {
    if (bytes == 0) {
        return nullptr;
    }

    void *ptr = nullptr;

#ifdef THEMIS_ENABLE_CUDA
    if (cudaMallocManaged(&ptr, bytes, cudaMemAttachGlobal) != cudaSuccess) {
        ptr = nullptr;
    }
#elif defined(THEMIS_ENABLE_HIP)
    if (hipMallocManaged(&ptr, bytes, hipMemAttachGlobal) != hipSuccess) {
        ptr = nullptr;
    }
#else
    ptr = std::malloc(bytes);
#endif

    if (!ptr) {
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
    return ptr;
}

// ============================================================================
// free
// ============================================================================

bool GPUUnifiedMemoryAllocator::free(void *ptr) {
    if (!ptr) {
        return false;
    }

    size_t bytes = 0;
    std::string tenant_id;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it
            = std::find_if(active_.begin(), active_.end(), [ptr](const AllocationRecord &r) { return r.ptr == ptr; });
        if (it == active_.end()) {
            return false;
        }

        bytes     = it->bytes;
        tenant_id = it->tenant_id;

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
    } // mutex released here — platform free runs without holding the lock

#ifdef THEMIS_ENABLE_CUDA
    cudaFree(ptr);
#elif defined(THEMIS_ENABLE_HIP)
    hipFree(ptr);
#else
    std::free(ptr);
#endif

    return true;
}

// ============================================================================
// prefetch
// ============================================================================

bool GPUUnifiedMemoryAllocator::prefetch(const void *ptr, size_t bytes, [[maybe_unused]] int device_id) {
    if (!ptr || bytes == 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    ++prefetch_calls_;

#ifdef THEMIS_ENABLE_CUDA
    return cudaMemPrefetchAsync(ptr, bytes, device_id, nullptr) == cudaSuccess;
#elif defined(THEMIS_ENABLE_HIP)
    return hipMemPrefetchAsync(ptr, bytes, device_id, nullptr) == hipSuccess;
#else
    static_cast<void>(device_id);
    return true;
#endif
}

// ============================================================================
// advise
// ============================================================================

bool GPUUnifiedMemoryAllocator::advise(const void *ptr, size_t bytes, [[maybe_unused]] MemAdvice advice, [[maybe_unused]] int device_id) {
    if (!ptr || bytes == 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    ++advise_calls_;

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
    return cudaMemAdvise(ptr, bytes, cuda_advice, device_id) == cudaSuccess;
#elif defined(THEMIS_ENABLE_HIP)
    hipMemoryAdvise hip_advice;
    switch (advice) {
        case MemAdvice::SET_PREFERRED_LOCATION:
            hip_advice = hipMemAdviseSetPreferredLocation;
            break;
        case MemAdvice::SET_ACCESSED_BY:
            hip_advice = hipMemAdviseSetAccessedBy;
            break;
        case MemAdvice::SET_READ_MOSTLY:
            hip_advice = hipMemAdviseSetReadMostly;
            break;
        case MemAdvice::UNSET_PREFERRED_LOCATION:
            hip_advice = hipMemAdviseUnsetPreferredLocation;
            break;
        case MemAdvice::UNSET_ACCESSED_BY:
            hip_advice = hipMemAdviseUnsetAccessedBy;
            break;
        case MemAdvice::UNSET_READ_MOSTLY:
            hip_advice = hipMemAdviseUnsetReadMostly;
            break;
        default:
            return false;
    }
    return hipMemAdvise(ptr, bytes, hip_advice, device_id) == hipSuccess;
#else
    static_cast<void>(advice);
    static_cast<void>(device_id);
    return true;
#endif
}

// ============================================================================
// getStats
// ============================================================================

GPUUnifiedMemoryAllocator::Stats GPUUnifiedMemoryAllocator::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.total_allocations = total_allocations_;
    s.total_frees       = total_frees_;
    s.allocated_bytes   = allocated_bytes_;
    s.peak_bytes        = peak_bytes_;
    s.prefetch_calls    = prefetch_calls_;
    s.advise_calls      = advise_calls_;
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
    s.hardware_unified = isSupported();
#else
    s.hardware_unified = false;
#endif
    return s;
}

// ============================================================================
// getActiveAllocations
// ============================================================================

std::vector<GPUUnifiedMemoryAllocator::AllocationRecord> GPUUnifiedMemoryAllocator::getActiveAllocations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_;
}

// ============================================================================
// getTenantBytes
// ============================================================================

uint64_t GPUUnifiedMemoryAllocator::getTenantBytes(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenant_bytes_.find(tenant_id);
    return (it != tenant_bytes_.end()) ? it->second : 0u;
}

// ============================================================================
// reset
// ============================================================================

void GPUUnifiedMemoryAllocator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Free all tracked pointers.
    for (auto &rec : active_) {
#ifdef THEMIS_ENABLE_CUDA
        cudaFree(rec.ptr);
#elif defined(THEMIS_ENABLE_HIP)
        hipFree(rec.ptr);
#else
        std::free(rec.ptr);
#endif
    }

    active_.clear();
    tenant_bytes_.clear();
    total_allocations_ = 0;
    total_frees_       = 0;
    allocated_bytes_   = 0;
    peak_bytes_        = 0;
    prefetch_calls_    = 0;
    advise_calls_      = 0;
}

// ============================================================================
// Destructor
// ============================================================================

GPUUnifiedMemoryAllocator::~GPUUnifiedMemoryAllocator() {
    // Free any allocations not explicitly freed by the caller.
    // Mirrors the cleanup pattern in GPUStreamManager::~GPUStreamManager().
    reset();
}

} // namespace gpu
} // namespace themis
