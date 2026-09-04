/**
 * @file spatial_join_filter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "geo/geo_json_geometry.h"

#include <memory>
#include <stdexcept>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// ISpatialJoinFilter — abstract base
// ---------------------------------------------------------------------------

/**
 * @brief Abstract spatial join predicate.
 *
 * A filter tests whether two `IGeoJSONGeometry` objects satisfy a spatial
 * relationship.  Implementations must be thread-safe and immutable.
 */
class ISpatialJoinFilter {
public:
    virtual ~ISpatialJoinFilter() = default;

    /**
     * @brief Test whether geometries @p a and @p b satisfy the predicate.
     *
     * @param a  First geometry.
     * @param b  Second geometry.
     * @return true if the spatial relationship holds.
     */
    [[nodiscard]] virtual bool matches(const IGeoJSONGeometry& a,
                                       const IGeoJSONGeometry& b) const = 0;
};

// ---------------------------------------------------------------------------
// Concrete filter types (public — allows sub-classing by plugins)
// ---------------------------------------------------------------------------

/**
 * @brief Predicate: bounding boxes of a and b overlap (MBR-based intersect).
 *
 * Uses MBR overlap as a conservative approximate test.  Two geometries with
 * non-overlapping bounding boxes cannot intersect.
 */
class IntersectsFilter final : public ISpatialJoinFilter {
public:
    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override;
};

/**
 * @brief Predicate: bounding box of b is fully contained in bounding box of a.
 *
 * MBR containment: a.bbox ⊇ b.bbox.
 */
class ContainsFilter final : public ISpatialJoinFilter {
public:
    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override;
};

/**
 * @brief Predicate: bounding box of a is fully contained in bounding box of b.
 *
 * Equivalent to `ContainsFilter` with arguments swapped.
 */
class WithinFilter final : public ISpatialJoinFilter {
public:
    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override;
};

/**
 * @brief Predicate: bounding boxes of a and b share at least one boundary point
 *        but do not overlap in the interior (MBR boundary touch).
 *
 * Two MBRs "touch" when they share exactly one edge or corner but do not have
 * interior overlap.
 */
class TouchesFilter final : public ISpatialJoinFilter {
public:
    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override;
};

/**
 * @brief Predicate: the geodesic distance between the bbox centroids is ≤
 *        @p radius_m metres.
 *
 * Distance is computed using the Haversine formula on the WGS-84 sphere.
 * For non-WGS-84 geometries, a Euclidean centroid-distance is used instead.
 *
 * @param radius_m  Maximum allowed distance in metres (must be ≥ 0).
 */
class DWithinFilter final : public ISpatialJoinFilter {
public:
    explicit DWithinFilter(double radius_m) : radius_m_(radius_m) {
        if (radius_m < 0.0) {
          throw std::invalid_argument("DWithinFilter: radius_m must be >= 0");
        }
    }

    [[nodiscard]] double radiusM() const noexcept { return radius_m_; }

    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override;

private:
    double radius_m_;
};

// ---------------------------------------------------------------------------
// Logical combinators
// ---------------------------------------------------------------------------

/**
 * @brief Predicate: both @p left and @p right predicates must hold.
 */
class AndFilter final : public ISpatialJoinFilter {
public:
    AndFilter(std::shared_ptr<ISpatialJoinFilter> left,
              std::shared_ptr<ISpatialJoinFilter> right)
        : left_(std::move(left)), right_(std::move(right)) {}

    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override {
        return left_->matches(a, b) && right_->matches(a, b);
    }

private:
    std::shared_ptr<ISpatialJoinFilter> left_;
    std::shared_ptr<ISpatialJoinFilter> right_;
};

/**
 * @brief Predicate: at least one of @p left or @p right predicates must hold.
 */
class OrFilter final : public ISpatialJoinFilter {
public:
    OrFilter(std::shared_ptr<ISpatialJoinFilter> left,
             std::shared_ptr<ISpatialJoinFilter> right)
        : left_(std::move(left)), right_(std::move(right)) {}

    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override {
        return left_->matches(a, b) || right_->matches(a, b);
    }

private:
    std::shared_ptr<ISpatialJoinFilter> left_;
    std::shared_ptr<ISpatialJoinFilter> right_;
};

/**
 * @brief Predicate: the wrapped filter must NOT hold.
 */
class NotFilter final : public ISpatialJoinFilter {
public:
    explicit NotFilter(std::shared_ptr<ISpatialJoinFilter> inner)
        : inner_(std::move(inner)) {}

    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override {
        return !inner_->matches(a, b);
    }

private:
    std::shared_ptr<ISpatialJoinFilter> inner_;
};

// ---------------------------------------------------------------------------
// Factory namespace
// ---------------------------------------------------------------------------

/**
 * @brief Factory functions for spatial join predicates.
 *
 * All returned instances are immutable and thread-safe.
 */
namespace SpatialJoinFilter {

/// Return a predicate that tests MBR intersection.
[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> intersects() {
    return std::make_shared<IntersectsFilter>();
}

/// Return a predicate that tests MBR containment (a contains b).
[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> contains() {
    return std::make_shared<ContainsFilter>();
}

/// Return a predicate that tests MBR containment (a is within b).
[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> within() {
    return std::make_shared<WithinFilter>();
}

/// Return a predicate that tests MBR boundary touch.
[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> touches() {
    return std::make_shared<TouchesFilter>();
}

/// Return a predicate that tests centroid-to-centroid geodesic distance ≤ radius_m.
[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> dWithin(double radius_m) {
    return std::make_shared<DWithinFilter>(radius_m);
}

/// Return a predicate that holds when both @p a and @p b hold.
[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> and_(
        std::shared_ptr<ISpatialJoinFilter> a,
        std::shared_ptr<ISpatialJoinFilter> b) {
    return std::make_shared<AndFilter>(std::move(a), std::move(b));
}

/// Return a predicate that holds when at least one of @p a or @p b holds.
[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> or_(
        std::shared_ptr<ISpatialJoinFilter> a,
        std::shared_ptr<ISpatialJoinFilter> b) {
    return std::make_shared<OrFilter>(std::move(a), std::move(b));
}

/// Return a predicate that holds when @p f does NOT hold.
[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> not_(
        std::shared_ptr<ISpatialJoinFilter> f) {
    return std::make_shared<NotFilter>(std::move(f));
}

} // namespace SpatialJoinFilter

} // namespace geo
} // namespace themis
