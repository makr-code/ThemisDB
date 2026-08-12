#include <gtest/gtest.h>
#include "utils/geo/validator.h"
#include <limits>
#include <cmath>

using namespace themis::geo;

// ============================================================================
// Coordinate Validation Tests
// ============================================================================

TEST(GeoValidator, ValidLatitude) {
    EXPECT_TRUE(GeoValidator::isValidLatitude(0.0));
    EXPECT_TRUE(GeoValidator::isValidLatitude(45.0));
    EXPECT_TRUE(GeoValidator::isValidLatitude(-45.0));
    EXPECT_TRUE(GeoValidator::isValidLatitude(90.0));
    EXPECT_TRUE(GeoValidator::isValidLatitude(-90.0));
}

TEST(GeoValidator, InvalidLatitude) {
    EXPECT_FALSE(GeoValidator::isValidLatitude(91.0));
    EXPECT_FALSE(GeoValidator::isValidLatitude(-91.0));
    EXPECT_FALSE(GeoValidator::isValidLatitude(180.0));
    EXPECT_FALSE(GeoValidator::isValidLatitude(std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(GeoValidator::isValidLatitude(-std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(GeoValidator::isValidLatitude(std::numeric_limits<double>::quiet_NaN()));
}

TEST(GeoValidator, ValidLongitude) {
    EXPECT_TRUE(GeoValidator::isValidLongitude(0.0));
    EXPECT_TRUE(GeoValidator::isValidLongitude(90.0));
    EXPECT_TRUE(GeoValidator::isValidLongitude(-90.0));
    EXPECT_TRUE(GeoValidator::isValidLongitude(180.0));
    EXPECT_TRUE(GeoValidator::isValidLongitude(-180.0));
}

TEST(GeoValidator, InvalidLongitude) {
    EXPECT_FALSE(GeoValidator::isValidLongitude(181.0));
    EXPECT_FALSE(GeoValidator::isValidLongitude(-181.0));
    EXPECT_FALSE(GeoValidator::isValidLongitude(360.0));
    EXPECT_FALSE(GeoValidator::isValidLongitude(std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(GeoValidator::isValidLongitude(-std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(GeoValidator::isValidLongitude(std::numeric_limits<double>::quiet_NaN()));
}

TEST(GeoValidator, ValidCoordinatePair) {
    EXPECT_TRUE(GeoValidator::isValidCoordinate(0.0, 0.0));
    EXPECT_TRUE(GeoValidator::isValidCoordinate(-73.935242, 40.730610));  // NYC
    EXPECT_TRUE(GeoValidator::isValidCoordinate(13.404954, 52.520008));   // Berlin
    EXPECT_TRUE(GeoValidator::isValidCoordinate(139.691706, 35.689487));  // Tokyo
}

TEST(GeoValidator, InvalidCoordinatePair) {
    EXPECT_FALSE(GeoValidator::isValidCoordinate(200.0, 45.0));   // Invalid longitude
    EXPECT_FALSE(GeoValidator::isValidCoordinate(45.0, 100.0));   // Invalid latitude
    EXPECT_FALSE(GeoValidator::isValidCoordinate(200.0, 100.0));  // Both invalid
}

// ============================================================================
// Coordinate Sanitization Tests
// ============================================================================

TEST(GeoValidator, SanitizeLatitude) {
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLatitude(0.0), 0.0);
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLatitude(45.0), 45.0);
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLatitude(100.0), 90.0);   // Clamped to max
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLatitude(-100.0), -90.0); // Clamped to min
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLatitude(std::numeric_limits<double>::infinity()), 0.0);
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLatitude(std::numeric_limits<double>::quiet_NaN()), 0.0);
}

TEST(GeoValidator, SanitizeLongitude) {
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLongitude(0.0), 0.0);
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLongitude(90.0), 90.0);
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLongitude(200.0), 180.0);   // Clamped to max
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLongitude(-200.0), -180.0); // Clamped to min
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLongitude(std::numeric_limits<double>::infinity()), 0.0);
    EXPECT_DOUBLE_EQ(GeoValidator::sanitizeLongitude(std::numeric_limits<double>::quiet_NaN()), 0.0);
}

// ============================================================================
// Geometry Size Validation Tests
// ============================================================================

TEST(GeoValidator, ValidGeometrySize) {
    EXPECT_NO_THROW(GeoValidator::validateGeometrySize(100));
    EXPECT_NO_THROW(GeoValidator::validateGeometrySize(1024));
    EXPECT_NO_THROW(GeoValidator::validateGeometrySize(1024 * 1024));  // 1MB
}

TEST(GeoValidator, InvalidGeometrySize) {
    // 10MB + 1 should exceed limit
    EXPECT_THROW(
        GeoValidator::validateGeometrySize(GeoValidator::MAX_GEOMETRY_SIZE_BYTES + 1),
        std::invalid_argument
    );
    
    // 100MB should definitely exceed limit
    EXPECT_THROW(
        GeoValidator::validateGeometrySize(100 * 1024 * 1024),
        std::invalid_argument
    );
}

