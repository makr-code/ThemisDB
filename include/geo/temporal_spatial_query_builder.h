#pragma once

/**
 * @file temporal_spatial_query_builder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "geo/geo_json_geometry.h"
#include "geo/spatial_join_filter.h"
#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"
#include "utils/geo/ewkb.h"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// TimeWindowType
// ---------------------------------------------------------------------------

enum class TimeWindowType {
    POINT_IN_TIME,
    INTERVAL,
    SLIDING_WINDOW,
};

// ---------------------------------------------------------------------------
// BuiltTemporalSpatialQuery — immutable value type
// ---------------------------------------------------------------------------

class BuiltTemporalSpatialQuery {
public:
    enum class SpatialType { BBOX, PREDICATE };

    struct TemporalConstraint {
        TimeWindowType window_type;
        themisdb::temporal::Timestamp point_in_time{0};
        themisdb::temporal::Timestamp interval_start{0};
        themisdb::temporal::Timestamp interval_end{themisdb::temporal::kMaxTimestamp};
        int64_t sliding_window_ms{0};
    };

    struct SpatialConstraint {
        SpatialType type{SpatialType::BBOX};
        MBR         bbox;
        std::shared_ptr<ISpatialJoinFilter> predicate; ///< Used when type == PREDICATE
    };

    BuiltTemporalSpatialQuery(TemporalConstraint temporal,
                               SpatialConstraint  spatial,
                               std::string        geo_field)
        : temporal_(std::move(temporal))
        , spatial_(std::move(spatial))
        , geo_field_(std::move(geo_field)) {}

    // ---- Accessors (immutable after construction) ----

    [[nodiscard]] TimeWindowType windowType() const noexcept {
        return temporal_.window_type;
    }

    [[nodiscard]] const MBR& bbox() const noexcept { return spatial_.bbox; }

    [[nodiscard]] const std::string& geoField() const noexcept {
        return geo_field_;
    }

    [[nodiscard]] std::vector<std::pair<std::string, GeometryInfo>> execute(
        const themisdb::temporal::SystemVersionedTable& table) const;

private:
    TemporalConstraint temporal_;
    SpatialConstraint  spatial_;
    std::string        geo_field_;
};

// ---------------------------------------------------------------------------
// ITemporalSpatialQueryBuilder — abstract builder interface
// ---------------------------------------------------------------------------

class ITemporalSpatialQueryBuilder {
public:
    /**
     * @brief ITemporal Spatial Query Builder.
     * @return Return value.
     */
    virtual ~ITemporalSpatialQueryBuilder() = default;

    [[nodiscard]] virtual ITemporalSpatialQueryBuilder& withinBBox(const MBR& bbox) = 0;

    /**
     * @brief With Predicate.
     * @param[in] predicate Input parameter.
     * @return Return value.
     */
    virtual ITemporalSpatialQueryBuilder& withPredicate(
        std::shared_ptr<ISpatialJoinFilter> predicate) = 0;

    /**
     * @brief During Interval.
     * @param[in] start Input parameter.
     * @param[in] end Input parameter.
     * @return Return value.
     */
    virtual ITemporalSpatialQueryBuilder& duringInterval(
        themisdb::temporal::Timestamp start,
        themisdb::temporal::Timestamp end) = 0;

    /**
     * @brief At Time.
     * @param[in] t Input parameter.
     * @return Return value.
     */
    virtual ITemporalSpatialQueryBuilder& atTime(
        themisdb::temporal::Timestamp t) = 0;

    [[nodiscard]] virtual ITemporalSpatialQueryBuilder& slidingWindow(int64_t width_ms) = 0;

    [[nodiscard]] virtual ITemporalSpatialQueryBuilder& withGeoField(const std::string& field) = 0;

    [[nodiscard]] virtual BuiltTemporalSpatialQuery build() = 0;
};

// ---------------------------------------------------------------------------
// TemporalSpatialQueryBuilder — concrete implementation
// ---------------------------------------------------------------------------

class TemporalSpatialQueryBuilder final : public ITemporalSpatialQueryBuilder {
public:
    TemporalSpatialQueryBuilder() = default;

    TemporalSpatialQueryBuilder& withinBBox(const MBR& bbox) override;
    TemporalSpatialQueryBuilder& withPredicate(
        std::shared_ptr<ISpatialJoinFilter> predicate) override;
    TemporalSpatialQueryBuilder& duringInterval(
        themisdb::temporal::Timestamp start,
        themisdb::temporal::Timestamp end) override;
    TemporalSpatialQueryBuilder& atTime(themisdb::temporal::Timestamp t) override;
    TemporalSpatialQueryBuilder& slidingWindow(int64_t width_ms) override;
    TemporalSpatialQueryBuilder& withGeoField(const std::string& field) override;

    [[nodiscard]] BuiltTemporalSpatialQuery build() override;

    /**
     * @brief Reset the modification detection flag.
     * @return None.
     */
    TemporalSpatialQueryBuilder& reset();

private:
    std::optional<BuiltTemporalSpatialQuery::TemporalConstraint> temporal_;
    std::optional<BuiltTemporalSpatialQuery::SpatialConstraint>  spatial_;
    std::string geo_field_{"location"};
};

} // namespace geo
} // namespace themis
