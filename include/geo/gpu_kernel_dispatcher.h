/**
 * @file gpu_kernel_dispatcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "acceleration/kernel_invocation.h"

namespace themis {
namespace geo {

/**
 * @brief Dispatches batch geospatial operations to CUDA kernels.
 *
 * GpuKernelDispatcher wraps the GeoKernelDispatch function-pointer table from
 * kernel_invocation.h and handles host↔device memory transfers for batch
 * point-in-polygon (containment) and Haversine distance operations.
 *
 * When THEMIS_GEO_CUDA is not defined all dispatch() calls return
 * ContainmentResult{false} / DistanceResult{false} immediately, so callers
 * can fall back to CPU without additional branching.
 *
 * Thread safety: instances are not thread-safe; each thread must use its own
 * GpuKernelDispatcher.
 *
 * Memory: each dispatch call allocates and frees device memory; no persistent
 * device allocations are held between calls.
 */
class GpuKernelDispatcher {
public:
    /// Result of a batch point-in-polygon GPU dispatch.
    struct ContainmentResult {
        bool                 dispatched = false; ///< true when the GPU kernel ran
        std::vector<uint8_t> mask;               ///< 1 = inside, 0 = outside [numPoints]
        int                  error_code = 0;     ///< non-zero CUDA error on failure
    };

    /// Result of a batch geodesic distance GPU dispatch.
    struct DistanceResult {
        bool               dispatched = false; ///< true when the GPU kernel ran
        std::vector<float> distances_km;       ///< distances in km [count]
        int                error_code = 0;     ///< non-zero CUDA error on failure
    };

    /// @param dispatch_table  GeoKernelDispatch table populated by
    ///   GeoAccelerationBridge::populateGeoDispatch().  Null entries disable
    ///   the corresponding GPU path (dispatch() returns dispatched=false).
    explicit GpuKernelDispatcher(
        const themis::acceleration::GeoKernelDispatch& dispatch_table) noexcept;

    /**
     * @brief Batch point-in-polygon using the GPU containment kernel.
     *
     * Allocates device memory, transfers host data to GPU, launches the
     * ray-casting containment kernel, copies results back, and frees device
     * memory.  Returns dispatched=false on any CUDA error; the caller should
     * then fall back to the CPU path and record the circuit-breaker failure.
     *
     * @param point_lats         Test point latitudes  [numPoints]
     * @param point_lons         Test point longitudes [numPoints]
     * @param numPoints          Number of test points (must be > 0)
     * @param polygon_coords     Interleaved [lat, lon] × numPolygonVertices
     * @param numPolygonVertices Number of polygon vertices (must be >= 3)
     * @return ContainmentResult with dispatched=true and mask populated on
     *         success, or dispatched=false with error_code set on failure.
     */
    ContainmentResult dispatchContainment(
        const double* point_lats,
        const double* point_lons,
        int           numPoints,
        const double* polygon_coords,
        int           numPolygonVertices
    );

    /**
     * @brief Batch Haversine distance using the GPU distance kernel.
     *
     * Allocates device memory, transfers host data to GPU, launches the
     * Haversine distance kernel, copies results back, and frees device
     * memory.  Returns dispatched=false on any CUDA error.
     *
     * @param lats1   First point latitudes  [count]
     * @param lons1   First point longitudes [count]
     * @param lats2   Second point latitudes [count]
     * @param lons2   Second point longitudes [count]
     * @param count   Number of point pairs (must be > 0)
     * @param formula Distance formula (default: Haversine)
     * @return DistanceResult with dispatched=true and distances_km populated
     *         on success, or dispatched=false with error_code set on failure.
     */
    DistanceResult dispatchDistance(
        const double* lats1,
        const double* lons1,
        const double* lats2,
        const double* lons2,
        int           count,
        themis::acceleration::GeoDistanceFormula formula =
            themis::acceleration::GeoDistanceFormula::HAVERSINE
    );

    /// Returns true when at least one dispatch function in the table is non-null.
    bool isAvailable() const noexcept;

#ifndef THEMIS_GEO_CUDA
    using ContainmentDispatchFn = std::function<ContainmentResult(const double*,
                                                                  const double*,
                                                                  int,
                                                                  const double*,
                                                                  int)>;
    using DistanceDispatchFn = std::function<DistanceResult(const double*,
                                                            const double*,
                                                            const double*,
                                                            const double*,
                                                            int,
                                                            themis::acceleration::GeoDistanceFormula)>;

    static void setContainmentDispatchFn(ContainmentDispatchFn fn);
    static void setDistanceDispatchFn(DistanceDispatchFn fn);
#endif

private:
    themis::acceleration::GeoKernelDispatch dispatch_table_;
};

} // namespace geo
} // namespace themis
