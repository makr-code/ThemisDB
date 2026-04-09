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

extern "C" {

/**
 * Launch the Haversine batch-distance kernel.
 * Matches the GeoDistanceFn typedef in kernel_invocation.h.
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
    themis::acceleration::GeoDistanceFormula /*formula*/,  // Haversine only in this impl
    void*                                  opaque_stream
) {
    if (count <= 0) return 0;

    // Query occupancy-tuned block size for the haversine kernel.
    int minGridSize = 0, optBlockSize = 0;
    const cudaError_t occErr = cudaOccupancyMaxPotentialBlockSize(
        &minGridSize, &optBlockSize, haversineDistanceKernel, 0, 0);
    const int kBlockSize = (occErr == cudaSuccess && optBlockSize > 0)
                           ? optBlockSize : 256;

    const dim3 blockDim(kBlockSize);
    const dim3 gridDim((count + kBlockSize - 1) / kBlockSize);

    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    haversineDistanceKernel<<<gridDim, blockDim, 0, stream>>>(
        d_lats1, d_lons1, d_lats2, d_lons2, d_distances, count);

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

    // Query occupancy-tuned block size for the point-in-polygon kernel.
    int minGridSize = 0, optBlockSize = 0;
    const cudaError_t occErr = cudaOccupancyMaxPotentialBlockSize(
        &minGridSize, &optBlockSize, pointInPolygonKernel, 0, 0);
    const int kBlockSize = (occErr == cudaSuccess && optBlockSize > 0)
                           ? optBlockSize : 256;

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
