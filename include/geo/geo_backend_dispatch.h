/**
 * @file geo_backend_dispatch.h
 * @brief Runtime GPU/CPU backend selection with device detection.
 *
 * Provides GeoBackendDispatcher for seamless GPU/CPU dispatch selection.
 * Supports compile-time feature gating (THEMIS_GEO_CUDA) and runtime
 * GPU device detection with graceful CPU fallback.
 *
 * Performance targets (Gates A-06-01/02):
 * - Haversine batch distance: p95/p99 ≤ 500ms (GPU), ≤ 50ms (CPU baseline)
 * - Point-in-polygon batch:   p95/p99 ≤ 2ms (GPU), ≤ 0.5ms (CPU baseline)
 */

#pragma once

#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

namespace themis {
namespace acceleration {
// Forward declare for dispatch table integration
struct GeoKernelDispatch;
}  // namespace acceleration

namespace geo {

/**
 * @class GeoBackendDispatcher
 * @brief Dispatches geospatial operations to GPU or CPU based on availability.
 *
 * Detects CUDA GPU availability at runtime (when THEMIS_GEO_CUDA=ON).
 * Decides GPU vs CPU dispatch based on batch size heuristics and error handling.
 * All operations have CPU fallback paths.
 *
 * Thread Safety: Not thread-safe. Each thread must use its own instance
 * or provide external synchronization.
 *
 * Memory: Allocates and frees GPU device memory per dispatch operation.
 */
class GeoBackendDispatcher {
public:
    /**
     * @brief Point coordinate (WGS84: latitude, longitude in degrees).
     */
    struct Point {
        double lat_deg = 0.0;
        double lon_deg = 0.0;
    };

    /**
     * @brief Polygon defined by vertices (WGS84 degrees).
     */
    struct Polygon {
        std::vector<Point> vertices;
    };

    /**
     * @brief Result of Haversine distance batch operation.
     */
    struct HaversineResult {
        std::vector<double> distances_km;  ///< Output distances [points1.size()]
        bool                cpu_fallback = true;  ///< true if CPU executed
        int                 error_code = 0;       ///< 0 = success
    };

    /**
     * @brief Result of Vincenty distance batch operation.
     */
    struct VincentyResult {
        std::vector<double> distances_km;  ///< Output distances [points1.size()]
        bool                cpu_fallback = true;  ///< true if CPU executed
        int                 error_code = 0;       ///< 0 = success
    };

    /**
     * @brief Result of point-in-polygon batch operation.
     */
    struct PointInPolygonResult {
        std::vector<uint8_t> containment_mask;  ///< 1=inside, 0=outside [num_test_points]
        bool                 cpu_fallback = true;  ///< true if CPU executed
        int                  error_code = 0;       ///< 0 = success
    };

    /**
     * @brief Constructor.
     *
     * Detects GPU availability at construction time. GPU state remains
     * constant for the lifetime of this dispatcher.
     */
    GeoBackendDispatcher() noexcept;

    /**
     * @brief Destructor.
     */
    ~GeoBackendDispatcher() noexcept;

    // Prevent copying
    GeoBackendDispatcher(const GeoBackendDispatcher&) = delete;
    GeoBackendDispatcher& operator=(const GeoBackendDispatcher&) = delete;

    // Allow moving
    GeoBackendDispatcher(GeoBackendDispatcher&&) noexcept noexcept = default;
    GeoBackendDispatcher& operator=(GeoBackendDispatcher&&) noexcept noexcept = default;

    /**
     * @brief Query whether CUDA GPU is available and functional.
     *
     * Returns true only if:
     * 1. THEMIS_GEO_CUDA is defined at compile time
     * 2. At least one GPU is detected at runtime (cudaGetDeviceCount > 0)
     * 3. GPU device is accessible (cudaGetDevice, cudaGetDeviceProperties OK)
     *
     * @return true if GPU is available for dispatch; false otherwise (CPU only)
     */
    bool isCudaAvailable() const noexcept;

    /**
     * @brief Compute Haversine distances for batch of point pairs.
     *
     * Dispatches to GPU if:
     * - CUDA is available
     * - Batch size exceeds heuristic threshold (~1000 points)
     *
     * Falls back to CPU on any GPU error or if threshold not met.
     *
     * @param points1         First set of points [n] (WGS84 degrees)
     * @param points2         Second set of points [n] (WGS84 degrees)
     * @param earth_radius_km Earth radius (km; typically 6371.0)
     * @return HaversineResult with distances_km and status
     *
     * Gate Target: GATE-A-06-01 ≤ 500ms p99 (GPU), p99 ≤ 50ms (CPU)
     */
    HaversineResult computeHaversineBatch(
        const std::vector<Point>& points1,
        const std::vector<Point>& points2,
        double earth_radius_km = 6371.0) noexcept;

    /**
     * @brief Compute Vincenty distances (ellipsoidal model) for batch.
     *
     * Higher precision than Haversine; appropriate for high-accuracy geodesy.
     * Dispatch strategy same as Haversine.
     *
     * @param points1 First set of points [n]
     * @param points2 Second set of points [n]
     * @return VincentyResult with distances_km and status
     *
     * Gate Target: Similar to Haversine (Phase 2-3 kernel)
     */
    VincentyResult computeVincentyBatch(
        const std::vector<Point>& points1,
        const std::vector<Point>& points2) noexcept;

    /**
     * @brief Test batch of points for containment in polygons.
     *
     * Uses ray-casting algorithm (GPU) or CPU fallback.
     * Dispatches to GPU if batch size exceeds threshold.
     *
     * @param test_points    Points to test [num_test_points]
     * @param polygons       Polygons for containment test
     * @param num_test_points Number of points to test
     * @return PointInPolygonResult with containment_mask [num_test_points]
     *
     * Gate Target: GATE-A-06-02 ≤ 2ms p99 (GPU), p99 ≤ 0.5ms (CPU)
     */
    PointInPolygonResult computePointInPolygonBatch(
        const std::vector<Point>& test_points,
        const std::vector<Polygon>& polygons,
        size_t num_test_points) noexcept;

private:
    /**
     * @brief Detect CUDA GPU availability at runtime.
     * @return true if GPU is available and accessible
     */
    bool detectCudaAvailability() const noexcept;

    /**
     * @brief Decide whether to use GPU for this operation.
     *
     * Uses batch-size heuristic: GPU overhead amortization threshold.
     *
     * @param batch_size Number of work items
     * @return true if should attempt GPU dispatch
     */
    bool shouldUseCuda(size_t batch_size) const noexcept;

    // CPU fallback implementations
    double haversineDistance(
        const Point& p1,
        const Point& p2,
        double earth_radius_km = 6371.0) const noexcept;

    double vincentyDistance(
        const Point& p1,
        const Point& p2) const noexcept;

    bool pointInPolygon(
        const Point& test_point,
        const Polygon& polygon) const noexcept;

    // State
    bool cuda_available_;
    const themis::acceleration::GeoKernelDispatch* dispatch_table_;
    void* fallback_cpu_backend_;  // Opaque pointer to CPU backend state
};

}  // namespace geo
}  // namespace themis
