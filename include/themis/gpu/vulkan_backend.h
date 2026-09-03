/**
 * @file vulkan_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
 * @brief Vulkan compute backend for cross-vendor GPU support.
 *
 * Provides Vulkan-backed equivalents of the GPU compute features used by the
 * GPU module, enabling cross-vendor acceleration on AMD, Intel, ARM, Qualcomm,
 * and NVIDIA hardware without requiring vendor-specific drivers (CUDA/HIP).
 *
 *  - Compute dispatch via `GPULauncher::BackendFn`
 *  - Named stream lifecycle (logical streams backed by Vulkan command queues)
 *  - Device availability query
 *
 * All real Vulkan calls are gated behind `THEMIS_ENABLE_VULKAN`.  When the
 * define is absent (e.g. CI without Vulkan SDK) the backend transparently falls
 * back to CPU execution so that `GPUStreamManager` and `GPULauncher` continue
 * to work correctly. Fallback and gate denials emit structured
 * `GPUBackendDispatchDiagnostics` events (`BACKEND_NOT_ENABLED`,
 * `BACKEND_NO_DEVICE_AVAILABLE`, `FALLBACK_CPU_DEGRADED`).
 *
 * Integration points
 * ------------------
 * - `createBackendFn(device_index)` — returns a `GPULauncher::BackendFn` that
 *   can be passed directly to `GPUStreamManager::createStream()` or
 *   `GPULauncher`.  On systems without Vulkan the returned function succeeds via
 *   the CPU path.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class VulkanComputeBackend {
public:
    // -----------------------------------------------------------------------
    // Operation result
    // -----------------------------------------------------------------------
    struct Result {
        bool        ok = false;
        std::string error_message;
    };

    // -----------------------------------------------------------------------
    // Logical stream handle (maps to a Vulkan command queue / pool)
    // -----------------------------------------------------------------------
    struct StreamHandle {
        /**
         * @brief Underlying VkQueue cast to uintptr_t; 0 = not created.
         *
         * In the current CPU-simulation build this field stores
         * `device_index + 1` as a non-zero sentinel so that `is_valid()`
         * returns true on real hardware.  When `THEMIS_ENABLE_VULKAN` is
         * active and a real VkQueue is obtained it will hold the actual
         * queue handle cast to uintptr_t.
         */
        uintptr_t   native       = 0;
        int         device_index = -1;
        std::string name;

        /** @brief True when the native handle has been associated. */
        bool is_valid() const noexcept { return native != 0; }
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static VulkanComputeBackend& GetInstance() {
        static VulkanComputeBackend instance;
        return instance;
    }

    // -----------------------------------------------------------------------
    // Device query
    // -----------------------------------------------------------------------

    /**
     * @brief Number of Vulkan-capable physical devices visible to the process.
     *
     * Returns 0 when `THEMIS_ENABLE_VULKAN` is not defined or when no Vulkan
     * device is detected at runtime.
     */
    int deviceCount() const;

    /**
     * @brief True when at least one Vulkan compute device is available.
     */
    bool isAvailable() const;

    /**
     * @brief Human-readable name of the vendor for the selected device.
     *
     * Returns "Unknown" when Vulkan is not available or the vendor cannot
     * be determined from the PCI vendor ID.
     */
    std::string vendorName() const;

    // -----------------------------------------------------------------------
    // Launcher backend
    // -----------------------------------------------------------------------

    /**
     * @brief Create a `GPULauncher::BackendFn` backed by Vulkan compute.
     *
     * The returned function:
     *  - On systems **with** Vulkan: selects a compute-capable device and
     *    dispatches the work item through the Vulkan acceleration backend.
     *    Falls back to CPU when the Vulkan backend is not initialized.
     *  - On systems **without** Vulkan: returns `true` immediately (CPU path).
     *
     * The function is safe to move into `GPULauncher` or
     * `GPUStreamManager::createStream()`.
     *
     * @param device_index  Vulkan physical device ordinal (0-based, ignored when
     *                      Vulkan is unavailable).
     */
    GPULauncher::BackendFn createBackendFn(int device_index = 0);

    // -----------------------------------------------------------------------
    // Stream management (logical streams backed by Vulkan command queues)
    // -----------------------------------------------------------------------

    /**
     * @brief Register a named logical compute stream on @p device_index.
     *
     * On systems without Vulkan records a virtual stream entry so that
     * `hasStream()` / `streamNames()` still work correctly.
     *
     * @return ok == false when a stream with @p name already exists or
     *         @p name is empty.
     */
    Result createStream(const std::string& name, int device_index = 0);

    /**
     * @brief Unregister and release a named logical compute stream.
     *
     * @return ok == false when no stream with @p name exists.
     */
    Result destroyStream(const std::string& name);

    /**
     * @brief Block until all pending work on the named stream completes.
     *
     * On the CPU fallback path this is a no-op that returns ok == true.
     * @return ok == false when the stream does not exist.
     */
    Result synchronizeStream(const std::string& name);

    /** @brief Return the handle for a named stream (invalid if not found). */
    StreamHandle getStream(const std::string& name) const;

    /** @brief True when a stream with @p name has been created. */
    bool hasStream(const std::string& name) const;

    /** @brief Return all registered stream names. */
    std::vector<std::string> streamNames() const;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t streams_created   = 0;  ///< createStream calls succeeded
        size_t streams_destroyed = 0;  ///< destroyStream calls succeeded
        size_t dispatched        = 0;  ///< Work items dispatched successfully
        size_t dispatch_errors   = 0;  ///< Work items that failed dispatch
        size_t cpu_fallbacks     = 0;  ///< Items routed to CPU (no Vulkan hw)
    };

    Stats getStats() const;

    /**
     * @brief Reset all statistics counters (for testing).
     *
     * Does not affect existing streams.
     */
    void resetStats();

private:
    VulkanComputeBackend() = default;

    VulkanComputeBackend(const VulkanComputeBackend&)            = delete;
    VulkanComputeBackend& operator=(const VulkanComputeBackend&) = delete;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, StreamHandle> streams_;
    Stats stats_;

    // Cached device count (populated lazily on first call to isAvailable()).
    mutable int         cached_device_count_ = -1;  // -1 = not yet probed
    mutable bool        vulkan_initialized_  = false;
    mutable std::string cached_vendor_name_;         // populated by probeDevices()

    // Internal: probe the Vulkan loader and count compute-capable devices.
    // Called under mutex_ the first time isAvailable() / deviceCount() is used.
    void probeDevices() const;
};

} // namespace gpu
} // namespace themis
