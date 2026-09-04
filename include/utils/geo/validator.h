/**
 * @file validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cmath>
#include <string>
#include <stdexcept>

namespace themis {
namespace geo {

/**
 * @brief Geo Coordinate Validator
 * 
 * Provides validation for geographic coordinates to prevent
 * invalid data from entering the spatial index.
 */
class GeoValidator {
public:
    // WGS84 coordinate bounds
    static constexpr double MIN_LATITUDE = -90.0;
    static constexpr double MAX_LATITUDE = 90.0;
    static constexpr double MIN_LONGITUDE = -180.0;
    static constexpr double MAX_LONGITUDE = 180.0;
    
    // Validation limits for security
    static constexpr size_t MAX_GEOMETRY_SIZE_BYTES = 10 * 1024 * 1024;  // 10MB
    static constexpr size_t MAX_COORDINATES = 100000;  // Max points in a geometry
    
    /**
     * @brief Validate latitude value
     * @param lat Latitude in degrees
     * @return true if valid, false otherwise
     */
    static bool isValidLatitude(double lat) {
        if (!std::isfinite(lat)) {
            return false;
        }
        return lat >= MIN_LATITUDE && lat <= MAX_LATITUDE;
    }
    
    /**
     * @brief Validate longitude value
     * @param lon Longitude in degrees
     * @return true if valid, false otherwise
     */
    static bool isValidLongitude(double lon) {
        if (!std::isfinite(lon)) {
            return false;
        }
        return lon >= MIN_LONGITUDE && lon <= MAX_LONGITUDE;
    }
    
    /**
     * @brief Validate coordinate pair
     * @param lon Longitude in degrees
     * @param lat Latitude in degrees
     * @return true if both coordinates are valid
     */
    static bool isValidCoordinate(double lon, double lat) {
        return isValidLongitude(lon) && isValidLatitude(lat);
    }
    
    /**
     * @brief Validate geometry size
     * @param size_bytes Size of geometry in bytes
     * @throws std::invalid_argument if size exceeds limit
     */
    static void validateGeometrySize(size_t size_bytes) {
        if (size_bytes > MAX_GEOMETRY_SIZE_BYTES) {
            throw std::invalid_argument(
                "Geometry size " + std::to_string(size_bytes) + 
                " bytes exceeds maximum allowed size of " + 
                std::to_string(MAX_GEOMETRY_SIZE_BYTES) + " bytes"
            );
        }
    }
    
    /**
     * @brief Validate coordinate count
     * @param count Number of coordinates
     * @throws std::invalid_argument if count exceeds limit
     */
    static void validateCoordinateCount(size_t count) {
        if (count > MAX_COORDINATES) {
            throw std::invalid_argument(
                "Coordinate count " + std::to_string(count) + 
                " exceeds maximum allowed count of " + 
                std::to_string(MAX_COORDINATES)
            );
        }
    }
    
    /**
     * @brief Sanitize latitude to valid range (clamp)
     * @param lat Latitude to sanitize
     * @return Clamped latitude value
     */
    static double sanitizeLatitude(double lat) {
        if (!std::isfinite(lat)) {
            return 0.0;
        }
        if (lat < MIN_LATITUDE) {
          return MIN_LATITUDE;
        }
        if (lat > MAX_LATITUDE) {
          return MAX_LATITUDE;
        }
        return lat;
    }
    
    /**
     * @brief Sanitize longitude to valid range (clamp)
     * @param lon Longitude to sanitize
     * @return Clamped longitude value
     */
    static double sanitizeLongitude(double lon) {
        if (!std::isfinite(lon)) {
            return 0.0;
        }
        if (lon < MIN_LONGITUDE) {
          return MIN_LONGITUDE;
        }
        if (lon > MAX_LONGITUDE) {
          return MAX_LONGITUDE;
        }
        return lon;
    }
    
    /**
     * @brief Check if a number is NaN or infinite
     * @param value Value to check
     * @return true if value is finite
     */
    static bool isFinite(double value) {
        return std::isfinite(value);
    }
    
    /**
     * @brief Validate and throw on invalid coordinates
     * @param lon Longitude
     * @param lat Latitude
     * @throws std::invalid_argument if coordinates are invalid
     */
    static void validateCoordinateOrThrow(double lon, double lat) {
        if (!isFinite(lon)) {
            throw std::invalid_argument("Longitude is not a finite number");
        }
        if (!isFinite(lat)) {
            throw std::invalid_argument("Latitude is not a finite number");
        }
        if (!isValidLongitude(lon)) {
            throw std::invalid_argument(
                "Longitude " + std::to_string(lon) + 
                " is outside valid range [" + 
                std::to_string(MIN_LONGITUDE) + ", " + 
                std::to_string(MAX_LONGITUDE) + "]"
            );
        }
        if (!isValidLatitude(lat)) {
            throw std::invalid_argument(
                "Latitude " + std::to_string(lat) + 
                " is outside valid range [" + 
                std::to_string(MIN_LATITUDE) + ", " + 
                std::to_string(MAX_LATITUDE) + "]"
            );
        }
    }
};

} // namespace geo
} // namespace themis
