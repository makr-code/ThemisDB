/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geojson_geometry.h                                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/geo/ewkb.h"

#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace geo {

// ── Coordinate Reference System ───────────────────────────────────────────

/**
 * @brief Coordinate reference system (CRS) identifier.
 *
 * CRS is a required constructor argument for all geometry types; there is
 * no implicit WGS84 default.  Pass `CRS::wgs84()` for the standard
 * geographic CRS used by GeoJSON RFC 7946.
 */
class CRS {
public:
    /// Standard WGS-84 geographic CRS (EPSG:4326), required by RFC 7946.
    static CRS wgs84() { return CRS{"EPSG:4326"}; }

    explicit CRS(std::string srid) : srid_(std::move(srid)) {}

    const std::string& srid() const noexcept { return srid_; }

    bool operator==(const CRS& other) const noexcept {
        return srid_ == other.srid_;
    }
    bool operator!=(const CRS& other) const noexcept {
        return !(*this == other);
    }

private:
    std::string srid_;
};

// ── Validation ────────────────────────────────────────────────────────────

/// Detailed error codes returned by IGeoJSONGeometry::validate().
enum class GeoValidationCode {
    OK = 0,
    NAN_COORDINATE,         ///< A coordinate value is NaN or infinite.
    OUT_OF_RANGE_LON,       ///< Longitude outside [-180, 180].
    OUT_OF_RANGE_LAT,       ///< Latitude outside [-90, 90].
    EMPTY_GEOMETRY,         ///< Geometry has no coordinates.
    RING_NOT_CLOSED,        ///< Polygon ring first != last vertex.
    WRONG_WINDING_ORDER,    ///< Polygon exterior ring not counter-clockwise (right-hand rule).
    CRS_MISMATCH,           ///< Geometries have incompatible CRS values.
    UNSUPPORTED_TYPE,       ///< Geometry type is not supported by this implementation.
};

/// Result of a call to IGeoJSONGeometry::validate().
struct ValidationResult {
    GeoValidationCode code{GeoValidationCode::OK};
    std::string message;

    bool ok() const noexcept { return code == GeoValidationCode::OK; }

    static ValidationResult success() { return {}; }
    static ValidationResult error(GeoValidationCode c, std::string msg) {
        return {c, std::move(msg)};
    }
};

// ── Base Interface ────────────────────────────────────────────────────────

/**
 * @brief Base interface for GeoJSON RFC 7946 geometry value types.
 *
 * All concrete geometry types are immutable value types.  Mutation returns
 * a new instance rather than modifying the original.
 *
 * Geometry equality uses coordinate tolerance configurable via
 * `GeoConfig::coordinateTolerance()` (defaults to 1e-9 degrees).
 */
class IGeoJSONGeometry {
public:
    virtual ~IGeoJSONGeometry() = default;

    /// Return the RFC 7946 geometry type string (e.g. "Point", "Polygon").
    virtual const char* type() const noexcept = 0;

    /// Return the axis-aligned minimum bounding rectangle.
    virtual MBR bbox() const = 0;

    /// Return the coordinate reference system of this geometry.
    virtual const CRS& crs() const noexcept = 0;

    /// Serialise the geometry to a compact GeoJSON string.
    virtual std::string toGeoJSON() const = 0;

    /**
     * @brief Validate the geometry for RFC 7946 compliance.
     *
     * Checks for NaN / infinite coordinates, out-of-range WGS84 values,
     * correct polygon winding order, and closed rings.
     *
     * @return ValidationResult::OK when the geometry is valid; an error
     *         result with a descriptive message otherwise.
     */
    virtual ValidationResult validate() const = 0;
};

// ── Concrete Geometry Types ───────────────────────────────────────────────

/**
 * @brief Immutable GeoJSON Point geometry.
 *
 * Holds a single (longitude, latitude) coordinate pair in WGS84.  The CRS
 * is a required constructor argument.
 */
class GeoPoint final : public IGeoJSONGeometry {
public:
    /**
     * @param lon  Longitude in degrees (WGS84: [-180, 180]).
     * @param lat  Latitude in degrees (WGS84: [-90, 90]).
     * @param crs  Coordinate reference system.
     */
    GeoPoint(double lon, double lat, CRS crs)
        : lon_(lon), lat_(lat), crs_(std::move(crs)) {}

    const char* type() const noexcept override { return "Point"; }

    double lon() const noexcept { return lon_; }
    double lat() const noexcept { return lat_; }

