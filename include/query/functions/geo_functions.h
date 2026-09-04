/**
 * @file geo_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "function_registry.h"
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <limits>



// Ensure M_PI is defined for portability
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace themis {
namespace query {
namespace functions {

/**
 * @brief Geo/Spatial Functions for AQL
 * 
 * Provides OGC-compatible spatial functions using GeoJSON format.
 * 
 * Sources:
 * - Standards: OGC Simple Features Specification
 *   URL: https://www.ogc.org/standards/sfa
 * - GeoJSON: RFC 7946
 *   URL: https://tools.ietf.org/html/rfc7946
 * - Inspiration: ArangoDB Geo Functions
 *   Repository: https://github.com/arangodb/arangodb
 *   License: Apache 2.0
 *   Documentation: https://www.arangodb.com/docs/stable/aql/functions-geo.html
 * - Inspiration: PostGIS
 *   Repository: https://github.com/postgis/postgis
 *   License: GPL 2.0
 * - ThemisDB Implementation: Custom spatial functions with AQL-compatible syntax
 *   - OGC Simple Features compliance
 *   - GeoJSON format support
 *   - Great-circle distance calculations (Haversine formula)
 *   - Integration with ThemisDB spatial indexes
 * 
 * ## Supported Geometry Types
 * - Point, LineString, Polygon
 * - MultiPoint, MultiLineString, MultiPolygon
 * - GeometryCollection
 * 
 * ## Coordinate Systems
 * - Default: WGS84 (EPSG:4326) for geographic data
 * - Supports 2D (x,y) and 3D (x,y,z) coordinates
 * 
 * ## Functions
 * - Construction: ST_POINT, ST_LINESTRING, ST_POLYGON, ST_GEOMFROMTEXT, ST_GEOMFROMGEOJSON
 * - Measurement: ST_DISTANCE, ST_LENGTH, ST_AREA, ST_PERIMETER
 * - Predicates: ST_INTERSECTS, ST_CONTAINS, ST_WITHIN, ST_TOUCHES, ST_OVERLAPS, ST_DWITHIN
 * - Accessors: ST_X, ST_Y, ST_Z, ST_SRID, ST_ASGEOJSON, ST_ASTEXT, ST_HASZ
 * - Processing: ST_BUFFER, ST_CENTROID, ST_ENVELOPE, ST_SIMPLIFY, ST_UNION, ST_INTERSECTION
 */

// ============================================================================
// Helper Functions
// ============================================================================

namespace geo_helpers {

// Earth radius in meters for great-circle calculations
constexpr double EARTH_RADIUS_M = 6371000.0;

// Minimum Bounding Rectangle
struct MBR {
    // UNINIT-21: add NSDMI so default-constructed MBR has defined values.
    double minx = 0.0, miny = 0.0, maxx = 0.0, maxy = 0.0;
    
    bool contains(double x, double y) const {
        return x >= minx && x <= maxx && y >= miny && y <= maxy;
    }
    
    bool containsMBR(const MBR& other) const {
        return other.minx >= minx && other.maxx <= maxx &&
               other.miny >= miny && other.maxy <= maxy;
    }
    
