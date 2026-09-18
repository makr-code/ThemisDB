/**
 * @file spatial_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    std::size_t count{0};

    std::vector<GeometryInfo> geoms_a;
    std::vector<GeometryInfo> geoms_b;
};

struct SpatialBatchResults {
    std::vector<uint8_t> mask; // 1 = hit, 0 = no hit
};

class ISpatialComputeBackend {
public:
    /**
     * @brief ISpatial Compute Backend.
     * @return Return value.
     */
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
class IGeoRegistry {
public:
    /**
     * @brief IGeo Registry.
     * @return Return value.
     */
    virtual ~IGeoRegistry() = default;
    /**
     * @brief Register Backend.
     * @param[in] backend Input parameter.
     */
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

/**
 * @brief Get the Boost CPU backend (if available)
 * @return Pointer to the result.
 */
ISpatialComputeBackend* getBoostCpuBackend();

/**
 * @brief Get the built-in CPU exact backend (always available, no Boost dependency)
 * @return Pointer to the result.
 */
ISpatialComputeBackend* getCpuExactBackend();

/**
 * @brief Get the built-in CPU approximate backend (always available).
 * @return Pointer to the result.
 * @details Uses MBR overlap checks for fast conservative spatial tests.
 */
ISpatialComputeBackend* getCpuApproximateBackend();

/**
 * @brief Get the global geo backend registry.
 * @return Pointer to the result.
 * @details Backends self-register at startup so they are discoverable at runtime.
 */
IGeoRegistry* getGeoBackendRegistry();

using GeoContainmentFn = std::function<bool(double px, double py,
                                             const std::vector<Coordinate>& ring)>;

/**
 * @brief Set Cpu Exact Containment Fn.
 * @param[in] fn Input parameter.
 */
void setCpuExactContainmentFn(GeoContainmentFn fn);

/**
 * @brief Get a backend for the requested precision mode.
 * @param[in] mode Input parameter.
 * @return Pointer to the result.
 * @details Exact → getCpuExactBackend() Approximate → getCpuApproximateBackend()
 */
ISpatialComputeBackend* getBackendForPrecision(GeoPrecisionMode mode);

/**
 * @brief Get the GPU spatial backend (falls back to CPU when no GPU is present)
 * @return Pointer to the result.
 */
ISpatialComputeBackend* getGpuSpatialBackend();

/**
 * @brief Get the production GPU backend (CUDA/OpenCL/CPU-parallel with automatic fallback)
 * @return Pointer to the result.
 */
ISpatialComputeBackend* getProductionGpuBackend();

/**
 * @brief Get Gpu Spatial Backend Stats Json.
 * @return Return value.
 */
std::string getGpuSpatialBackendStatsJson();

/**
 * @brief Get Geo Device Report Json.
 * @return Return value.
 */
std::string getGeoDeviceReportJson();

} // namespace geo
} // namespace themis
