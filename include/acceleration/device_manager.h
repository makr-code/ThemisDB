/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            device_manager.h                                   ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:05:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     96                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#include "acceleration/compute_backend.h"

namespace themis {
namespace acceleration {

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
