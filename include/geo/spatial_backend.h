/**
 * @file spatial_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

#include "utils/geo/ewkb.h"

namespace themis {
namespace geo {

// Minimal abstraction for compute backends (CPU/GPU) used by Geo exact checks
struct SpatialBatchInputs {
    /// Number of geometry pairs to test.  When geoms_a / geoms_b are
    /// populated they must contain exactly `count` elements each.  If the
    /// vectors are empty, `count` is still used to size the output mask
    /// (all entries will be 0).
    std::size_t count{0};

    /// First geometry of each pair.  Size must equal count when non-empty.
    std::vector<GeometryInfo> geoms_a;
    /// Second geometry of each pair.  Size must equal count when non-empty.
    std::vector<GeometryInfo> geoms_b;
};

struct SpatialBatchResults {
    std::vector<uint8_t> mask; // 1 = hit, 0 = no hit
};

/** @brief I spatial compute backend implementation. */
class ISpatialComputeBackend {
public:
    virtual ~ISpatialComputeBackend() = default;
    [[nodiscard]] virtual const char* name() const noexcept = 0;
    [[nodiscard]] virtual bool isAvailable() const noexcept = 0;

    // Example operation: batch Intersects exact-checks on prefiltered candidates
    [[nodiscard]] virtual SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) = 0;
    
    // Exact intersects check between two geometries (used by search path)
    // Returns true if geometries actually intersect, false otherwise
    [[nodiscard]] virtual bool exactIntersects(const GeometryInfo& geom1, const GeometryInfo& geom2) = 0;

    // ST_BUFFER: expand geometry by a fixed geodesic distance (metres).
    // Returns a Polygon approximating the buffered geometry, or an empty
    // GeometryInfo if the geometry type is unsupported.
    // arc_points controls the number of vertices used to approximate curves
    // (default 36; minimum 3).
    // Supported types: Point → circular polygon, Polygon → outward expansion.
    // GPU path deferred: implementations without CUDA delegate to the CPU path.
    virtual GeometryInfo stBuffer([[maybe_unused]] const GeometryInfo& geom, [[maybe_unused]] double distance_m,
                                  [[maybe_unused]] int arc_points = 36) {
        return GeometryInfo{};
    }

    // Geodesic distance on the WGS-84 ellipsoid using the Vincenty formula.
    // Returns the distance in metres between two geographic coordinates.
    // lat1/lon1 and lat2/lon2 are in decimal degrees (WGS-84).
    // Returns 0.0 for coincident points.
    // For nearly-antipodal inputs where Vincenty does not converge, falls back
    // to the Haversine formula and returns a positive finite approximation
    // (accuracy ±0.5 %; never returns a negative sentinel value).
    virtual double geodesicDistance([[maybe_unused]] double lat1, [[maybe_unused]] double lon1,
                    [[maybe_unused]] double lat2, [[maybe_unused]] double lon2) const {
        return 0.0;
    }

    // ST_UNION: compute the geometric union of two geometries.
    // Returns a geometry that contains all points in either geom1 or geom2.
    // For two non-overlapping polygons the result is a GeometryCollection.
    // For overlapping polygons the result is a merged Polygon.
    // Returns an empty GeometryInfo on unsupported type combinations.
    virtual GeometryInfo stUnion([[maybe_unused]] const GeometryInfo& geom1,
                                 [[maybe_unused]] const GeometryInfo& geom2) {
        return GeometryInfo{};
    }

    // ST_DIFFERENCE: compute geom1 minus geom2 (the set-difference).
    // Returns a geometry containing all points in geom1 that are not in geom2.
    // Returns geom1 unchanged when the two geometries do not intersect.
    // Returns an empty GeometryInfo when geom1 is fully contained in geom2.
    virtual GeometryInfo stDifference([[maybe_unused]] const GeometryInfo& geom1,
                                      [[maybe_unused]] const GeometryInfo& geom2) {
        return GeometryInfo{};
    }
};

