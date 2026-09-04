/**
 * @file cpu_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=21, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <algorithm>
#include <cmath>
#include <iostream>
#include <mutex>
#include <stdexcept>

#include "geo/geo_math.h"
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
#include "utils/logger.h"

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Internal geometry helpers
// ---------------------------------------------------------------------------

static const double kCpuEpsilon = 1e-9;
static const double kCpuPi      = 3.14159265358979323846;

// STUB/SIMULATION NOTE:
// Purpose: Provide a pure-C++ point-in-polygon fallback when Boost.Geometry
//   is not available.  Uses a ray-casting algorithm that handles simple,
//   non-self-intersecting polygons correctly.
// Activation: Always active (no Boost.Geometry dependency is compiled in by
//   default; no build flag gate).
// Production Delta: No support for geodesic / spherical projection — all
//   coordinates are treated as planar (Cartesian).  Does not handle
//   self-intersecting polygons, holes (inner rings), or polygons with > ~1 000
//   vertices efficiently.  For production workloads requiring full OGC
//   compliance or geodesic correctness, integrate Boost.Geometry and route
//   through its `covered_by` / `intersects` predicates.
// Removal Plan: Add Boost.Geometry as a CMake dependency and compile a
//   `BoostGeometryBackend` that delegates to `boost::geometry::intersects`.
//   Update BackendRegistry priority to prefer Boost over the CPU fallback.
// Roadmap ref: src/geo/FUTURE_ENHANCEMENTS.md §"Boost.Geometry Integration"

// Helper function to check point-in-polygon using ray casting algorithm
// This provides a reasonable fallback when Boost.Geometry is not available
static bool pointInPolygon(double px, double py, const std::vector<Coordinate> &polygon) {
    if (polygon.size() < 3) {
        return false;
    }

    bool inside = false;
    size_t n    = polygon.size();

    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        double xi = polygon[i].x, yi = polygon[i].y;
        double xj = polygon[j].x, yj = polygon[j].y;

        bool intersect = ((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi);
        if (intersect) {
            inside = !inside;
        }
    }

    return inside;
}

/// Cross product of vectors OA and OB.
static double cpuCross(double ox, double oy, double ax, double ay, double bx, double by) {
    return (ax - ox) * (by - oy) - (ay - oy) * (bx - ox);
}

/// True if value d is in [min(a,b), max(a,b)] (with epsilon).
static bool cpuOnSegment1D(double a, double b, double d) {
    if (a > b) {
        std::swap(a, b);
    }
    return d >= a - kCpuEpsilon && d <= b + kCpuEpsilon;
}

/// True if segments AB and CD intersect (including collinear / endpoint cases).
/// The primary cross-product check identifies proper crossings (d1,d2 opposite
/// signs).  The four collinearOn calls cover the endpoint-on-segment cases
/// (d = 0 for one or both endpoints), matching the GPU backend's algorithm.
static bool cpuSegmentsIntersect(double ax, double ay, double bx, double by, double cx, double cy, double dx,
                                 double dy) {
    double d1 = cpuCross(cx, cy, dx, dy, ax, ay);
    double d2 = cpuCross(cx, cy, dx, dy, bx, by);
    double d3 = cpuCross(ax, ay, bx, by, cx, cy);
    double d4 = cpuCross(ax, ay, bx, by, dx, dy);

    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
        return true;
    }
    auto collinearOn = [&](double px, double py, double qx, double qy, double rx, double ry) {
        return std::abs(cpuCross(qx, qy, rx, ry, px, py)) < kCpuEpsilon && cpuOnSegment1D(qx, rx, px)
               && cpuOnSegment1D(qy, ry, py);
    };
    return collinearOn(ax, ay, cx, cy, dx, dy) || collinearOn(bx, by, cx, cy, dx, dy)
           || collinearOn(cx, cy, ax, ay, bx, by) || collinearOn(dx, dy, ax, ay, bx, by);
}

// Full polygon-polygon intersection: edge-edge crossing + containment.
// Complexity: O(n1 * n2) edge comparisons — adequate for the CPU fallback
// path where n is typically small (< 1000 vertices).  Callers with very
// large polygons should use the Boost.Geometry backend instead.
// When pip is non-null, it replaces the built-in ray-casting containment check.
static bool polygonIntersects(const std::vector<Coordinate> &poly1, const std::vector<Coordinate> &poly2,
                              const GeoContainmentFn &pip = {}) {
    if (poly1.empty() || poly2.empty()) {
        return false;
    }

    // Edge-edge crossing check
    std::size_t n1 = poly1.size(), n2 = poly2.size();
    for (std::size_t i = 0, j = n1 - 1; i < n1; j = i++) {
        for (std::size_t k = 0, l = n2 - 1; k < n2; l = k++) {
            if (cpuSegmentsIntersect(poly1[j].x, poly1[j].y, poly1[i].x, poly1[i].y, poly2[l].x, poly2[l].y, poly2[k].x,
                                     poly2[k].y)) {
                return true;
            }
        }
    }
    // Containment: one polygon wholly inside the other.
    // Delegate to injected fn when available, otherwise use built-in ray-casting.
    if (pip) {
        if (pip(poly1[0].x, poly1[0].y, poly2)) {
            return true;
        }
        if (pip(poly2[0].x, poly2[0].y, poly1)) {
            return true;
        }
    } else {
        if (pointInPolygon(poly1[0].x, poly1[0].y, poly2)) {
            return true;
        }
        if (pointInPolygon(poly2[0].x, poly2[0].y, poly1)) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// ST_BUFFER helpers
// ---------------------------------------------------------------------------

/// Build a closed polygon ring approximating a circle around (cx, cy).
/// d_lat/d_lon are the semi-axis lengths in degrees (latitude / longitude).
static std::vector<Coordinate> cpuCircleRing(double cx, double cy, double d_lat, double d_lon, int arc_points) {
    std::vector<Coordinate> ring;
    ring.reserve(static_cast<std::size_t>(arc_points) + 1);
    for (int i = 0; i < arc_points; ++i) {
        const double angle = 2.0 * kCpuPi * i / arc_points;
        ring.push_back({cx + d_lon * std::cos(angle), cy + d_lat * std::sin(angle)});
    }
    ring.push_back(ring[0]); // close the ring
    return ring;
}

/// Compute the signed area of a ring (Shoelace formula).
/// Positive → CCW, Negative → CW.
static double cpuSignedArea(const std::vector<Coordinate> &ring, std::size_t n_verts) {
    double area = 0.0;
    for (std::size_t i = 0, j = n_verts - 1; i < n_verts; j = i++) {
        area += ring[j].x * ring[i].y - ring[i].x * ring[j].y;
    }
    return area; // 2× actual area; sign is what matters
}

/// Expand a polygon ring outward by (d_lon, d_lat) degrees using the
/// edge-shift + intersection method.  Works correctly for convex rings;
/// produces a reasonable approximation for mildly concave rings.
static std::vector<Coordinate> cpuExpandRing(const std::vector<Coordinate> &ring, double d_lat, double d_lon) {
    // Determine the number of unique vertices (open ring).
    std::size_t n = ring.size();
    if (n < 2) {
        return ring;
    }
    std::size_t n_verts = n;
    if (n > 1 && std::abs(ring[0].x - ring[n - 1].x) < kCpuEpsilon
        && std::abs(ring[0].y - ring[n - 1].y) < kCpuEpsilon) {
        n_verts = n - 1; // closed ring: ignore duplicate closing vertex
    }
    if (n_verts < 3) {
        return ring;
    }

    // Determine orientation: CCW (area > 0) → outward normal is right of edge.
    // CW (area < 0) → outward normal is left of edge.
    const double signed_area = cpuSignedArea(ring, n_verts);
    const double orient      = (signed_area >= 0.0) ? 1.0 : -1.0;

    // Build shifted edges: each edge is translated outward by distance.
    // The outward unit normal of edge (A→B) for a CCW ring is (dy, -dx)/len;
    // for CW the sign flips, captured by orient.
    struct ShiftedEdge {
        Coordinate p1, p2;
    };
    std::vector<ShiftedEdge> shifted(n_verts);
    for (std::size_t i = 0; i < n_verts; ++i) {
        std::size_t j    = (i + 1) % n_verts;
        const double dx  = ring[j].x - ring[i].x;
        const double dy  = ring[j].y - ring[i].y;
        const double len = std::sqrt(dx * dx + dy * dy);
        if (len < kCpuEpsilon) {
            shifted[i] = {ring[i], ring[j]};
            continue;
        }
        // Outward normal components (scaled to geographic degrees).
        const double ox = orient * dy / len * d_lon;
        const double oy = orient * (-dx) / len * d_lat;
        shifted[i]      = {{ring[i].x + ox, ring[i].y + oy}, {ring[j].x + ox, ring[j].y + oy}};
    }

    // For each vertex, compute the intersection of the two flanking shifted edges.
    std::vector<Coordinate> result;
    result.reserve(n_verts + 1);
    for (std::size_t i = 0; i < n_verts; ++i) {
        const std::size_t prev = (i + n_verts - 1) % n_verts;
        const auto &e1         = shifted[prev];
        const auto &e2         = shifted[i];
        // Direction vectors of the two shifted edges.
        const double d1x = e1.p2.x - e1.p1.x;
        const double d1y = e1.p2.y - e1.p1.y;
        const double d2x = e2.p2.x - e2.p1.x;
        const double d2y = e2.p2.y - e2.p1.y;
        // Cross product d1 × d2 (denominator).
        const double denom = d1x * d2y - d1y * d2x;
        if (std::abs(denom) < kCpuEpsilon) {
            // Parallel edges: use the start of the current shifted edge.
            result.push_back(e2.p1);
        } else {
            // t = ((e2.p1 - e1.p1) × d2) / (d1 × d2)
            const double px = e2.p1.x - e1.p1.x;
            const double py = e2.p1.y - e1.p1.y;
            const double t  = (px * d2y - py * d2x) / denom;
            result.push_back({e1.p1.x + t * d1x, e1.p1.y + t * d1y});
        }
    }
    result.push_back(result[0]); // close the ring
    return result;
}

/** @brief Cpu exact backend implementation. */
class CpuExactBackend final : public ISpatialComputeBackend {
  public:
    const char *name() const noexcept override {
        return "cpu_exact";
    }
    bool isAvailable() const noexcept override {
        return true;
    }

