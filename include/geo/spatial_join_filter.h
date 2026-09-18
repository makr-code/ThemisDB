/**
 * @file spatial_join_filter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

class ISpatialJoinFilter {
public:
    /**
     * @brief ISpatial Join Filter.
     * @return Return value.
     */
    virtual ~ISpatialJoinFilter() = default;

    [[nodiscard]] virtual bool matches(const IGeoJSONGeometry& a,
                                       const IGeoJSONGeometry& b) const = 0;
};

// ---------------------------------------------------------------------------
// Concrete filter types (public — allows sub-classing by plugins)
// ---------------------------------------------------------------------------

class IntersectsFilter final : public ISpatialJoinFilter {
public:
    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override;
};

class ContainsFilter final : public ISpatialJoinFilter {
public:
    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override;
};

class WithinFilter final : public ISpatialJoinFilter {
public:
    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override;
};

class TouchesFilter final : public ISpatialJoinFilter {
public:
    [[nodiscard]] bool matches(const IGeoJSONGeometry& a,
                               const IGeoJSONGeometry& b) const override;
};

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

class NotFilter final : public ISpatialJoinFilter {
public:
    /**
     * @brief Not Filter.
     * @param[in] inner Input parameter.
     * @return Return value.
     */
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

namespace SpatialJoinFilter {

[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> intersects() {
    return std::make_shared<IntersectsFilter>();
}

[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> contains() {
    return std::make_shared<ContainsFilter>();
}

[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> within() {
    return std::make_shared<WithinFilter>();
}

[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> touches() {
    return std::make_shared<TouchesFilter>();
}

[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> dWithin(double radius_m) {
    return std::make_shared<DWithinFilter>(radius_m);
}

[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> and_(
        std::shared_ptr<ISpatialJoinFilter> a,
        std::shared_ptr<ISpatialJoinFilter> b) {
    return std::make_shared<AndFilter>(std::move(a), std::move(b));
}

[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> or_(
        std::shared_ptr<ISpatialJoinFilter> a,
        std::shared_ptr<ISpatialJoinFilter> b) {
    return std::make_shared<OrFilter>(std::move(a), std::move(b));
}

[[nodiscard]] inline std::shared_ptr<ISpatialJoinFilter> not_(
        std::shared_ptr<ISpatialJoinFilter> f) {
    return std::make_shared<NotFilter>(std::move(f));
}

} // namespace SpatialJoinFilter

} // namespace geo
} // namespace themis
