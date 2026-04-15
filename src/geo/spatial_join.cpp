/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_join.cpp                                   ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:11:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     275                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3a43c52c92  2026-03-13  feat(geo): add SpatialJoinIterator lazy iterator and AQL ... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "geo/spatial_join.h"
#include "geo/geo_rtree.h"
#include "utils/logger.h"

#include <cmath>
#include <unordered_map>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Haversine distance helper
// ---------------------------------------------------------------------------

static constexpr double kPi         = 3.14159265358979323846;
static constexpr double kEarthRadiusM = 6371000.0; // mean Earth radius in metres

double haversineDistanceM(double lon1, double lat1,
                          double lon2, double lat2) noexcept {
    const double rlat1 = lat1 * kPi / 180.0;
    const double rlat2 = lat2 * kPi / 180.0;
    const double dlat  = (lat2 - lat1) * kPi / 180.0;
    const double dlon  = (lon2 - lon1) * kPi / 180.0;

    const double sinDlat = std::sin(dlat * 0.5);
    const double sinDlon = std::sin(dlon * 0.5);
    const double a = sinDlat * sinDlat
                   + std::cos(rlat1) * std::cos(rlat2) * sinDlon * sinDlon;
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusM * c;
}

// ---------------------------------------------------------------------------
// Centroid extraction for distance computation
// ---------------------------------------------------------------------------

/// Return the representative point (lon, lat) for a geometry.
/// For a Point the coordinate itself is used; for all other types the centroid
/// computed by GeometryInfo::computeCentroid() is used.
static Coordinate geometryCentroid(const GeometryInfo& geom) {
    if (geom.isPoint() && !geom.coords.empty()) {
        return geom.coords[0];
    }
    return geom.computeCentroid();
}

// ---------------------------------------------------------------------------
// spatialJoin implementation
// ---------------------------------------------------------------------------

std::vector<SpatialJoinPair> spatialJoin(
    const std::vector<std::pair<std::string, GeometryInfo>>& outer,
    const std::vector<std::pair<std::string, GeometryInfo>>& inner,
    double threshold_m,
    const SpatialJoinConfig& config)
{
    if (threshold_m <= 0.0) {
        THEMIS_WARN("spatialJoin: threshold_m ({}) must be positive; returning empty result",
                    threshold_m);
        return {};
    }

    std::vector<SpatialJoinPair> results;

    if (outer.empty() || inner.empty()) {
        return results;
    }

    // Build R-tree index on the inner collection for sub-linear candidate lookup.
    GeoRTree index;
    index.bulkLoad(inner);

    // Pre-compute centroids for all inner geometries keyed by position, because
    // the R-tree returns keys (strings) and we need O(1) centroid lookup.
    // Build a map key -> (idx in inner) for centroid retrieval.
    // Since inner keys may not be unique, we store the first occurrence.
    std::unordered_map<std::string, std::size_t> inner_key_idx;
    inner_key_idx.reserve(inner.size());
    for (std::size_t i = 0; i < inner.size(); ++i) {
        inner_key_idx.emplace(inner[i].first, i);
    }

    bool limit_reached = false;

    for (const auto& [key_a, geom_a] : outer) {
        if (limit_reached) break;

        const Coordinate centroid_a = geometryCentroid(geom_a);

        // Expand the geometry's MBR by threshold_m to get the candidate search box.
        const MBR search_box = geom_a.computeMBR().expand(threshold_m);

        // Query R-tree for all inner geometries whose MBR intersects the search box.
        const std::vector<std::string> candidates = index.intersects(search_box);

        for (const auto& key_b : candidates) {
            auto it = inner_key_idx.find(key_b);
            if (it == inner_key_idx.end()) continue;

            const Coordinate centroid_b = geometryCentroid(inner[it->second].second);

            const double dist = haversineDistanceM(centroid_a.x, centroid_a.y,
                                                   centroid_b.x, centroid_b.y);
            if (dist <= threshold_m) {
                results.push_back({key_a, key_b, dist});

                if (results.size() >= config.max_pairs) {
                    THEMIS_WARN("spatialJoin: max_pairs limit ({}) reached; "
                                "result set may be incomplete", config.max_pairs);
                    limit_reached = true;
                    break;
                }
            }
        }
    }

    return results;
}

// ---------------------------------------------------------------------------
// SpatialJoinIterator – lazy, one-pair-at-a-time implementation
// ---------------------------------------------------------------------------

struct SpatialJoinIterator::Impl {
    // References into the caller-owned outer collection.
    const std::vector<std::pair<std::string, GeometryInfo>>* outer_ptr;
    double threshold_m;
    SpatialJoinConfig config;

