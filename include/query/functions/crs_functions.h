/**
 * @file crs_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "function_registry.h"
#include <cmath>
#include <unordered_map>
#include <string>
#include <tuple>



namespace themis {
namespace query {
namespace functions {

/**
 * @brief Coordinate Reference System (CRS) Transformation Functions
 * 
 * Provides comprehensive coordinate transformation between different CRS:
 * 
 * ## Supported Coordinate Systems
 * 
 * ### Geographic (lat/lon on ellipsoid)
 * - WGS84 (EPSG:4326) - GPS, global standard
 * - ETRS89 (EPSG:4258) - European Terrestrial Reference System
 * - GRS80 - Geodetic Reference System 1980
 * 
 * ### Projected (meters on plane)
 * - UTM (Universal Transverse Mercator) - Zones 1-60 N/S
 * - ETRS89/UTM zone 32N (EPSG:25832) - Germany West
 * - ETRS89/UTM zone 33N (EPSG:25833) - Germany East
 * - Gauß-Krüger (EPSG:31466-31469) - Legacy German system
 * - Web Mercator (EPSG:3857) - Google Maps, OSM
 * 
 * ## Mathematical Foundation
 * 
 * UTM uses Transverse Mercator projection:
 * - Central meridian per zone
 * - Scale factor k0 = 0.9996
 * - False easting 500000m
 * - False northing 0m (N) or 10000000m (S)
 * 
 * ## Usage
 * 
 * ```aql
 * // Convert UTM to WGS84
 * LET wgs84 = ST_TRANSFORM(utm_point, 25832, 4326)
 * 
 * // Convert WGS84 to UTM zone 32N
 * LET utm = ST_TRANSFORM(wgs84_point, 4326, 25832)
 * 
 * // Get EPSG code for coordinates
 * LET epsg = ST_SRID(point)
 * ```
 */

// ============================================================================
// Ellipsoid Definitions
// ============================================================================

namespace crs {

/**
 * @brief Ellipsoid parameters
 */
struct Ellipsoid {
    std::string name = {};
    double a;      // Semi-major axis (meters)
    double b;      // Semi-minor axis (meters)
    double f;      // Flattening = (a-b)/a
    double e2;     // First eccentricity squared = (a²-b²)/a²
    double ep2;    // Second eccentricity squared = (a²-b²)/b²
    
    Ellipsoid(const char* n, double semi_major, double inv_flattening)
        : name(n)
        , a(semi_major)
        , f(1.0 / inv_flattening)
        , b(semi_major * (1.0 - f))
        , e2((2.0 * f) - (f * f))
        , ep2(e2 / (1.0 - e2))
    {}
};

// Standard ellipsoids
inline const Ellipsoid WGS84_ELLIPSOID("WGS84", 6378137.0, 298.257223563);
inline const Ellipsoid GRS80_ELLIPSOID("GRS80", 6378137.0, 298.257222101);
inline const Ellipsoid BESSEL_ELLIPSOID("Bessel 1841", 6377397.155, 299.1528128);

/**
 * @brief UTM Zone parameters
 */
struct UTMZone {
    int zone = 0;          // 1-60
    bool isNorth;      // Northern or Southern hemisphere
    double lon0;       // Central meridian in degrees
    double k0;         // Scale factor (0.9996 for UTM)
    double falseE;     // False easting (500000m)
    double falseN;     // False northing (0 for N, 10000000 for S)
    
