#include <gtest/gtest.h>
#include "utils/geo/ewkb.h"
#include <cmath>

using namespace themis::geo;

class EWKBTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    
    bool approxEqual(double a, double b, double epsilon = 1e-9) {
        return std::fabs(a - b) < epsilon;
    }
};

// Test: Point 2D parsing
TEST_F(EWKBTest, ParsePoint2D) {
    // Create EWKB for Point(13.4, 52.5) - Berlin
    std::vector<uint8_t> ewkb = {
        0x01,  // Little Endian
        0x01, 0x00, 0x00, 0x00,  // Type: Point
        // X: 13.4
        0xCD, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x2A, 0x40,
        // Y: 52.5
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x4A, 0x40
    };
    
    auto geom = EWKBParser::parse(ewkb);
    
    EXPECT_EQ(geom.type, GeometryType::Point);
    EXPECT_FALSE(geom.has_z);
    EXPECT_EQ(geom.coords.size(), 1);
    EXPECT_TRUE(approxEqual(geom.coords[0].x, 13.4));
    EXPECT_TRUE(approxEqual(geom.coords[0].y, 52.5));
    EXPECT_FALSE(geom.coords[0].hasZ());
}

// Test: Point 3D parsing
TEST_F(EWKBTest, ParsePoint3D) {
    GeometryInfo geom(GeometryType::PointZ);
    geom.has_z = true;
    geom.coords.emplace_back(13.4, 52.5, 100.5);  // Berlin with elevation
    
    auto ewkb = EWKBParser::serialize(geom);
    auto parsed = EWKBParser::parse(ewkb);
    
    EXPECT_EQ(parsed.type, GeometryType::PointZ);
    EXPECT_TRUE(parsed.has_z);
    EXPECT_EQ(parsed.coords.size(), 1);
    EXPECT_TRUE(approxEqual(parsed.coords[0].x, 13.4));
    EXPECT_TRUE(approxEqual(parsed.coords[0].y, 52.5));
    EXPECT_TRUE(parsed.coords[0].hasZ());
    EXPECT_TRUE(approxEqual(parsed.coords[0].getZ(), 100.5));
}

// Test: LineString parsing
TEST_F(EWKBTest, ParseLineString) {
    GeometryInfo geom(GeometryType::LineString);
    geom.coords.emplace_back(0.0, 0.0);
    geom.coords.emplace_back(1.0, 1.0);
    geom.coords.emplace_back(2.0, 0.0);
    
    auto ewkb = EWKBParser::serialize(geom);
    auto parsed = EWKBParser::parse(ewkb);
    
    EXPECT_EQ(parsed.type, GeometryType::LineString);
    EXPECT_EQ(parsed.coords.size(), 3);
    EXPECT_TRUE(approxEqual(parsed.coords[0].x, 0.0));
    EXPECT_TRUE(approxEqual(parsed.coords[1].y, 1.0));
    EXPECT_TRUE(approxEqual(parsed.coords[2].x, 2.0));
}

// Test: Polygon parsing
TEST_F(EWKBTest, ParsePolygon) {
    GeometryInfo geom(GeometryType::Polygon);
    
    // Exterior ring (square)
    std::vector<Coordinate> ring;
    ring.emplace_back(0.0, 0.0);
    ring.emplace_back(1.0, 0.0);
    ring.emplace_back(1.0, 1.0);
    ring.emplace_back(0.0, 1.0);
    ring.emplace_back(0.0, 0.0);  // Closed
    geom.rings.push_back(ring);
    
    auto ewkb = EWKBParser::serialize(geom);
    auto parsed = EWKBParser::parse(ewkb);
    
    EXPECT_EQ(parsed.type, GeometryType::Polygon);
    EXPECT_EQ(parsed.rings.size(), 1);
    EXPECT_EQ(parsed.rings[0].size(), 5);
    EXPECT_TRUE(approxEqual(parsed.rings[0][0].x, 0.0));
    EXPECT_TRUE(approxEqual(parsed.rings[0][2].y, 1.0));
}

// Test: MBR computation
TEST_F(EWKBTest, ComputeMBR) {
    GeometryInfo geom(GeometryType::LineString);
    geom.coords.emplace_back(0.0, 0.0);
    geom.coords.emplace_back(5.0, 10.0);
    geom.coords.emplace_back(-2.0, 3.0);
    
    auto mbr = geom.computeMBR();
    
    EXPECT_TRUE(approxEqual(mbr.minx, -2.0));
    EXPECT_TRUE(approxEqual(mbr.maxx, 5.0));
    EXPECT_TRUE(approxEqual(mbr.miny, 0.0));
    EXPECT_TRUE(approxEqual(mbr.maxy, 10.0));
}

// Test: MBR 3D computation
TEST_F(EWKBTest, ComputeMBR3D) {
    GeometryInfo geom(GeometryType::LineStringZ);
    geom.has_z = true;
    geom.coords.emplace_back(0.0, 0.0, 10.0);
    geom.coords.emplace_back(5.0, 10.0, 50.0);
    geom.coords.emplace_back(-2.0, 3.0, 5.0);
    
    auto mbr = geom.computeMBR();
    
    EXPECT_TRUE(mbr.hasZ());
    EXPECT_TRUE(approxEqual(mbr.z_min.value(), 5.0));
    EXPECT_TRUE(approxEqual(mbr.z_max.value(), 50.0));
}

