#include <gtest/gtest.h>
#include "geo/raster.h"
#include "utils/geo/ewkb.h"

#include <cmath>
#include <vector>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Fill a grid row-major with consecutive integers 0, 1, 2, ...
static void fillSequential(RasterGrid& g) {
    for (std::size_t i = 0; i < g.data.size(); ++i) {
        g.data[i] = static_cast<float>(i);
    }
}

// ---------------------------------------------------------------------------
// RasterGrid construction and accessors
// ---------------------------------------------------------------------------

TEST(RasterGrid, DefaultConstructed_IsEmpty) {
    RasterGrid g;
    EXPECT_TRUE(g.empty());
}

TEST(RasterGrid, Constructor_SetsExpectedFields) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 4, 4, 0.0f);
    EXPECT_EQ(g.width,  4u);
    EXPECT_EQ(g.height, 4u);
    EXPECT_EQ(g.data.size(), 16u);
    EXPECT_DOUBLE_EQ(g.cell_size_x, 0.25);
    EXPECT_DOUBLE_EQ(g.cell_size_y, 0.25);
    EXPECT_FALSE(g.empty());
}

TEST(RasterGrid, At_OutOfBounds_ReturnsNoData) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 3, 3, 0.0f);
    EXPECT_TRUE(g.isNoData(g.at(3, 0)));
    EXPECT_TRUE(g.isNoData(g.at(0, 3)));
    EXPECT_TRUE(g.isNoData(g.at(99, 99)));
}

TEST(RasterGrid, SetAndAt_RoundTrip) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 3, 3, 0.0f);
    g.set(1, 2, 42.0f);
    EXPECT_FLOAT_EQ(g.at(1, 2), 42.0f);
}

TEST(RasterGrid, Set_OutOfBounds_IsNoOp) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 2, 2, 5.0f);
    g.set(99, 0, -1.0f);
    EXPECT_FLOAT_EQ(g.at(0, 0), 5.0f); // unchanged
}

TEST(RasterGrid, IsNoData_NaN) {
    RasterGrid g;
    EXPECT_TRUE(g.isNoData(std::numeric_limits<float>::quiet_NaN()));
    EXPECT_FALSE(g.isNoData(0.0f));
    EXPECT_FALSE(g.isNoData(-1.0f));
}

// ---------------------------------------------------------------------------
// sampleAt — exact cell centres
// ---------------------------------------------------------------------------

TEST(SampleAt, EmptyGrid_ReturnsInvalid) {
    RasterGrid g;
    auto result = sampleAt(g, 0.5, 0.5);
    EXPECT_FALSE(result.valid);
}

TEST(SampleAt, PointOutsideGrid_ReturnsInvalid) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 4, 4, 1.0f);
    EXPECT_FALSE(sampleAt(g, -0.1, 0.5).valid);
    EXPECT_FALSE(sampleAt(g,  1.1, 0.5).valid);
    EXPECT_FALSE(sampleAt(g,  0.5, -0.1).valid);
    EXPECT_FALSE(sampleAt(g,  0.5,  1.1).valid);
}

TEST(SampleAt, ExactCellCentre_ReturnsExactValue) {
    // 2×2 grid covering [0,1]×[0,1]; cell size = 0.5
    // Cell centres: (0.25, 0.25), (0.75, 0.25), (0.25, 0.75), (0.75, 0.75)
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 2, 2, 0.0f);
    g.set(0, 0, 10.0f);
    g.set(1, 0, 20.0f);
    g.set(0, 1, 30.0f);
    g.set(1, 1, 40.0f);

    auto r00 = sampleAt(g, 0.25, 0.25);
    EXPECT_TRUE(r00.valid);
    EXPECT_NEAR(r00.value, 10.0f, 1e-4f);

    auto r10 = sampleAt(g, 0.75, 0.25);
    EXPECT_TRUE(r10.valid);
    EXPECT_NEAR(r10.value, 20.0f, 1e-4f);

    auto r01 = sampleAt(g, 0.25, 0.75);
    EXPECT_TRUE(r01.valid);
    EXPECT_NEAR(r01.value, 30.0f, 1e-4f);

    auto r11 = sampleAt(g, 0.75, 0.75);
    EXPECT_TRUE(r11.valid);
    EXPECT_NEAR(r11.value, 40.0f, 1e-4f);
}