    UTMZone(int z, bool north) 
        : zone(z)
        , isNorth(north)
        , lon0(-183.0 + z * 6.0)  // Central meridian = -180 + (zone-1)*6 + 3
        , k0(0.9996)
        , falseE(500000.0)
        , falseN(north ? 0.0 : 10000000.0)
    {}
};

/**
 * @brief EPSG code mapping
 */
struct EPSGDefinition {
    int code = 0;
    std::string name;
    std::string type;           // "geographic" or "projected"
    Ellipsoid ellipsoid;
    int utmZone = 0;            // For UTM-based systems
    bool utmNorth = false;
    double centralMeridian = 0.0;     // For other projections
    double scaleFactor = 1.0;
    double falseEasting = 0.0;
    double falseNorthing = 0.0;
};

// EPSG code database
inline const std::unordered_map<int, EPSGDefinition>& getEPSGDatabase() {
    static const std::unordered_map<int, EPSGDefinition> db = {
        // Geographic CRS
        {4326, {4326, "WGS 84", "geographic", WGS84_ELLIPSOID, 0, true, 0.0, 1.0, 0.0, 0.0}},
        {4258, {4258, "ETRS89", "geographic", GRS80_ELLIPSOID, 0, true, 0.0, 1.0, 0.0, 0.0}},
        {4314, {4314, "DHDN (Potsdam Datum)", "geographic", BESSEL_ELLIPSOID, 0, true, 0.0, 1.0, 0.0, 0.0}},
        
        // ETRS89/UTM zones (Germany)
        {25831, {25831, "ETRS89 / UTM zone 31N", "projected", GRS80_ELLIPSOID, 31, true, 3.0, 0.9996, 500000.0, 0.0}},
        {25832, {25832, "ETRS89 / UTM zone 32N", "projected", GRS80_ELLIPSOID, 32, true, 9.0, 0.9996, 500000.0, 0.0}},
        {25833, {25833, "ETRS89 / UTM zone 33N", "projected", GRS80_ELLIPSOID, 33, true, 15.0, 0.9996, 500000.0, 0.0}},
        
        // WGS84/UTM zones
        {32631, {32631, "WGS 84 / UTM zone 31N", "projected", WGS84_ELLIPSOID, 31, true, 3.0, 0.9996, 500000.0, 0.0}},
        {32632, {32632, "WGS 84 / UTM zone 32N", "projected", WGS84_ELLIPSOID, 32, true, 9.0, 0.9996, 500000.0, 0.0}},
        {32633, {32633, "WGS 84 / UTM zone 33N", "projected", WGS84_ELLIPSOID, 33, true, 15.0, 0.9996, 500000.0, 0.0}},
        
        // Gauß-Krüger (legacy German)
        {31466, {31466, "DHDN / 3-degree Gauss-Kruger zone 2", "projected", BESSEL_ELLIPSOID, 0, true, 6.0, 1.0, 2500000.0, 0.0}},
        {31467, {31467, "DHDN / 3-degree Gauss-Kruger zone 3", "projected", BESSEL_ELLIPSOID, 0, true, 9.0, 1.0, 3500000.0, 0.0}},
        {31468, {31468, "DHDN / 3-degree Gauss-Kruger zone 4", "projected", BESSEL_ELLIPSOID, 0, true, 12.0, 1.0, 4500000.0, 0.0}},
        {31469, {31469, "DHDN / 3-degree Gauss-Kruger zone 5", "projected", BESSEL_ELLIPSOID, 0, true, 15.0, 1.0, 5500000.0, 0.0}},
        
        // Web Mercator (Google Maps, OSM)
        {3857, {3857, "WGS 84 / Pseudo-Mercator", "projected", WGS84_ELLIPSOID, 0, true, 0.0, 1.0, 0.0, 0.0}},
    };
    return db;
}

// ============================================================================
// Conversion Helper Functions
// ============================================================================

// Degrees to radians
inline double deg2rad(double deg) {
    return deg * M_PI / 180.0;
}

// Radians to degrees
inline double rad2deg(double rad) {
    return rad * 180.0 / M_PI;
}

/**
 * @brief Convert geographic (lat/lon) to UTM coordinates
 * 
 * Uses Transverse Mercator projection formulas.
 * Reference: Snyder, J.P., "Map Projections - A Working Manual", USGS Professional Paper 1395
 * 
 * @param lat Latitude in degrees
 * @param lon Longitude in degrees
 * @param zone UTM zone parameters
 * @param ellipsoid Ellipsoid parameters
 * @return (easting, northing) in meters
 */
inline std::pair<double, double> geographicToUTM(
    double lat, double lon,
    const UTMZone& zone,
    const Ellipsoid& ellipsoid
) {
    double phi = deg2rad(lat);
    double lambda = deg2rad(lon);
    double lambda0 = deg2rad(zone.lon0);
    
    double a = ellipsoid.a;
    double e2 = ellipsoid.e2;
    double ep2 = ellipsoid.ep2;
    double k0 = zone.k0;
    
    // Auxiliary values
    double N = a / std::sqrt(1.0 - e2 * std::sin(phi) * std::sin(phi));
    double T = std::tan(phi) * std::tan(phi);
    double C = ep2 * std::cos(phi) * std::cos(phi);
    double A = (lambda - lambda0) * std::cos(phi);
    
    // Meridional arc length from equator to latitude phi
    double e4 = e2 * e2;
    double e6 = e4 * e2;
    double M = a * (
        (1.0 - e2/4.0 - 3.0*e4/64.0 - 5.0*e6/256.0) * phi
        - (3.0*e2/8.0 + 3.0*e4/32.0 + 45.0*e6/1024.0) * std::sin(2.0*phi)
        + (15.0*e4/256.0 + 45.0*e6/1024.0) * std::sin(4.0*phi)
        - (35.0*e6/3072.0) * std::sin(6.0*phi)
    );
    
    // Easting
    double A2 = A * A;
    double A3 = A2 * A;
    double A4 = A3 * A;
    double A5 = A4 * A;
    double A6 = A5 * A;
    
    double easting = zone.falseE + k0 * N * (
        A 
        + (1.0 - T + C) * A3 / 6.0
        + (5.0 - 18.0*T + T*T + 72.0*C - 58.0*ep2) * A5 / 120.0
    );
    
    // Northing
    double northing = zone.falseN + k0 * (
        M + N * std::tan(phi) * (
            A2 / 2.0
            + (5.0 - T + 9.0*C + 4.0*C*C) * A4 / 24.0
            + (61.0 - 58.0*T + T*T + 600.0*C - 330.0*ep2) * A6 / 720.0
        )
    );
    
    return {easting, northing};
}

/**
 * @brief Convert UTM coordinates to geographic (lat/lon)
 * 
 * Inverse Transverse Mercator projection.
 * 
 * @param easting Easting in meters
 * @param northing Northing in meters
 * @param zone UTM zone parameters
 * @param ellipsoid Ellipsoid parameters
 * @return (latitude, longitude) in degrees
 */
inline std::pair<double, double> utmToGeographic(
    double easting, double northing,
    const UTMZone& zone,
    const Ellipsoid& ellipsoid
) {
    double a = ellipsoid.a;
    double e2 = ellipsoid.e2;
    double ep2 = ellipsoid.ep2;
    double k0 = zone.k0;
    double lambda0 = deg2rad(zone.lon0);
    
    // Remove false easting/northing
    double x = easting - zone.falseE;
    double y = northing - zone.falseN;
    
    // Meridional arc
    double M = y / k0;
    
    // Footprint latitude (iterative calculation)
    double e1 = (1.0 - std::sqrt(1.0 - e2)) / (1.0 + std::sqrt(1.0 - e2));
    double e12 = e1 * e1;
    double e13 = e12 * e1;
    double e14 = e13 * e1;
    
    double mu = M / (a * (1.0 - e2/4.0 - 3.0*e2*e2/64.0 - 5.0*e2*e2*e2/256.0));
    
    double phi1 = mu 
        + (3.0*e1/2.0 - 27.0*e13/32.0) * std::sin(2.0*mu)
        + (21.0*e12/16.0 - 55.0*e14/32.0) * std::sin(4.0*mu)
        + (151.0*e13/96.0) * std::sin(6.0*mu)
        + (1097.0*e14/512.0) * std::sin(8.0*mu);
    
    // Auxiliary values at footprint latitude
    double sinPhi1 = std::sin(phi1);
    double cosPhi1 = std::cos(phi1);
    double tanPhi1 = std::tan(phi1);
    
    double N1 = a / std::sqrt(1.0 - e2 * sinPhi1 * sinPhi1);
    double R1 = a * (1.0 - e2) / std::pow(1.0 - e2 * sinPhi1 * sinPhi1, 1.5);
    double D = x / (N1 * k0);
    
    double T1 = tanPhi1 * tanPhi1;
    double C1 = ep2 * cosPhi1 * cosPhi1;
    double D2 = D * D;
    double D3 = D2 * D;
    double D4 = D3 * D;
    double D5 = D4 * D;
    double D6 = D5 * D;
    
    // Latitude
    double phi = phi1 - (N1 * tanPhi1 / R1) * (
        D2 / 2.0
        - (5.0 + 3.0*T1 + 10.0*C1 - 4.0*C1*C1 - 9.0*ep2) * D4 / 24.0
        + (61.0 + 90.0*T1 + 298.0*C1 + 45.0*T1*T1 - 252.0*ep2 - 3.0*C1*C1) * D6 / 720.0
    );
    
    // Longitude
    double lambda = lambda0 + (
        D 
        - (1.0 + 2.0*T1 + C1) * D3 / 6.0
        + (5.0 - 2.0*C1 + 28.0*T1 - 3.0*C1*C1 + 8.0*ep2 + 24.0*T1*T1) * D5 / 120.0
    ) / cosPhi1;
    
    return {rad2deg(phi), rad2deg(lambda)};
}

/**
 * @brief Convert geographic to Web Mercator (EPSG:3857)
 */
inline std::pair<double, double> geographicToWebMercator(double lat, double lon) {
    constexpr double R = 6378137.0; // WGS84 semi-major axis
    
    double x = R * deg2rad(lon);
    double y = R * std::log(std::tan(M_PI/4.0 + deg2rad(lat)/2.0));
    
    return {x, y};
}

/**
 * @brief Convert Web Mercator to geographic
 */
inline std::pair<double, double> webMercatorToGeographic(double x, double y) {
    constexpr double R = 6378137.0;
    
    double lon = rad2deg(x / R);
    double lat = rad2deg(2.0 * std::atan(std::exp(y / R)) - M_PI/2.0);
    
    return {lat, lon};
}

/**
 * @brief Helmert 7-parameter datum transformation
 * 
 * Transforms coordinates between different geodetic datums (e.g., DHDN to ETRS89)
 * using the Bursa-Wolf model with 7 parameters.
 * 
 * @param x, y, z Cartesian coordinates in source datum
 * @param dx, dy, dz Translation parameters (meters)
 * @param rx, ry, rz Rotation parameters (arc-seconds)
 * @param s Scale factor (ppm)
 * @return Transformed (x, y, z)
 */
inline std::tuple<double, double, double> helmertTransform(
    double x, double y, double z,
    double dx, double dy, double dz,
    double rx, double ry, double rz,
    double s
) {
    // Convert rotations from arc-seconds to radians
    constexpr double asToRad = M_PI / (180.0 * 3600.0);
    double rxRad = rx * asToRad;
    double ryRad = ry * asToRad;
    double rzRad = rz * asToRad;
    
    // Scale factor (ppm to factor)
    double sf = 1.0 + s * 1e-6;
    
    // Apply transformation
    double xNew = dx + sf * (x - rzRad * y + ryRad * z);
    double yNew = dy + sf * (rzRad * x + y - rxRad * z);
    double zNew = dz + sf * (-ryRad * x + rxRad * y + z);
    
    return {xNew, yNew, zNew};
}

/**
 * @brief Convert geographic (lat/lon/h) to Cartesian (X/Y/Z)
 */
inline std::tuple<double, double, double> geographicToCartesian(
    double lat, double lon, double h,
    const Ellipsoid& ellipsoid
) {
    double phi = deg2rad(lat);
    double lambda = deg2rad(lon);
    
    double sinPhi = std::sin(phi);
    double cosPhi = std::cos(phi);
    double sinLambda = std::sin(lambda);
    double cosLambda = std::cos(lambda);
    
    double N = ellipsoid.a / std::sqrt(1.0 - ellipsoid.e2 * sinPhi * sinPhi);
    
    double x = (N + h) * cosPhi * cosLambda;
    double y = (N + h) * cosPhi * sinLambda;
    double z = (N * (1.0 - ellipsoid.e2) + h) * sinPhi;
    
    return {x, y, z};
}

/**
 * @brief Convert Cartesian (X/Y/Z) to geographic (lat/lon/h)
 * 
 * Uses Bowring's iterative method for high accuracy.
 */
inline std::tuple<double, double, double> cartesianToGeographic(
    double x, double y, double z,
    const Ellipsoid& ellipsoid
) {
    double a = ellipsoid.a;
    double b = ellipsoid.b;
    double e2 = ellipsoid.e2;
    double ep2 = ellipsoid.ep2;
    
    // Longitude
    double lambda = std::atan2(y, x);
    
    // Distance from Z-axis
    double p = std::sqrt(x*x + y*y);
    
    // Initial estimate using Bowring's formula
    double theta = std::atan2(z * a, p * b);
    double sinTheta = std::sin(theta);
    double cosTheta = std::cos(theta);
    
    // Latitude (Bowring's formula)
    double phi = std::atan2(
        z + ep2 * b * sinTheta * sinTheta * sinTheta,
        p - e2 * a * cosTheta * cosTheta * cosTheta
    );
    
    // Iterate for better accuracy
    for (int i = 0; i < 5; ++i) {
        double sinPhi = std::sin(phi);
        double N = a / std::sqrt(1.0 - e2 * sinPhi * sinPhi);
        phi = std::atan2(z + e2 * N * sinPhi, p);
    }
    
    // Height
    double sinPhi = std::sin(phi);
    double cosPhi = std::cos(phi);
    double N = a / std::sqrt(1.0 - e2 * sinPhi * sinPhi);
    double h = p / cosPhi - N;
    
    return {rad2deg(phi), rad2deg(lambda), h};
}

/**
 * @brief DHDN (Potsdam) to ETRS89 datum transformation parameters
 * 
 * Official BKG parameters for Germany
 */
struct DHDNToETRS89Params {
    static constexpr double dx = 598.1;
    static constexpr double dy = 73.7;
    static constexpr double dz = 418.2;
    static constexpr double rx = 0.202;    // arc-seconds
    static constexpr double ry = 0.045;    // arc-seconds
    static constexpr double rz = -2.455;   // arc-seconds
    static constexpr double s = 6.7;       // ppm
};

/**
 * @brief Determine UTM zone from longitude
 */
inline int getUTMZone(double lon) {
    return static_cast<int>((lon + 180.0) / 6.0) + 1;
}

/**
 * @brief Get EPSG code for UTM zone
 */
inline int getUTMEpsg(int zone, bool isNorth, bool isWGS84 = true) {
    if (isWGS84) {
        return isNorth ? (32600 + zone) : (32700 + zone);
    } else {
        // ETRS89
        return isNorth ? (25800 + zone) : (25800 + zone); // No southern ETRS89 codes
    }
}

} // namespace crs

