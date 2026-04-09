/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_join_filter.cpp                            ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "geo/spatial_join_filter.h"
#include "geo/spatial_join.h"  // haversineDistanceM

#include <cmath>
#include <stdexcept>

namespace themis {
namespace geo {

// ── Helpers ───────────────────────────────────────────────────────────────

namespace {

/// Return the representative lon/lat of a geometry (centroid or first coord).
static std::pair<double, double> centroid(const IGeoJSONGeometry& g) {
    MBR b = g.bbox();
    return {(b.minx + b.maxx) * 0.5, (b.miny + b.maxy) * 0.5};
}

/// MBR-based intersects test (fast approximation).
static bool mbrIntersects(const IGeoJSONGeometry& a,
                           const IGeoJSONGeometry& b) {
    MBR ba = a.bbox();
    MBR bb = b.bbox();
    return ba.intersects(bb);
}

/// MBR-based contains test: does bbox(a) contain bbox(b)?
static bool mbrContains(const IGeoJSONGeometry& a,
                         const IGeoJSONGeometry& b) {
    MBR ba = a.bbox();
    MBR bb = b.bbox();
    return ba.minx <= bb.minx && ba.miny <= bb.miny &&
           ba.maxx >= bb.maxx && ba.maxy >= bb.maxy;
}

/// MBR-based within test: does bbox(b) contain bbox(a)?
static bool mbrWithin(const IGeoJSONGeometry& a,
                       const IGeoJSONGeometry& b) {
    return mbrContains(b, a);
}

/// MBR-based touches test: bboxes share boundary but interiors don't overlap.
static bool mbrTouches(const IGeoJSONGeometry& a,
                        const IGeoJSONGeometry& b) {
    MBR ba = a.bbox();
    MBR bb = b.bbox();
    return (ba.minx == bb.maxx || ba.maxx == bb.minx ||
            ba.miny == bb.maxy || ba.maxy == bb.miny);
}

// ── Concrete filter types ──────────────────────────────────────────────────

class IntersectsFilter final : public ISpatialJoinFilter {
public:
    bool matches(const IGeoJSONGeometry& a,
                 const IGeoJSONGeometry& b) const override {
        return mbrIntersects(a, b);
    }
};

class ContainsFilter final : public ISpatialJoinFilter {
public:
    bool matches(const IGeoJSONGeometry& a,
                 const IGeoJSONGeometry& b) const override {
        return mbrContains(a, b);
    }
};

class WithinFilter final : public ISpatialJoinFilter {
public:
    bool matches(const IGeoJSONGeometry& a,
                 const IGeoJSONGeometry& b) const override {
        return mbrWithin(a, b);
    }
};

class TouchesFilter final : public ISpatialJoinFilter {
public:
    bool matches(const IGeoJSONGeometry& a,
                 const IGeoJSONGeometry& b) const override {
        return mbrTouches(a, b);
    }
};

class DWithinFilter final : public ISpatialJoinFilter {
public:
    explicit DWithinFilter(double radius_m) : radius_m_(radius_m) {
        if (radius_m <= 0.0) {
            throw std::invalid_argument(
                "DWithin radius must be positive");
        }
    }

    bool matches(const IGeoJSONGeometry& a,
                 const IGeoJSONGeometry& b) const override {
        auto [lon1, lat1] = centroid(a);
        auto [lon2, lat2] = centroid(b);
        double dist = haversineDistanceM(lon1, lat1, lon2, lat2);
        return dist <= radius_m_;
    }

private:
    double radius_m_;
};

class AndFilter final : public ISpatialJoinFilter {
public:
    AndFilter(std::shared_ptr<ISpatialJoinFilter> l,
              std::shared_ptr<ISpatialJoinFilter> r)
        : lhs_(std::move(l)), rhs_(std::move(r)) {}

    bool matches(const IGeoJSONGeometry& a,
                 const IGeoJSONGeometry& b) const override {
        return lhs_->matches(a, b) && rhs_->matches(a, b);
    }

private:
    std::shared_ptr<ISpatialJoinFilter> lhs_, rhs_;
};

class OrFilter final : public ISpatialJoinFilter {
public:
    OrFilter(std::shared_ptr<ISpatialJoinFilter> l,
             std::shared_ptr<ISpatialJoinFilter> r)
        : lhs_(std::move(l)), rhs_(std::move(r)) {}

    bool matches(const IGeoJSONGeometry& a,
                 const IGeoJSONGeometry& b) const override {
        return lhs_->matches(a, b) || rhs_->matches(a, b);
    }

private:
    std::shared_ptr<ISpatialJoinFilter> lhs_, rhs_;
};

class NotFilter final : public ISpatialJoinFilter {
public:
    explicit NotFilter(std::shared_ptr<ISpatialJoinFilter> inner)
        : inner_(std::move(inner)) {}

    bool matches(const IGeoJSONGeometry& a,
                 const IGeoJSONGeometry& b) const override {
        return !inner_->matches(a, b);
    }

private:
    std::shared_ptr<ISpatialJoinFilter> inner_;
};

} // anonymous namespace

// ── SpatialJoinFilter factory ─────────────────────────────────────────────

std::shared_ptr<ISpatialJoinFilter> SpatialJoinFilter::intersects() {
    return std::make_shared<IntersectsFilter>();
}

std::shared_ptr<ISpatialJoinFilter> SpatialJoinFilter::contains() {
    return std::make_shared<ContainsFilter>();
}

std::shared_ptr<ISpatialJoinFilter> SpatialJoinFilter::within() {
    return std::make_shared<WithinFilter>();
}

std::shared_ptr<ISpatialJoinFilter> SpatialJoinFilter::touches() {
    return std::make_shared<TouchesFilter>();
}

std::shared_ptr<ISpatialJoinFilter> SpatialJoinFilter::dWithin(double radius_m) {
    return std::make_shared<DWithinFilter>(radius_m);
}

std::shared_ptr<ISpatialJoinFilter> SpatialJoinFilter::and_(
    std::shared_ptr<ISpatialJoinFilter> lhs,
    std::shared_ptr<ISpatialJoinFilter> rhs) {
    return std::make_shared<AndFilter>(std::move(lhs), std::move(rhs));
}

std::shared_ptr<ISpatialJoinFilter> SpatialJoinFilter::or_(
    std::shared_ptr<ISpatialJoinFilter> lhs,
    std::shared_ptr<ISpatialJoinFilter> rhs) {
    return std::make_shared<OrFilter>(std::move(lhs), std::move(rhs));
}

std::shared_ptr<ISpatialJoinFilter> SpatialJoinFilter::not_(
    std::shared_ptr<ISpatialJoinFilter> inner) {
    return std::make_shared<NotFilter>(std::move(inner));
}

} // namespace geo
} // namespace themis
