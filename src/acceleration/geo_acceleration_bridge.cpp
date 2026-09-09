/**
 * @file geo_acceleration_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Acceleration module — Geo Acceleration Bridge
// ================================================
// Bridges the acceleration IGeoBackend interface to the geo module's
// ISpatialComputeBackend (GpuBatchBackend).
//
// Dispatch chain position
// -----------------------
//   BackendRegistry::getSelectedGeoBackend()
//       └─► GeoAccelerationBridge (self-registered at static-init time)
//               ├─► GeoKernelFallbackDispatcher::dispatch()  — via populateGeoDispatch()
//               │       ├─ GPU: launchGeoDistanceKernel() (cuda/geo_kernels.cu) [CUDA only]
//               │       └─ CPU: Haversine implementation (batchDistances fallback)
//               └─► GpuBatchBackend::batchIntersects()  — point-in-polygon via ray-casting
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
//
// Related files
// -------------
//   include/acceleration/geo_acceleration_bridge.h   — GeoAccelerationBridge declaration
//   src/acceleration/cuda/geo_kernels.cu              — GPU Haversine + point-in-polygon kernels
//   include/acceleration/kernel_invocation.h          — GeoKernelDispatch interface (frozen v1.x)
//   include/acceleration/kernel_fallback_dispatcher.h — GeoKernelFallbackDispatcher
//   src/acceleration/backend_registry.cpp             — registry that selects this bridge
//   src/acceleration/ARCHITECTURE.md                  — Integration Points (Section 5)

#include "acceleration/geo_acceleration_bridge.h"
#include "acceleration/cpu_backend.h"
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
#include "utils/logger.h"
#include "utils/geometric_distances.h"

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

// ---------------------------------------------------------------------------
// Static kernel dispatch functions
//
// These match the GeoDistanceFn / GeoContainmentFn typedefs from
// kernel_invocation.h and are registered via populateGeoDispatch() so that
// BackendRegistry can invoke them without knowing the concrete backend type.
// ---------------------------------------------------------------------------

static int bridge_geo_distance(
    const double* lats1, const double* lons1,
    const double* lats2, const double* lons2,
    float* out_distances, int count,
    themis::acceleration::GeoDistanceFormula /*formula*/,
    void* /*stream*/)
{
    if (!lats1 || !lons1 || !lats2 || !lons2 || !out_distances || count <= 0) {
        return 1;
    }
    for (int i = 0; i < count; ++i) {
        const double dlat  = (lats2[i] - lats1[i]) * kDegToRad;
        const double dlon  = (lons2[i] - lons1[i]) * kDegToRad;
        const double rlat1 = lats1[i] * kDegToRad;
        const double rlat2 = lats2[i] * kDegToRad;
        const double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
                         std::cos(rlat1) * std::cos(rlat2) *
                         std::sin(dlon / 2.0) * std::sin(dlon / 2.0);
        out_distances[i] = static_cast<float>(
            kEarthRadiusKm * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a)));
    }
    return 0;
}

static int bridge_geo_containment(
    const double* point_lats, const double* point_lons, int numPoints,
    const double* polygon_coords, int numVertices,
    uint8_t* results, void* /*stream*/)
{
    if (!point_lats || !point_lons || !polygon_coords || !results ||
            numPoints <= 0 || numVertices < 3) {
        return 1;
    }

    // Build the polygon GeometryInfo once.
    themis::geo::GeometryInfo poly(themis::geo::GeometryType::Polygon);
    std::vector<themis::geo::Coordinate> ring;
    ring.reserve(static_cast<size_t>(numVertices));
    for (int v = 0; v < numVertices; ++v) {
        ring.push_back({polygon_coords[v * 2], polygon_coords[v * 2 + 1]});
    }
    poly.rings.push_back(std::move(ring));

    // Build the per-point GeometryInfo vector and delegate to GpuBatchBackend
    // (which falls back to CPU when no GPU is present).
    themis::geo::SpatialBatchInputs batch;
    batch.count = static_cast<size_t>(numPoints);
    batch.geoms_a.reserve(batch.count);
    batch.geoms_b.reserve(batch.count);
    for (int i = 0; i < numPoints; ++i) {
        themis::geo::GeometryInfo pt(themis::geo::GeometryType::Point);
        pt.coords.push_back({point_lats[i], point_lons[i]});
        batch.geoms_a.push_back(std::move(pt));
        batch.geoms_b.push_back(poly);
    }

    auto* geo = themis::geo::getGpuSpatialBackend();
    themis::geo::SpatialBatchResults res = geo->batchIntersects(batch);
    const size_t n = static_cast<size_t>(numPoints);
    for (size_t i = 0; i < res.mask.size() && i < n; ++i) {
        results[i] = res.mask[i];
    }
    return 0;
}

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
    return themis::geo::haversine_km(lat1, lon1, lat2, lon2);
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
    const size_t mask_size = static_cast<size_t>(res.mask.size());
    for (size_t i = 0; i < mask_size && i < numPoints; ++i) {
        out[i] = (res.mask[i] != 0);
    }
    return out;
}

// ---------------------------------------------------------------------------
// populateGeoDispatch
//
// Returns CUDA kernel launchers when THEMIS_ENABLE_CUDA is defined and a GPU
// device is available; otherwise returns CPU-fallback launchers so the
// dispatch table is always fully populated.
// ---------------------------------------------------------------------------

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
    // CPU fallback: bridge_geo_distance / bridge_geo_containment are always
    // available, so the dispatch table is never left with null entries.
    GeoKernelDispatch d;
    d.launchDistance    = bridge_geo_distance;
    d.launchContainment = bridge_geo_containment;
    return d;
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
