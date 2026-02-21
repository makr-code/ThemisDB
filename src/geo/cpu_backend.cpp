/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cpu_backend.cpp                                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:38:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     243                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3055833f6  2026-02-20  GPU geospatial backend: replace stub with real intersecti... ║
    • 235d2ca7f  2026-02-10  Refactor tests and update dependencies   ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
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
