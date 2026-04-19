/*
 * Test suite: GeoJSON Geometry Type API (geo_json_geometry.h)
 *
 * Tests: GJS-01 … GJS-12
 *
 * Covers:
 *  GJS-01  GeoPoint: construction and toGeoJSON
 *  GJS-02  GeoPoint: validate() — valid WGS-84 coordinate
 *  GJS-03  GeoPoint: validate() — NaN/infinity → NON_FINITE_COORDINATE
 *  GJS-04  GeoPoint: validate() — longitude out of WGS-84 range
 *  GJS-05  GeoLineString: bbox() and validate() — valid 3-point line
 *  GJS-06  GeoLineString: validate() — single-point (invalid)
 *  GJS-07  GeoPolygon: construction, bbox(), toGeoJSON
 *  GJS-08  GeoPolygon: validate() — right-hand-rule CCW exterior ring passes
 *  GJS-09  GeoPolygon: validate() — CW exterior ring → WINDING_ORDER_VIOLATION
 *  GJS-10  GeoPolygon: validate() — unclosed ring → RING_NOT_CLOSED
 *  GJS-11  GeoMultiPolygon: bbox() spans all sub-polygons
 *  GJS-12  GeoGeometryCollection: validate() propagates member errors
 */

#include <gtest/gtest.h>
#include "geo/geo_json_geometry.h"

#include <cmath>
#include <memory>
#include <string>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Build a simple CCW unit-square polygon ring (exterior, counter-clockwise).
GeoPolygon::Ring ccwSquare() {
    return {
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}
    };
}

/// Build a CW unit-square polygon ring (clockwise — invalid exterior).
GeoPolygon::Ring cwSquare() {
    return {
        {0.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}, {0.0, 0.0}
    };
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// GJS-01 GeoPoint: construction and toGeoJSON
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS01_GeoPoint_ToGeoJson) {
    GeoPoint p({13.4050, 52.5200}, CrsId::WGS84);
    EXPECT_EQ(p.type(), "Point");
    EXPECT_EQ(p.crs(), CrsId::WGS84);

    const auto json = p.toGeoJSON();
    EXPECT_NE(json.find("\"type\":\"Point\""), std::string::npos);
    EXPECT_NE(json.find("13.405"), std::string::npos);
    EXPECT_NE(json.find("52.52"), std::string::npos);
}

// ---------------------------------------------------------------------------
// GJS-02 GeoPoint: validate() — valid WGS-84 coordinate
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS02_GeoPoint_Validate_Valid) {
    GeoPoint p({13.4050, 52.5200}, CrsId::WGS84);
    const auto vr = p.validate();
    EXPECT_TRUE(vr.ok()) << "Expected no errors for valid Berlin coordinate";
}

// ---------------------------------------------------------------------------
// GJS-03 GeoPoint: validate() — NaN coordinate → NON_FINITE_COORDINATE
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS03_GeoPoint_Validate_NaN) {
    GeoPoint p({std::numeric_limits<double>::quiet_NaN(), 52.52}, CrsId::WGS84);
    const auto vr = p.validate();
    EXPECT_FALSE(vr.ok());
    ASSERT_FALSE(vr.errors().empty());
    EXPECT_EQ(vr.errors()[0].code, "NON_FINITE_COORDINATE");
}

// ---------------------------------------------------------------------------
// GJS-04 GeoPoint: validate() — longitude out of WGS-84 range
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS04_GeoPoint_Validate_LongitudeOutOfRange) {
    GeoPoint p({200.0, 52.52}, CrsId::WGS84);
    const auto vr = p.validate();
    EXPECT_FALSE(vr.ok());
    bool found = false;
    for (const auto& e : vr.errors()) {
        if (e.code == "LONGITUDE_OUT_OF_RANGE") { found = true; break; }
    }
    EXPECT_TRUE(found) << "Expected LONGITUDE_OUT_OF_RANGE for x=200";
}

// ---------------------------------------------------------------------------
// GJS-05 GeoLineString: bbox() and validate() — valid 3-point line
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS05_GeoLineString_BboxAndValidate) {
    GeoLineString ls({{{0.0, 0.0}, {5.0, 0.0}, {5.0, 5.0}}}, CrsId::EPSG3857);
    EXPECT_EQ(ls.type(), "LineString");
    const auto bb = ls.bbox();
    EXPECT_DOUBLE_EQ(bb.min_x, 0.0);
    EXPECT_DOUBLE_EQ(bb.max_x, 5.0);
    EXPECT_DOUBLE_EQ(bb.min_y, 0.0);
    EXPECT_DOUBLE_EQ(bb.max_y, 5.0);
    EXPECT_TRUE(ls.validate().ok());
}

// ---------------------------------------------------------------------------
// GJS-06 GeoLineString: validate() — single-position (invalid)
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS06_GeoLineString_Validate_TooFewPoints) {
    GeoLineString ls({{{1.0, 2.0}}}, CrsId::WGS84);
    const auto vr = ls.validate();
    EXPECT_FALSE(vr.ok());
    ASSERT_FALSE(vr.errors().empty());
    EXPECT_EQ(vr.errors()[0].code, "INSUFFICIENT_POSITIONS");
}

