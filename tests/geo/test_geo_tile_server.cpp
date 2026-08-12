#include <gtest/gtest.h>
#include "geo/tile_server.h"
#include "utils/geo/ewkb.h"

#include <cmath>
#include <string>
#include <vector>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// latLonToTile
// ---------------------------------------------------------------------------

TEST(LatLonToTile, Zoom0_WholeWorld) {
    // At zoom 0 the entire world is tile (0, 0).
    const TileCoord t = latLonToTile(0.0, 0.0, 0);
    EXPECT_EQ(t.x,    0u);
    EXPECT_EQ(t.y,    0u);
    EXPECT_EQ(t.zoom, 0u);
}

TEST(LatLonToTile, Zoom1_NorthWestQuadrant) {
    // At zoom 1 the NW quadrant is (0, 0).
    const TileCoord t = latLonToTile(-90.0, 45.0, 1);
    EXPECT_EQ(t.x,    0u);
    EXPECT_EQ(t.y,    0u);
    EXPECT_EQ(t.zoom, 1u);
}

TEST(LatLonToTile, Zoom1_NorthEastQuadrant) {
    // NE quadrant at zoom 1 is (1, 0).
    const TileCoord t = latLonToTile(90.0, 45.0, 1);
    EXPECT_EQ(t.x,    1u);
    EXPECT_EQ(t.y,    0u);
    EXPECT_EQ(t.zoom, 1u);
}

TEST(LatLonToTile, Zoom1_SouthEastQuadrant) {
    // SE quadrant at zoom 1 is (1, 1).
    const TileCoord t = latLonToTile(90.0, -45.0, 1);
    EXPECT_EQ(t.x,    1u);
    EXPECT_EQ(t.y,    1u);
    EXPECT_EQ(t.zoom, 1u);
}

TEST(LatLonToTile, Berlin_Zoom10) {
    // Berlin: lon=13.4050, lat=52.5200
    // Expected tile at zoom 10: x=550, y=335  (standard Web Mercator)
    const TileCoord t = latLonToTile(13.4050, 52.5200, 10);
    EXPECT_EQ(t.zoom, 10u);
    EXPECT_EQ(t.x,    550u);
    EXPECT_EQ(t.y,    335u);
}

TEST(LatLonToTile, MaxLonClamped) {
    // Longitude > 180 should be clamped to the rightmost column.
    const TileCoord t1 = latLonToTile(180.0,  0.0, 2);
    const TileCoord t2 = latLonToTile(200.0,  0.0, 2);
    EXPECT_EQ(t1.x, t2.x);
    EXPECT_EQ(t1.y, t2.y);
}

TEST(LatLonToTile, PolarLatClamped) {
    // Latitude at 90° is clamped to Mercator max ~85.05°; must not produce NaN.
    const TileCoord t = latLonToTile(0.0, 90.0, 5);
    EXPECT_EQ(t.zoom, 5u);
    EXPECT_LT(t.y, 1u << 5);  // row within valid range
}

// ---------------------------------------------------------------------------
// tileXToLon / tileYToLat
// ---------------------------------------------------------------------------

TEST(TileEdgeLon, Zoom0_WestEdge) {
    EXPECT_DOUBLE_EQ(tileXToLon(0u, 0u), -180.0);
}

TEST(TileEdgeLon, Zoom1_MiddleMeridian) {
    // x=1 at zoom 1 starts at lon 0.
    EXPECT_DOUBLE_EQ(tileXToLon(1u, 1u), 0.0);
}

TEST(TileEdgeLat, Zoom0_TopEdge) {
    // y=0 at zoom 0 is the north pole (Mercator max).
    const double lat = tileYToLat(0u, 0u);
    EXPECT_NEAR(lat, 85.0511, 0.001);
}

TEST(TileEdgeLat, Zoom1_Equator) {
    // y=1 at zoom 1 reaches the equator.
    const double lat = tileYToLat(1u, 1u);
    EXPECT_NEAR(lat, 0.0, 1e-9);
}

// ---------------------------------------------------------------------------
// tileToBBox
// ---------------------------------------------------------------------------

TEST(TileToBBox, Zoom0_CoversWorld) {
    const MBR bbox = tileToBBox({0u, 0u, 0u});
    EXPECT_NEAR(bbox.minx, -180.0,  1e-6);
    EXPECT_NEAR(bbox.maxx,  180.0,  1e-6);
    EXPECT_NEAR(bbox.maxy,  85.051, 0.001);
    EXPECT_NEAR(bbox.miny, -85.051, 0.001);
}

TEST(TileToBBox, WestEdge_LessThanEastEdge) {
    const MBR bbox = tileToBBox({100u, 200u, 10u});
    EXPECT_LT(bbox.minx, bbox.maxx);
    EXPECT_LT(bbox.miny, bbox.maxy);
}