// ============================================================================
// AQL Functions
// ============================================================================

/**
 * @brief ST_TRANSFORM(geometry, from_srid, to_srid) - Transform between CRS
 * 
 * Main coordinate transformation function.
 */
class StTransformFunction : public IFunction {
public:
    ~StTransformFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_TRANSFORM",
            "Geo",
            "Transform geometry from one coordinate reference system to another",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "Input geometry"},
                {"from_srid", ArgType::INTEGER, true, nullptr, "Source EPSG code (e.g., 25832)"},
                {"to_srid", ArgType::INTEGER, true, nullptr, "Target EPSG code (e.g., 4326)"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {
                "ST_TRANSFORM(utm_point, 25832, 4326)  // ETRS89/UTM32N → WGS84",
                "ST_TRANSFORM(wgs84_point, 4326, 25832)  // WGS84 → ETRS89/UTM32N"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& geom = args[0];
        int fromSrid = args[1].get<int>();
        int toSrid = args[2].get<int>();
        
        if (fromSrid == toSrid) {
            return geom; // No transformation needed
        }
        
        // Transform coordinates based on geometry type
        return transformGeometry(geom, fromSrid, toSrid);
    }
    
private:
    nlohmann::json transformGeometry(const nlohmann::json& geom, int fromSrid, int toSrid) const {
        std::string type = geom["type"];
        nlohmann::json result = geom;
        
        if (type == "Point") {
            auto [x, y] = transformPoint(
                geom["coordinates"][0].get<double>(),
                geom["coordinates"][1].get<double>(),
                fromSrid, toSrid
            );
            result["coordinates"] = {x, y};
            if (geom["coordinates"].size() > 2) {
                result["coordinates"].push_back(geom["coordinates"][2]);
            }
        } else if (type == "LineString" || type == "MultiPoint") {
            result["coordinates"] = nlohmann::json::array();
            for (const auto& coord : geom["coordinates"]) {
                auto [x, y] = transformPoint(
                    coord[0].get<double>(), coord[1].get<double>(),
                    fromSrid, toSrid
                );
                if (coord.size() > 2) {
                    result["coordinates"].push_back({x, y, coord[2]});
                } else {
                    result["coordinates"].push_back({x, y});
                }
            }
        } else if (type == "Polygon" || type == "MultiLineString") {
            result["coordinates"] = nlohmann::json::array();
            for (const auto& ring : geom["coordinates"]) {
                nlohmann::json newRing = nlohmann::json::array();
                for (const auto& coord : ring) {
                    auto [x, y] = transformPoint(
                        coord[0].get<double>(), coord[1].get<double>(),
                        fromSrid, toSrid
                    );
                    if (coord.size() > 2) {
                        newRing.push_back({x, y, coord[2]});
                    } else {
                        newRing.push_back({x, y});
                    }
                }
                result["coordinates"].push_back(newRing);
            }
        } else if (type == "MultiPolygon") {
            result["coordinates"] = nlohmann::json::array();
            for (const auto& polygon : geom["coordinates"]) {
                nlohmann::json newPolygon = nlohmann::json::array();
                for (const auto& ring : polygon) {
                    nlohmann::json newRing = nlohmann::json::array();
                    for (const auto& coord : ring) {
                        auto [x, y] = transformPoint(
                            coord[0].get<double>(), coord[1].get<double>(),
                            fromSrid, toSrid
                        );
                        if (coord.size() > 2) {
                            newRing.push_back({x, y, coord[2]});
                        } else {
                            newRing.push_back({x, y});
                        }
                    }
                    newPolygon.push_back(newRing);
                }
                result["coordinates"].push_back(newPolygon);
            }
        }
        
        return result;
    }
    
    std::pair<double, double> transformPoint(double x, double y, int fromSrid, int toSrid) const {
        const auto& db = crs::getEPSGDatabase();
        
        auto fromIt = db.find(fromSrid);
        auto toIt = db.find(toSrid);
        
        if (fromIt == db.end()) {
            throw std::runtime_error("ST_TRANSFORM: Unknown source EPSG code: " + std::to_string(fromSrid));
        }
        if (toIt == db.end()) {
            throw std::runtime_error("ST_TRANSFORM: Unknown target EPSG code: " + std::to_string(toSrid));
        }
        
        const auto& fromDef = fromIt->second;
        const auto& toDef = toIt->second;
        
        // Step 1: Convert to geographic coordinates (on source ellipsoid)
        double lat, lon;
        if (fromDef.type == "geographic") {
            // Already geographic (assuming lon, lat order for GeoJSON)
            lon = x;
            lat = y;
        } else if (fromDef.type == "projected") {
            if (fromSrid == 3857) {
                // Web Mercator
                std::tie(lat, lon) = crs::webMercatorToGeographic(x, y);
            } else if ((fromDef.utmZone > 0 || (fromSrid >= 31466 && fromSrid <= 31469))) {
                // UTM or Gauß-Krüger
                crs::UTMZone zone(fromDef.utmZone > 0 ? fromDef.utmZone : 0, fromDef.utmNorth);
                zone.lon0 = fromDef.centralMeridian;
                zone.k0 = fromDef.scaleFactor;
                zone.falseE = fromDef.falseEasting;
                zone.falseN = fromDef.falseNorthing;
                std::tie(lat, lon) = crs::utmToGeographic(x, y, zone, fromDef.ellipsoid);
            } else {
                throw std::runtime_error("ST_TRANSFORM: Unsupported source projection");
            }
        }
        
        // Step 2: Datum transformation (if ellipsoids differ)
        if (fromDef.ellipsoid.name != toDef.ellipsoid.name) {
            // Convert to Cartesian, apply Helmert, convert back
            auto [cx, cy, cz] = crs::geographicToCartesian(lat, lon, 0, fromDef.ellipsoid);
            
            // Apply transformation (DHDN → ETRS89/WGS84)
            if (fromDef.ellipsoid.name == "Bessel 1841") {
                std::tie(cx, cy, cz) = crs::helmertTransform(
                    cx, cy, cz,
                    crs::DHDNToETRS89Params::dx,
                    crs::DHDNToETRS89Params::dy,
                    crs::DHDNToETRS89Params::dz,
                    crs::DHDNToETRS89Params::rx,
                    crs::DHDNToETRS89Params::ry,
                    crs::DHDNToETRS89Params::rz,
                    crs::DHDNToETRS89Params::s
                );
            }
            // TODO: Add more datum transformations as needed
            
            double h = {};
            std::tie(lat, lon, h) = crs::cartesianToGeographic(cx, cy, cz, toDef.ellipsoid);
        }
        
        // Step 3: Convert to target coordinates
        if (toDef.type == "geographic") {
            // Return as lon, lat (GeoJSON order)
            return {lon, lat};
        } else if (toDef.type == "projected") {
            if (toSrid == 3857) {
                return crs::geographicToWebMercator(lat, lon);
            } else if (toDef.utmZone > 0 || toSrid >= 31466 && toSrid <= 31469) {
                crs::UTMZone zone(toDef.utmZone > 0 ? toDef.utmZone : 0, toDef.utmNorth);
                zone.lon0 = toDef.centralMeridian;
                zone.k0 = toDef.scaleFactor;
                zone.falseE = toDef.falseEasting;
                zone.falseN = toDef.falseNorthing;
                return crs::geographicToUTM(lat, lon, zone, toDef.ellipsoid);
            }
        }
        
        throw std::runtime_error("ST_TRANSFORM: Unsupported target projection");
    }
};