// Test: Centroid computation
TEST_F(EWKBTest, ComputeCentroid) {
    GeometryInfo geom(GeometryType::LineString);
    geom.coords.emplace_back(0.0, 0.0);
    geom.coords.emplace_back(2.0, 0.0);
    geom.coords.emplace_back(2.0, 2.0);
    geom.coords.emplace_back(0.0, 2.0);
    
    auto centroid = geom.computeCentroid();
    
    EXPECT_TRUE(approxEqual(centroid.x, 1.0));
    EXPECT_TRUE(approxEqual(centroid.y, 1.0));
}

// Test: Sidecar computation
TEST_F(EWKBTest, ComputeSidecar) {
    GeometryInfo geom(GeometryType::PointZ);
    geom.has_z = true;
    geom.coords.emplace_back(13.4, 52.5, 100.5);
    
    auto sidecar = EWKBParser::computeSidecar(geom);
    
    EXPECT_TRUE(approxEqual(sidecar.mbr.minx, 13.4));
    EXPECT_TRUE(approxEqual(sidecar.mbr.maxx, 13.4));
    EXPECT_TRUE(approxEqual(sidecar.centroid.x, 13.4));
    EXPECT_TRUE(approxEqual(sidecar.centroid.y, 52.5));
    EXPECT_TRUE(approxEqual(sidecar.z_min, 100.5));
    EXPECT_TRUE(approxEqual(sidecar.z_max, 100.5));
}

// Test: MBR intersection
TEST_F(EWKBTest, MBRIntersection) {
    MBR mbr1(0.0, 0.0, 10.0, 10.0);
    MBR mbr2(5.0, 5.0, 15.0, 15.0);
    MBR mbr3(20.0, 20.0, 30.0, 30.0);
    
    EXPECT_TRUE(mbr1.intersects(mbr2));
    EXPECT_TRUE(mbr2.intersects(mbr1));
    EXPECT_FALSE(mbr1.intersects(mbr3));
}

// Test: MBR contains point
TEST_F(EWKBTest, MBRContainsPoint) {
    MBR mbr(0.0, 0.0, 10.0, 10.0);
    
    EXPECT_TRUE(mbr.contains(5.0, 5.0));
    EXPECT_TRUE(mbr.contains(0.0, 0.0));  // Edge
    EXPECT_TRUE(mbr.contains(10.0, 10.0));  // Edge
    EXPECT_FALSE(mbr.contains(15.0, 5.0));
}

// Test: GeoJSON parsing (Point)
TEST_F(EWKBTest, GeoJSONPoint) {
    std::string geojson = R"({"type":"Point","coordinates":[13.4,52.5]})";
    
    auto geom = EWKBParser::parseGeoJSON(geojson);
    
    EXPECT_EQ(geom.type, GeometryType::Point);
    EXPECT_EQ(geom.coords.size(), 1);
    EXPECT_TRUE(approxEqual(geom.coords[0].x, 13.4));
    EXPECT_TRUE(approxEqual(geom.coords[0].y, 52.5));
}

// Test: GeoJSON 3D Point
TEST_F(EWKBTest, GeoJSONPoint3D) {
    std::string geojson = R"({"type":"Point","coordinates":[13.4,52.5,100.5]})";
    
    auto geom = EWKBParser::parseGeoJSON(geojson);
    
    EXPECT_EQ(geom.type, GeometryType::PointZ);
    EXPECT_TRUE(geom.has_z);
    EXPECT_TRUE(approxEqual(geom.coords[0].getZ(), 100.5));
}

// Test: To GeoJSON
TEST_F(EWKBTest, ToGeoJSON) {
    GeometryInfo geom(GeometryType::Point);
    geom.coords.emplace_back(13.4, 52.5);
    
    std::string geojson = EWKBParser::toGeoJSON(geom);
    
    EXPECT_TRUE(geojson.find("\"type\":\"Point\"") != std::string::npos);
    EXPECT_TRUE(geojson.find("13.4") != std::string::npos);
    EXPECT_TRUE(geojson.find("52.5") != std::string::npos);
}

// Test: EWKB validation
TEST_F(EWKBTest, ValidateEWKB) {
    GeometryInfo geom(GeometryType::Point);
    geom.coords.emplace_back(13.4, 52.5);
    
    auto ewkb = EWKBParser::serialize(geom);
    EXPECT_TRUE(EWKBParser::validate(ewkb));
    
    // Invalid EWKB (too short)
    std::vector<uint8_t> invalid = {0x01, 0x02};
    EXPECT_FALSE(EWKBParser::validate(invalid));
}

