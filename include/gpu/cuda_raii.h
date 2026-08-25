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

/// @brief RAII guard for CUDA stream lifecycle management.
/// @details Ensures cudaStreamDestroy is called when guard goes out of scope.
///          Thread-safe: each guard owns exactly one stream.
/// @throws Does not throw; CUDA errors logged via GpuDiagnostics.
///
/// ### CUDA-CALL-AUDIT note
/// Addresses raw `cudaStreamCreate` / `cudaStreamDestroy` sites in:
/// - `src/gpu/stream_manager.cpp` (6 raw sites — wrapper available)
/// - `src/gpu/cuda_operations.cpp` (CudaStream class — wrapper available)
/// Those files already perform manual create/destroy correctly; this guard
/// provides a drop-in for new call sites and documents the available pattern.
///
/// Activation: GPU path active (non-CPU-fallback).
/// Production Delta: none — wraps raw CUDA API for exception safety.
/// Removal Plan: N/A — permanent RAII governance.
struct CudaStreamGuard {
    cudaStream_t stream{nullptr};

    /// @brief Construct and create a new CUDA stream.
    /// @post  isValid() == true on success; stream == nullptr on failure.
    explicit CudaStreamGuard() {
#if THEMIS_CUDA_RAII_HAS_CUDA
        if (cudaStreamCreate(&stream) != cudaSuccess) {
            THEMIS_CUDA_RAII_LOG_WARN("CudaStreamGuard: cudaStreamCreate failed");
            stream = nullptr;
        }
#endif
    }

    /// @brief Destructor — destroys the stream unconditionally.
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

    /// @brief Move constructor — transfers ownership; source becomes empty.
    CudaStreamGuard(CudaStreamGuard&& o) noexcept
        : stream(std::exchange(o.stream, nullptr)) {}

    /// @brief Move assignment — destroys current stream then takes ownership.
    CudaStreamGuard& operator=(CudaStreamGuard&& o) noexcept {
        if (this != &o) {
#if THEMIS_CUDA_RAII_HAS_CUDA
            if (stream) cudaStreamDestroy(stream);
#endif
            stream = std::exchange(o.stream, nullptr);
        }
        return *this;
    }

    /// @return true if the stream was created successfully.
    [[nodiscard]] bool isValid() const noexcept { return stream != nullptr; }

    // -----------------------------------------------------------------------
    // Adoption factory
    // -----------------------------------------------------------------------

    /// @brief Adopt ownership of an existing, already-created CUDA stream.
    ///
    /// The guard takes ownership: the caller must not destroy the stream
    /// after calling this function.  Useful when stream creation is
    /// interleaved with fallback logic (e.g., cudaSetDevice + cudaStreamCreate
    /// in a try-else path) and the raw handle needs to be transferred to RAII
    /// after successful creation.
    ///
    /// @param existing  A valid cudaStream_t that was created by the caller.
    ///                  Passing nullptr is allowed and produces an empty guard.
    /// @return          A CudaStreamGuard that owns `existing`.
    ///
    /// ### CUDA-CALL-AUDIT note (Wave A — Phase C prerequisite)
    /// Adoption targets:
    /// - `src/gpu/stream_manager.cpp` createStream path: raw handle stored as
    ///   `uintptr_t cuda_stream`; migrated to adopt().
    /// - `src/gpu/stream_manager.cpp` createCudaStream path: raw handle
    ///   registered in `cudaStreamRegistry()`; migrated to adopt() in Stream.
    ///
    /// Activation: CUDA-enabled build (THEMIS_ENABLE_CUDA=1).
    /// Production Delta: none — identical lifecycle; ownership transferred.
    /// Removal Plan: N/A — permanent RAII governance.
    [[nodiscard]] static CudaStreamGuard adopt(cudaStream_t existing) noexcept {
        CudaStreamGuard g;
        g.stream = existing;
        return g;
    }
};

// ===========================================================================
// CudaEventGuard
// ===========================================================================

/// @brief RAII guard for CUDA event lifecycle management.
/// @details Ensures cudaEventDestroy is called when guard goes out of scope.
///          Thread-safe: each guard owns exactly one event.
/// @throws Does not throw; CUDA errors logged via GpuDiagnostics.
///
/// ### CUDA-CALL-AUDIT note
/// Addresses raw `cudaEventCreate` / `cudaEventDestroy` sites in:
/// - `src/gpu/cuda_operations.cpp` (CudaOperation class — wrapper available)
/// - `src/gpu/gpu_resource_handles.cpp` (manual create/destroy — wrapper available)
/// Those files already perform manual lifecycle correctly; this guard is the
/// canonical pattern for new call sites.
///
/// Activation: GPU path active (non-CPU-fallback).
/// Production Delta: none — wraps raw CUDA API for exception safety.
/// Removal Plan: N/A — permanent RAII governance.
struct CudaEventGuard {
#if THEMIS_CUDA_RAII_HAS_CUDA
    cudaEvent_t event{nullptr};
#else
    void* event{nullptr};
#endif

    /// @brief Construct and create a new CUDA event.
    /// @post  isValid() == true on success; event == nullptr on failure.
    explicit CudaEventGuard() {
#if THEMIS_CUDA_RAII_HAS_CUDA
        if (cudaEventCreate(&event) != cudaSuccess) {
            THEMIS_CUDA_RAII_LOG_WARN("CudaEventGuard: cudaEventCreate failed");
            event = nullptr;
        }
#endif
    }

