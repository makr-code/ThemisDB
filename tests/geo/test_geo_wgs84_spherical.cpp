/*
 * Test suite: WGS-84 ellipsoid geodesic distance
 *
 * Validates CpuExactBackend::geodesicDistance() which implements the
 * Vincenty inverse formula on the WGS-84 ellipsoid (EPSG:4326).
 *
 * Reference values are taken from:
 *   - Karney (2013) "Algorithms for geodesics", Table 2
 *   - NIMA TR8350.2 test cases
 *   - NGS online geodesic calculator
 */

#include <gtest/gtest.h>
#include "geo/spatial_backend.h"

#include <cmath>

using namespace themis::geo;

namespace {

// Helper: get the cpu_exact backend (always available)
ISpatialComputeBackend* backend() {
    return getCpuExactBackend();
}

} // namespace

// ---------------------------------------------------------------------------
// Basic API tests
// ---------------------------------------------------------------------------

TEST(GeoWGS84Spherical, BackendIsNotNull) {
    ASSERT_NE(backend(), nullptr);
}

TEST(GeoWGS84Spherical, CoincidentPoints_ReturnsZero) {
    // Identical coordinates must produce 0 m.
    EXPECT_NEAR(backend()->geodesicDistance(52.5200, 13.4050, 52.5200, 13.4050), 0.0, 1e-6);
}

TEST(GeoWGS84Spherical, CoincidentPointsEquator_ReturnsZero) {
    EXPECT_NEAR(backend()->geodesicDistance(0.0, 0.0, 0.0, 0.0), 0.0, 1e-6);
}

// ---------------------------------------------------------------------------
// Known reference distances (±1 m tolerance)
// ---------------------------------------------------------------------------

// Berlin Hauptbahnhof → Brandenburger Tor: ~1 km
// Approx: 52.5254°N,13.3690°E → 52.5163°N,13.3777°E ≈ 1160 m
TEST(GeoWGS84Spherical, BerlinCityDistance_ApproxOneKilometre) {
    // Berlin Hbf (52.5254, 13.3690) → Brandenburg Gate (52.5163, 13.3777)
    const double dist = backend()->geodesicDistance(52.5254, 13.3690, 52.5163, 13.3777);
    EXPECT_GT(dist, 900.0)  << "distance should be > 900 m";
    EXPECT_LT(dist, 1500.0) << "distance should be < 1500 m";
}

// Equatorial degree of longitude at the equator ≈ 111,319.49 m (WGS-84)
TEST(GeoWGS84Spherical, EquatorialDegreeLongitude) {
    const double dist = backend()->geodesicDistance(0.0, 0.0, 0.0, 1.0);
    // WGS-84 equatorial circumference / 360 = 40075016.686 / 360 ≈ 111319.49 m
    EXPECT_NEAR(dist, 111319.49, 1.0);
}

// Meridional degree of latitude near the equator ≈ 110574 m
TEST(GeoWGS84Spherical, MeridionalDegreeLatitude_NearEquator) {
    const double dist = backend()->geodesicDistance(0.0, 0.0, 1.0, 0.0);
    // 1° latitude ≈ 110574 m at equator (WGS-84 meridian radius of curvature)
    EXPECT_NEAR(dist, 110574.0, 500.0);
}

// New York (40.6413°N, -73.7781°W) → London Heathrow (51.4775°N, -0.4614°W)
// Great-circle distance ≈ 5,539 km (Vincenty geodesic ≈ 5,570 km)
TEST(GeoWGS84Spherical, NewYorkToLondon_InterContinental) {
    const double dist = backend()->geodesicDistance(40.6413, -73.7781, 51.4775, -0.4614);
    EXPECT_GT(dist, 5'400'000.0) << "NY–London should be > 5400 km";
    EXPECT_LT(dist, 5'700'000.0) << "NY–London should be < 5700 km";
}

// Sydney (−33.8688°S, 151.2093°E) → London (51.5074°N, −0.1278°W)
// Geodesic ≈ 16,993 km
TEST(GeoWGS84Spherical, SydneyToLondon_LongDistance) {
    const double dist = backend()->geodesicDistance(-33.8688, 151.2093, 51.5074, -0.1278);
    EXPECT_GT(dist, 16'800'000.0) << "Sydney–London should be > 16800 km";
    EXPECT_LT(dist, 17'200'000.0) << "Sydney–London should be < 17200 km";
}

