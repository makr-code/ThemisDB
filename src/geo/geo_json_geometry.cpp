/**
 * @file geo_json_geometry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=16, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "geo/geo_json_geometry.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

double ringSignedArea(const std::vector<Coordinate> &ring) noexcept {
    const std::size_t n = ring.size();
    if (n < 3) {
        return 0.0;
    }
    double area = 0.0;
    // Standard cross-product shoelace: positive result → CCW (right-hand rule).
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        area += ring[j].x * ring[i].y - ring[i].x * ring[j].y;
    }
    return area * 0.5;
}

bool ringIsCCW(const std::vector<Coordinate> &ring) noexcept {
    return ringSignedArea(ring) >= 0.0;
}

bool isValidWGS84Coordinate(const Coordinate &c) noexcept {
    return std::isfinite(c.x) && std::isfinite(c.y) && c.x >= -180.0 && c.x <= 180.0 && c.y >= -90.0 && c.y <= 90.0;
}

// ---------------------------------------------------------------------------
// Internal JSON helpers
// ---------------------------------------------------------------------------

namespace {

std::string coordToJson(const Coordinate &c) {
    std::ostringstream os;
    os << "[" << c.x << "," << c.y << "]";
    return os.str();
}

std::string coordsToJson(const std::vector<Coordinate> &coords) {
    // Use ostringstream to avoid O(n²) string reallocations from repeated
    // concatenation inside the coordinate loop.
    std::ostringstream os;
    os << "[";
    for (std::size_t i = 0; i < coords.size(); ++i) {
        if (i > 0) {
            os << ",";
        }
        os << "[" << coords[i].x << "," << coords[i].y << "]";
    }
    os << "]";
    return os.str();
}

ValidationResult validateCoordinate(const Coordinate &c, CrsId crs, const std::string &ctx) {
    ValidationResult vr;
    if (!std::isfinite(c.x) || !std::isfinite(c.y)) {
        vr.addError({"NON_FINITE_COORDINATE", ctx + ": coordinate contains NaN or infinity"});
    } else if (crs == CrsId::WGS84) {
        if (c.x < -180.0 || c.x > 180.0) {
            vr.addError({"LONGITUDE_OUT_OF_RANGE", ctx + ": longitude " + std::to_string(c.x) + " outside [-180,180]"});
        }
        if (c.y < -90.0 || c.y > 90.0) {
            vr.addError({"LATITUDE_OUT_OF_RANGE", ctx + ": latitude " + std::to_string(c.y) + " outside [-90,90]"});
        }
    }
    return vr;
}

BBox bboxFromCoords(const std::vector<Coordinate> &coords) noexcept {
    if (coords.empty()) {
        return {0.0, 0.0, 0.0, 0.0};
    }
    BBox bb{coords[0].x, coords[0].y, coords[0].x, coords[0].y};
    for (const auto &c : coords) {
        if (c.x < bb.min_x)
            bb.min_x = c.x;
        if (c.y < bb.min_y)
            bb.min_y = c.y;
        if (c.x > bb.max_x)
            bb.max_x = c.x;
        if (c.y > bb.max_y)
            bb.max_y = c.y;
    }
    return bb;
}

BBox mergeBBox(const BBox &a, const BBox &b) noexcept {
    return {std::min(a.min_x, b.min_x), std::min(a.min_y, b.min_y), std::max(a.max_x, b.max_x),
            std::max(a.max_y, b.max_y)};
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// GeoPoint
// ---------------------------------------------------------------------------

std::string GeoPoint::toGeoJSON() const {
    return R"({"type":"Point","coordinates":)" + coordToJson(coord_) + "}";
}

ValidationResult GeoPoint::validate() const {
    return validateCoordinate(coord_, crs_, "GeoPoint");
}

// ---------------------------------------------------------------------------
// GeoLineString
// ---------------------------------------------------------------------------

BBox GeoLineString::bbox() const noexcept {
    return bboxFromCoords(coords_);
}

std::string GeoLineString::toGeoJSON() const {
    return R"({"type":"LineString","coordinates":)" + coordsToJson(coords_) + "}";
}

ValidationResult GeoLineString::validate() const {
    ValidationResult vr;
    if (coords_.size() < 2) {
        vr.addError({"INSUFFICIENT_POSITIONS",
                     "GeoLineString requires at least 2 positions, got " + std::to_string(coords_.size())});
    }
    for (std::size_t i = 0; i < coords_.size(); ++i) {
        vr.merge(validateCoordinate(coords_[i], crs_, "GeoLineString[" + std::to_string(i) + "]"));
    }
    return vr;
}

// ---------------------------------------------------------------------------
// GeoPolygon — helpers
// ---------------------------------------------------------------------------

namespace {

ValidationResult validateRing(const GeoPolygon::Ring &ring, CrsId crs, const std::string &name, bool must_be_ccw) {
    ValidationResult vr;
    if (ring.size() < 4) {
        vr.addError({"INSUFFICIENT_RING_POSITIONS",
                     name + ": ring requires >= 4 positions, got " + std::to_string(ring.size())});
        return vr; // Cannot check further
    }
    // Closure check: first == last
    if (ring.front().x != ring.back().x || ring.front().y != ring.back().y) {
        vr.addError({"RING_NOT_CLOSED", name + ": first and last position must be identical"});
    }
    // Coordinate validity
    for (std::size_t i = 0; i < ring.size(); ++i) {
        vr.merge(validateCoordinate(ring[i], crs, name + "[" + std::to_string(i) + "]"));
    }
    // Winding-order check (only meaningful when ring is closed and has ≥ 3 unique vertices)
    if (vr.ok()) {
        const bool ccw = ringIsCCW(ring);
        if (must_be_ccw && !ccw) {
            vr.addError(
                {"WINDING_ORDER_VIOLATION", name + ": exterior ring must be counter-clockwise (right-hand rule)"});
        } else if (!must_be_ccw && ccw) {
            vr.addError({"WINDING_ORDER_VIOLATION", name + ": interior ring must be clockwise"});
        }
    }
    return vr;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// GeoPolygon
// ---------------------------------------------------------------------------

BBox GeoPolygon::bbox() const noexcept {
    if (rings_.empty()) {
        return {0.0, 0.0, 0.0, 0.0};
    }
    return bboxFromCoords(rings_[0]);
}

std::string GeoPolygon::toGeoJSON() const {
    std::string result = R"({"type":"Polygon","coordinates":[)";
    for (std::size_t i = 0; i < rings_.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += coordsToJson(rings_[i]);
    }
    result += "]}";
    return result;
}

ValidationResult GeoPolygon::validate() const {
    ValidationResult vr;
    if (rings_.empty()) {
        vr.addError({"NO_RINGS", "GeoPolygon must have at least one ring"});
        return vr;
    }
    // Exterior ring: must be CCW
    vr.merge(validateRing(rings_[0], crs_, "exterior_ring", /*must_be_ccw=*/true));
    // Interior rings: must be CW
    for (std::size_t i = 1; i < rings_.size(); ++i) {
        vr.merge(validateRing(rings_[i], crs_, "interior_ring[" + std::to_string(i - 1) + "]",
                              /*must_be_ccw=*/false));
    }
    return vr;
}