    /**
     * @brief Inject a custom point-in-polygon function.
     *
     * When set, exactIntersects() calls this fn for every point-in-polygon
     * test instead of the built-in ray-casting algorithm.  Pass nullptr to
     * restore the built-in fallback.
     */
    void setContainmentFn(GeoContainmentFn fn) {
        std::lock_guard<std::mutex> lk(containment_fn_mtx_);
        containment_fn_ = std::move(fn);
    }

    SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
        SpatialBatchResults out;
        out.mask.assign(in.count, 0u);
        const bool have_geoms = !in.geoms_a.empty() || !in.geoms_b.empty();
        if (have_geoms && (in.geoms_a.size() != in.count || in.geoms_b.size() != in.count)) {
            THEMIS_WARN("CPU exact batchIntersects: geometry vector sizes ({},{}) "
                        "do not match count ({})",
                        in.geoms_a.size(), in.geoms_b.size(), in.count);
        }
        std::size_t n = std::min({in.count, in.geoms_a.size(), in.geoms_b.size()});
        for (std::size_t i = 0; i < n; ++i) {
            out.mask[i] = exactIntersects(in.geoms_a[i], in.geoms_b[i]) ? 1u : 0u;
        }
        return out;
    }

    // Exact geometry intersection check.
    // Supports: Point×Point, Point×Polygon (symmetric), Polygon×Polygon,
    // MultiPolygon (decomposed into constituent polygons),
    // GeometryCollection (decomposed into member geometries).
    // Uses ray-casting (point-in-polygon) and segment-intersection
    // to handle all cases including edge-only polygon crossings.
    bool exactIntersects(const GeometryInfo &geom1, const GeometryInfo &geom2) override {
        // Snapshot the injected containment fn once to avoid locking inside the loop.
        GeoContainmentFn pip;
        {
            std::lock_guard<std::mutex> lk(containment_fn_mtx_);
            pip = containment_fn_;
        }

        // Helper: point-in-polygon, dispatches to injected fn or ray-casting.
        auto containmentCheck = [&pip](double px, double py, const std::vector<Coordinate> &ring) -> bool {
            return pip ? pip(px, py, ring) : pointInPolygon(px, py, ring);
        };

        try {
            // Point-Point intersection
            if (geom1.isPoint() && geom2.isPoint()) {
                if (geom1.coords.empty() || geom2.coords.empty()) {
                    return false;
                }
                const auto &p1 = geom1.coords[0];
                const auto &p2 = geom2.coords[0];
                // Use epsilon for floating-point comparison
                const double epsilon = 1e-9;
                return std::abs(p1.x - p2.x) < epsilon && std::abs(p1.y - p2.y) < epsilon;
            }

            // Point-Polygon intersection
            if (geom1.isPoint() && geom2.isPolygon()) {
                if (geom1.coords.empty()) {
                    return false;
                }
                const auto &pt = geom1.coords[0];

                // Check against outer ring
                if (!geom2.rings.empty()) {
                    return containmentCheck(pt.x, pt.y, geom2.rings[0]);
                } else if (!geom2.coords.empty()) {
                    return containmentCheck(pt.x, pt.y, geom2.coords);
                }
                return false;
            }

            // Polygon-Point intersection (symmetric)
            if (geom1.isPolygon() && geom2.isPoint()) {
                if (geom2.coords.empty()) {
                    return false;
                }
                const auto &pt = geom2.coords[0];

                // Check against outer ring
                if (!geom1.rings.empty()) {
                    return containmentCheck(pt.x, pt.y, geom1.rings[0]);
                } else if (!geom1.coords.empty()) {
                    return containmentCheck(pt.x, pt.y, geom1.coords);
                }
                return false;
            }

            // Polygon-Polygon intersection: edge-edge + containment
            if (geom1.isPolygon() && geom2.isPolygon()) {
                const std::vector<Coordinate> &poly1 = !geom1.rings.empty() ? geom1.rings[0] : geom1.coords;
                const std::vector<Coordinate> &poly2 = !geom2.rings.empty() ? geom2.rings[0] : geom2.coords;
                return polygonIntersects(poly1, poly2, pip);
            }

            // MultiPolygon: intersects if any constituent polygon intersects
            if (geom1.isMultiPolygon()) {
                for (const auto &sub : geom1.geometries) {
                    if (exactIntersects(sub, geom2)) {
                        return true;
                    }
                }
                return false;
            }
            if (geom2.isMultiPolygon()) {
                for (const auto &sub : geom2.geometries) {
                    if (exactIntersects(geom1, sub)) {
                        return true;
                    }
                }
                return false;
            }

            // GeometryCollection: intersects if any member intersects
            if (geom1.isGeometryCollection()) {
                for (const auto &sub : geom1.geometries) {
                    if (exactIntersects(sub, geom2)) {
                        return true;
                    }
                }
                return false;
            }
            if (geom2.isGeometryCollection()) {
                for (const auto &sub : geom2.geometries) {
                    if (exactIntersects(geom1, sub)) {
                        return true;
                    }
                }
                return false;
            }

            // For all other geometry-type combinations or inconclusive cases,
            // we conservatively report no intersection rather than using an
            // approximate MBR-based fallback, to avoid false positives.
            return false;

        } catch (const std::exception &e) {
            // On error, conservatively report no intersection instead of
            // falling back to an approximate MBR-based check.
            THEMIS_WARN("CPU exact backend error: {}", e.what());
            return false;
        }
    }

    // Geodesic distance on the WGS-84 ellipsoid (Vincenty inverse formula).
    // Accurate to sub-millimetre for all non-degenerate point pairs.
    // Returns the distance in metres. Returns 0.0 for coincident points.
    // Returns -1.0 for nearly-antipodal points where the iterative formula
    // does not converge within the maximum iteration count.
    double geodesicDistance(double lat1, double lon1, double lat2, double lon2) const override {
        // WGS-84 ellipsoid parameters
        static constexpr double a     = 6378137.0;           // semi-major axis (m)
        static constexpr double f     = 1.0 / 298.257223563; // flattening
        static constexpr double b     = a * (1.0 - f);       // semi-minor axis (m)
        static constexpr int kMaxIter = 200;
        static constexpr double kTol  = 1e-12;

        const double phi1 = lat1 * kCpuPi / 180.0;
        const double phi2 = lat2 * kCpuPi / 180.0;
        const double L    = (lon2 - lon1) * kCpuPi / 180.0;

        const double U1    = std::atan((1.0 - f) * std::tan(phi1));
        const double U2    = std::atan((1.0 - f) * std::tan(phi2));
        const double sinU1 = std::sin(U1), cosU1 = std::cos(U1);
        const double sinU2 = std::sin(U2), cosU2 = std::cos(U2);

        // Degenerate case: one or both endpoints at a geographic pole.
        // At the poles cosU ≈ 0, which causes sinSigma = 0 in the first
        // Vincenty iteration and would incorrectly return distance = 0.
        if (cosU1 < kTol && cosU2 < kTol) {
            if (sinU1 * sinU2 > 0.0) {
                return 0.0; // same pole
            }
            // Opposite poles: half the WGS-84 meridional circumference.
            return 20003931.459;
        }

        double lambda   = L;
        double sinSigma = 0.0, cosSigma = 0.0, sigma = 0.0;
        double sinAlpha = 0.0, cos2Alpha = 0.0, cos2SigmaM = 0.0;
        bool converged = false;

        for (int iter = 0; iter < kMaxIter; ++iter) {
            const double sinLambda = std::sin(lambda);
            const double cosLambda = std::cos(lambda);

            const double a1 = cosU2 * sinLambda;
            const double a2 = cosU1 * sinU2 - sinU1 * cosU2 * cosLambda;
            sinSigma        = std::sqrt(a1 * a1 + a2 * a2);

            if (sinSigma < kTol) {
                return 0.0; // coincident points
            }

            cosSigma   = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
            sigma      = std::atan2(sinSigma, cosSigma);
            sinAlpha   = cosU1 * cosU2 * sinLambda / sinSigma;
            cos2Alpha  = 1.0 - sinAlpha * sinAlpha;
            cos2SigmaM = (cos2Alpha > kTol) ? cosSigma - 2.0 * sinU1 * sinU2 / cos2Alpha : 0.0; // equatorial line

            const double C           = f / 16.0 * cos2Alpha * (4.0 + f * (4.0 - 3.0 * cos2Alpha));
            const double lambda_prev = lambda;
            lambda
                = L
                  + (1.0 - C) * f * sinAlpha
                        * (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)));

            if (std::abs(lambda - lambda_prev) <= kTol) {
                converged = true;
                break;
            }
        }

        if (!converged) {
            // Nearly-antipodal case: Vincenty did not converge within kMaxIter
            // iterations.  Fall back to Haversine which always converges.
            // Haversine accuracy (±0.5 %) is sufficient for the antipodal edge case.
            return haversineDistanceM(lon1, lat1, lon2, lat2);
        }

        const double u2     = cos2Alpha * (a * a - b * b) / (b * b);
        const double kA     = 1.0 + u2 / 16384.0 * (4096.0 + u2 * (-768.0 + u2 * (320.0 - 175.0 * u2)));
        const double kB     = u2 / 1024.0 * (256.0 + u2 * (-128.0 + u2 * (74.0 - 47.0 * u2)));
        const double dSigma = kB * sinSigma
                              * (cos2SigmaM
                                 + kB / 4.0
                                       * (cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)
                                          - kB / 6.0 * cos2SigmaM * (-3.0 + 4.0 * sinSigma * sinSigma)
                                                * (-3.0 + 4.0 * cos2SigmaM * cos2SigmaM)));
        return b * kA * (sigma - dSigma);
    }

    // ST_BUFFER: expand a geometry by distance_m metres.
    // Supported types: Point → closed polygon ring, Polygon → outward expansion.
    // arc_points controls the vertex count used for curved approximations.
    GeometryInfo stBuffer(const GeometryInfo &geom, double distance_m, int arc_points) override {
        if (arc_points < 3)
            arc_points = 3;
        if (distance_m <= 0.0) {
            THEMIS_WARN("CPU stBuffer: distance_m ({}) must be positive", distance_m);
            return GeometryInfo{};
        }
        try {
            if (geom.isPoint()) {
                if (geom.coords.empty()) {
                    return GeometryInfo{};
                }
                const Coordinate &c  = geom.coords[0];
                const double lat_rad = c.y * kCpuPi / 180.0;
                const double d_lat   = distance_m / 111320.0;
                const double cos_lat = std::cos(lat_rad);
                const double d_lon   = distance_m / (111320.0 * (cos_lat > 1e-6 ? cos_lat : 1e-6));
                GeometryInfo result(GeometryType::Polygon);
                result.rings.push_back(cpuCircleRing(c.x, c.y, d_lat, d_lon, arc_points));
                return result;
            }
            if (geom.isPolygon()) {
                const std::vector<Coordinate> &ring_in = geom.rings.empty() ? geom.coords : geom.rings[0];
                if (ring_in.size() < 3) {
                    return GeometryInfo{};
                }
                const auto mbr          = geom.computeMBR();
                const double center_lat = (mbr.miny + mbr.maxy) * 0.5;
                const double lat_rad    = center_lat * kCpuPi / 180.0;
                const double d_lat      = distance_m / 111320.0;
                const double cos_lat    = std::cos(lat_rad);
                const double d_lon      = distance_m / (111320.0 * (cos_lat > 1e-6 ? cos_lat : 1e-6));
                GeometryInfo result(GeometryType::Polygon);
                result.rings.push_back(cpuExpandRing(ring_in, d_lat, d_lon));
                return result;
            }
            THEMIS_WARN("CPU stBuffer: unsupported geometry type {}", static_cast<int>(geom.type));
        } catch (const std::exception &e) {
            THEMIS_WARN("CPU stBuffer error: {}", e.what());
        }
        return GeometryInfo{};
    }

    // ST_UNION / ST_DIFFERENCE implementation
    GeometryInfo stUnion(const GeometryInfo &geom1, const GeometryInfo &geom2) override {
        try {
            if (geom1.isPolygon() && geom2.isPolygon()) {
                return cpuPolyUnion(geom1, geom2);
            }
            if (geom1.isPoint() && geom2.isPoint()) {
                if (geom1.coords.empty()) {
                    return geom2;
                }
                if (geom2.coords.empty()) {
                    return geom1;
                }
                const auto &p1 = geom1.coords[0];
                const auto &p2 = geom2.coords[0];
                if (std::abs(p1.x - p2.x) < kCpuEpsilon && std::abs(p1.y - p2.y) < kCpuEpsilon) {
                    return geom1;
                }
                GeometryInfo col(GeometryType::GeometryCollection);
                col.geometries.push_back(geom1);
                col.geometries.push_back(geom2);
                return col;
            }
            if (geom1.isPoint() && geom2.isPolygon()) {
                if (geom1.coords.empty()) {
                    return geom2;
                }
                const auto &ring = geom2.rings.empty() ? geom2.coords : geom2.rings[0];
                if (pointInPolygon(geom1.coords[0].x, geom1.coords[0].y, ring)) {
                    return geom2;
                }
                GeometryInfo col(GeometryType::GeometryCollection);
                col.geometries.push_back(geom1);
                col.geometries.push_back(geom2);
                return col;
            }
            if (geom1.isPolygon() && geom2.isPoint()) {
                return stUnion(geom2, geom1);
            }
            THEMIS_WARN("CPU stUnion: unsupported geometry type combination");
        } catch (const std::exception &e) {
            THEMIS_WARN("CPU stUnion error: {}", e.what());
        }
        return GeometryInfo{};
    }

    GeometryInfo stDifference(const GeometryInfo &geom1, const GeometryInfo &geom2) override {
        try {
            if (geom1.isPolygon() && geom2.isPolygon()) {
                return cpuPolyDiff(geom1, geom2);
            }
            if (geom1.isPoint() && geom2.isPoint()) {
                if (geom1.coords.empty()) {
                    return GeometryInfo{};
                }
                if (geom2.coords.empty()) {
                    return geom1;
                }
                const auto &p1 = geom1.coords[0];
                const auto &p2 = geom2.coords[0];
                if (std::abs(p1.x - p2.x) < kCpuEpsilon && std::abs(p1.y - p2.y) < kCpuEpsilon) {
                    return GeometryInfo{};
                }
                return geom1;
            }
            if (geom1.isPoint() && geom2.isPolygon()) {
                if (geom1.coords.empty()) {
                    return GeometryInfo{};
                }
                const auto &ring = geom2.rings.empty() ? geom2.coords : geom2.rings[0];
                if (pointInPolygon(geom1.coords[0].x, geom1.coords[0].y, ring)) {
                    return GeometryInfo{};
                }
                return geom1;
            }
            if (geom1.isPolygon() && geom2.isPoint()) {
                // Subtracting a point from a polygon has no effect on area
                return geom1;
            }
            THEMIS_WARN("CPU stDifference: unsupported geometry type combination");
        } catch (const std::exception &e) {
            THEMIS_WARN("CPU stDifference error: {}", e.what());
        }
        return GeometryInfo{};
    }

  private:
    // -----------------------------------------------------------------------
    // Greiner-Hormann polygon boolean operations
    // Reference: Greiner & Hormann, "Efficient Clipping of Arbitrary Polygons",
    //            ACM TOG 1998.
    // Supports Union and Difference for simple (non-self-intersecting) polygons.
    // -----------------------------------------------------------------------

    struct GHVert {
        double x{0}, y{0};
        double alpha{0}; // parametric position along the chain; original vertices
                         // store their integer index; intersection vertices store
                         // edge_index + t where t ∈ (0,1)
        bool is_isect{false};
        bool ent_B{false}; // (A vertex) entering B when traversing A forward
        bool ent_A{false}; // (B vertex) entering A when traversing B forward
        bool used{false};
        int link{-1};
    };

    // Build an open chain from a ring (drops the closing duplicate vertex).
    static std::vector<GHVert> ghChain(const std::vector<Coordinate> &ring) {
        std::size_t n = ring.size();
        while (n > 1 && std::abs(ring[n - 1].x - ring[0].x) < kCpuEpsilon
               && std::abs(ring[n - 1].y - ring[0].y) < kCpuEpsilon) {
            --n;
        }
        std::vector<GHVert> v(n);
        for (std::size_t i = 0; i < n; ++i) {
            v[i].x     = ring[i].x;
            v[i].y     = ring[i].y;
            v[i].alpha = static_cast<double>(i);
        }
        return v;
    }

    // Strict interior segment intersection: t,s ∈ (eps, 1-eps).
    static bool ghSegIsect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4,
                           double &t, double &s) {
        const double dx = x2 - x1, dy = y2 - y1;
        const double ex = x4 - x3, ey = y4 - y3;
        const double D = dx * ey - dy * ex;
        if (std::abs(D) < kCpuEpsilon) {
            return false;
        }
        const double fx = x3 - x1, fy = y3 - y1;
        t = (fx * ey - fy * ex) / D;
        s = (fx * dy - fy * dx) / D;
        return t > kCpuEpsilon && t < 1.0 - kCpuEpsilon && s > kCpuEpsilon && s < 1.0 - kCpuEpsilon;
    }

    // Phase 1: find all edge-edge intersections; insert and sort; cross-link.
    static void ghPhase1(std::vector<GHVert> &A, std::vector<GHVert> &B) {
        const std::size_t na = A.size(), nb = B.size();
        struct IP {
            double aa, ab, x, y;
        };
        std::vector<IP> ips;
        // Reserve a conservative hint: at most min(na, nb) intersections is
        // typical for non-pathological polygons; avoids repeated reallocation
        // in the inner push_back loop.
        ips.reserve(std::min(na, nb));
        for (std::size_t i = 0; i < na; ++i) {
            const std::size_t i2 = (i + 1) % na;
            for (std::size_t j = 0; j < nb; ++j) {
                const std::size_t j2 = (j + 1) % nb;
                double t, s;
                if (ghSegIsect(A[i].x, A[i].y, A[i2].x, A[i2].y, B[j].x, B[j].y, B[j2].x, B[j2].y, t, s)) {
                    ips.push_back({static_cast<double>(i) + t, static_cast<double>(j) + s,
                                   A[i].x + t * (A[i2].x - A[i].x), A[i].y + t * (A[i2].y - A[i].y)});
                }
            }
        }
        if (ips.empty()) {
            return;
        }
        // Reserve capacity for intersection vertices before inserting them
        // to avoid repeated reallocations in the push_back loop.
        A.reserve(A.size() + ips.size());
        for (const auto &ip : ips) {
            GHVert v;
            v.x        = ip.x;
            v.y        = ip.y;
            v.alpha    = ip.aa;
            v.is_isect = true;
            A.push_back(v);
        }
        std::stable_sort(A.begin(), A.end(), [](const GHVert &a, const GHVert &b) { return a.alpha < b.alpha; });
        B.reserve(B.size() + ips.size());
        for (const auto &ip : ips) {
            GHVert v;
            v.x        = ip.x;
            v.y        = ip.y;
            v.alpha    = ip.ab;
            v.is_isect = true;
            B.push_back(v);
        }
        std::stable_sort(B.begin(), B.end(), [](const GHVert &a, const GHVert &b) { return a.alpha < b.alpha; });
        // Cross-link by matching position.
        for (std::size_t ia = 0; ia < A.size(); ++ia) {
            if (!A[ia].is_isect || A[ia].link >= 0) {
                continue;
            }
            for (std::size_t ib = 0; ib < B.size(); ++ib) {
                if (!B[ib].is_isect || B[ib].link >= 0) {
                    continue;
                }
                if (std::abs(A[ia].x - B[ib].x) < kCpuEpsilon && std::abs(A[ia].y - B[ib].y) < kCpuEpsilon) {
                    A[ia].link = static_cast<int>(ib);
                    B[ib].link = static_cast<int>(ia);
                    break;
                }
            }
        }
    }

    // Phase 2A: label each A intersection as entering (ent_B) or exiting B.
    static void ghPhase2A(std::vector<GHVert> &A, const std::vector<Coordinate> &b_ring) {
        bool inside = false;
        for (const auto &v : A) {
            if (!v.is_isect) {
                inside = pointInPolygon(v.x, v.y, b_ring);
                break;
            }
        }
        for (auto &v : A) {
            if (!v.is_isect) {
                continue;
            }
            v.ent_B = !inside;
            inside  = !inside;
        }
    }

    // Phase 2B: label each B intersection as entering (ent_A) or exiting A.
    static void ghPhase2B(std::vector<GHVert> &B, const std::vector<Coordinate> &a_ring) {
        bool inside = false;
        for (const auto &v : B) {
            if (!v.is_isect) {
                inside = pointInPolygon(v.x, v.y, a_ring);
                break;
            }
        }
        for (auto &v : B) {
            if (!v.is_isect) {
                continue;
            }
            v.ent_A = !inside;
            inside  = !inside;
        }
    }

    // Collect one union polygon ring starting from an unvisited A-exiting intersection.
    // Returns an empty vector if no suitable start vertex is found.
    static std::vector<Coordinate> ghTraverseUnion(std::vector<GHVert> &A, std::vector<GHVert> &B) {
        const int na = static_cast<int>(A.size());
        const int nb = static_cast<int>(B.size());
        int start_a  = -1;
        for (int i = 0; i < na; ++i) {
            if (A[i].is_isect && !A[i].used && !A[i].ent_B) {
                start_a = i;
                break;
            }
        }
        if (start_a < 0) {
            return {};
        }

        std::vector<Coordinate> ring;
        // Reserve capacity for at most na + nb vertices plus the closing
        // duplicate, to avoid repeated reallocations during traversal.
        ring.reserve(static_cast<std::size_t>(na + nb) + 1);
        bool on_A = true;
        int cur_a = start_a, cur_b = -1;

        for (int iter = 0; iter < na + nb + 8; ++iter) {
            if (on_A) {
                if (cur_a == start_a && ring.size() > 1) {
                    break;
                }
                GHVert &v = A[cur_a];
                ring.push_back({v.x, v.y});
                v.used = true;
                if (v.is_isect && v.ent_B) {
                    cur_b         = v.link;
                    B[cur_b].used = true;
                    cur_b         = (cur_b + 1) % nb;
                    on_A          = false;
                } else {
                    cur_a = (cur_a + 1) % na;
                }
            } else {
                GHVert &v = B[cur_b];
                if (v.is_isect && v.ent_A) {
                    ring.push_back({v.x, v.y});
                    v.used        = true;
                    cur_a         = v.link;
                    A[cur_a].used = true;
                    if (cur_a == start_a) {
                        break;
                    }
                    on_A  = true;
                    cur_a = (cur_a + 1) % na;
                } else {
                    ring.push_back({v.x, v.y});
                    v.used = true;
                    cur_b  = (cur_b + 1) % nb;
                }
            }
        }
        // Close the ring if not already closed.
        if (ring.size() > 2
            && (std::abs(ring.back().x - ring.front().x) > kCpuEpsilon
                || std::abs(ring.back().y - ring.front().y) > kCpuEpsilon)) {
            ring.push_back(ring[0]);
        }
        return ring;
    }

    // Collect one difference polygon ring starting from an unvisited A-exiting
    // intersection.  Returns an empty vector if none is found.
    static std::vector<Coordinate> ghTraverseDiff(std::vector<GHVert> &A, std::vector<GHVert> &B) {
        const int na = static_cast<int>(A.size());
        const int nb = static_cast<int>(B.size());
        int start_a  = -1;
        for (int i = 0; i < na; ++i) {
            if (A[i].is_isect && !A[i].used && !A[i].ent_B) {
                start_a = i;
                break;
            }
        }
        if (start_a < 0) {
            return {};
        }

        std::vector<Coordinate> ring;
        // Reserve capacity for at most na + nb vertices plus the closing
        // duplicate, to avoid repeated reallocations during traversal.
        ring.reserve(static_cast<std::size_t>(na + nb) + 1);
        bool on_A = true;
        int cur_a = start_a, cur_b = -1;

        for (int iter = 0; iter < na + nb + 8; ++iter) {
            if (on_A) {
                if (cur_a == start_a && ring.size() > 1) {
                    break;
                }
                GHVert &v = A[cur_a];
                ring.push_back({v.x, v.y});
                v.used = true;
                if (v.is_isect && v.ent_B) {
                    // Switch to B traversed backward.
                    cur_b         = v.link;
                    B[cur_b].used = true;
                    cur_b         = ((cur_b - 1) + nb) % nb;
                    on_A          = false;
                } else {
                    cur_a = (cur_a + 1) % na;
                }
            } else {
                GHVert &v = B[cur_b];
                if (v.is_isect && v.ent_A) {
                    // B enters A going forward → switch back to A.
                    ring.push_back({v.x, v.y});
                    v.used        = true;
                    cur_a         = v.link;
                    A[cur_a].used = true;
                    if (cur_a == start_a) {
                        break;
                    }
                    on_A  = true;
                    cur_a = (cur_a + 1) % na;
                } else {
                    ring.push_back({v.x, v.y});
                    v.used = true;
                    cur_b  = ((cur_b - 1) + nb) % nb;
                }
            }
        }
        if (ring.size() > 2
            && (std::abs(ring.back().x - ring.front().x) > kCpuEpsilon
                || std::abs(ring.back().y - ring.front().y) > kCpuEpsilon)) {
            ring.push_back(ring[0]);
        }
        return ring;
    }

    // Extract the outer ring from a GeometryInfo polygon.
    static const std::vector<Coordinate> &outerRing(const GeometryInfo &g) {
        return g.rings.empty() ? g.coords : g.rings[0];
    }

    // Polygon-Polygon union using Greiner-Hormann.
    static GeometryInfo cpuPolyUnion(const GeometryInfo &geom1, const GeometryInfo &geom2) {
        const auto &ring1 = outerRing(geom1);
        const auto &ring2 = outerRing(geom2);
        if (ring1.size() < 3 || ring2.size() < 3) {
            return GeometryInfo{};
        }

        // Fast-path: no overlap.
        if (!polygonIntersects(ring1, ring2)) {
            // Check containment.
            if (pointInPolygon(ring1[0].x, ring1[0].y, ring2)) {
                return geom2; // A ⊆ B
            }
            if (pointInPolygon(ring2[0].x, ring2[0].y, ring1)) {
                return geom1; // B ⊆ A
            }
            // Truly disjoint.
            GeometryInfo col(GeometryType::GeometryCollection);
            col.geometries.push_back(geom1);
            col.geometries.push_back(geom2);
            return col;
        }

        auto A = ghChain(ring1);
        auto B = ghChain(ring2);
        ghPhase1(A, B);

        // Check if intersections were actually found (handles edge cases
        // where polygonIntersects returned true but only at endpoints/edges).
        bool has_isect = false;
        for (const auto &v : A) {
            if (v.is_isect) {
                has_isect = true;
                break;
            }
        }
        if (!has_isect) {
            // Containment fallback.
            if (!ring1.empty() && pointInPolygon(ring1[0].x, ring1[0].y, ring2)) {
                return geom2;
            }
            if (!ring2.empty() && pointInPolygon(ring2[0].x, ring2[0].y, ring1)) {
                return geom1;
            }
            GeometryInfo col(GeometryType::GeometryCollection);
            col.geometries.push_back(geom1);
            col.geometries.push_back(geom2);
            return col;
        }

        ghPhase2A(A, ring2);
        ghPhase2B(B, ring1);

        GeometryInfo result(GeometryType::Polygon);
        for (;;) {
            auto r = ghTraverseUnion(A, B);
            if (r.size() < 4) {
                break;
            }
            result.rings.push_back(std::move(r));
        }
        if (result.rings.empty()) {
            // Fallback: return the bounding union approximation.
            GeometryInfo col(GeometryType::GeometryCollection);
            col.geometries.push_back(geom1);
            col.geometries.push_back(geom2);
            return col;
        }
        return result;
    }

    // Polygon-Polygon difference (geom1 \ geom2) using Greiner-Hormann.
    static GeometryInfo cpuPolyDiff(const GeometryInfo &geom1, const GeometryInfo &geom2) {
        const auto &ring1 = outerRing(geom1);
        const auto &ring2 = outerRing(geom2);
        if (ring1.size() < 3) {
            return GeometryInfo{};
        }
        if (ring2.size() < 3) {
            return geom1;
        }

        // Fast-path: no overlap.
        if (!polygonIntersects(ring1, ring2)) {
            if (pointInPolygon(ring1[0].x, ring1[0].y, ring2)) {
                return GeometryInfo{}; // A ⊆ B
            }
            // B ⊆ A: return A with B as a hole (subtracts B's area from A).
            if (!ring2.empty() && pointInPolygon(ring2[0].x, ring2[0].y, ring1)) {
                GeometryInfo result(GeometryType::Polygon);
                // Outer ring: geom1's outer ring (already closed).
                result.rings.push_back(ring1);
                // Hole: geom2's outer ring (already closed).
                result.rings.push_back(ring2);
                return result;
            }
            return geom1; // Disjoint.
        }

        // Check if A is fully inside B.
        auto A = ghChain(ring1);
        auto B = ghChain(ring2);
        ghPhase1(A, B);

        bool has_isect = false;
        for (const auto &v : A) {
            if (v.is_isect) {
                has_isect = true;
                break;
            }
        }
        if (!has_isect) {
            if (!ring1.empty() && pointInPolygon(ring1[0].x, ring1[0].y, ring2)) {
                return GeometryInfo{}; // A ⊆ B → empty difference
            }
            // B ⊆ A (edges touch but don't cross): return A with B as hole.
            if (!ring2.empty() && pointInPolygon(ring2[0].x, ring2[0].y, ring1)) {
                GeometryInfo result(GeometryType::Polygon);
                result.rings.push_back(ring1);
                result.rings.push_back(ring2);
                return result;
            }
            return geom1; // Disjoint (edge-touching only)
        }

        ghPhase2A(A, ring2);
        ghPhase2B(B, ring1);

        GeometryInfo result(GeometryType::Polygon);
        for (;;) {
            auto r = ghTraverseDiff(A, B);
            if (r.size() < 4) {
                break;
            }
            result.rings.push_back(std::move(r));
        }
        if (result.rings.empty()) {
            return geom1; // Fallback
        }
        return result;
    }

  private:
    mutable std::mutex containment_fn_mtx_;
    GeoContainmentFn containment_fn_; ///< Injected point-in-polygon; null → ray-casting fallback
};