    /// @brief Destructor — destroys the event unconditionally.
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

    /// @brief Move constructor — transfers ownership; source becomes empty.
    CudaEventGuard(CudaEventGuard&& o) noexcept
        : event(std::exchange(o.event, nullptr)) {}

    /// @brief Move assignment — destroys current event then takes ownership.
    CudaEventGuard& operator=(CudaEventGuard&& o) noexcept {
        if (this != &o) {
#if THEMIS_CUDA_RAII_HAS_CUDA
            if (event) cudaEventDestroy(event);
#endif
            event = std::exchange(o.event, nullptr);
        }
        return *this;
    }

    /// @return true if the event was created successfully.
    [[nodiscard]] bool isValid() const noexcept { return event != nullptr; }
};

// ===========================================================================
// CudaDeviceMemoryGuard
// ===========================================================================

/// @brief RAII guard for raw CUDA device memory (typed, byte-count-aware).
/// @details Wraps cudaMalloc / cudaFree.  Use `DeviceMemoryGuard<T>` from
///          `gpu_safe_raii.h` or `GPUMemoryHandle<T>` from
///          `gpu_raii_wrappers.hpp` for richer functionality; this guard
///          targets the small number of remaining raw cudaMalloc/cudaFree
///          sites that do not yet use those wrappers.
/// @throws Does not throw; CUDA errors logged via GpuDiagnostics.
///
/// ### CUDA-CALL-AUDIT note
/// Addresses raw `cudaMalloc` / `cudaFree` call sites in:
/// - `src/gpu/unified_memory_coordinator.cpp:140` (cudaFree in destructor,
///   no error check — pattern documented below as acceptable destructor pattern)
/// - `src/gpu/gpu_memory_pool_safety.cpp:44`    (cudaFree in destructor —
///   same: acceptable in destructor, guard available for new code)
/// Those destructor-only frees are acceptable (can't throw in destructor);
/// this guard prevents new call sites from introducing the same raw pattern.
///
/// Activation: GPU path active (non-CPU-fallback).
/// Production Delta: none — wraps raw CUDA API for exception safety.
/// Removal Plan: N/A — permanent RAII governance.
struct CudaDeviceMemoryGuard {
    void*  ptr{nullptr};
    size_t bytes{0};

    /// @brief Allocate `n_bytes` of CUDA device memory.
    /// @post  isValid() == true on success; ptr == nullptr on failure.
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

    /// @brief Default constructor — empty (no allocation).
    CudaDeviceMemoryGuard() noexcept = default;

    /// @brief Destructor — frees device memory unconditionally.
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

    /// @brief Move constructor — transfers ownership; source becomes empty.
    CudaDeviceMemoryGuard(CudaDeviceMemoryGuard&& o) noexcept
        : ptr(std::exchange(o.ptr, nullptr)),
          bytes(std::exchange(o.bytes, 0)) {}

    /// @brief Move assignment — frees current then takes ownership.
    CudaDeviceMemoryGuard& operator=(CudaDeviceMemoryGuard&& o) noexcept {
        if (this != &o) {
#if THEMIS_CUDA_RAII_HAS_CUDA
            if (ptr) cudaFree(ptr);
#endif
            ptr  = std::exchange(o.ptr, nullptr);
            bytes = std::exchange(o.bytes, 0);
        }
        return *this;
    }

    /// @return true if device memory was allocated successfully.
    [[nodiscard]] bool isValid() const noexcept { return ptr != nullptr; }

    /// @brief Release ownership without freeing.  Caller must cudaFree.
    void* release() noexcept {
        bytes = 0;
        return std::exchange(ptr, nullptr);
    }
};

// ===========================================================================
// CudaMemcpyCheck — inline helper (not RAII; documents checked-call pattern)
// ===========================================================================

/// @brief Execute a cudaMemcpy and return true on success, false on failure.
/// @details Inline helper used to enforce the "no unchecked cudaMemcpy" rule
///          in call sites that do not yet use `CUDA_CHECK` or `CHECKED_CUDA`.
///
/// ### CUDA-CALL-AUDIT note
/// Remaining unchecked `cudaMemcpy` sites identified in audit:
/// - `src/gpu/gpu_memory_allocator.cpp` lines 226-227, 254, 271
///   → error return already captured in local `cudaError_t err`; checked.
/// - `src/gpu/gpu_kernel_manager.cpp`   lines 255, 266
///   → same pattern; already checked.
/// - `src/gpu/memory_pool.cpp`          line 398
///   → error captured and logged; already checked.
/// - `src/gpu/p2p_transfer.cpp`         line 319
///   → `cudaMemcpyPeer`; error captured and propagated; already checked.
/// - `src/gpu/query_accelerator.cpp`    lines 1248, 1265, 1280, 1282
///   → wrapped with `CHECKED_CUDA` macro; already safe.
///
/// All identified `cudaMemcpy` sites capture the return value.
/// No **completely** unchecked sites remain; partial-check sites are noted
/// above and marked for tightening in the next hardening pass.
///
/// Activation: GPU path active.
/// Production Delta: none — documents existing pattern, provides helper.
/// Removal Plan: N/A — permanent governance helper.
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
