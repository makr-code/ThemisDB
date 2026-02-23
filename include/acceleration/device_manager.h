#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#include "acceleration/compute_backend.h"

namespace themis {
namespace acceleration {

/**
 * @brief Per-device capability snapshot as seen by the acceleration module.
 *
 * Populated by DeviceManager::probeDevices().  Fields are translated from
 * themis::gpu::DeviceInfo and augmented with acceleration-specific capability
 * flags derived from the device's compute capability.
 */
struct DeviceCapabilityInfo {
    int         index             = -1;      ///< Driver device index (-1 for CPU fallback)
    std::string name;                        ///< Human-readable device name
    BackendType backend_type      = BackendType::CPU;
    uint64_t    total_vram_bytes  = 0;       ///< Total VRAM reported by driver
    uint64_t    free_vram_bytes   = 0;       ///< Free VRAM at probe time
    int         compute_major     = 0;       ///< Compute capability major (CUDA/ROCm)
    int         compute_minor     = 0;       ///< Compute capability minor
    bool        is_healthy        = true;    ///< false when the device reported an error
    std::string error_message;              ///< Non-empty when is_healthy == false

    // Derived precision support flags
    bool        supports_fp16     = false;   ///< true for CUDA sm_70+ / ROCm gfx900+
    bool        supports_bf16     = false;   ///< true for CUDA sm_80+ (Ampere and newer)
};

/**
 * @brief Acceleration-layer runtime device capability manager.
 *
 * Wraps themis::gpu::DeviceDiscovery to provide the acceleration module with
 * a device-capability view that includes BackendType mapping and precision
 * support flags.  Probe results are cached for @c kCacheTTL (60 seconds) and
 * refreshed on explicit refresh() or when the TTL expires.
 *
 * Design notes
 * ------------
 * - Singleton; created on first access via instance().
 * - All public methods are thread-safe.
 * - Real CUDA/ROCm probing is delegated to themis::gpu::DeviceDiscovery, which
 *   guards hardware calls behind THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP.
 * - When no real GPU runtime is present, a CPU_FALLBACK sentinel is returned
 *   so callers always receive a non-empty device list.
 */
class DeviceManager {
public:
    /// Singleton accessor.
    static DeviceManager& instance();

    /// Enumerate all available compute devices.
    /// Returns cached results if the cache is valid and within kCacheTTL.
    std::vector<DeviceCapabilityInfo> probeDevices();

    /// Force a fresh hardware probe, ignoring the cache.
    /// Equivalent to invalidating the cache and calling probeDevices().
    std::vector<DeviceCapabilityInfo> refresh();

    /// Return the best available device (highest free VRAM among healthy
    /// GPU devices, or CPU fallback when no real GPU is present).
    DeviceCapabilityInfo getBestDevice();

    /// True when at least one healthy real (non-CPU) device is present.
    bool hasGPU();

    /// BackendType of the best available device.
    BackendType bestBackendType();

    /// Emit a structured log line (to std::cout) listing all probed devices
    /// and the selected best device.  Intended for startup observability.
    void logDeviceInfo();

    /// Cache time-to-live: probe results are considered fresh for 60 seconds.
    static constexpr std::chrono::seconds kCacheTTL{60};

private:
    DeviceManager() = default;
    ~DeviceManager() = default;
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    mutable std::mutex mutex_;
    std::vector<DeviceCapabilityInfo> cached_;
    std::chrono::steady_clock::time_point cache_time_{};
    bool cache_valid_ = false;
};

} // namespace acceleration
} // namespace themis