    bool intersects(const MBR& other) const {
        return !(other.maxx < minx || other.minx > maxx ||
                 other.maxy < miny || other.miny > maxy);
    }
};

inline double deg2rad(double deg) {
    return deg * M_PI / 180.0;
}

inline double rad2deg(double rad) {
    return rad * 180.0 / M_PI;
}

// Haversine distance in meters
inline double haversineDistance(double lon1, double lat1, double lon2, double lat2) {
    double dLat = deg2rad(lat2 - lat1);
    double dLon = deg2rad(lon2 - lon1);
    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(deg2rad(lat1)) * std::cos(deg2rad(lat2)) *
               std::sin(dLon/2) * std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return EARTH_RADIUS_M * c;
}

// Euclidean distance (2D)
inline double euclideanDistance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

// Euclidean distance (3D)
inline double euclideanDistance3D(double x1, double y1, double z1, double x2, double y2, double z2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Extract point coordinates from GeoJSON
inline std::tuple<double, double, double> extractPoint(const nlohmann::json& geojson) {
    if (!geojson.is_object() || !geojson.contains("type") || geojson["type"] != "Point") {
        throw std::runtime_error("Expected Point geometry");
    }
    if (!geojson.contains("coordinates") || !geojson["coordinates"].is_array()) {
        throw std::runtime_error("Invalid Point coordinates");
    }
    const auto& coords = geojson["coordinates"];
    double x = coords[0].get<double>();
    double y = coords[1].get<double>();
    double z = coords.size() >= 3 ? coords[2].get<double>() : 0.0;
    return {x, y, z};
}

// Extract MBR from any geometry
inline MBR extractMBR(const nlohmann::json& geojson) {
    if (!geojson.is_object() || !geojson.contains("type")) {
        throw std::runtime_error("Invalid geometry");
    }
    
    std::string type = geojson["type"];
    double minx = std::numeric_limits<double>::max();
    double miny = std::numeric_limits<double>::max();
    double maxx = std::numeric_limits<double>::lowest();
    double maxy = std::numeric_limits<double>::lowest();
    
    auto updateBounds = [&](double x, double y) {
        minx = std::min(minx, x);
        miny = std::min(miny, y);
        maxx = std::max(maxx, x);
        maxy = std::max(maxy, y);
    };
    
    if (type == "Point") {
        auto [x, y, z] = extractPoint(geojson);
        updateBounds(x, y);
    } else if (type == "LineString" || type == "MultiPoint") {
        for (const auto& coord : geojson["coordinates"]) {
            updateBounds(coord[0].get<double>(), coord[1].get<double>());
        }
    } else if (type == "Polygon" || type == "MultiLineString") {
        for (const auto& ring : geojson["coordinates"]) {
            for (const auto& coord : ring) {
                updateBounds(coord[0].get<double>(), coord[1].get<double>());
            }
        }
    } else if (type == "MultiPolygon") {
        for (const auto& polygon : geojson["coordinates"]) {
            for (const auto& ring : polygon) {
                for (const auto& coord : ring) {
                    updateBounds(coord[0].get<double>(), coord[1].get<double>());
                }
            }
        }
    }
    
    return {minx, miny, maxx, maxy};
}

// Check if coordinates look like WGS84 degrees
inline bool looksLikeDegrees(double lon, double lat) {
    return lon >= -180.0 && lon <= 180.0 && lat >= -90.0 && lat <= 90.0;
}

} // namespace geo_helpers

// ============================================================================
// Construction Functions
// ============================================================================

/**
 * @brief ST_POINT(x, y) or ST_POINT(x, y, z) - Create a Point geometry
 */
class StPointFunction : public IFunction {
public:
    ~StPointFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_POINT",
            "Geo",
            "Create a Point geometry from coordinates",
            {
                {"x", ArgType::NUMBER, true, nullptr, "X coordinate (longitude)"},
                {"y", ArgType::NUMBER, true, nullptr, "Y coordinate (latitude)"},
                {"z", ArgType::NUMBER, false, nullptr, "Z coordinate (elevation)"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_POINT(13.4, 52.5)", "ST_POINT(13.4, 52.5, 100)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        double x = args[0].get<double>();
        double y = args[1].get<double>();
        
        nlohmann::json geojson;
        geojson["type"] = "Point";
        
        if (args.size() >= 3 && !args[2].is_null()) {
            double z = args[2].get<double>();
            geojson["coordinates"] = {x, y, z};
        } else {
            geojson["coordinates"] = {x, y};
        }
        
        return geojson;
    }
};

/**
 * @brief ST_LINESTRING([[x1,y1], [x2,y2], ...]) - Create a LineString geometry
 */
class StLinestringFunction : public IFunction {
public:
    ~StLinestringFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_LINESTRING",
            "Geo",
            "Create a LineString geometry from array of coordinate pairs",
            {
                {"coordinates", ArgType::ARRAY, true, nullptr, "Array of [x,y] or [x,y,z] coordinates"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_LINESTRING([[0,0], [1,1], [2,0]])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        nlohmann::json geojson;
        geojson["type"] = "LineString";
        geojson["coordinates"] = args[0];
        return geojson;
    }
};

/**
 * @brief ST_POLYGON([[[x1,y1], [x2,y2], ...]]) - Create a Polygon geometry
 */
class StPolygonFunction : public IFunction {
public:
    ~StPolygonFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_POLYGON",
            "Geo",
            "Create a Polygon geometry from array of rings (first is exterior, rest are holes)",
            {
                {"rings", ArgType::ARRAY, true, nullptr, "Array of linear rings"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_POLYGON([[[0,0], [1,0], [1,1], [0,1], [0,0]]])"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        nlohmann::json geojson;
        geojson["type"] = "Polygon";
        geojson["coordinates"] = args[0];
        return geojson;
    }
};

/**
 * @brief ST_GEOMFROMTEXT(wkt) - Parse WKT to geometry
 */
class StGeomFromTextFunction : public IFunction {
public:
    ~StGeomFromTextFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_GEOMFROMTEXT",
            "Geo",
            "Parse Well-Known Text (WKT) to GeoJSON geometry",
            {
                {"wkt", ArgType::STRING, true, nullptr, "WKT string (e.g., 'POINT(1 2)')"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_GEOMFROMTEXT('POINT(1 2)')", "ST_GEOMFROMTEXT('LINESTRING(0 0, 1 1)')"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::string wkt = args[0].get<std::string>();
        
        // Trim whitespace
        auto trim = [](std::string s) {
            s.erase(0, s.find_first_not_of(" \t\n\r"));
            s.erase(s.find_last_not_of(" \t\n\r") + 1);
            return s;
        };
        wkt = trim(wkt);
        
        // Uppercase for keyword matching
        std::string wktUpper = wkt;
        std::transform(wktUpper.begin(), wktUpper.end(), wktUpper.begin(), ::toupper);
        
        nlohmann::json geojson;
        
        // Parse POINT
        if (wktUpper.find("POINT") == 0) {
            size_t start = wkt.find('(');
            size_t end = wkt.find(')');
            if (start == std::string::npos || end == std::string::npos) {
                throw std::runtime_error("Invalid WKT POINT syntax");
            }
            
            std::string coords = wkt.substr(start + 1, end - start - 1);
            std::istringstream iss(coords);
            double x, y, z;
            
            if (!(iss >> x >> y)) {
                throw std::runtime_error("Invalid POINT coordinates");
            }
            
            geojson["type"] = "Point";
            if (iss >> z) {
                geojson["coordinates"] = {x, y, z};
            } else {
                geojson["coordinates"] = {x, y};
            }
            return geojson;
        }
        
        // Parse LINESTRING
        if (wktUpper.find("LINESTRING") == 0) {
            size_t start = wkt.find('(');
            size_t end = wkt.find(')');
            if (start == std::string::npos || end == std::string::npos) {
                throw std::runtime_error("Invalid WKT LINESTRING syntax");
            }
            
            std::string coordsStr = wkt.substr(start + 1, end - start - 1);
            nlohmann::json coordinates = nlohmann::json::array();
            
            std::istringstream iss(coordsStr);
            std::string pointStr = {};
            while (std::getline(iss, pointStr, ',')) {
                pointStr = trim(pointStr);
                std::istringstream pss(pointStr);
                double x, y;
                if (pss >> x >> y) {
                    coordinates.push_back({x, y});
                }
            }
            
            geojson["type"] = "LineString";
            geojson["coordinates"] = coordinates;
            return geojson;
        }
        
        // Parse POLYGON (simplified)
        if (wktUpper.find("POLYGON") == 0) {
            size_t start = wkt.find("((");
            size_t end = wkt.rfind("))");
            if (start == std::string::npos || end == std::string::npos) {
                throw std::runtime_error("Invalid WKT POLYGON syntax");
            }
            
            std::string ringStr = wkt.substr(start + 2, end - start - 2);
            nlohmann::json ring = nlohmann::json::array();
            
            std::istringstream iss(ringStr);
            std::string pointStr = {};
            while (std::getline(iss, pointStr, ',')) {
                pointStr = trim(pointStr);
                std::istringstream pss(pointStr);
                double x, y;
                if (pss >> x >> y) {
                    ring.push_back({x, y});
                }
            }
            
            geojson["type"] = "Polygon";
            geojson["coordinates"] = nlohmann::json::array({ring});
            return geojson;
        }
        
        throw std::runtime_error("Unsupported WKT geometry type");
    }
};

/**
 * @brief ST_GEOMFROMGEOJSON(json) - Parse GeoJSON string or object
 */
class StGeomFromGeoJSONFunction : public IFunction {
public:
    ~StGeomFromGeoJSONFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_GEOMFROMGEOJSON",
            "Geo",
            "Parse GeoJSON string or return GeoJSON object as-is",
            {
                {"geojson", ArgType::ANY, true, nullptr, "GeoJSON string or object"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_GEOMFROMGEOJSON('{\"type\":\"Point\",\"coordinates\":[1,2]}')"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& arg = args[0];
        
        // Already a GeoJSON object
        if (arg.is_object() && arg.contains("type") && arg.contains("coordinates")) {
            return arg;
        }
        
        // Parse string
        if (arg.is_string()) {
            nlohmann::json geojson = nlohmann::json::parse(arg.get<std::string>());
            if (!geojson.is_object() || !geojson.contains("type") || !geojson.contains("coordinates")) {
                throw std::runtime_error("Invalid GeoJSON");
            }
            return geojson;
        }
        
        throw std::runtime_error("Expected GeoJSON object or string");
    }
};

// ============================================================================
// Measurement Functions
// ============================================================================

/**
 * @brief ST_DISTANCE(geom1, geom2) - Distance between two geometries
 */
class StDistanceFunction : public IFunction {
public:
    ~StDistanceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_DISTANCE",
            "Geo",
            "Calculate distance between two geometries (meters for geographic, units for projected)",
            {
                {"geom1", ArgType::GEOMETRY, true, nullptr, "First geometry"},
                {"geom2", ArgType::GEOMETRY, true, nullptr, "Second geometry"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"ST_DISTANCE(ST_POINT(0,0), ST_POINT(1,1))"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto [x1, y1, z1] = geo_helpers::extractPoint(args[0]);
        auto [x2, y2, z2] = geo_helpers::extractPoint(args[1]);
        
        // Check if both points have Z coordinates
        bool has_z1 = args[0]["coordinates"].size() >= 3;
        bool has_z2 = args[1]["coordinates"].size() >= 3;
        bool use_3d = has_z1 && has_z2;
        
        // Use haversine for geographic coordinates (2D only)
        if (!use_3d && geo_helpers::looksLikeDegrees(x1, y1) && geo_helpers::looksLikeDegrees(x2, y2)) {
            return geo_helpers::haversineDistance(x1, y1, x2, y2);
        }
        
        // Euclidean for projected coordinates or 3D
        if (use_3d) {
            return geo_helpers::euclideanDistance3D(x1, y1, z1, x2, y2, z2);
        }
        return geo_helpers::euclideanDistance(x1, y1, x2, y2);
    }
};

/**
 * @brief GEO_DISTANCE(geom1, geom2) - ArangoDB-style distance function
 */
class GeoDistanceFunction : public IFunction {
public:
    ~GeoDistanceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "GEO_DISTANCE",
            "Geo",
            "Calculate great-circle distance in meters (ArangoDB compatible)",
            {
                {"geom1", ArgType::GEOMETRY, true, nullptr, "First geometry"},
                {"geom2", ArgType::GEOMETRY, true, nullptr, "Second geometry"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"GEO_DISTANCE(point1, point2)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto [x1, y1, z1] = geo_helpers::extractPoint(args[0]);
        auto [x2, y2, z2] = geo_helpers::extractPoint(args[1]);
        return geo_helpers::haversineDistance(x1, y1, x2, y2);
    }
};

/**
 * @brief ST_LENGTH(linestring) - Length of a LineString
 */
class StLengthFunction : public IFunction {
public:
    ~StLengthFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_LENGTH",
            "Geo",
            "Calculate length of a LineString (meters for geographic)",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "LineString geometry"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"ST_LENGTH(linestring)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& geom = args[0];
        if (!geom.is_object() || geom["type"] != "LineString") {
            throw std::runtime_error("ST_LENGTH requires LineString geometry");
        }
        
