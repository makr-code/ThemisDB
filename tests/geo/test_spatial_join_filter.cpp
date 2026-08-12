/*
 * Test suite: Spatial JOIN Filter API (spatial_join_filter.h)
 *
 * Tests: SJF-01 … SJF-10
 *
 * Covers:
 *  SJF-01  IntersectsFilter: overlapping MBRs → true
 *  SJF-02  IntersectsFilter: non-overlapping MBRs → false
 *  SJF-03  ContainsFilter: a contains b → true; b contains a → false
 *  SJF-04  WithinFilter: a within b → true; b within a → false
 *  SJF-05  TouchesFilter: edge-touching MBRs → true
 *  SJF-06  DWithinFilter: centroid distance ≤ radius → true; > radius → false
 *  SJF-07  DWithinFilter: invalid (negative) radius → exception
 *  SJF-08  AndFilter: both must hold
 *  SJF-09  OrFilter: at least one must hold
 *  SJF-10  NotFilter: negation
 */

#include <gtest/gtest.h>
#include "geo/spatial_join_filter.h"
#include "geo/geo_json_geometry.h"

#include <cmath>
#include <memory>
#include <stdexcept>

using namespace themis::geo;

// ---------------------------------------------------------------------------
// Helpers: build simple GeoPolygon instances from known bounding boxes
// ---------------------------------------------------------------------------

namespace {

/// Build a CCW-winding polygon for the axis-aligned box [x0,y0]–[x1,y1].
GeoPolygon makeBox(double x0, double y0, double x1, double y1,
                   CrsId crs = CrsId::WGS84) {
    GeoPolygon::Ring ring = {
        {x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, {x0, y0}
    };
    return GeoPolygon({ring}, crs);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// SJF-01 IntersectsFilter: overlapping MBRs → true
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF01_Intersects_Overlap_True) {
    const auto f = SpatialJoinFilter::intersects();
    const auto a = makeBox(0.0, 0.0, 2.0, 2.0);
    const auto b = makeBox(1.0, 1.0, 3.0, 3.0);
    EXPECT_TRUE(f->matches(a, b));
}

// ---------------------------------------------------------------------------
// SJF-02 IntersectsFilter: non-overlapping MBRs → false
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF02_Intersects_NoOverlap_False) {
    const auto f = SpatialJoinFilter::intersects();
    const auto a = makeBox(0.0, 0.0, 1.0, 1.0);
    const auto b = makeBox(2.0, 2.0, 3.0, 3.0);
    EXPECT_FALSE(f->matches(a, b));
}

// ---------------------------------------------------------------------------
// SJF-03 ContainsFilter: a contains b
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF03_Contains_AContainsB) {
    const auto f = SpatialJoinFilter::contains();
    const auto outer = makeBox(0.0, 0.0, 10.0, 10.0);
    const auto inner = makeBox(2.0, 2.0,  8.0,  8.0);
    EXPECT_TRUE(f->matches(outer, inner))  << "outer contains inner";
    EXPECT_FALSE(f->matches(inner, outer)) << "inner does not contain outer";
}

// ---------------------------------------------------------------------------
// SJF-04 WithinFilter: a within b
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF04_Within_AWithinB) {
    const auto f = SpatialJoinFilter::within();
    const auto small = makeBox(2.0, 2.0, 4.0, 4.0);
    const auto large = makeBox(0.0, 0.0, 6.0, 6.0);
    EXPECT_TRUE(f->matches(small, large))  << "small is within large";
    EXPECT_FALSE(f->matches(large, small)) << "large is not within small";
}

// ---------------------------------------------------------------------------
// SJF-05 TouchesFilter: edge-touching MBRs → true
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF05_Touches_EdgeTouch_True) {
    const auto f = SpatialJoinFilter::touches();
    // Boxes share the x=1 edge (no interior overlap)
    const auto a = makeBox(0.0, 0.0, 1.0, 1.0);
    const auto b = makeBox(1.0, 0.0, 2.0, 1.0);
    EXPECT_TRUE(f->matches(a, b)) << "Boxes sharing x=1 edge should touch";
}

