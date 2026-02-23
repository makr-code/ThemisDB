/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_backend.h                                  ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     115                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 25e932e7f  2026-02-22  feat(geo): implement ST_Buffer operation (Point + Polygon... ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
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
    virtual GeometryInfo stBuffer(const GeometryInfo& geom, double distance_m,
                                  int arc_points = 36) {
        (void)geom; (void)distance_m; (void)arc_points;
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

// Get the Boost CPU backend (if available)
ISpatialComputeBackend* getBoostCpuBackend();

// Get the built-in CPU exact backend (always available, no Boost dependency)
ISpatialComputeBackend* getCpuExactBackend();

// Get the GPU spatial backend (falls back to CPU when no GPU is present)
ISpatialComputeBackend* getGpuSpatialBackend();

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

} // namespace geo
} // namespace themis
