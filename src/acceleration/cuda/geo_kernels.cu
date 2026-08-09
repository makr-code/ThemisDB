// CUDA Kernels for Geospatial Operations
// ThemisDB Hardware Acceleration
//
// Implements the GeoDistanceFn and GeoContainmentFn interfaces declared in
// include/acceleration/kernel_invocation.h using CUDA device kernels.
//
// Conformance notes:
//  - GeoDistanceFn signature: see kernel_invocation.h
//  - GeoContainmentFn signature: see kernel_invocation.h
//  - opaque_stream is cast to cudaStream_t; pass nullptr for the default stream.

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cmath>
#include <cstdint>
#include "acceleration/kernel_invocation.h"

namespace themis {
namespace acceleration {
namespace cuda {

// =============================================================================
// Device-side constants
// =============================================================================

__constant__ double kEarthRadiusKm = 6371.0;
__constant__ double kPi            = 3.141592653589793238462643383279502884;

// WGS-84 ellipsoid constants (for Vincenty)
__constant__ double kWgsA          = 6378137.0;           // semi-major axis (m)
__constant__ double kWgsF          = 1.0 / 298.257223563; // flattening
__constant__ double kWgsB          = 6356752.314245;      // semi-minor axis (m)
__constant__ double kVincentyTol   = 1e-12;

// =============================================================================
// Haversine distance kernel
// =============================================================================

/**
 * Compute per-pair Haversine distances in kilometres.
 *
 * Thread layout: one thread per (point-pair) index.
 * Grid:  ceil(count / 256) blocks
 * Block: 256 threads
 *
 * @param lats1    Input latitudes  set 1  [count]
 * @param lons1    Input longitudes set 1  [count]
 * @param lats2    Input latitudes  set 2  [count]
 * @param lons2    Input longitudes set 2  [count]
 * @param out      Output distances in km  [count]
 * @param count    Number of point pairs
 */
__global__ void haversineDistanceKernel(
    const double* lats1,
    const double* lons1,
    const double* lats2,
    const double* lons2,
    float*        out,
    int           count
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    // Degrees → radians
    const double lat1 = lats1[i] * kPi / 180.0;
    const double lon1 = lons1[i] * kPi / 180.0;
    const double lat2 = lats2[i] * kPi / 180.0;
    const double lon2 = lons2[i] * kPi / 180.0;

    const double dlat = lat2 - lat1;
    const double dlon = lon2 - lon1;

    const double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
                     cos(lat1) * cos(lat2) *
                     sin(dlon / 2.0) * sin(dlon / 2.0);

    out[i] = static_cast<float>(kEarthRadiusKm * 2.0 * atan2(sqrt(a), sqrt(1.0 - a)));
}

// =============================================================================
// Vincenty distance kernel (WGS-84 ellipsoid)
// =============================================================================

/**
 * Compute per-pair Vincenty geodesic distances in kilometres.
 * Uses the WGS-84 ellipsoid for a more accurate geodesic model than Haversine;
 * output precision remains bounded by float kilometre storage.
 *
 * Thread layout: one thread per (point-pair) index.
 * Grid:  ceil(count / 256) blocks
 * Block: 256 threads
 *
 * For nearly-antipodal points where the iterative formula does not converge
 * within kMaxIterations, falls back to Haversine.
 *
 * @param lats1    Input latitudes  set 1  [count]
 * @param lons1    Input longitudes set 1  [count]
 * @param lats2    Input latitudes  set 2  [count]
 * @param lons2    Input longitudes set 2  [count]
 * @param out      Output distances in km  [count]
 * @param count    Number of point pairs
 */
__global__ void vincentyDistanceKernel(
    const double* lats1,
    const double* lons1,
    const double* lats2,
    const double* lons2,
    float*        out,
    int           count
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    // Degrees → radians
    const double phi1 = lats1[i] * kPi / 180.0;
    const double phi2 = lats2[i] * kPi / 180.0;
    const double L    = (lons2[i] - lons1[i]) * kPi / 180.0;

    const double U1    = atan((1.0 - kWgsF) * tan(phi1));
    const double U2    = atan((1.0 - kWgsF) * tan(phi2));
    const double sinU1 = sin(U1), cosU1 = cos(U1);
    const double sinU2 = sin(U2), cosU2 = cos(U2);

    // Degenerate case: one or both endpoints at a geographic pole
    if (cosU1 < kVincentyTol && cosU2 < kVincentyTol) {
        if (sinU1 * sinU2 > 0.0) {
            out[i] = 0.0f; // same pole
        } else {
            // Opposite poles: half the WGS-84 meridional circumference (m → km)
            out[i] = 20003931.459f / 1000.0f;
        }
        return;
    }

    double lambda   = L;
    double sinSigma = 0.0, cosSigma = 0.0, sigma = 0.0;
    double sinAlpha = 0.0, cos2Alpha = 0.0, cos2SigmaM = 0.0;
    bool converged = false;
    const int kMaxIterations = 200;

    for (int iter = 0; iter < kMaxIterations; ++iter) {
        const double sinLambda = sin(lambda);
        const double cosLambda = cos(lambda);

        const double a1 = cosU2 * sinLambda;
        const double a2 = cosU1 * sinU2 - sinU1 * cosU2 * cosLambda;
        sinSigma        = sqrt(a1 * a1 + a2 * a2);

        if (sinSigma < kVincentyTol) {
            out[i] = 0.0f; // coincident points
            return;
        }

        cosSigma   = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
        sigma      = atan2(sinSigma, cosSigma);
        sinAlpha   = cosU1 * cosU2 * sinLambda / sinSigma;
        cos2Alpha  = 1.0 - sinAlpha * sinAlpha;
        cos2SigmaM = (cos2Alpha > kVincentyTol) ? cosSigma - 2.0 * sinU1 * sinU2 / cos2Alpha : 0.0;

        const double C           = kWgsF / 16.0 * cos2Alpha * (4.0 + kWgsF * (4.0 - 3.0 * cos2Alpha));
        const double lambda_prev = lambda;
        lambda = L + (1.0 - C) * kWgsF * sinAlpha
                     * (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)));

