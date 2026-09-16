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
// Point set-operation kernels (single-pair operations)
// =============================================================================

/**
 * Compute ST_UNION for two points.
 *
 * Input layout:
 *   points_xy = [x1, y1, x2, y2]
 *
 * Output layout:
 *   out_count = 1 when points are equal, 2 otherwise
 *   out_points_xy = [x1, y1, x2, y2] (second point ignored when out_count=1)
 */
__global__ void pointUnionKernel(
    const double* points_xy,
    double*       out_points_xy,
    int*          out_count
) {
    if (blockIdx.x != 0 || threadIdx.x != 0) {
        return;
    }

    constexpr double kPointEqEpsilon = 1e-12;
    const double x1 = points_xy[0];
    const double y1 = points_xy[1];
    const double x2 = points_xy[2];
    const double y2 = points_xy[3];

    out_points_xy[0] = x1;
    out_points_xy[1] = y1;

    if (fabs(x1 - x2) <= kPointEqEpsilon && fabs(y1 - y2) <= kPointEqEpsilon) {
        *out_count = 1;
        out_points_xy[2] = x1;
        out_points_xy[3] = y1;
        return;
    }

    *out_count = 2;
    out_points_xy[2] = x2;
    out_points_xy[3] = y2;
}

/**
 * Compute ST_DIFFERENCE for two points (geom1 \ geom2).
 *
 * Input layout:
 *   points_xy = [x1, y1, x2, y2]
 *
 * Output layout:
 *   out_count = 0 when points are equal, 1 otherwise
 *   out_points_xy = [x1, y1, _, _]
 */
__global__ void pointDifferenceKernel(
    const double* points_xy,
    double*       out_points_xy,
    int*          out_count
) {
    if (blockIdx.x != 0 || threadIdx.x != 0) {
        return;
    }

    constexpr double kPointEqEpsilon = 1e-12;
    const double x1 = points_xy[0];
    const double y1 = points_xy[1];
    const double x2 = points_xy[2];
    const double y2 = points_xy[3];

    out_points_xy[0] = x1;
    out_points_xy[1] = y1;
    out_points_xy[2] = x1;
    out_points_xy[3] = y1;

    if (fabs(x1 - x2) <= kPointEqEpsilon && fabs(y1 - y2) <= kPointEqEpsilon) {
        *out_count = 0;
        return;
    }

    *out_count = 1;
}

// =============================================================================
// Polygon set-operation kernels (single polygon-pair operations)
// =============================================================================

