/**
 * @file cuda_raii.h
 * @brief Canonical RAII guards for raw CUDA resource lifecycle management.
 * @version 1.0.0
 * @date 2026-08-24
 *
 * @details
 * This header consolidates lightweight RAII guards for CUDA primitives that
 * are used outside the heavier `GPUStreamHandle`/`GPUMemoryHandle` wrappers
 * in `gpu_raii_wrappers.hpp`.  These guards target the **remaining raw call
 * sites** identified in the 2026-08-24 CUDA-Call Audit (see
 * `src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`).
 *
 * Design rules
 * - No heap allocation: all guards are value-types suitable for stack use.
 * - No-throw destructors: CUDA errors in destructors are swallowed (resource
 *   is always released; caller cannot meaningfully handle cleanup failure).
 * - Move-only semantics: prevents accidental double-destroy.
 * - CPU-only builds: guards compile to no-ops; CUDA types are forward-declared
 *   stubs so the header is always includable without the CUDA toolkit.
 *
 * ## Wave A Context
 * These guards address RAII lifecycle gap items identified during Wave A
 * closure.  They do NOT replace `GPUStreamHandle` or `GPUMemoryHandle` in
 * files that already use those types — they are the lightweight alternative
 * for lower-level call sites (stream_manager, cuda_operations, unified_memory).
 *
 * @see include/gpu/gpu_raii_wrappers.hpp  (full-featured handles)
 * @see include/gpu/gpu_safe_raii.h        (DeviceMemoryGuard, KernelTimeoutGuard)
 * @see include/gpu/gpu_resource_handles.h (CublasHandleGuard, additional RAII)
 * @see src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md (audit evidence)
 * @see include/themis/gpu/gpu_timeout.h   (KernelSLAGuard — 5 s hard limit)
 *
 * @throws Does not throw.  CUDA errors during construction are logged via
 *         spdlog "gpu" logger when available; callers must check isValid().
 */

#pragma once

#include <cstddef>   // size_t
#include <utility>   // std::exchange

// ---------------------------------------------------------------------------
// CUDA type availability
// ---------------------------------------------------------------------------
#if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA && defined(__has_include)
#  if __has_include(<cuda_runtime.h>)
#    define THEMIS_CUDA_RAII_HAS_CUDA 1
#    include <cuda_runtime.h>
#  endif
#endif
#ifndef THEMIS_CUDA_RAII_HAS_CUDA
#  define THEMIS_CUDA_RAII_HAS_CUDA 0
/// @cond INTERNAL
using cudaStream_t = void*;
using cudaEvent_t  = void*;
/// @endcond
#endif

// ---------------------------------------------------------------------------
// Optional diagnostic helper (no hard dependency on spdlog)
// ---------------------------------------------------------------------------
#if defined(__has_include) && __has_include(<spdlog/spdlog.h>)
#  include <spdlog/spdlog.h>
#  define THEMIS_CUDA_RAII_LOG_WARN(msg) \
       do { auto _l = spdlog::get("gpu"); if (_l) _l->warn(msg); } while(0)
#else
#  define THEMIS_CUDA_RAII_LOG_WARN(msg) ((void)0)
#endif

namespace themis {
namespace gpu {

// ===========================================================================
// CudaStreamGuard
// ===========================================================================

struct CudaStreamGuard {
    cudaStream_t stream{nullptr};

    /**
     * @brief Cuda Stream Guard.
     * @return Return value.
     * @details Calls: cudaStreamCreate(), THEMIS_CUDA_RAII_LOG_WARN().
     */
    explicit CudaStreamGuard() {
#if THEMIS_CUDA_RAII_HAS_CUDA
        if (cudaStreamCreate(&stream) != cudaSuccess) {
            THEMIS_CUDA_RAII_LOG_WARN("CudaStreamGuard: cudaStreamCreate failed");
            stream = nullptr;
        }
#endif
    }

    ~CudaStreamGuard() noexcept {
#if THEMIS_CUDA_RAII_HAS_CUDA
        if (stream) {
            if (cudaStreamDestroy(stream) != cudaSuccess) {
                THEMIS_CUDA_RAII_LOG_WARN("CudaStreamGuard: cudaStreamDestroy failed");
            }
        }
#endif
    }

    CudaStreamGuard(const CudaStreamGuard&) = delete;
    CudaStreamGuard& operator=(const CudaStreamGuard&) = delete;

    CudaStreamGuard(CudaStreamGuard&& o) noexcept
        : stream(std::exchange(o.stream, nullptr)) {}

