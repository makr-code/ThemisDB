/*
 * Test suite: Raster Query Interface (raster_query_interface.h)
 *
 * Tests: RQI-01 … RQI-08
 *
 * RQI-01  NoOpRasterQueryImpl: queryTile() returns NOT_SUPPORTED
 * RQI-02  NoOpRasterQueryImpl: queryBBox() returns NOT_SUPPORTED
 * RQI-03  makeRasterQueryInterface(): empty grid → NoOp (NOT_SUPPORTED)
 * RQI-04  makeRasterQueryInterface(): non-empty grid → RasterGridQueryImpl (OK)
 * RQI-05  RasterGridQueryImpl: queryBBox() with valid bbox → OK + correct dims
 * RQI-06  RasterGridQueryImpl: queryBBox() with invalid resolution → INVALID_BBOX
 * RQI-07  RasterGridQueryImpl: queryTile() with out-of-range tile → INVALID_KEY
 * RQI-08  RasterGridQueryImpl: queryBBox() exceeding maxTileSizeBytes → TILE_TOO_LARGE
 */

#include <gtest/gtest.h>
#include "geo/raster_query_interface.h"

#include <cstddef>
#include <memory>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Build a small 4×4 raster covering [0,4]×[0,4] degrees.
RasterGrid makeSmallGrid() {
    return RasterGrid{0.0, 0.0, 4.0, 4.0, 4, 4, 1.0f};
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// RQI-01 NoOpRasterQueryImpl: queryTile() returns NOT_SUPPORTED
// ---------------------------------------------------------------------------

TEST(RasterQueryInterface, RQI01_NoOp_QueryTile_NotSupported) {
    NoOpRasterQueryImpl noop;
    TileCoord tile{0, 0, 0};
    const auto r = noop.queryTile(tile);
    EXPECT_EQ(r.status, RasterStatus::NOT_SUPPORTED);
    EXPECT_FALSE(r.ok());
    EXPECT_FALSE(r.error_message.empty());
}

// ---------------------------------------------------------------------------
// RQI-02 NoOpRasterQueryImpl: queryBBox() returns NOT_SUPPORTED
// ---------------------------------------------------------------------------

TEST(RasterQueryInterface, RQI02_NoOp_QueryBBox_NotSupported) {
    NoOpRasterQueryImpl noop;
    MBR bbox{0.0, 0.0, 1.0, 1.0};
    const auto r = noop.queryBBox(bbox, 0.01);
    EXPECT_EQ(r.status, RasterStatus::NOT_SUPPORTED);
    EXPECT_FALSE(r.ok());
}

// ---------------------------------------------------------------------------
// RQI-03 makeRasterQueryInterface(): empty grid → NoOp (NOT_SUPPORTED)
// ---------------------------------------------------------------------------

TEST(RasterQueryInterface, RQI03_Factory_EmptyGrid_NoOp) {
    auto iface = makeRasterQueryInterface(RasterGrid{});
    ASSERT_NE(iface, nullptr);
    TileCoord tile{0, 0, 0};
    const auto r = iface->queryTile(tile);
    EXPECT_EQ(r.status, RasterStatus::NOT_SUPPORTED);
}

// ---------------------------------------------------------------------------
// RQI-04 makeRasterQueryInterface(): non-empty grid → OK
// ---------------------------------------------------------------------------

TEST(RasterQueryInterface, RQI04_Factory_NonEmptyGrid_OK) {
    auto iface = makeRasterQueryInterface(makeSmallGrid(), "EPSG:4326");
    ASSERT_NE(iface, nullptr);
    // Query a tile that covers the grid
    TileCoord tile{0, 0, 1};  // zoom=1, top-left tile
    const auto r = iface->queryTile(tile);
    // The tile may or may not overlap the grid fully; just check no crash.
    // Status is either OK or TILE_TOO_LARGE — not NOT_SUPPORTED.
    EXPECT_NE(r.status, RasterStatus::NOT_SUPPORTED);
}

// ---------------------------------------------------------------------------
// RQI-05 RasterGridQueryImpl: queryBBox() with valid bbox → OK
// ---------------------------------------------------------------------------

TEST(RasterQueryInterface, RQI05_QueryBBox_Valid_OK) {
    RasterGridQueryImpl impl(makeSmallGrid(), "EPSG:4326");
    MBR bbox{0.5, 0.5, 2.5, 2.5};
    const auto r = impl.queryBBox(bbox, 0.5);
    EXPECT_EQ(r.status, RasterStatus::OK);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.crs_wkt, "EPSG:4326");
    EXPECT_EQ(r.band_count, 1u);
    EXPECT_DOUBLE_EQ(r.resolution_x, 0.5);
}

// ---------------------------------------------------------------------------
// RQI-06 RasterGridQueryImpl: queryBBox() with resolution <= 0 → INVALID_BBOX
// ---------------------------------------------------------------------------

TEST(RasterQueryInterface, RQI06_QueryBBox_InvalidResolution) {
    RasterGridQueryImpl impl(makeSmallGrid());
    MBR bbox{0.0, 0.0, 1.0, 1.0};
    const auto r = impl.queryBBox(bbox, 0.0);
    EXPECT_EQ(r.status, RasterStatus::INVALID_BBOX);
}

// ---------------------------------------------------------------------------
// RQI-07 RasterGridQueryImpl: queryTile() with out-of-range zoom → INVALID_KEY
// ---------------------------------------------------------------------------

TEST(RasterQueryInterface, RQI07_QueryTile_OutOfRange_InvalidKey) {
    RasterGridQueryImpl impl(makeSmallGrid());
    // zoom > 22 is invalid
    TileCoord bad_tile{0, 0, 25};
    const auto r = impl.queryTile(bad_tile);
    EXPECT_EQ(r.status, RasterStatus::INVALID_KEY);
}

// ---------------------------------------------------------------------------
// RQI-08 queryBBox(): exceeding maxTileSizeBytes → TILE_TOO_LARGE
// ---------------------------------------------------------------------------

TEST(RasterQueryInterface, RQI08_QueryBBox_TileTooLarge) {
    RasterGridQueryImpl impl(makeSmallGrid());
    MBR bbox{0.0, 0.0, 4.0, 4.0};
    // Set a very small max tile size — 1 byte — to force the error
    const RasterConfig tight_config{1};
    const auto r = impl.queryBBox(bbox, 0.001, tight_config);
    EXPECT_EQ(r.status, RasterStatus::TILE_TOO_LARGE);
}
