/**
 * @file geo_json_geometry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

// Use the canonical Coordinate/MBR/GeometryInfo types from ewkb.h so that
// this header composes cleanly with other geo headers that also include it.
#include "utils/geo/ewkb.h"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Coordinate equality helper
// ---------------------------------------------------------------------------
// Adds == comparison on the ewkb.h Coordinate without modifying that header.
inline bool coordinateEqual(const Coordinate& a, const Coordinate& b) noexcept {
    return a.x == b.x && a.y == b.y;
}

// ---------------------------------------------------------------------------
// Coordinate Reference System
// ---------------------------------------------------------------------------

/**
 * @brief Supported coordinate reference systems.
 *
 * All geometry types require an explicit CRS; there is no implicit WGS-84 default.
 */
enum class CrsId : int {
    /// WGS-84 geographic (EPSG:4326) — decimal degrees, geodesic distances.
    WGS84   = 4326,
    /// Web Mercator (EPSG:3857) — projected metres.
    EPSG3857 = 3857,
    /// WGS-84 geocentric 3D Cartesian / ECEF (EPSG:4978).
    EPSG4978 = 4978,
    /// Caller-defined CRS; identified by a custom SRID set at construction.
    Custom   = 0,
};

// ---------------------------------------------------------------------------
// Bounding box
// ---------------------------------------------------------------------------

/// Axis-aligned bounding box following RFC 7946 §5.
///
/// @note RFC 7946 §5: a bbox is represented as [west, south, east, north]
///       for 2D coordinates.  The fields below map to that order:
///       min_x=west, min_y=south, max_x=east, max_y=north.
struct BBox {
    double min_x{0.0};
    double min_y{0.0};
    double max_x{0.0};
    double max_y{0.0};

    bool operator==(const BBox& o) const noexcept {
        return min_x == o.min_x && min_y == o.min_y &&
               max_x == o.max_x && max_y == o.max_y;
    }
};

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

/**
 * @brief A single geometry validation error.
 */
struct ValidationError {
    std::string code;    ///< Machine-readable code, e.g. "WINDING_ORDER_VIOLATION".
    std::string message; ///< Human-readable description.
};

/**
 * @brief Result of a `validate()` call.
 *
 * `ok()` returns true when no errors were found.  Errors are accumulated by
 * `addError()` and retrieved via `errors()`.
 */
class ValidationResult {
public:
    /// @return true when there are no validation errors.
    [[nodiscard]] bool ok() const noexcept { return errors_.empty(); }

    /// @return the list of validation errors (may be empty).
    [[nodiscard]] const std::vector<ValidationError>& errors() const noexcept {
        return errors_;
    }

    /// Append a validation error.
    void addError(ValidationError e) { errors_.push_back(std::move(e)); }

    /// Merge errors from another result into this one.
    void merge(const ValidationResult& other) {
        for (const auto& e : other.errors_)
            errors_.push_back(e);
    }

private:
    std::vector<ValidationError> errors_;
};

// ---------------------------------------------------------------------------
// IGeoJSONGeometry — abstract base
// ---------------------------------------------------------------------------

/**
 * @brief Abstract base for all CRS-aware GeoJSON geometry value types.
 *
 * Derived classes are immutable after construction.  All mutation operations
 * return a new instance rather than modifying the receiver.
 */
class IGeoJSONGeometry {
public:
    virtual ~IGeoJSONGeometry() = default;

    /// @return RFC 7946 geometry type string, e.g. "Point", "Polygon".
    [[nodiscard]] virtual std::string type() const noexcept = 0;

    /// @return Axis-aligned bounding box of this geometry.
    [[nodiscard]] virtual BBox bbox() const noexcept = 0;

    /// @return The coordinate reference system used by this geometry.
    [[nodiscard]] virtual CrsId crs() const noexcept = 0;

    /// @return RFC 7946 GeoJSON representation as a JSON string.
    [[nodiscard]] virtual std::string toGeoJSON() const = 0;

    /**
     * @brief Validate this geometry for RFC 7946 conformance.
     *
     * Checks include: finite coordinates, latitude/longitude range for WGS-84,
     * minimum ring size, right-hand-rule winding for `GeoPolygon`, and ring
     * closure.
     */
    [[nodiscard]] virtual ValidationResult validate() const = 0;
};

// ---------------------------------------------------------------------------
// GeoPoint
// ---------------------------------------------------------------------------

