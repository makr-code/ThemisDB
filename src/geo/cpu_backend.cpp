/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cpu_backend.cpp                                    ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     390                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 25e932e7f  2026-02-22  feat(geo): implement ST_Buffer operation (Point + Polygon... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
#include "utils/logger.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace themis { namespace geo {

// ---------------------------------------------------------------------------
// Internal geometry helpers
// ---------------------------------------------------------------------------

static const double kCpuEpsilon = 1e-9;
static const double kCpuPi = 3.14159265358979323846;

// Helper function to check point-in-polygon using ray casting algorithm
// This provides a reasonable fallback when Boost.Geometry is not available
static bool pointInPolygon(double px, double py, const std::vector<Coordinate>& polygon) {
    if (polygon.size() < 3) return false;
    
    bool inside = false;
    size_t n = polygon.size();
    
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        double xi = polygon[i].x, yi = polygon[i].y;
        double xj = polygon[j].x, yj = polygon[j].y;
        
        bool intersect = ((yi > py) != (yj > py)) &&
                        (px < (xj - xi) * (py - yi) / (yj - yi) + xi);
        if (intersect) inside = !inside;
    }
    
    return inside;
}

/// Cross product of vectors OA and OB.
static double cpuCross(double ox, double oy,
                       double ax, double ay,
                       double bx, double by) {
    return (ax - ox) * (by - oy) - (ay - oy) * (bx - ox);
}

/// True if value d is in [min(a,b), max(a,b)] (with epsilon).
static bool cpuOnSegment1D(double a, double b, double d) {
    if (a > b) std::swap(a, b);
    return d >= a - kCpuEpsilon && d <= b + kCpuEpsilon;
}

/// True if segments AB and CD intersect (including collinear / endpoint cases).
/// The primary cross-product check identifies proper crossings (d1,d2 opposite
/// signs).  The four collinearOn calls cover the endpoint-on-segment cases
/// (d = 0 for one or both endpoints), matching the GPU backend's algorithm.
static bool cpuSegmentsIntersect(double ax, double ay, double bx, double by,
                                  double cx, double cy, double dx, double dy) {
    double d1 = cpuCross(cx, cy, dx, dy, ax, ay);
    double d2 = cpuCross(cx, cy, dx, dy, bx, by);
    double d3 = cpuCross(ax, ay, bx, by, cx, cy);
    double d4 = cpuCross(ax, ay, bx, by, dx, dy);

    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
        return true;
    }
    auto collinearOn = [&](double px, double py,
                            double qx, double qy, double rx, double ry) {
        return std::abs(cpuCross(qx, qy, rx, ry, px, py)) < kCpuEpsilon &&
               cpuOnSegment1D(qx, rx, px) && cpuOnSegment1D(qy, ry, py);
    };
    return collinearOn(ax, ay, cx, cy, dx, dy) ||
           collinearOn(bx, by, cx, cy, dx, dy) ||
           collinearOn(cx, cy, ax, ay, bx, by) ||
           collinearOn(dx, dy, ax, ay, bx, by);
}

// Full polygon-polygon intersection: edge-edge crossing + containment.
// Complexity: O(n1 * n2) edge comparisons — adequate for the CPU fallback
// path where n is typically small (< 1000 vertices).  Callers with very
// large polygons should use the Boost.Geometry backend instead.
static bool polygonIntersects(const std::vector<Coordinate>& poly1,
                               const std::vector<Coordinate>& poly2) {
    if (poly1.empty() || poly2.empty()) return false;

    // Edge-edge crossing check
    std::size_t n1 = poly1.size(), n2 = poly2.size();
    for (std::size_t i = 0, j = n1 - 1; i < n1; j = i++) {
        for (std::size_t k = 0, l = n2 - 1; k < n2; l = k++) {
            if (cpuSegmentsIntersect(poly1[j].x, poly1[j].y,
                                     poly1[i].x, poly1[i].y,
                                     poly2[l].x, poly2[l].y,
                                     poly2[k].x, poly2[k].y)) {
                return true;
            }
        }
    }
    // Containment: one polygon wholly inside the other
    if (pointInPolygon(poly1[0].x, poly1[0].y, poly2)) return true;
    if (pointInPolygon(poly2[0].x, poly2[0].y, poly1)) return true;
    return false;
}

// ---------------------------------------------------------------------------
// ST_BUFFER helpers
// ---------------------------------------------------------------------------

/// Build a closed polygon ring approximating a circle around (cx, cy).
/// d_lat/d_lon are the semi-axis lengths in degrees (latitude / longitude).
static std::vector<Coordinate> cpuCircleRing(double cx, double cy,
                                              double d_lat, double d_lon,
                                              int arc_points) {
    std::vector<Coordinate> ring;
    ring.reserve(static_cast<std::size_t>(arc_points) + 1);
    for (int i = 0; i < arc_points; ++i) {
        const double angle = 2.0 * kCpuPi * i / arc_points;
        ring.push_back({cx + d_lon * std::cos(angle),
                        cy + d_lat * std::sin(angle)});
    }
    ring.push_back(ring[0]); // close the ring
    return ring;
}

