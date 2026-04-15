/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_backend.h                                  ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:02:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     185                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

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

class ISpatialComputeBackend {
public:
    virtual ~ISpatialComputeBackend() = default;
    virtual const char* name() const noexcept = 0;
    virtual bool isAvailable() const noexcept = 0;

    // Example operation: batch Intersects exact-checks on prefiltered candidates
    virtual SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) = 0;
    
    // Exact intersects check between two geometries (used by search path)
    // Returns true if geometries actually intersect, false otherwise
    virtual bool exactIntersects(const GeometryInfo& geom1, const GeometryInfo& geom2) = 0;

    // ST_BUFFER: expand geometry by a fixed geodesic distance (metres).
    // Returns a Polygon approximating the buffered geometry, or an empty
    // GeometryInfo if the geometry type is unsupported.
    // arc_points controls the number of vertices used to approximate curves
    // (default 36; minimum 3).
    // Supported types: Point → circular polygon, Polygon → outward expansion.
    // GPU path deferred: implementations without CUDA delegate to the CPU path.
    virtual GeometryInfo stBuffer([[maybe_unused]] const GeometryInfo& geom, double distance_m,
                                  [[maybe_unused]] int arc_points = 36) {
        return GeometryInfo{};
    }

    // Geodesic distance on the WGS-84 ellipsoid using the Vincenty formula.
    // Returns the distance in metres between two geographic coordinates.
    // lat1/lon1 and lat2/lon2 are in decimal degrees (WGS-84).
    // Returns 0.0 for coincident points and a negative value if the formula
    // fails to converge (nearly-antipodal degenerate case).
    virtual double geodesicDistance([[maybe_unused]] double lat1, double lon1,
                                    [[maybe_unused]] double lat2, double lon2) const {
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
