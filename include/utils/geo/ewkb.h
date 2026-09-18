/**
 * @file ewkb.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace geo {

// WKB Geometry Types (PostGIS Extended)
enum class GeometryType : uint32_t {
    Point = 1,
    LineString = 2,
    Polygon = 3,
    MultiPoint = 4,
    MultiLineString = 5,
    MultiPolygon = 6,
    GeometryCollection = 7,
    
    // 3D variants (EWKB Z flag: type | 0x80000000)
    PointZ = 0x80000001,
    LineStringZ = 0x80000002,
    PolygonZ = 0x80000003,
    MultiPointZ = 0x80000004,
    MultiLineStringZ = 0x80000005,
    MultiPolygonZ = 0x80000006,
    GeometryCollectionZ = 0x80000007
};

// 2D/3D Coordinate
struct Coordinate {
    double x = 0;
    double y = {};
    std::optional<double> z;  // For 3D geometries
    
    Coordinate() : x(0.0), y(0.0) {}
    Coordinate(double x_, double y_) : x(x_), y(y_) {}
    Coordinate(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    
    bool hasZ() const { return z.has_value(); }
    double getZ() const { return z.value_or(0.0); }
};

// Minimum Bounding Rectangle (2D + optional Z)
struct MBR {
    double minx = 0.0;
    double miny = 0.0;
    double maxx = 0.0;
    double maxy = 0.0;
    
    std::optional<double> z_min;
    std::optional<double> z_max;
    
    MBR() = default;
    MBR(double minx_, double miny_, double maxx_, double maxy_)
        : minx(minx_), miny(miny_), maxx(maxx_), maxy(maxy_) {}
    
    // Check if this MBR intersects with another
    bool intersects(const MBR& other) const {
        return !(minx > other.maxx || maxx < other.minx ||
                 miny > other.maxy || maxy < other.miny);
    }
    
    // Check if this MBR contains a point
    bool contains(double x, double y) const {
        return x >= minx && x <= maxx && y >= miny && y <= maxy;
    }
    
    /**
     * @brief Expand MBR by distance (meters, approximate for lat/lon)
     * @param[in] distance_meters Input parameter.
     * @return Return value.
     */
    MBR expand(double distance_meters) const;
    
    // Area in square degrees (approximate)
    double area() const {
        return (maxx - minx) * (maxy - miny);
    }
    
    // Center point
    Coordinate center() const {
        return Coordinate((minx + maxx) / 2.0, (miny + maxy) / 2.0);
    }
    
    bool hasZ() const { return z_min.has_value() && z_max.has_value(); }
};

// Geometry metadata (sidecar for fast filtering)
struct GeoSidecar {
    MBR mbr;              // 2D bounding box
    Coordinate centroid;  // Geometric center
    double z_min = 0.0;   // Min elevation (for 3D)
    double z_max = 0.0;   // Max elevation (for 3D)
    
    GeoSidecar() = default;
    explicit GeoSidecar(const MBR& mbr_) : mbr(mbr_), centroid(mbr_.center()) {}
};

// Parsed geometry information
struct GeometryInfo {
    GeometryType type;
    int srid = 4326;  // Default: WGS84
    bool has_z = false;
    bool has_m = false;  // Measure (not supported yet)
    
    std::vector<Coordinate> coords;  // Point: 1 coord, LineString: N coords, Polygon: N coords (ring)
    
    // For complex types (MultiPoint, Polygon with holes, etc.)
    std::vector<std::vector<Coordinate>> rings;  // Polygon rings
    std::vector<GeometryInfo> geometries;        // GeometryCollection
    
    GeometryInfo() = default;
    GeometryInfo(GeometryType type_) : type(type_) {}
    
    bool isPoint() const { return type == GeometryType::Point || type == GeometryType::PointZ; }
    bool isLineString() const { return type == GeometryType::LineString || type == GeometryType::LineStringZ; }
    bool isPolygon() const { return type == GeometryType::Polygon || type == GeometryType::PolygonZ; }
    bool isMultiPolygon() const { return type == GeometryType::MultiPolygon || type == GeometryType::MultiPolygonZ; }
    bool isGeometryCollection() const { return type == GeometryType::GeometryCollection || type == GeometryType::GeometryCollectionZ; }
    bool hasZ() const { return has_z; }
    
    /**
     * @brief Compute MBR from coordinates
     * @return Return value.
     */
    MBR computeMBR() const;
    
    /**
     * @brief Compute centroid
     * @return Return value.
     */
    Coordinate computeCentroid() const;
};

