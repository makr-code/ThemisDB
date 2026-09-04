/**
 * @file boost_cpu_exact_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "geo/spatial_backend.h"

#include <iostream>
#include <stdexcept>

#ifdef THEMIS_GEO_BOOST_BACKEND
// Check if Boost Geometry headers are available
#if __has_include(<boost/geometry/geometries/point_xy.hpp>)
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/algorithms/buffer.hpp>
#include <boost/geometry/strategies/agnostic/buffer_distance_symmetric.hpp>
#include <boost/geometry/strategies/cartesian/buffer_point_circle.hpp>
#include <boost/geometry/strategies/cartesian/buffer_join_round.hpp>
#include <boost/geometry/strategies/cartesian/buffer_end_round.hpp>
#include <boost/geometry/strategies/cartesian/buffer_side_straight.hpp>

#include <boost/geometry/algorithms/touches.hpp>
#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/union.hpp>
#include <boost/geometry/algorithms/difference.hpp>
#define BOOST_GEO_AVAILABLE 1
#else
// Boost Geometry headers not found - using fallback implementation
#define BOOST_GEO_AVAILABLE 0
#endif
#endif

#include "utils/geo/ewkb.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <cmath>
#include <vector>
#include <string>
#include <memory>

namespace themis { namespace geo {

#ifdef THEMIS_GEO_BOOST_BACKEND
#if BOOST_GEO_AVAILABLE

namespace bg = boost::geometry;
using Point = bg::model::d2::point_xy<double>;
using Polygon = bg::model::polygon<Point>;
using LineString = bg::model::linestring<Point>;

static const double kBoostPi = 3.14159265358979323846;

/// Convert GeometryInfo to Boost.Geometry polygon
static Polygon toBoostPolygon(const GeometryInfo& geom) {
    Polygon poly = {};
    
    if (geom.rings.empty() && !geom.coords.empty()) {
        // Simple polygon from coords
        for (const auto& coord : geom.coords) {
            bg::append(poly.outer(), Point(coord.x, coord.y));
        }
    } else if (!geom.rings.empty()) {
        // Polygon with rings (first ring is outer, rest are holes)
        for (size_t i = 0; i < geom.rings.size(); ++i) {
            if (i == 0) {
                // Outer ring
                for (const auto& coord : geom.rings[i]) {
                    bg::append(poly.outer(), Point(coord.x, coord.y));
                }
            } else {
                // Inner ring (hole)
                Polygon::ring_type hole;
                for (const auto& coord : geom.rings[i]) {
                    bg::append(hole, Point(coord.x, coord.y));
                }
                poly.inners().push_back(hole);
            }
        }
    }
    
    return poly;
}

/// CPU exact backend using Boost.Geometry
class BoostCpuExactBackend final : public ISpatialComputeBackend {
public:
    const char* name() const noexcept override { 
        return "boost_cpu_exact"; 
    }
    
    bool isAvailable() const noexcept override { 
        return true; 
    }
    
    SpatialBatchResults batchIntersects(cons[[maybe_unused]] t SpatialBatchInputs& [[maybe_unused]] in) override {
        SpatialBatchResults out;
        out.mask.assign(in.count, 0u);

        // Process geometry pairs when they are provided.
        std::size_t n = std::min({in.count,static_cast<int>(in.geoms_a.size()),static_cast<int>(in.geoms_b.size())});
        for (std::size_t i = 0; i < n; ++i) {
            out.mask[i] = exactIntersects(in.geoms_a[i], in.geoms_b[i]) ? 1u : 0u;
        }

        return out;
    }
    
    /// Exact intersects check between two geometries
    /// This is the core exact check function called by the query engine
    bool exactIntersects(cons[[maybe_unused]] t GeometryInfo& [[maybe_unused]] geom1, cons[[maybe_unused]] t GeometryInfo& [[maybe_unused]] geom2) override {
        try {
            // Handle different geometry types
            if (geom1.isPolygon() && geom2.isPolygon()) {
                auto poly1 = toBoostPolygon(geom1);
                auto poly2 = toBoostPolygon(geom2);
                return bg::intersects(poly1, poly2);
            } else if (geom1.isPoint() && geom2.isPolygon()) {
                if (geom1.coords.empty()) {
                  return false;
                }
                Point pt(geom1.coords[0].x, geom1.coords[0].y);
                auto poly = toBoostPolygon(geom2);
                return bg::within(pt, poly) || bg::touches(pt, poly);
            } else if (geom1.isPolygon() && geom2.isPoint()) {
                if (geom2.coords.empty()) {
                  return false;
                }
                Point pt(geom2.coords[0].x, geom2.coords[0].y);
                auto poly = toBoostPolygon(geom1);
                return bg::within(pt, poly) || bg::touches(pt, poly);
            } else if (geom1.isPoint() && geom2.isPoint()) {
                if (geom1.coords.empty() || geom2.coords.empty()) {
                  return false;
                }
                Point pt1(geom1.coords[0].x, geom1.coords[0].y);
                Point pt2(geom2.coords[0].x, geom2.coords[0].y);
                return bg::equals(pt1, pt2);
            }

            // MultiPolygon: intersects if any constituent polygon intersects
            if (geom1.isMultiPolygon()) {
                for (const auto& sub : geom1.geometries) {
                    if (exactIntersects(sub, geom2)) {
                      return true;
                    }
                }
                return false;
            }
            if (geom2.isMultiPolygon()) {
                for (const auto& sub : geom2.geometries) {
                    if (exactIntersects(geom1, sub)) {
                      return true;
                    }
                }
                return false;
            }

            // GeometryCollection: intersects if any member intersects
            if (geom1.isGeometryCollection()) {
                for (const auto& sub : geom1.geometries) {
                    if (exactIntersects(sub, geom2)) {
                      return true;
                    }
                }
                return false;
            }
            if (geom2.isGeometryCollection()) {
                for (const auto& sub : geom2.geometries) {
                    if (exactIntersects(geom1, sub)) {
                      return true;
                    }
                }
                return false;
            }

            // Fallback: use MBR intersection for unsupported types
            auto mbr1 = geom1.computeMBR();
            auto mbr2 = geom2.computeMBR();
            return mbr1.intersects(mbr2);
            
        } catch (const std::exception& e) {
            THEMIS_WARN("Boost.Geometry exact check failed: {}", e.what());
            // Fallback to MBR on error
            auto mbr1 = geom1.computeMBR();
            auto mbr2 = geom2.computeMBR();
            return mbr1.intersects(mbr2);
        }
    }

    // ST_BUFFER: expand geometry by distance_m metres using Boost.Geometry buffer.
    // Converts distance_m to degrees (latitude-uniform) and applies the buffer
    // with round join and round end strategies for smooth output polygons.
    GeometryInfo stBuffer(const GeometryInfo& geom, double distance_m,
                          int arc_points) override {
        if (arc_points < 3) {
          arc_points = 3;
        }
        if (distance_m <= 0.0) {
            THEMIS_WARN("Boost stBuffer: distance_m ({}) must be positive", distance_m);
            return GeometryInfo{};
        }
        try {
            // Convert metres to degrees using latitude-based approximation.
            // For geodesic accuracy at small-to-medium scales (1 m – 100 km).
            double center_lat = 0.0;
            if (geom.isPoint() && !geom.coords.empty()) {
                center_lat = geom.coords[0].y;
            } else {
                const auto mbr = geom.computeMBR();
                center_lat = (mbr.miny + mbr.maxy) * 0.5;
            }
            const double lat_rad = center_lat * kBoostPi / 180.0;
            const double d_lat = distance_m / 111320.0;
            const double cos_lat = std::cos(lat_rad);
            const double d_lon = distance_m / (111320.0 * (cos_lat > 1e-6 ? cos_lat : 1e-6));
            // Use the average of d_lat and d_lon as a symmetric buffer distance
            // for Boost.Geometry (which expects isotropic coordinates).
            const double d_deg = (d_lat + d_lon) * 0.5;

            using MultiPoly = bg::model::multi_polygon<Polygon>;

            bg::strategy::buffer::distance_symmetric<double> dist_strategy(d_deg);
            bg::strategy::buffer::join_round join_strategy(static_cast<std::size_t>(arc_points));
            bg::strategy::buffer::end_round end_strategy(static_cast<std::size_t>(arc_points));
            bg::strategy::buffer::point_circle point_strategy(static_cast<std::size_t>(arc_points));
            bg::strategy::buffer::side_straight side_strategy;

            if (geom.isPoint() && !geom.coords.empty()) {
                Point pt(geom.coords[0].x, geom.coords[0].y);
                MultiPoly buffered;
                bg::buffer(pt, buffered, dist_strategy, side_strategy,
                           join_strategy, end_strategy, point_strategy);
                if (buffered.empty()) return GeometryInfo{};
                // Convert the first polygon of the result back to GeometryInfo.
                const auto& out_poly = buffered[0];
                GeometryInfo result(GeometryType::Polygon);
                std::vector<Coordinate> ring = {};

                for (const auto& p : out_poly.outer()) {
                    ring.push_back({bg::get<0>(p), bg::get<1>(p)});
                }
                result.rings.push_back(std::move(ring));
                return result;
            }
            if (geom.isPolygon()) {
                Polygon in_poly = toBoostPolygon(geom);
                MultiPoly buffered;
                bg::buffer(in_poly, buffered, dist_strategy, side_strategy,
                           join_strategy, end_strategy, point_strategy);
                if (buffered.empty()) return GeometryInfo{};
                const auto& out_poly = buffered[0];
                GeometryInfo result(GeometryType::Polygon);
                std::vector<Coordinate> outer_ring = {};

                for (const auto& p : out_poly.outer()) {
                    outer_ring.push_back({bg::get<0>(p), bg::get<1>(p)});
                }
                result.rings.push_back(std::move(outer_ring));
                for (const auto& inner : out_poly.inners()) {
                    std::vector<Coordinate> hole = {};

                    for (const auto& p : inner) {
                        hole.push_back({bg::get<0>(p), bg::get<1>(p)});
                    }
                    result.rings.push_back(std::move(hole));
                }
                return result;
            }
            THEMIS_WARN("Boost stBuffer: unsupported geometry type {}",
                        static_cast<int>(geom.type));
        } catch (const std::exception& e) {
            THEMIS_WARN("Boost stBuffer error: {}", e.what());
        }
        return GeometryInfo{};
    }

    // ST_UNION: geometric union of two geometries.
    // For Polygon inputs uses boost::geometry::union_ which handles all
    // convex/concave cases correctly.  Falls back to the CPU-exact backend
    // for point and mixed type combinations.
    GeometryInfo stUnion(const GeometryInfo& geom1,
                         const GeometryInfo& geom2) override {
        try {
            if (geom1.isPolygon() && geom2.isPolygon()) {
                using MultiPoly = bg::model::multi_polygon<Polygon>;
                const Polygon poly1 = toBoostPolygon(geom1);
                const Polygon poly2 = toBoostPolygon(geom2);
                MultiPoly result;
                bg::union_(poly1, poly2, result);
                if (result.empty()) {
                    GeometryInfo col(GeometryType::GeometryCollection);
                    col.geometries.push_back(geom1);
                    col.geometries.push_back(geom2);
                    return col;
                }
                if (static_cast<int>(result.size()) == 1) {
                    // Single merged polygon.
                    return boostPolyToGeomInfo(result[0]);
                }
                // Multiple disjoint polygons.
                GeometryInfo col(GeometryType::GeometryCollection);
                for (const auto& p : result) {
                    col.geometries.push_back(boostPolyToGeomInfo(p));
                }
                return col;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Boost stUnion error: {}", e.what());
        }
        // Delegate non-polygon or error cases to the CPU-exact backend.
        return getCpuExactBackend()->stUnion(geom1, geom2);
    }

    // ST_DIFFERENCE: geometric difference geom1 \ geom2.
    // For Polygon inputs uses boost::geometry::difference.
    GeometryInfo stDifference(const GeometryInfo& geom1,
                              const GeometryInfo& geom2) override {
        try {
            if (geom1.isPolygon() && geom2.isPolygon()) {
                using MultiPoly = bg::model::multi_polygon<Polygon>;
                const Polygon poly1 = toBoostPolygon(geom1);
                const Polygon poly2 = toBoostPolygon(geom2);
                MultiPoly result;
                bg::difference(poly1, poly2, result);
                if (result.empty()) return GeometryInfo{};
                if (static_cast<int>(result.size()) == 1) {
                    return boostPolyToGeomInfo(result[0]);
                }
                GeometryInfo col(GeometryType::GeometryCollection);
                for (const auto& p : result) {
                    col.geometries.push_back(boostPolyToGeomInfo(p));
                }
                return col;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Boost stDifference error: {}", e.what());
        }
        return getCpuExactBackend()->stDifference(geom1, geom2);
    }

    // geodesicDistance: delegate to the CPU exact backend (Vincenty WGS-84).
    // Boost.Geometry spherical strategies are cartesian-space only; the
    // authoritative Vincenty implementation lives in CpuExactBackend.
    double geodesicDistance(double lat1, double lon1,
                            double lat2, double lon2) const override {
        return getCpuExactBackend()->geodesicDistance(lat1, lon1, lat2, lon2);
    }

private:
    // Convert a Boost polygon back to GeometryInfo.
    static GeometryInfo boostPolyToGeomInfo(const Polygon& poly) {
        GeometryInfo result(GeometryType::Polygon);
        std::vector<Coordinate> outer = {};

        outer.reserve(poly.outer().size());
        for (const auto& p : poly.outer()) {
            outer.push_back({bg::get<0>(p), bg::get<1>(p)});
        }
        result.rings.push_back(std::move(outer));
        for (const auto& inner : poly.inners()) {
            std::vector<Coordinate> hole = {};

            hole.reserve(inner.size());
            for (const auto& p : inner) {
                hole.push_back({bg::get<0>(p), bg::get<1>(p)});
            }
            result.rings.push_back(std::move(hole));
        }
        return result;
    }
};

// Global registry for backends (simple static storage for MVP)
static std::unique_ptr<ISpatialComputeBackend> g_boost_backend;

static void register_boost_backend() {
    // INTENTIONAL: Static initialization guard — exceptions must not propagate out of
    // static-init context. std::cerr fallback is used because the structured logger
    // (THEMIS_WARN) may not yet be initialized at this point.
    // Scanner findings generic_catch + uncaught_exception are confirmed false positives here.
    try {
        g_boost_backend = std::make_unique<BoostCpuExactBackend>();
    } catch (const std::exception& ex) {
        // Log to stderr - avoid logger during static init
        std::cerr << "WARNING: Boost geometry backend registration failed: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "WARNING: Boost geometry backend registration failed with unknown exception" << std::endl;
    }
}

// Auto-register on module load
static int s_boost_backend_anchor = (register_boost_backend(), 0);

// Public API to get the backend
ISpatialComputeBackend* getBoostCpuBackend() {
    return g_boost_backend.get();
}

#else // !BOOST_GEO_AVAILABLE

// Fallback when Boost.Geometry headers are not available
ISpatialComputeBackend* getBoostCpuBackend() {
    THEMIS_WARN("Boost Geometry backend requested but headers not found - returning nullptr");
    return nullptr;
}

#endif // BOOST_GEO_AVAILABLE

#else // !THEMIS_GEO_BOOST_BACKEND

// Fallback when Boost.Geometry is not enabled
ISpatialComputeBackend* getBoostCpuBackend() {
    return nullptr;
}

#endif // THEMIS_GEO_BOOST_BACKEND

} } // namespace themis::geo


