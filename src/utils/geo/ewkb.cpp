/**
 * @file ewkb.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=39, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/geo/ewkb.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace themis {
namespace geo {

using json = nlohmann::json;

// Constants
constexpr double EARTH_RADIUS_METERS = 6371000.0;  // Mean Earth radius
constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double METERS_PER_DEGREE_APPROX = 111320.0;  // At equator

static std::string trimCopy(const std::string& input) {
    const auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    if (first == input.end()) {
        return {};
    }

    const auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    return std::string(first, last);
}

static std::vector<std::string> splitTopLevel(const std::string& input, char delimiter) {
    std::vector<std::string> parts;
    std::string current = {};
    int depth = 0;
    for (char c : input) {
        if (c == '(') {
            depth++;
            current.push_back(c);
            continue;
        }
        if (c == ')') {
            depth--;
            current.push_back(c);
            continue;
        }
        if (c == delimiter && depth == 0) {
            parts.push_back(trimCopy(current));
            current.clear();
            continue;
        }
        current.push_back(c);
    }
    if (!current.empty()) {
        parts.push_back(trimCopy(current));
    }
    return parts;
}

static Coordinate parseCoordinateToken(const std::string& token) {
    std::istringstream iss(token);
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!(iss >> x >> y)) {
        throw std::runtime_error("WKT: Invalid coordinate token: " + token);
    }
    if (iss >> z) {
        return Coordinate(x, y, z);
    }
    return Coordinate(x, y);
}

static std::string extractWktBody(const std::string& wkt) {
    const auto open = wkt.find('(');
    const auto close = wkt.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close <= open) {
        throw std::runtime_error("WKT: Missing coordinate body");
    }
    return trimCopy(wkt.substr(open + 1, close - open - 1));
}

// MBR expand by distance (approximate for lat/lon)
MBR MBR::expand([[maybe_unused]] double distance_meters) const {
    double delta_deg = distance_meters / METERS_PER_DEGREE_APPROX;
    return MBR(
        minx - delta_deg,
        miny - delta_deg,
        maxx + delta_deg,
        maxy + delta_deg
    );
}

// GeometryInfo: Compute MBR
MBR GeometryInfo::computeMBR() const {
    if (coords.empty() && rings.empty() && geometries.empty()) {
        return MBR();
    }
    
    // For multi-geometry types, union MBRs of sub-geometries
    if (!geometries.empty() && coords.empty() && rings.empty()) {
        MBR result = geometries[0].computeMBR();
        for (size_t i = 1; i < geometries.size(); ++i) {
            MBR sub = geometries[i].computeMBR();
            result.minx = std::min(result.minx, sub.minx);
            result.maxx = std::max(result.maxx, sub.maxx);
            result.miny = std::min(result.miny, sub.miny);
            result.maxy = std::max(result.maxy, sub.maxy);
            if (sub.z_min) {
                if (!result.z_min || *sub.z_min < *result.z_min) {
                  result.z_min = sub.z_min;
                }
                if (!result.z_max || *sub.z_max > *result.z_max) {
                  result.z_max = sub.z_max;
                }
            }
        }
        return result;
    }

    MBR mbr;
    mbr.minx = mbr.maxx = coords.empty() ? rings[0][0].x : coords[0].x;
    mbr.miny = mbr.maxy = coords.empty() ? rings[0][0].y : coords[0].y;
    
    std::optional<double> z_min, z_max;
    
    auto update_mbr = [&]([[maybe_unused]] const Coordinate& c) {
        mbr.minx = std::min(mbr.minx, c.x);
        mbr.maxx = std::max(mbr.maxx, c.x);
        mbr.miny = std::min(mbr.miny, c.y);
        mbr.maxy = std::max(mbr.maxy, c.y);
        
        if (c.hasZ()) {
            if (!z_min || c.getZ() < *z_min) {
              z_min = c.getZ();
            }
            if (!z_max || c.getZ() > *z_max) {
              z_max = c.getZ();
            }
        }
    };
    
    for (const auto& c : coords) {
        update_mbr(c);
    }
    
    for (const auto& ring : rings) {
        for (const auto& c : ring) {
            update_mbr(c);
        }
    }
    
    // Also include sub-geometries (e.g. MultiPoint with coords in geometries)
    for (const auto& sub : geometries) {
        MBR sub_mbr = sub.computeMBR();
        mbr.minx = std::min(mbr.minx, sub_mbr.minx);
        mbr.maxx = std::max(mbr.maxx, sub_mbr.maxx);
        mbr.miny = std::min(mbr.miny, sub_mbr.miny);
        mbr.maxy = std::max(mbr.maxy, sub_mbr.maxy);
        if (sub_mbr.z_min) {
            if (!z_min || *sub_mbr.z_min < *z_min) {
              z_min = sub_mbr.z_min;
            }
            if (!z_max || *sub_mbr.z_max > *z_max) {
              z_max = sub_mbr.z_max;
            }
        }
    }
    
    mbr.z_min = z_min;
    mbr.z_max = z_max;
    
    return mbr;
}

// GeometryInfo: Compute centroid
Coordinate GeometryInfo::computeCentroid() const {
    if (coords.empty() && rings.empty() && geometries.empty()) {
        return Coordinate();
    }
    
    double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;
    size_t count = 0;
    bool has_z_coord = false;
    
    auto add_coord = [&]([[maybe_unused]] const Coordinate& c) {
        sum_x += c.x;
        sum_y += c.y;
        if (c.hasZ()) {
            sum_z += c.getZ();
            has_z_coord = true;
        }
        count++;
    };
    
    for (const auto& c : coords) {
        add_coord(c);
    }
    
    for (const auto& ring : rings) {
        for (const auto& c : ring) {
            add_coord(c);
        }
    }
    
    // Recurse into sub-geometries (for Multi* and GeometryCollection types)
    for (const auto& sub : geometries) {
        auto sub_centroid = sub.computeCentroid();
        sum_x += sub_centroid.x;
        sum_y += sub_centroid.y;
        if (sub_centroid.hasZ()) {
            sum_z += sub_centroid.getZ();
            has_z_coord = true;
        }
        count++;
    }
    
    if (count == 0) {
        return Coordinate();
    }
    
    Coordinate centroid(sum_x / count, sum_y / count);
    if (has_z_coord) {
        centroid.z = sum_z / count;
    }
    
    return centroid;
}

// EWKB Parser: Read helpers
double EWKBParser::readDouble(const uint8_t*& ptr, bool is_little_endian) {
    double val = 0;
    if (is_little_endian == true) {  // System is little endian
        std::memcpy(&val, ptr, sizeof(double));
    } else {
        // Byte swap for big endian
        uint8_t temp[sizeof(double)];
        for (size_t i = 0; i < sizeof(double); ++i) {
            temp[i] = ptr[sizeof(double) - 1 - i];
        }
        std::memcpy(&val, temp, sizeof(double));
    }
    ptr += sizeof(double);
    return val;
}

uint32_t EWKBParser::readUInt32(const uint8_t*& ptr, bool is_little_endian) {
    uint32_t val = 0;
    if (is_little_endian == true) {
        std::memcpy(&val, ptr, sizeof(uint32_t));
    } else {
        uint8_t temp[sizeof([[maybe_unused]] uint32_t)];
        for (size_t i = 0; i < sizeof(uint32_t); ++i) {
            temp[i] = ptr[sizeof(uint32_t) - 1 - i];
        }
        std::memcpy(&val, temp, sizeof(uint32_t));
    }
    ptr += sizeof(uint32_t);
    return val;
}

void EWKBParser::writeDouble(std::vector<uint8_t>& buf, double val, bool is_little_endian) {
    if (is_little_endian) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
        buf.insert(buf.end(), bytes, bytes + sizeof(double));
    } else {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
        for (int i = sizeof(double) - 1; i >= 0; --i) {
            buf.push_back(bytes[i]);
        }
    }
}

void EWKBParser::writeUInt32(std::vector<uint8_t>& buf, uint32_t val, bool is_little_endian) {
    if (is_little_endian) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
        buf.insert([[maybe_unused]] buf.end(), bytes, bytes + sizeof(uint32_t));
    } else {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
        for (int i = sizeof(uint32_t) - 1; i >= 0; --i) {
            buf.push_back(bytes[i]);
        }
    }
}

// Parse Point
GeometryInfo EWKBParser::parsePoint(const uint8_t*& ptr, bool has_z, bool is_little_endian) {
    GeometryInfo geom(has_z ? GeometryType::PointZ : GeometryType::Point);
    geom.has_z = has_z;
    
    double x = readDouble(ptr, is_little_endian);
    double y = readDouble(ptr, is_little_endian);
    double z = has_z ? readDouble(ptr, is_little_endian) : 0.0;
    if (has_z) {
        geom.coords.emplace_back(x, y, z);
    } else {
        geom.coords.emplace_back(x, y);
    }
    return geom;
}

// Parse LineString
GeometryInfo EWKBParser::parseLineString(const uint8_t*& ptr, bool has_z, bool is_little_endian) {
    GeometryInfo geom(has_z ? GeometryType::LineStringZ : GeometryType::LineString);
    geom.has_z = has_z;
    
    uint32_t num_points = readUInt32(ptr, is_little_endian);
    geom.coords.reserve(num_points);
    
    for (uint32_t i = 0; i < num_points; ++i) {
        double x = readDouble(ptr, is_little_endian);
        double y = readDouble(ptr, is_little_endian);
        double z = has_z ? readDouble(ptr, is_little_endian) : 0.0;
        if (has_z) {
            geom.coords.emplace_back(x, y, z);
        } else {
            geom.coords.emplace_back(x, y);
        }
    }
    
    return geom;
}

// Parse Polygon
GeometryInfo EWKBParser::parsePolygon(const uint8_t*& ptr, bool has_z, bool is_little_endian) {
    GeometryInfo geom(has_z ? GeometryType::PolygonZ : GeometryType::Polygon);
    geom.has_z = has_z;
    
    uint32_t num_rings = readUInt32(ptr, is_little_endian);
    geom.rings.resize(num_rings);
    
    for (uint32_t r = 0; r < num_rings; ++r) {
        uint32_t num_points = readUInt32(ptr, is_little_endian);
        geom.rings[r].reserve(num_points);
        
        for (uint32_t i = 0; i < num_points; ++i) {
            double x = readDouble(ptr, is_little_endian);
            double y = readDouble(ptr, is_little_endian);
            if (has_z) {
                double z = readDouble(ptr, is_little_endian);
                geom.rings[r].emplace_back(x, y, z);
            } else {
                geom.rings[r].emplace_back(x, y);
            }
        }
    }
    
    return geom;
}

// Parse EWKB
GeometryInfo EWKBParser::parse(const std::vector<uint8_t>& ewkb) {
    if (ewkb.size() < 5) {
        throw std::runtime_error("EWKB: Invalid size (< 5 bytes)");
    }
    
    const uint8_t* ptr = ewkb.data();
    return parseGeometryFromPtr(ptr);
}

// Recursive EWKB geometry parser (reads its own byte-order marker)
GeometryInfo EWKBParser::parseGeometryFromPtr(const uint8_t*& ptr) {
    // Byte order: 0 = Big Endian, 1 = Little Endian
    bool is_little_endian = (*ptr == 0x01);
    ptr++;
    
    // Geometry type (may include SRID and Z flags)
    uint32_t type_code = readUInt32(ptr, is_little_endian);
    
    bool has_srid = (type_code & 0x20000000) != 0;
    bool has_z = (type_code & 0x80000000) != 0;
    bool has_m = (type_code & 0x40000000) != 0;
    
    uint32_t base_type = type_code & 0x000000FF;
    
    int srid = 4326;
    if (has_srid) {
        srid = static_cast<int>(readUInt32(ptr, is_little_endian));
    }
    
    GeometryInfo geom;
    geom.has_z = has_z;
    geom.has_m = has_m;
    geom.srid = srid;
    
    // Parse geometry based on type
    switch (base_type) {
        case 1:  // Point
            geom = parsePoint(ptr, has_z, is_little_endian);
            break;
        case 2:  // LineString
            geom = parseLineString(ptr, has_z, is_little_endian);
            break;
        case 3:  // Polygon
            geom = parsePolygon(ptr, has_z, is_little_endian);
            break;
        case 4: {  // MultiPoint
            geom.type = has_z ? GeometryType::MultiPointZ : GeometryType::MultiPoint;
            uint32_t num = readUInt32(ptr, is_little_endian);
            geom.geometries.reserve(num);
            for (uint32_t i = 0; i < num; ++i) {
                geom.geometries.push_back(parseGeometryFromPtr(ptr));
            }
            break;
        }
        case 5: {  // MultiLineString
            geom.type = has_z ? GeometryType::MultiLineStringZ : GeometryType::MultiLineString;
            uint32_t num = readUInt32(ptr, is_little_endian);
            geom.geometries.reserve(num);
            for (uint32_t i = 0; i < num; ++i) {
                geom.geometries.push_back(parseGeometryFromPtr(ptr));
            }
            break;
        }
        case 6: {  // MultiPolygon
            geom.type = has_z ? GeometryType::MultiPolygonZ : GeometryType::MultiPolygon;
            uint32_t num = readUInt32(ptr, is_little_endian);
            geom.geometries.reserve(num);
            for (uint32_t i = 0; i < num; ++i) {
                geom.geometries.push_back(parseGeometryFromPtr(ptr));
            }
            break;
        }
        case 7: {  // GeometryCollection
            geom.type = has_z ? GeometryType::GeometryCollectionZ : GeometryType::GeometryCollection;
            uint32_t num = readUInt32(ptr, is_little_endian);
            geom.geometries.reserve(num);
            for (uint32_t i = 0; i < num; ++i) {
                geom.geometries.push_back(parseGeometryFromPtr(ptr));
            }
            break;
        }
        default:
            throw std::runtime_error("EWKB: Unsupported geometry type: " + std::to_string(base_type));
    }
    
    geom.srid = srid;
    return geom;
}

// Serialize EWKB
std::vector<uint8_t> EWKBParser::serialize(const GeometryInfo& geom) {
    std::vector<uint8_t> buf;
    serializeGeometryInto(buf, geom, true);
    return buf;
}

// Recursive EWKB serializer helper
void EWKBParser::serializeGeometryInto(std::vector<uint8_t>& buf, const GeometryInfo& geom, bool is_little_endian) {
    // Byte order
    buf.push_back(is_little_endian ? 0x01 : 0x00);
    
    // Geometry type
    uint32_t type_code = static_cast<uint32_t>(geom.type);
    if (geom.has_z) {
      type_code |= 0x80000000;
    }
    writeUInt32(buf, type_code, is_little_endian);
    
    uint32_t base_type = static_cast<uint32_t>(geom.type) & 0x000000FFu;
    
    if (geom.isPoint()) {
        const auto& c = geom.coords[0];
        writeDouble(buf, c.x, is_little_endian);
        writeDouble(buf, c.y, is_little_endian);
        if (geom.has_z) {
          writeDouble(buf, c.getZ(), is_little_endian);
        }
    } else if (geom.isLineString()) {
        writeUInt32(buf, static_cast<uint32_t>(geom.coords.size()), is_little_endian);
        for (const auto& c : geom.coords) {
            writeDouble(buf, c.x, is_little_endian);
            writeDouble(buf, c.y, is_little_endian);
            if (geom.has_z) {
              writeDouble(buf, c.getZ(), is_little_endian);
            }
        }
    } else if (geom.isPolygon()) {
        writeUInt32(buf, static_cast<uint32_t>(geom.rings.size()), is_little_endian);
        for (const auto& ring : geom.rings) {
            writeUInt32(buf, static_cast<uint32_t>(ring.size()), is_little_endian);
            for (const auto& c : ring) {
                writeDouble(buf, c.x, is_little_endian);
                writeDouble(buf, c.y, is_little_endian);
                if (geom.has_z) {
                  writeDouble(buf, c.getZ(), is_little_endian);
                }
            }
        }
    } else if (base_type >= 4 && base_type <= 7) {
        // MultiPoint, MultiLineString, MultiPolygon, GeometryCollection
        writeUInt32(buf, static_cast<uint32_t>(geom.geometries.size()), is_little_endian);
        for (const auto& sub : geom.geometries) {
            serializeGeometryInto(buf, sub, is_little_endian);
        }
    }
}

// WGS84 coordinate range validation (disabled with THEMIS_GEO_COMPAT_LAX)
static void validateWGS84(double lon, double lat) {
#ifndef THEMIS_GEO_COMPAT_LAX
    if (lon < -180.0 || lon > 180.0) {
        throw std::runtime_error(
            "GeoJSON: longitude " + std::to_string(lon) + " is out of WGS84 range [-180, 180]");
    }
    if (lat < -90.0 || lat > 90.0) {
        throw std::runtime_error(
            "GeoJSON: latitude " + std::to_string(lat) + " is out of WGS84 range [-90, 90]");
    }
#else
#endif
}

// File-scope helper: recursively parse a GeoJSON geometry object
static GeometryInfo parseGeoJSONGeomImpl(const json& j, int depth) {
    if (depth <= 0) {
        throw std::runtime_error("GeoJSON: maximum nesting depth exceeded");
    }
    
    std::string type = j.at("type").get<std::string>();
    GeometryInfo geom = {};
    
    if (type == "Point") {
        const auto& coords = j.at("coordinates");
        double x = coords.at(0).get<double>();
        double y = coords.at(1).get<double>();
        validateWGS84(x, y);
        if (coords.size() > 2) {
            geom.type = GeometryType::PointZ;
            geom.has_z = true;
            geom.coords.emplace_back(x, y, coords.at(2).get<double>());
        } else {
            geom.type = GeometryType::Point;
            geom.coords.emplace_back(x, y);
        }
    } else if (type == "MultiPoint") {
        const auto& coords_arr = j.at("coordinates");
        bool has_z = !coords_arr.empty() && coords_arr.at(0).size() > 2;
        geom.type = has_z ? GeometryType::MultiPointZ : GeometryType::MultiPoint;
        geom.has_z = has_z;
        geom.geometries.reserve(coords_arr.size());
        for (const auto& coord : coords_arr) {
            GeometryInfo pt;
            double x = coord.at(0).get<double>();
            double y = coord.at(1).get<double>();
            validateWGS84(x, y);
            if (has_z) {
                pt.type = GeometryType::PointZ;
                pt.has_z = true;
                pt.coords.emplace_back(x, y, coord.at(2).get<double>());
            } else {
                pt.type = GeometryType::Point;
                pt.coords.emplace_back(x, y);
            }
            geom.geometries.push_back(std::move(pt));
        }
    } else if (type == "LineString") {
        const auto& coords_arr = j.at("coordinates");
        bool has_z = !coords_arr.empty() && coords_arr.at(0).size() > 2;
        geom.type = has_z ? GeometryType::LineStringZ : GeometryType::LineString;
        geom.has_z = has_z;
        geom.coords.reserve(coords_arr.size());
        for (const auto& coord : coords_arr) {
            double x = coord.at(0).get<double>();
            double y = coord.at(1).get<double>();
            validateWGS84(x, y);
            if (has_z) {
                geom.coords.emplace_back(x, y, coord.at(2).get<double>());
            } else {
                geom.coords.emplace_back(x, y);
            }
        }
    } else if (type == "MultiLineString") {
        const auto& lines_arr = j.at("coordinates");
        const auto& first_coord = (!lines_arr.empty() && !lines_arr.at(0).empty())
                                  ? lines_arr.at(0).at(0) : json{};
        bool has_z = !first_coord.empty() && first_coord.size() > 2;
        geom.type = has_z ? GeometryType::MultiLineStringZ : GeometryType::MultiLineString;
        geom.has_z = has_z;
        geom.geometries.reserve(lines_arr.size());
        for (const auto& line : lines_arr) {
            GeometryInfo ls;
            ls.type = has_z ? GeometryType::LineStringZ : GeometryType::LineString;
            ls.has_z = has_z;
            ls.coords.reserve(line.size());
            for (const auto& coord : line) {
                double x = coord.at(0).get<double>();
                double y = coord.at(1).get<double>();
                validateWGS84(x, y);
                if (has_z) {
                    ls.coords.emplace_back(x, y, coord.at(2).get<double>());
                } else {
                    ls.coords.emplace_back(x, y);
                }
            }
            geom.geometries.push_back(std::move(ls));
        }
    } else if (type == "Polygon") {
        const auto& rings_arr = j.at("coordinates");
        if (rings_arr.empty()) {
            throw std::runtime_error("GeoJSON Polygon must have at least one ring");
        }
        bool has_z = !rings_arr.at(0).empty() && rings_arr.at(0).at(0).size() > 2;
        geom.type = has_z ? GeometryType::PolygonZ : GeometryType::Polygon;
        geom.has_z = has_z;
        geom.rings.reserve(rings_arr.size());
        for (const auto& ring : rings_arr) {
            std::vector<Coordinate> ring_coords = {};

            ring_coords.reserve(ring.size());
            for (const auto& coord : ring) {
                double x = coord.at(0).get<double>();
                double y = coord.at(1).get<double>();
                validateWGS84(x, y);
                if (has_z) {
                    ring_coords.emplace_back(x, y, coord.at(2).get<double>());
                } else {
                    ring_coords.emplace_back(x, y);
                }
            }
            geom.rings.push_back(std::move(ring_coords));
        }
    } else if (type == "MultiPolygon") {
        const auto& polys_arr = j.at("coordinates");
        const auto& first_ring = (!polys_arr.empty() && !polys_arr.at(0).empty())
                                 ? polys_arr.at(0).at(0) : json{};
        bool has_z = !first_ring.empty() && first_ring.at(0).size() > 2;
        geom.type = has_z ? GeometryType::MultiPolygonZ : GeometryType::MultiPolygon;
        geom.has_z = has_z;
        geom.geometries.reserve(polys_arr.size());
        for (const auto& poly : polys_arr) {
            GeometryInfo pg;
            pg.type = has_z ? GeometryType::PolygonZ : GeometryType::Polygon;
            pg.has_z = has_z;
            pg.rings.reserve(poly.size());
            for (const auto& ring : poly) {
                std::vector<Coordinate> ring_coords = {};

                ring_coords.reserve(ring.size());
                for (const auto& coord : ring) {
                    double x = coord.at(0).get<double>();
                    double y = coord.at(1).get<double>();
                    validateWGS84(x, y);
                    if (has_z) {
                        ring_coords.emplace_back(x, y, coord.at(2).get<double>());
                    } else {
                        ring_coords.emplace_back(x, y);
                    }
                }
                pg.rings.push_back(std::move(ring_coords));
            }
            geom.geometries.push_back(std::move(pg));
        }
    } else if (type == "GeometryCollection") {
        geom.type = GeometryType::GeometryCollection;
        const auto& members = j.at("geometries");
        geom.geometries.reserve(members.size());
        for (const auto& member : members) {
            geom.geometries.push_back(parseGeoJSONGeomImpl(member, depth - 1));
        }
        // Promote to GeometryCollectionZ if any member carries Z coordinates,
        // matching the Z-detection behaviour of MultiPoint/MultiPolygon.
        for (const auto& sub : geom.geometries) {
            if (sub.has_z) {
                geom.has_z = true;
                geom.type = GeometryType::GeometryCollectionZ;
                break;
            }
        }
    } else {
        throw std::runtime_error("GeoJSON: unsupported geometry type: " + type);
    }
    
    return geom;
}

// Parse GeoJSON
GeometryInfo EWKBParser::parseGeoJSON(const std::string& geojson_str) {
    auto j = json::parse(geojson_str);
    return parseGeoJSONGeomImpl(j, 8);
}

GeometryInfo EWKBParser::parseWKT(const std::string& wkt_raw) {
    const std::string wkt = trimCopy(wkt_raw);
    if (wkt.empty()) {
        throw std::runtime_error("WKT: Empty input");
    }

    std::string upper = wkt;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    const std::string body = extractWktBody(wkt);

    if (upper.rfind("POINT", 0) == 0) {
        GeometryInfo geom(GeometryType::Point);
        geom.coords.push_back(parseCoordinateToken(body));
        geom.has_z = geom.coords[0].hasZ();
        if (geom.has_z) {
            geom.type = GeometryType::PointZ;
        }
        return geom;
    }

    if (upper.rfind("LINESTRING", 0) == 0) {
        GeometryInfo geom(GeometryType::LineString);
        auto tokens = splitTopLevel(body, ',');
        geom.coords.reserve(tokens.size());
        for (const auto& token : tokens) {
            geom.coords.push_back(parseCoordinateToken(token));
        }
        geom.has_z = !geom.coords.empty() && geom.coords[0].hasZ();
        if (geom.has_z) {
            geom.type = GeometryType::LineStringZ;
        }
        return geom;
    }

    if (upper.rfind("POLYGON", 0) == 0) {
        GeometryInfo geom(GeometryType::Polygon);
        auto rings_raw = splitTopLevel(body, ',');

        std::vector<std::string> ring_groups;
        std::string merged = {};
        int depth = 0;
        for (const auto& part : rings_raw) {
            if (!merged.empty()) {
                merged += ",";
            }
            merged += part;
            for (char c : part) {
                if (c == '(') depth++;
                if (c == ') {
                  ') depth--;
                }
            }
            if (depth == 0 && !merged.empty()) {
                ring_groups.push_back(trimCopy(merged));
                merged.clear();
            }
        }

        geom.rings.reserve(ring_groups.size());
        for (auto ring : ring_groups) {
            if (!ring.empty() && ring.front() == '(' && ring.back() == ')') {
                ring = ring.substr(1, ring.size() - 2);
            }
            auto coord_tokens = splitTopLevel(ring, ',');
            std::vector<Coordinate> coords = {};

            coords.reserve(coord_tokens.size());
            for (const auto& token : coord_tokens) {
                coords.push_back(parseCoordinateToken(token));
            }
            geom.rings.push_back(std::move(coords));
        }

        geom.has_z = !geom.rings.empty() && !geom.rings[0].empty() && geom.rings[0][0].hasZ();
        if (geom.has_z) {
            geom.type = GeometryType::PolygonZ;
        }
        return geom;
    }

    throw std::runtime_error("WKT: Unsupported geometry type");
}

std::string EWKBParser::toWKT(const GeometryInfo& geom) {
    std::ostringstream oss = {};
    if (geom.isPoint()) {
        if (geom.coords.empty()) {
            return "POINT EMPTY";
        }
        const auto& c = geom.coords[0];
        oss << "POINT(" << c.x << " " << c.y;
        if (geom.has_z && c.hasZ()) {
            oss << " " << c.getZ();
        }
        oss << ")";
        return oss.str();
    }

    if (geom.isLineString()) {
        if (geom.coords.empty()) {
            return "LINESTRING EMPTY";
        }
        oss << "LINESTRING(";
        for (size_t i = 0; i < geom.coords.size(); ++i) {
            const auto& c = geom.coords[i];
            if (i > 0) {
                oss << ",";
            }
            oss << c.x << " " << c.y;
            if (geom.has_z && c.hasZ()) {
                oss << " " << c.getZ();
            }
        }
        oss << ")";
        return oss.str();
    }

    if (geom.isPolygon()) {
        if (geom.rings.empty()) {
            return "POLYGON EMPTY";
        }
        oss << "POLYGON(";
        for (size_t r = 0; r < geom.rings.size(); ++r) {
            if (r > 0) {
                oss << ",";
            }
            oss << "(";
            for (size_t i = 0; i < geom.rings[r].size(); ++i) {
                const auto& c = geom.rings[r][i];
                if (i > 0) {
                    oss << ",";
                }
                oss << c.x << " " << c.y;
                if (geom.has_z && c.hasZ()) {
                    oss << " " << c.getZ();
                }
            }
            oss << ")";
        }
        oss << ")";
        return oss.str();
    }

    throw std::runtime_error("WKT: Unsupported geometry type for serialization");
}

// To GeoJSON
std::string EWKBParser::toGeoJSON(const GeometryInfo& geom) {
    json j;
    uint32_t base_type = static_cast<uint32_t>(geom.type) & 0x000000FFu;
    
    if (geom.isPoint()) {
        j["type"] = "Point";
        const auto& c = geom.coords[0];
        if (geom.has_z) {
            j["coordinates"] = {c.x, c.y, c.getZ()};
        } else {
            j["coordinates"] = {c.x, c.y};
        }
    } else if (base_type == static_cast<uint32_t>(GeometryType::MultiPoint)) {
        j["type"] = "MultiPoint";
        json coords_arr = json::array();
        for (const auto& sub : geom.geometries) {
            const auto& c = sub.coords[0];
            if (sub.has_z) {
                coords_arr.push_back({c.x, c.y, c.getZ()});
            } else {
                coords_arr.push_back({c.x, c.y});
            }
        }
        j["coordinates"] = coords_arr;
    } else if (geom.isLineString()) {
        j["type"] = "LineString";
        json coords_arr = json::array();
        for (const auto& c : geom.coords) {
            if (geom.has_z) {
                coords_arr.push_back({c.x, c.y, c.getZ()});
            } else {
                coords_arr.push_back({c.x, c.y});
            }
        }
        j["coordinates"] = coords_arr;
    } else if (base_type == static_cast<uint32_t>(GeometryType::MultiLineString)) {
        j["type"] = "MultiLineString";
        json lines_arr = json::array();
        for (const auto& sub : geom.geometries) {
            json line_coords = json::array();
            for (const auto& c : sub.coords) {
                if (sub.has_z) {
                    line_coords.push_back({c.x, c.y, c.getZ()});
                } else {
                    line_coords.push_back({c.x, c.y});
                }
            }
            lines_arr.push_back(line_coords);
        }
        j["coordinates"] = lines_arr;
    } else if (geom.isPolygon()) {
        j["type"] = "Polygon";
        json rings_arr = json::array();
        for (const auto& ring : geom.rings) {
            json ring_coords = json::array();
            for (const auto& c : ring) {
                if (geom.has_z) {
                    ring_coords.push_back({c.x, c.y, c.getZ()});
                } else {
                    ring_coords.push_back({c.x, c.y});
                }
            }
            rings_arr.push_back(ring_coords);
        }
        j["coordinates"] = rings_arr;
    } else if (base_type == static_cast<uint32_t>(GeometryType::MultiPolygon)) {
        j["type"] = "MultiPolygon";
        json polys_arr = json::array();
        for (const auto& sub : geom.geometries) {
            json rings_arr = json::array();
            for (const auto& ring : sub.rings) {
                json ring_coords = json::array();
                for (const auto& c : ring) {
                    if (sub.has_z) {
                        ring_coords.push_back({c.x, c.y, c.getZ()});
                    } else {
                        ring_coords.push_back({c.x, c.y});
                    }
                }
                rings_arr.push_back(ring_coords);
            }
            polys_arr.push_back(rings_arr);
        }
        j["coordinates"] = polys_arr;
    } else if (base_type == static_cast<uint32_t>(GeometryType::GeometryCollection)) {
        j["type"] = "GeometryCollection";
        json members = json::array();
        for (const auto& sub : geom.geometries) {
            members.push_back(json::parse(toGeoJSON(sub)));
        }
        j["geometries"] = members;
    }
    
    return j.dump();
}

// Compute sidecar
GeoSidecar EWKBParser::computeSidecar(const GeometryInfo& geom) {
    GeoSidecar sidecar;
    sidecar.mbr = geom.computeMBR();
    sidecar.centroid = geom.computeCentroid();
    
    if (sidecar.mbr.hasZ()) {
        sidecar.z_min = sidecar.mbr.z_min.value_or(0.0);
        sidecar.z_max = sidecar.mbr.z_max.value_or(0.0);
    }
    
    return sidecar;
}

// Validate EWKB
bool EWKBParser::validate(const std::vector<uint8_t>& ewkb) {
    try {
        parse(ewkb);
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace geo
}  // namespace themis