TEST(TileToBBox, BBoxContainsTileCenter) {
    // The tile containing Berlin should have a bbox that includes Berlin's coords.
    const TileCoord t = latLonToTile(13.4050, 52.5200, 10);
    const MBR bbox   = tileToBBox(t);
    EXPECT_GE(13.4050, bbox.minx);
    EXPECT_LE(13.4050, bbox.maxx);
    EXPECT_GE(52.5200, bbox.miny);
    EXPECT_LE(52.5200, bbox.maxy);
}

// ---------------------------------------------------------------------------
// formatTileUrl
// ---------------------------------------------------------------------------

TEST(FormatTileUrl, OSMTemplate) {
    const std::string tmpl = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
    const std::string url  = formatTileUrl(tmpl, {549u, 335u, 10u});
    EXPECT_EQ(url, "https://tile.openstreetmap.org/10/549/335.png");
}

TEST(FormatTileUrl, CustomTemplate_AllPlaceholders) {
    const std::string tmpl = "http://example.com/tiles/{z}/{x}/{y}.jpg";
    const std::string url  = formatTileUrl(tmpl, {0u, 0u, 0u});
    EXPECT_EQ(url, "http://example.com/tiles/0/0/0.jpg");
}

TEST(FormatTileUrl, NoPlaceholders_Unchanged) {
    const std::string tmpl = "http://example.com/static_tile.png";
    EXPECT_EQ(formatTileUrl(tmpl, {1u, 2u, 3u}), tmpl);
}

TEST(FormatTileUrl, RepeatedPlaceholders) {
    const std::string tmpl = "{z}/{x}/{y}?z={z}";
    const std::string url  = formatTileUrl(tmpl, {5u, 10u, 2u});
    EXPECT_EQ(url, "2/5/10?z=2");
}

// ---------------------------------------------------------------------------
// tilesForBBox
// ---------------------------------------------------------------------------

TEST(TilesForBBox, SingleTile_Zoom0) {
    // Zoom 0 has exactly one tile for the whole world.
    const MBR world{-180.0, -85.05, 180.0, 85.05};
    const auto tiles = tilesForBBox(world, 0);
    ASSERT_EQ(tiles.size(), 1u);
    EXPECT_EQ(tiles.front().x,    0u);
    EXPECT_EQ(tiles.front().y,    0u);
    EXPECT_EQ(tiles.front().zoom, 0u);
}

TEST(TilesForBBox, BerlinSmallArea_Zoom10) {
    // Small area around Berlin should produce a handful of tiles at zoom 10.
    const MBR bbox{13.38, 52.50, 13.42, 52.54};
    const auto tiles = tilesForBBox(bbox, 10);
    EXPECT_GE(tiles.size(), 1u);
    EXPECT_LE(tiles.size(), 4u); // at most a 2×2 patch
    for (const auto& t : tiles) {
        EXPECT_EQ(t.zoom, 10u);
    }
}

TEST(TilesForBBox, InvertedBBox_ReturnsEmpty) {
    // min > max → no tiles.
    const MBR bad{10.0, 10.0, 5.0, 5.0};
    EXPECT_TRUE(tilesForBBox(bad, 5).empty());
}

TEST(TilesForBBox, AllTilesContainBBox) {
    // Every returned tile should overlap the query bbox.
    const MBR query{13.3, 52.4, 13.6, 52.7};
    const auto tiles = tilesForBBox(query, 12);
    for (const auto& t : tiles) {
        const MBR tb = tileToBBox(t);
        // Tile overlaps query iff tile.max > query.min AND tile.min < query.max
        EXPECT_GT(tb.maxx, query.minx);
        EXPECT_LT(tb.minx, query.maxx);
        EXPECT_GT(tb.maxy, query.miny);
        EXPECT_LT(tb.miny, query.maxy);
    }
}

// ---------------------------------------------------------------------------
// encodeVectorTile
// ---------------------------------------------------------------------------

TEST(EncodeVectorTile, EmptyInput_EmptyResult) {
    const TileCoord tile{549u, 335u, 10u};
    const VectorTileResult r = encodeVectorTile(tile, {});
    EXPECT_EQ(r.tile.x,          tile.x);
    EXPECT_EQ(r.tile.y,          tile.y);
    EXPECT_EQ(r.tile.zoom,       tile.zoom);
    EXPECT_TRUE(r.features.empty());
}