// ============================================================================
// Coordinate Count Validation Tests
// ============================================================================

TEST(GeoValidator, ValidCoordinateCount) {
    EXPECT_NO_THROW(GeoValidator::validateCoordinateCount(10));
    EXPECT_NO_THROW(GeoValidator::validateCoordinateCount(1000));
    EXPECT_NO_THROW(GeoValidator::validateCoordinateCount(10000));
}

TEST(GeoValidator, InvalidCoordinateCount) {
    EXPECT_THROW(
        GeoValidator::validateCoordinateCount(GeoValidator::MAX_COORDINATES + 1),
        std::invalid_argument
    );
    
    EXPECT_THROW(
        GeoValidator::validateCoordinateCount(1000000),  // 1 million
        std::invalid_argument
    );
}

// ============================================================================
// Validation with Exceptions Tests
// ============================================================================

TEST(GeoValidator, ValidateCoordinateOrThrowSuccess) {
    EXPECT_NO_THROW(GeoValidator::validateCoordinateOrThrow(0.0, 0.0));
    EXPECT_NO_THROW(GeoValidator::validateCoordinateOrThrow(-73.935242, 40.730610));  // NYC
    EXPECT_NO_THROW(GeoValidator::validateCoordinateOrThrow(180.0, 90.0));  // Max values
    EXPECT_NO_THROW(GeoValidator::validateCoordinateOrThrow(-180.0, -90.0));  // Min values
}

TEST(GeoValidator, ValidateCoordinateOrThrowInfinite) {
    EXPECT_THROW(
        GeoValidator::validateCoordinateOrThrow(std::numeric_limits<double>::infinity(), 0.0),
        std::invalid_argument
    );
    
    EXPECT_THROW(
        GeoValidator::validateCoordinateOrThrow(0.0, std::numeric_limits<double>::infinity()),
        std::invalid_argument
    );
    
    EXPECT_THROW(
        GeoValidator::validateCoordinateOrThrow(std::numeric_limits<double>::quiet_NaN(), 0.0),
        std::invalid_argument
    );
}

TEST(GeoValidator, ValidateCoordinateOrThrowOutOfRange) {
    EXPECT_THROW(
        GeoValidator::validateCoordinateOrThrow(200.0, 0.0),
        std::invalid_argument
    );
    
    EXPECT_THROW(
        GeoValidator::validateCoordinateOrThrow(0.0, 100.0),
        std::invalid_argument
    );
    
    EXPECT_THROW(
        GeoValidator::validateCoordinateOrThrow(-200.0, -100.0),
        std::invalid_argument
    );
}

// ============================================================================
// Finite Number Check Tests
// ============================================================================

TEST(GeoValidator, IsFiniteCheck) {
    EXPECT_TRUE(GeoValidator::isFinite(0.0));
    EXPECT_TRUE(GeoValidator::isFinite(1.0));
    EXPECT_TRUE(GeoValidator::isFinite(-1.0));
    EXPECT_TRUE(GeoValidator::isFinite(std::numeric_limits<double>::max()));
    EXPECT_TRUE(GeoValidator::isFinite(std::numeric_limits<double>::lowest()));
    
    EXPECT_FALSE(GeoValidator::isFinite(std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(GeoValidator::isFinite(-std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(GeoValidator::isFinite(std::numeric_limits<double>::quiet_NaN()));
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

TEST(GeoValidator, BoundaryValues) {
    // Test exact boundary values
    EXPECT_TRUE(GeoValidator::isValidLatitude(90.0));
    EXPECT_TRUE(GeoValidator::isValidLatitude(-90.0));
    EXPECT_FALSE(GeoValidator::isValidLatitude(90.0 + 1e-10));  // Just beyond boundary
    
    EXPECT_TRUE(GeoValidator::isValidLongitude(180.0));
    EXPECT_TRUE(GeoValidator::isValidLongitude(-180.0));
    EXPECT_FALSE(GeoValidator::isValidLongitude(180.0 + 1e-10));  // Just beyond boundary
}

TEST(GeoValidator, ZeroValues) {
    EXPECT_TRUE(GeoValidator::isValidLatitude(0.0));
    EXPECT_TRUE(GeoValidator::isValidLongitude(0.0));
    EXPECT_TRUE(GeoValidator::isValidCoordinate(0.0, 0.0));
}

TEST(GeoValidator, VerySmallValues) {
    double tiny = 1e-15;
    EXPECT_TRUE(GeoValidator::isValidLatitude(tiny));
    EXPECT_TRUE(GeoValidator::isValidLongitude(tiny));
    EXPECT_TRUE(GeoValidator::isValidCoordinate(tiny, tiny));
}