// Test: Round-trip (serialize -> parse)
TEST_F(EWKBTest, RoundTrip) {
    GeometryInfo original(GeometryType::PolygonZ);
    original.has_z = true;
    
    std::vector<Coordinate> ring;
    ring.emplace_back(0.0, 0.0, 10.0);
    ring.emplace_back(10.0, 0.0, 20.0);
    ring.emplace_back(10.0, 10.0, 30.0);
    ring.emplace_back(0.0, 10.0, 15.0);
    ring.emplace_back(0.0, 0.0, 10.0);
    original.rings.push_back(ring);
    
    auto ewkb = EWKBParser::serialize(original);
    auto parsed = EWKBParser::parse(ewkb);
    
    EXPECT_EQ(parsed.type, original.type);
    EXPECT_EQ(parsed.has_z, original.has_z);
    EXPECT_EQ(parsed.rings.size(), original.rings.size());
    EXPECT_EQ(parsed.rings[0].size(), original.rings[0].size());
    
    for (size_t i = 0; i < parsed.rings[0].size(); ++i) {
        EXPECT_TRUE(approxEqual(parsed.rings[0][i].x, original.rings[0][i].x));
        EXPECT_TRUE(approxEqual(parsed.rings[0][i].y, original.rings[0][i].y));
        EXPECT_TRUE(approxEqual(parsed.rings[0][i].getZ(), original.rings[0][i].getZ()));
    }
}

// ============================================================
// GeoJSON: Full RFC 7946 geometry type tests
// ============================================================

// Test: GeoJSON MultiPoint parsing
TEST_F(EWKBTest, GeoJSONMultiPoint) {
    std::string geojson = R"({"type":"MultiPoint","coordinates":[[10.0,20.0],[30.0,40.0],[50.0,60.0]]})";
    auto geom = EWKBParser::parseGeoJSON(geojson);

    EXPECT_EQ(geom.type, GeometryType::MultiPoint);
    EXPECT_FALSE(geom.has_z);
    ASSERT_EQ(geom.geometries.size(), 3u);
    EXPECT_TRUE(approxEqual(geom.geometries[0].coords[0].x, 10.0));
    EXPECT_TRUE(approxEqual(geom.geometries[0].coords[0].y, 20.0));
    EXPECT_TRUE(approxEqual(geom.geometries[2].coords[0].x, 50.0));
    EXPECT_TRUE(approxEqual(geom.geometries[2].coords[0].y, 60.0));
}

// Test: GeoJSON MultiPoint 3D parsing
TEST_F(EWKBTest, GeoJSONMultiPoint3D) {
    std::string geojson = R"({"type":"MultiPoint","coordinates":[[1.0,2.0,3.0],[4.0,5.0,6.0]]})";
    auto geom = EWKBParser::parseGeoJSON(geojson);

    EXPECT_EQ(geom.type, GeometryType::MultiPointZ);
    EXPECT_TRUE(geom.has_z);
    ASSERT_EQ(geom.geometries.size(), 2u);
    EXPECT_TRUE(approxEqual(geom.geometries[0].coords[0].getZ(), 3.0));
    EXPECT_TRUE(approxEqual(geom.geometries[1].coords[0].getZ(), 6.0));
}

// Test: GeoJSON MultiLineString parsing
TEST_F(EWKBTest, GeoJSONMultiLineString) {
    std::string geojson = R"({
        "type": "MultiLineString",
        "coordinates": [
            [[0.0,0.0],[1.0,1.0]],
            [[2.0,2.0],[3.0,3.0],[4.0,4.0]]
        ]
    })";
    auto geom = EWKBParser::parseGeoJSON(geojson);

    EXPECT_EQ(geom.type, GeometryType::MultiLineString);
    ASSERT_EQ(geom.geometries.size(), 2u);
    EXPECT_EQ(geom.geometries[0].coords.size(), 2u);
    EXPECT_EQ(geom.geometries[1].coords.size(), 3u);
    EXPECT_TRUE(approxEqual(geom.geometries[0].coords[1].x, 1.0));
    EXPECT_TRUE(approxEqual(geom.geometries[1].coords[2].x, 4.0));
}

// Test: GeoJSON MultiPolygon parsing
TEST_F(EWKBTest, GeoJSONMultiPolygon) {
    std::string geojson = R"({
        "type": "MultiPolygon",
        "coordinates": [
            [[[0.0,0.0],[1.0,0.0],[1.0,1.0],[0.0,1.0],[0.0,0.0]]],
            [[[2.0,2.0],[3.0,2.0],[3.0,3.0],[2.0,3.0],[2.0,2.0]]]
        ]
    })";
    auto geom = EWKBParser::parseGeoJSON(geojson);

    EXPECT_EQ(geom.type, GeometryType::MultiPolygon);
    ASSERT_EQ(geom.geometries.size(), 2u);
    EXPECT_EQ(geom.geometries[0].rings.size(), 1u);
    EXPECT_EQ(geom.geometries[0].rings[0].size(), 5u);
    EXPECT_EQ(geom.geometries[1].rings.size(), 1u);
    EXPECT_TRUE(approxEqual(geom.geometries[1].rings[0][0].x, 2.0));
    EXPECT_TRUE(approxEqual(geom.geometries[1].rings[0][0].y, 2.0));
}

