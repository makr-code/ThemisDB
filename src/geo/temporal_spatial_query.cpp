/**
 * @file temporal_spatial_query.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "geo/temporal_spatial_query.h"
#include "geo/geo_math.h"
#include "geo/geo_rtree.h"
#include "geo/spatial_join.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Extract the representative point (lon, lat) for a geometry.
/// For Point geometries the single coordinate is returned; for all other
/// types the centroid is used.
static Coordinate geometryCentroidTSQ(const GeometryInfo& geom) {
    if (geom.isPoint() && !geom.coords.empty()) {
        return geom.coords[0];
    }
    return geom.computeCentroid();
}

// ---------------------------------------------------------------------------
// TemporalSpatialQuery implementation
// ---------------------------------------------------------------------------

std::optional<GeometryInfo> TemporalSpatialQuery::extractGeometry(
    const themisdb::temporal::VersionedDocument& doc,
    const std::string& geo_field)
{
    auto it = doc.data.find(geo_field);
    if (it == doc.data.end()) {
        return std::nullopt;
    }

    try {
        std::string geojson_str = {};
        if (it->is_string()) {
            geojson_str = it->get<std::string>();
        } else {
            // Embedded JSON object — serialise it back to string for the parser.
            geojson_str = it->dump();
        }
        GeometryInfo geom = EWKBParser::parseGeoJSON(geojson_str);
        // An empty geometry (no coords and no sub-geometries) is treated as
        // parse failure.
        if (geom.coords.empty() && geom.rings.empty() && geom.geometries.empty()) {
            return std::nullopt;
        }
        return geom;
    } catch (const std::exception& ex) {
        THEMIS_WARN("TemporalSpatialQuery::extractGeometry: failed to parse "
                    "field '{}': {}", geo_field, ex.what());
        return std::nullopt;
    }
}

std::optional<GeometryInfo> TemporalSpatialQuery::locationAtTime(
    const themisdb::temporal::SystemVersionedTable& table,
    const std::string& key,
    themisdb::temporal::Timestamp as_of,
    const std::string& geo_field)
{
    auto version = table.getAsOf(key, as_of);
    if (!version.has_value()) {
        return std::nullopt;
    }
    return extractGeometry(*version, geo_field);
}

std::vector<std::pair<std::string, GeometryInfo>>
TemporalSpatialQuery::allLocationsAtTime(
    const themisdb::temporal::SystemVersionedTable& table,
    themisdb::temporal::Timestamp as_of,
    const std::string& geo_field)
{
    auto rows = table.scan(as_of);
    std::vector<std::pair<std::string, GeometryInfo>> result;
    result.reserve(rows.size());
    for (const auto& row : rows) {
        auto geom = extractGeometry(row, geo_field);
        if (geom.has_value()) {
            result.emplace_back(row.key, std::move(*geom));
        }
    }
    return result;
}

std::vector<themisdb::temporal::VersionedDocument>
TemporalSpatialQuery::entitiesInBBoxAtTime(
    const themisdb::temporal::SystemVersionedTable& table,
    const MBR& bbox,
    themisdb::temporal::Timestamp as_of,
    const std::string& geo_field)
{
    auto rows = table.scan(as_of);
    std::vector<themisdb::temporal::VersionedDocument> result = {};

    for (auto& row : rows) {
        auto geom = extractGeometry(row, geo_field);
        if (!geom.has_value()) {
            continue;
        }
        const Coordinate centroid = geometryCentroidTSQ(*geom);
        if (bbox.contains(centroid.x, centroid.y)) {
            result.push_back(std::move(row));
        }
    }
    return result;
}

std::vector<themisdb::temporal::VersionedDocument>
TemporalSpatialQuery::entitiesWithinDistanceAtTime(
    const themisdb::temporal::SystemVersionedTable& table,
    double center_lon,
    double center_lat,
    double distance_m,
    themisdb::temporal::Timestamp as_of,
    const std::string& geo_field)
{
    if (distance_m <= 0.0) {
        THEMIS_WARN("TemporalSpatialQuery::entitiesWithinDistanceAtTime: "
                    "distance_m ({}) must be positive; returning empty result",
                    distance_m);
        return {};
    }

    auto rows = table.scan(as_of);

    // For large snapshots, build an inline R-Tree to reduce the candidate set
    // from O(n) to O(log n + k) before the exact Haversine pass.
    // The threshold is set conservatively: below it the R-Tree overhead
    // (build + query) would outweigh the benefit over a linear scan.
    static constexpr std::size_t kIndexThreshold = 512;

    if (static_cast<int>(rows.size()) >= kIndexThreshold) {
        // Build (key → row index) map and a (key → centroid) list for bulk load.
        std::unordered_map<std::string, std::size_t> key_idx = {};

        key_idx.reserve(rows.size());
        std::vector<std::pair<std::string, GeometryInfo>> geo_entries;
        geo_entries.reserve(rows.size());

        for (std::size_t i = 0; i <static_cast<int>(rows.size()); ++i) {
            auto geom = extractGeometry(rows[i], geo_field);
            if (!geom.has_value()) {
                continue;
            }
            // Use a synthetic centroid-point geometry as the index entry.
            const Coordinate centroid = geometryCentroidTSQ(*geom);
            GeometryInfo pt(GeometryType::Point);
            pt.coords.push_back(centroid);
            geo_entries.emplace_back(rows[i].key, std::move(pt));
            key_idx.emplace(rows[i].key, i);
        }

        if (!geo_entries.empty()) {
            GeoRTree snapshot_idx;
            snapshot_idx.bulkLoad(geo_entries);

            // Expand search box by distance_m → approximate degree radius.
            static constexpr double kMetersPerDeg = 111320.0;
            const double deg_lat = distance_m / kMetersPerDeg;
            const double cos_lat = std::cos(center_lat * 3.14159265358979323846 / 180.0);
            const double deg_lon = (cos_lat > 1e-6)
                                       ? distance_m / (kMetersPerDeg * cos_lat)
                                       : 180.0;

            MBR search_box;
            search_box.minx = center_lon - deg_lon;
            search_box.miny = center_lat - deg_lat;
            search_box.maxx = center_lon + deg_lon;
            search_box.maxy = center_lat + deg_lat;

            // Anti-meridian safety: when the radius crosses ±180° the MBR
            // would wrap and exclude valid candidates.  Expand to the full
            // longitude range instead; false positives are filtered by the
            // exact Haversine check below.
            if (search_box.minx < -180.0 || search_box.maxx > 180.0) {
                search_box.minx = -180.0;
                search_box.maxx = 180.0;
            }

            // Clamp latitude range to valid WGS-84 limits.
            search_box.miny = std::max(search_box.miny, -90.0);
            search_box.maxy = std::min(search_box.maxy,  90.0);

            const auto candidate_keys = snapshot_idx.intersects(search_box);

            std::vector<themisdb::temporal::VersionedDocument> result = {};

            for (const auto& key : candidate_keys) {
                auto it = key_idx.find(key);
                if (it == key_idx.end()) {
                    continue;
                }
                // Exact Haversine check on the centroid.
                auto geom = extractGeometry(rows[it->second], geo_field);
                if (!geom.has_value()) {
                    continue;
                }
                const Coordinate centroid = geometryCentroidTSQ(*geom);
                if (haversineDistanceM(center_lon, center_lat,
                                       centroid.x, centroid.y) <= distance_m) {
                    result.push_back(rows[it->second]);
                }
            }
            return result;
        }
    }

    // Fallback: linear scan (used when row count < kIndexThreshold or
    // when all geometry fields are unparseable).
    std::vector<themisdb::temporal::VersionedDocument> result = {};

    for (auto& row : rows) {
        auto geom = extractGeometry(row, geo_field);
        if (!geom.has_value()) {
            continue;
        }
        const Coordinate centroid = geometryCentroidTSQ(*geom);
        const double dist = haversineDistanceM(
            center_lon, center_lat, centroid.x, centroid.y);
        if (dist <= distance_m) {
            result.push_back(std::move(row));
        }
    }
    return result;
}

std::vector<std::pair<themisdb::temporal::VersionedDocument, double>>
TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted(
    const themisdb::temporal::SystemVersionedTable& table,
    double center_lon,
    double center_lat,
    double distance_m,
    themisdb::temporal::Timestamp as_of,
    const std::string& geo_field)
{
    if (distance_m <= 0.0) {
        THEMIS_WARN("TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted: "
                    "distance_m ({}) must be positive; returning empty result",
                    distance_m);
        return {};
    }

    auto rows = table.scan(as_of);
    std::vector<std::pair<themisdb::temporal::VersionedDocument, double>> result;
    for (auto& row : rows) {
        auto geom = extractGeometry(row, geo_field);
        if (!geom.has_value()) {
            continue;
        }
        const Coordinate centroid = geometryCentroidTSQ(*geom);
        const double dist = haversineDistanceM(
            center_lon, center_lat, centroid.x, centroid.y);
        if (dist <= distance_m) {
            result.emplace_back(std::move(row), dist);
        }
    }
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    return result;
}

} // namespace geo
} // namespace themis
