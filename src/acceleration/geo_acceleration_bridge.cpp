/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geo_acceleration_bridge.cpp                       ║
  Version:         0.0.32                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Bridges the acceleration IGeoBackend interface to the geo module's
// ISpatialComputeBackend (GpuBatchBackend).
//
// The bridge is registered in BackendRegistry so that callers using the
// acceleration framework's capability-driven backend selection automatically
// benefit from the geo module's GPU spatial backend with circuit-breaker
// fallback.
//
// Key design decisions
// --------------------
// 1. batchPointInPolygon() converts flat coordinate arrays to GeometryInfo and
//    delegates to GpuBatchBackend::batchIntersects().  This reuses the
//    production-quality CPU exact backend (ray-casting) and the GPU device
//    discovery / circuit-breaker machinery already wired into GpuBatchBackend.
//
// 2. batchDistances() implements Haversine on CPU.  GPU kernel dispatch for
//    distance is wired via populateGeoDispatch() when THEMIS_ENABLE_CUDA is
//    defined (uses launchGeoDistanceKernel from cuda/geo_kernels.cu).
//
// 3. isAvailable() returns true always — the bridge itself is always usable
//    because it falls back to CPU.  type() returns BackendType::CUDA when the
//    underlying geo GPU backend is available, BackendType::CPU otherwise, so
//    BackendRegistry's kFallbackOrder gives the right priority.
//
// 4. Self-registration: a static initializer at the bottom of this file calls
//    BackendRegistry::instance().registerBackend() so that the bridge is
//    available as soon as the geo module (themis_geo) is loaded.  This avoids
//    a circular dependency between themis_base (BackendRegistry) and
//    themis_geo (GpuBatchBackend).

#include "acceleration/geo_acceleration_bridge.h"
#include "acceleration/cpu_backend.h"
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
#include "utils/logger.h"

#include <cmath>
#include <stdexcept>

#ifdef THEMIS_ENABLE_CUDA
#include <cstdint>
extern "C" {
int launchGeoDistanceKernel(
    const double* d_lats1,
    const double* d_lons1,
    const double* d_lats2,
    const double* d_lons2,
    float* d_distances,
    int count,
    themis::acceleration::GeoDistanceFormula formula,
    void* opaque_stream
);
int launchGeoContainmentKernel(
    const double* d_point_lats,
    const double* d_point_lons,
    int numPoints,
    const double* d_polygon_coords,
    int numPolygonVertices,
    uint8_t* d_results,
    void* opaque_stream
);
} // extern "C"
#endif