        const auto& coords = geom["coordinates"];
        double totalLength = 0.0;
        
        for (size_t i = 1; i < coords.size(); ++i) {
            double x1 = coords[i-1][0].get<double>();
            double y1 = coords[i-1][1].get<double>();
            double x2 = coords[i][0].get<double>();
            double y2 = coords[i][1].get<double>();
            
            if (geo_helpers::looksLikeDegrees(x1, y1) && geo_helpers::looksLikeDegrees(x2, y2)) {
                totalLength += geo_helpers::haversineDistance(x1, y1, x2, y2);
            } else {
                totalLength += geo_helpers::euclideanDistance(x1, y1, x2, y2);
            }
        }
        
        return totalLength;
    }
};

/**
 * @brief ST_AREA(polygon) - Area of a Polygon (simplified Shoelace formula)
 */
class StAreaFunction : public IFunction {
public:
    ~StAreaFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_AREA",
            "Geo",
            "Calculate area of a Polygon (square units)",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "Polygon geometry"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"ST_AREA(polygon)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& geom = args[0];
        if (!geom.is_object() || geom["type"] != "Polygon") {
            throw std::runtime_error("ST_AREA requires Polygon geometry");
        }
        
        // Use Shoelace formula for exterior ring
        const auto& ring = geom["coordinates"][0];
        double area = 0.0;
        size_t n = ring.size();
        