// Test: GeoJSON MultiPolygon with hole
TEST_F(EWKBTest, GeoJSONMultiPolygonWithHole) {
    std::string geojson = R"({
        "type": "MultiPolygon",
        "coordinates": [
            [
                [[0.0,0.0],[10.0,0.0],[10.0,10.0],[0.0,10.0],[0.0,0.0]],
                [[2.0,2.0],[8.0,2.0],[8.0,8.0],[2.0,8.0],[2.0,2.0]]
            ]
        ]
    })";
    auto geom = EWKBParser::parseGeoJSON(geojson);

    EXPECT_EQ(geom.type, GeometryType::MultiPolygon);
    ASSERT_EQ(geom.geometries.size(), 1u);
    EXPECT_EQ(geom.geometries[0].rings.size(), 2u);  // exterior + hole
    EXPECT_EQ(geom.geometries[0].rings[0].size(), 5u);
    EXPECT_EQ(geom.geometries[0].rings[1].size(), 5u);
}

// Test: GeoJSON GeometryCollection parsing
TEST_F(EWKBTest, GeoJSONGeometryCollection) {
    std::string geojson = R"({
        "type": "GeometryCollection",
        "geometries": [
            {"type":"Point","coordinates":[1.0,2.0]},
            {"type":"LineString","coordinates":[[0.0,0.0],[1.0,1.0]]},
            {"type":"Polygon","coordinates":[[[0.0,0.0],[1.0,0.0],[1.0,1.0],[0.0,0.0]]]}
        ]
    })";
    auto geom = EWKBParser::parseGeoJSON(geojson);

    EXPECT_EQ(geom.type, GeometryType::GeometryCollection);
    ASSERT_EQ(geom.geometries.size(), 3u);
    EXPECT_EQ(geom.geometries[0].type, GeometryType::Point);
    EXPECT_EQ(geom.geometries[1].type, GeometryType::LineString);
    EXPECT_EQ(geom.geometries[2].type, GeometryType::Polygon);
    EXPECT_TRUE(approxEqual(geom.geometries[0].coords[0].x, 1.0));
    EXPECT_EQ(geom.geometries[1].coords.size(), 2u);
    EXPECT_EQ(geom.geometries[2].rings.size(), 1u);
}

// Test: GeoJSON nested GeometryCollection
TEST_F(EWKBTest, GeoJSONNestedGeometryCollection) {
    std::string geojson = R"({
        "type": "GeometryCollection",
        "geometries": [
            {
                "type": "GeometryCollection",
                "geometries": [
                    {"type":"Point","coordinates":[5.0,10.0]}
                ]
            }
        ]
    })";
    auto geom = EWKBParser::parseGeoJSON(geojson);

    EXPECT_EQ(geom.type, GeometryType::GeometryCollection);
    ASSERT_EQ(geom.geometries.size(), 1u);
    EXPECT_EQ(geom.geometries[0].type, GeometryType::GeometryCollection);
    ASSERT_EQ(geom.geometries[0].geometries.size(), 1u);
    EXPECT_EQ(geom.geometries[0].geometries[0].type, GeometryType::Point);
    EXPECT_TRUE(approxEqual(geom.geometries[0].geometries[0].coords[0].x, 5.0));
}

// Test: GeoJSON 3D GeometryCollection — type promoted to GeometryCollectionZ
TEST_F(EWKBTest, GeoJSONGeometryCollection3D) {
    std::string geojson = R"({
        "type": "GeometryCollection",
        "geometries": [
            {"type":"Point","coordinates":[1.0,2.0,3.0]},
            {"type":"Point","coordinates":[4.0,5.0,6.0]}
        ]
    })";
    auto geom = EWKBParser::parseGeoJSON(geojson);

    EXPECT_EQ(geom.type, GeometryType::GeometryCollectionZ);
    EXPECT_TRUE(geom.has_z);
    ASSERT_EQ(geom.geometries.size(), 2u);
    EXPECT_EQ(geom.geometries[0].type, GeometryType::PointZ);
    EXPECT_TRUE(approxEqual(geom.geometries[0].coords[0].getZ(), 3.0));
    EXPECT_TRUE(approxEqual(geom.geometries[1].coords[0].getZ(), 6.0));
}

// Test: GeoJSON empty GeometryCollection is valid and yields zero members
TEST_F(EWKBTest, GeoJSONEmptyGeometryCollection) {
    std::string geojson = R"({"type":"GeometryCollection","geometries":[]})";
    auto geom = EWKBParser::parseGeoJSON(geojson);

    EXPECT_EQ(geom.type, GeometryType::GeometryCollection);
    EXPECT_FALSE(geom.has_z);
    EXPECT_EQ(geom.geometries.size(), 0u);
}

// Test: GeoJSON nesting depth limit — 8 GeometryCollection objects exceed the limit
TEST_F(EWKBTest, GeoJSONMaxNestingDepthExceeded) {
    // The parser allows at most 7 GC objects in a chain; the 8th triggers the depth guard.
    std::string geojson =
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"Point","coordinates":[1.0,2.0]})"
        R"(]}]}]}]}]}]}]}]})";
    EXPECT_THROW(EWKBParser::parseGeoJSON(geojson), std::runtime_error);
}