// EWKB Parser/Serializer
/** @brief EWKB Parser/Serializer. */
class EWKBParser {
public:
    /**
     * @brief Parse EWKB binary to GeometryInfo
     * @param[in] ewkb Input parameter.
     * @return Return value.
     */
    static GeometryInfo parse(const std::vector<uint8_t>& ewkb);
    
    /**
     * @brief Serialize GeometryInfo to EWKB binary
     * @param[in] geom Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> serialize(const GeometryInfo& geom);
    
    /**
     * @brief Parse from WKT (Well-Known Text)
     * @param[in] wkt Input parameter.
     * @return Return value.
     */
    static GeometryInfo parseWKT(const std::string& wkt);
    
    /**
     * @brief Parse from GeoJSON geometry object
     * @param[in] geojson Input parameter.
     * @return Return value.
     */
    static GeometryInfo parseGeoJSON(const std::string& geojson);
    
    /**
     * @brief Serialize to GeoJSON geometry object
     * @param[in] geom Input parameter.
     * @return Return value.
     */
    static std::string toGeoJSON(const GeometryInfo& geom);
    
    /**
     * @brief Serialize to WKT
     * @param[in] geom Input parameter.
     * @return Return value.
     */
    static std::string toWKT(const GeometryInfo& geom);
    
    /**
     * @brief Compute sidecar metadata from geometry
     * @param[in] geom Input parameter.
     * @return Return value.
     */
    static GeoSidecar computeSidecar(const GeometryInfo& geom);
    
    /**
     * @brief Validate EWKB format
     * @param[in] ewkb Input parameter.
     * @return True on success.
     */
    static bool validate(const std::vector<uint8_t>& ewkb);
    
private:
    /**
     * @brief Internal parsing helpers
     * @param[in] ptr Input parameter.
     * @param[in] has_z Input parameter.
     * @param[in] is_little_endian Input parameter.
     * @return Return value.
     */
    static GeometryInfo parsePoint(const uint8_t*& ptr, bool has_z, bool is_little_endian);
    /**
     * @brief TBD: Describe parseLineString.
     * @param[in] ptr Input parameter.
     * @param[in] has_z Input parameter.
     * @param[in] is_little_endian Input parameter.
     * @return Return value.
     */
    static GeometryInfo parseLineString(const uint8_t*& ptr, bool has_z, bool is_little_endian);
    /**
     * @brief TBD: Describe parsePolygon.
     * @param[in] ptr Input parameter.
     * @param[in] has_z Input parameter.
     * @param[in] is_little_endian Input parameter.
     * @return Return value.
     */
    static GeometryInfo parsePolygon(const uint8_t*& ptr, bool has_z, bool is_little_endian);
    /**
     * @brief Recursive helper: parse one EWKB geometry starting at ptr (reads its own byte-order marker)
     * @param[in] ptr Input parameter.
     * @return Return value.
     */
    static GeometryInfo parseGeometryFromPtr(const uint8_t*& ptr);
    /**
     * @brief Recursive helper: serialize one geometry into buf
     * @param[in,out] buf Input/output parameter.
     * @param[in] geom Input parameter.
     * @param[in] is_little_endian Input parameter.
     */
    static void serializeGeometryInto(std::vector<uint8_t>& buf, const GeometryInfo& geom, bool is_little_endian);
    
    /**
     * @brief Binary read helpers
     * @param[in] ptr Input parameter.
     * @param[in] is_little_endian Input parameter.
     * @return Return value.
     */
    static double readDouble(const uint8_t*& ptr, bool is_little_endian);
    /**
     * @brief TBD: Describe readUInt32.
     * @param[in] ptr Input parameter.
     * @param[in] is_little_endian Input parameter.
     * @return Return value.
     */
    static uint32_t readUInt32(const uint8_t*& ptr, bool is_little_endian);
    /**
     * @brief TBD: Describe writeDouble.
     * @param[in,out] buf Input/output parameter.
     * @param[in] val Input parameter.
     * @param[in] is_little_endian Input parameter.
     */
    static void writeDouble(std::vector<uint8_t>& buf, double val, bool is_little_endian);
    /**
     * @brief TBD: Describe writeUInt32.
     * @param[in,out] buf Input/output parameter.
     * @param[in] val Input parameter.
     * @param[in] is_little_endian Input parameter.
     */
    static void writeUInt32(std::vector<uint8_t>& buf, uint32_t val, bool is_little_endian);
};

}  // namespace geo
}  // namespace themis