TEST(EncodeVectorTile, PointInsideTile_Included) {
    const TileCoord tile{549u, 335u, 10u};
    const MBR bbox = tileToBBox(tile);
    const double cx = (bbox.minx + bbox.maxx) / 2.0;
    const double cy = (bbox.miny + bbox.maxy) / 2.0;

    GeometryInfo pt(GeometryType::Point);
    pt.coords.emplace_back(cx, cy);

    const VectorTileResult r = encodeVectorTile(tile, {pt});
    ASSERT_EQ(r.features.size(), 1u);

    // Projected point should be near the centre of the tile (extent/2).
    const double ext = 4096.0;
    EXPECT_NEAR(r.features[0].geometry.coords[0].x, ext / 2.0, ext * 0.01);
    EXPECT_NEAR(r.features[0].geometry.coords[0].y, ext / 2.0, ext * 0.01);
}

TEST(EncodeVectorTile, PointOutsideTile_Excluded) {
    const TileCoord tile{0u, 0u, 10u};
    // A point far outside the tile bounding box.
    GeometryInfo pt(GeometryType::Point);
    pt.coords.emplace_back(170.0, -80.0); // SE corner of world; tile is NW

    const VectorTileResult r = encodeVectorTile(tile, {pt});
    EXPECT_TRUE(r.features.empty());
}

TEST(EncodeVectorTile, LineStringPreservesVertexCount) {
    const TileCoord tile{549u, 335u, 10u};
    const MBR bbox = tileToBBox(tile);

    GeometryInfo line(GeometryType::LineString);
    line.coords.emplace_back(bbox.minx, bbox.miny);
    line.coords.emplace_back(bbox.maxx, bbox.maxy);
    line.coords.emplace_back((bbox.minx + bbox.maxx) / 2.0,
                              (bbox.miny + bbox.maxy) / 2.0);

    const VectorTileResult r = encodeVectorTile(tile, {line});
    ASSERT_EQ(r.features.size(), 1u);
    EXPECT_EQ(r.features[0].geometry.coords.size(), 3u);
}

TEST(EncodeVectorTile, PolygonEncoded) {
    const TileCoord tile{549u, 335u, 10u};
    const MBR bbox = tileToBBox(tile);
    const double cx = (bbox.minx + bbox.maxx) / 2.0;
    const double cy = (bbox.miny + bbox.maxy) / 2.0;
    const double d  = (bbox.maxx - bbox.minx) * 0.1;

    GeometryInfo poly(GeometryType::Polygon);
    poly.coords.emplace_back(cx - d, cy - d);
    poly.coords.emplace_back(cx + d, cy - d);
    poly.coords.emplace_back(cx + d, cy + d);
    poly.coords.emplace_back(cx - d, cy + d);
    poly.coords.emplace_back(cx - d, cy - d); // close ring

    const VectorTileResult r = encodeVectorTile(tile, {poly});
    ASSERT_EQ(r.features.size(), 1u);
    EXPECT_EQ(r.features[0].geometry.coords.size(), 5u);
}

TEST(EncodeVectorTile, UnsupportedType_Skipped) {
    const TileCoord tile{549u, 335u, 10u};
    GeometryInfo gc(GeometryType::GeometryCollection);

    const VectorTileResult r = encodeVectorTile(tile, {gc});
    EXPECT_TRUE(r.features.empty());
}

TEST(EncodeVectorTile, CustomExtent) {
    const TileCoord tile{549u, 335u, 10u};
    const MBR bbox = tileToBBox(tile);
    const double cx = (bbox.minx + bbox.maxx) / 2.0;
    const double cy = (bbox.miny + bbox.maxy) / 2.0;

    GeometryInfo pt(GeometryType::Point);
    pt.coords.emplace_back(cx, cy);

    const uint32_t ext = 256;
    const VectorTileResult r = encodeVectorTile(tile, {pt}, ext);
    ASSERT_EQ(r.features.size(), 1u);
    EXPECT_NEAR(r.features[0].geometry.coords[0].x, ext / 2.0, ext * 0.01);
    EXPECT_NEAR(r.features[0].geometry.coords[0].y, ext / 2.0, ext * 0.01);
}

// ---------------------------------------------------------------------------
// TileLayerConfig smoke test
// ---------------------------------------------------------------------------

TEST(TileLayerConfig, DefaultValues) {
    TileLayerConfig cfg;
    EXPECT_EQ(cfg.min_zoom,  0u);
    EXPECT_EQ(cfg.max_zoom,  19u);
    EXPECT_EQ(cfg.tile_size, 256u);
    EXPECT_TRUE(cfg.url_template.empty());
}

TEST(TileLayerConfig, FormatUrl_UsingConfig) {
    TileLayerConfig cfg;
    cfg.url_template = "https://tile.example.com/{z}/{x}/{y}.png";
    const std::string url = formatTileUrl(cfg.url_template, {1u, 2u, 3u});
    EXPECT_EQ(url, "https://tile.example.com/3/1/2.png");
}
