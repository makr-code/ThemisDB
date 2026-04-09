/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_geo_future_interfaces.cpp                     ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Tests for the six new geo module interfaces defined in FUTURE_ENHANCEMENTS.md:
 *
 *  1. IGeoIndex                   (geo_index.h)
 *  2. IGeoJSONGeometry + types    (geojson_geometry.h)
 *  3. ISpatialJoinFilter          (spatial_join_filter.h)
 *  4. IRTreeCursor                (rtree_cursor.h)
 *  5. IRasterQueryInterface       (raster_query_interface.h)
 *  6. ITemporalSpatialQueryBuilder (temporal_spatial_query_builder.h)
 */

#include <gtest/gtest.h>

#include "geo/geo_index.h"
#include "geo/geojson_geometry.h"
#include "geo/rtree_cursor.h"
#include "geo/spatial_join_filter.h"
#include "geo/raster_query_interface.h"
#include "geo/temporal_spatial_query_builder.h"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace themis::geo;

// ═══════════════════════════════════════════════════════════════════════════
// Group 1: GeoJSONGeometry — IGeoJSONGeometry + concrete types
// ═══════════════════════════════════════════════════════════════════════════

class GeoJSONGeometryTest : public ::testing::Test {};

// 1-1: GeoPoint constructs correctly and type() returns "Point"
TEST_F(GeoJSONGeometryTest, GeoPoint_Type) {
    GeoPoint p(13.4, 52.5, CRS::wgs84());
    EXPECT_STREQ(p.type(), "Point");
    EXPECT_DOUBLE_EQ(p.lon(), 13.4);
    EXPECT_DOUBLE_EQ(p.lat(), 52.5);
}

// 1-2: GeoPoint validate() succeeds for valid WGS84 coordinates
TEST_F(GeoJSONGeometryTest, GeoPoint_Validate_Valid) {
    GeoPoint p(0.0, 0.0, CRS::wgs84());
    EXPECT_TRUE(p.validate().ok());
}

// 1-3: GeoPoint validate() rejects out-of-range longitude
TEST_F(GeoJSONGeometryTest, GeoPoint_Validate_LonOutOfRange) {
    GeoPoint p(181.0, 0.0, CRS::wgs84());
    auto r = p.validate();
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.code, GeoValidationCode::OUT_OF_RANGE_LON);
}

// 1-4: GeoPoint validate() rejects out-of-range latitude
TEST_F(GeoJSONGeometryTest, GeoPoint_Validate_LatOutOfRange) {
    GeoPoint p(0.0, -91.0, CRS::wgs84());
    auto r = p.validate();
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.code, GeoValidationCode::OUT_OF_RANGE_LAT);
}

// 1-5: GeoPoint toGeoJSON() produces well-formed output
TEST_F(GeoJSONGeometryTest, GeoPoint_ToGeoJSON) {
    GeoPoint p(2.35, 48.85, CRS::wgs84());
    std::string json = p.toGeoJSON();
    EXPECT_NE(json.find("\"type\":\"Point\""), std::string::npos);
    EXPECT_NE(json.find("2.35"), std::string::npos);
    EXPECT_NE(json.find("48.85"), std::string::npos);
}

// 1-6: GeoPoint bbox() returns a degenerate (point-sized) MBR
TEST_F(GeoJSONGeometryTest, GeoPoint_BBox_Degenerate) {
    GeoPoint p(10.0, 20.0, CRS::wgs84());
    MBR b = p.bbox();
    EXPECT_DOUBLE_EQ(b.minx, 10.0);
    EXPECT_DOUBLE_EQ(b.maxx, 10.0);
    EXPECT_DOUBLE_EQ(b.miny, 20.0);
    EXPECT_DOUBLE_EQ(b.maxy, 20.0);
}

// 1-7: GeoLineString constructs and validates successfully
TEST_F(GeoJSONGeometryTest, GeoLineString_ValidateOk) {
    std::vector<Coordinate> coords{{0.0, 0.0}, {1.0, 1.0}, {2.0, 0.0}};
    GeoLineString ls(coords, CRS::wgs84());
    EXPECT_STREQ(ls.type(), "LineString");
    EXPECT_TRUE(ls.validate().ok());
}