namespace {

constexpr int kMaxPolyVerts = 512;
constexpr int kMaxGhVerts   = 1024;
constexpr int kPolyStatusError            = 0;
constexpr int kPolyStatusPolygon          = 1;
constexpr int kPolyStatusReturnGeom1      = 2;
constexpr int kPolyStatusReturnGeom2      = 3;
constexpr int kPolyStatusCollection       = 4;
constexpr int kPolyStatusEmpty            = 5;
constexpr int kPolyStatusPolygonWithHole  = 6;
constexpr double kPolyEpsilon = 1e-9;

struct GhVert {
    double x;
    double y;
    double alpha;
    bool is_isect;
    bool ent_other;
    bool used;
    int link;
};

__device__ bool approxEq(double a, double b) {
    return fabs(a - b) <= kPolyEpsilon;
}

__device__ int normalizeRing(const double* in_xy, int in_vertices, double* out_xy) {
    if (!in_xy || in_vertices < 3 || in_vertices > kMaxPolyVerts) {
        return 0;
    }
    int n = in_vertices;
    const int last = (n - 1) * 2;
    if (approxEq(in_xy[0], in_xy[last]) && approxEq(in_xy[1], in_xy[last + 1])) {
        --n;
    }
    if (n < 3 || n > kMaxPolyVerts) {
        return 0;
    }
    for (int i = 0; i < n; ++i) {
        out_xy[i * 2]     = in_xy[i * 2];
        out_xy[i * 2 + 1] = in_xy[i * 2 + 1];
    }
    return n;
}

__device__ bool pointInPolygonXY(double px, double py, const double* ring_xy, int n) {
    if (!ring_xy || n < 3) {
        return false;
    }
    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const double xi = ring_xy[i * 2];
        const double yi = ring_xy[i * 2 + 1];
        const double xj = ring_xy[j * 2];
        const double yj = ring_xy[j * 2 + 1];
        const bool intersects = ((yi > py) != (yj > py))
            && (px < (xj - xi) * (py - yi) / ((yj - yi) + 1e-30) + xi);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

__device__ double cross2d(double ox, double oy, double ax, double ay, double bx, double by) {
    return (ax - ox) * (by - oy) - (ay - oy) * (bx - ox);
}

__device__ bool onSegment1D(double a, double b, double d) {
    if (a > b) {
        const double t = a;
        a = b;
        b = t;
    }
    return d >= a - kPolyEpsilon && d <= b + kPolyEpsilon;
}

__device__ bool segmentsIntersect(
    double ax, double ay, double bx, double by,
    double cx, double cy, double dx, double dy) {
    const double d1 = cross2d(cx, cy, dx, dy, ax, ay);
    const double d2 = cross2d(cx, cy, dx, dy, bx, by);
    const double d3 = cross2d(ax, ay, bx, by, cx, cy);
    const double d4 = cross2d(ax, ay, bx, by, dx, dy);
    if (((d1 > 0.0 && d2 < 0.0) || (d1 < 0.0 && d2 > 0.0))
        && ((d3 > 0.0 && d4 < 0.0) || (d3 < 0.0 && d4 > 0.0))) {
        return true;
    }
    const auto collinearOn = [&](double px, double py, double qx, double qy, double rx, double ry) {
        return fabs(cross2d(qx, qy, rx, ry, px, py)) < kPolyEpsilon
            && onSegment1D(qx, rx, px)
            && onSegment1D(qy, ry, py);
    };
    return collinearOn(ax, ay, cx, cy, dx, dy)
        || collinearOn(bx, by, cx, cy, dx, dy)
        || collinearOn(cx, cy, ax, ay, bx, by)
        || collinearOn(dx, dy, ax, ay, bx, by);
}

__device__ bool polygonsIntersect(const double* a_xy, int na, const double* b_xy, int nb) {
    if (!a_xy || !b_xy || na < 3 || nb < 3) {
        return false;
    }
    for (int i = 0, j = na - 1; i < na; j = i++) {
        const double ax1 = a_xy[j * 2];
        const double ay1 = a_xy[j * 2 + 1];
        const double ax2 = a_xy[i * 2];
        const double ay2 = a_xy[i * 2 + 1];
        for (int k = 0, l = nb - 1; k < nb; l = k++) {
            const double bx1 = b_xy[l * 2];
            const double by1 = b_xy[l * 2 + 1];
            const double bx2 = b_xy[k * 2];
            const double by2 = b_xy[k * 2 + 1];
            if (segmentsIntersect(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2)) {
                return true;
            }
        }
    }
    return pointInPolygonXY(a_xy[0], a_xy[1], b_xy, nb)
        || pointInPolygonXY(b_xy[0], b_xy[1], a_xy, na);
}

__device__ bool ghSegIsect(double x1, double y1, double x2, double y2,
                           double x3, double y3, double x4, double y4,
                           double &t, double &s) {
    const double dx = x2 - x1;
    const double dy = y2 - y1;
    const double ex = x4 - x3;
    const double ey = y4 - y3;
    const double D  = dx * ey - dy * ex;
    if (fabs(D) < kPolyEpsilon) {
        return false;
    }
    const double fx = x3 - x1;
    const double fy = y3 - y1;
    t = (fx * ey - fy * ex) / D;
    s = (fx * dy - fy * dx) / D;
    return t > kPolyEpsilon && t < 1.0 - kPolyEpsilon
        && s > kPolyEpsilon && s < 1.0 - kPolyEpsilon;
}

__device__ int ghInitChain(const double* ring_xy, int n, GhVert* out) {
    if (!ring_xy || !out || n < 3 || n > kMaxPolyVerts) {
        return 0;
    }
    for (int i = 0; i < n; ++i) {
        out[i].x = ring_xy[i * 2];
        out[i].y = ring_xy[i * 2 + 1];
        out[i].alpha = static_cast<double>(i);
        out[i].is_isect = false;
        out[i].ent_other = false;
        out[i].used = false;
        out[i].link = -1;
    }
    return n;
}

__device__ void ghSortByAlpha(GhVert* v, int n) {
    for (int i = 1; i < n; ++i) {
        GhVert key = v[i];
        int j = i - 1;
        while (j >= 0 && v[j].alpha > key.alpha) {
            v[j + 1] = v[j];
            --j;
        }
        v[j + 1] = key;
    }
}

__device__ bool ghPhase1(
    GhVert* A, int a_base, int &a_count,
    GhVert* B, int b_base, int &b_count) {
    if (!A || !B) {
        return false;
    }
    int a_insert = a_base;
    int b_insert = b_base;
    for (int i = 0; i < a_base; ++i) {
        const int i2 = (i + 1) % a_base;
        for (int j = 0; j < b_base; ++j) {
            const int j2 = (j + 1) % b_base;
            double t = 0.0, s = 0.0;
            if (!ghSegIsect(A[i].x, A[i].y, A[i2].x, A[i2].y,
                            B[j].x, B[j].y, B[j2].x, B[j2].y, t, s)) {
                continue;
            }
            if (a_insert >= kMaxGhVerts || b_insert >= kMaxGhVerts) {
                return false;
            }
            const double px = A[i].x + t * (A[i2].x - A[i].x);
            const double py = A[i].y + t * (A[i2].y - A[i].y);

            A[a_insert].x = px;
            A[a_insert].y = py;
            A[a_insert].alpha = static_cast<double>(i) + t;
            A[a_insert].is_isect = true;
            A[a_insert].ent_other = false;
            A[a_insert].used = false;
            A[a_insert].link = -1;
            ++a_insert;

            B[b_insert].x = px;
            B[b_insert].y = py;
            B[b_insert].alpha = static_cast<double>(j) + s;
            B[b_insert].is_isect = true;
            B[b_insert].ent_other = false;
            B[b_insert].used = false;
            B[b_insert].link = -1;
            ++b_insert;
        }
    }

    a_count = a_insert;
    b_count = b_insert;
    ghSortByAlpha(A, a_count);
    ghSortByAlpha(B, b_count);

    for (int ia = 0; ia < a_count; ++ia) {
        if (!A[ia].is_isect || A[ia].link >= 0) {
            continue;
        }
        for (int ib = 0; ib < b_count; ++ib) {
            if (!B[ib].is_isect || B[ib].link >= 0) {
                continue;
            }
            if (approxEq(A[ia].x, B[ib].x) && approxEq(A[ia].y, B[ib].y)) {
                A[ia].link = ib;
                B[ib].link = ia;
                break;
            }
        }
    }
    return true;
}

__device__ bool ghHasIntersection(const GhVert* A, int a_count) {
    for (int i = 0; i < a_count; ++i) {
        if (A[i].is_isect) {
            return true;
        }
    }
    return false;
}

__device__ void ghLabelIntersections(GhVert* chain, int count, const double* other_ring, int other_n) {
    bool inside = false;
    for (int i = 0; i < count; ++i) {
        if (!chain[i].is_isect) {
            inside = pointInPolygonXY(chain[i].x, chain[i].y, other_ring, other_n);
            break;
        }
    }
    for (int i = 0; i < count; ++i) {
        if (!chain[i].is_isect) {
            continue;
        }
        chain[i].ent_other = !inside;
        inside = !inside;
    }
}

__device__ int closeRing(double* ring_xy, int count) {
    if (count < 3) {
        return 0;
    }
    if (!approxEq(ring_xy[0], ring_xy[(count - 1) * 2])
        || !approxEq(ring_xy[1], ring_xy[(count - 1) * 2 + 1])) {
        ring_xy[count * 2]     = ring_xy[0];
        ring_xy[count * 2 + 1] = ring_xy[1];
        ++count;
    }
    return count;
}

__device__ int ghTraverseUnion(GhVert* A, int na, GhVert* B, int nb, double* out_xy) {
    int start_a = -1;
    for (int i = 0; i < na; ++i) {
        if (A[i].is_isect && !A[i].used && !A[i].ent_other) {
            start_a = i;
            break;
        }
    }
    if (start_a < 0) {
        return 0;
    }
    bool on_A = true;
    int cur_a = start_a;
    int cur_b = -1;
    int out_n = 0;
    for (int iter = 0; iter < na + nb + 16; ++iter) {
        if (out_n >= kMaxPolyVerts) {
            return 0;
        }
        if (on_A) {
            if (cur_a == start_a && out_n > 1) {
                break;
            }
            GhVert &v = A[cur_a];
            out_xy[out_n * 2]     = v.x;
            out_xy[out_n * 2 + 1] = v.y;
            ++out_n;
            v.used = true;
            if (v.is_isect && v.ent_other && v.link >= 0 && v.link < nb) {
                cur_b = v.link;
                B[cur_b].used = true;
                cur_b = (cur_b + 1) % nb;
                on_A = false;
            } else {
                cur_a = (cur_a + 1) % na;
            }
        } else {
            GhVert &v = B[cur_b];
            if (v.is_isect && v.ent_other && v.link >= 0 && v.link < na) {
                out_xy[out_n * 2]     = v.x;
                out_xy[out_n * 2 + 1] = v.y;
                ++out_n;
                v.used = true;
                cur_a = v.link;
                A[cur_a].used = true;
                if (cur_a == start_a) {
                    break;
                }
                on_A = true;
                cur_a = (cur_a + 1) % na;
            } else {
                out_xy[out_n * 2]     = v.x;
                out_xy[out_n * 2 + 1] = v.y;
                ++out_n;
                v.used = true;
                cur_b = (cur_b + 1) % nb;
            }
        }
    }
    return closeRing(out_xy, out_n);
}

__device__ int ghTraverseDiff(GhVert* A, int na, GhVert* B, int nb, double* out_xy) {
    int start_a = -1;
    for (int i = 0; i < na; ++i) {
        if (A[i].is_isect && !A[i].used && !A[i].ent_other) {
            start_a = i;
            break;
        }
    }
    if (start_a < 0) {
        return 0;
    }
    bool on_A = true;
    int cur_a = start_a;
    int cur_b = -1;
    int out_n = 0;
    for (int iter = 0; iter < na + nb + 16; ++iter) {
        if (out_n >= kMaxPolyVerts) {
            return 0;
        }
        if (on_A) {
            if (cur_a == start_a && out_n > 1) {
                break;
            }
            GhVert &v = A[cur_a];
            out_xy[out_n * 2]     = v.x;
            out_xy[out_n * 2 + 1] = v.y;
            ++out_n;
            v.used = true;
            if (v.is_isect && v.ent_other && v.link >= 0 && v.link < nb) {
                cur_b = v.link;
                B[cur_b].used = true;
                cur_b = ((cur_b - 1) + nb) % nb;
                on_A = false;
            } else {
                cur_a = (cur_a + 1) % na;
            }
        } else {
            GhVert &v = B[cur_b];
            if (v.is_isect && v.ent_other && v.link >= 0 && v.link < na) {
                out_xy[out_n * 2]     = v.x;
                out_xy[out_n * 2 + 1] = v.y;
                ++out_n;
                v.used = true;
                cur_a = v.link;
                A[cur_a].used = true;
                if (cur_a == start_a) {
                    break;
                }
                on_A = true;
                cur_a = (cur_a + 1) % na;
            } else {
                out_xy[out_n * 2]     = v.x;
                out_xy[out_n * 2 + 1] = v.y;
                ++out_n;
                v.used = true;
                cur_b = ((cur_b - 1) + nb) % nb;
            }
        }
    }
    return closeRing(out_xy, out_n);
}

} // anonymous namespace

__global__ void polygonUnionKernel(
    const double* ring1_xy,
    int           ring1_n,
    const double* ring2_xy,
    int           ring2_n,
    double*       out_ring1_xy,
    int*          out_ring1_n,
    double*       out_ring2_xy,
    int*          out_ring2_n,
    int*          out_status
) {
    if (blockIdx.x != 0 || threadIdx.x != 0) {
        return;
    }
    *out_status  = kPolyStatusError;
    *out_ring1_n = 0;
    *out_ring2_n = 0;

    double a_xy[(kMaxPolyVerts + 1) * 2] = {};
    double b_xy[(kMaxPolyVerts + 1) * 2] = {};
    const int a_n = normalizeRing(ring1_xy, ring1_n, a_xy);
    const int b_n = normalizeRing(ring2_xy, ring2_n, b_xy);
    if (a_n < 3 || b_n < 3) {
        return;
    }

    if (!polygonsIntersect(a_xy, a_n, b_xy, b_n)) {
        if (pointInPolygonXY(a_xy[0], a_xy[1], b_xy, b_n)) {
            *out_status = kPolyStatusReturnGeom2;
            return;
        }
        if (pointInPolygonXY(b_xy[0], b_xy[1], a_xy, a_n)) {
            *out_status = kPolyStatusReturnGeom1;
            return;
        }
        *out_status = kPolyStatusCollection;
        return;
    }

    GhVert A[kMaxGhVerts] = {};
    GhVert B[kMaxGhVerts] = {};
    int a_count = ghInitChain(a_xy, a_n, A);
    int b_count = ghInitChain(b_xy, b_n, B);
    if (a_count == 0 || b_count == 0) {
        return;
    }
    if (!ghPhase1(A, a_n, a_count, B, b_n, b_count)) {
        return;
    }
    if (!ghHasIntersection(A, a_count)) {
        if (pointInPolygonXY(a_xy[0], a_xy[1], b_xy, b_n)) {
            *out_status = kPolyStatusReturnGeom2;
            return;
        }
        if (pointInPolygonXY(b_xy[0], b_xy[1], a_xy, a_n)) {
            *out_status = kPolyStatusReturnGeom1;
            return;
        }
        *out_status = kPolyStatusCollection;
        return;
    }

    ghLabelIntersections(A, a_count, b_xy, b_n);
    ghLabelIntersections(B, b_count, a_xy, a_n);
    const int out_n = ghTraverseUnion(A, a_count, B, b_count, out_ring1_xy);
    if (out_n < 4) {
        *out_status = kPolyStatusCollection;
        return;
    }
    *out_ring1_n = out_n;
    *out_status = kPolyStatusPolygon;
}

__global__ void polygonDifferenceKernel(
    const double* ring1_xy,
    int           ring1_n,
    const double* ring2_xy,
    int           ring2_n,
    double*       out_ring1_xy,
    int*          out_ring1_n,
    double*       out_ring2_xy,
    int*          out_ring2_n,
    int*          out_status
) {
    if (blockIdx.x != 0 || threadIdx.x != 0) {
        return;
    }
    *out_status  = kPolyStatusError;
    *out_ring1_n = 0;
    *out_ring2_n = 0;

    double a_xy[(kMaxPolyVerts + 1) * 2] = {};
    double b_xy[(kMaxPolyVerts + 1) * 2] = {};
    const int a_n = normalizeRing(ring1_xy, ring1_n, a_xy);
    const int b_n = normalizeRing(ring2_xy, ring2_n, b_xy);
    if (a_n < 3) {
        return;
    }
    if (b_n < 3) {
        *out_status = kPolyStatusReturnGeom1;
        return;
    }

    if (!polygonsIntersect(a_xy, a_n, b_xy, b_n)) {
        if (pointInPolygonXY(a_xy[0], a_xy[1], b_xy, b_n)) {
            *out_status = kPolyStatusEmpty;
            return;
        }
        if (pointInPolygonXY(b_xy[0], b_xy[1], a_xy, a_n)) {
            for (int i = 0; i < a_n; ++i) {
                out_ring1_xy[i * 2] = a_xy[i * 2];
                out_ring1_xy[i * 2 + 1] = a_xy[i * 2 + 1];
            }
            for (int i = 0; i < b_n; ++i) {
                out_ring2_xy[i * 2] = b_xy[i * 2];
                out_ring2_xy[i * 2 + 1] = b_xy[i * 2 + 1];
            }
            *out_ring1_n = closeRing(out_ring1_xy, a_n);
            *out_ring2_n = closeRing(out_ring2_xy, b_n);
            *out_status = kPolyStatusPolygonWithHole;
            return;
        }
        *out_status = kPolyStatusReturnGeom1;
        return;
    }

    GhVert A[kMaxGhVerts] = {};
    GhVert B[kMaxGhVerts] = {};
    int a_count = ghInitChain(a_xy, a_n, A);
    int b_count = ghInitChain(b_xy, b_n, B);
    if (a_count == 0 || b_count == 0) {
        return;
    }
    if (!ghPhase1(A, a_n, a_count, B, b_n, b_count)) {
        return;
    }
    if (!ghHasIntersection(A, a_count)) {
        if (pointInPolygonXY(a_xy[0], a_xy[1], b_xy, b_n)) {
            *out_status = kPolyStatusEmpty;
            return;
        }
        if (pointInPolygonXY(b_xy[0], b_xy[1], a_xy, a_n)) {
            for (int i = 0; i < a_n; ++i) {
                out_ring1_xy[i * 2] = a_xy[i * 2];
                out_ring1_xy[i * 2 + 1] = a_xy[i * 2 + 1];
            }
            for (int i = 0; i < b_n; ++i) {
                out_ring2_xy[i * 2] = b_xy[i * 2];
                out_ring2_xy[i * 2 + 1] = b_xy[i * 2 + 1];
            }
            *out_ring1_n = closeRing(out_ring1_xy, a_n);
            *out_ring2_n = closeRing(out_ring2_xy, b_n);
            *out_status = kPolyStatusPolygonWithHole;
            return;
        }
        *out_status = kPolyStatusReturnGeom1;
        return;
    }

    ghLabelIntersections(A, a_count, b_xy, b_n);
    ghLabelIntersections(B, b_count, a_xy, a_n);
    const int out_n = ghTraverseDiff(A, a_count, B, b_count, out_ring1_xy);
    if (out_n < 4) {
        *out_status = kPolyStatusReturnGeom1;
        return;
    }
    *out_ring1_n = out_n;
    *out_status = kPolyStatusPolygon;
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
 * Launch the ST_UNION kernel for a single point pair.
 *
 * @param d_points_xy      Device buffer [4]: [x1,y1,x2,y2]
 * @param d_out_points_xy  Device buffer [4]
 * @param d_out_count      Device scalar output count
 * @param opaque_stream    Optional cudaStream_t cast to void*
 * @return 0 on success, non-zero CUDA error code on failure.
 */
int launchGeoPointUnionKernel(
    const double* d_points_xy,
    double*       d_out_points_xy,
    int*          d_out_count,
    void*         opaque_stream
) {
    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);
    pointUnionKernel<<<1, 1, 0, stream>>>(d_points_xy, d_out_points_xy, d_out_count);
    const cudaError_t err = cudaGetLastError();
    return static_cast<int>(err);
}

/**
 * Launch the ST_DIFFERENCE kernel for a single point pair.
 *
 * @param d_points_xy      Device buffer [4]: [x1,y1,x2,y2]
 * @param d_out_points_xy  Device buffer [4]
 * @param d_out_count      Device scalar output count
 * @param opaque_stream    Optional cudaStream_t cast to void*
 * @return 0 on success, non-zero CUDA error code on failure.
 */
int launchGeoPointDifferenceKernel(
    const double* d_points_xy,
    double*       d_out_points_xy,
    int*          d_out_count,
    void*         opaque_stream
) {
    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);
    pointDifferenceKernel<<<1, 1, 0, stream>>>(d_points_xy, d_out_points_xy, d_out_count);
    const cudaError_t err = cudaGetLastError();
    return static_cast<int>(err);
}