    CudaStreamGuard& operator=(CudaStreamGuard&& o) noexcept {
        if (this != &o) {
#if THEMIS_CUDA_RAII_HAS_CUDA
            if (stream) {
              cudaStreamDestroy(stream);
            }
#endif
            stream = std::exchange(o.stream, nullptr);
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return stream != nullptr; }

    // -----------------------------------------------------------------------
    // Adoption factory
    // -----------------------------------------------------------------------

    [[nodiscard]] static CudaStreamGuard adopt(cudaStream_t existing) noexcept {
        CudaStreamGuard g;
        g.stream = existing;
        return g;
    }
};

// ===========================================================================
// CudaEventGuard
// ===========================================================================

struct CudaEventGuard {
#if THEMIS_CUDA_RAII_HAS_CUDA
    cudaEvent_t event{nullptr};
#else
    void* event{nullptr};
#endif

    /**
     * @brief Cuda Event Guard.
     * @return Return value.
     * @details Calls: cudaEventCreate(), THEMIS_CUDA_RAII_LOG_WARN().
     */
    explicit CudaEventGuard() {
#if THEMIS_CUDA_RAII_HAS_CUDA
        if (cudaEventCreate(&event) != cudaSuccess) {
            THEMIS_CUDA_RAII_LOG_WARN("CudaEventGuard: cudaEventCreate failed");
            event = nullptr;
        }
#endif
    }

    ~CudaEventGuard() noexcept {
#if THEMIS_CUDA_RAII_HAS_CUDA
        if (event) {
            if (cudaEventDestroy(event) != cudaSuccess) {
                THEMIS_CUDA_RAII_LOG_WARN("CudaEventGuard: cudaEventDestroy failed");
            }
        }
#endif
    }

    CudaEventGuard(const CudaEventGuard&) = delete;
    CudaEventGuard& operator=(const CudaEventGuard&) = delete;

    CudaEventGuard(CudaEventGuard&& o) noexcept
        : event(std::exchange(o.event, nullptr)) {}

    CudaEventGuard& operator=(CudaEventGuard&& o) noexcept {
        if (this != &o) {
#if THEMIS_CUDA_RAII_HAS_CUDA
            if (event) {
              cudaEventDestroy(event);
            }
#endif
            event = std::exchange(o.event, nullptr);
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return event != nullptr; }
};

// ===========================================================================
// CudaDeviceMemoryGuard
// ===========================================================================

struct CudaDeviceMemoryGuard {
    void*  ptr{nullptr};
    size_t bytes{0};

    explicit CudaDeviceMemoryGuard(size_t n_bytes) : bytes(n_bytes) {
#if THEMIS_CUDA_RAII_HAS_CUDA
        if (n_bytes > 0) {
            if (cudaMalloc(&ptr, n_bytes) != cudaSuccess) {
                THEMIS_CUDA_RAII_LOG_WARN("CudaDeviceMemoryGuard: cudaMalloc failed");
                ptr = nullptr;
            }
        }
#endif
    }

    CudaDeviceMemoryGuard() noexcept = default;

    ~CudaDeviceMemoryGuard() noexcept {
#if THEMIS_CUDA_RAII_HAS_CUDA
        if (ptr) {
            cudaFree(ptr);  // errors swallowed: cannot throw in destructor
            ptr = nullptr;
        }
#endif
    }

    CudaDeviceMemoryGuard(const CudaDeviceMemoryGuard&) = delete;
    CudaDeviceMemoryGuard& operator=(const CudaDeviceMemoryGuard&) = delete;

    CudaDeviceMemoryGuard(CudaDeviceMemoryGuard&& o) noexcept
        : ptr(std::exchange(o.ptr, nullptr)),
          bytes(std::exchange(o.bytes, 0)) {}

    CudaDeviceMemoryGuard& operator=(CudaDeviceMemoryGuard&& o) noexcept {
        if (this != &o) {
#if THEMIS_CUDA_RAII_HAS_CUDA
            if (ptr) {
              cudaFree(ptr);
            }
#endif
            ptr  = std::exchange(o.ptr, nullptr);
            bytes = std::exchange(o.bytes, 0);
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return ptr != nullptr; }

    void* release() noexcept {
        bytes = 0;
        return std::exchange(ptr, nullptr);
    }
};

// ===========================================================================
// CudaMemcpyCheck — inline helper (not RAII; documents checked-call pattern)
// ===========================================================================

#if THEMIS_CUDA_RAII_HAS_CUDA
inline bool cudaMemcpyChecked(
    void* dst, const void* src, size_t count, cudaMemcpyKind kind) noexcept
{
    return cudaMemcpy(dst, src, count, kind) == cudaSuccess;
}
#endif

} // namespace gpu
} // namespace themis

// Clean up internal macros
#undef THEMIS_CUDA_RAII_LOG_WARN
