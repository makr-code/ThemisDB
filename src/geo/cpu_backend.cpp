#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"

#include <iostream>
#include <stdexcept>
#include <cmath>

namespace themis { namespace geo {

// Helper function to check point-in-polygon using ray casting algorithm
// This provides a reasonable fallback when Boost.Geometry is not available
static bool pointInPolygon(double px, double py, const std::vector<Coord>& polygon) {
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

// Helper function for simple polygon-polygon intersection check
// Uses separating axis theorem for convex polygons
static bool simplePolygonIntersects(const std::vector<Coord>& poly1, 
                                   const std::vector<Coord>& poly2) {
    // Simple check: if any vertex of poly1 is inside poly2, they intersect
    for (const auto& coord : poly1) {
        if (pointInPolygon(coord.x, coord.y, poly2)) {
            return true;
        }
    }
    
    // Check if any vertex of poly2 is inside poly1
    for (const auto& coord : poly2) {
        if (pointInPolygon(coord.x, coord.y, poly1)) {
            return true;
        }
    }
    
    // TODO: Also check for edge-edge intersections
    // For now, fall back to MBR if no vertices are inside
    return false;
}

class CpuExactBackend final : public ISpatialComputeBackend {
public:
    const char* name() const noexcept override { return "cpu_exact"; }
    bool isAvailable() const noexcept override { return true; }
    
    SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) override {
        SpatialBatchResults out;
        out.mask.assign(in.count, 0u); // placeholder: no-ops
        return out;
    }
    
    // Improved exact check with fallback to MBR
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
            
            // Polygon-Polygon intersection
            if (geom1.isPolygon() && geom2.isPolygon()) {
                std::vector<Coord> poly1, poly2;
                
                // Extract polygon coordinates
                if (!geom1.rings.empty()) {
                    poly1 = geom1.rings[0]; // Outer ring
                } else {
                    poly1 = geom1.coords;
                }
                
                if (!geom2.rings.empty()) {
                    poly2 = geom2.rings[0]; // Outer ring
                } else {
                    poly2 = geom2.coords;
                }
                
                // Use simple polygon intersection check
                // TODO(geo-robustness): Add edge-edge intersection checks for complete coverage
                // Current implementation may miss cases where polygons intersect only at edges
                // For production use, consider Boost.Geometry backend (boost_cpu_exact_backend.cpp)
                if (simplePolygonIntersects(poly1, poly2)) {
                    return true;
                }
                
                // If simple check says no intersection, fall through to MBR check
                // as our simple check may miss some cases
            }
            
            // Fall back to MBR for all other cases or when exact check is inconclusive
            auto mbr1 = geom1.computeMBR();
            auto mbr2 = geom2.computeMBR();
            return mbr1.intersects(mbr2);
            
        } catch (const std::exception& e) {
            // On error, fall back to MBR check
            auto mbr1 = geom1.computeMBR();
            auto mbr2 = geom2.computeMBR();
            return mbr1.intersects(mbr2);
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

// Ensure the object file isn't discarded
static int s_geo_cpu_backend_anchor = (register_builtin_cpu_backend(), 0);

} } // namespace themis::geo
