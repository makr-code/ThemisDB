/**
 * @file rtree_cursor.cpp
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

#include "geo/rtree_cursor.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "geo/geo_math.h"
#include "geo/geo_rtree.h"

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Internal: snapshot of indexed entries for cursor traversal
// ---------------------------------------------------------------------------
//
// Both cursor types work from a snapshot of all matching entries taken at
// open-time.  This avoids iterator invalidation while keeping the cursor API
// simple.  The snapshot cost is O(k) for k-NN and O(hits) for range queries.

namespace {

/// Convert a GeometryInfo MBR into a Coordinate centroid (lon, lat).
Coordinate mbrCentroid(const GeometryInfo &geom) noexcept {
    const auto mbr = geom.computeMBR();
    return Coordinate{(mbr.minx + mbr.maxx) * 0.5, (mbr.miny + mbr.maxy) * 0.5};
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// RTreeRangeCursor — range query cursor (MBR overlap)
// ---------------------------------------------------------------------------

/** @brief RTreeRangeCursor — range query cursor (MBR overlap). */
class RTreeRangeCursor final : public IRTreeCursor {
  public:
    RTreeRangeCursor(std::vector<GeoIndexEntry> hits, std::size_t index_version, const std::size_t *live_version)
        : hits_(std::move(hits)), index_version_(index_version), live_version_(live_version), pos_(0) {}

    CursorStatus next(GeoIndexEntry &entry) override {
        if (live_version_ && *live_version_ != index_version_) {
            return CursorStatus::STALE;
        }
        if (pos_ >= hits_.size()) {
            return CursorStatus::END;
        }
        entry = hits_[pos_++];
        return CursorStatus::OK;
    }

    std::size_t estimatedResultCount() const noexcept override {
        return hits_.size();
    }

  private:
    std::vector<GeoIndexEntry> hits_;
    std::size_t index_version_;
    const std::size_t *live_version_; ///< Points into GeoRTreeIndex::Impl
    std::size_t pos_;
};

// ---------------------------------------------------------------------------
// RTreeKNNCursor — k-nearest-neighbour cursor (sorted by distance)
// ---------------------------------------------------------------------------

/** @brief RTreeKNNCursor — k-nearest-neighbour cursor (sorted by distance). */
class RTreeKNNCursor final : public IRTreeCursor {
  public:
    RTreeKNNCursor(std::vector<GeoIndexEntry> hits, std::size_t k, std::size_t index_version,
                   const std::size_t *live_version)
        : hits_(std::move(hits)), k_(k), index_version_(index_version), live_version_(live_version), pos_(0) {}

    CursorStatus next(GeoIndexEntry &entry) override {
        if (live_version_ && *live_version_ != index_version_) {
            return CursorStatus::STALE;
        }
        if (pos_ >= hits_.size()) {
            return CursorStatus::END;
        }
        entry = hits_[pos_++];
        return CursorStatus::OK;
    }

    std::size_t estimatedResultCount() const noexcept override {
        return std::min(k_, hits_.size());
    }

  private:
    std::vector<GeoIndexEntry> hits_;
    std::size_t k_;
    std::size_t index_version_;
    const std::size_t *live_version_;
    std::size_t pos_;
};

// ---------------------------------------------------------------------------
// GeoRTreeIndex::Impl
// ---------------------------------------------------------------------------

struct GeoRTreeIndex::Impl {
    GeoRTree rtree;
    std::size_t version{0};

    /// Snapshot of all entries for cursor materialisation
    std::vector<std::pair<std::string, GeometryInfo>> entries;

    void bumpVersion() {
        ++version;
    }
};

// ---------------------------------------------------------------------------
// GeoRTreeIndex — public API
// ---------------------------------------------------------------------------

GeoRTreeIndex::GeoRTreeIndex() : impl_(std::make_unique<Impl>()) {}
GeoRTreeIndex::~GeoRTreeIndex() = default;

GeoRTreeIndex::GeoRTreeIndex(GeoRTreeIndex &&) noexcept            = default;
GeoRTreeIndex &GeoRTreeIndex::operator=(GeoRTreeIndex &&) noexcept = default;

std::size_t GeoRTreeIndex::size() const noexcept {
    return impl_->rtree.size();
}

void GeoRTreeIndex::insert(const std::string &key, const GeometryInfo &geom) {
    impl_->rtree.insert(key, geom);
    impl_->entries.emplace_back(key, geom);
    impl_->bumpVersion();
}

void GeoRTreeIndex::bulkLoad(const std::vector<std::pair<std::string, GeometryInfo>> &entries) {
    impl_->rtree.bulkLoad(entries);
    impl_->entries = entries;
    impl_->bumpVersion();
}

void GeoRTreeIndex::clear() {
    impl_->rtree.clear();
    impl_->entries.clear();
    impl_->bumpVersion();
}

std::unique_ptr<IRTreeCursor> GeoRTreeIndex::openRangeCursor(const MBR &bbox) {
    // Materialise matching entries
    const auto &all = impl_->entries;
    std::vector<GeoIndexEntry> hits;
    hits.reserve(all.size() / 4 + 1); // optimistic reserve

    for (const auto &[key, geom] : all) {
        const auto mbr = geom.computeMBR();
        if (mbr.minx <= bbox.maxx && mbr.maxx >= bbox.minx && mbr.miny <= bbox.maxy && mbr.maxy >= bbox.miny) {
            hits.push_back({key, geom, 0.0});
        }
    }

    return std::make_unique<RTreeRangeCursor>(std::move(hits), impl_->version, &impl_->version);
}

std::unique_ptr<IRTreeCursor> GeoRTreeIndex::openKNNCursor(const Coordinate &query_point, std::size_t k) {
    const auto &all = impl_->entries;

    // Build (distance, entry) list
    std::vector<GeoIndexEntry> candidates;
    candidates.reserve(all.size());
    for (const auto &[key, geom] : all) {
        const auto centroid = mbrCentroid(geom);
        const double dist   = haversineDistanceM(query_point.x, query_point.y, centroid.x, centroid.y);
        candidates.push_back({key, geom, dist});
    }

    // Partial sort to get k nearest
    const std::size_t take = std::min(k, candidates.size());
    std::partial_sort(candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(take), candidates.end(),
                      [](const GeoIndexEntry &a, const GeoIndexEntry &b) { return a.distance_m < b.distance_m; });
    candidates.resize(take);

    return std::make_unique<RTreeKNNCursor>(std::move(candidates), k, impl_->version, &impl_->version);
}

} // namespace geo
} // namespace themis
