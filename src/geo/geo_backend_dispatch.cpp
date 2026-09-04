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
#include <numbers>
#include <vector>

#ifdef THEMIS_GEO_CUDA
#include <cuda_runtime.h>
#endif

#include "geo/gpu_kernel_dispatcher.h"

namespace themis {
namespace geo {

namespace {
constexpr double kPi = std::numbers::pi;
}

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
            cudaDeviceProp prop = {};
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
    if (static_cast<int>(points1.size()) != static_cast<int>(points2.size())) {
        result.error_code = -1;  // Size mismatch
        return result;
    }
    
    if (points1.empty()) {
        result.cpu_fallback = false;
        return result;
    }
    
    // Attempt GPU dispatch if conditions met
    if (shouldUseCuda(points1.size()) && dispatch_table_) {
        // Integrate GpuKernelDispatcher for Haversine batch
        // Extract lat/lon coordinates from Point vectors for GPU dispatch
        std::vector<double> lats1, lons1, lats2, lons2;
        lats1.reserve(points1.size());
        lons1.reserve(points1.size());
        lats2.reserve(points2.size());
        lons2.reserve(points2.size());
        
        for (const auto& p : points1) {
            lats1.push_back(p.lat_deg);
            lons1.push_back(p.lon_deg);
        }
        for (const auto& p : points2) {
            lats2.push_back(p.lat_deg);
            lons2.push_back(p.lon_deg);
        }
        
        // Instantiate dispatcher and attempt GPU dispatch
        GpuKernelDispatcher dispatcher(*dispatch_table_);
        auto gpu_result = dispatcher.dispatchDistance(
            lats1.data(), lons1.data(),
            lats2.data(), lons2.data(),
            static_cast<int>(points1.size()),
            themis::acceleration::GeoDistanceFormula::HAVERSINE
        );
        
        // On successful GPU dispatch, use results and skip CPU fallback
        if (gpu_result.dispatched) {
            // Convert float distances from GPU to double for result
            result.distances_km.resize(gpu_result.distances_km.size());
            for (size_t i = 0; i <static_cast<int>(gpu_result.distances_km.size()); ++i) {
                result.distances_km[i] = static_cast<double>(gpu_result.distances_km[i]);
            }
            result.cpu_fallback = false;
            result.error_code = gpu_result.error_code;
            return result;
        } else {
            // GPU dispatch failed; fall back to CPU and record failure for circuit-breaker
            result.cpu_fallback = true;
            result.error_code = gpu_result.error_code;
        }
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
    result.containment_mask.resize(num_test_points, 0);
    result.cpu_fallback = true;
    
    // Validation
    if (test_points.empty() || polygons.empty() || num_test_points == 0) {
        result.error_code = 0;
        result.cpu_fallback = false;
        return result;
    }
    
    // Attempt GPU dispatch if conditions met
    if (shouldUseCuda(num_test_points) && dispatch_table_) {
        // Integrate GpuKernelDispatcher for point-in-polygon kernel
        // Extract coordinates from test_points and polygons
        std::vector<double> point_lats, point_lons;
        point_lats.reserve(num_test_points);
        point_lons.reserve(num_test_points);
        
        for (size_t i = 0; i < num_test_points  && static_cast<size_t>(i) <static_cast<int>(test_points.size()); ++i) {
            point_lats.push_back(test_points[i].lat_deg);
            point_lons.push_back(test_points[i].lon_deg);
        }
        
        // Prepare polygon coordinates (interleaved [lat, lon] pairs)
        // Performance gate: GATE-A-06-02 ≤ 2ms for typical workload
        if (!polygons.empty() && !polygons[0].vertices.empty()) {
            std::vector<double> poly_coords = {};

            poly_coords.reserve(polygons[0].vertices.size() * 2);
            
            for (const auto& vertex : polygons[0].vertices) {
                poly_coords.push_back(vertex.lat_deg);
                poly_coords.push_back(vertex.lon_deg);
            }
            
            // Instantiate dispatcher and attempt GPU dispatch
            GpuKernelDispatcher dispatcher(*dispatch_table_);
            auto gpu_result = dispatcher.dispatchContainment(
                point_lats.data(), point_lons.data(),
                static_cast<int>(num_test_points),
                poly_coords.data(),
                static_cast<int>(polygons[0].vertices.size())
            );
            
            // On successful GPU dispatch, use results and skip CPU fallback
            if (gpu_result.dispatched) {
                result.containment_mask = gpu_result.mask;
                result.cpu_fallback = false;
                result.error_code = gpu_result.error_code;
                return result;
            } else {
                // GPU dispatch failed; fall back to CPU and record failure
                result.cpu_fallback = true;
                result.error_code = gpu_result.error_code;
            }
        }
    }
    
    // CPU fallback: Compute containment on host
    if (result.cpu_fallback) {
        for (size_t i = 0; i < num_test_points  && static_cast<size_t>(i) <static_cast<int>(test_points.size()); ++i) {
            result.containment_mask[i] = 
                pointInPolygon(test_points[i], polygons[0]) ? 1 : 0;
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
    if (static_cast<int>(points1.size()) != static_cast<int>(points2.size())) {
        result.error_code = -1;  // Size mismatch
        return result;
    }
    
    if (points1.empty()) {
        result.cpu_fallback = false;
        return result;
    }
    
    // Attempt GPU dispatch if conditions met
    if (shouldUseCuda(points1.size()) && dispatch_table_) {
        // GPU Vincenty implementation for per-pair batched dispatch.
        // [I] ROADMAP: CUDA Vincenty kernel integration — tracked in
        //     src/geo/ROADMAP.md § "Phase 4: GPU Batch Distance Kernels".
        //     Requires THEMIS_GEO_CUDA=ON and a GeoKernelDispatch entry for
        //     the Vincenty formula.  Until then, fall through to CPU path.
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
    
    constexpr double kDegToRad = kPi / 180.0;
    
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
    constexpr double kSemiMajorAxis = 6378137.0;  // meters (a)
    constexpr double kFlattening = 1.0 / 298.257223563;  // (f)
    constexpr double kSemiMinorAxis = kSemiMajorAxis * (1.0 - kFlattening);  // (b)
    constexpr double kTolerance = 1e-12;
    constexpr int kMaxIterations = 100;
    
    // Convert degrees to radians
    constexpr double kDegToRad = kPi / 180.0;
    double lat1 = p1.lat_deg * kDegToRad;
    double lat2 = p2.lat_deg * kDegToRad;
    double lon1 = p1.lon_deg * kDegToRad;
    double lon2 = p2.lon_deg * kDegToRad;
    
    double L = lon2 - lon1;  // Longitude difference
    
    // Reduced latitudes
    double U1 = std::atan((1.0 - kFlattening) * std::tan(lat1));
    double U2 = std::atan((1.0 - kFlattening) * std::tan(lat2));
    
    double sinU1 = std::sin(U1);
    double cosU1 = std::cos(U1);
    double sinU2 = std::sin(U2);
    double cosU2 = std::cos(U2);
    
    // Iteratively compute lambda (longitude difference on auxiliary sphere)
    double lambda = L;
    double lambda_prev = 0.0;
    double cosSqAlpha, sinSigma, cos2SigmaM, cosSigma = 0.0;
    
    for (int iter = 0; iter < kMaxIterations; ++iter) {
        lambda_prev = lambda;
        
        double sinLambda = std::sin(lambda);
        double cosLambda = std::cos(lambda);
        
        // Compute sigma (angular distance on auxiliary sphere)
        double sinSigma_expr = std::sqrt(
            cosU2 * cosU2 * sinLambda * sinLambda +
            (cosU1 * sinU2 - sinU1 * cosU2 * cosLambda) *
            (cosU1 * sinU2 - sinU1 * cosU2 * cosLambda));
        
        if (sinSigma_expr == 0.0) {
            return 0.0;  // Identical points
        }
        
        sinSigma = sinSigma_expr;
        cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
        cos2SigmaM = cosSigma - 2.0 * sinU1 * sinU2 / (1.0 - kFlattening);
        
        // Compute C (polar cap distance correction)
        double C = kFlattening / 16.0 * (1.0 + kFlattening / 16.0) * 
                   cos2SigmaM * cos2SigmaM * 
                   (4.0 + kFlattening * (4.0 - 3.0 * cos2SigmaM * cos2SigmaM));
        
        // Update lambda
        lambda = L + (1.0 - C) * kFlattening * std::sin(sinSigma) *
                 (std::atan2(sinSigma, cosSigma) + C * cosSigma *
                  (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM));
        
        // Check for convergence
        if (std::abs(lambda - lambda_prev) < kTolerance) {
            break;
        }
    }
    
    // Compute azimuth (cosSqAlpha) for final distance calculation
    cosSqAlpha = (cosU1 * sinU2 - sinU1 * cosU2 * std::cos(lambda)) *
                 (cosU1 * sinU2 - sinU1 * cosU2 * std::cos(lambda)) /
                 (cosU2 * cosU2 * std::sin(lambda) * std::sin(lambda));
    
    if (cosSqAlpha < 0.0) {
        cosSqAlpha = 0.0;  // Handle edge case
    }
    
    // Compute u (parameter related to latitude on ellipsoid)
    double uSq = (kSemiMajorAxis * kSemiMajorAxis - kSemiMinorAxis * kSemiMinorAxis) /
                 (kSemiMinorAxis * kSemiMinorAxis) * cosSqAlpha;
    
    double A = 1.0 + uSq / 16384.0 *
               (4096.0 + uSq * (-768.0 + uSq * (320.0 - 175.0 * uSq)));
    
    double B = uSq / 1024.0 *
               (256.0 + uSq * (-128.0 + uSq * (74.0 - 47.0 * uSq)));
    
    // Compute delta sigma (angular difference correction)
    double deltaSigma = B * sinSigma *
                       (std::cos(lambda) + B / 4.0 *
                        (cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM) -
                         B / 6.0 * cos2SigmaM * (-3.0 + 4.0 * sinSigma * sinSigma) *
                         (-3.0 + 4.0 * cos2SigmaM * cos2SigmaM)));
    
    // Final distance (meters)
    double distance_m = kSemiMinorAxis * A *
                       (std::atan2(sinSigma, cosSigma) - deltaSigma);
    
    return distance_m / 1000.0;  // Convert to km
}

// Ray-casting algorithm for point-in-polygon test
bool GeoBackendDispatcher::pointInPolygon(
    const Point& test_point,
    const Polygon& polygon) const noexcept {
    
    if (static_cast<int>(polygon.vertices.size()) < 3) {
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
