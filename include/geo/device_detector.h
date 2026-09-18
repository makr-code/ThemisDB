/**
 * @file device_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include "themis/gpu/device_discovery.h"

namespace themis {
namespace geo {

constexpr int kGeoMinComputeMajor = 5;
constexpr int kGeoMinComputeMinor = 0;

constexpr uint64_t kGeoMinVramBytes = 128ULL * 1024ULL * 1024ULL; // 128 MiB

struct GeoDeviceCapability {
    themis::gpu::DeviceInfo device;   ///< Underlying device information

    bool meets_compute_requirement = false; ///< Compute capability >= kGeoMin
    bool meets_vram_requirement    = false; ///< Free VRAM >= kGeoMinVramBytes
    bool suitable_for_geo          = false; ///< Both requirements met and healthy

    std::string reason; ///< Human-readable explanation when suitable_for_geo == false
};

class GeoDeviceDetector {
public:
    using EnumerateFn = std::function<std::vector<themis::gpu::DeviceInfo>()>;

    /**
     * @brief Detect.
     * @return Return value.
     */
    static std::vector<GeoDeviceCapability> Detect();

    /**
     * @brief Best Device.
     * @param[in] capabilities Input parameter.
     * @return Return value.
     */
    static GeoDeviceCapability BestDevice(
        const std::vector<GeoDeviceCapability>& capabilities);

    /**
     * @brief Best Device.
     * @return Return value.
     */
    static GeoDeviceCapability BestDevice();

    /**
     * @brief Has Suitable Device.
     * @param[in] capabilities Input parameter.
     * @return True when the operation succeeds.
     */
    static bool HasSuitableDevice(
        const std::vector<GeoDeviceCapability>& capabilities);

    /**
     * @brief Has Suitable Device.
     * @return True when the operation succeeds.
     */
    static bool HasSuitableDevice();

    /**
     * @brief Assess.
     * @param[in] device Input parameter.
     * @return Return value.
     */
    static GeoDeviceCapability Assess(const themis::gpu::DeviceInfo& device);

    /**
     * @brief Report Json.
     * @param[in] capabilities Input parameter.
     * @return Return value.
     */
    static std::string ReportJson(
        const std::vector<GeoDeviceCapability>& capabilities);

    /**
     * @brief Report Json.
     * @return Return value.
     */
    static std::string ReportJson();

    /**
     * @brief Set Enumerate Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), enumerateFnMutex(), enumerateFnStorage(), std::move().
     */
    static void setEnumerateFn(EnumerateFn fn) {
        std::lock_guard<std::mutex> lk(enumerateFnMutex());
        enumerateFnStorage() = std::move(fn);
    }

private:
    /**
     * @brief Enumerate Fn Mutex.
     * @return Return value.
     * @details Implements enumerateFnMutex without additional internal calls.
     */
    static std::mutex& enumerateFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief Enumerate Fn Storage.
     * @return Return value.
     * @details Implements enumerateFnStorage without additional internal calls.
     */
    static EnumerateFn& enumerateFnStorage() {
        static EnumerateFn fn;
        return fn;
    }
};

} // namespace geo
} // namespace themis

