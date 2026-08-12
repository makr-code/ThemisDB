/**
 * @file rocm_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "themis/gpu/launcher.h"

namespace themis {
namespace gpu {

/**
 * @brief ROCm/HIP backend providing feature parity with the CUDA backend.
 *
 * Provides HIP-backed equivalents of the CUDA features used by the GPU module:
 *  - Device memory allocation / deallocation (`hipMalloc` / `hipFree`)
 *  - Device memory zeroing (`hipMemset`)
 *  - Async stream lifecycle (`hipStreamCreate` / `hipStreamDestroy` /
 *    `hipStreamSynchronize`)
 *  - Kernel dispatch via `GPULauncher::BackendFn`
 *
 * All real HIP calls are gated behind `THEMIS_ENABLE_HIP`.  When the define
 * is absent (e.g. CI without AMD GPU hardware) the backend transparently falls
 * back to CPU execution so that `GPUStreamManager`, `GPUMemoryPool`, and
 * `GPULauncher` continue to work correctly.
 *
 * Integration points
 * ------------------
 * - `createBackendFn(device_index)` — returns a `GPULauncher::BackendFn` that
 *   can be passed directly to `GPUStreamManager::createStream()` or
 *   `GPULauncher`.  On systems without HIP the returned function succeeds via
 *   the CPU path.
 * - `allocate(size_bytes, tag)` / `deallocate(rec)` — wraps `hipMalloc` /
 *   `hipFree` with ownership tracking.  Use these from `GPUMemoryPool` when
 *   backing a real device memory pool.
 * - `zeroMemory(device_ptr, size_bytes)` — wraps `hipMemset`; used by
 *   `GPUMemoryPool::release()` when `zero_on_free` is set.
 * - `createStream(name, device_index)` / `destroyStream(name)` /
 *   `synchronizeStream(name)` — HIP stream lifecycle, mirroring CUDA streams.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class ROCmBackend {
public:
    // -----------------------------------------------------------------------
    // Operation result
    // -----------------------------------------------------------------------
    struct Result {
        bool        ok            = false;
        std::string error_message;
    };

    // -----------------------------------------------------------------------
    // HIP stream handle (opaque wrapper)
    // -----------------------------------------------------------------------
    struct StreamHandle {
        /** @brief Underlying hipStream_t cast to uintptr_t; 0 = not created. */
        uintptr_t   native       = 0;
        int         device_index = -1;
        std::string name;

        /** @brief True when the native handle has been created. */
        bool is_valid() const noexcept { return native != 0; }
    };

    // -----------------------------------------------------------------------
    // Device memory allocation record
    // -----------------------------------------------------------------------
    struct AllocationRecord {
        /** @brief hipMalloc return value cast to uintptr_t; 0 = invalid. */
        uintptr_t   device_ptr = 0;
        size_t      size_bytes = 0;
        std::string tag;

        /** @brief True when the allocation holds a real device pointer. */
        bool is_valid() const noexcept { return device_ptr != 0; }
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static ROCmBackend& GetInstance() {
        static ROCmBackend instance;
        return instance;
    }

    // -----------------------------------------------------------------------
    // Device query
    // -----------------------------------------------------------------------

    /**
     * @brief Number of HIP-capable devices visible to the process.
     *
     * Returns 0 when `THEMIS_ENABLE_HIP` is not defined or when no device is
     * detected at runtime.
     */
    int deviceCount() const;

    /**
     * @brief True when at least one HIP device is available and responsive.
     */
    bool isAvailable() const;

    // -----------------------------------------------------------------------
    // Launcher backend
    // -----------------------------------------------------------------------

    /**
     * @brief Create a `GPULauncher::BackendFn` backed by HIP.
     *
     * The returned function:
     *  - On systems **with** HIP: selects @p device_index, then executes the
     *    work item.  Kernel blob dispatch via `.hsaco` is a hardware-only
     *    feature; on a device without loaded kernels the call succeeds (no-op
     *    kernel).
     *  - On systems **without** HIP: returns `true` immediately (CPU path).
     *
     * The function is safe to move into `GPULauncher` or
     * `GPUStreamManager::createStream()`.
     *
     * @param device_index  Target HIP device index (0-based).
     */
    GPULauncher::BackendFn createBackendFn(int device_index = 0);

    // -----------------------------------------------------------------------
    // Stream management (HIP parity with CUDA)
    // -----------------------------------------------------------------------

    /**
     * @brief Create a named HIP stream on @p device_index.
     *
     * On systems without HIP records a virtual stream entry so that
     * `hasStream()` / `streamNames()` still work correctly.
     *
    * @return ok == false only when a stream with @p name already exists.
    *         If HIP stream creation fails at runtime, a virtual stream entry
    *         is registered so fallback execution remains available.
     */
    Result createStream(const std::string& name, int device_index = 0);

    /**
     * @brief Destroy a named HIP stream.
     *
     * Calls `hipStreamDestroy` when `THEMIS_ENABLE_HIP` is active.
     * @return ok == false when no stream with @p name exists.
     */
    Result destroyStream(const std::string& name);

    /**
     * @brief Block until all work enqueued on the named stream completes.
     *
     * Calls `hipStreamSynchronize` when `THEMIS_ENABLE_HIP` is active.
     * @return ok == false when the stream does not exist or synchronization
     *         fails.
     */
    Result synchronizeStream(const std::string& name);

    /** @brief Return the handle for a named stream (invalid if not found). */
    StreamHandle getStream(const std::string& name) const;

    /** @brief True when a stream with @p name has been created. */
    bool hasStream(const std::string& name) const;

    /** @brief Return all registered stream names. */
    std::vector<std::string> streamNames() const;

    // -----------------------------------------------------------------------
    // Device memory (HIP parity with CUDA)
    // -----------------------------------------------------------------------

    /**
     * @brief Allocate @p size_bytes of device memory via `hipMalloc`.
     *
     * On systems without HIP returns an invalid record
     * (`AllocationRecord::device_ptr == 0`).
     *
     * @param size_bytes  Bytes to allocate.
     * @param tag         Owner / reason label for diagnostics.
     * @return AllocationRecord; check `is_valid()` before use.
     */
    AllocationRecord allocate(size_t size_bytes, const std::string& tag = "");

    /**
     * @brief Release device memory previously returned by `allocate()`.
     *
     * Calls `hipFree` on the stored pointer; safe to call with an invalid
     * record.  Clears @p rec on success.
     */
    Result deallocate(AllocationRecord& rec);

    /**
     * @brief Fill device memory with zeros via `hipMemset`.
     *
     * Falls back to `std::memset` on a CPU-side buffer when HIP is absent.
     * Used by `GPUMemoryPool::release()` when `zero_on_free` is set.
     *
     * @param device_ptr  Device pointer (hipMalloc result cast to uintptr_t).
     * @param size_bytes  Bytes to zero.
     */
    Result zeroMemory(uintptr_t device_ptr, size_t size_bytes);

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t streams_created    = 0;  ///< hipStreamCreate calls succeeded
        size_t streams_destroyed  = 0;  ///< hipStreamDestroy calls succeeded
        size_t alloc_count        = 0;  ///< hipMalloc calls succeeded
        size_t dealloc_count      = 0;  ///< hipFree calls succeeded
        size_t bytes_allocated    = 0;  ///< Current live device bytes
    };

    Stats getStats() const;

    /**
     * @brief Reset all statistics counters (for testing).
     *
     * Does not affect existing streams or allocations.
     */
    void resetStats();

private:
    ROCmBackend() = default;

    ROCmBackend(const ROCmBackend&)            = delete;
    ROCmBackend& operator=(const ROCmBackend&) = delete;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, StreamHandle> streams_;
    std::vector<AllocationRecord> active_allocations_;
    Stats stats_;
};

} // namespace gpu
} // namespace themis
