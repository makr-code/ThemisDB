/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            kernel_invocation.h                                ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-23 03:57:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     226                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • 6d203e11f  2026-02-21  Freeze ANN & geospatial kernel invocation interfaces; wir... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

// =============================================================================
// ThemisDB - Frozen Kernel Invocation Interfaces
//
// File:    include/acceleration/kernel_invocation.h
// Version: 1.0.0  (INTERFACE_VERSION = 100)
// Status:  FROZEN — signatures stable from Q2 2026 / v1.x
//
// This header defines the stable, versioned C++ kernel invocation API for
// Approximate Nearest Neighbour (ANN) and geospatial operations.  All
// acceleration backends (CUDA, Vulkan, HIP, CPU) MUST conform to the
// function-pointer typedefs declared here when populating their dispatch
// tables.
//
// Compatibility guarantee
// -----------------------
//  - Field additions to parameter / result structs are allowed; new fields
//    are zero-initialised by default so existing callers are unaffected.
//  - Enum values will not be renumbered or removed.
//  - Function-pointer typedefs (ANNDistanceFn, ANNTopKFn, GeoDistanceFn,
//    GeoContainmentFn) will not change parameter order or types.
//  - INTERFACE_VERSION is bumped ONLY on breaking changes.
//
// Usage
// -----
//  Backend implementations fill an ANNKernelDispatch / GeoKernelDispatch
//  table.  The BackendRegistry dispatches kernel calls through these tables
//  at query time.
// =============================================================================

#include <cstddef>
#include <cstdint>