TEST(SampleAt, Midpoint_IsBilinearAverage) {
    // Sample at the centre of a 2×2 grid → average of all four cells.
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 2, 2, 0.0f);
    g.set(0, 0, 10.0f);
    g.set(1, 0, 20.0f);
    g.set(0, 1, 30.0f);
    g.set(1, 1, 40.0f);

    auto r = sampleAt(g, 0.5, 0.5); // geometric centre of the 2×2 grid
    EXPECT_TRUE(r.valid);
    EXPECT_NEAR(r.value, 25.0f, 1e-3f); // (10+20+30+40)/4 = 25
}

TEST(SampleAt, LinearInterpolation_Horizontal) {
    // 1×1 grid not useful; use 1-row / 2-col: interpolate along longitude.
    RasterGrid g(0.0, 0.0, 1.0, 0.5, 2, 1, 0.0f);
    g.set(0, 0, 0.0f);
    g.set(1, 0, 100.0f);

    // At the midpoint between the two column centres the value should be ~50.
    auto r = sampleAt(g, 0.5, 0.25); // lon=0.5 sits between centres 0.25 and 0.75
    EXPECT_TRUE(r.valid);
    EXPECT_NEAR(r.value, 50.0f, 1.0f);
}

TEST(SampleAt, NoDataCells_ExcludedFromBlend) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 2, 2, 0.0f);
    g.set(0, 0, 10.0f);
    g.set(1, 0, 20.0f);
    // Mark the top two cells as no-data.
    g.set(0, 1, g.no_data_value);
    g.set(1, 1, g.no_data_value);

    // Sampling near the top should still produce a valid result from the two
    // valid bottom cells.
    auto r = sampleAt(g, 0.5, 0.5);
    EXPECT_TRUE(r.valid);
    EXPECT_NEAR(r.value, 15.0f, 1.0f); // average of 10 and 20
}

TEST(SampleAt, AllNoData_ReturnsInvalid) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 2, 2);
    // Default-fill is NaN (no-data), so all cells are no-data.
    auto r = sampleAt(g, 0.5, 0.5);
    EXPECT_FALSE(r.valid);
}

// ---------------------------------------------------------------------------
// queryBBox — sub-raster extraction
// ---------------------------------------------------------------------------

TEST(QueryBBox, EmptyGrid_ReturnsEmpty) {
    RasterGrid g;
    MBR bbox(0.0, 0.0, 1.0, 1.0);
    EXPECT_TRUE(queryBBox(g, bbox).empty());
}

TEST(QueryBBox, BboxOutsideGrid_ReturnsEmpty) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 4, 4, 1.0f);
    MBR outside(2.0, 2.0, 3.0, 3.0);
    EXPECT_TRUE(queryBBox(g, outside).empty());
}

TEST(QueryBBox, FullExtent_ReturnsSameSize) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 4, 4, 0.0f);
    fillSequential(g);
    MBR full(0.0, 0.0, 1.0, 1.0);
    auto sub = queryBBox(g, full);
    EXPECT_EQ(sub.width,  g.width);
    EXPECT_EQ(sub.height, g.height);
    // Values should match.
    for (std::size_t r = 0; r < g.height; ++r) {
        for (std::size_t c = 0; c < g.width; ++c) {
            EXPECT_FLOAT_EQ(sub.at(c, r), g.at(c, r));
        }
    }
}

TEST(QueryBBox, HalfExtent_ReturnsCells) {
    // 4×4 grid [0,1]×[0,1]; cell size = 0.25
    // Cell centres: col 0→0.125, 1→0.375, 2→0.625, 3→0.875
    //               row 0→0.125, 1→0.375, 2→0.625, 3→0.875
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 4, 4, 0.0f);
    fillSequential(g);

    // Query the right half: lon ∈ [0.5, 1.0] → columns 2 and 3
    //                       lat ∈ [0.0, 1.0] → rows   0-3
    MBR right(0.5, 0.0, 1.0, 1.0);
    auto sub = queryBBox(g, right);
    EXPECT_EQ(sub.width,  2u);
    EXPECT_EQ(sub.height, 4u);
    EXPECT_FLOAT_EQ(sub.at(0, 0), g.at(2, 0));
    EXPECT_FLOAT_EQ(sub.at(1, 3), g.at(3, 3));
}

TEST(QueryBBox, PreservesNoDataSentinel) {
    RasterGrid g(0.0, 0.0, 1.0, 1.0, 2, 2, 1.0f);
    g.set(1, 1, g.no_data_value);
    MBR full(0.0, 0.0, 1.0, 1.0);
    auto sub = queryBBox(g, full);
    EXPECT_TRUE(sub.isNoData(sub.at(1, 1)));
}