/**
 * Launch the polygon ST_UNION kernel for one polygon pair.
 */
int launchGeoPolygonUnionKernel(
    const double* d_ring1_xy,
    int           ring1_vertices,
    const double* d_ring2_xy,
    int           ring2_vertices,
    double*       d_out_ring1_xy,
    int*          d_out_ring1_vertices,
    double*       d_out_ring2_xy,
    int*          d_out_ring2_vertices,
    int*          d_out_status,
    void*         opaque_stream
) {
    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);
    polygonUnionKernel<<<1, 1, 0, stream>>>(
        d_ring1_xy, ring1_vertices, d_ring2_xy, ring2_vertices,
        d_out_ring1_xy, d_out_ring1_vertices, d_out_ring2_xy, d_out_ring2_vertices, d_out_status);
    const cudaError_t err = cudaGetLastError();
    return static_cast<int>(err);
}

/**
 * Launch the polygon ST_DIFFERENCE kernel for one polygon pair.
 */
int launchGeoPolygonDifferenceKernel(
    const double* d_ring1_xy,
    int           ring1_vertices,
    const double* d_ring2_xy,
    int           ring2_vertices,
    double*       d_out_ring1_xy,
    int*          d_out_ring1_vertices,
    double*       d_out_ring2_xy,
    int*          d_out_ring2_vertices,
    int*          d_out_status,
    void*         opaque_stream
) {
    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);
    polygonDifferenceKernel<<<1, 1, 0, stream>>>(
        d_ring1_xy, ring1_vertices, d_ring2_xy, ring2_vertices,
        d_out_ring1_xy, d_out_ring1_vertices, d_out_ring2_xy, d_out_ring2_vertices, d_out_status);
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
