/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geojson_geometry.cpp                               ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "geo/geojson_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <string>

namespace themis {
namespace geo {

// ── GeoConfig singleton ───────────────────────────────────────────────────

GeoConfig& GeoConfig::instance() {
    static GeoConfig inst;
    return inst;
}

// ── Internal helpers ──────────────────────────────────────────────────────

namespace {

bool isFinite(double v) {
    return std::isfinite(v);
}

ValidationResult validateCoordinate(double lon, double lat) {
    if (!isFinite(lon) || !isFinite(lat)) {
        return ValidationResult::error(GeoValidationCode::NAN_COORDINATE,
            "Non-finite coordinate value (NaN or Inf)");
    }
    if (lon < -180.0 || lon > 180.0) {
        return ValidationResult::error(GeoValidationCode::OUT_OF_RANGE_LON,
            "Longitude " + std::to_string(lon) + " is outside [-180, 180]");
    }
    if (lat < -90.0 || lat > 90.0) {
        return ValidationResult::error(GeoValidationCode::OUT_OF_RANGE_LAT,
            "Latitude " + std::to_string(lat) + " is outside [-90, 90]");
    }
    return ValidationResult::success();
}

// Shoelace formula — positive area = counter-clockwise (right-hand rule for exterior).
double ringSignedArea(const std::vector<Coordinate>& ring) {
    double area = 0.0;
    std::size_t n = ring.size();
    if (n < 3) return 0.0;
    for (std::size_t i = 0; i < n - 1; ++i) {
        area += (ring[i].x * ring[i + 1].y) - (ring[i + 1].x * ring[i].y);
    }
    return area * 0.5;
}

std::string coordToJson(const Coordinate& c) {
    std::ostringstream os;
    os << "[" << c.x << "," << c.y;
    if (c.hasZ()) os << "," << c.getZ();
    os << "]";
    return os.str();
}

std::string ringToJson(const std::vector<Coordinate>& ring) {
    std::ostringstream os;
    os << "[";
    for (std::size_t i = 0; i < ring.size(); ++i) {
        if (i > 0) os << ",";
        os << coordToJson(ring[i]);
    }
    os << "]";
    return os.str();
}

} // anonymous namespace

// ── GeoPoint ─────────────────────────────────────────────────────────────

ValidationResult GeoPoint::validate() const {
    return validateCoordinate(lon_, lat_);
}

std::string GeoPoint::toGeoJSON() const {
    std::ostringstream os;
    os << R"({"type":"Point","coordinates":[)" << lon_ << "," << lat_ << "]}";
    return os.str();
}

// ── GeoLineString ─────────────────────────────────────────────────────────

MBR GeoLineString::bbox() const {
    if (coords_.empty()) return {};
    double minx = coords_[0].x, miny = coords_[0].y;
    double maxx = minx, maxy = miny;
    for (const auto& c : coords_) {
        minx = std::min(minx, c.x);
        miny = std::min(miny, c.y);
        maxx = std::max(maxx, c.x);
        maxy = std::max(maxy, c.y);
    }
    return MBR{minx, miny, maxx, maxy};
}

ValidationResult GeoLineString::validate() const {
    if (coords_.empty()) {
        return ValidationResult::error(GeoValidationCode::EMPTY_GEOMETRY,
            "LineString has no coordinates");
    }
    for (const auto& c : coords_) {
        auto r = validateCoordinate(c.x, c.y);
        if (!r.ok()) return r;
    }
    return ValidationResult::success();
}

std::string GeoLineString::toGeoJSON() const {
    std::ostringstream os;
    os << R"({"type":"LineString","coordinates":[)";
    for (std::size_t i = 0; i < coords_.size(); ++i) {
        if (i > 0) os << ",";
        os << coordToJson(coords_[i]);
    }
    os << "]}";
    return os.str();
}

// ── GeoPolygon ────────────────────────────────────────────────────────────

MBR GeoPolygon::bbox() const {
    if (rings_.empty() || rings_[0].empty()) return {};
    double minx = rings_[0][0].x, miny = rings_[0][0].y;
    double maxx = minx, maxy = miny;
    for (const auto& ring : rings_) {
        for (const auto& c : ring) {
            minx = std::min(minx, c.x);
            miny = std::min(miny, c.y);
            maxx = std::max(maxx, c.x);
            maxy = std::max(maxy, c.y);
        }
    }
    return MBR{minx, miny, maxx, maxy};
}

ValidationResult GeoPolygon::validate() const {
    if (rings_.empty()) {
        return ValidationResult::error(GeoValidationCode::EMPTY_GEOMETRY,
            "Polygon has no rings");
    }
    for (std::size_t ri = 0; ri < rings_.size(); ++ri) {
        const auto& ring = rings_[ri];
        if (ring.size() < 4) {
            return ValidationResult::error(GeoValidationCode::RING_NOT_CLOSED,
                "Polygon ring " + std::to_string(ri) + " has fewer than 4 vertices");
        }
        // Validate all coordinates
        for (const auto& c : ring) {
            auto r = validateCoordinate(c.x, c.y);
            if (!r.ok()) return r;
        }
        // Check closed ring (first == last)
        const auto& first = ring.front();
        const auto& last  = ring.back();
        double tol = GeoConfig::instance().coordinateTolerance();
        if (std::fabs(first.x - last.x) > tol ||
            std::fabs(first.y - last.y) > tol) {
            return ValidationResult::error(GeoValidationCode::RING_NOT_CLOSED,
                "Polygon ring " + std::to_string(ri) + " is not closed");
        }
        // Check winding order: exterior ring (ri==0) must be CCW (positive area).
        double area = ringSignedArea(ring);
        if (ri == 0 && area <= 0.0) {
            return ValidationResult::error(GeoValidationCode::WRONG_WINDING_ORDER,
                "Polygon exterior ring must be counter-clockwise (right-hand rule)");
        }
        if (ri > 0 && area >= 0.0) {
            return ValidationResult::error(GeoValidationCode::WRONG_WINDING_ORDER,
                "Polygon hole ring " + std::to_string(ri) + " must be clockwise");
        }
    }
    return ValidationResult::success();
}

std::string GeoPolygon::toGeoJSON() const {
    std::ostringstream os;
    os << R"({"type":"Polygon","coordinates":[)";
    for (std::size_t i = 0; i < rings_.size(); ++i) {
        if (i > 0) os << ",";
        os << ringToJson(rings_[i]);
    }
    os << "]}";
    return os.str();
}

// ── GeoMultiPolygon ───────────────────────────────────────────────────────

MBR GeoMultiPolygon::bbox() const {
    if (polygons_.empty()) return {};
    MBR result = polygons_[0].bbox();
    for (std::size_t i = 1; i < polygons_.size(); ++i) {
        MBR b = polygons_[i].bbox();
        result.minx = std::min(result.minx, b.minx);
        result.miny = std::min(result.miny, b.miny);
        result.maxx = std::max(result.maxx, b.maxx);
        result.maxy = std::max(result.maxy, b.maxy);
    }
    return result;
}

ValidationResult GeoMultiPolygon::validate() const {
    if (polygons_.empty()) {
        return ValidationResult::error(GeoValidationCode::EMPTY_GEOMETRY,
            "MultiPolygon has no polygons");
    }
    for (std::size_t i = 0; i < polygons_.size(); ++i) {
        auto r = polygons_[i].validate();
        if (!r.ok()) return r;
    }
    return ValidationResult::success();
}

std::string GeoMultiPolygon::toGeoJSON() const {
    std::ostringstream os;
    os << R"({"type":"MultiPolygon","coordinates":[)";
    for (std::size_t pi = 0; pi < polygons_.size(); ++pi) {
        if (pi > 0) os << ",";
        os << "[";
        const auto& rings = polygons_[pi].rings();
        for (std::size_t ri = 0; ri < rings.size(); ++ri) {
            if (ri > 0) os << ",";
            os << ringToJson(rings[ri]);
        }
        os << "]";
    }
    os << "]}";
    return os.str();
}

// ── GeoGeometryCollection ─────────────────────────────────────────────────

MBR GeoGeometryCollection::bbox() const {
    if (geometries_.empty()) return {};
    MBR result{std::numeric_limits<double>::max(),
               std::numeric_limits<double>::max(),
               std::numeric_limits<double>::lowest(),
               std::numeric_limits<double>::lowest()};
    for (const auto& g : geometries_) {
        if (!g) continue;
        MBR b = g->bbox();
        result.minx = std::min(result.minx, b.minx);
        result.miny = std::min(result.miny, b.miny);
        result.maxx = std::max(result.maxx, b.maxx);
        result.maxy = std::max(result.maxy, b.maxy);
    }
    return result;
}

ValidationResult GeoGeometryCollection::validate() const {
    for (const auto& g : geometries_) {
        if (!g) continue;
        auto r = g->validate();
        if (!r.ok()) return r;
    }
    return ValidationResult::success();
}

std::string GeoGeometryCollection::toGeoJSON() const {
    std::ostringstream os;
    os << R"({"type":"GeometryCollection","geometries":[)";
    bool first = true;
    for (const auto& g : geometries_) {
        if (!first) os << ",";
        first = false;
        if (g) os << g->toGeoJSON();
        else   os << "null";
    }
    os << "]}";
    return os.str();
}

} // namespace geo
} // namespace themis
