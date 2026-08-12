/**
 * @file temporal_spatial_query.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/geo/ewkb.h"
#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace geo {

/**
 * TemporalSpatialQuery
 *
 * Bridges the temporal versioning system with geospatial queries.
 * Answers questions of the form "where was entity X at time T?" or
 * "which entities were inside region R at time T?".
 *
 * Geometry is read from a named field inside each VersionedDocument's data
 * JSON object.  The field value must be a GeoJSON geometry string (e.g.
 * `{"type":"Point","coordinates":[13.4,52.5]}`).
 *
 * Thread-safety: all methods are const / stateless and thread-safe as long
 * as the SystemVersionedTable passed by const-reference is stable for the
 * duration of the call.
 */
class TemporalSpatialQuery {
public:
    /// Default document field that holds a GeoJSON geometry string.
    static constexpr const char* kDefaultGeoField = "location";

    /**
     * Return the geometry of a single entity as it was at time @p as_of.
     *
     * @param table      System-versioned table.
     * @param key        Entity key.
     * @param as_of      Point-in-time timestamp (ms since epoch).
     * @param geo_field  Name of the JSON field containing the GeoJSON string.
     * @return           Parsed GeometryInfo, or empty optional when the key
     *                   did not exist at time T or the field is absent/invalid.
     */
    static std::optional<GeometryInfo> locationAtTime(
        const themisdb::temporal::SystemVersionedTable& table,
        const std::string& key,
        themisdb::temporal::Timestamp as_of,
        const std::string& geo_field = kDefaultGeoField);

    /**
     * Return (key, geometry) pairs for every entity that was alive at time T
     * and had a parseable geometry in @p geo_field.
     *
     * @param table      System-versioned table.
     * @param as_of      Point-in-time timestamp (ms since epoch).
     * @param geo_field  Name of the JSON field containing the GeoJSON string.
     * @return           Vector of (key, geometry) pairs.
     */
    static std::vector<std::pair<std::string, GeometryInfo>> allLocationsAtTime(
        const themisdb::temporal::SystemVersionedTable& table,
        themisdb::temporal::Timestamp as_of,
        const std::string& geo_field = kDefaultGeoField);

    /**
     * Return all entities whose geometry centroid was inside the given axis-
     * aligned bounding box at time T.
     *
     * @param table      System-versioned table.
     * @param bbox       Bounding box filter (WGS-84 lon/lat degrees).
     * @param as_of      Point-in-time timestamp (ms since epoch).
     * @param geo_field  Name of the JSON field containing the GeoJSON string.
     * @return           Matching VersionedDocuments.
     */
    static std::vector<themisdb::temporal::VersionedDocument> entitiesInBBoxAtTime(
        const themisdb::temporal::SystemVersionedTable& table,
        const MBR& bbox,
        themisdb::temporal::Timestamp as_of,
        const std::string& geo_field = kDefaultGeoField);

    /**
     * Return all entities whose geometry centroid was within @p distance_m
     * metres of (@p center_lon, @p center_lat) at time T.
     *
     * Uses the Haversine formula (spherical earth approximation).
     *
     * @param table       System-versioned table.
     * @param center_lon  Center longitude in decimal degrees (WGS-84).
     * @param center_lat  Center latitude in decimal degrees (WGS-84).
     * @param distance_m  Radius threshold in metres (must be > 0).
     * @param as_of       Point-in-time timestamp (ms since epoch).
     * @param geo_field   Name of the JSON field containing the GeoJSON string.
     * @return            Matching VersionedDocuments (unordered).
     */
    static std::vector<themisdb::temporal::VersionedDocument> entitiesWithinDistanceAtTime(
        const themisdb::temporal::SystemVersionedTable& table,
        double center_lon,
        double center_lat,
        double distance_m,
        themisdb::temporal::Timestamp as_of,
        const std::string& geo_field = kDefaultGeoField);

    /**
     * Same as entitiesWithinDistanceAtTime() but returns (document, distance_m)
     * pairs sorted ascending by distance from the center.
     */
    static std::vector<std::pair<themisdb::temporal::VersionedDocument, double>>
    entitiesWithinDistanceAtTimeSorted(
        const themisdb::temporal::SystemVersionedTable& table,
        double center_lon,
        double center_lat,
        double distance_m,
        themisdb::temporal::Timestamp as_of,
        const std::string& geo_field = kDefaultGeoField);

    /**
     * Extract and parse the geometry stored in @p geo_field of @p doc.
     *
     * The field value may be:
     *   - A JSON string containing a GeoJSON geometry object, or
     *   - A JSON object (GeoJSON geometry directly embedded).
     *
     * @return Parsed GeometryInfo, or empty optional when the field is missing
     *         or the geometry string is invalid.
     */
    static std::optional<GeometryInfo> extractGeometry(
        const themisdb::temporal::VersionedDocument& doc,
        const std::string& geo_field = kDefaultGeoField);
};

} // namespace geo
} // namespace themis