/**
 * @brief A single GeoJSON Point geometry.
 *
 * Immutable after construction.  The coordinate is stored as (x, y) where
 * x is longitude (or easting) and y is latitude (or northing).
 *
 * @note RFC 7946 §3.1.2 — Point:
 *   Position is an array of at least two numbers [longitude, latitude].
 *   For WGS-84 (CrsId::WGS84): longitude ∈ [-180, 180], latitude ∈ [-90, 90].
 */
class GeoPoint final : public IGeoJSONGeometry {
public:
    /**
     * @param coord  Point coordinate.
     * @param crs    Coordinate reference system (required; no implicit WGS-84).
     */
    GeoPoint(Coordinate coord, CrsId crs) noexcept
        : coord_(coord), crs_(crs) {}

    [[nodiscard]] const Coordinate& coordinate() const noexcept { return coord_; }

    [[nodiscard]] std::string type() const noexcept override { return "Point"; }

    [[nodiscard]] BBox bbox() const noexcept override {
        return {coord_.x, coord_.y, coord_.x, coord_.y};
    }

    [[nodiscard]] CrsId crs() const noexcept override { return crs_; }

    [[nodiscard]] std::string toGeoJSON() const override;

    [[nodiscard]] ValidationResult validate() const override;

private:
    Coordinate coord_;
    CrsId      crs_;
};

// ---------------------------------------------------------------------------
// GeoLineString
// ---------------------------------------------------------------------------

/**
 * @brief A GeoJSON LineString geometry (ordered sequence of ≥ 2 positions).
 *
 * Immutable after construction.
 *
 * @note RFC 7946 §3.1.4 — LineString:
 *   Two or more positions.  A LinearRing (closed LineString) requires ≥ 4
 *   positions with the first and last being identical.
 */
class GeoLineString final : public IGeoJSONGeometry {
public:
    /**
     * @param coords  Ordered list of coordinates (must have ≥ 2 elements for
     *                a valid LineString; a single-position LineString is
     *                invalid per RFC 7946 but is accepted at construction and
     *                reported by `validate()`).
     * @param crs     Coordinate reference system.
     */
    GeoLineString(std::vector<Coordinate> coords, CrsId crs)
        : coords_(std::move(coords)), crs_(crs) {}

    [[nodiscard]] const std::vector<Coordinate>& coordinates() const noexcept {
        return coords_;
    }

    [[nodiscard]] std::string type() const noexcept override { return "LineString"; }

    [[nodiscard]] BBox bbox() const noexcept override;

    [[nodiscard]] CrsId crs() const noexcept override { return crs_; }

    [[nodiscard]] std::string toGeoJSON() const override;

    [[nodiscard]] ValidationResult validate() const override;

private:
    std::vector<Coordinate> coords_;
    CrsId                   crs_;
};

// ---------------------------------------------------------------------------
// GeoPolygon
// ---------------------------------------------------------------------------

/**
 * @brief A GeoJSON Polygon geometry.
 *
 * The first ring is the exterior (outer) ring; subsequent rings are interior
 * (hole) rings.  `GeoPolygon` enforces RFC 7946 right-hand-rule winding on
 * the exterior ring (counter-clockwise); interior rings must be clockwise.
 * A `ValidationResult` error with code `WINDING_ORDER_VIOLATION` is returned
 * by `validate()` when the rule is violated; construction still succeeds.
 *
 * Immutable after construction.
 *
 * @note RFC 7946 §3.1.6 — Polygon:
 *   - The exterior ring MUST be counter-clockwise (right-hand rule).
 *   - Interior rings (holes) MUST be clockwise.
 *   - Each ring MUST be a LinearRing: ≥ 4 positions, first == last position.
 *   - Coordinate order within each position: [longitude, latitude] for WGS-84.
 *
 * @note RFC 7946 §3.1.9 — Winding order note:
 *   Tools consuming GeoJSON "SHOULD" follow the right-hand rule even if
 *   produced by systems that use the left-hand rule.  ThemisDB enforces this
 *   at validation time and reports WINDING_ORDER_VIOLATION when violated.
 */
class GeoPolygon final : public IGeoJSONGeometry {
public:
    using Ring = std::vector<Coordinate>;

    /**
     * @param rings  Exterior ring followed by zero or more hole rings.
     *               Each ring must have ≥ 4 positions and be closed (first ==
     *               last coordinate).
     * @param crs    Coordinate reference system.
     */
    GeoPolygon(std::vector<Ring> rings, CrsId crs)
        : rings_(std::move(rings)), crs_(crs) {}