/**
 * @brief ST_SRID(geometry) - Get or set SRID
 */
class StSridFunction : public IFunction {
public:
    ~StSridFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_SRID",
            "Geo",
            "Get or set the Spatial Reference ID (EPSG code) of a geometry",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "Input geometry"},
                {"srid", ArgType::INTEGER, false, nullptr, "New SRID to set (optional)"}
            },
            ArgType::ANY,  // Returns integer if getting, geometry if setting
            true,
            false,
            {"ST_SRID(point)  // Get SRID", "ST_SRID(point, 4326)  // Set SRID"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        const auto& geom = args[0];
        
        if (args.size() == 1) {
            // Get SRID
            if (geom.contains("crs") && geom["crs"].contains("properties") && 
                geom["crs"]["properties"].contains("name")) {
                std::string name = geom["crs"]["properties"]["name"];
                // Parse EPSG code from name like "EPSG:4326"
                // REL-21: wrap stoi() — the suffix after ':' may be non-numeric or
                // out-of-range; fall through to the default WGS84 (4326) on failure.
                size_t colonPos = name.find(':');
                if (colonPos != std::string::npos) {
                    try {
                        return std::stoi(name.substr(colonPos + 1));
                    } catch (const std::exception&) {
                        // Fall through to default 4326
                    }
                }
            }
            // Default to WGS84
            return 4326;
        } else {
            // Set SRID
            int srid = args[1].get<int>();
            nlohmann::json result = geom;
            result["crs"] = {
                {"type", "name"},
                {"properties", {
                    {"name", "EPSG:" + std::to_string(srid)}
                }}
            };
            return result;
        }
    }
};