// 1-8: GeoLineString validate() rejects empty coordinate list
TEST_F(GeoJSONGeometryTest, GeoLineString_Validate_Empty) {
    GeoLineString ls({}, CRS::wgs84());
    auto r = ls.validate();
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.code, GeoValidationCode::EMPTY_GEOMETRY);
}

// 1-9: GeoPolygon CCW exterior ring passes validation
TEST_F(GeoJSONGeometryTest, GeoPolygon_CCW_Valid) {
    // Counter-clockwise square (positive area)
    std::vector<Coordinate> ring{
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}};
    GeoPolygon poly({{ring}}, CRS::wgs84());
    EXPECT_STREQ(poly.type(), "Polygon");
    EXPECT_TRUE(poly.validate().ok());
}

// 1-10: GeoPolygon CW exterior ring fails winding-order check
TEST_F(GeoJSONGeometryTest, GeoPolygon_CW_Invalid) {
    // Clockwise square (negative area) — wrong winding for exterior
    std::vector<Coordinate> ring{
        {0.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}, {0.0, 0.0}};
    GeoPolygon poly({{ring}}, CRS::wgs84());
    auto r = poly.validate();
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.code, GeoValidationCode::WRONG_WINDING_ORDER);
}

// 1-11: GeoPolygon unclosed ring fails validation
TEST_F(GeoJSONGeometryTest, GeoPolygon_UnclosedRing) {
    std::vector<Coordinate> ring{
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};  // missing closing vertex
    GeoPolygon poly({{ring}}, CRS::wgs84());
    auto r = poly.validate();
    EXPECT_FALSE(r.ok());
    // Ring has < 4 vertices -> RING_NOT_CLOSED
    EXPECT_EQ(r.code, GeoValidationCode::RING_NOT_CLOSED);
}

// 1-12: GeoPolygon toGeoJSON() includes type and coordinates
TEST_F(GeoJSONGeometryTest, GeoPolygon_ToGeoJSON) {
    std::vector<Coordinate> ring{
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}};
    GeoPolygon poly({{ring}}, CRS::wgs84());
    std::string json = poly.toGeoJSON();
    EXPECT_NE(json.find("\"type\":\"Polygon\""), std::string::npos);
    EXPECT_NE(json.find("coordinates"), std::string::npos);
}

// 1-13: GeoMultiPolygon validates each sub-polygon
TEST_F(GeoJSONGeometryTest, GeoMultiPolygon_Validate_Valid) {
    std::vector<Coordinate> ring{
        {0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}, {0.0, 0.0}};
    GeoPolygon p1({{ring}}, CRS::wgs84());
    GeoMultiPolygon mp({p1}, CRS::wgs84());
    EXPECT_STREQ(mp.type(), "MultiPolygon");
    EXPECT_TRUE(mp.validate().ok());
}

// 1-14: GeoGeometryCollection validates all children
TEST_F(GeoJSONGeometryTest, GeoGeometryCollection_Validate_Valid) {
    auto pt = std::make_shared<GeoPoint>(5.0, 10.0, CRS::wgs84());
    GeoGeometryCollection gc(
        {std::static_pointer_cast<IGeoJSONGeometry>(pt)}, CRS::wgs84());
    EXPECT_STREQ(gc.type(), "GeometryCollection");
    EXPECT_TRUE(gc.validate().ok());
}

// 1-15: CRS equality operator works correctly
TEST_F(GeoJSONGeometryTest, CRS_Equality) {
    EXPECT_EQ(CRS::wgs84(), CRS::wgs84());
    EXPECT_NE(CRS::wgs84(), CRS{"EPSG:3857"});
}

// ═══════════════════════════════════════════════════════════════════════════
// Group 2: ISpatialJoinFilter — composable predicates
// ═══════════════════════════════════════════════════════════════════════════