// ---------------------------------------------------------------------------
// GeoBackendRegistry — thread-safe global registry for spatial compute backends.
// Replaces the NullRegistry stub; backends self-register at startup so they
// are discoverable at runtime via getGeoBackendRegistry().
// ---------------------------------------------------------------------------
/** @brief are discoverable at runtime via getGeoBackendRegistry(). */
class GeoBackendRegistry final : public IGeoRegistry {
  public:
    void registerBackend(std::unique_ptr<ISpatialComputeBackend> b) override {
        std::lock_guard<std::mutex> lk(mtx_);
        backends_.push_back(std::move(b));
    }

  private:
    mutable std::mutex mtx_;
    std::vector<std::unique_ptr<ISpatialComputeBackend>> backends_;
};

static GeoBackendRegistry &getGeoRegistryInstance() {
    static GeoBackendRegistry instance;
    return instance;
}

IGeoRegistry *getGeoBackendRegistry() {
    return &getGeoRegistryInstance();
}

// ---------------------------------------------------------------------------
// Approximate CPU backend
// Uses MBR (bounding-box) overlap as a conservative spatial check.
// Never returns a false negative (if geometries truly intersect, their MBRs
// overlap). May return false positives (MBRs overlap but geometries do not).
// This makes it suitable as a fast pre-filter for spatial queries.
// ---------------------------------------------------------------------------