// ---------------------------------------------------------------------------
// GJS-07 GeoPolygon: construction, bbox(), toGeoJSON
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS07_GeoPolygon_ConstructionAndBbox) {
    GeoPolygon poly({ccwSquare()}, CrsId::WGS84);
    EXPECT_EQ(poly.type(), "Polygon");
    const auto bb = poly.bbox();
    EXPECT_DOUBLE_EQ(bb.min_x, 0.0);
    EXPECT_DOUBLE_EQ(bb.max_x, 1.0);
    EXPECT_DOUBLE_EQ(bb.min_y, 0.0);
    EXPECT_DOUBLE_EQ(bb.max_y, 1.0);

    const auto json = poly.toGeoJSON();
    EXPECT_NE(json.find("\"type\":\"Polygon\""), std::string::npos);
    EXPECT_NE(json.find("\"coordinates\""), std::string::npos);
}

// ---------------------------------------------------------------------------
// GJS-08 GeoPolygon: validate() — CCW exterior ring passes
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS08_GeoPolygon_Validate_CCW_Passes) {
    GeoPolygon poly({ccwSquare()}, CrsId::WGS84);
    const auto vr = poly.validate();
    // Note: WGS-84 range check passes for coords in [0,1]
    EXPECT_TRUE(vr.ok()) << "CCW exterior ring should pass winding-order check";
}

// ---------------------------------------------------------------------------
// GJS-09 GeoPolygon: validate() — CW exterior ring → WINDING_ORDER_VIOLATION
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS09_GeoPolygon_Validate_CW_ExteriorRing) {
    GeoPolygon poly({cwSquare()}, CrsId::WGS84);
    const auto vr = poly.validate();
    EXPECT_FALSE(vr.ok());
    bool found = false;
    for (const auto& e : vr.errors()) {
        if (e.code == "WINDING_ORDER_VIOLATION") { found = true; break; }
    }
    EXPECT_TRUE(found) << "Expected WINDING_ORDER_VIOLATION for CW exterior ring";
}

// ---------------------------------------------------------------------------
// GJS-10 GeoPolygon: validate() — unclosed ring → RING_NOT_CLOSED
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS10_GeoPolygon_Validate_UnclosedRing) {
    GeoPolygon::Ring unclosed = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};
    GeoPolygon poly({unclosed}, CrsId::WGS84);
    const auto vr = poly.validate();
    EXPECT_FALSE(vr.ok());
    bool found = false;
    for (const auto& e : vr.errors()) {
        if (e.code == "RING_NOT_CLOSED") { found = true; break; }
    }
    EXPECT_TRUE(found) << "Expected RING_NOT_CLOSED for unclosed ring";
}

// ---------------------------------------------------------------------------
// GJS-11 GeoMultiPolygon: bbox() spans all sub-polygons
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS11_GeoMultiPolygon_BboxSpansAll) {
    GeoPolygon::Ring ring1 = {{0.0,0.0},{2.0,0.0},{2.0,2.0},{0.0,2.0},{0.0,0.0}};
    GeoPolygon::Ring ring2 = {{5.0,5.0},{7.0,5.0},{7.0,7.0},{5.0,7.0},{5.0,5.0}};
    GeoMultiPolygon mp(
        {GeoPolygon({ring1}, CrsId::WGS84), GeoPolygon({ring2}, CrsId::WGS84)},
        CrsId::WGS84);

    EXPECT_EQ(mp.type(), "MultiPolygon");
    const auto bb = mp.bbox();
    EXPECT_DOUBLE_EQ(bb.min_x, 0.0);
    EXPECT_DOUBLE_EQ(bb.max_x, 7.0);
    EXPECT_DOUBLE_EQ(bb.min_y, 0.0);
    EXPECT_DOUBLE_EQ(bb.max_y, 7.0);
}

// ---------------------------------------------------------------------------
// GJS-12 GeoGeometryCollection: validate() propagates member errors
// ---------------------------------------------------------------------------

TEST(GeoJsonGeometry, GJS12_GeoGeometryCollection_PropagatesErrors) {
    // Good point
    auto good = std::make_shared<GeoPoint>(Coordinate{10.0, 50.0}, CrsId::WGS84);
    // Bad point (latitude out of range)
    auto bad  = std::make_shared<GeoPoint>(Coordinate{10.0, 200.0}, CrsId::WGS84);

    GeoGeometryCollection coll({good, bad}, CrsId::WGS84);
    EXPECT_EQ(coll.type(), "GeometryCollection");

    const auto vr = coll.validate();
    EXPECT_FALSE(vr.ok()) << "Collection with invalid member should fail validation";
    bool found = false;
    for (const auto& e : vr.errors()) {
        if (e.code.find("LATITUDE_OUT_OF_RANGE") != std::string::npos) {
            found = true; break;
        }
    }
    EXPECT_TRUE(found) << "Expected propagated LATITUDE_OUT_OF_RANGE in collection";
}
