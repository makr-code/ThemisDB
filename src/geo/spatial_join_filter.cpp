/**
 * @file spatial_join_filter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//

#include "geo/spatial_join_filter.h"

#include <cmath>

#include "geo/geo_math.h"

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// BBox helpers
// ---------------------------------------------------------------------------

namespace {

/// @return Centroid of a bounding box.
Coordinate bboxCentroid(const BBox &bb) noexcept {
    return {(bb.min_x + bb.max_x) * 0.5, (bb.min_y + bb.max_y) * 0.5};
}

/// @return true if the two bounding boxes have any overlap in their interiors or edges.
bool bboxOverlap(const BBox &a, const BBox &b) noexcept {
    return a.min_x <= b.max_x && a.max_x >= b.min_x && a.min_y <= b.max_y && a.max_y >= b.min_y;
}

/// @return true if bounding box @p b is fully contained within @p a.
bool bboxContains(const BBox &a, const BBox &b) noexcept {
    return b.min_x >= a.min_x && b.max_x <= a.max_x && b.min_y >= a.min_y && b.max_y <= a.max_y;
}

/// @return true if the two bounding boxes share a boundary point but
///         have no interior overlap.
bool bboxTouches(const BBox &a, const BBox &b) noexcept {
    // They touch iff they share at least one boundary line/point
    // but do not overlap in their interiors.
    const bool share_x   = (a.min_x == b.max_x) || (a.max_x == b.min_x);
    const bool share_y   = (a.min_y == b.max_y) || (a.max_y == b.min_y);
    const bool overlap_x = a.min_x < b.max_x && a.max_x > b.min_x;
    const bool overlap_y = a.min_y < b.max_y && a.max_y > b.min_y;

    // Touch on a vertical or horizontal boundary
    if (share_x && overlap_y) {
        return true;
    }
    if (share_y && overlap_x) {
        return true;
    }
    // Touch at a corner
    if (share_x && share_y) {
        return true;
    }
    return false;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// IntersectsFilter
// ---------------------------------------------------------------------------

bool IntersectsFilter::matches(const IGeoJSONGeometry &a, const IGeoJSONGeometry &b) const {
    return bboxOverlap(a.bbox(), b.bbox());
}

// ---------------------------------------------------------------------------
// ContainsFilter
// ---------------------------------------------------------------------------

bool ContainsFilter::matches(const IGeoJSONGeometry &a, const IGeoJSONGeometry &b) const {
    return bboxContains(a.bbox(), b.bbox());
}

// ---------------------------------------------------------------------------
// WithinFilter
// ---------------------------------------------------------------------------

bool WithinFilter::matches(const IGeoJSONGeometry &a, const IGeoJSONGeometry &b) const {
    return bboxContains(b.bbox(), a.bbox());
}

// ---------------------------------------------------------------------------
// TouchesFilter
// ---------------------------------------------------------------------------

bool TouchesFilter::matches(const IGeoJSONGeometry &a, const IGeoJSONGeometry &b) const {
    return bboxTouches(a.bbox(), b.bbox());
}

// ---------------------------------------------------------------------------
// DWithinFilter
// ---------------------------------------------------------------------------

bool DWithinFilter::matches(const IGeoJSONGeometry &a, const IGeoJSONGeometry &b) const {
    const auto ca = bboxCentroid(a.bbox());
    const auto cb = bboxCentroid(b.bbox());

    double dist_m = 0.0;
    // For WGS-84 geometries use the geodesic (Haversine) formula.
    if (a.crs() == CrsId::WGS84 && b.crs() == CrsId::WGS84) {
        dist_m = haversineDistanceM(ca.x, ca.y, cb.x, cb.y);
    } else {
        // Euclidean fallback for projected CRS
        const double dx = ca.x - cb.x;
        const double dy = ca.y - cb.y;
        dist_m          = std::sqrt(dx * dx + dy * dy);
    }
    return dist_m <= radius_m_;
}

} // namespace geo
} // namespace themis
