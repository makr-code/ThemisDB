#include <gtest/gtest.h>

#include "geo/geo_math.h"

#include <cmath>

namespace {

TEST(GeoMathBoundaryTest, AntimeridianDistanceUsesShortArcInBothDirections) {
    const double expected = themis::geo::kEarthRadiusM * themis::geo::kPi / 180.0;
    const double east = themis::geo::haversineDistanceM(179.5, 0.0, -179.5, 0.0);
    const double west = themis::geo::haversineDistanceM(-179.5, 0.0, 179.5, 0.0);
    ASSERT_TRUE(std::isfinite(east));
    EXPECT_NEAR(east, expected, 1e-6);
    EXPECT_NEAR(west, expected, 1e-6);
    EXPECT_NEAR(themis::geo::haversineDistanceKm(179.5, 0.0, -179.5, 0.0),
                expected / 1000.0, 1e-9);
}

TEST(GeoMathBoundaryTest, AntimeridianBearingsAreNormalizedInTravelDirection) {
    EXPECT_NEAR(themis::geo::bearingDeg(179.5, 0.0, -179.5, 0.0), 90.0, 1e-9);
    EXPECT_NEAR(themis::geo::bearingDeg(-179.5, 0.0, 179.5, 0.0), 270.0, 1e-9);
}

TEST(GeoMathBoundaryTest, PoleToPoleDistanceIsHalfEarthCircumference) {
    const auto distance = themis::geo::haversineDistanceM(0.0, 90.0, 0.0, -90.0);
    ASSERT_TRUE(std::isfinite(distance));
    EXPECT_NEAR(distance, themis::geo::kPi * themis::geo::kEarthRadiusM, 1e-6);
}

} // namespace