        if (fabs(lambda - lambda_prev) <= kVincentyTol) {
            converged = true;
            break;
        }
    }

    if (!converged) {
        // Nearly-antipodal case: fall back to Haversine
        const double lat1 = lats1[i] * kPi / 180.0;
        const double lon1 = lons1[i] * kPi / 180.0;
        const double lat2 = lats2[i] * kPi / 180.0;
        const double lon2 = lons2[i] * kPi / 180.0;

        const double dlat = lat2 - lat1;
        const double dlon = lon2 - lon1;

        const double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
                         cos(lat1) * cos(lat2) *
                         sin(dlon / 2.0) * sin(dlon / 2.0);

        out[i] = static_cast<float>(kEarthRadiusKm * 2.0 * atan2(sqrt(a), sqrt(1.0 - a)));
        return;
    }

    const double u2     = cos2Alpha * (kWgsA * kWgsA - kWgsB * kWgsB) / (kWgsB * kWgsB);
    const double kA     = 1.0 + u2 / 16384.0 * (4096.0 + u2 * (-768.0 + u2 * (320.0 - 175.0 * u2)));
    const double kB     = u2 / 1024.0 * (256.0 + u2 * (-128.0 + u2 * (74.0 - 47.0 * u2)));
    const double dSigma = kB * sinSigma
                          * (cos2SigmaM
                             + kB / 4.0
                                   * (cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)
                                      - kB / 6.0 * cos2SigmaM * (-3.0 + 4.0 * sinSigma * sinSigma)
                                            * (-3.0 + 4.0 * cos2SigmaM * cos2SigmaM)));
    out[i] = static_cast<float>((kWgsB * kA * (sigma - dSigma)) / 1000.0);  // metres → km
}

// =============================================================================
// Point-in-polygon kernel (ray-casting algorithm)
// =============================================================================

/**
 * Test whether each point lies inside a convex or concave polygon.
 * Uses the ray-casting algorithm: cast a ray along +longitude; count
 * edge crossings.  Odd count → inside.
 *
 * Thread layout: one thread per point.
 * Grid:  ceil(numPoints / 256) blocks
 * Block: 256 threads
 *
 * @param point_lats      Test-point latitudes  [numPoints]
 * @param point_lons      Test-point longitudes [numPoints]
 * @param numPoints       Number of test points
 * @param polygon_coords  Interleaved [lat, lon] × numVertices
 * @param numVertices     Number of polygon vertices
 * @param results         Output: 1 = inside, 0 = outside [numPoints]
 */
__global__ void pointInPolygonKernel(
    const double* point_lats,
    const double* point_lons,
    int           numPoints,
    const double* polygon_coords,
    int           numVertices,
    uint8_t*      results
) {
    const int p = blockIdx.x * blockDim.x + threadIdx.x;
    if (p >= numPoints) return;

    const double testLat = point_lats[p];
    const double testLon = point_lons[p];

    bool inside = false;
    int  j      = numVertices - 1;

    for (int i = 0; i < numVertices; ++i) {
        const double lat_i = polygon_coords[i * 2];
        const double lon_i = polygon_coords[i * 2 + 1];
        const double lat_j = polygon_coords[j * 2];
        const double lon_j = polygon_coords[j * 2 + 1];

        if (((lon_i > testLon) != (lon_j > testLon)) &&
            (testLat < (lat_j - lat_i) * (testLon - lon_i) / (lon_j - lon_i) + lat_i)) {
            inside = !inside;
        }
        j = i;
    }

    results[p] = inside ? 1u : 0u;
}

// =============================================================================
// Kernel launchers — conform to GeoDistanceFn / GeoContainmentFn typedefs
// =============================================================================