/// Compute the signed area of a ring (Shoelace formula).
/// Positive → CCW, Negative → CW.
static double cpuSignedArea(const std::vector<Coordinate>& ring,
                             std::size_t n_verts) {
    double area = 0.0;
    for (std::size_t i = 0, j = n_verts - 1; i < n_verts; j = i++) {
        area += ring[j].x * ring[i].y - ring[i].x * ring[j].y;
    }
    return area; // 2× actual area; sign is what matters
}

/// Expand a polygon ring outward by (d_lon, d_lat) degrees using the
/// edge-shift + intersection method.  Works correctly for convex rings;
/// produces a reasonable approximation for mildly concave rings.
static std::vector<Coordinate> cpuExpandRing(const std::vector<Coordinate>& ring,
                                              double d_lat, double d_lon) {
    // Determine the number of unique vertices (open ring).
    std::size_t n = ring.size();
    if (n < 2) return ring;
    std::size_t n_verts = n;
    if (n > 1 &&
        std::abs(ring[0].x - ring[n - 1].x) < kCpuEpsilon &&
        std::abs(ring[0].y - ring[n - 1].y) < kCpuEpsilon) {
        n_verts = n - 1; // closed ring: ignore duplicate closing vertex
    }
    if (n_verts < 3) return ring;

    // Determine orientation: CCW (area > 0) → outward normal is right of edge.
    // CW (area < 0) → outward normal is left of edge.
    const double signed_area = cpuSignedArea(ring, n_verts);
    const double orient = (signed_area >= 0.0) ? 1.0 : -1.0;

    // Build shifted edges: each edge is translated outward by distance.
    // The outward unit normal of edge (A→B) for a CCW ring is (dy, -dx)/len;
    // for CW the sign flips, captured by orient.
    struct ShiftedEdge { Coordinate p1, p2; };
    std::vector<ShiftedEdge> shifted(n_verts);
    for (std::size_t i = 0; i < n_verts; ++i) {
        std::size_t j = (i + 1) % n_verts;
        const double dx = ring[j].x - ring[i].x;
        const double dy = ring[j].y - ring[i].y;
        const double len = std::sqrt(dx * dx + dy * dy);
        if (len < kCpuEpsilon) {
            shifted[i] = {ring[i], ring[j]};
            continue;
        }
        // Outward normal components (scaled to geographic degrees).
        const double ox = orient * dy / len * d_lon;
        const double oy = orient * (-dx) / len * d_lat;
        shifted[i] = {{ring[i].x + ox, ring[i].y + oy},
                      {ring[j].x + ox, ring[j].y + oy}};
    }

    // For each vertex, compute the intersection of the two flanking shifted edges.
    std::vector<Coordinate> result;
    result.reserve(n_verts + 1);
    for (std::size_t i = 0; i < n_verts; ++i) {
        const std::size_t prev = (i + n_verts - 1) % n_verts;
        const auto& e1 = shifted[prev];
        const auto& e2 = shifted[i];
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
            const double t = (px * d2y - py * d2x) / denom;
            result.push_back({e1.p1.x + t * d1x, e1.p1.y + t * d1y});
        }
    }
    result.push_back(result[0]); // close the ring
    return result;
}

class CpuExactBackend final : public ISpatialComputeBackend {
public:
    const char* name() const noexcept override { return "cpu_exact"; }
    bool isAvailable() const noexcept override { return true; }
    
    SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) override {
        SpatialBatchResults out;
        out.mask.assign(in.count, 0u);
        const bool have_geoms = !in.geoms_a.empty() || !in.geoms_b.empty();
        if (have_geoms &&
            (in.geoms_a.size() != in.count || in.geoms_b.size() != in.count)) {
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
    // Supports: Point×Point, Point×Polygon (symmetric), Polygon×Polygon.
    // Uses ray-casting (point-in-polygon) and segment-intersection
    // to handle all cases including edge-only polygon crossings.
    bool exactIntersects(const GeometryInfo& geom1, const GeometryInfo& geom2) override {
        try {
            // Point-Point intersection
            if (geom1.isPoint() && geom2.isPoint()) {
                if (geom1.coords.empty() || geom2.coords.empty()) return false;
                const auto& p1 = geom1.coords[0];
                const auto& p2 = geom2.coords[0];
                // Use epsilon for floating-point comparison
                const double epsilon = 1e-9;
                return std::abs(p1.x - p2.x) < epsilon && 
                       std::abs(p1.y - p2.y) < epsilon;
            }
            
            // Point-Polygon intersection
            if (geom1.isPoint() && geom2.isPolygon()) {
                if (geom1.coords.empty()) return false;
                const auto& pt = geom1.coords[0];
                
                // Check against outer ring
                if (!geom2.rings.empty()) {
                    return pointInPolygon(pt.x, pt.y, geom2.rings[0]);
                } else if (!geom2.coords.empty()) {
                    return pointInPolygon(pt.x, pt.y, geom2.coords);
                }
                return false;
            }
            
            // Polygon-Point intersection (symmetric)
            if (geom1.isPolygon() && geom2.isPoint()) {
                if (geom2.coords.empty()) return false;
                const auto& pt = geom2.coords[0];
                
                // Check against outer ring
                if (!geom1.rings.empty()) {
                    return pointInPolygon(pt.x, pt.y, geom1.rings[0]);
                } else if (!geom1.coords.empty()) {
                    return pointInPolygon(pt.x, pt.y, geom1.coords);
                }
                return false;
            }
            
            // Polygon-Polygon intersection: edge-edge + containment
            if (geom1.isPolygon() && geom2.isPolygon()) {
                const std::vector<Coordinate>& poly1 =
                    !geom1.rings.empty() ? geom1.rings[0] : geom1.coords;
                const std::vector<Coordinate>& poly2 =
                    !geom2.rings.empty() ? geom2.rings[0] : geom2.coords;
                return polygonIntersects(poly1, poly2);
            }
            
            // For all other geometry-type combinations or inconclusive cases,
            // we conservatively report no intersection rather than using an
            // approximate MBR-based fallback, to avoid false positives.
            return false;
            
        } catch (const std::exception& e) {
            // On error, conservatively report no intersection instead of
            // falling back to an approximate MBR-based check.
            THEMIS_WARN("CPU exact backend error: {}", e.what());
            return false;
        }
    }

    // ST_BUFFER: expand a geometry by distance_m metres.
    // Supported types: Point → closed polygon ring, Polygon → outward expansion.
    // arc_points controls the vertex count used for curved approximations.
    GeometryInfo stBuffer(const GeometryInfo& geom, double distance_m,
                          int arc_points) override {
        if (arc_points < 3) arc_points = 3;
        if (distance_m <= 0.0) {
            THEMIS_WARN("CPU stBuffer: distance_m ({}) must be positive", distance_m);
            return GeometryInfo{};
        }
        try {
            if (geom.isPoint()) {
                if (geom.coords.empty()) return GeometryInfo{};
                const Coordinate& c = geom.coords[0];
                const double lat_rad = c.y * kCpuPi / 180.0;
                const double d_lat = distance_m / 111320.0;
                const double cos_lat = std::cos(lat_rad);
                const double d_lon = distance_m / (111320.0 * (cos_lat > 1e-6 ? cos_lat : 1e-6));
                GeometryInfo result(GeometryType::Polygon);
                result.rings.push_back(cpuCircleRing(c.x, c.y, d_lat, d_lon, arc_points));
                return result;
            }
            if (geom.isPolygon()) {
                const std::vector<Coordinate>& ring_in =
                    geom.rings.empty() ? geom.coords : geom.rings[0];
                if (ring_in.size() < 3) return GeometryInfo{};
                const auto mbr = geom.computeMBR();
                const double center_lat = (mbr.miny + mbr.maxy) * 0.5;
                const double lat_rad = center_lat * kCpuPi / 180.0;
                const double d_lat = distance_m / 111320.0;
                const double cos_lat = std::cos(lat_rad);
                const double d_lon = distance_m / (111320.0 * (cos_lat > 1e-6 ? cos_lat : 1e-6));
                GeometryInfo result(GeometryType::Polygon);
                result.rings.push_back(cpuExpandRing(ring_in, d_lat, d_lon));
                return result;
            }
            THEMIS_WARN("CPU stBuffer: unsupported geometry type {}",
                        static_cast<int>(geom.type));
        } catch (const std::exception& e) {
            THEMIS_WARN("CPU stBuffer error: {}", e.what());
        }
        return GeometryInfo{};
    }
};

// Simple internal registry stub (no global linkage yet)
struct NullRegistry : public IGeoRegistry {
    void registerBackend(std::unique_ptr<ISpatialComputeBackend>) override {}
};

static void register_builtin_cpu_backend() {
#ifdef THEMIS_GEO_ENABLED
    try {
        NullRegistry reg;
        reg.registerBackend(std::make_unique<CpuExactBackend>());
    } catch (const std::exception& ex) {
        std::cerr << "WARNING: CPU geometry backend registration failed: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "WARNING: CPU geometry backend registration failed with unknown exception" << std::endl;
    }
#endif
}

// Public factory: returns the built-in CPU exact backend singleton.
static CpuExactBackend& getCpuExactBackendInstance() {
    static CpuExactBackend instance;
    return instance;
}

ISpatialComputeBackend* getCpuExactBackend() {
    return &getCpuExactBackendInstance();
}

// Ensure the object file isn't discarded
static int s_geo_cpu_backend_anchor = (register_builtin_cpu_backend(), 0);

} } // namespace themis::geo
