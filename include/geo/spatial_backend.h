/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_backend.h                                  ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     106                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