    // R-tree index built once over the inner collection.
    GeoRTree index;
    // key -> inner-vector index (first occurrence wins for duplicate keys).
    std::unordered_map<std::string, std::size_t> inner_key_idx;
    // Owning copy of the inner geometries (needed for centroid lookup).
    const std::vector<std::pair<std::string, GeometryInfo>>* inner_ptr;

    // Iterator state --------------------------------------------------------
    std::size_t outer_idx    = 0; // current position in outer
    std::size_t cand_idx     = 0; // current position in current candidate list
    std::vector<std::string> candidates; // R-tree results for current outer element
    std::size_t pairs_yielded = 0;
    bool exhausted = false;

    // Last yielded pair (valid when not exhausted and advance() returned true).
    SpatialJoinPair current_pair{};

    Impl(const std::vector<std::pair<std::string, GeometryInfo>>& outer,
         const std::vector<std::pair<std::string, GeometryInfo>>& inner,
         double thr,
         const SpatialJoinConfig& cfg)
        : outer_ptr(&outer), threshold_m(thr), config(cfg), inner_ptr(&inner)
    {
        if (thr <= 0.0 || outer.empty() || inner.empty()) {
            exhausted = true;
            return;
        }
        index.bulkLoad(inner);
        inner_key_idx.reserve(inner.size());
        for (std::size_t i = 0; i < inner.size(); ++i) {
            inner_key_idx.emplace(inner[i].first, i);
        }
        // Pre-load candidates for the first outer element.
        loadCandidates();
    }

    void loadCandidates() {
        candidates.clear();
        cand_idx = 0;
        if (outer_idx >= outer_ptr->size()) {
            exhausted = true;
            return;
        }
        const GeometryInfo& geom_a = (*outer_ptr)[outer_idx].second;
        const MBR search_box = geom_a.computeMBR().expand(threshold_m);
        candidates = index.intersects(search_box);
    }

    /// Advance to the next valid pair.  Returns true on success.
    bool advance() {
        if (exhausted) return false;

        while (outer_idx < outer_ptr->size()) {
            // Scan remaining candidates for the current outer element.
            const auto& [key_a, geom_a] = (*outer_ptr)[outer_idx];
            const Coordinate centroid_a = geometryCentroid(geom_a);

            while (cand_idx < candidates.size()) {
                const std::string& key_b = candidates[cand_idx++];
                auto it = inner_key_idx.find(key_b);
                if (it == inner_key_idx.end()) continue;

                const Coordinate centroid_b =
                    geometryCentroid((*inner_ptr)[it->second].second);
                const double dist = haversineDistanceM(centroid_a.x, centroid_a.y,
                                                       centroid_b.x, centroid_b.y);
                if (dist <= threshold_m) {
                    current_pair = {key_a, key_b, dist};
                    ++pairs_yielded;
                    if (pairs_yielded >= config.max_pairs) {
                        THEMIS_WARN("spatialJoin (iterator): max_pairs limit ({}) reached; "
                                    "result set may be incomplete", config.max_pairs);
                        exhausted = true;
                    }
                    return true;
                }
            }

            // Move to next outer element.
            ++outer_idx;
            if (outer_idx < outer_ptr->size()) {
                loadCandidates();
            } else {
                exhausted = true;
            }
        }

        exhausted = true;
        return false;
    }
};

SpatialJoinIterator::SpatialJoinIterator(
    const std::vector<std::pair<std::string, GeometryInfo>>& outer,
    const std::vector<std::pair<std::string, GeometryInfo>>& inner,
    double threshold_m,
    const SpatialJoinConfig& config)
    : impl_(std::make_unique<Impl>(outer, inner, threshold_m, config))
{
    if (threshold_m <= 0.0) {
        THEMIS_WARN("SpatialJoinIterator: threshold_m ({}) must be positive; "
                    "iterator will yield no results", threshold_m);
    }
}

SpatialJoinIterator::~SpatialJoinIterator() = default;
SpatialJoinIterator::SpatialJoinIterator(SpatialJoinIterator&&) noexcept = default;
SpatialJoinIterator& SpatialJoinIterator::operator=(SpatialJoinIterator&&) noexcept = default;

bool SpatialJoinIterator::advance() {
    return impl_->advance();
}

const SpatialJoinPair& SpatialJoinIterator::current() const {
    return impl_->current_pair;
}

bool SpatialJoinIterator::done() const {
    return impl_->exhausted;
}

} // namespace geo
} // namespace themis