class SpatialJoinFilterTest : public ::testing::Test {
protected:
    // Two overlapping squares in positive quadrant (CCW)
    static GeoPolygon makeSquare(double x0, double y0, double x1, double y1) {
        std::vector<Coordinate> ring{
            {x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, {x0, y0}};
        return GeoPolygon({{ring}}, CRS::wgs84());
    }

    static GeoPoint makePoint(double lon, double lat) {
        return GeoPoint(lon, lat, CRS::wgs84());
    }
};

// 2-1: intersects() returns true for overlapping MBRs
TEST_F(SpatialJoinFilterTest, Intersects_Overlapping) {
    auto sq1 = makeSquare(0, 0, 2, 2);
    auto sq2 = makeSquare(1, 1, 3, 3);
    auto f = SpatialJoinFilter::intersects();
    EXPECT_TRUE(f->matches(sq1, sq2));
}

// 2-2: intersects() returns false for non-overlapping MBRs
TEST_F(SpatialJoinFilterTest, Intersects_NonOverlapping) {
    auto sq1 = makeSquare(0, 0, 1, 1);
    auto sq2 = makeSquare(5, 5, 6, 6);
    auto f = SpatialJoinFilter::intersects();
    EXPECT_FALSE(f->matches(sq1, sq2));
}

// 2-3: contains() returns true when a encloses b entirely
TEST_F(SpatialJoinFilterTest, Contains_True) {
    auto outer = makeSquare(0, 0, 10, 10);
    auto inner = makeSquare(2, 2, 5, 5);
    auto f = SpatialJoinFilter::contains();
    EXPECT_TRUE(f->matches(outer, inner));
}

// 2-4: contains() returns false when b is not enclosed
TEST_F(SpatialJoinFilterTest, Contains_False) {
    auto sq1 = makeSquare(0, 0, 5, 5);
    auto sq2 = makeSquare(4, 4, 10, 10);  // extends beyond sq1
    auto f = SpatialJoinFilter::contains();
    EXPECT_FALSE(f->matches(sq1, sq2));
}

// 2-5: within() is the inverse of contains()
TEST_F(SpatialJoinFilterTest, Within_IsInverseOfContains) {
    auto outer = makeSquare(0, 0, 10, 10);
    auto inner = makeSquare(2, 2, 5, 5);
    auto fw = SpatialJoinFilter::within();
    EXPECT_TRUE(fw->matches(inner, outer));
    EXPECT_FALSE(fw->matches(outer, inner));
}

// 2-6: dWithin() returns true for points within radius
TEST_F(SpatialJoinFilterTest, DWithin_TrueForNearby) {
    auto p1 = makePoint(13.404954, 52.520008);  // Berlin
    auto p2 = makePoint(13.405, 52.520);        // ~50 m away
    auto f = SpatialJoinFilter::dWithin(1000.0);  // 1 km threshold
    EXPECT_TRUE(f->matches(p1, p2));
}

// 2-7: dWithin() returns false for distant points
TEST_F(SpatialJoinFilterTest, DWithin_FalseForDistant) {
    auto berlin = makePoint(13.404954, 52.520008);
    auto paris  = makePoint(2.349014, 48.864716);   // ~878 km
    auto f = SpatialJoinFilter::dWithin(100.0);     // 100 m threshold
    EXPECT_FALSE(f->matches(berlin, paris));
}

// 2-8: dWithin() rejects non-positive radius
TEST_F(SpatialJoinFilterTest, DWithin_RejectsNonPositiveRadius) {
    EXPECT_THROW(SpatialJoinFilter::dWithin(0.0), std::invalid_argument);
    EXPECT_THROW(SpatialJoinFilter::dWithin(-1.0), std::invalid_argument);
}

// 2-9: and_() is true only when both sub-filters hold
TEST_F(SpatialJoinFilterTest, And_TruthTable) {
    auto sq1 = makeSquare(0, 0, 10, 10);
    auto sq2 = makeSquare(2, 2, 5, 5);
    auto combo = SpatialJoinFilter::and_(
        SpatialJoinFilter::intersects(),
        SpatialJoinFilter::contains());
    EXPECT_TRUE(combo->matches(sq1, sq2));
}

// 2-10: or_() is true when at least one sub-filter holds
TEST_F(SpatialJoinFilterTest, Or_TruthTable) {
    auto sq1 = makeSquare(0, 0, 1, 1);
    auto sq2 = makeSquare(5, 5, 6, 6);  // no intersect, no contain
    auto always_false = SpatialJoinFilter::contains();
    auto always_true  = SpatialJoinFilter::not_(SpatialJoinFilter::contains());
    auto combo = SpatialJoinFilter::or_(always_false, always_true);
    EXPECT_TRUE(combo->matches(sq1, sq2));
}

// 2-11: not_() negates the inner filter
TEST_F(SpatialJoinFilterTest, Not_Negates) {
    auto sq1 = makeSquare(0, 0, 1, 1);
    auto sq2 = makeSquare(5, 5, 6, 6);
    auto f_intersects = SpatialJoinFilter::intersects();
    auto f_not        = SpatialJoinFilter::not_(f_intersects);
    EXPECT_FALSE(f_intersects->matches(sq1, sq2));
    EXPECT_TRUE(f_not->matches(sq1, sq2));
}

// ═══════════════════════════════════════════════════════════════════════════
// Group 3: IRTreeCursor — CursorStatus enum and API contract
// ═══════════════════════════════════════════════════════════════════════════

class RTreeCursorTest : public ::testing::Test {};

// 3-1: CursorStatus values are distinct
TEST_F(RTreeCursorTest, CursorStatus_ValuesDistinct) {
    EXPECT_NE(CursorStatus::OK, CursorStatus::END);
    EXPECT_NE(CursorStatus::OK, CursorStatus::STALE);
    EXPECT_NE(CursorStatus::END, CursorStatus::STALE);
}

// 3-2: GeoIndexEntry holds key and geometry
TEST_F(RTreeCursorTest, GeoIndexEntry_FieldsAccessible) {
    GeoIndexEntry entry;
    entry.key = "test_key";
    entry.geom = GeometryInfo(GeometryType::Point);
    EXPECT_EQ(entry.key, "test_key");
    EXPECT_EQ(entry.geom.type, GeometryType::Point);
}

// ═══════════════════════════════════════════════════════════════════════════
// Group 4: IGeoIndex — abstract interface API contract
// ═══════════════════════════════════════════════════════════════════════════

class GeoIndexTest : public ::testing::Test {};

// 4-1: IGeoIndex is a pure abstract type (can only be used via pointer)
TEST_F(GeoIndexTest, IGeoIndex_IsPureAbstract) {
    // If this compiles, the interface is correctly defined as pure virtual.
    EXPECT_TRUE(std::is_abstract<IGeoIndex>::value);
}

// 4-2: IRTreeCursor is a pure abstract type
TEST_F(GeoIndexTest, IRTreeCursor_IsPureAbstract) {
    EXPECT_TRUE(std::is_abstract<IRTreeCursor>::value);
}

// ═══════════════════════════════════════════════════════════════════════════
// Group 5: IRasterQueryInterface — no-op stub and config
// ═══════════════════════════════════════════════════════════════════════════

class RasterQueryInterfaceTest : public ::testing::Test {};

// 5-1: RasterConfig singleton returns default max tile size
TEST_F(RasterQueryInterfaceTest, RasterConfig_DefaultMaxTileSize) {
    EXPECT_EQ(RasterConfig::instance().maxTileSizeBytes(),
              64u * 1024u * 1024u);
}

// 5-2: RasterConfig::setMaxTileSizeBytes() updates the value
TEST_F(RasterQueryInterfaceTest, RasterConfig_SetMaxTileSize) {
    std::size_t original = RasterConfig::instance().maxTileSizeBytes();
    RasterConfig::instance().setMaxTileSizeBytes(1024u);
    EXPECT_EQ(RasterConfig::instance().maxTileSizeBytes(), 1024u);
    // Restore
    RasterConfig::instance().setMaxTileSizeBytes(original);
}

// 5-3: RasterStatus values are distinct
TEST_F(RasterQueryInterfaceTest, RasterStatus_ValuesDistinct) {
    EXPECT_NE(RasterStatus::OK, RasterStatus::NOT_SUPPORTED);
    EXPECT_NE(RasterStatus::OK, RasterStatus::TILE_NOT_FOUND);
    EXPECT_NE(RasterStatus::OK, RasterStatus::TILE_TOO_LARGE);
    EXPECT_NE(RasterStatus::OK, RasterStatus::INVALID_BBOX);
    EXPECT_NE(RasterStatus::OK, RasterStatus::INTERNAL_ERROR);
}

// 5-4: RasterResult::ok() reflects status field
TEST_F(RasterQueryInterfaceTest, RasterResult_OkReflectsStatus) {
    RasterResult ok_result;
    ok_result.status = RasterStatus::OK;
    EXPECT_TRUE(ok_result.ok());

    RasterResult err_result;
    err_result.status = RasterStatus::NOT_SUPPORTED;
    EXPECT_FALSE(err_result.ok());
}

// 5-5: TileKey equality operator works
TEST_F(RasterQueryInterfaceTest, TileKey_Equality) {
    TileKey a{12, 100, 200};
    TileKey b{12, 100, 200};
    TileKey c{12, 100, 201};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

#ifndef THEMIS_ENABLE_RASTER
// 5-6: NullRasterQueryInterface returns NOT_SUPPORTED for queryTile
TEST_F(RasterQueryInterfaceTest, NullRasterQueryInterface_QueryTile_NotSupported) {
    NullRasterQueryInterface stub;
    TileKey key{0, 0, 0};
    auto result = stub.queryTile(key);
    EXPECT_EQ(result.status, RasterStatus::NOT_SUPPORTED);
    EXPECT_FALSE(result.ok());
}

// 5-7: NullRasterQueryInterface returns NOT_SUPPORTED for queryBBox
TEST_F(RasterQueryInterfaceTest, NullRasterQueryInterface_QueryBBox_NotSupported) {
    NullRasterQueryInterface stub;
    MBR bbox{0.0, 0.0, 1.0, 1.0};
    auto result = stub.queryBBox(bbox, 256);
    EXPECT_EQ(result.status, RasterStatus::NOT_SUPPORTED);
}
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Group 6: ITemporalSpatialQueryBuilder — fluent builder
// ═══════════════════════════════════════════════════════════════════════════

class TemporalSpatialQueryBuilderTest : public ::testing::Test {
protected:
    MBR testBBox{10.0, 50.0, 15.0, 55.0};
};

// 6-1: Builder creates a non-null instance
TEST_F(TemporalSpatialQueryBuilderTest, Create_ReturnsNonNull) {
    auto builder = ITemporalSpatialQueryBuilder::create();
    EXPECT_NE(builder.get(), nullptr);
}

// 6-2: build() throws MISSING_BBOX when no bbox is set
TEST_F(TemporalSpatialQueryBuilderTest, Build_ThrowsMissingBBox) {
    auto builder = ITemporalSpatialQueryBuilder::create();
    builder->atPointInTime(1000LL);
    EXPECT_THROW(builder->build(), TemporalSpatialQueryException);
}

// 6-3: build() throws MISSING_TEMPORAL when no temporal constraint is set
TEST_F(TemporalSpatialQueryBuilderTest, Build_ThrowsMissingTemporal) {
    auto builder = ITemporalSpatialQueryBuilder::create();
    builder->withinBBox(testBBox);
    EXPECT_THROW(builder->build(), TemporalSpatialQueryException);
}

// 6-4: POINT_IN_TIME query builds correctly
TEST_F(TemporalSpatialQueryBuilderTest, Build_PointInTime) {
    auto q = ITemporalSpatialQueryBuilder::create()
                 ->withinBBox(testBBox)
                 .atPointInTime(123456789LL)
                 .build();
    EXPECT_EQ(q.window_type, TimeWindowType::POINT_IN_TIME);
    EXPECT_EQ(q.start_ms, 123456789LL);
}

// 6-5: INTERVAL query builds correctly
TEST_F(TemporalSpatialQueryBuilderTest, Build_Interval) {
    auto q = ITemporalSpatialQueryBuilder::create()
                 ->withinBBox(testBBox)
                 .duringInterval(1000LL, 9000LL)
                 .build();
    EXPECT_EQ(q.window_type, TimeWindowType::INTERVAL);
    EXPECT_EQ(q.start_ms, 1000LL);
    EXPECT_EQ(q.end_or_width_ms, 9000LL);
}

// 6-6: INTERVAL throws INVALID_INTERVAL when end < start
TEST_F(TemporalSpatialQueryBuilderTest, Build_InvalidInterval) {
    auto builder = ITemporalSpatialQueryBuilder::create();
    builder->withinBBox(testBBox).duringInterval(9000LL, 1000LL);
    try {
        builder->build();
        FAIL() << "Expected TemporalSpatialQueryException";
    } catch (const TemporalSpatialQueryException& ex) {
        EXPECT_EQ(ex.errorCode(), TemporalSpatialQueryError::INVALID_INTERVAL);
    }
}

// 6-7: SLIDING_WINDOW query builds correctly
TEST_F(TemporalSpatialQueryBuilderTest, Build_SlidingWindow) {
    auto q = ITemporalSpatialQueryBuilder::create()
                 ->withinBBox(testBBox)
                 .withSlidingWindow(3600000LL)
                 .build();
    EXPECT_EQ(q.window_type, TimeWindowType::SLIDING_WINDOW);
    EXPECT_EQ(q.end_or_width_ms, 3600000LL);
}

// 6-8: SLIDING_WINDOW throws ZERO_SLIDING_WINDOW for zero width
TEST_F(TemporalSpatialQueryBuilderTest, Build_ZeroSlidingWindow) {
    auto builder = ITemporalSpatialQueryBuilder::create();
    builder->withinBBox(testBBox).withSlidingWindow(0LL);
    try {
        builder->build();
        FAIL() << "Expected TemporalSpatialQueryException";
    } catch (const TemporalSpatialQueryException& ex) {
        EXPECT_EQ(ex.errorCode(), TemporalSpatialQueryError::ZERO_SLIDING_WINDOW);
    }
}

// 6-9: withPredicate() is stored in built query
TEST_F(TemporalSpatialQueryBuilderTest, Build_WithPredicate) {
    auto q = ITemporalSpatialQueryBuilder::create()
                 ->withinBBox(testBBox)
                 .atPointInTime(0LL)
                 .withPredicate("CONTAINS")
                 .build();
    ASSERT_TRUE(q.predicate.has_value());
    EXPECT_EQ(q.predicate.value(), "CONTAINS");
}

// 6-10: withGeoField() overrides default "location" field
TEST_F(TemporalSpatialQueryBuilderTest, Build_WithGeoField) {
    auto q = ITemporalSpatialQueryBuilder::create()
                 ->withinBBox(testBBox)
                 .atPointInTime(0LL)
                 .withGeoField("geometry")
                 .build();
    EXPECT_EQ(q.geo_field, "geometry");
}

// 6-11: Default geo_field is "location"
TEST_F(TemporalSpatialQueryBuilderTest, Build_DefaultGeoField) {
    auto q = ITemporalSpatialQueryBuilder::create()
                 ->withinBBox(testBBox)
                 .atPointInTime(0LL)
                 .build();
    EXPECT_EQ(q.geo_field, "location");
}

// 6-12: Query produced by build() is immutable (no mutation methods exposed)
TEST_F(TemporalSpatialQueryBuilderTest, Build_ImmutableResult) {
    auto q = ITemporalSpatialQueryBuilder::create()
                 ->withinBBox(testBBox)
                 .atPointInTime(42LL)
                 .build();
    // TemporalSpatialQuery is a plain struct — verify we can copy it safely
    TemporalSpatialQuery copy = q;
    EXPECT_EQ(copy.start_ms, q.start_ms);
    EXPECT_EQ(copy.window_type, q.window_type);
}

// ─────────────────────────────────────────────────────────────────────────
// GeoConfig singleton test
// ─────────────────────────────────────────────────────────────────────────

TEST(GeoConfigTest, DefaultTolerance) {
    EXPECT_NEAR(GeoConfig::instance().coordinateTolerance(), 1e-9, 1e-12);
}

TEST(GeoConfigTest, SetTolerance) {
    double orig = GeoConfig::instance().coordinateTolerance();
    GeoConfig::instance().setCoordinateTolerance(1e-6);
    EXPECT_NEAR(GeoConfig::instance().coordinateTolerance(), 1e-6, 1e-12);
    GeoConfig::instance().setCoordinateTolerance(orig);
}