// Test: GeoJSON nesting at the limit — 7 GeometryCollection objects are valid
TEST_F(EWKBTest, GeoJSONMaxNestingDepthExactlyValid) {
    // 7 chained GeometryCollection objects are within the allowed limit and parse correctly.
    std::string geojson =
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"GeometryCollection","geometries":[)"
        R"({"type":"Point","coordinates":[1.0,2.0]})"
        R"(]}]}]}]}]}]}]})";
    EXPECT_NO_THROW(EWKBParser::parseGeoJSON(geojson));
}

// Test: GeoJSON unsupported type throws
TEST_F(EWKBTest, GeoJSONUnsupportedTypeThrows) {
    std::string geojson = R"({"type":"Foo","coordinates":[]})";
    EXPECT_THROW(EWKBParser::parseGeoJSON(geojson), std::runtime_error);
}

// Test: toGeoJSON MultiPoint round-trip
TEST_F(EWKBTest, ToGeoJSONMultiPoint) {
    GeometryInfo geom(GeometryType::MultiPoint);
    for (auto [x, y] : std::vector<std::pair<double,double>>{{1.0,2.0},{3.0,4.0}}) {
        GeometryInfo pt(GeometryType::Point);
        pt.coords.emplace_back(x, y);
        geom.geometries.push_back(pt);
    }
    std::string js = EWKBParser::toGeoJSON(geom);
    EXPECT_NE(js.find("\"MultiPoint\""), std::string::npos);
    // Round-trip
    auto parsed = EWKBParser::parseGeoJSON(js);
    EXPECT_EQ(parsed.type, GeometryType::MultiPoint);
    ASSERT_EQ(parsed.geometries.size(), 2u);
    EXPECT_TRUE(approxEqual(parsed.geometries[0].coords[0].x, 1.0));
    EXPECT_TRUE(approxEqual(parsed.geometries[1].coords[0].y, 4.0));
}

// Test: toGeoJSON MultiPolygon round-trip
TEST_F(EWKBTest, ToGeoJSONMultiPolygon) {
    GeometryInfo geom(GeometryType::MultiPolygon);
    GeometryInfo poly(GeometryType::Polygon);
    std::vector<Coordinate> ring = {
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}
    };
    poly.rings.push_back(ring);
    geom.geometries.push_back(poly);

    std::string js = EWKBParser::toGeoJSON(geom);
    EXPECT_NE(js.find("\"MultiPolygon\""), std::string::npos);
    // Round-trip
    auto parsed = EWKBParser::parseGeoJSON(js);
    EXPECT_EQ(parsed.type, GeometryType::MultiPolygon);
    ASSERT_EQ(parsed.geometries.size(), 1u);
    EXPECT_EQ(parsed.geometries[0].rings.size(), 1u);
    EXPECT_EQ(parsed.geometries[0].rings[0].size(), 5u);
}

// Test: toGeoJSON GeometryCollection round-trip
TEST_F(EWKBTest, ToGeoJSONGeometryCollection) {
    GeometryInfo geom(GeometryType::GeometryCollection);
    GeometryInfo pt(GeometryType::Point);
    pt.coords.emplace_back(7.0, 8.0);
    geom.geometries.push_back(pt);

    std::string js = EWKBParser::toGeoJSON(geom);
    EXPECT_NE(js.find("\"GeometryCollection\""), std::string::npos);
    // Round-trip
    auto parsed = EWKBParser::parseGeoJSON(js);
    EXPECT_EQ(parsed.type, GeometryType::GeometryCollection);
    ASSERT_EQ(parsed.geometries.size(), 1u);
    EXPECT_EQ(parsed.geometries[0].type, GeometryType::Point);
    EXPECT_TRUE(approxEqual(parsed.geometries[0].coords[0].x, 7.0));
}

// Test: EWKB round-trip for MultiPolygon
TEST_F(EWKBTest, EWKBRoundTripMultiPolygon) {
    GeometryInfo original(GeometryType::MultiPolygon);
    for (int i = 0; i < 2; ++i) {
        GeometryInfo poly(GeometryType::Polygon);
        std::vector<Coordinate> ring = {
            {i*10.0, 0.0}, {i*10.0+1.0, 0.0},
            {i*10.0+1.0, 1.0}, {i*10.0, 1.0}, {i*10.0, 0.0}
        };
        poly.rings.push_back(ring);
        original.geometries.push_back(poly);
    }

    auto ewkb = EWKBParser::serialize(original);
    auto parsed = EWKBParser::parse(ewkb);

    EXPECT_EQ(parsed.type, GeometryType::MultiPolygon);
    ASSERT_EQ(parsed.geometries.size(), 2u);
    EXPECT_EQ(parsed.geometries[0].rings.size(), 1u);
    EXPECT_EQ(parsed.geometries[0].rings[0].size(), 5u);
    EXPECT_TRUE(approxEqual(parsed.geometries[1].rings[0][0].x, 10.0));
}