    MBR bbox() const override {
        return MBR{lon_, lat_, lon_, lat_};
    }

    const CRS& crs() const noexcept override { return crs_; }

    std::string toGeoJSON() const override;
    ValidationResult validate() const override;

private:
    double lon_;
    double lat_;
    CRS crs_;
};

/**
 * @brief Immutable GeoJSON LineString geometry.
 */
class GeoLineString final : public IGeoJSONGeometry {
public:
    explicit GeoLineString(std::vector<Coordinate> coords, CRS crs)
        : coords_(std::move(coords)), crs_(std::move(crs)) {}

    const char* type() const noexcept override { return "LineString"; }

    const std::vector<Coordinate>& coordinates() const noexcept {
        return coords_;
    }

    MBR bbox() const override;
    const CRS& crs() const noexcept override { return crs_; }
    std::string toGeoJSON() const override;
    ValidationResult validate() const override;

private:
    std::vector<Coordinate> coords_;
    CRS crs_;
};

/**
 * @brief Immutable GeoJSON Polygon geometry.
 *
 * The exterior ring (index 0) must follow the right-hand rule (counter-
 * clockwise winding in geographic coordinates, per RFC 7946).  Hole rings
 * (indices ≥ 1) must be clockwise.  Construction validates winding order
 * and returns an error via `validate()` on violation.
 */
class GeoPolygon final : public IGeoJSONGeometry {
public:
    /**
     * @param rings  Coordinate rings.  rings[0] is the exterior ring;
     *               rings[1..n-1] are interior holes.
     * @param crs    Coordinate reference system.
     */
    explicit GeoPolygon(std::vector<std::vector<Coordinate>> rings, CRS crs)
        : rings_(std::move(rings)), crs_(std::move(crs)) {}

    const char* type() const noexcept override { return "Polygon"; }

    const std::vector<std::vector<Coordinate>>& rings() const noexcept {
        return rings_;
    }

    MBR bbox() const override;
    const CRS& crs() const noexcept override { return crs_; }
    std::string toGeoJSON() const override;
    ValidationResult validate() const override;

private:
    std::vector<std::vector<Coordinate>> rings_;
    CRS crs_;
};

/**
 * @brief Immutable GeoJSON MultiPolygon geometry.
 */
class GeoMultiPolygon final : public IGeoJSONGeometry {
public:
    explicit GeoMultiPolygon(std::vector<GeoPolygon> polygons, CRS crs)
        : polygons_(std::move(polygons)), crs_(std::move(crs)) {}

    const char* type() const noexcept override { return "MultiPolygon"; }

    const std::vector<GeoPolygon>& polygons() const noexcept {
        return polygons_;
    }

    MBR bbox() const override;
    const CRS& crs() const noexcept override { return crs_; }
    std::string toGeoJSON() const override;
    ValidationResult validate() const override;

private:
    std::vector<GeoPolygon> polygons_;
    CRS crs_;
};

/**
 * @brief Immutable GeoJSON GeometryCollection.
 */
class GeoGeometryCollection final : public IGeoJSONGeometry {
public:
    explicit GeoGeometryCollection(
        std::vector<std::shared_ptr<IGeoJSONGeometry>> geometries, CRS crs)
        : geometries_(std::move(geometries)), crs_(std::move(crs)) {}

    const char* type() const noexcept override { return "GeometryCollection"; }

    const std::vector<std::shared_ptr<IGeoJSONGeometry>>& geometries()
        const noexcept {
        return geometries_;
    }

    MBR bbox() const override;
    const CRS& crs() const noexcept override { return crs_; }
    std::string toGeoJSON() const override;
    ValidationResult validate() const override;

private:
    std::vector<std::shared_ptr<IGeoJSONGeometry>> geometries_;
    CRS crs_;
};

// ── Configuration ─────────────────────────────────────────────────────────

/**
 * @brief Module-level geometry configuration.
 *
 * All geometry equality comparisons use `coordinateTolerance()`.
 * The singleton is mutable only during initialisation; concurrent writes
 * are not safe.
 */
class GeoConfig {
public:
    static GeoConfig& instance();

    double coordinateTolerance() const noexcept { return coord_tolerance_; }
    void setCoordinateTolerance(double tol) noexcept { coord_tolerance_ = tol; }

private:
    GeoConfig() = default;
    double coord_tolerance_{1e-9};
};

} // namespace geo
} // namespace themis