TEST(SpatialJoinFilter, SJF05b_Touches_Overlapping_False) {
    const auto f = SpatialJoinFilter::touches();
    const auto a = makeBox(0.0, 0.0, 2.0, 2.0);
    const auto b = makeBox(1.0, 1.0, 3.0, 3.0);
    // These overlap in their interiors — this is a degenerate case:
    // touch() returns false for overlapping, non-touching boxes.
    // (touching means share boundary but no interior)
    // Note: this is a bbox-level check; interior overlap ≠ touch.
    EXPECT_FALSE(f->matches(a, b)) << "Overlapping boxes do not touch";
}

// ---------------------------------------------------------------------------
// SJF-06 DWithinFilter: centroid distance ≤ radius → true; > radius → false
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF06_DWithin_Within) {
    // Two points ~111 km apart (1° latitude ≈ 111 km)
    const GeoPoint a({13.0, 52.0}, CrsId::WGS84);
    const GeoPoint b({13.0, 53.0}, CrsId::WGS84);  // 1° north

    const auto within200km = SpatialJoinFilter::dWithin(200'000.0);
    EXPECT_TRUE(within200km->matches(a, b)) << "1° latitude ≈ 111 km should be within 200 km";

    const auto within50km = SpatialJoinFilter::dWithin(50'000.0);
    EXPECT_FALSE(within50km->matches(a, b)) << "1° latitude ≈ 111 km should NOT be within 50 km";
}

// ---------------------------------------------------------------------------
// SJF-07 DWithinFilter: invalid (negative) radius → exception
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF07_DWithin_NegativeRadius_Throws) {
    EXPECT_THROW(
        {
            auto filter = SpatialJoinFilter::dWithin(-1.0);
            static_cast<void>(filter);
        },
        std::invalid_argument);
}

// ---------------------------------------------------------------------------
// SJF-08 AndFilter: both must hold
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF08_And_BothMustHold) {
    // overlapping boxes
    const auto a = makeBox(0.0, 0.0, 2.0, 2.0);
    const auto b = makeBox(1.0, 1.0, 3.0, 3.0);

    const auto and_filter = SpatialJoinFilter::and_(
        SpatialJoinFilter::intersects(),
        SpatialJoinFilter::dWithin(1'000'000.0));  // surely within 1000 km

    EXPECT_TRUE(and_filter->matches(a, b));

    // Use a very small radius that won't hold
    const auto and_filter_fail = SpatialJoinFilter::and_(
        SpatialJoinFilter::intersects(),
        SpatialJoinFilter::dWithin(0.0));  // radius 0 m fails for non-coincident

    // boxes have centroid distance > 0, so this may fail
    // a centroid = (1.0, 1.0), b centroid = (2.0, 2.0) → dist > 0
    EXPECT_FALSE(and_filter_fail->matches(a, b));
}

// ---------------------------------------------------------------------------
// SJF-09 OrFilter: at least one must hold
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF09_Or_AtLeastOneMustHold) {
    const auto a = makeBox(0.0, 0.0, 1.0, 1.0);
    const auto b = makeBox(5.0, 5.0, 6.0, 6.0);

    // Neither intersects nor is within 1 m of the other
    const auto or_filter = SpatialJoinFilter::or_(
        SpatialJoinFilter::intersects(),
        SpatialJoinFilter::dWithin(1.0));

    EXPECT_FALSE(or_filter->matches(a, b)) << "Neither intersects nor d≤1m";

    // But one of them: within 2000 km should hold
    const auto or_filter_holds = SpatialJoinFilter::or_(
        SpatialJoinFilter::intersects(),
        SpatialJoinFilter::dWithin(2'000'000.0));

    EXPECT_TRUE(or_filter_holds->matches(a, b));
}

// ---------------------------------------------------------------------------
// SJF-10 NotFilter: negation
// ---------------------------------------------------------------------------

TEST(SpatialJoinFilter, SJF10_Not_Negation) {
    const auto a = makeBox(0.0, 0.0, 2.0, 2.0);
    const auto b = makeBox(1.0, 1.0, 3.0, 3.0);  // overlaps a

    const auto not_intersects = SpatialJoinFilter::not_(
        SpatialJoinFilter::intersects());

    EXPECT_FALSE(not_intersects->matches(a, b)) << "NOT(intersects) should be false for overlapping";

    // Non-overlapping pair
    const auto c = makeBox(10.0, 10.0, 11.0, 11.0);
    EXPECT_TRUE(not_intersects->matches(a, c)) << "NOT(intersects) should be true for non-overlapping";
}