namespace themis {
namespace acceleration {

// ---------------------------------------------------------------------------
// Haversine constant
// ---------------------------------------------------------------------------

namespace {
constexpr double kEarthRadiusKm = 6371.0;
constexpr double kDegToRad      = 3.141592653589793238462643383279502884 / 180.0;
} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor / lifecycle
// ---------------------------------------------------------------------------

GeoAccelerationBridge::GeoAccelerationBridge() = default;

BackendType GeoAccelerationBridge::type() const noexcept {
    // Report as CUDA when the GPU spatial backend is available so that the
    // BackendRegistry gives this backend higher priority than the CPU fallback.
    auto* geo = themis::geo::getGpuSpatialBackend();
    if (geo && geo->isAvailable()) {
        return BackendType::CUDA;
    }
    return BackendType::CPU;
}

bool GeoAccelerationBridge::isAvailable() const noexcept {
    // The bridge is always available; it falls back to CPU when no GPU is present.
    return true;
}

BackendCapabilities GeoAccelerationBridge::getCapabilities() const {
    BackendCapabilities caps;
    caps.supportsVectorOps       = false;
    caps.supportsGraphOps        = false;
    caps.supportsGeoOps          = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = false;
    caps.supportedPrecisions     = PrecisionMode::FP32;
    caps.supportedMetrics        = 0; // geo ops do not use ANN distance metrics

    auto* geo = themis::geo::getGpuSpatialBackend();
    if (geo && geo->isAvailable()) {
        caps.deviceName = std::string("GeoAccelerationBridge[") + geo->name() + "]";
    } else {
        caps.deviceName = "GeoAccelerationBridge[CPU fallback]";
    }
    return caps;
}

bool GeoAccelerationBridge::initialize() {
    clearError();
    // Eagerly touch the geo GPU backend singleton so that device discovery
    // and the circuit-breaker are initialised before the first operation.
    (void)themis::geo::getGpuSpatialBackend();
    return true;
}

void GeoAccelerationBridge::shutdown() {}

// ---------------------------------------------------------------------------
// batchDistances
// ---------------------------------------------------------------------------

// static
double GeoAccelerationBridge::haversineKm(double lat1, double lon1,
                                           double lat2, double lon2) noexcept {
    const double dlat = (lat2 - lat1) * kDegToRad;
    const double dlon = (lon2 - lon1) * kDegToRad;
    const double rlat1 = lat1 * kDegToRad;
    const double rlat2 = lat2 * kDegToRad;

    const double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
                     std::cos(rlat1) * std::cos(rlat2) *
                     std::sin(dlon / 2.0) * std::sin(dlon / 2.0);
    return kEarthRadiusKm * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

std::vector<float> GeoAccelerationBridge::batchDistances(
    const double* latitudes1,
    const double* longitudes1,
    const double* latitudes2,
    const double* longitudes2,
    size_t count,
    bool useHaversine
) {
    if (!latitudes1 || !longitudes1 || !latitudes2 || !longitudes2) {
        THEMIS_WARN("GeoAccelerationBridge::batchDistances: null input pointer");
        return {};
    }

    std::vector<float> results(count);
    for (size_t i = 0; i < count; ++i) {
        if (useHaversine) {
            results[i] = static_cast<float>(
                haversineKm(latitudes1[i], longitudes1[i],
                            latitudes2[i], longitudes2[i]));
        } else {
            // Planar Euclidean distance in coordinate units (degrees).
            const double dlat = latitudes2[i]  - latitudes1[i];
            const double dlon = longitudes2[i] - longitudes1[i];
            results[i] = static_cast<float>(std::sqrt(dlat * dlat + dlon * dlon));
        }
    }
    return results;
}

// ---------------------------------------------------------------------------
// batchPointInPolygon
// ---------------------------------------------------------------------------

std::vector<bool> GeoAccelerationBridge::batchPointInPolygon(
    const double* pointLats,
    const double* pointLons,
    size_t numPoints,
    const double* polygonCoords,
    size_t numPolygonVertices
) {
    if (!pointLats || !pointLons || !polygonCoords || numPolygonVertices < 3) {
        THEMIS_WARN("GeoAccelerationBridge::batchPointInPolygon: "
                    "invalid inputs (null pointer or < 3 polygon vertices)");
        return std::vector<bool>(numPoints, false);
    }

    // Build the polygon GeometryInfo once.
    themis::geo::GeometryInfo poly(themis::geo::GeometryType::Polygon);
    std::vector<themis::geo::Coordinate> ring;
    ring.reserve(numPolygonVertices);
    for (size_t v = 0; v < numPolygonVertices; ++v) {
        // polygonCoords is interleaved [lat, lon] pairs.  Map lat→x, lon→y to
        // match the calling convention documented in the IGeoBackend interface.
        ring.push_back({polygonCoords[v * 2], polygonCoords[v * 2 + 1]});
    }
    poly.rings.push_back(std::move(ring));

    // Build the per-point GeometryInfo vector.
    themis::geo::SpatialBatchInputs batch;
    batch.count  = numPoints;
    batch.geoms_a.reserve(numPoints);
    batch.geoms_b.reserve(numPoints);

    for (size_t i = 0; i < numPoints; ++i) {
        themis::geo::GeometryInfo pt(themis::geo::GeometryType::Point);
        pt.coords.push_back({pointLats[i], pointLons[i]});
        batch.geoms_a.push_back(std::move(pt));
        batch.geoms_b.push_back(poly); // shared polygon — copy is cheap for small rings
    }

    // Delegate to the geo GPU backend (CPU fallback is handled internally).
    auto* geo = themis::geo::getGpuSpatialBackend();
    themis::geo::SpatialBatchResults res = geo->batchIntersects(batch);

    // Convert uint8_t mask → bool.
    std::vector<bool> out(numPoints, false);
    for (size_t i = 0; i < res.mask.size() && i < numPoints; ++i) {
        out[i] = (res.mask[i] != 0u);
    }
    return out;
}

} // namespace acceleration
} // namespace themis

// ---------------------------------------------------------------------------
// GeoKernelDispatch population
// ---------------------------------------------------------------------------

namespace themis {
namespace acceleration {

GeoKernelDispatch GeoAccelerationBridge::populateGeoDispatch() const {
#ifdef THEMIS_ENABLE_CUDA
    auto* geo = themis::geo::getGpuSpatialBackend();
    if (geo && geo->isAvailable()) {
        GeoKernelDispatch d;
        d.launchDistance    = launchGeoDistanceKernel;
        d.launchContainment = launchGeoContainmentKernel;
        return d;
    }
#endif
    // CPU fallback: always available, no GPU required.
    CPUGeoBackend cpu;
    return cpu.populateGeoDispatch();
}

} // namespace acceleration
} // namespace themis

// ---------------------------------------------------------------------------
// Self-registration
//
// When the geo module (themis_geo) is loaded this static initializer registers
// the bridge with the acceleration BackendRegistry.  This avoids the circular
// dependency that would arise if backend_registry.cpp (in themis_base) were
// to include and reference geo module symbols directly.
// ---------------------------------------------------------------------------

namespace {
struct GeoAccelerationBridgeRegistrar {
    GeoAccelerationBridgeRegistrar() {
        ::themis::acceleration::BackendRegistry::instance()
            .registerBackend(
                std::make_unique<themis::acceleration::GeoAccelerationBridge>());
    }
};
// NOLINTNEXTLINE(cert-err58-cpp) — intentional static init for self-registration
static const GeoAccelerationBridgeRegistrar g_geo_bridge_registrar;
} // anonymous namespace