    [[nodiscard]] const std::vector<Ring>& rings() const noexcept { return rings_; }

    [[nodiscard]] const Ring& exteriorRing() const noexcept { return rings_[0]; }

    [[nodiscard]] std::string type() const noexcept override { return "Polygon"; }

    [[nodiscard]] BBox bbox() const noexcept override;

    [[nodiscard]] CrsId crs() const noexcept override { return crs_; }

    [[nodiscard]] std::string toGeoJSON() const override;

    /**
     * Validates:
     *  - rings_.size() >= 1
     *  - each ring has >= 4 positions and is closed
     *  - exterior ring is counter-clockwise (right-hand rule)
     *  - interior rings are clockwise
     *  - all coordinates finite (WGS-84 range check for CrsId::WGS84)
     */
    [[nodiscard]] ValidationResult validate() const override;

private:
    std::vector<Ring> rings_;
    CrsId             crs_;
};

// ---------------------------------------------------------------------------
// GeoMultiPolygon
// ---------------------------------------------------------------------------

/**
 * @brief A GeoJSON MultiPolygon geometry (collection of Polygons).
 *
 * Immutable after construction.
 */
class GeoMultiPolygon final : public IGeoJSONGeometry {
public:
    explicit GeoMultiPolygon(std::vector<GeoPolygon> polygons, CrsId crs)
        : polygons_(std::move(polygons)), crs_(crs) {}

    [[nodiscard]] const std::vector<GeoPolygon>& polygons() const noexcept {
        return polygons_;
    }

    [[nodiscard]] std::string type() const noexcept override { return "MultiPolygon"; }

    [[nodiscard]] BBox bbox() const noexcept override;

    [[nodiscard]] CrsId crs() const noexcept override { return crs_; }

    [[nodiscard]] std::string toGeoJSON() const override;

    [[nodiscard]] ValidationResult validate() const override;

private:
    std::vector<GeoPolygon> polygons_;
    CrsId                   crs_;
};

// ---------------------------------------------------------------------------
// GeoGeometryCollection
// ---------------------------------------------------------------------------

/**
 * @brief A GeoJSON GeometryCollection (heterogeneous collection of geometries).
 *
 * Immutable after construction.  Each member geometry may be of any type,
 * including a nested GeometryCollection.
 */
class GeoGeometryCollection final : public IGeoJSONGeometry {
public:
    explicit GeoGeometryCollection(
            std::vector<std::shared_ptr<IGeoJSONGeometry>> members, CrsId crs)
        : members_(std::move(members)), crs_(crs) {}

    [[nodiscard]] const std::vector<std::shared_ptr<IGeoJSONGeometry>>& members()
            const noexcept {
        return members_;
    }

    [[nodiscard]] std::string type() const noexcept override {
        return "GeometryCollection";
    }

    [[nodiscard]] BBox bbox() const noexcept override;

    [[nodiscard]] CrsId crs() const noexcept override { return crs_; }

    [[nodiscard]] std::string toGeoJSON() const override;

    [[nodiscard]] ValidationResult validate() const override;

private:
    std::vector<std::shared_ptr<IGeoJSONGeometry>> members_;
    CrsId                                          crs_;
};

// ---------------------------------------------------------------------------
// Helpers / factories
// ---------------------------------------------------------------------------

/**
 * @brief Compute the signed area of a ring using the shoelace formula.
 *
 * Positive area indicates counter-clockwise (CCW) winding.
 * Negative area indicates clockwise (CW) winding.
 */
[[nodiscard]] double ringSignedArea(const std::vector<Coordinate>& ring) noexcept;

/**
 * @brief Return true if the ring has counter-clockwise (CCW) winding.
 *
 * Used internally by GeoPolygon::validate() to enforce the RFC 7946
 * right-hand rule (exterior rings must be CCW).
 */
[[nodiscard]] bool ringIsCCW(const std::vector<Coordinate>& ring) noexcept;

/**
 * @brief Return true if the coordinate is within the valid WGS-84 range.
 *
 * x ∈ [−180, +180], y ∈ [−90, +90], both finite.
 */
[[nodiscard]] bool isValidWGS84Coordinate(const Coordinate& c) noexcept;

} // namespace geo
} // namespace themis
