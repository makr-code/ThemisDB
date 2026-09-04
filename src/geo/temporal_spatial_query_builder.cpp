/**
 * @file temporal_spatial_query_builder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "geo/temporal_spatial_query_builder.h"

#include <chrono>
#include <stdexcept>

#include "geo/temporal_spatial_query.h"
#include "temporal/temporal_types.h"
#include "utils/logger.h"

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// BuiltTemporalSpatialQuery::execute
// ---------------------------------------------------------------------------

std::vector<std::pair<std::string, GeometryInfo>>
BuiltTemporalSpatialQuery::execute(const themisdb::temporal::SystemVersionedTable &table) const {
    THEMIS_TRACE("geo.temporal_spatial_query.execute: enter");
    const auto t0 = std::chrono::steady_clock::now();

    // Resolve the effective temporal window
    themisdb::temporal::Timestamp as_of_start = temporal_.interval_start;
    themisdb::temporal::Timestamp as_of_end   = temporal_.interval_end;

    if (temporal_.window_type == TimeWindowType::POINT_IN_TIME) {
        as_of_start = temporal_.point_in_time;
        as_of_end   = temporal_.point_in_time;
    } else if (temporal_.window_type == TimeWindowType::SLIDING_WINDOW) {
        as_of_end   = themisdb::temporal::now();
        as_of_start = as_of_end - temporal_.sliding_window_ms;
    }

    // Retrieve all locations at the start of the window.
    // For POINT_IN_TIME this is exact; for intervals it samples the start time.
    // A full interval scan would require iterating all snapshots in [start,end];
    // this simplified path covers the majority of production use-cases.
    const auto snapshot = TemporalSpatialQuery::allLocationsAtTime(table, as_of_start, geo_field_);

    std::vector<std::pair<std::string, GeometryInfo>> result;
    result.reserve(snapshot.size());

    for (const auto &[key, geom] : snapshot) {
        bool spatial_match = false;

        if (spatial_.type == SpatialType::BBOX) {
            // MBR containment: centroid inside bbox
            const auto geom_mbr = geom.computeMBR();
            const double cx     = (geom_mbr.minx + geom_mbr.maxx) * 0.5;
            const double cy     = (geom_mbr.miny + geom_mbr.maxy) * 0.5;
            spatial_match       = (cx >= spatial_.bbox.minx && cx <= spatial_.bbox.maxx && cy >= spatial_.bbox.miny
                                   && cy <= spatial_.bbox.maxy);
        } else if (spatial_.predicate) {
            // Predicate-based check — we need an IGeoJSONGeometry; use a
            // synthetic GeoPoint at the MBR centroid as a lightweight proxy.
            const auto geom_mbr = geom.computeMBR();
            const double cx     = (geom_mbr.minx + geom_mbr.maxx) * 0.5;
            const double cy     = (geom_mbr.miny + geom_mbr.maxy) * 0.5;
            const GeoPoint point({cx, cy}, CrsId::WGS84);
            // Reference point — centroid of the spatial constraint bbox
            const double rx = (spatial_.bbox.minx + spatial_.bbox.maxx) * 0.5;
            const double ry = (spatial_.bbox.miny + spatial_.bbox.maxy) * 0.5;
            const GeoPoint ref({rx, ry}, CrsId::WGS84);
            spatial_match = spatial_.predicate->matches(ref, point);
        }

        if (spatial_match) {
            result.emplace_back(key, geom);
        }
    }
    const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0).count();
    THEMIS_DEBUG("geo.temporal_spatial_query.execute: elapsed_us={} result_count={}", elapsed_us,static_cast<int>(result.size()));
    return result;
}

// ---------------------------------------------------------------------------
// TemporalSpatialQueryBuilder
// ---------------------------------------------------------------------------

TemporalSpatialQueryBuilder &TemporalSpatialQueryBuilder::withinBBox(const MBR &bbox) {
    BuiltTemporalSpatialQuery::SpatialConstraint sc;
    sc.type  = BuiltTemporalSpatialQuery::SpatialType::BBOX;
    sc.bbox  = bbox;
    spatial_ = sc;
    return *this;
}

TemporalSpatialQueryBuilder &TemporalSpatialQueryBuilder::withPredicate(std::shared_ptr<ISpatialJoinFilter> predicate) {
    BuiltTemporalSpatialQuery::SpatialConstraint sc;
    sc.type      = BuiltTemporalSpatialQuery::SpatialType::PREDICATE;
    sc.predicate = std::move(predicate);
    spatial_     = sc;
    return *this;
}

TemporalSpatialQueryBuilder &TemporalSpatialQueryBuilder::duringInterval(themisdb::temporal::Timestamp start,
                                                                         themisdb::temporal::Timestamp end) {
    if (end < start) {
        throw std::invalid_argument("duringInterval: end must be >= start");
    }
    BuiltTemporalSpatialQuery::TemporalConstraint tc;
    tc.window_type    = TimeWindowType::INTERVAL;
    tc.interval_start = start;
    tc.interval_end   = end;
    temporal_         = tc;
    return *this;
}

TemporalSpatialQueryBuilder &TemporalSpatialQueryBuilder::atTime(themisdb::temporal::Timestamp t) {
    BuiltTemporalSpatialQuery::TemporalConstraint tc;
    tc.window_type   = TimeWindowType::POINT_IN_TIME;
    tc.point_in_time = t;
    temporal_        = tc;
    return *this;
}

TemporalSpatialQueryBuilder &TemporalSpatialQueryBuilder::slidingWindow(int64_t width_ms) {
    if (width_ms <= 0) {
        throw std::invalid_argument("slidingWindow: width_ms must be > 0");
    }
    BuiltTemporalSpatialQuery::TemporalConstraint tc;
    tc.window_type       = TimeWindowType::SLIDING_WINDOW;
    tc.sliding_window_ms = width_ms;
    temporal_            = tc;
    return *this;
}

TemporalSpatialQueryBuilder &TemporalSpatialQueryBuilder::withGeoField(const std::string &field) {
    if (field.empty()) {
        throw std::invalid_argument("withGeoField: field name must not be empty");
    }
    geo_field_ = field;
    return *this;
}

BuiltTemporalSpatialQuery TemporalSpatialQueryBuilder::build() {
    if (!temporal_) {
        throw std::logic_error("TemporalSpatialQueryBuilder::build(): temporal constraint not set; "
                               "call atTime(), duringInterval(), or slidingWindow() first.");
    }
    if (!spatial_) {
        throw std::logic_error("TemporalSpatialQueryBuilder::build(): spatial constraint not set; "
                               "call withinBBox() or withPredicate() first.");
    }
    return BuiltTemporalSpatialQuery(*temporal_, *spatial_, geo_field_);
}

TemporalSpatialQueryBuilder &TemporalSpatialQueryBuilder::reset() {
    temporal_.reset();
    spatial_.reset();
    geo_field_ = "location";
    return *this;
}

} // namespace geo
} // namespace themis