// Test: EWKB round-trip for GeometryCollection
TEST_F(EWKBTest, EWKBRoundTripGeometryCollection) {
    GeometryInfo original(GeometryType::GeometryCollection);
    GeometryInfo pt(GeometryType::Point);
    pt.coords.emplace_back(1.0, 2.0);
    GeometryInfo ls(GeometryType::LineString);
    ls.coords.emplace_back(0.0, 0.0);
    ls.coords.emplace_back(1.0, 1.0);
    original.geometries.push_back(pt);
    original.geometries.push_back(ls);

    auto ewkb = EWKBParser::serialize(original);
    auto parsed = EWKBParser::parse(ewkb);

    EXPECT_EQ(parsed.type, GeometryType::GeometryCollection);
    ASSERT_EQ(parsed.geometries.size(), 2u);
    EXPECT_EQ(parsed.geometries[0].type, GeometryType::Point);
    EXPECT_EQ(parsed.geometries[1].type, GeometryType::LineString);
    EXPECT_TRUE(approxEqual(parsed.geometries[0].coords[0].x, 1.0));
    EXPECT_EQ(parsed.geometries[1].coords.size(), 2u);
}

// Test: computeMBR for GeometryCollection
TEST_F(EWKBTest, ComputeMBRGeometryCollection) {
    GeometryInfo geom(GeometryType::GeometryCollection);
    GeometryInfo pt(GeometryType::Point);
    pt.coords.emplace_back(-5.0, 3.0);
    GeometryInfo ls(GeometryType::LineString);
    ls.coords.emplace_back(0.0, 0.0);
    ls.coords.emplace_back(7.0, 9.0);
    geom.geometries.push_back(pt);
    geom.geometries.push_back(ls);

    auto mbr = geom.computeMBR();
    EXPECT_TRUE(approxEqual(mbr.minx, -5.0));
    EXPECT_TRUE(approxEqual(mbr.maxx, 7.0));
    EXPECT_TRUE(approxEqual(mbr.miny, 0.0));
    EXPECT_TRUE(approxEqual(mbr.maxy, 9.0));
}

// ============================================================
// WGS84 coordinate range validation tests
// ============================================================

// Test: out-of-range longitude throws (strict mode)
TEST_F(EWKBTest, GeoJSONInvalidLongitudeThrows) {
#ifndef THEMIS_GEO_COMPAT_LAX
    EXPECT_THROW(
        EWKBParser::parseGeoJSON(R"({"type":"Point","coordinates":[200.0,50.0]})"),
        std::runtime_error);
#else
    GTEST_SKIP() << "THEMIS_GEO_COMPAT_LAX: coordinate validation disabled";
#endif
}

// Test: out-of-range latitude throws (strict mode)
TEST_F(EWKBTest, GeoJSONInvalidLatitudeThrows) {
#ifndef THEMIS_GEO_COMPAT_LAX
    EXPECT_THROW(
        EWKBParser::parseGeoJSON(R"({"type":"Point","coordinates":[10.0,100.0]})"),
        std::runtime_error);
#else
    GTEST_SKIP() << "THEMIS_GEO_COMPAT_LAX: coordinate validation disabled";
#endif
}

// Test: valid WGS84 boundary coordinates are accepted
TEST_F(EWKBTest, GeoJSONValidWGS84Boundaries) {
    // Min/max valid longitude and latitude
    EXPECT_NO_THROW(EWKBParser::parseGeoJSON(R"({"type":"Point","coordinates":[-180.0,-90.0]})"));
    EXPECT_NO_THROW(EWKBParser::parseGeoJSON(R"({"type":"Point","coordinates":[180.0,90.0]})"));
}

// Test: out-of-range coordinate in LineString throws
TEST_F(EWKBTest, GeoJSONLineStringInvalidCoordThrows) {
#ifndef THEMIS_GEO_COMPAT_LAX
    EXPECT_THROW(
        EWKBParser::parseGeoJSON(
            R"({"type":"LineString","coordinates":[[0.0,0.0],[181.0,50.0]]})"),
        std::runtime_error);
#else
    GTEST_SKIP() << "THEMIS_GEO_COMPAT_LAX: coordinate validation disabled";
#endif
}

// Test: out-of-range coordinate in Polygon throws
TEST_F(EWKBTest, GeoJSONPolygonInvalidCoordThrows) {
#ifndef THEMIS_GEO_COMPAT_LAX
    EXPECT_THROW(
        EWKBParser::parseGeoJSON(
            R"({"type":"Polygon","coordinates":[[[0.0,0.0],[0.0,-91.0],[1.0,0.0],[0.0,0.0]]]})"),
        std::runtime_error);
#else
    GTEST_SKIP() << "THEMIS_GEO_COMPAT_LAX: coordinate validation disabled";
#endif
}

// ============================================================
// GeometryInfo type-check helper tests (isMultiPolygon, isGeometryCollection)
// ============================================================

TEST_F(EWKBTest, IsMultiPolygon_True) {
    GeometryInfo geom(GeometryType::MultiPolygon);
    EXPECT_TRUE(geom.isMultiPolygon());
    EXPECT_FALSE(geom.isPolygon());
    EXPECT_FALSE(geom.isGeometryCollection());
}