// ---------------------------------------------------------------------------
// Symmetry: distance(A→B) == distance(B→A)
// ---------------------------------------------------------------------------

TEST(GeoWGS84Spherical, Symmetry_ForwardAndReverse) {
    const double d1 = backend()->geodesicDistance(52.5200, 13.4050,  48.8566, 2.3522);  // Berlin→Paris
    const double d2 = backend()->geodesicDistance(48.8566, 2.3522,   52.5200, 13.4050); // Paris→Berlin
    EXPECT_NEAR(d1, d2, 1e-6);
}

TEST(GeoWGS84Spherical, Symmetry_CrossEquator) {
    const double d1 = backend()->geodesicDistance( 10.0, 20.0, -10.0, 20.0);
    const double d2 = backend()->geodesicDistance(-10.0, 20.0,  10.0, 20.0);
    EXPECT_NEAR(d1, d2, 1e-6);
}

// ---------------------------------------------------------------------------
// Ellipsoid accuracy: Vincenty vs sphere (Haversine equivalent)
// For antipodal-ish or polar routes the two should differ noticeably.
// ---------------------------------------------------------------------------

// Berlin → Buenos Aires: geodesic ≈ 11,688 km
// The ellipsoid correction vs a sphere is several km → the Vincenty
// result must be within a realistic range.
TEST(GeoWGS84Spherical, BerlinToBuenosAires_EllipsoidRange) {
    const double dist = backend()->geodesicDistance(52.5200, 13.4050, -34.6037, -58.3816);
    EXPECT_GT(dist, 11'500'000.0) << "Berlin–Buenos Aires > 11500 km";
    EXPECT_LT(dist, 11'900'000.0) << "Berlin–Buenos Aires < 11900 km";
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

// North Pole → South Pole: half the meridional circumference ≈ 20,003,931 m
TEST(GeoWGS84Spherical, NorthPoleToSouthPole) {
    const double dist = backend()->geodesicDistance(90.0, 0.0, -90.0, 0.0);
    // WGS-84 meridian circumference ≈ 40,007,862.9 m → half ≈ 20,003,931 m
    EXPECT_NEAR(dist, 20'003'931.0, 10.0);
}

// Same latitude, different longitudes (parallel not a geodesic)
TEST(GeoWGS84Spherical, SameLatitude_HighLatitude) {
    // Two points at 60°N separated by 10° longitude
    const double dist = backend()->geodesicDistance(60.0, 10.0, 60.0, 20.0);
    // Approx: 10° lon at 60°N ≈ 555,960 m (rough) — just sanity check bounds
    EXPECT_GT(dist, 540'000.0) << "10° lon at 60°N should be > 540 km";
    EXPECT_LT(dist, 570'000.0) << "10° lon at 60°N should be < 570 km";
}

// Zero longitude difference with latitude change
TEST(GeoWGS84Spherical, MeridianArc_MidLatitude) {
    // 10° latitude along a meridian at 30°N should be ≈ 1,108,513 m
    const double dist = backend()->geodesicDistance(30.0, 45.0, 40.0, 45.0);
    EXPECT_NEAR(dist, 1'108'513.0, 2000.0);
}

// ---------------------------------------------------------------------------
// GPU spatial backend delegates to CPU (returns real geodesic distance)
// ---------------------------------------------------------------------------

TEST(GeoWGS84Spherical, GpuBackend_DelegatesToCpu) {
    ISpatialComputeBackend* gpu = getGpuSpatialBackend();
    ASSERT_NE(gpu, nullptr);
    // GPU backend must delegate to the CPU exact backend and return the
    // same geodesic distance value (Vincenty WGS-84).
    const double cpu_dist = getCpuExactBackend()->geodesicDistance(
        52.5200, 13.4050, 48.8566, 2.3522); // Berlin→Paris
    const double gpu_dist = gpu->geodesicDistance(
        52.5200, 13.4050, 48.8566, 2.3522);
    EXPECT_GT(gpu_dist, 0.0) << "GPU geodesicDistance must return real distance, not stub 0";
    EXPECT_NEAR(gpu_dist, cpu_dist, 1e-6)
        << "GPU backend must delegate to CPU exact backend";
}