        for (size_t i = 0; i < n; ++i) {
            size_t j = (i + 1) % n;
            double xi = ring[i][0].get<double>();
            double yi = ring[i][1].get<double>();
            double xj = ring[j][0].get<double>();
            double yj = ring[j][1].get<double>();
            area += xi * yj - xj * yi;
        }
        
        return std::abs(area) / 2.0;
    }
};

// ============================================================================
// Predicate Functions
// ============================================================================

/**
 * @brief ST_INTERSECTS(geom1, geom2) - Test if geometries intersect
 */
class StIntersectsFunction : public IFunction {
public:
    ~StIntersectsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_INTERSECTS",
            "Geo",
            "Test if two geometries spatially intersect",
            {
                {"geom1", ArgType::GEOMETRY, true, nullptr, "First geometry"},
                {"geom2", ArgType::GEOMETRY, true, nullptr, "Second geometry"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"ST_INTERSECTS(polygon, point)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto mbr1 = geo_helpers::extractMBR(args[0]);
        auto mbr2 = geo_helpers::extractMBR(args[1]);
        return mbr1.intersects(mbr2);
    }
};

/**
 * @brief ST_CONTAINS(geom1, geom2) - Test if geom1 contains geom2
 */
class StContainsFunction : public IFunction {
public:
    ~StContainsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_CONTAINS",
            "Geo",
            "Test if first geometry completely contains second geometry",
            {
                {"geom1", ArgType::GEOMETRY, true, nullptr, "Container geometry"},
                {"geom2", ArgType::GEOMETRY, true, nullptr, "Contained geometry"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"ST_CONTAINS(polygon, point)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto mbr1 = geo_helpers::extractMBR(args[0]);
        auto mbr2 = geo_helpers::extractMBR(args[1]);
        return mbr1.containsMBR(mbr2);
    }
};

/**
 * @brief ST_WITHIN(geom1, geom2) - Test if geom1 is within geom2
 */
class StWithinFunction : public IFunction {
public:
    ~StWithinFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_WITHIN",
            "Geo",
            "Test if first geometry is completely within second geometry",
            {
                {"geom1", ArgType::GEOMETRY, true, nullptr, "Inner geometry"},
                {"geom2", ArgType::GEOMETRY, true, nullptr, "Outer geometry"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"ST_WITHIN(point, polygon)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto mbr1 = geo_helpers::extractMBR(args[0]);
        auto mbr2 = geo_helpers::extractMBR(args[1]);
        return mbr2.containsMBR(mbr1);
    }
};

/**
 * @brief ST_DWITHIN(geom1, geom2, distance) - Test if within distance
 */
class StDWithinFunction : public IFunction {
public:
    ~StDWithinFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_DWITHIN",
            "Geo",
            "Test if two geometries are within specified distance",
            {
                {"geom1", ArgType::GEOMETRY, true, nullptr, "First geometry"},
                {"geom2", ArgType::GEOMETRY, true, nullptr, "Second geometry"},
                {"distance", ArgType::NUMBER, true, nullptr, "Maximum distance"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"ST_DWITHIN(point1, point2, 1000)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto [x1, y1, z1] = geo_helpers::extractPoint(args[0]);
        auto [x2, y2, z2] = geo_helpers::extractPoint(args[1]);
        double maxDistance = args[2].get<double>();
        
        // Check if both points have Z coordinates
        bool has_z1 = args[0]["coordinates"].size() >= 3;
        bool has_z2 = args[1]["coordinates"].size() >= 3;
        bool use_3d = has_z1 && has_z2;
        
        double distance = 0;
        if (!use_3d && geo_helpers::looksLikeDegrees(x1, y1) && geo_helpers::looksLikeDegrees(x2, y2)) {
            distance = geo_helpers::haversineDistance(x1, y1, x2, y2);
        } else if (use_3d) {
            distance = geo_helpers::euclideanDistance3D(x1, y1, z1, x2, y2, z2);
        } else {
            distance = geo_helpers::euclideanDistance(x1, y1, x2, y2);
        }
        
        return distance <= maxDistance;
    }
};

/**
 * @brief GEO_CONTAINS(polygon, point) - ArangoDB-style contains check
 */
class GeoContainsFunction : public IFunction {
public:
    ~GeoContainsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "GEO_CONTAINS",
            "Geo",
            "Test if polygon contains point (ArangoDB compatible)",
            {
                {"polygon", ArgType::GEOMETRY, true, nullptr, "Polygon geometry"},
                {"point", ArgType::GEOMETRY, true, nullptr, "Point geometry"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"GEO_CONTAINS(polygon, point)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto mbr = geo_helpers::extractMBR(args[0]);
        auto [x, y, z] = geo_helpers::extractPoint(args[1]);
        return mbr.contains(x, y);
    }
};

// ============================================================================
// Accessor Functions
// ============================================================================

/**
 * @brief ST_X(point) - Get X coordinate
 */
class StXFunction : public IFunction {
public:
    ~StXFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_X",
            "Geo",
            "Extract X coordinate (longitude) from Point",
            {
                {"point", ArgType::GEOMETRY, true, nullptr, "Point geometry"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"ST_X(point)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto [x, y, z] = geo_helpers::extractPoint(args[0]);
        return x;
    }
};

/**
 * @brief ST_Y(point) - Get Y coordinate
 */
class StYFunction : public IFunction {
public:
    ~StYFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_Y",
            "Geo",
            "Extract Y coordinate (latitude) from Point",
            {
                {"point", ArgType::GEOMETRY, true, nullptr, "Point geometry"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"ST_Y(point)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto [x, y, z] = geo_helpers::extractPoint(args[0]);
        return y;
    }
};

/**
 * @brief ST_Z(point) - Get Z coordinate
 */
class StZFunction : public IFunction {
public:
    ~StZFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_Z",
            "Geo",
            "Extract Z coordinate (elevation) from Point, or null if 2D",
            {
                {"point", ArgType::GEOMETRY, true, nullptr, "Point geometry"}
            },
            ArgType::NULLABLE,
            true,
            false,
            {"ST_Z(point3d)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& geom = args[0];
        if (geom.is_object() && geom["type"] == "Point" && 
            geom["coordinates"].size() >= 3) {
            return geom["coordinates"][2];
        }
        return nullptr;
    }
};

/**
 * @brief ST_HASZ(geom) - Check if geometry has Z coordinate
 */
class StHasZFunction : public IFunction {
public:
    ~StHasZFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_HASZ",
            "Geo",
            "Check if geometry has Z coordinates",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "Any geometry"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"ST_HASZ(point3d)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& geom = args[0];
        if (!geom.is_object() || !geom.contains("coordinates")) {
            return false;
        }
        