/**
 * @brief ST_SETSRID(geometry, srid) - Set SRID without transformation
 */
class StSetSridFunction : public IFunction {
public:
    ~StSetSridFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_SETSRID",
            "Geo",
            "Set the SRID of a geometry without transforming coordinates",
            {
                {"geometry", ArgType::GEOMETRY, true, nullptr, "Input geometry"},
                {"srid", ArgType::INTEGER, true, nullptr, "EPSG code to assign"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_SETSRID(point, 25832)  // Mark point as UTM32N"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        nlohmann::json result = args[0];
        int srid = args[1].get<int>();
        result["crs"] = {
            {"type", "name"},
            {"properties", {
                {"name", "EPSG:" + std::to_string(srid)}
            }}
        };
        return result;
    }
};

/**
 * @brief UTM_ZONE(longitude) - Calculate UTM zone from longitude
 */
class UtmZoneFunction : public IFunction {
public:
    ~UtmZoneFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "UTM_ZONE",
            "Geo",
            "Calculate UTM zone number from longitude",
            {
                {"longitude", ArgType::NUMBER, true, nullptr, "Longitude in degrees"}
            },
            ArgType::INTEGER,
            true,
            false,
            {"UTM_ZONE(9.0)  // Returns 32 (Central Germany)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        double lon = args[0].get<double>();
        return crs::getUTMZone(lon);
    }
};

/**
 * @brief UTM_EPSG(zone, hemisphere, ellipsoid) - Get EPSG code for UTM zone
 */
class UtmEpsgFunction : public IFunction {
public:
    ~UtmEpsgFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "UTM_EPSG",
            "Geo",
            "Get EPSG code for a UTM zone",
            {
                {"zone", ArgType::INTEGER, true, nullptr, "UTM zone number (1-60)"},
                {"hemisphere", ArgType::STRING, false, nlohmann::json("N"), "'N' for north or 'S' for south"},
                {"ellipsoid", ArgType::STRING, false, nlohmann::json("WGS84"), "'WGS84' or 'ETRS89'"}
            },
            ArgType::INTEGER,
            true,
            false,
            {"UTM_EPSG(32)  // Returns 32632 (WGS84/UTM32N)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        int zone = args[0].get<int>();
        std::string hemisphere = args.size() > 1 ? args[1].get<std::string>() : "N";
        std::string ellipsoid = args.size() > 2 ? args[2].get<std::string>() : "WGS84";
        
        bool isNorth = (hemisphere == "N" || hemisphere == "n");
        bool isWGS84 = (ellipsoid == "WGS84" || ellipsoid == "wgs84");
        
        return crs::getUTMEpsg(zone, isNorth, isWGS84);
    }
};

/**
 * @brief CRS_NAME(epsg) - Get name for EPSG code
 */
class CrsNameFunction : public IFunction {
public:
    ~CrsNameFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "CRS_NAME",
            "Geo",
            "Get the name of a coordinate reference system by EPSG code",
            {
                {"epsg", ArgType::INTEGER, true, nullptr, "EPSG code"}
            },
            ArgType::STRING,
            true,
            false,
            {"CRS_NAME(25832)  // Returns 'ETRS89 / UTM zone 32N'"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        int epsg = args[0].get<int>();
        const auto& db = crs::getEPSGDatabase();
        
        auto it = db.find(epsg);
        if (it != db.end()) {
            return it->second.name;
        }
        return "Unknown CRS (EPSG:" + std::to_string(epsg) + ")";
    }
};

/**
 * @brief CRS_IS_GEOGRAPHIC(epsg) - Check if CRS is geographic
 */
class CrsIsGeographicFunction : public IFunction {
public:
    ~CrsIsGeographicFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "CRS_IS_GEOGRAPHIC",
            "Geo",
            "Check if a CRS uses geographic (lat/lon) coordinates",
            {
                {"epsg", ArgType::INTEGER, true, nullptr, "EPSG code"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"CRS_IS_GEOGRAPHIC(4326)  // Returns true"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        int epsg = args[0].get<int>();
        const auto& db = crs::getEPSGDatabase();
        
        auto it = db.find(epsg);
        if (it != db.end()) {
            return it->second.type == "geographic";
        }
        return false;
    }
};

/**
 * @brief CRS_IS_PROJECTED(epsg) - Check if CRS is projected
 */
class CrsIsProjectedFunction : public IFunction {
public:
    ~CrsIsProjectedFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "CRS_IS_PROJECTED",
            "Geo",
            "Check if a CRS uses projected (meter) coordinates",
            {
                {"epsg", ArgType::INTEGER, true, nullptr, "EPSG code"}
            },
            ArgType::BOOLEAN,
            true,
            false,
            {"CRS_IS_PROJECTED(25832)  // Returns true"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        int epsg = args[0].get<int>();
        const auto& db = crs::getEPSGDatabase();
        
        auto it = db.find(epsg);
        if (it != db.end()) {
            return it->second.type == "projected";
        }
        return false;
    }
};

/**
 * @brief ST_MAKEPOINT_UTM(easting, northing, zone, hemisphere) - Create point from UTM
 */
class StMakePointUtmFunction : public IFunction {
public:
    ~StMakePointUtmFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ST_MAKEPOINT_UTM",
            "Geo",
            "Create a WGS84 Point from UTM coordinates",
            {
                {"easting", ArgType::NUMBER, true, nullptr, "Easting in meters"},
                {"northing", ArgType::NUMBER, true, nullptr, "Northing in meters"},
                {"zone", ArgType::INTEGER, true, nullptr, "UTM zone (1-60)"},
                {"hemisphere", ArgType::STRING, false, nlohmann::json("N"), "'N' or 'S'"}
            },
            ArgType::GEOMETRY,
            true,
            false,
            {"ST_MAKEPOINT_UTM(500000, 5500000, 32, 'N')  // Create point from UTM32N"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        double easting = args[0].get<double>();
        double northing = args[1].get<double>();
        int zone = args[2].get<int>();
        std::string hemisphere = args.size() > 3 ? args[3].get<std::string>() : "N";
        
        bool isNorth = (hemisphere == "N" || hemisphere == "n");
        crs::UTMZone utmZone(zone, isNorth);
        
        auto [lat, lon] = crs::utmToGeographic(easting, northing, utmZone, crs::WGS84_ELLIPSOID);
        
        nlohmann::json result;
        result["type"] = "Point";
        result["coordinates"] = {lon, lat};
        return result;
    }
};

// ============================================================================
// Registration Function
// ============================================================================

/**
 * @brief Register all CRS transformation functions with the registry
 */
inline void registerCrsFunctions(FunctionRegistry& registry) {
    registry.registerFunction(std::make_unique<StTransformFunction>());
    registry.registerFunction(std::make_unique<StSridFunction>());
    registry.registerFunction(std::make_unique<StSetSridFunction>());
    registry.registerFunction(std::make_unique<UtmZoneFunction>());
    registry.registerFunction(std::make_unique<UtmEpsgFunction>());
    registry.registerFunction(std::make_unique<CrsNameFunction>());
    registry.registerFunction(std::make_unique<CrsIsGeographicFunction>());
    registry.registerFunction(std::make_unique<CrsIsProjectedFunction>());
    registry.registerFunction(std::make_unique<StMakePointUtmFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis


