/**
 * @file device_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "acceleration/compute_backend.h"

namespace themis {
namespace acceleration {

class DeviceManager {
public:
    using EnumerateFn = std::function<std::vector<DeviceCapabilityInfo>()>;

    /**
     * @brief Instance.
     * @return Return value.
     */
    static DeviceManager& instance();

    /**
     * @brief Probe Devices.
     * @return Return value.
     */
    std::vector<DeviceCapabilityInfo> probeDevices();

    /**
     * @brief Refresh.
     * @return Return value.
     */
    std::vector<DeviceCapabilityInfo> refresh();

    /**
     * @brief Get Best Device.
     * @return Return value.
     */
    DeviceCapabilityInfo getBestDevice();

    /**
     * @brief Has GPU.
     * @return True when the operation succeeds.
     */
    bool hasGPU();

    /**
     * @brief Best Backend Type.
     * @return Return value.
     */
    BackendType bestBackendType();

    /**
     * @brief Log Device Info.
     */
    void logDeviceInfo();

    /**
     * @brief Set Enumerate Fn.
     * @param[in] fn Input parameter.
     */
    static void setEnumerateFn(EnumerateFn fn);

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
