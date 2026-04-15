/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_spatial_query.cpp                         ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:07:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     203                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "geo/temporal_spatial_query.h"
#include "geo/spatial_join.h"
#include "utils/logger.h"

#include <algorithm>

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
        std::string geojson_str;
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
    std::vector<themisdb::temporal::VersionedDocument> result;
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
    std::vector<themisdb::temporal::VersionedDocument> result;
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
