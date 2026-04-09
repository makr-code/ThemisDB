/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_join_filter.h                              ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "geo/geojson_geometry.h"

#include <memory>

namespace themis {
namespace geo {

/**
 * @brief Composable spatial predicate for spatial JOIN operations.
 *
 * Each filter is immutable after construction and safe to share across
 * threads.  Filters are composed via the static factory methods `and_()`,
 * `or_()`, and `not_()`.
 *
 * **Performance contract**: `matches()` for a point-vs-point containment
 * check completes in ≤ 500 ns on a single thread.
 *
 * **Access control**: `ISpatialJoinFilter` evaluates pure geometry
 * predicates.  It does NOT bypass collection-level permissions; callers
 * must enforce ACLs before invoking the filter.
 *
 * **Usage:**
 * @code
 *   auto filter = SpatialJoinFilter::intersects();
 *   bool hit = filter->matches(geomA, geomB);
 *
 *   auto combo = SpatialJoinFilter::and_(
 *       SpatialJoinFilter::contains(),
 *       SpatialJoinFilter::dWithin(500.0));
 * @endcode
 */
class ISpatialJoinFilter {
public:
    virtual ~ISpatialJoinFilter() = default;

    /**
     * @brief Evaluate the predicate for a geometry pair.
     *
     * @param a  First geometry.
     * @param b  Second geometry.
     * @return true when the predicate holds for (a, b).
     */
    virtual bool matches(const IGeoJSONGeometry& a,
                         const IGeoJSONGeometry& b) const = 0;
};

/**
 * @brief Factory for built-in spatial predicates and logical combinators.
 *
 * All returned instances are immutable and heap-allocated.
 */
class SpatialJoinFilter {
public:
    // ── Built-in OGC predicates ────────────────────────────────────────

    /// Predicate: geometries share at least one point (DE-9IM: T********).
    static std::shared_ptr<ISpatialJoinFilter> intersects();

    /// Predicate: geometry @p a contains geometry @p b entirely.
    static std::shared_ptr<ISpatialJoinFilter> contains();

    /// Predicate: geometry @p a is entirely within geometry @p b.
    static std::shared_ptr<ISpatialJoinFilter> within();

    /// Predicate: geometries share boundary points but no interior points.
    static std::shared_ptr<ISpatialJoinFilter> touches();

    /**
     * @brief Predicate: the distance between @p a and @p b is ≤ @p radius_m.
     *
     * Uses the Haversine formula for geodesic distance on the WGS84 sphere.
     *
     * @param radius_m  Maximum distance threshold in metres (must be > 0).
     */
    static std::shared_ptr<ISpatialJoinFilter> dWithin(double radius_m);

    // ── Logical combinators ────────────────────────────────────────────

    /**
     * @brief Return a filter that is true when both @p lhs AND @p rhs hold.
     */
    static std::shared_ptr<ISpatialJoinFilter> and_(
        std::shared_ptr<ISpatialJoinFilter> lhs,
        std::shared_ptr<ISpatialJoinFilter> rhs);

    /**
     * @brief Return a filter that is true when either @p lhs OR @p rhs holds.
     */
    static std::shared_ptr<ISpatialJoinFilter> or_(
        std::shared_ptr<ISpatialJoinFilter> lhs,
        std::shared_ptr<ISpatialJoinFilter> rhs);

    /**
     * @brief Return a filter that negates @p inner.
     */
    static std::shared_ptr<ISpatialJoinFilter> not_(
        std::shared_ptr<ISpatialJoinFilter> inner);
};

} // namespace geo
} // namespace themis