        std::string type = geom["type"];
        const auto& coords = geom["coordinates"];
        
        if (type == "Point") {
            return coords.size() >= 3;
        }
        if (type == "LineString" || type == "MultiPoint") {
            return !coords.empty() && coords[0].size() >= 3;
        }
        if (type == "Polygon") {
            return !coords.empty() && !coords[0].empty() && coords[0][0].size() >= 3;
        }
        
        return false;
    }
};

/**
 * @brief ST_ASGEOJSON(geom) - Convert geometry to GeoJSON string
 */
class StAsGeoJSONFunction : public IFunction {
public:
    ~StAsGeoJSONFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_ASGEOJSON",
            "Geo",
            "Convert geometry to GeoJSON string",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "Any geometry"}
            },
            ArgType::STRING,
            true,
            false,
            {"ST_ASGEOJSON(point)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        return args[0].dump();
    }
};

/**
 * @brief ST_ASTEXT(geom) - Convert geometry to WKT string
 */
class StAsTextFunction : public IFunction {
public:
    ~StAsTextFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_ASTEXT",
            "Geo",
            "Convert geometry to Well-Known Text (WKT) string",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "Any geometry"}
            },
            ArgType::STRING,
            true,
            false,
            {"ST_ASTEXT(point)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& geom = args[0];
        std::string type = geom["type"];
        const auto& coords = geom["coordinates"];
        
        if (type == "Point") {
            std::ostringstream oss = {};
            oss << "POINT(" << coords[0].get<double>() << " " << coords[1].get<double>();
            if (coords.size() >= 3) {
                oss << " " << coords[2].get<double>();
            }
            oss << ")";
            return oss.str();
        }
        
        if (type == "LineString") {
            std::ostringstream oss = {};
            oss << "LINESTRING(";
            for (size_t i = 0; i < coords.size(); ++i) {
                if (i > 0) {
                  oss << ", ";
                }
                oss << coords[i][0].get<double>() << " " << coords[i][1].get<double>();
            }
            oss << ")";
            return oss.str();
        }
        
        if (type == "Polygon") {
            std::ostringstream oss = {};
            oss << "POLYGON((";
            const auto& ring = coords[0];
            for (size_t i = 0; i < ring.size(); ++i) {
                if (i > 0) {
                  oss << ", ";
                }
                oss << ring[i][0].get<double>() << " " << ring[i][1].get<double>();
            }
            oss << "))";
            return oss.str();
        }
        
        throw std::runtime_error("ST_ASTEXT: Unsupported geometry type: " + type);
    }
};

// ============================================================================
// Processing Functions
// ============================================================================

/**
 * @brief ST_CENTROID(geom) - Calculate centroid of geometry
 */
class StCentroidFunction : public IFunction {
public:
    ~StCentroidFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_CENTROID",
            "Geo",
            "Calculate centroid (center of mass) of geometry",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "Any geometry"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_CENTROID(polygon)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& geom = args[0];
        std::string type = geom["type"];
        
