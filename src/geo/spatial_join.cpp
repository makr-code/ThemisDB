/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_join.cpp                                   ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:58:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     144                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 48e1d07b1  2026-02-25  fix(geo): audit cleanup — remove unused include, update s... ║
    • a6b9a1ca8  2026-02-24  feat(geo): implement spatial JOIN for nearby point pairs ║
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

} // namespace geo
} // namespace themis
