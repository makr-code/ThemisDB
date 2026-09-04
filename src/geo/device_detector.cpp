/**
 * @file device_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Runtime GPU device detection and capability reporting for the geo module.
//
// Wraps themis::gpu::DeviceDiscovery with geo-specific compute-capability and
// VRAM threshold checks.  The GPU backend stub uses these checks on start-up
// to decide whether to attempt GPU dispatch or fall back to the CPU path.
//
// Thread safety: all methods are static and stateless; safe for concurrent use.

#include "geo/device_detector.h"

#include <sstream>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Build a GeoDeviceCapability for the CPU-fallback sentinel device.
/// STUB/SIMULATION NOTE:
/// Purpose: Return a well-defined "no GPU available" sentinel so that callers
///   of `GeoDeviceDetector::Detect()` can always inspect the device list
///   without special-casing an empty result.  The sentinel device has
///   `suitable_for_geo = false` and `reason = "no GPU device available; using
///   CPU fallback"`, causing `GpuBatchBackend` to route all geo ops to the
///   CPU path.
/// Activation: Called when `themis::gpu::DeviceDiscovery::queryDevices()`
///   returns an empty list (no GPU drivers or devices detected at runtime).
/// Production Delta: All geo spatial operations run on the CPU exact backend.
///   GPU distance and containment kernels are not invoked; expected ≥ 8× GPU
///   speedup is absent.
/// Removal Plan: Ensure a CUDA or HIP-capable GPU is present and that the
///   CUDA/ROCm driver is installed.  `DeviceDiscovery::queryDevices()` will
///   then return real devices and this sentinel path will not be taken.
/// Roadmap ref: src/geo/FUTURE_ENHANCEMENTS.md §"CUDA Geospatial Kernels"
static GeoDeviceCapability MakeCpuFallbackCapability() {
    GeoDeviceCapability cap;
    cap.device.index              = -1;
    cap.device.device_index       = -1;
    cap.device.name               = "CPU Fallback";
    cap.device.backend            = "CPU_FALLBACK";
    cap.device.is_healthy         = true;
    cap.meets_compute_requirement = false;
    cap.meets_vram_requirement    = false;
    cap.suitable_for_geo          = false;
    cap.reason                    = "no GPU device available; using CPU fallback";
    return cap;
}

} // namespace

// ---------------------------------------------------------------------------
// GeoDeviceDetector — public static methods
// ---------------------------------------------------------------------------

GeoDeviceCapability GeoDeviceDetector::Assess(const themis::gpu::DeviceInfo &device) {
    GeoDeviceCapability cap;
    cap.device = device;

    if (device.backend == "CPU_FALLBACK") {
        cap.meets_compute_requirement = false;
        cap.meets_vram_requirement    = false;
        cap.suitable_for_geo          = false;
        cap.reason                    = "no GPU device available; using CPU fallback";
        return cap;
    }

    if (!device.is_healthy) {
        cap.meets_compute_requirement = false;
        cap.meets_vram_requirement    = false;
        cap.suitable_for_geo          = false;
        cap.reason                    = "device is unhealthy: " + device.error_message;
        return cap;
    }

    cap.meets_compute_requirement
        = (device.compute_major > kGeoMinComputeMajor)
          || (device.compute_major == kGeoMinComputeMajor && device.compute_minor >= kGeoMinComputeMinor);

    cap.meets_vram_requirement = (device.free_vram_bytes >= kGeoMinVramBytes);

    if (!cap.meets_compute_requirement) {
        cap.suitable_for_geo = false;
        cap.reason           = "compute capability " + std::to_string(device.compute_major) + "."
                               + std::to_string(device.compute_minor) + " is below the required minimum "
                               + std::to_string(kGeoMinComputeMajor) + "." + std::to_string(kGeoMinComputeMinor);
        return cap;
    }

    if (!cap.meets_vram_requirement) {
        cap.suitable_for_geo = false;
        cap.reason = "free VRAM (" + std::to_string(device.free_vram_bytes / (1024 * 1024))
                     + " MiB) is below the required minimum " + std::to_string(kGeoMinVramBytes / (1024 * 1024))
                     + " MiB";
        return cap;
    }

    cap.suitable_for_geo = true;
    cap.reason.clear();
    return cap;
}

std::vector<GeoDeviceCapability> GeoDeviceDetector::Detect() {
    EnumerateFn fn;
    {
        std::lock_guard<std::mutex> lk(GeoDeviceDetector::enumerateFnMutex());
        fn = GeoDeviceDetector::enumerateFnStorage();
    }
    const auto raw = fn ? fn() : themis::gpu::DeviceDiscovery::Enumerate();

    std::vector<GeoDeviceCapability> result = {};

    result.reserve(raw.size());

    for (const auto &d : raw) {
        result.push_back(Assess(d));
    }

    // Guarantee the caller always receives at least one entry.
    if (result.empty()) {
        result.push_back(MakeCpuFallbackCapability());
    }

    return result;
}

GeoDeviceCapability GeoDeviceDetector::BestDevice(const std::vector<GeoDeviceCapability> &capabilities) {
    const GeoDeviceCapability *best = nullptr;
    for (const auto &cap : capabilities) {
        if (!cap.suitable_for_geo) {
            continue;
        }
        if (best == nullptr || cap.device.free_vram_bytes > best->device.free_vram_bytes) {
            best = &cap;
        }
    }

    if (best != nullptr) {
        return *best;
    }

    // No suitable GPU device — return the CPU-fallback sentinel.
    for (const auto &cap : capabilities) {
        if (cap.device.backend == "CPU_FALLBACK") {
            return cap;
        }
    }

    return MakeCpuFallbackCapability();
}

GeoDeviceCapability GeoDeviceDetector::BestDevice() {
    return BestDevice(Detect());
}

bool GeoDeviceDetector::HasSuitableDevice(const std::vector<GeoDeviceCapability> &capabilities) {
    for (const auto &cap : capabilities) {
        if (cap.suitable_for_geo) {
            return true;
        }
    }
    return false;
}

bool GeoDeviceDetector::HasSuitableDevice() {
    return HasSuitableDevice(Detect());
}

std::string GeoDeviceDetector::ReportJson(const std::vector<GeoDeviceCapability> &capabilities) {
    const bool has_suitable = HasSuitableDevice(capabilities);

    std::ostringstream ss;
    ss << "{\"has_suitable_device\":" << (has_suitable ? "true" : "false") << ",\"devices\":[";

    bool first = true;
    for (const auto &cap : capabilities) {
        if (!first) {
            ss << ",";
        }
        first = false;

        const auto &d           = cap.device;
        const uint64_t total_mb = d.total_vram_bytes / (1024ULL * 1024ULL);
        const uint64_t free_mb  = d.free_vram_bytes / (1024ULL * 1024ULL);

        ss << "{"
           << "\"index\":" << d.index << ","
           << "\"name\":\"" << d.name << "\","
           << "\"backend\":\"" << d.backend << "\","
           << "\"total_vram_mb\":" << total_mb << ","
           << "\"free_vram_mb\":" << free_mb << ","
           << "\"compute_capability\":\"" << d.compute_major << "." << d.compute_minor << "\","
           << "\"is_healthy\":" << (d.is_healthy ? "true" : "false") << ","
           << "\"suitable_for_geo\":" << (cap.suitable_for_geo ? "true" : "false") << ","
           << "\"reason\":\"" << cap.reason << "\""
           << "}";
    }

    ss << "]}";
    return ss.str();
}

std::string GeoDeviceDetector::ReportJson() {
    return ReportJson(Detect());
}

} // namespace geo
} // namespace themis