        if (type == "Point") {
            return geom;
        }
        
        // Calculate average of all coordinates including Z if present
        double sumX = 0.0, sumY = 0.0, sumZ = 0.0;
        size_t count = 0;
        bool hasZ = false;
        
        auto addCoords = [&](const nlohmann::json& coordList) {
            for (const auto& coord : coordList) {
                sumX += coord[0].get<double>();
                sumY += coord[1].get<double>();
                if (coord.size() >= 3) {
                    sumZ += coord[2].get<double>();
                    hasZ = true;
                }
                count++;
            }
        };
        
        if (type == "LineString" || type == "MultiPoint") {
            addCoords(geom["coordinates"]);
        } else if (type == "Polygon") {
            addCoords(geom["coordinates"][0]); // Exterior ring only
        }
        
        if (count == 0) {
            throw std::runtime_error("Cannot calculate centroid of empty geometry");
        }
        
        nlohmann::json centroid;
        centroid["type"] = "Point";
        if (hasZ) {
            centroid["coordinates"] = {sumX / count, sumY / count, sumZ / count};
        } else {
            centroid["coordinates"] = {sumX / count, sumY / count};
        }
        return centroid;
    }
};

/**
 * @brief ST_ENVELOPE(geom) - Calculate bounding box as Polygon
 */
class StEnvelopeFunction : public IFunction {
public:
    ~StEnvelopeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_ENVELOPE",
            "Geo",
            "Calculate minimum bounding rectangle as Polygon",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "Any geometry"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_ENVELOPE(linestring)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        auto mbr = geo_helpers::extractMBR(args[0]);
        
