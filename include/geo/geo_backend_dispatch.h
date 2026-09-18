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

class GeoBackendDispatcher {
public:
    struct Point {
        double lat_deg = 0.0;
        double lon_deg = 0.0;
    };

    struct Polygon {
        std::vector<Point> vertices;
    };

    struct HaversineResult {
        std::vector<double> distances_km;  ///< Output distances [points1.size()]
        bool                cpu_fallback = true;  ///< true if CPU executed
        int                 error_code = 0;       ///< 0 = success
    };

    struct VincentyResult {
        std::vector<double> distances_km;  ///< Output distances [points1.size()]
        bool                cpu_fallback = true;  ///< true if CPU executed
        int                 error_code = 0;       ///< 0 = success
    };

    struct PointInPolygonResult {
        std::vector<uint8_t> containment_mask;  ///< 1=inside, 0=outside [num_test_points]
        bool                 cpu_fallback = true;  ///< true if CPU executed
        int                  error_code = 0;       ///< 0 = success
    };

    GeoBackendDispatcher() noexcept;

    ~GeoBackendDispatcher() noexcept;

    // Prevent copying
    GeoBackendDispatcher(const GeoBackendDispatcher&) = delete;
    GeoBackendDispatcher& operator=(const GeoBackendDispatcher&) = delete;

    // Allow moving
    GeoBackendDispatcher(GeoBackendDispatcher&&) noexcept = default;
    GeoBackendDispatcher& operator=(GeoBackendDispatcher&&) noexcept = default;

    /**
     * @brief Is Cuda Available.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isCudaAvailable() const noexcept;

    HaversineResult computeHaversineBatch(
        const std::vector<Point>& points1,
        const std::vector<Point>& points2,
        double earth_radius_km = 6371.0) noexcept;

    /**
     * @brief Compute Vincenty Batch.
     * @param[in] points1 Input parameter.
     * @param[in] points2 Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    VincentyResult computeVincentyBatch(
        const std::vector<Point>& points1,
        const std::vector<Point>& points2) noexcept;

    /**
     * @brief Compute Point In Polygon Batch.
     * @param[in] test_points Input parameter.
     * @param[in] polygons Input parameter.
     * @param[in] num_test_points Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    PointInPolygonResult computePointInPolygonBatch(
        const std::vector<Point>& test_points,
        const std::vector<Polygon>& polygons,
        size_t num_test_points) noexcept;

private:
    /**
     * @brief Detect Cuda Availability.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool detectCudaAvailability() const noexcept;

    /**
     * @brief Should Use Cuda.
     * @param[in] batch_size Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool shouldUseCuda(size_t batch_size) const noexcept;

    // CPU fallback implementations
    double haversineDistance(
        const Point& p1,
        const Point& p2,
        double earth_radius_km = 6371.0) const noexcept;

    /**
     * @brief Vincenty Distance.
     * @param[in] p1 Input parameter.
     * @param[in] p2 Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    double vincentyDistance(
        const Point& p1,
        const Point& p2) const noexcept;

    /**
     * @brief Point In Polygon.
     * @param[in] test_point Input parameter.
     * @param[in] polygon Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
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