// ---------------------------------------------------------------------------
// generateHeatmap
// ---------------------------------------------------------------------------

TEST(GenerateHeatmap, EmptyPoints_ReturnsEmpty) {
    std::vector<Coordinate> pts;
    MBR bbox(0.0, 0.0, 1.0, 1.0);
    auto h = generateHeatmap(pts, bbox);
    EXPECT_TRUE(h.empty());
}

TEST(GenerateHeatmap, ZeroWidthConfig_ReturnsEmpty) {
    std::vector<Coordinate> pts{{0.5, 0.5}};
    MBR bbox(0.0, 0.0, 1.0, 1.0);
    HeatmapConfig cfg;
    cfg.width = 0;
    EXPECT_TRUE(generateHeatmap(pts, bbox, cfg).empty());
}

TEST(GenerateHeatmap, InvalidBbox_ReturnsEmpty) {
    std::vector<Coordinate> pts{{0.5, 0.5}};
    MBR bad(1.0, 1.0, 0.0, 0.0); // inverted
    EXPECT_TRUE(generateHeatmap(pts, bad).empty());
}

TEST(GenerateHeatmap, SinglePoint_PeakAtCellNearest) {
    std::vector<Coordinate> pts{{0.5, 0.5}};
    MBR bbox(0.0, 0.0, 1.0, 1.0);
    HeatmapConfig cfg;
    cfg.width  = 10;
    cfg.height = 10;
    cfg.bandwidth_m = 50000.0; // large bandwidth to spread across cells

    auto h = generateHeatmap(pts, bbox, cfg);
    EXPECT_FALSE(h.empty());
    EXPECT_EQ(h.width,  10u);
    EXPECT_EQ(h.height, 10u);

    // The peak should be in the centre cell (col 4 or 5, row 4 or 5 for 10×10).
    float max_val = -1.0f;
    std::size_t max_c = 0, max_r = 0;
    for (std::size_t r = 0; r < h.height; ++r) {
        for (std::size_t c = 0; c < h.width; ++c) {
            float v = h.at(c, r);
            if (v > max_val) { max_val = v; max_c = c; max_r = r; }
        }
    }
    EXPECT_GE(max_c, 4u);
    EXPECT_LE(max_c, 5u);
    EXPECT_GE(max_r, 4u);
    EXPECT_LE(max_r, 5u);
}

TEST(GenerateHeatmap, Normalize_MaxIsOne) {
    std::vector<Coordinate> pts{{0.5, 0.5}, {0.2, 0.2}};
    MBR bbox(0.0, 0.0, 1.0, 1.0);
    HeatmapConfig cfg;
    cfg.width  = 8;
    cfg.height = 8;
    cfg.bandwidth_m = 30000.0;
    cfg.normalize = true;

    auto h = generateHeatmap(pts, bbox, cfg);
    float max_val = 0.0f;
    for (float v : h.data) {
        if (!std::isnan(v) && v > max_val) max_val = v;
    }
    EXPECT_NEAR(max_val, 1.0f, 1e-5f);
}

TEST(GenerateHeatmap, PointOutsideBbox_Ignored) {
    // Place only a point outside the grid; density should be all zeros.
    std::vector<Coordinate> pts{{5.0, 5.0}}; // outside [0,1]×[0,1]
    MBR bbox(0.0, 0.0, 1.0, 1.0);
    HeatmapConfig cfg;
    cfg.width  = 5;
    cfg.height = 5;
    cfg.bandwidth_m = 1000.0;
    auto h = generateHeatmap(pts, bbox, cfg);
    for (float v : h.data) {
        EXPECT_FLOAT_EQ(v, 0.0f);
    }
}

TEST(GenerateHeatmap, TwoPoints_BothContribute) {
    // Place two well-separated points; both should produce positive density.
    std::vector<Coordinate> pts{{0.1, 0.1}, {0.9, 0.9}};
    MBR bbox(0.0, 0.0, 1.0, 1.0);
    HeatmapConfig cfg;
    cfg.width  = 10;
    cfg.height = 10;
    cfg.bandwidth_m = 100000.0; // very wide kernel to ensure overlap with grid cells
    cfg.normalize = false;

    auto h = generateHeatmap(pts, bbox, cfg);
    // Corner cells should have positive density for a wide-enough kernel.
    EXPECT_GT(h.at(0, 0), 0.0f);
    EXPECT_GT(h.at(9, 9), 0.0f);
}