// Module-level block size tuned at initialize() time via
// cudaOccupancyMaxPotentialBlockSize().  Default 256 is safe for any SM 7.0+
// device; CUDAGeoBackend::initialize() overwrites this with the optimal value.
static int g_cuda_geo_block_size = 256;

extern "C" {

/**
 * Launch the geospatial distance kernel (Haversine or Vincenty).
 * Matches the GeoDistanceFn typedef in kernel_invocation.h.
 *
 * Selects the appropriate kernel based on the formula parameter:
 *  - HAVERSINE: spherical Earth model (fast, ±0.5% accuracy)
 *  - VINCENTY:  WGS-84 ellipsoid model (slower, more accurate model; output
 *               precision remains bounded by float kilometre storage)
 *
 * @return 0 on success, non-zero CUDA error code on failure.
 */
int launchGeoDistanceKernel(
    const double*                          d_lats1,
    const double*                          d_lons1,
    const double*                          d_lats2,
    const double*                          d_lons2,
    float*                                 d_distances,
    int                                    count,
    themis::acceleration::GeoDistanceFormula formula,
    void*                                  opaque_stream
) {
    if (count <= 0) return 0;

    const int kBlockSize = g_cuda_geo_block_size;
    const dim3 blockDim(kBlockSize);
    const dim3 gridDim((count + kBlockSize - 1) / kBlockSize);

    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    // Dispatch to appropriate kernel based on formula
    if (formula == themis::acceleration::GeoDistanceFormula::VINCENTY) {
        vincentyDistanceKernel<<<gridDim, blockDim, 0, stream>>>(
            d_lats1, d_lons1, d_lats2, d_lons2, d_distances, count);
    } else {
        // Default to Haversine for HAVERSINE or unknown formulas
        haversineDistanceKernel<<<gridDim, blockDim, 0, stream>>>(
            d_lats1, d_lons1, d_lats2, d_lons2, d_distances, count);
    }

    const cudaError_t err = cudaGetLastError();
    return static_cast<int>(err);
}


/**
 * Launch the point-in-polygon kernel.
 * Matches the GeoContainmentFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero CUDA error code on failure.
 */
int launchGeoContainmentKernel(
    const double* d_point_lats,
    const double* d_point_lons,
    int           numPoints,
    const double* d_polygon_coords,
    int           numPolygonVertices,
    uint8_t*      d_results,
    void*         opaque_stream
) {
    if (numPoints <= 0) return 0;

    const int kBlockSize = g_cuda_geo_block_size;
    const dim3 blockDim(kBlockSize);
    const dim3 gridDim((numPoints + kBlockSize - 1) / kBlockSize);

    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    pointInPolygonKernel<<<gridDim, blockDim, 0, stream>>>(
        d_point_lats, d_point_lons, numPoints,
        d_polygon_coords, numPolygonVertices,
        d_results);

    const cudaError_t err = cudaGetLastError();
    return static_cast<int>(err);
}

/**
 * Update the block size used by the geo kernel launchers.
 *
 * Called from CUDAGeoBackend::initialize() with the value returned by
 * cudaOccupancyMaxPotentialBlockSize().  Falls back to 256 when not called.
 *
 * @param blockSize  Number of threads per block (must be a multiple of 32
 *                   and ≤ the device's maxThreadsPerBlock).
 */
void setGeoKernelBlockSize(int blockSize) {
    g_cuda_geo_block_size = blockSize;
}

/**
 * Query the CUDA occupancy API for the Haversine distance kernel and update
 * g_cuda_geo_block_size with the device-optimal block size.
 *
 * Called by CUDAGeoBackend::initialize() so the launcher uses the tuned value
 * from the very first kernel dispatch.
 *
 * @return  The occupancy-tuned block size (also stored in g_cuda_geo_block_size).
 */
int tuneGeoKernelBlockSize() {
    int minGridSize   = 0;
    int tunedBlockSize = 256;
    cudaError_t err = cudaOccupancyMaxPotentialBlockSize(
        &minGridSize, &tunedBlockSize, haversineDistanceKernel, 0, 0);
    if (err == cudaSuccess && tunedBlockSize > 0) {
        // Round to nearest multiple of 32 (warp size), minimum 32.
        tunedBlockSize = (tunedBlockSize / 32) * 32;
        if (tunedBlockSize < 32) tunedBlockSize = 32;
        g_cuda_geo_block_size = tunedBlockSize;
    }
    return g_cuda_geo_block_size;
}

} // extern "C"

/**
 * Populate a GeoKernelDispatch table with the CUDA kernel launchers defined
 * in this translation unit.
 *
 * Call this function during CUDA backend initialisation to wire the dispatch
 * table used by the BackendRegistry.
 */
void populateCudaGeoDispatch(GeoKernelDispatch& dispatch) {
    dispatch.launchDistance    = &launchGeoDistanceKernel;
    dispatch.launchContainment = &launchGeoContainmentKernel;
}

} // namespace cuda
} // namespace acceleration
} // namespace themis
