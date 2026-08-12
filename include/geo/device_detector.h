/**
 * @file device_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Minimum compute capability (major.minor) required for geo CUDA kernels.
 *
 * CUDA devices below compute capability 5.0 lack atomic operations and
 * warp-shuffle instructions relied upon by the geo distance/containment kernels.
 */
constexpr int kGeoMinComputeMajor = 5;
constexpr int kGeoMinComputeMinor = 0;

/**
 * @brief Minimum VRAM (bytes) required to run the geo GPU backend.
 *
 * The geo batch kernels allocate point arrays and result buffers; 128 MiB is
 * the practical minimum for a 65 536-element batch.
 */
constexpr uint64_t kGeoMinVramBytes = 128ULL * 1024ULL * 1024ULL; // 128 MiB

/**
 * @brief Capability assessment for a single GPU device from the geo module's
 *        perspective.
 */
struct GeoDeviceCapability {
    themis::gpu::DeviceInfo device;   ///< Underlying device information

    bool meets_compute_requirement = false; ///< Compute capability >= kGeoMin
    bool meets_vram_requirement    = false; ///< Free VRAM >= kGeoMinVramBytes
    bool suitable_for_geo          = false; ///< Both requirements met and healthy

    std::string reason; ///< Human-readable explanation when suitable_for_geo == false
};

/**
 * @brief Detect and report GPU devices available for the geo module.
 *
 * Wraps themis::gpu::DeviceDiscovery with geo-specific capability checks so
 * that the GPU backend and the admin / observability layer can determine
 * device suitability without duplicating logic.
 *
 * All methods are static and stateless — thread-safe to call concurrently.
 */
class GeoDeviceDetector {
public:
    using EnumerateFn = std::function<std::vector<themis::gpu::DeviceInfo>()>;

    /**
     * @brief Enumerate all devices and assess their geo capability.
     *
     * Calls DeviceDiscovery::Enumerate() and evaluates each device against
     * the geo compute-capability and VRAM thresholds.  Always returns at
     * least one entry (CPU_FALLBACK sentinel) so callers never receive an
     * empty list.
     */
    static std::vector<GeoDeviceCapability> Detect();

    /**
     * @brief Return the best device suited for geo GPU operations.
     *
     * Selects the suitable device with the most free VRAM.  Returns the
     * CPU_FALLBACK sentinel capability when no suitable GPU device is present.
     *
     * @param capabilities  List produced by Detect() (avoids re-enumeration).
     */
    static GeoDeviceCapability BestDevice(
        const std::vector<GeoDeviceCapability>& capabilities);

    /**
     * @brief Convenience overload: detect then pick the best device.
     */
    static GeoDeviceCapability BestDevice();

    /**
     * @brief True when at least one device suitable for geo operations exists.
     *
     * @param capabilities  List produced by Detect() (avoids re-enumeration).
     */
    static bool HasSuitableDevice(
        const std::vector<GeoDeviceCapability>& capabilities);

    /**
     * @brief Convenience overload: detect then check.
     */
    static bool HasSuitableDevice();

    /**
     * @brief Assess a single DeviceInfo against geo requirements.
     *
     * Useful for unit testing individual device records without invoking the
     * full discovery path.
     */
    static GeoDeviceCapability Assess(const themis::gpu::DeviceInfo& device);

    /**
     * @brief Serialize the capability list to a JSON string for the admin API.
     *
     * The returned object has the shape:
     * @code
     * {
     *   "has_suitable_device": <bool>,
     *   "devices": [
     *     {
     *       "index": <int>,
     *       "name": "<string>",
     *       "backend": "<string>",
     *       "total_vram_mb": <uint64>,
     *       "free_vram_mb": <uint64>,
     *       "compute_capability": "<major>.<minor>",
     *       "is_healthy": <bool>,
     *       "suitable_for_geo": <bool>,
     *       "reason": "<string>"
     *     },
     *     ...
     *   ]
     * }
     * @endcode
     *
     * @param capabilities  List produced by Detect() (avoids re-enumeration).
     */
    static std::string ReportJson(
        const std::vector<GeoDeviceCapability>& capabilities);

    /**
     * @brief Convenience overload: detect then serialise.
     */
    static std::string ReportJson();

    /// Register a custom device enumeration bridge for CPU-only or test builds.
    /// Thread-safe; pass an empty function to fall back to DeviceDiscovery::Enumerate().
    static void setEnumerateFn(EnumerateFn fn) {
        std::lock_guard<std::mutex> lk(enumerateFnMutex());
        enumerateFnStorage() = std::move(fn);
    }

private:
    static std::mutex& enumerateFnMutex() {
        static std::mutex m;
        return m;
    }
    static EnumerateFn& enumerateFnStorage() {
        static EnumerateFn fn;
        return fn;
    }
};

} // namespace geo
} // namespace themis

