// HIP Kernels for Geospatial Operations
// ThemisDB Hardware Acceleration — AMD ROCm/HIP backend
//
// Implements Haversine batch distance computation and point-in-polygon
// containment testing on AMD GPUs via the HIP programming model.
//
// Kernel launcher functions conform to the GeoDistanceFn / GeoContainmentFn
// typedefs declared in include/acceleration/kernel_invocation.h
// (INTERFACE_VERSION 100).
//
// opaque_stream must be a hipStream_t cast to void*.  Pass nullptr to use the
// default (null) HIP stream.
//
// This file is the HIP-native equivalent of src/acceleration/cuda/geo_kernels.cu.

#include <hip/hip_runtime.h>
#include <cmath>
#include <cstdint>
#include "acceleration/kernel_invocation.h"

namespace themis {
namespace acceleration {
namespace hip {

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
    const double* __restrict__ lats1,
    const double* __restrict__ lons1,
    const double* __restrict__ lats2,
    const double* __restrict__ lons2,
    float*        __restrict__ out,
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
    const double* __restrict__ point_lats,
    const double* __restrict__ point_lons,
    int                        numPoints,
    const double* __restrict__ polygon_coords,
    int                        numVertices,
    uint8_t*      __restrict__ results
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
// hipOccupancyMaxPotentialBlockSize().  AMD GCN devices (warpSize == 64) use 64
// as their baseline to avoid half-occupancy.  HIPGeoBackend::initialize()
// overwrites this with the device-optimal value.
static int g_hip_geo_block_size = 256;

extern "C" {

/**
 * Launch the Haversine batch-distance kernel.
 * Matches the GeoDistanceFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero hipError_t on failure.
 */
int hip_launchGeoDistanceKernel(
    const double*                             d_lats1,
    const double*                             d_lons1,
    const double*                             d_lats2,
    const double*                             d_lons2,
    float*                                    d_distances,
    int                                       count,
    themis::acceleration::GeoDistanceFormula  /*formula*/,  // Haversine only in this impl
    void*                                     opaque_stream
) {
    if (count <= 0) return 0;

    const int kBlockSize = g_hip_geo_block_size;
    const dim3 blockDim(kBlockSize);
    const dim3 gridDim((static_cast<unsigned>(count) + static_cast<unsigned>(kBlockSize) - 1u) / static_cast<unsigned>(kBlockSize));

    const hipStream_t stream = static_cast<hipStream_t>(opaque_stream);

    hipLaunchKernelGGL(haversineDistanceKernel,
        gridDim, blockDim, 0, stream,
        d_lats1, d_lons1, d_lats2, d_lons2, d_distances, count);

    return static_cast<int>(hipGetLastError());
}

/**
 * Launch the point-in-polygon containment kernel.
 * Matches the GeoContainmentFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero hipError_t on failure.
 */
int hip_launchGeoContainmentKernel(
    const double* d_point_lats,
    const double* d_point_lons,
    int           numPoints,
    const double* d_polygon_coords,
    int           numPolygonVertices,
    uint8_t*      d_results,
    void*         opaque_stream
) {
    if (numPoints <= 0) return 0;

    const int kBlockSize = g_hip_geo_block_size;
    const dim3 blockDim(kBlockSize);
    const dim3 gridDim((static_cast<unsigned>(numPoints) + static_cast<unsigned>(kBlockSize) - 1u) / static_cast<unsigned>(kBlockSize));

    const hipStream_t stream = static_cast<hipStream_t>(opaque_stream);

    hipLaunchKernelGGL(pointInPolygonKernel,
        gridDim, blockDim, 0, stream,
        d_point_lats, d_point_lons, numPoints,
        d_polygon_coords, numPolygonVertices,
        d_results);

    return static_cast<int>(hipGetLastError());
}

/**
 * Update the block size used by the HIP geo kernel launchers.
 *
 * Called from HIPGeoBackend::initialize() with the value returned by
 * hipOccupancyMaxPotentialBlockSize().  AMD GCN devices (warpSize == 64)
 * default to 64 threads to avoid half-occupancy.
 *
 * @param blockSize  Number of threads per block.
 */
void hipSetGeoKernelBlockSize(int blockSize) {
    g_hip_geo_block_size = blockSize;
}

} // extern "C"

/**
 * Populate a GeoKernelDispatch table with the HIP kernel launchers defined
 * in this translation unit.
 *
 * Call this function during HIP backend initialisation to wire the dispatch
 * table used by the BackendRegistry.
 */
void populateHIPGeoDispatch(GeoKernelDispatch& dispatch) {
    dispatch.launchDistance    = &hip_launchGeoDistanceKernel;
    dispatch.launchContainment = &hip_launchGeoContainmentKernel;
}

} // namespace hip
} // namespace acceleration
} // namespace themis