namespace themis {
namespace acceleration {

// =============================================================================
// Interface version
// =============================================================================

/// Monotonically increasing integer encoding major*100 + minor.
/// Callers may compare this at runtime to detect mismatched shared libraries.
inline constexpr uint32_t KERNEL_INVOCATION_INTERFACE_VERSION = 100; // v1.0

// =============================================================================
// Shared enumerations
// =============================================================================

/// Distance metric used by ANN kernel launchers.
/// Values are stable and will not be renumbered.
enum class DistanceMetric : uint32_t {
    L2            = 0, ///< Squared Euclidean distance (no sqrt — fast, consistent)
    COSINE        = 1, ///< Cosine distance: 1 - dot(a,b) / (|a| * |b|)
    INNER_PRODUCT = 2, ///< Negative inner product (max-IP search)
};

/// Distance formula for geospatial kernel launchers.
/// Values are stable and will not be renumbered.
enum class GeoDistanceFormula : uint32_t {
    HAVERSINE = 0, ///< Spherical-Earth Haversine formula  (default, < 0.5 % error)
    VINCENTY  = 1, ///< Ellipsoidal Vincenty formula       (higher precision)
};

// =============================================================================
// ANN kernel parameters and results
// =============================================================================

/// Describes the input shape and search configuration for an ANN kernel call.
/// All pointer fields are device pointers unless the CPU backend is used,
/// in which case they are host pointers.
struct ANNKernelParams {
    const float*   queries    = nullptr;          ///< Query matrix      [numQueries × dim]
    size_t         numQueries = 0;                ///< Number of queries
    size_t         dim        = 0;                ///< Vector dimensionality
    const float*   vectors    = nullptr;          ///< Database matrix   [numVectors × dim]
    size_t         numVectors = 0;                ///< Database size
    size_t         topK       = 1;                ///< k nearest neighbours to retrieve
    DistanceMetric metric     = DistanceMetric::L2; ///< Distance metric
};

/// Output container for ANN kernel results.
/// Caller allocates the arrays; kernel populates them.
struct ANNKernelResult {
    uint32_t* indices   = nullptr; ///< Result indices   [numQueries × topK]
    float*    distances = nullptr; ///< Result distances [numQueries × topK]
};

// =============================================================================
// Geospatial kernel parameters
// =============================================================================

/// Describes the input shape and formula for a batch-distance kernel call.
struct GeoKernelParams {
    const double*      latitudes1  = nullptr;                   ///< First latitudes  [count]
    const double*      longitudes1 = nullptr;                   ///< First longitudes [count]
    const double*      latitudes2  = nullptr;                   ///< Second latitudes [count]
    const double*      longitudes2 = nullptr;                   ///< Second longitudes [count]
    size_t             count       = 0;                         ///< Number of point pairs
    GeoDistanceFormula formula     = GeoDistanceFormula::HAVERSINE; ///< Distance formula
};

/// Describes the input shape for a point-in-polygon kernel call.
/// Polygon vertices are interleaved as [lat0, lon0, lat1, lon1, …].
struct GeoContainmentParams {
    const double* pointLats          = nullptr; ///< Test point latitudes  [numPoints]
    const double* pointLons          = nullptr; ///< Test point longitudes [numPoints]
    size_t        numPoints          = 0;       ///< Number of test points
    const double* polygonCoords      = nullptr; ///< Interleaved vertex coords [numVertices × 2]
    size_t        numPolygonVertices = 0;       ///< Number of polygon vertices
};

// =============================================================================
// Frozen kernel-launcher function-pointer typedefs
//
// All functions follow the same stream-forwarding convention:
//   opaque_stream — backend-specific execution stream handle cast to void*:
//     CUDA:   cudaStream_t
//     Vulkan: VkCommandBuffer
//     HIP:    hipStream_t
//     CPU:    ignored (pass nullptr)
//
// Return value: 0 on success, non-zero error code on failure.
// =============================================================================

/// Computes the full [numQueries × numVectors] distance matrix.
using ANNDistanceFn = int (*)(
    const float* d_queries,   ///< [numQueries × dim]
    const float* d_vectors,   ///< [numVectors × dim]
    float*       d_distances, ///< [numQueries × numVectors] — caller-allocated
    int          numQueries,
    int          numVectors,
    int          dim,
    void*        opaque_stream
);

/// Extracts the top-k nearest neighbours from a precomputed distance matrix.
using ANNTopKFn = int (*)(
    const float* d_distances,    ///< [numQueries × numVectors]
    uint32_t*    d_topk_indices, ///< [numQueries × topK] — caller-allocated
    float*       d_topk_dists,   ///< [numQueries × topK] — caller-allocated
    int          numQueries,
    int          numVectors,
    int          topK,
    void*        opaque_stream
);

/// Computes per-pair geodesic distances in kilometres.
using GeoDistanceFn = int (*)(
    const double*      d_latitudes1,  ///< [count]
    const double*      d_longitudes1, ///< [count]
    const double*      d_latitudes2,  ///< [count]
    const double*      d_longitudes2, ///< [count]
    float*             d_distances,   ///< [count] — output in km, caller-allocated
    int                count,
    GeoDistanceFormula formula,
    void*              opaque_stream
);

/// Tests whether each point is inside the given polygon (ray-casting).
using GeoContainmentFn = int (*)(
    const double* d_point_lats,      ///< [numPoints]
    const double* d_point_lons,      ///< [numPoints]
    int           numPoints,
    const double* d_polygon_coords,  ///< interleaved [lat, lon] × numVertices
    int           numPolygonVertices,
    uint8_t*      d_results,         ///< [numPoints] — non-zero if inside, caller-allocated
    void*         opaque_stream
);

// =============================================================================
// Kernel dispatch tables
//
// Backends fill one of these structs during initialization.  Null entries
// indicate unsupported operations; use ANNKernelFallbackDispatcher /
// GeoKernelFallbackDispatcher (kernel_fallback_dispatcher.h) to route null
// slots to a CPU fallback table and retry transient device errors.
// =============================================================================

/// ANN kernel dispatch table — one per backend instance.
struct ANNKernelDispatch {
    ANNDistanceFn launchL2Distance   = nullptr; ///< Squared L2 distance kernel
    ANNDistanceFn launchCosine       = nullptr; ///< Cosine distance kernel
    ANNDistanceFn launchInnerProduct = nullptr; ///< Inner-product kernel
    ANNTopKFn     launchTopK         = nullptr; ///< Top-k selection kernel

    /// Returns the distance launcher matching @p metric, or nullptr if absent.
    [[nodiscard]] ANNDistanceFn distanceLauncherFor(DistanceMetric metric) const noexcept {
        switch (metric) {
            case DistanceMetric::L2:            return launchL2Distance;
            case DistanceMetric::COSINE:        return launchCosine;
            case DistanceMetric::INNER_PRODUCT: return launchInnerProduct;
        }
        return nullptr;
    }
};

/// Geospatial kernel dispatch table — one per backend instance.
struct GeoKernelDispatch {
    GeoDistanceFn    launchDistance    = nullptr; ///< Haversine / Vincenty distance kernel
    GeoContainmentFn launchContainment = nullptr; ///< Point-in-polygon kernel
};

} // namespace acceleration
} // namespace themis
