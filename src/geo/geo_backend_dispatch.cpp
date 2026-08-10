/**
 * @file geo_backend_dispatch.cpp
 * @brief Runtime GPU/CPU backend selection with device detection.
 *
 * Implements GeoBackendDispatcher with compile-time (THEMIS_GEO_CUDA) and
 * runtime GPU device detection. Provides graceful CPU fallback when GPU
 * is unavailable or encounters errors.
 *
 * Thread Safety: Each thread should use its own GeoBackendDispatcher instance.
 * CUDA operations are not thread-safe; device memory allocation/deallocation
 * is serialized per-thread via thread-local storage.
 */

#include "geo/geo_backend_dispatch.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#ifdef THEMIS_GEO_CUDA
#include <cuda_runtime.h>
#endif

namespace themis {
namespace geo {

// ============================================================================
// GeoBackendDispatcher Implementation
// ============================================================================

GeoBackendDispatcher::GeoBackendDispatcher() noexcept
    : cuda_available_(detectCudaAvailability()),
      dispatch_table_(nullptr),
      fallback_cpu_backend_(nullptr) {}

GeoBackendDispatcher::~GeoBackendDispatcher() noexcept {
    // No explicit cleanup needed; dispatch_table_ and fallback_cpu_backend_
    // are externally managed.
}

bool GeoBackendDispatcher::detectCudaAvailability() const noexcept {
#ifdef THEMIS_GEO_CUDA
    int device_count = 0;
    cudaError_t status = cudaGetDeviceCount(&device_count);
    
    // Return true only if CUDA is compiled in AND at least one GPU is detected
    if (status == cudaSuccess && device_count > 0) {
        // Additional check: ensure default device is accessible
        int current_device = -1;
        if (cudaGetDevice(&current_device) == cudaSuccess && current_device >= 0) {
            cudaDeviceProp prop;
            if (cudaGetDeviceProperties(&prop, current_device) == cudaSuccess) {
                return true;
            }
        }
    }
    return false;
#else
    // THEMIS_GEO_CUDA not defined; CPU-only mode
    return false;
#endif
}

bool GeoBackendDispatcher::isCudaAvailable() const noexcept {
    return cuda_available_;
}

bool GeoBackendDispatcher::shouldUseCuda(size_t batch_size) const noexcept {
    if (!cuda_available_) {
        return false;
    }
    
    // Performance heuristic: use CUDA only for sufficiently large batches.
    // Threshold tuned to amortize GPU transfer overhead.
    // Gate targets: Haversine ≤500ms, Point-in-Polygon ≤2ms
    // Adjust based on GATE-A-06-01, GATE-A-06-02 performance validation.
    constexpr size_t kCudaMinBatchSize = 1000;  // Tunable; see benchmarks
    
    return batch_size >= kCudaMinBatchSize;
}

GeoBackendDispatcher::HaversineResult GeoBackendDispatcher::computeHaversineBatch(
    const std::vector<Point>& points1,
    const std::vector<Point>& points2,
    double earth_radius_km) noexcept {
    
    HaversineResult result;
    result.distances_km.resize(points1.size());
    result.cpu_fallback = true;
    
    // Validation
    if (points1.size() != points2.size()) {
        result.error_code = -1;  // Size mismatch
        return result;
    }
    
    if (points1.empty()) {
        result.cpu_fallback = false;
        return result;
    }
    
    // Attempt GPU dispatch if conditions met
    if (shouldUseCuda(points1.size()) && dispatch_table_) {
        // GPU implementation would populate distances_km via GPU kernel
        // For now, fall back to CPU (GPU kernel dispatch in progress)
        // TODO: Integrate GpuKernelDispatcher for Haversine batch
        result.cpu_fallback = true;
    }
    
    // CPU fallback: Compute distances on host
    if (result.cpu_fallback) {
        for (size_t i = 0; i < points1.size(); ++i) {
            result.distances_km[i] = haversineDistance(
                points1[i], points2[i], earth_radius_km);
        }
        result.error_code = 0;
    }
    
    return result;
}

GeoBackendDispatcher::PointInPolygonResult GeoBackendDispatcher::computePointInPolygonBatch(
    const std::vector<Point>& test_points,
    const std::vector<Polygon>& polygons,
    size_t num_test_points) noexcept {
    
    PointInPolygonResult result;
    result.containment_mask.resize(num_test_points, 0u);
    result.cpu_fallback = true;
    
    // Validation
    if (test_points.empty() || polygons.empty() || num_test_points == 0) {
        result.error_code = 0;
        result.cpu_fallback = false;
        return result;
    }
    
    // Attempt GPU dispatch if conditions met
    if (shouldUseCuda(num_test_points) && dispatch_table_) {
        // GPU implementation via GpuKernelDispatcher
        // TODO: Integrate GpuKernelDispatcher for point-in-polygon kernel
        // Performance gate: GATE-A-06-02 ≤ 2ms for typical workload
        result.cpu_fallback = true;
    }
    
    // CPU fallback: Compute containment on host
    if (result.cpu_fallback) {
        for (size_t i = 0; i < num_test_points && i < test_points.size(); ++i) {
            result.containment_mask[i] = 
                pointInPolygon(test_points[i], polygons[0]) ? 1u : 0u;
        }
        result.error_code = 0;
    }
    
    return result;
}

GeoBackendDispatcher::VincentyResult GeoBackendDispatcher::computeVincentyBatch(
    const std::vector<Point>& points1,
    const std::vector<Point>& points2) noexcept {
    
    VincentyResult result;
    result.distances_km.resize(points1.size());
    result.cpu_fallback = true;
    
    // Validation
    if (points1.size() != points2.size()) {
        result.error_code = -1;  // Size mismatch
        return result;
    }
    
    if (points1.empty()) {
        result.cpu_fallback = false;
        return result;
    }
    
    // Attempt GPU dispatch if conditions met
    if (shouldUseCuda(points1.size()) && dispatch_table_) {
        // GPU Vincenty implementation for per-pair batched dispatch
        // TODO: Integrate Vincenty CUDA kernel with batch dispatch
        result.cpu_fallback = true;
    }
    
    // CPU fallback: Compute Vincenty distances on host
    if (result.cpu_fallback) {
        for (size_t i = 0; i < points1.size(); ++i) {
            result.distances_km[i] = vincentyDistance(points1[i], points2[i]);
        }
        result.error_code = 0;
    }
    
    return result;
}

// ============================================================================
// CPU Fallback Implementations (Reference)
// ============================================================================

// Haversine formula for geodesic distance
double GeoBackendDispatcher::haversineDistance(
    const Point& p1,
    const Point& p2,
    double earth_radius_km) const noexcept {
    
    constexpr double kDegToRad = M_PI / 180.0;
    
    double lat1_rad = p1.lat_deg * kDegToRad;
    double lat2_rad = p2.lat_deg * kDegToRad;
    double lon1_rad = p1.lon_deg * kDegToRad;
    double lon2_rad = p2.lon_deg * kDegToRad;
    
    double dlat = lat2_rad - lat1_rad;
    double dlon = lon2_rad - lon1_rad;
    
    double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
               std::cos(lat1_rad) * std::cos(lat2_rad) *
               std::sin(dlon / 2.0) * std::sin(dlon / 2.0);
    
    double c = 2.0 * std::asin(std::sqrt(a));
    return earth_radius_km * c;
}

// Vincenty formula for ellipsoidal distance (higher precision)
double GeoBackendDispatcher::vincentyDistance(
    const Point& p1,
    const Point& p2) const noexcept {
    
    // WGS84 ellipsoid parameters
    constexpr double kSemiMajorAxis = 6378137.0;  // meters
    constexpr double kFlattening = 1.0 / 298.257223563;
    constexpr double kSemiMinorAxis = kSemiMajorAxis * (1.0 - kFlattening);
    
    // TODO: Implement full Vincenty formula
    // For now, use Haversine as approximation
    return haversineDistance(p1, p2, kSemiMajorAxis / 1000.0);
}

// Ray-casting algorithm for point-in-polygon test
bool GeoBackendDispatcher::pointInPolygon(
    const Point& test_point,
    const Polygon& polygon) const noexcept {
    
    if (polygon.vertices.size() < 3) {
        return false;  // Degenerate polygon
    }
    
    // Ray-casting algorithm: cast ray to right and count crossings
    bool inside = false;
    size_t n = polygon.vertices.size();
    
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        const Point& vi = polygon.vertices[i];
        const Point& vj = polygon.vertices[j];
        
        bool xi_condition = (vi.lon_deg > test_point.lon_deg) != 
                           (vj.lon_deg > test_point.lon_deg);
        
        if (xi_condition) {
            // Compute y-intercept of edge with test point's latitude
            double slope = (vj.lat_deg - vi.lat_deg) / (vj.lon_deg - vi.lon_deg);
            double y_intersect = vi.lat_deg + 
                                slope * (test_point.lon_deg - vi.lon_deg);
            
            if (test_point.lat_deg < y_intersect) {
                inside = !inside;
            }
        }
    }
    
    return inside;
}

}  // namespace geo
}  // namespace themis