// ---------------------------------------------------------------------------
// GeoMultiPolygon
// ---------------------------------------------------------------------------

BBox GeoMultiPolygon::bbox() const noexcept {
    if (polygons_.empty()) {
        return {0.0, 0.0, 0.0, 0.0};
    }
    BBox bb = polygons_[0].bbox();
    for (std::size_t i = 1; i < polygons_.size(); ++i) {
        bb = mergeBBox(bb, polygons_[i].bbox());
    }
    return bb;
}

std::string GeoMultiPolygon::toGeoJSON() const {
    std::string result = R"({"type":"MultiPolygon","coordinates":[)";
    for (std::size_t i = 0; i < polygons_.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        // Extract the coordinates array from each polygon's GeoJSON
        const auto polyJson = polygons_[i].toGeoJSON();
        // Find "coordinates":[...] and extract the [...] part
        const auto pos = polyJson.find("\"coordinates\":");
        if (pos != std::string::npos) {
            result += polyJson.substr(pos + 14, polyJson.size() - pos - 15);
        }
    }
    result += "]}";
    return result;
}

ValidationResult GeoMultiPolygon::validate() const {
    ValidationResult vr;
    for (std::size_t i = 0; i < polygons_.size(); ++i) {
        const auto sub = polygons_[i].validate();
        if (!sub.ok()) {
            for (const auto &e : sub.errors()) {
                vr.addError({"polygon[" + std::to_string(i) + "]." + e.code, e.message});
            }
        }
    }
    return vr;
}

// ---------------------------------------------------------------------------
// GeoGeometryCollection
// ---------------------------------------------------------------------------

BBox GeoGeometryCollection::bbox() const noexcept {
    if (members_.empty()) {
        return {0.0, 0.0, 0.0, 0.0};
    }
    BBox bb = members_[0]->bbox();
    for (std::size_t i = 1; i < members_.size(); ++i) {
        bb = mergeBBox(bb, members_[i]->bbox());
    }
    return bb;
}

std::string GeoGeometryCollection::toGeoJSON() const {
    std::string result = R"({"type":"GeometryCollection","geometries":[)";
    for (std::size_t i = 0; i < members_.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += members_[i]->toGeoJSON();
    }
    result += "]}";
    return result;
}

ValidationResult GeoGeometryCollection::validate() const {
    ValidationResult vr;
    for (std::size_t i = 0; i < members_.size(); ++i) {
        const auto sub = members_[i]->validate();
        if (!sub.ok()) {
            for (const auto &e : sub.errors()) {
                vr.addError({"geometry[" + std::to_string(i) + "]." + e.code, e.message});
            }
        }
    }
    return vr;
}

} // namespace geo
} // namespace themis
