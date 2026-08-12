/*
 * Test suite: Temporal-Spatial Query Builder (temporal_spatial_query_builder.h)
 *
 * Tests: TSB-01 … TSB-08
 *
 * TSB-01  Builder: build() without temporal constraint throws logic_error
 * TSB-02  Builder: build() without spatial constraint throws logic_error
 * TSB-03  Builder: atTime() + withinBBox() → POINT_IN_TIME query immutable after build()
 * TSB-04  Builder: duringInterval() with end < start throws invalid_argument
 * TSB-05  Builder: duringInterval() + withinBBox() → INTERVAL window type
 * TSB-06  Builder: slidingWindow() with zero/negative ms throws invalid_argument
 * TSB-07  Builder: slidingWindow() + withPredicate() → SLIDING_WINDOW type
 * TSB-08  Builder: reset() clears constraints; build() throws after reset
 */

#include <gtest/gtest.h>
#include "geo/temporal_spatial_query_builder.h"

#include <stdexcept>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// TSB-01 build() without temporal constraint throws logic_error
// ---------------------------------------------------------------------------

TEST(TemporalSpatialQueryBuilder, TSB01_MissingTemporal_Throws) {
    TemporalSpatialQueryBuilder builder;
    static_cast<void>(builder.withinBBox(MBR{0.0, 0.0, 10.0, 10.0}));
    EXPECT_THROW(static_cast<void>(builder.build()), std::logic_error);
}

// ---------------------------------------------------------------------------
// TSB-02 build() without spatial constraint throws logic_error
// ---------------------------------------------------------------------------

TEST(TemporalSpatialQueryBuilder, TSB02_MissingSpatial_Throws) {
    TemporalSpatialQueryBuilder builder;
    static_cast<void>(builder.atTime(1'000'000LL));
    EXPECT_THROW(static_cast<void>(builder.build()), std::logic_error);
}

// ---------------------------------------------------------------------------
// TSB-03 atTime() + withinBBox() → POINT_IN_TIME; immutable after build()
// ---------------------------------------------------------------------------

TEST(TemporalSpatialQueryBuilder, TSB03_PointInTime_BBox) {
    TemporalSpatialQueryBuilder builder;
    builder.atTime(42LL).withinBBox(MBR{10.0, 50.0, 15.0, 55.0});

    // build() must succeed
    const auto q = builder.build();
    EXPECT_EQ(q.windowType(), TimeWindowType::POINT_IN_TIME);
    EXPECT_DOUBLE_EQ(q.bbox().minx, 10.0);
    EXPECT_DOUBLE_EQ(q.bbox().maxy, 55.0);

    // build() a second time must also succeed (builder not consumed)
    const auto q2 = builder.build();
    EXPECT_EQ(q2.windowType(), TimeWindowType::POINT_IN_TIME);
}

// ---------------------------------------------------------------------------
// TSB-04 duringInterval() with end < start throws invalid_argument
// ---------------------------------------------------------------------------

TEST(TemporalSpatialQueryBuilder, TSB04_InvalidInterval_Throws) {
    TemporalSpatialQueryBuilder builder;
    EXPECT_THROW(builder.duringInterval(100LL, 50LL), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// TSB-05 duringInterval() + withinBBox() → INTERVAL window type
// ---------------------------------------------------------------------------

TEST(TemporalSpatialQueryBuilder, TSB05_Interval_BBox) {
    TemporalSpatialQueryBuilder builder;
    builder.duringInterval(1000LL, 2000LL)
           .withinBBox(MBR{0.0, 0.0, 5.0, 5.0});
    const auto q = builder.build();
    EXPECT_EQ(q.windowType(), TimeWindowType::INTERVAL);
}

// ---------------------------------------------------------------------------
// TSB-06 slidingWindow() with zero/negative throws invalid_argument
// ---------------------------------------------------------------------------

TEST(TemporalSpatialQueryBuilder, TSB06_InvalidSlidingWindow_Throws) {
    TemporalSpatialQueryBuilder builder;
    EXPECT_THROW(builder.slidingWindow(0LL), std::invalid_argument);
    EXPECT_THROW(builder.slidingWindow(-1000LL), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// TSB-07 slidingWindow() + withPredicate() → SLIDING_WINDOW type
// ---------------------------------------------------------------------------

TEST(TemporalSpatialQueryBuilder, TSB07_SlidingWindow_Predicate) {
    TemporalSpatialQueryBuilder builder;
    builder.slidingWindow(60'000LL)  // 1-minute window
           .withPredicate(SpatialJoinFilter::dWithin(500'000.0));  // 500 km
    const auto q = builder.build();
    EXPECT_EQ(q.windowType(), TimeWindowType::SLIDING_WINDOW);
}

// ---------------------------------------------------------------------------
// TSB-08 reset() clears constraints; build() throws after reset
// ---------------------------------------------------------------------------

TEST(TemporalSpatialQueryBuilder, TSB08_Reset_ClearsConstraints) {
    TemporalSpatialQueryBuilder builder;
    static_cast<void>(builder.atTime(1000LL).withinBBox(MBR{0.0, 0.0, 1.0, 1.0}));

    // Build succeeds
    EXPECT_NO_THROW({
        static_cast<void>(builder.build());
    });

    // After reset, build() must throw
    builder.reset();
    EXPECT_THROW(static_cast<void>(builder.build()), std::logic_error);
}