// Forward declaration — defined after CpuExactBackend below.
static CpuExactBackend &getCpuExactBackendInstance();

/** @brief Approximate cpu backend implementation. */
class ApproximateCpuBackend final : public ISpatialComputeBackend {
  public:
    const char *name() const noexcept override {
        return "cpu_approximate";
    }
    bool isAvailable() const noexcept override {
        return true;
    }

    SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
        SpatialBatchResults out;
        out.mask.assign(in.count, 0u);
        std::size_t n = std::min({in.count, in.geoms_a.size(), in.geoms_b.size()});
        for (std::size_t i = 0; i < n; ++i) {
            out.mask[i] = exactIntersects(in.geoms_a[i], in.geoms_b[i]) ? 1u : 0u;
        }
        return out;
    }

    // Approximate intersection check using MBR overlap.
    // Guaranteed no false negatives; may have false positives.
    bool exactIntersects(const GeometryInfo &geom1, const GeometryInfo &geom2) override {
        const auto mbr1 = geom1.computeMBR();
        const auto mbr2 = geom2.computeMBR();
        return mbr1.intersects(mbr2);
    }

    // stBuffer delegates to the exact backend; buffering correctness matters
    // regardless of the caller's chosen precision mode.
    GeometryInfo stBuffer(const GeometryInfo &geom, double distance_m, int arc_points) override {
        return getCpuExactBackendInstance().stBuffer(geom, distance_m, arc_points);
    }
};

