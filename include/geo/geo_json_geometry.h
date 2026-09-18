/**
 * @file geo_json_geometry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

enum class CrsId : int {
    WGS84   = 4326,
    EPSG3857 = 3857,
    EPSG4978 = 4978,
    Custom   = 0,
};

// ---------------------------------------------------------------------------
// Bounding box
// ---------------------------------------------------------------------------

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

struct ValidationError {
    std::string code;    ///< Machine-readable code, e.g. "WINDING_ORDER_VIOLATION".
    std::string message; ///< Human-readable description.
};

class ValidationResult {
public:
    [[nodiscard]] bool ok() const noexcept { return errors_.empty(); }

    [[nodiscard]] const std::vector<ValidationError>& errors() const noexcept {
        return errors_;
    }

    /**
     * @brief Add Error.
     * @param[in] e Input parameter.
     * @details Calls: push_back(), std::move().
     */
    void addError(ValidationError e) { errors_.push_back(std::move(e)); }

    /**
     * @brief Merge.
     * @param[in] other Input parameter.
     * @details Calls: push_back().
     */
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

class IGeoJSONGeometry {
public:
    /**
     * @brief IGeo JSONGeometry.
     * @return Return value.
     */
    virtual ~IGeoJSONGeometry() = default;

    [[nodiscard]] virtual std::string type() const noexcept = 0;

    [[nodiscard]] virtual BBox bbox() const noexcept = 0;

    [[nodiscard]] virtual CrsId crs() const noexcept = 0;

    [[nodiscard]] virtual std::string toGeoJSON() const = 0;

    [[nodiscard]] virtual ValidationResult validate() const = 0;
};

// ---------------------------------------------------------------------------
// GeoPoint
// ---------------------------------------------------------------------------

class GeoPoint final : public IGeoJSONGeometry {
public:
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

class GeoLineString final : public IGeoJSONGeometry {
public:
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

class GeoPolygon final : public IGeoJSONGeometry {
public:
    using Ring = std::vector<Coordinate>;

    GeoPolygon(std::vector<Ring> rings, CrsId crs)
        : rings_(std::move(rings)), crs_(crs) {}

    [[nodiscard]] const std::vector<Ring>& rings() const noexcept { return rings_; }

    [[nodiscard]] const Ring& exteriorRing() const noexcept { return rings_[0]; }

    [[nodiscard]] std::string type() const noexcept override { return "Polygon"; }

    [[nodiscard]] BBox bbox() const noexcept override;

    [[nodiscard]] CrsId crs() const noexcept override { return crs_; }

    [[nodiscard]] std::string toGeoJSON() const override;

    [[nodiscard]] ValidationResult validate() const override;

private:
    std::vector<Ring> rings_;
    CrsId             crs_;
};

// ---------------------------------------------------------------------------
// GeoMultiPolygon
// ---------------------------------------------------------------------------

class GeoMultiPolygon final : public IGeoJSONGeometry {
public:
    /**
     * @brief Geo Multi Polygon.
     * @param[in] polygons Input parameter.
     * @param[in] crs Input parameter.
     * @return Return value.
     */
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

class GeoGeometryCollection final : public IGeoJSONGeometry {
public:
    /**
     * @brief Geo Geometry Collection.
     * @param[in] members Input parameter.
     * @param[in] crs Input parameter.
     * @return Return value.
     */
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

[[nodiscard]] double ringSignedArea(const std::vector<Coordinate>& ring) noexcept;

[[nodiscard]] bool ringIsCCW(const std::vector<Coordinate>& ring) noexcept;

[[nodiscard]] bool isValidWGS84Coordinate(const Coordinate& c) noexcept;

} // namespace geo
} // namespace themis