TEST_F(EWKBTest, IsMultiPolygonZ_True) {
    GeometryInfo geom(GeometryType::MultiPolygonZ);
    EXPECT_TRUE(geom.isMultiPolygon());
}

TEST_F(EWKBTest, IsGeometryCollection_True) {
    GeometryInfo geom(GeometryType::GeometryCollection);
    EXPECT_TRUE(geom.isGeometryCollection());
    EXPECT_FALSE(geom.isMultiPolygon());
    EXPECT_FALSE(geom.isPolygon());
}

TEST_F(EWKBTest, IsGeometryCollectionZ_True) {
    GeometryInfo geom(GeometryType::GeometryCollectionZ);
    EXPECT_TRUE(geom.isGeometryCollection());
}

TEST_F(EWKBTest, IsPolygon_NotMultiPolygon) {
    GeometryInfo geom(GeometryType::Polygon);
    EXPECT_TRUE(geom.isPolygon());
    EXPECT_FALSE(geom.isMultiPolygon());
    EXPECT_FALSE(geom.isGeometryCollection());
}

// ============================================================
// exactIntersects: MultiPolygon and GeometryCollection (CPU backend)
// ============================================================

#include "geo/spatial_backend.h"

namespace {

static GeometryInfo makeTestPoint(double x, double y) {
    GeometryInfo g(GeometryType::Point);
    g.coords.push_back({x, y});
    return g;
}

static GeometryInfo makeTestPolygon(std::initializer_list<std::pair<double,double>> pts) {
    GeometryInfo g(GeometryType::Polygon);
    std::vector<Coordinate> ring;
    for (auto& p : pts) ring.push_back({p.first, p.second});
    g.rings.push_back(ring);
    return g;
}

static GeometryInfo makeTestMultiPolygon(
    std::initializer_list<std::initializer_list<std::pair<double,double>>> polys) {
    GeometryInfo g(GeometryType::MultiPolygon);
    for (const auto& pts : polys) {
        GeometryInfo poly(GeometryType::Polygon);
        std::vector<Coordinate> ring;
        for (const auto& p : pts) ring.push_back({p.first, p.second});
        poly.rings.push_back(ring);
        g.geometries.push_back(poly);
    }
    return g;
}

} // namespace

// Test: Point inside one polygon of a MultiPolygon → true
TEST_F(EWKBTest, ExactIntersects_PointInsideMultiPolygon_True) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    auto mp = makeTestMultiPolygon({
        {{0,0},{1,0},{1,1},{0,1},{0,0}},   // first polygon: [0-1, 0-1]
        {{5,5},{6,5},{6,6},{5,6},{5,5}}    // second polygon: [5-6, 5-6]
    });
    EXPECT_TRUE(cpu->exactIntersects(makeTestPoint(0.5, 0.5), mp));
}

// Test: Point inside second polygon of a MultiPolygon → true
TEST_F(EWKBTest, ExactIntersects_PointInsideSecondPolygonOfMulti_True) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    auto mp = makeTestMultiPolygon({
        {{0,0},{1,0},{1,1},{0,1},{0,0}},
        {{5,5},{6,5},{6,6},{5,6},{5,5}}
    });
    EXPECT_TRUE(cpu->exactIntersects(makeTestPoint(5.5, 5.5), mp));
}

// Test: Point outside all polygons of a MultiPolygon → false
TEST_F(EWKBTest, ExactIntersects_PointOutsideMultiPolygon_False) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    auto mp = makeTestMultiPolygon({
        {{0,0},{1,0},{1,1},{0,1},{0,0}},
        {{5,5},{6,5},{6,6},{5,6},{5,5}}
    });
    EXPECT_FALSE(cpu->exactIntersects(makeTestPoint(3.0, 3.0), mp));
}

// Test: MultiPolygon × MultiPolygon overlapping → true
TEST_F(EWKBTest, ExactIntersects_MultiPolygonMultiPolygon_Overlapping_True) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    auto mp1 = makeTestMultiPolygon({{{0,0},{2,0},{2,2},{0,2},{0,0}}});
    auto mp2 = makeTestMultiPolygon({{{1,1},{3,1},{3,3},{1,3},{1,1}}});
    EXPECT_TRUE(cpu->exactIntersects(mp1, mp2));
}

// Test: MultiPolygon × MultiPolygon disjoint → false
TEST_F(EWKBTest, ExactIntersects_MultiPolygonMultiPolygon_Disjoint_False) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    auto mp1 = makeTestMultiPolygon({{{0,0},{1,0},{1,1},{0,1},{0,0}}});
    auto mp2 = makeTestMultiPolygon({{{5,5},{6,5},{6,6},{5,6},{5,5}}});
    EXPECT_FALSE(cpu->exactIntersects(mp1, mp2));
}