// Registry for dynamically loaded plugins
/** @brief Registry for dynamically loaded plugins. */
class IGeoRegistry {
public:
    virtual ~IGeoRegistry() = default;
    virtual void registerBackend(std::unique_ptr<ISpatialComputeBackend> backend) = 0;
};

// Plugin entry point signature a plugin must export if present
// extern "C" void RegisterGeoPlugin(IGeoRegistry* registry);
using RegisterGeoPluginFn = void(*)(IGeoRegistry*);

// Precision mode for spatial computation backends.
// Exact mode uses full geometric algorithms (ray-casting, segment-intersection).
// Approximate mode uses MBR (bounding-box) overlap for faster but potentially
// conservative checks (no false negatives; may produce false positives).
enum class GeoPrecisionMode {
    Exact,       // Full geometric exactness; higher CPU cost.
    Approximate  // MBR-based fast approximation; safe for pre-filtering.
};

// Get the Boost CPU backend (if available)
ISpatialComputeBackend* getBoostCpuBackend();

// Get the built-in CPU exact backend (always available, no Boost dependency)
ISpatialComputeBackend* getCpuExactBackend();

// Get the built-in CPU approximate backend (always available).
// Uses MBR overlap checks for fast conservative spatial tests.
ISpatialComputeBackend* getCpuApproximateBackend();

// Get the global geo backend registry.
// Backends self-register at startup so they are discoverable at runtime.
IGeoRegistry* getGeoBackendRegistry();

/**
 * @brief Point-in-polygon containment callback type.
 *
 * When injected via setCpuExactContainmentFn(), this function is called by
 * CpuExactBackend::exactIntersects() for every point-in-polygon test instead
 * of the built-in ray-casting algorithm.
 *
 * Signature: (px, py, polygon_ring) → true if (px, py) is inside the ring.
 *
 * This injection point lets callers (e.g. Boost.Geometry wrappers) provide
 * OGC-compliant geodesic or spherical containment without recompiling the
 * backend.  Pass nullptr to restore the built-in ray-casting fallback.
 */
using GeoContainmentFn = std::function<bool(double px, double py,
                                             const std::vector<Coordinate>& ring)>;

/**
 * @brief Inject a custom point-in-polygon function into the CPU exact backend.
 *
 * Thread-safe.  Can be called multiple times; the last value wins.
 * Pass nullptr to restore the built-in ray-casting algorithm.
 *
 * @param fn  Containment callback; nullptr clears the override.
 */
void setCpuExactContainmentFn(GeoContainmentFn fn);

// Get a backend for the requested precision mode.
// Exact   → getCpuExactBackend()
// Approximate → getCpuApproximateBackend()
ISpatialComputeBackend* getBackendForPrecision(GeoPrecisionMode mode);

// Get the GPU spatial backend (falls back to CPU when no GPU is present)
ISpatialComputeBackend* getGpuSpatialBackend();

// Get the production GPU backend (CUDA/OpenCL/CPU-parallel with automatic fallback)
ISpatialComputeBackend* getProductionGpuBackend();

/**
 * @brief Return a JSON string with current GPU spatial backend operational stats.
 *
 * The returned object contains:
 *   backend_name, gpu_present, circuit_open, device_name,
 *   batch_calls, batch_fallbacks, batch_pairs_processed,
 *   exact_calls, exact_errors,
 *   batch_avg_latency_us, batch_max_latency_us
 *
 * This free function surfaces the `GpuBatchBackend::Stats` struct without
 * exposing the concrete class to callers.
 */
std::string getGpuSpatialBackendStatsJson();

/**
 * @brief Return a JSON string with GPU device capability information for the
 *        geo module.
 *
 * Delegates to GeoDeviceDetector::ReportJson() and reports all enumerated
 * devices together with geo-specific suitability assessments
 * (compute capability, VRAM threshold).
 *
 * The returned object contains:
 *   has_suitable_device, devices[]{index, name, backend,
 *   total_vram_mb, free_vram_mb, compute_capability, is_healthy,
 *   suitable_for_geo, reason}
 */
std::string getGeoDeviceReportJson();

} // namespace geo
} // namespace themis