static void register_builtin_cpu_backend() {
#ifdef THEMIS_GEO_ENABLED
    try {
        getGeoRegistryInstance().registerBackend(std::make_unique<CpuExactBackend>());
    } catch (const std::exception &ex) {
        std::cerr << "WARNING: CPU geometry backend registration failed: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "WARNING: CPU geometry backend registration failed with unknown exception" << std::endl;
    }
#endif
}

// Public factory: returns the built-in CPU exact backend singleton.
static CpuExactBackend &getCpuExactBackendInstance() {
    static CpuExactBackend instance;
    return instance;
}

ISpatialComputeBackend *getCpuExactBackend() {
    return &getCpuExactBackendInstance();
}

void setCpuExactContainmentFn(GeoContainmentFn fn) {
    getCpuExactBackendInstance().setContainmentFn(std::move(fn));
}

// Public factory: returns the built-in CPU approximate backend singleton.
static ApproximateCpuBackend &getCpuApproximateBackendInstance() {
    static ApproximateCpuBackend instance;
    return instance;
}

ISpatialComputeBackend *getCpuApproximateBackend() {
    return &getCpuApproximateBackendInstance();
}

ISpatialComputeBackend *getBackendForPrecision(GeoPrecisionMode mode) {
    switch (mode) {
        case GeoPrecisionMode::Approximate:
            return getCpuApproximateBackend();
        case GeoPrecisionMode::Exact:
        [[fallthrough]];\n        default:
            return getCpuExactBackend();
    }
}

// Ensure the object file isn't discarded
static int s_geo_cpu_backend_anchor = (register_builtin_cpu_backend(), 0);

} // namespace geo
} // namespace themis