// Test: Empty MultiPolygon → false
TEST_F(EWKBTest, ExactIntersects_EmptyMultiPolygon_False) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    GeometryInfo empty_mp(GeometryType::MultiPolygon);
    EXPECT_FALSE(cpu->exactIntersects(makeTestPoint(0.5, 0.5), empty_mp));
    EXPECT_FALSE(cpu->exactIntersects(empty_mp, makeTestPoint(0.5, 0.5)));
}

// Test: GeometryCollection containing a polygon that intersects the point → true
TEST_F(EWKBTest, ExactIntersects_PointInsideGeometryCollection_True) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    GeometryInfo gc(GeometryType::GeometryCollection);
    gc.geometries.push_back(makeTestPoint(10.0, 10.0));
    gc.geometries.push_back(makeTestPolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}));
    EXPECT_TRUE(cpu->exactIntersects(makeTestPoint(0.5, 0.5), gc));
}

// Test: GeometryCollection with no intersecting member → false
TEST_F(EWKBTest, ExactIntersects_PointOutsideGeometryCollection_False) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    GeometryInfo gc(GeometryType::GeometryCollection);
    gc.geometries.push_back(makeTestPolygon({{0,0},{1,0},{1,1},{0,1},{0,0}}));
    gc.geometries.push_back(makeTestPolygon({{5,5},{6,5},{6,6},{5,6},{5,5}}));
    EXPECT_FALSE(cpu->exactIntersects(makeTestPoint(3.0, 3.0), gc));
}

// Test: GeometryCollection × GeometryCollection with matching members → true
TEST_F(EWKBTest, ExactIntersects_GeometryCollectionGeometryCollection_Match_True) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    GeometryInfo gc1(GeometryType::GeometryCollection);
    gc1.geometries.push_back(makeTestPolygon({{0,0},{2,0},{2,2},{0,2},{0,0}}));
    GeometryInfo gc2(GeometryType::GeometryCollection);
    gc2.geometries.push_back(makeTestPolygon({{1,1},{3,1},{3,3},{1,3},{1,1}}));
    EXPECT_TRUE(cpu->exactIntersects(gc1, gc2));
}

// Test: Empty GeometryCollection → false
TEST_F(EWKBTest, ExactIntersects_EmptyGeometryCollection_False) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    GeometryInfo empty_gc(GeometryType::GeometryCollection);
    EXPECT_FALSE(cpu->exactIntersects(makeTestPoint(0.5, 0.5), empty_gc));
    EXPECT_FALSE(cpu->exactIntersects(empty_gc, makeTestPoint(0.5, 0.5)));
}

// Test: Polygon intersects MultiPolygon (asymmetric call) → true
TEST_F(EWKBTest, ExactIntersects_PolygonIntersectsMultiPolygon_True) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    auto poly = makeTestPolygon({{0,0},{2,0},{2,2},{0,2},{0,0}});
    auto mp = makeTestMultiPolygon({{{1,1},{3,1},{3,3},{1,3},{1,1}}});
    EXPECT_TRUE(cpu->exactIntersects(poly, mp));
    EXPECT_TRUE(cpu->exactIntersects(mp, poly));  // symmetric
}

// Test: GeoJSON-parsed MultiPolygon with exactIntersects
TEST_F(EWKBTest, ExactIntersects_ParsedMultiPolygon_True) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    std::string geojson = R"({
        "type": "MultiPolygon",
        "coordinates": [
            [[[0.0,0.0],[1.0,0.0],[1.0,1.0],[0.0,1.0],[0.0,0.0]]],
            [[[5.0,5.0],[6.0,5.0],[6.0,6.0],[5.0,6.0],[5.0,5.0]]]
        ]
    })";
    auto mp = EWKBParser::parseGeoJSON(geojson);
    EXPECT_EQ(mp.type, GeometryType::MultiPolygon);
    EXPECT_TRUE(cpu->exactIntersects(makeTestPoint(0.5, 0.5), mp));
    EXPECT_TRUE(cpu->exactIntersects(makeTestPoint(5.5, 5.5), mp));
    EXPECT_FALSE(cpu->exactIntersects(makeTestPoint(3.0, 3.0), mp));
}

// Test: GeoJSON-parsed GeometryCollection with exactIntersects
TEST_F(EWKBTest, ExactIntersects_ParsedGeometryCollection_True) {
    auto* cpu = themis::geo::getCpuExactBackend();
    ASSERT_NE(cpu, nullptr);
    std::string geojson = R"({
        "type": "GeometryCollection",
        "geometries": [
            {"type":"Point","coordinates":[10.0,10.0]},
            {"type":"Polygon","coordinates":[[[0.0,0.0],[1.0,0.0],[1.0,1.0],[0.0,1.0],[0.0,0.0]]]}
        ]
    })";
    auto gc = EWKBParser::parseGeoJSON(geojson);
    EXPECT_EQ(gc.type, GeometryType::GeometryCollection);
    // Point (10,10) is in the collection — point-point intersection
    EXPECT_TRUE(cpu->exactIntersects(makeTestPoint(10.0, 10.0), gc));
    // A point outside all members
    EXPECT_FALSE(cpu->exactIntersects(makeTestPoint(5.0, 5.0), gc));
}

 