        nlohmann::json envelope;
        envelope["type"] = "Polygon";
        envelope["coordinates"] = nlohmann::json::array({
            nlohmann::json::array({
                nlohmann::json::array({mbr.minx, mbr.miny}),
                nlohmann::json::array({mbr.maxx, mbr.miny}),
                nlohmann::json::array({mbr.maxx, mbr.maxy}),
                nlohmann::json::array({mbr.minx, mbr.maxy}),
                nlohmann::json::array({mbr.minx, mbr.miny})
            })
        });
        
        return envelope;
    }
};

/**
 * @brief ST_BUFFER(geom, distance_m [, arc_points]) - Expand a geometry by a
 * fixed geodesic distance.
 *
 * Returns a GeoJSON Polygon that approximates the input geometry expanded
 * outward by `distance_m` metres.  Converts metres to degrees using the
 * latitude of the geometry's centroid for geodesic accuracy at scales up to
 * ~100 km.
 *
 * Supported input types:
 *   - Point   → circular polygon with `arc_points` vertices (default 36).
 *   - Polygon → outward ring expansion via edge-shift method.
 *
 * Returns an empty GeometryCollection for unsupported geometry types or when
 * `distance_m` ≤ 0.
 *
 * Uses the CPU-exact spatial backend.
 */
class StBufferFunction : public IFunction {
public:
    ~StBufferFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_BUFFER",
            "Geo",
            "Expand a geometry by a fixed geodesic distance in metres",
            {
                {"geometry",   ArgType::GEOMETRY, true,  nullptr, "Input geometry (Point or Polygon)"},
                {"distance_m", ArgType::NUMBER,   true,  nullptr, "Buffer distance in metres (must be > 0)"},
                {"arc_points", ArgType::NUMBER,   false, 36,      "Vertices per arc for circular approximation (default 36, min 3)"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_BUFFER(point, 500)", "ST_BUFFER(polygon, 1000)", "ST_BUFFER(point, 500, 64)"}
        };
    }

    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        using namespace themis::geo;
        const GeometryInfo geom = EWKBParser::parseGeoJSON(args[0].dump());
        const double distance_m = args[1].get<double>();
        // Clamp before narrowing to avoid undefined behaviour on extreme doubles.
        const int arc_points = (args.size() >= 3 && args[2].is_number())
                               ? static_cast<int>(std::clamp(args[2].get<double>(), 3.0, 360.0))
                               : 36;
        const GeometryInfo result = getCpuExactBackend()->stBuffer(geom, distance_m, arc_points);
        const std::string json_str = EWKBParser::toGeoJSON(result);
        if (json_str == "{}" || json_str.empty() || json_str == "null") {
            nlohmann::json empty;
            empty["type"] = "GeometryCollection";
            empty["geometries"] = nlohmann::json::array();
            return empty;
        }
        return nlohmann::json::parse(json_str);
    }
};

// ============================================================================
// Registration Function
// ============================================================================

/**
 * @brief ST_UNION(geom1, geom2) - Compute the geometric union of two geometries.
 *
 * Returns a GeoJSON geometry that contains all points from either input.
 * Non-overlapping polygons produce a GeometryCollection; overlapping polygons
 * are merged into a single Polygon.  Uses the CPU-exact spatial backend.
 */
class StUnionFunction : public IFunction {
public:
    ~StUnionFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_UNION",
            "Geo",
            "Compute the geometric union of two geometries",
            {
                {"geom1", ArgType::GEOMETRY, true, nullptr, "First geometry"},
                {"geom2", ArgType::GEOMETRY, true, nullptr, "Second geometry"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_UNION(polygon_a, polygon_b)"}
        };
    }

    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        using namespace themis::geo;
        const GeometryInfo g1 = EWKBParser::parseGeoJSON(args[0].dump());
        const GeometryInfo g2 = EWKBParser::parseGeoJSON(args[1].dump());
        const GeometryInfo result = getCpuExactBackend()->stUnion(g1, g2);
        const std::string json_str = EWKBParser::toGeoJSON(result);
        if (json_str == "{}" || json_str.empty() || json_str == "null") {
            nlohmann::json empty;
            empty["type"] = "GeometryCollection";
            empty["geometries"] = nlohmann::json::array();
            return empty;
        }
        return nlohmann::json::parse(json_str);
    }
};

/**
 * @brief ST_DIFFERENCE(geom1, geom2) - Compute the set-difference geom1 \ geom2.
 *
 * Returns the part of geom1 that is not in geom2.  Returns an empty
 * GeometryCollection when geom1 is fully contained in geom2.  Uses the
 * CPU-exact spatial backend.
 */
class StDifferenceFunction : public IFunction {
public:
    ~StDifferenceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_DIFFERENCE",
            "Geo",
            "Compute the set-difference of two geometries (geom1 minus geom2)",
            {
                {"geom1", ArgType::GEOMETRY, true, nullptr, "First geometry (minuend)"},
                {"geom2", ArgType::GEOMETRY, true, nullptr, "Second geometry (subtrahend)"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_DIFFERENCE(polygon_a, polygon_b)"}
        };
    }

    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        using namespace themis::geo;
        const GeometryInfo g1 = EWKBParser::parseGeoJSON(args[0].dump());
        const GeometryInfo g2 = EWKBParser::parseGeoJSON(args[1].dump());
        const GeometryInfo result = getCpuExactBackend()->stDifference(g1, g2);
        const std::string json_str = EWKBParser::toGeoJSON(result);
        if (json_str == "{}" || json_str.empty() || json_str == "null") {
            // Empty difference — geom1 is fully contained in geom2.
            nlohmann::json empty;
            empty["type"] = "GeometryCollection";
            empty["geometries"] = nlohmann::json::array();
            return empty;
        }
        return nlohmann::json::parse(json_str);
    }
};

/**
 * @brief Register all Geo functions with the registry
 */
inline void registerGeoFunctions(FunctionRegistry& registry) {
    // Construction
    registry.registerFunction(std::make_unique<StPointFunction>());
    registry.registerFunction(std::make_unique<StLinestringFunction>());
    registry.registerFunction(std::make_unique<StPolygonFunction>());
    registry.registerFunction(std::make_unique<StGeomFromTextFunction>());
    registry.registerFunction(std::make_unique<StGeomFromGeoJSONFunction>());
    
    // Measurement
    registry.registerFunction(std::make_unique<StDistanceFunction>());
    registry.registerFunction(std::make_unique<GeoDistanceFunction>());
    registry.registerFunction(std::make_unique<StLengthFunction>());
    registry.registerFunction(std::make_unique<StAreaFunction>());
    
    // Predicates
    registry.registerFunction(std::make_unique<StIntersectsFunction>());
    registry.registerFunction(std::make_unique<StContainsFunction>());
    registry.registerFunction(std::make_unique<StWithinFunction>());
    registry.registerFunction(std::make_unique<StDWithinFunction>());
    registry.registerFunction(std::make_unique<GeoContainsFunction>());
    
    // Accessors
    registry.registerFunction(std::make_unique<StXFunction>());
    registry.registerFunction(std::make_unique<StYFunction>());
    registry.registerFunction(std::make_unique<StZFunction>());
    registry.registerFunction(std::make_unique<StHasZFunction>());
    registry.registerFunction(std::make_unique<StAsGeoJSONFunction>());
    registry.registerFunction(std::make_unique<StAsTextFunction>());
    
    // Processing
    registry.registerFunction(std::make_unique<StCentroidFunction>());
    registry.registerFunction(std::make_unique<StEnvelopeFunction>());
    registry.registerFunction(std::make_unique<StBufferFunction>());
    // ArangoDB-compatible alias: GEO_BUFFER maps to the same geodesic ST_BUFFER backend.
    registry.registerAlias("GEO_BUFFER", "ST_BUFFER");
    registry.registerFunction(std::make_unique<StUnionFunction>());
    registry.registerFunction(std::make_unique<StDifferenceFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis

